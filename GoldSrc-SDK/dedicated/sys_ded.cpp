#include "sys_ded.h"
#include "CreateMultiplayerGameServerPage.h"
#include "console/conproc.h"
#include "../common/icommandline.h"
#include "../common/IAdminServer.h"
#include "../common/IManageServer.h"
#include "../common/TextConsoleWin32.h"
#include "../public/FileSystem.h"
#include "../public/idedicatedexports.h"
#include "../GameUI/common/IVGuiModule.h"
#include "../vgui2/controls/Controls.h"
#include "../vgui2/controls/ProgressBox.h"
#include "../engine/controls.h"
#include "vgui/Cursor.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"

#include <assert.h>
#include <ctype.h>
#include <direct.h>
#include <mmsystem.h>
#include <process.h>
#include <shellapi.h>
#include <sys/stat.h>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if defined( NDEBUG )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* expression, const wchar_t* filename,
	unsigned int line );
#endif

class IDedicatedServerAPI : public IBaseInterface
{
public:
	virtual bool Init( char* basedir, char* command, CreateInterfaceFn launcherFactory,
		CreateInterfaceFn filesystemFactory ) = 0;
	virtual int Shutdown() = 0;
	virtual bool RunFrame() = 0;
	virtual void AddConsoleText( char* text ) = 0;
	virtual void UpdateStatus( float* fps, int* activePlayers, int* maxPlayers, char* map ) = 0;
};

class CDedicatedExports : public IDedicatedExports
{
public:
	void Sys_Printf( char* text ) override;
};

typedef void ( *SleepType )( int milliseconds );

static IDedicatedServerAPI*	engineAPI;
static CTextConsoleWin32		console;
static CMainPanel*			g_pMainPanel;
static CMainPanel*			g_pMainPanelInstance;
static CSysModule*			vguiModule;
static CSysModule*			filesystemModule;
static CSysModule*			engineModule;
static IAdminServer*			g_pAdminServer;
static IVGuiModule*			g_pAdminVGuiModule;
static HMODULE				hDLLThirdParty;
static bool					g_bVGUI;
static int					gbAppHasBeenTerminated;
static char*				gpszCvars;
static CreateInterfaceFn		filesystemFactory;
static IFileSystem*			pFileSystem;
static SleepType			Sys_Sleep;
static double				g_flLastStatusUpdate;
static const char*			g_pszEngine = "swds.dll";

vgui2::MessageMapItem_t CMainPanel::m_MessageMap[1] =
{
	{ "Quit", (vgui2::MessageFunc_t)&CMainPanel::Shutdown, 0, vgui2::DATATYPE_VOID, NULL,
		vgui2::DATATYPE_VOID, NULL, 0, 0, 0 }
};

vgui2::PanelMap_t CMainPanel::m_PanelMap =
{
	m_MessageMap, 1, "CMainPanel", &vgui2::Panel::m_PanelMap, 0
};

#pragma warning( push )
#pragma warning( disable: 26495 )
CMainPanel::CMainPanel( CreateInterfaceFn*, int )
	: Panel( NULL, "CMainPanel" )
{
	m_bStarting = false;
	m_bStarted = false;
	m_bQuit = false;
	m_bWaiting = true;
	SetPaintBackgroundEnabled( false );
	SetFgColor( Color( 0, 0, 0, 0 ) );
	m_bStarting = false;
	m_flProgress = 0.0f;
}
#pragma warning( pop )

CMainPanel::~CMainPanel()
{
}

void CMainPanel::Printf( char* text )
{
	if ( m_pManageServer )
		m_pManageServer->AddToConsole( text );
}

vgui2::PanelMap_t* CMainPanel::GetPanelMap()
{
	return &m_PanelMap;
}

CMainPanel* MainPanel()
{
	return g_pMainPanelInstance;
}

void CMainPanel::Initialize( CreateInterfaceFn*, int )
{
	g_pMainPanelInstance = this;
	m_pManageServer = NULL;
	m_bStarted = false;
	m_bQuit = false;
	m_hProgress = NULL;
	m_hEvent = NULL;
	MoveToFront();
	m_pConfig = new CCreateMultiplayerGameServerPage( this, "Config" );
}

void CMainPanel::ShowConfig()
{
	m_pConfig->SetVisible( true );
	m_pConfig->MoveToFront();
}

void CMainPanel::Shutdown()
{
	Stop();
}

void CMainPanel::Stop()
{
	vgui2::surface()->SetCursor( vgui2::dc_user );
	m_bStarted = false;
	m_bQuit = true;
	if ( m_hProgress.Get() )
	{
		m_hProgress->Close();
		m_hProgress = NULL;
	}
	vgui2::ivgui()->Stop();
}

void CMainPanel::StartServer( char* command )
{
	vgui2::surface()->SetCursor( vgui2::dc_hourglass );
	m_pConfig->GetServerInfo( &m_Server );
	m_pConfig->SetVisible( false );
	m_pConfig->Close();
	gpszCvars = (char*)malloc( strlen( command ) + 1 );
#pragma warning( suppress: 6387 )
	strcpy( gpszCvars, command );
	m_hProgress = new vgui2::ProgressBox( "#Start_Server_Loading_Title", "#Server_UpdatingSteamResources",
		"Starting dedicated server...", NULL );
	m_hProgress->SetCancelButtonVisible( true );
	m_hProgress->ShowWindow( NULL );

	char resourceList[MAX_PATH];
	_snprintf( resourceList, sizeof( resourceList ), "reslists/%s/dedicated_preloads.lst",
		m_pConfig->m_szMod + CCreateMultiplayerGameServerPage::GAME_DIR_LENGTH );
	m_hResourceWait = vgui2::filesystem()->WaitForResources( resourceList );
	if ( !m_hResourceWait )
	{
		m_bStarting = true;
		m_bWaiting = false;
		m_hProgress->SetCancelButtonEnabled( false );
	}
	m_hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
	vgui2::ivgui()->AddTickSignal( GetVPanel(), 0 );
}

void CMainPanel::OnTick()
{
	static int retries;
	if ( m_hResourceWait )
	{
		if ( !m_hProgress.Get() || !m_hProgress->IsVisible() )
		{
			vgui2::filesystem()->CancelWaitForResources( m_hResourceWait );
			m_hResourceWait = 0;
			Stop();
			return;
		}

		float fraction;
		bool complete;
		if ( vgui2::filesystem()->GetWaitForResourcesProgress( m_hResourceWait, &fraction, &complete )
			&& ( fraction != 1.0f || m_flProgress != 0.0f ) )
		{
			m_hProgress->SetProgress( fraction );
			m_flProgress = fraction;
		}
		if ( complete )
		{
			m_hResourceWait = 0;
			m_bStarting = true;
			m_bWaiting = false;
			m_hProgress->SetCancelButtonEnabled( false );
		}
	}

	if ( !m_bStarting )
		return;
	if ( WaitForSingleObject( m_hEvent, 10 ) != WAIT_OBJECT_0 && retries <= 1000 )
	{
		++retries;
		return;
	}
	if ( m_bStarted )
	{
		Stop();
	}
	else
	{
		serveritem_t server;
		m_pConfig->GetServerInfo( &server );
		ManageServerUIHandle_t handle = g_pAdminServer->OpenManageServerDialog( server.name, server.gameDir );
		m_pManageServer = g_pAdminServer->GetManageServerInterface( handle );
		m_bStarted = true;
		if ( m_hProgress.Get() )
		{
			m_hProgress->Close();
			m_hProgress = NULL;
		}
	}
	vgui2::surface()->SetCursor( vgui2::dc_user );
	m_bStarting = false;
	ResetEvent( m_hEvent );
}

void Sys_Printf( char* format, ... )
{
	char text[1024];
	va_list arguments;
	va_start( arguments, format );
	_vsnprintf( text, sizeof( text ), format, arguments );
	va_end( arguments );
	if ( g_bVGUI )
	{
		if ( g_pMainPanel )
			g_pMainPanel->Printf( text );
	}
	else
	{
		console.Print( text );
	}
}

void CDedicatedExports::Sys_Printf( char* text )
{
	::Sys_Printf( "%s", text );
}

EXPOSE_SINGLE_INTERFACE( CDedicatedExports, IDedicatedExports, VENGINE_DEDICATEDEXPORTS_API_VERSION );

static void Sys_SleepMilliseconds( int milliseconds )
{
	Sleep( milliseconds );
}

static BOOL __cdecl CtrlHandler( DWORD type )
{
	switch ( type )
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		gbAppHasBeenTerminated = true;
		return TRUE;
	default:
		return FALSE;
	}
}

int RunServer()
{
	int result = 0;
	if ( g_bVGUI )
		vgui2::ivgui()->SetSleep( false );
	while ( !gbAppHasBeenTerminated )
	{
		CreateInterfaceFn engineFactory = Sys_GetFactory( engineModule );
		if ( g_bVGUI )
			vgui2::ivgui()->RunFrame();
		if ( engineFactory )
			engineAPI = (IDedicatedServerAPI*)engineFactory( "VENGINE_HLDS_API_VERSION002", NULL );
		if ( g_bVGUI )
			vgui2::ivgui()->RunFrame();
		if ( !engineAPI || !engineAPI->Init( ".", (char*)cmdline->GetCmdLine(), Sys_GetFactoryThis(), filesystemFactory ) )
			return -1;

		if ( g_bVGUI )
			vgui2::ivgui()->RunFrame();
		if ( g_bVGUI )
		{
			vgui2::filesystem()->AddSearchPath( "platform", "PLATFORM" );
			char configPath[512];
			const char* installPath = getenv( "SteamInstallPath" );
			if ( installPath )
				_snprintf( configPath, sizeof( configPath ), "%s/config", installPath );
			else
				strcpy( configPath, "platform/config" );
			vgui2::filesystem()->AddSearchPath( configPath, "CONFIG" );
			if ( g_bVGUI )
				vgui2::ivgui()->RunFrame();
		}
		if ( gpszCvars )
			engineAPI->AddConsoleText( gpszCvars );
		if ( g_pMainPanel )
			SetEvent( g_pMainPanel->m_hEvent );
		if ( g_bVGUI )
			vgui2::ivgui()->RunFrame();

		for ( ;; )
		{
			if ( g_bVGUI )
				vgui2::ivgui()->RunFrame();
			Sys_Sleep( 1 );
			if ( !g_bVGUI )
			{
				MSG message;
				bool quit = false;
				while ( PeekMessageA( &message, NULL, 0, 0, PM_NOREMOVE ) )
				{
					if ( !GetMessageA( &message, NULL, 0, 0 ) )
					{
						quit = true;
						break;
					}
					TranslateMessage( &message );
					DispatchMessageA( &message );
				}
				if ( quit || gbAppHasBeenTerminated )
					break;
				if ( !g_bVGUI && engineAPI )
				{
					char* line = console.GetLine();
					if ( line )
					{
						char command[256];
						_snprintf( command, sizeof( command ), "%s\n", line );
						engineAPI->AddConsoleText( command );
					}
				}
			}
			if ( !engineAPI->RunFrame() )
				break;
			if ( engineAPI )
			{
				float fps;
				int activePlayers;
				int maxPlayers;
				char map[64];
				double now = (double)( (float)timeGetTime() / 1000.0f );
				engineAPI->UpdateStatus( &fps, &activePlayers, &maxPlayers, map );
				if ( now - g_flLastStatusUpdate >= 0.5 )
				{
					g_flLastStatusUpdate = now;
					char status[256];
					_snprintf( status, sizeof( status ), "%.1f fps %2i/%2i on %16s", fps, activePlayers, maxPlayers, map );
					console.SetStatusLine( status );
					console.UpdateStatus();
				}
			}
		}

		if ( g_bVGUI )
		{
			vgui2::ivgui()->RunFrame();
			vgui2::surface()->Shutdown();
			delete g_pMainPanel;
			g_pMainPanel = NULL;
			Sys_UnloadModule( vguiModule );
		}
		else
		{
			FreeConsole();
			DeinitConProc();
			console.ShutDown();
		}
		result = engineAPI->Shutdown();
		Sys_UnloadModule( engineModule );
		if ( g_pMainPanel )
			SetEvent( g_pMainPanel->m_hEvent );
		if ( result == 3 )
			break;
	}
	return result;
}

static int InitVGUI()
{
	vguiModule = Sys_LoadModule( "vgui2.dll" );
	CreateInterfaceFn factories[] =
	{
		Sys_GetFactoryThis(), Sys_GetFactory( vguiModule ), Sys_GetFactory( filesystemModule ), Sys_GetFactory( engineModule )
	};
	if ( !vgui2::VGuiControls_Init( "DEDICATED", factories, 4 ) )
	{
		MessageBoxA( NULL, "Fatal Error: Could not initialize vgui.", "Steam - Fatal Error", MB_ICONERROR );
		return -1;
	}
	vgui2::filesystem()->AddSearchPath( ".", "MAIN" );
	vgui2::filesystem()->AddSearchPath( "platform", "PLATFORM" );
	char configPath[512];
	const char* installPath = getenv( "SteamInstallPath" );
	if ( installPath )
		_snprintf( configPath, sizeof( configPath ), "%s/config", installPath );
	else
		strcpy( configPath, "platform/config" );
#pragma warning( suppress: 6053 )
	(void)_mkdir( configPath );
	vgui2::filesystem()->AddSearchPath( configPath, "CONFIG" );
	vgui2::system()->SetUserConfigFile( "DialogConfig.vdf", "CONFIG" );
	g_pMainPanel = new CMainPanel( factories, 4 );
	g_pMainPanel->SetVisible( true );
	vgui2::surface()->SetEmbeddedPanel( g_pMainPanel->GetVPanel() );
	vgui2::scheme()->LoadSchemeFromFile( "Resource/TrackerScheme.res", NULL );
	vgui2::localize()->AddFile( vgui2::filesystem(), "Resource/platform_%language%.txt" );
	vgui2::localize()->AddFile( vgui2::filesystem(), "Resource/vgui_%language%.txt" );
	vgui2::localize()->AddFile( vgui2::filesystem(), "Admin/server_%language%.txt" );
	vgui2::ivgui()->Start();
	CreateInterfaceFn adminFactory = Sys_GetFactoryThis();
	g_pAdminServer = (IAdminServer*)adminFactory( ADMINSERVER_INTERFACE_VERSION, NULL );
	g_pAdminVGuiModule = (IVGuiModule*)adminFactory( "VGuiModuleAdminServer001", NULL );
	if ( !g_pAdminServer )
		_wassert( L"g_pAdminServer != NULL", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\dedicated\\sys_ded.cpp", 904 );
	if ( !g_pAdminVGuiModule )
		_wassert( L"g_pAdminVGuiModule != NULL", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\dedicated\\sys_ded.cpp", 905 );
	if ( !g_pAdminServer )
		vgui2::ivgui()->DPrintf( "Admin Error: module version (Admin/AdminServer.dll, %s) invalid, not loading\n",
			IMANAGESERVER_INTERFACE_VERSION );
	g_pAdminVGuiModule->Initialize( factories, 4 );
	g_pAdminVGuiModule->PostInitialize( &adminFactory, 1 );
	g_pAdminVGuiModule->SetParent( g_pMainPanel->GetVPanel() );
	g_pMainPanel->Initialize( factories, 4 );
	g_pMainPanel->ShowConfig();
	return 0;
}

int Main( int, wchar_t** )
{
	WSADATA sockets;
	WSAStartup( MAKEWORD( 2, 0 ), &sockets );
	cmdline->CreateCmdLine( GetCommandLineA() );
	engineModule = Sys_LoadModule( g_pszEngine );
	if ( !engineModule )
	{
		MessageBoxA( NULL, "Unable to load engine, image is corrupt.", "Half-Life Dedicated Server Error", 0 );
		return -1;
	}
	Sys_Sleep = Sys_SleepMilliseconds;
	filesystemModule = Sys_LoadModule( "filesystem_stdio.dll" );
	filesystemFactory = Sys_GetFactory( filesystemModule );
	if ( !filesystemFactory )
		return -1;
	pFileSystem = (IFileSystem*)filesystemFactory( "VFileSystem009", NULL );
	if ( !pFileSystem )
		return -1;
	pFileSystem->Mount();
	g_bVGUI = true;
	char* consoleValue = NULL;
	if ( !cmdline->CheckParm( "-steam", NULL ) && cmdline->CheckParm( "-console", &consoleValue ) && consoleValue )
		g_bVGUI = false;
	if ( g_bVGUI )
	{
		InitVGUI();
	}
	else
	{
		if ( !console.Init( NULL ) )
		{
			MessageBoxA( NULL, "Failed to initialize console.", "Half-Life Dedicated Server Error", 0 );
			return 0;
		}
		InitConProc();
		console.SetStatusLineAttrib( FOREGROUND_RED | FOREGROUND_INTENSITY );
#pragma warning( suppress: 4191 )
		if ( !SetConsoleCtrlHandler( (PHANDLER_ROUTINE)CtrlHandler, TRUE ) )
		{
			MessageBoxA( NULL, "Unable to set control handler", "Half-Life Dedicated Server Error", 0 );
			return 0;
		}
	}
	gbAppHasBeenTerminated = false;
	if ( cmdline->CheckParm( "-usegh", NULL ) )
		hDLLThirdParty = LoadLibraryA( "ghostinj.dll" );
	if ( g_bVGUI )
	{
		while ( g_pMainPanel->m_bWaiting && vgui2::ivgui()->IsRunning() )
			vgui2::ivgui()->RunFrame();
	}
	if ( !g_bVGUI || !g_pMainPanel->m_bQuit )
		RunServer();
	if ( gpszCvars )
		free( gpszCvars );
	if ( pFileSystem )
		pFileSystem->Unmount();
	Sys_UnloadModule( filesystemModule );
	if ( hDLLThirdParty )
	{
		FreeLibrary( hDLLThirdParty );
		hDLLThirdParty = NULL;
	}
	WSACleanup();
	return 0;
}

static void LaunchSteam( const char* command )
{
	char executablePath[MAX_PATH];
	GetModuleFileNameA( GetModuleHandleA( NULL ), executablePath, sizeof( executablePath ) );
	char* separator = strrchr( executablePath, '\\' );
	if ( separator )
		*separator = '\0';
	HKEY key;
	if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Valve\\Steam", &key ) == ERROR_SUCCESS )
	{
		RegSetValueExA( key, "TempAppPath", 0, REG_SZ, (const BYTE*)executablePath, (DWORD)strlen( executablePath ) + 1 );
		RegSetValueExA( key, "TempAppCmdLine", 0, REG_SZ, (const BYTE*)command, (DWORD)strlen( command ) + 1 );
		DWORD appID = (DWORD)-1;
		RegSetValueExA( key, "TempAppID", 0, REG_DWORD, (const BYTE*)&appID, sizeof( appID ) );
		RegCloseKey( key );
	}
	HWND steamWindow = FindWindowA( "Valve_SteamIPC_Class", "Hidden Window" );
	if ( steamWindow )
	{
		PostMessageA( steamWindow, WM_USER + 3, 0, 0 );
		return;
	}

	char steamExecutable[MAX_PATH];
	char directory[MAX_PATH];
	steamExecutable[0] = '\0';
	if ( GetCurrentDirectoryA( sizeof( directory ), directory ) )
	{
		separator = strrchr( directory, '\\' );
		while ( separator )
		{
			separator[1] = '\0';
			strcat( directory, "steam_dev.exe" );
			FILE* file = fopen( directory, "rb" );
			if ( file )
			{
				fclose( file );
				strcpy( steamExecutable, directory );
				break;
			}
			separator[1] = '\0';
			strcat( directory, "steam.exe" );
			file = fopen( directory, "rb" );
			if ( file )
			{
				fclose( file );
				strcpy( steamExecutable, directory );
				break;
			}
			*separator = '\0';
			separator = strrchr( directory, '\\' );
		}
	}
	if ( !steamExecutable[0] )
	{
		if ( RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Valve\\Steam", &key ) == ERROR_SUCCESS )
		{
			DWORD length = sizeof( steamExecutable );
			DWORD type;
			RegQueryValueExA( key, "SteamExe", NULL, &type, (BYTE*)steamExecutable, &length );
			RegCloseKey( key );
		}
		if ( !steamExecutable[0] )
		{
			MessageBoxA( NULL, "Error running game: could not find steam.exe to launch", "Fatal Error", MB_ICONERROR );
			return;
		}
	}
	for ( char* cursor = steamExecutable; *cursor; ++cursor )
	{
		if ( *cursor == '/' )
			*cursor = '\\';
	}
	strcpy( directory, steamExecutable );
	separator = strrchr( directory, '\\' );
	if ( separator )
	{
		*separator = '\0';
		(void)_chdir( directory );
	}
	const char* arguments[] = { steamExecutable, "-silent", "-applaunch", NULL };
	_spawnv( _P_NOWAIT, steamExecutable, arguments );
}

static bool CheckForSteam( const char* command, const char* moduleName, const char* fileName )
{
	const char* steam = strstr( command, "-steam" );
	if ( steam && ( !steam[6] || isspace( steam[6] ) ) )
		return false;
	struct _stat64i32 information;
	if ( _stat64i32( fileName, &information ) == 0 || _stat64i32( moduleName, &information ) != 0 )
		return false;
	LaunchSteam( command );
	return true;
}

int WINAPI WinMain( _In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR command, _In_ int )
{
	if ( CheckForSteam( command, "filesystem_stdio.dll", "filesystem_stdio.dll" ) )
		return 0;
	int argc;
	wchar_t** argv = CommandLineToArgvW( GetCommandLineW(), &argc );
	int result = Main( argc, argv );
	GlobalFree( argv );
	return result;
}
