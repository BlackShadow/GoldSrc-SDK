#include "SystemWrapper.h"

#include "engine_apiproxy.h"
#include "TokenLine.h"
#include "buildnum.h"
#include "cl_demo.h"
#include "client.h"
#include "cmd.h"
#include "common.h"
#include "conprint.h"
#include "cvar.h"
#include "filesystem.h"
#include "mem.h"
#include "render.h"
#include "vgui_intwrap2.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

static void CopyLimited( char* dest, const char* src, int count )
{
	int i = 0;

	if ( src )
	{
		for ( ; i < count && src[i]; i++ )
			dest[i] = src[i];
	}

	for ( ; i < count; i++ )
		dest[i] = '\0';
}

EngineWrapper::EngineWrapper()
{
}

EngineWrapper::~EngineWrapper()
{
}

char* EngineWrapper::GetStatusLine()
{
	return "No status available.\n";
}

char* EngineWrapper::GetType()
{
	return "enginewrapper002";
}

bool EngineWrapper::GetViewOrigin( float* origin )
{
	VectorCopy( r_refdef.vieworg, origin );
	return true;
}

bool EngineWrapper::GetViewAngles( float* angles )
{
	VectorCopy( r_refdef.viewangles, angles );
	return true;
}

int EngineWrapper::GetTraceEntity()
{
	return 0;
}

float EngineWrapper::GetCvarFloat( char* szName )
{
	cvar_t* var = Cvar_FindVar( szName );
	if ( !var )
		return 0.0f;

	return var->value;
}

char* EngineWrapper::GetCvarString( char* szName )
{
	cvar_t* var = Cvar_FindVar( szName );
	if ( !var )
		return NULL;

	return var->string;
}

void EngineWrapper::SetCvar( char* szName, char* szValue )
{
	Cvar_Set( szName, szValue );
}

void EngineWrapper::Cbuf_AddText( char* text )
{
	::Cbuf_AddText( text );
}

void EngineWrapper::DemoUpdateClientData( client_data_s* cdat )
{
	ClientDLL_DemoUpdateClientData( cdat );
	scr_fov_value = cdat->fov;
}

void EngineWrapper::CL_QueueEvent( int flags, int index, float delay, event_args_s* pargs )
{
	::CL_QueueEvent( flags, index, delay, pargs );
}

void EngineWrapper::HudWeaponAnim( int iAnim, int body )
{
	hudWeaponAnim( iAnim, body );
}

void EngineWrapper::CL_DemoPlaySound( int channel, char* sample, float attenuation, float volume, int flags, int pitch )
{
	::CL_DemoPlaySound( channel, sample, attenuation, volume, flags, pitch );
}

void EngineWrapper::ClientDLL_ReadDemoBuffer( int size, unsigned char* buffer )
{
	::ClientDLL_ReadDemoBuffer( size, buffer );
}

bool EngineWrapper::ValidStuffText( const char* pMsg )
{
	return ::ValidStuffText( pMsg ) != FALSE;
}

void EngineWrapper::Cbuf_AddFilteredText( char* text )
{
	::Cbuf_AddFilteredText( text );
}

#ifdef _MSC_VER
#pragma warning( suppress: 26495 )
#endif
SystemWrapper::SystemWrapper()
{
}

SystemWrapper::~SystemWrapper()
{
}

char* SystemWrapper::GetStatusLine()
{
	return "No status available.\n";
}

char* SystemWrapper::GetType()
{
	return "basesystem002";
}

void SystemWrapper::Stop()
{
	m_State = MODULE_DISCONNECTED;
}

double SystemWrapper::GetTime()
{
	return m_SystemTime;
}

unsigned int SystemWrapper::GetTick()
{
	return m_Tick;
}

IFileSystem* SystemWrapper::GetFileSystem()
{
	return g_pFileSystem;
}

void SystemWrapper::GetCommandMatches( char*, ObjectList* pMatchList )
{
	pMatchList->Clear( true );
}

bool SystemWrapper::InitVGUI( IVGuiModule* )
{
	return false;
}

void* SystemWrapper::GetPanel()
{
	return VGuiWrap2_GetPanel();
}

char* SystemWrapper::CheckParam( char* param )
{
	int i = COM_CheckParm( param );
	if ( !i )
		return NULL;

	if ( i + 1 >= com_argc )
		return (char*)com_empty_string;

	return com_argv[i + 1];
}

void SystemWrapper::SetTitle( char* )
{
	Con_Printf( "TODO: SystemWrapper::SetTitle ?\n" );
}

void SystemWrapper::SetStatusLine( char* )
{
	Con_Printf( "TODO: SystemWrapper::SetStatusLine ?\n" );
}

void SystemWrapper::RedirectOutput( char*, int )
{
	Con_Printf( "WARNIG! SystemWrapper::RedirectOutput: not implemented.\n" );
}

void SystemWrapper::Printf( char* fmt, ... )
{
	static char string[8192];
	va_list argptr;

	va_start( argptr, fmt );
	vsnprintf( string, sizeof( string ), fmt, argptr );
	va_end( argptr );

	Con_Printf( "%s", string );
}

void SystemWrapper::DPrintf( char* fmt, ... )
{
	static char string[8192];
	va_list argptr;

	va_start( argptr, fmt );
	vsnprintf( string, sizeof( string ), fmt, argptr );
	va_end( argptr );

	Con_DPrintf( string );
}

void SystemWrapper::ExecuteFile( char* filename )
{
	char cmd[1024];

	snprintf( cmd, sizeof( cmd ) - 1, "exec %s\n", filename );
	cmd[sizeof( cmd ) - 1] = '\0';

	Cmd_ExecuteString( cmd, src_command );
}

unsigned char* SystemWrapper::LoadFile( const char* name, int* length )
{
	return COM_LoadFile( (char*)name, 5, length );
}

void SystemWrapper::FreeFile( unsigned char* fileHandle )
{
	COM_FreeFile( fileHandle );
}

void SystemWrapper::LogConsole( char* filename )
{
	if ( filename )
		Cmd_ExecuteString( "log on", src_command );
	else
		Cmd_ExecuteString( "log off", src_command );
}

bool SystemWrapper::AddModule( ISystemModule* module, char* name )
{
	if ( !module )
		return false;

	if ( !module->Init( this, (int)m_SerialCounter, name ) )
	{
		Printf( "ERROR! System::AddModule: couldn't initialize module %s.\n", name );
		return false;
	}

	m_Modules.AddHead( module );
	m_SerialCounter++;
	return true;
}

void SystemWrapper::Errorf( char* fmt, ... )
{
	static char string[1024];
	va_list argptr;

	va_start( argptr, fmt );
	vsnprintf( string, sizeof( string ), fmt, argptr );
	va_end( argptr );

	Printf( "***** FATAL ERROR *****\n" );
	Printf( "%s", string );
	Printf( "*** STOPPING SYSTEM ***\n" );

	Stop();
}

int COM_BuildNumber()
{
	return build_number();
}

void COM_RemoveEvilChars( char* string )
{
	char* c;

	if ( !string )
		return;

	for ( c = string; *c; c++ )
	{
#if defined( _WIN32 )
		if ( *c == '%' || (signed char)*c < ' ' )
#else
		if ( *c == '%' || (unsigned char)*c < ' ' )
#endif
			*c = ' ';
	}
}

bool SystemWrapper::Init( IBaseSystem* system, int serial, char* name )
{
	BaseSystemModule::Init( system, serial, name );

	m_Commands.Init();
	m_Modules.Init();
	m_Libraries.Init();
	m_Listener.Init();

	m_EngineWrapper = new EngineWrapper();
#if defined( _WIN32 )
	AddModule( m_EngineWrapper, "enginewrapper002" );
#endif

	m_State = MODULE_RUNNING;
	return true;
}

void SystemWrapper::ShutDown()
{
	ISystemModule* module;
	library_t* lib;

	m_Listener.Clear( false );
	m_Commands.Clear( true );

	while ( ( module = (ISystemModule*)m_Modules.GetFirst() ) != NULL )
		module->ShutDown();

	while ( ( lib = (library_t*)m_Libraries.RemoveTail() ) != NULL )
	{
		if ( lib->handle )
			Sys_UnloadModule( lib->handle );

		Mem_Free( lib );
	}

	delete m_EngineWrapper;

	Cmd_RemoveWrapperCmds();
	m_State = MODULE_DISCONNECTED;
}

void SystemWrapper::RunFrame( double time )
{
	ISystemModule* module;

	m_SystemTime += time;
	m_Tick++;

	if ( m_State != MODULE_RUNNING )
		return;

	for ( module = (ISystemModule*)m_Modules.GetFirst(); module; module = (ISystemModule*)m_Modules.GetNext() )
	{
		if ( m_State == MODULE_DISCONNECTED )
			break;

		module->RunFrame( m_SystemTime );
	}

	m_LastTime = m_SystemTime;
}

ISystemModule* SystemWrapper::FindModule( char* type, char* name )
{
	ISystemModule* module;

	if ( !type || !type[0] )
		return NULL;

	for ( module = (ISystemModule*)m_Modules.GetFirst(); module; module = (ISystemModule*)m_Modules.GetNext() )
	{
		if ( !Q_strcasecmp( type, module->GetType() ) && ( !name || !Q_strcasecmp( name, module->GetName() ) ) )
			return module;
	}

	return NULL;
}

void SystemWrapper::ExecuteSingleCommand( char* text )
{
	while ( *text == ' ' )
		text++;

	if ( *text )
		DispatchCommand( text );
}

void SystemWrapper::RunExecuteString( char* commands )
{
	char singleCmd[256];
	bool quotes;
	char* p;
	char* c;
	int size;

	if ( !commands || !commands[0] )
		return;

	COM_RemoveEvilChars( commands );

	c = commands;
	p = singleCmd;
	size = 1;
	quotes = false;

	while ( 1 )
	{
		*p = *c;

		if ( *c == '"' )
		{
			quotes = !quotes;
		}
		else if ( ( *c == ';' && !quotes ) || !*c )
		{
			*p = '\0';
			ExecuteSingleCommand( singleCmd );

			if ( !*c )
				return;

			c++;
			p = singleCmd;
			size = 1;
			continue;
		}

		c++;
		if ( ++size >= (int)sizeof( singleCmd ) )
		{
			Printf( "WARNING! System::ExecuteString: Command token too long.\n" );
			return;
		}

		p++;
	}
}

void SystemWrapper::ExecuteString( char* commands )
{
	RunExecuteString( commands );
}

bool SystemWrapper::DispatchCommand( char* command )
{
	command_t* cmd;

	if ( !command || !command[0] )
		return true;

	for ( cmd = (command_t*)m_Commands.GetFirst(); cmd; cmd = (command_t*)m_Commands.GetNext() )
	{
		if ( !Q_strnicmp( cmd->name, command, (int)strlen( cmd->name ) ) )
		{
			cmd->module->ExecuteCommand( cmd->commandID, command );
			return true;
		}
	}

	Cmd_ExecuteString( command, src_command );
	return true;
}

SystemWrapper::library_t* SystemWrapper::GetLibrary( char* name )
{
	char fixedname[MAX_LIBRARY_PATH];
	library_t* lib;

	CopyLimited( fixedname, name, sizeof( fixedname ) - 1 );
	fixedname[sizeof( fixedname ) - 1] = '\0';
	COM_FixSlashes( fixedname );

	for ( lib = (library_t*)m_Libraries.GetFirst(); lib; lib = (library_t*)m_Libraries.GetNext() )
	{
		if ( !Q_strcasecmp( name, lib->name ) )
			return lib;
	}

	lib = (library_t*)Mem_Malloc( sizeof( library_t ) );
	if ( !lib )
	{
		DPrintf( "ERROR! System::GetLibrary: out of memory (%s).\n", name );
		return NULL;
	}

#if defined( _WIN32 )
	snprintf( lib->name, sizeof( lib->name ), "%s.dll", fixedname );
#else
	snprintf( lib->name, sizeof( lib->name ), "%s.so", fixedname );
#endif
	FS_GetLocalCopy( lib->name );

	lib->handle = Sys_LoadModule( lib->name );
	if ( !lib->handle )
	{
		DPrintf( "ERROR! System::GetLibrary: coulnd't load library (%s).\n", lib->name );
		Mem_Free( lib );
		return NULL;
	}

	lib->createInterfaceFn = Sys_GetFactory( lib->handle );
	if ( !lib->createInterfaceFn )
	{
		DPrintf( "ERROR! System::GetLibrary: coulnd't get object factory(%s).\n", lib->name );
		Mem_Free( lib );
		return NULL;
	}

	m_Libraries.Add( lib );
	DPrintf( "Loaded library %s.\n", lib->name );
	return lib;
}

ISystemModule* SystemWrapper::GetModule( char* interfacename, char* library, char* instancename )
{
	library_t* lib;
	ISystemModule* module;

	if ( interfacename && interfacename[0] )
	{
		module = FindModule( interfacename, instancename );
		if ( module )
			return module;
	}

	lib = GetLibrary( library );
	if ( !lib )
		return NULL;

	module = (ISystemModule*)lib->createInterfaceFn( interfacename, NULL );
	if ( !module )
	{
		Printf( "ERROR! System::GetModule: interface \"%s\" not found in library %s.\n", interfacename, lib->name );
		return NULL;
	}

	AddModule( module, instancename );
	return module;
}

void SystemWrapper::CMD_UnloadModule( char* cmdLine )
{
	TokenLine params( cmdLine );
	ISystemModule* module;
	int count;

	count = params.CountToken();
	if ( count <= 1 )
	{
		Printf( "Syntax: unloadmodule <module> [<name>]\n" );
		return;
	}

	if ( count == 2 )
		module = FindModule( params.GetToken( 1 ), NULL );
	else if ( count == 3 )
		module = FindModule( params.GetToken( 1 ), params.GetToken( 2 ) );
	else
		module = NULL;

	if ( !module )
	{
		Printf( "Module not found.\n" );
		return;
	}

	RemoveModule( module );
}

void SystemWrapper::CMD_LoadModule( char* cmdLine )
{
	TokenLine params( cmdLine );
	int count;

	count = params.CountToken();
	if ( count <= 1 )
	{
		Printf( "Syntax: loadmodule <module> [<library>] [<name>]\n" );
		return;
	}

	if ( count == 2 )
		GetModule( params.GetToken( 1 ), params.GetToken( 1 ), NULL );
	else if ( count == 3 )
		GetModule( params.GetToken( 1 ), params.GetToken( 2 ), NULL );
	else
		GetModule( params.GetToken( 1 ), params.GetToken( 2 ), params.GetToken( 3 ) );
}

bool SystemWrapper::RegisterCommand( char* name, ISystemModule* module, int commandID )
{
	command_t* cmd;

	for ( cmd = (command_t*)m_Commands.GetFirst(); cmd; cmd = (command_t*)m_Commands.GetNext() )
	{
		if ( !Q_strcasecmp( cmd->name, name ) )
		{
			Printf( "WARNING! System::RegisterCommand: command \"%s\" already exists.\n", name );
			return false;
		}
	}

	cmd = (command_t*)Mem_ZeroMalloc( sizeof( command_t ) );
	CopyLimited( cmd->name, name, sizeof( cmd->name ) );
	cmd->name[sizeof( cmd->name ) - 1] = '\0';
	cmd->module = module;
	cmd->commandID = commandID;

	m_Commands.Add( cmd );
	Cmd_AddWrapperCommand( cmd->name, SystemWrapperCommandForwarder );
	return true;
}

void SystemWrapper::CMD_Modules( char* )
{
	ISystemModule* module;
	const char* status;
	const char* type;
	const char* name;

	for ( module = (ISystemModule*)m_Modules.GetFirst(); module; module = (ISystemModule*)m_Modules.GetNext() )
	{
		status = module->GetStatusLine();
		type = module->GetType();
		name = module->GetName();
		Printf( "%s(%s):%s", name, type, status );
	}

	Printf( "--- %i modules in total ---\n", m_Modules.CountElements() );
}

void SystemWrapper::ExecuteCommand( int commandID, char* commandLine )
{
	switch ( commandID )
	{
	case COMMAND_LOADMODULE:
		CMD_LoadModule( commandLine );
		break;
	case COMMAND_UNLOADMODULE:
		CMD_UnloadModule( commandLine );
		break;
	case COMMAND_MODULES:
		CMD_Modules( commandLine );
		break;
	default:
		Printf( "SystemWrapper::ExecuteCommand: unknown command ID %i.\n", commandID );
		break;
	}
}

bool SystemWrapper::RemoveModule( ISystemModule* module )
{
	command_t* cmd;
	ISystemModule* current;

	if ( !module )
		return true;

	module->ShutDown();

	for ( cmd = (command_t*)m_Commands.GetFirst(); cmd; cmd = (command_t*)m_Commands.GetNext() )
	{
		if ( cmd->module->GetSerial() == module->GetSerial() )
		{
			m_Commands.Remove( cmd );
			Mem_Free( cmd );
		}
	}

	for ( current = (ISystemModule*)m_Modules.GetFirst(); current;
		current = (ISystemModule*)m_Modules.GetNext() )
	{
		if ( current->GetSerial() == module->GetSerial() )
		{
			m_Modules.Remove( current );
			return true;
		}
	}

	return false;
}

#if defined( _MSC_VER )
#pragma warning( push )
#pragma warning( disable : 4717 )
#endif
char* SystemWrapper::COM_GetBaseDir()
{
	return COM_GetBaseDir();
}
#if defined( _MSC_VER )
#pragma warning( pop )
#endif

SystemWrapper gSystemWrapper;

void SystemWrapper_Init()
{
	gSystemWrapper.SystemWrapper::Init( &gSystemWrapper, 0, "SystemWrapper" );
}

void SystemWrapper_ShutDown()
{
	gSystemWrapper.SystemWrapper::ShutDown();
}

void SystemWrapper_RunFrame( double time )
{
	gSystemWrapper.SystemWrapper::RunFrame( time );
}

void SystemWrapper_ExecuteString( char* command )
{
	gSystemWrapper.SystemWrapper::ExecuteString( command );
}

void SystemWrapperCommandForwarder()
{
	char cmd[1024];

	strcpy( cmd, Cmd_Argv( 0 ) );
	if ( Cmd_Argc() > 1 )
	{
		strcat( cmd, " " );
		strcat( cmd, Cmd_Args() );
	}
	cmd[sizeof( cmd ) - 1] = '\0';

	gSystemWrapper.RunExecuteString( cmd );
}

int SystemWrapper_LoadModule( char* interfacename, char* library, char* instancename )
{
	return gSystemWrapper.GetModule( interfacename, library, instancename ) ? 1 : 0;
}
