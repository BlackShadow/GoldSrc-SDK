#include "filesystem.h"
#include "registry.h"
#include "common.h"
#include "host.h"
#include "cmd.h"
#include "server.h"
#include "steam_api.h"
#include "utlvector.h"
#include "../external/steamworks/isteamapps.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"engine\\filesystem.cpp" )

#define APPID_HALF_LIFE 70

static char s_pBaseDir[512];
static CSysModule* g_pFileSystemModule;
CreateInterfaceFn g_FileSystemFactory;
bool bLowViolenceBuild;
CUtlVector<char*> g_fallbackLocalizationFiles;

bool BEnabledHDAddon()
{
	if ( COM_CheckParm( "-nohdmodels" ) )
		return false;

	return registry->ReadInt( "hdmodels", 0 ) > 0;
}

bool BEnableAddonsFolder()
{
	if ( COM_CheckParm( "-addons" ) )
		return true;

	return registry->ReadInt( "addons_folder", 0 ) > 0;
}

int FileSystem_Init( char* basedir, void* voidfilesystemFactory )
{
	Q_strncpy( s_pBaseDir, basedir, sizeof( s_pBaseDir ) - 1 );
	s_pBaseDir[sizeof( s_pBaseDir ) - 1] = '\0';
	host_parms.basedir = s_pBaseDir;

	CreateInterfaceFn factory = (CreateInterfaceFn)voidfilesystemFactory;
	if ( factory )
	{
		g_FileSystemFactory = factory;
	}
	else
	{
#if !defined( _WIN32 )
		g_pFileSystemModule = Sys_LoadModule( "filesystem_stdio.so" );
#else
		g_pFileSystemModule = Sys_LoadModule( "filesystem_stdio.dll" );
#endif
#if defined( _WIN32 )
		ASSERT_AT( g_pFileSystemModule, 95 );
#else
		if ( !g_pFileSystemModule )
			return 0;
#endif

		factory = Sys_GetFactory( g_pFileSystemModule );
		g_FileSystemFactory = factory;
		if ( !factory )
			return 0;
	}

	g_pFileSystem = (IFileSystem*)factory( FILESYSTEM_INTERFACE_VERSION, NULL );
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 109 );
#else
	if ( !g_pFileSystem )
		return 0;
#endif

	return COM_SetupDirectories() != 0;
}

void FileSystem_Shutdown()
{
	g_pFileSystem->RemoveAllSearchPaths();
	if ( g_pFileSystemModule )
	{
		Sys_UnloadModule( g_pFileSystemModule );
		g_pFileSystemModule = NULL;
	}
}

int FileSystem_SetGameDirectory( const char* defaultDirectory, const char* gameDirectory )
{
	char language[128];
	char path[512];
#if defined( _WIN32 ) && !defined( SWDS_DLL )
	char baseDirectory[MAX_PATH];
#else
	char baseDirectory[4096];
#endif

	g_pFileSystem->RemoveAllSearchPaths();
	language[0] = '\0';

#if defined( _WIN32 )
	char lowViolenceToken[128];
	lowViolenceToken[0] = '\0';
	const char* settingsKey = "Software\\Valve\\Half-Life\\Settings";
	Sys_GetRegKeyValue( settingsKey, "User Token 2", lowViolenceToken, sizeof( lowViolenceToken ) - 1, "" );
	bLowViolenceBuild = Q_strlen( lowViolenceToken ) > 0;
	if ( !bLowViolenceBuild )
	{
		Sys_GetRegKeyValue( settingsKey, "User Token 3", lowViolenceToken, sizeof( lowViolenceToken ) - 1, "" );
		bLowViolenceBuild = Q_strlen( lowViolenceToken ) > 0;
	}
#endif

	if ( !bLowViolenceBuild && SteamApps() && GetGameAppID() == APPID_HALF_LIFE )
		bLowViolenceBuild = SteamApps()->BIsLowViolence();

	const char* steamLanguage = NULL;
	if ( SteamApps() )
		steamLanguage = SteamApps()->GetCurrentGameLanguage();
	if ( steamLanguage )
		strncpy( language, steamLanguage, sizeof( language ) );
	else
		strncpy( language, "english", sizeof( language ) );

	if ( !g_bIsDedicatedServer && !IsGameSubscribed( gameDirectory ) )
		return 0;

	SteamAPI_SetBreakpadAppID( GetGameAppID() );

	bool highDefinition = BEnabledHDAddon();
	bool localized = language[0] && _stricmp( language, "english" );
	const char* directory = gameDirectory ? gameDirectory : defaultDirectory;

	if ( directory )
	{
		if ( bLowViolenceBuild )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_lv", s_pBaseDir, directory );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME" );
		}

		if ( BEnableAddonsFolder() )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_addon", s_pBaseDir, directory );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME" );
		}

		if ( localized )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_%s", s_pBaseDir, directory, language );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME" );

			if ( !COM_CheckParm( "-steam" ) )
			{
				Q_strncpy( baseDirectory, s_pBaseDir, sizeof( baseDirectory ) - 1 );
				baseDirectory[sizeof( baseDirectory ) - 1] = '\0';
				char* gamePath = Q_strstr( baseDirectory, "\\game" );
				if ( gamePath )
				{
					*gamePath = '\0';
					snprintf( path, sizeof( path ) - 1, "%s\\localization\\%s_%s", baseDirectory, directory, language );
					path[sizeof( path ) - 1] = '\0';
					COM_FixSlashes( path );
					g_pFileSystem->AddSearchPathNoWrite( path, "GAME" );
				}
			}
		}

		if ( highDefinition )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_hd", s_pBaseDir, directory );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME" );
		}

		snprintf( path, sizeof( path ) - 1, "%s/%s", s_pBaseDir, directory );
		path[sizeof( path ) - 1] = '\0';
		COM_FixSlashes( path );
		g_pFileSystem->AddSearchPath( path, "GAME" );
		g_pFileSystem->AddSearchPath( path, "GAMECONFIG" );

		snprintf( path, sizeof( path ) - 1, "%s/%s_downloads", s_pBaseDir, directory );
		path[sizeof( path ) - 1] = '\0';
		COM_FixSlashes( path );
		g_pFileSystem->AddSearchPath( path, "GAMEDOWNLOAD" );

		CheckLiblistForFallbackDir( directory, localized, language, bLowViolenceBuild );
	}

	if ( localized )
	{
		if ( bLowViolenceBuild )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_lv", s_pBaseDir, defaultDirectory );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );
		}

		if ( BEnableAddonsFolder() )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_addon", s_pBaseDir, defaultDirectory );
			path[sizeof( path ) - 1] = '\0';
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );
		}

		snprintf( path, sizeof( path ) - 1, "%s/%s_%s", s_pBaseDir, defaultDirectory, language );
		path[sizeof( path ) - 1] = '\0';
		COM_FixSlashes( path );
		g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );

		if ( !COM_CheckParm( "-steam" ) )
		{
			Q_strncpy( baseDirectory, s_pBaseDir, sizeof( baseDirectory ) - 1 );
			baseDirectory[sizeof( baseDirectory ) - 1] = '\0';
			char* gamePath = Q_strstr( baseDirectory, "\\game" );
			if ( gamePath )
			{
				*gamePath = '\0';
				snprintf( path, sizeof( path ) - 1, "%s\\localization\\%s_%s", baseDirectory, defaultDirectory, language );
				path[sizeof( path ) - 1] = '\0';
				COM_FixSlashes( path );
				g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );
			}
		}
	}

	if ( highDefinition )
	{
		snprintf( path, sizeof( path ) - 1, "%s/%s_hd", s_pBaseDir, defaultDirectory );
		path[sizeof( path ) - 1] = '\0';
		COM_FixSlashes( path );
		g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );
	}

	snprintf( path, sizeof( path ) - 1, "%s", s_pBaseDir );
	path[sizeof( path ) - 1] = '\0';
	COM_FixSlashes( path );
	g_pFileSystem->AddSearchPath( path, "BASE" );

	snprintf( path, sizeof( path ) - 1, "%s/%s", s_pBaseDir, defaultDirectory );
	path[sizeof( path ) - 1] = '\0';
	COM_FixSlashes( path );
	g_pFileSystem->AddSearchPathNoWrite( path, "DEFAULTGAME" );

	snprintf( path, sizeof( path ) - 1, "%s/platform", s_pBaseDir );
	path[sizeof( path ) - 1] = '\0';
	COM_FixSlashes( path );
	g_pFileSystem->AddSearchPath( path, "PLATFORM" );

	return 1;
}


void CheckLiblistForFallbackDir( const char* pGameDir, bool bLanguage, const char* pLanguage, bool lowViolence ){
	char path[512];
	char line[512];
	char fallback[128];
#if defined( _WIN32 ) && !defined( SWDS_DLL )
	char baseDir[MAX_PATH];
#else
	char baseDir[4096];
#endif

	snprintf( path, sizeof( path ) - 1, "%s/liblist.gam", pGameDir );
	COM_FixSlashes( path );
	g_pFileSystem->GetLocalCopy( path );

	FileHandle_t file;
	if ( !_stricmp( com_gamedir, pGameDir ) )
	{
		file = FS_Open( "liblist.gam", "rt" );
	}
	else
	{
		snprintf( path, sizeof( path ) - 1, "../%s/liblist.gam", pGameDir );
		COM_FixSlashes( path );
		file = FS_Open( path, "rt" );
	}

	if ( !file )
		return;

	while ( !FS_EndOfFile( file ) )
	{
		line[0] = 0;
		FS_ReadLine( line, sizeof( line ) - 1, file );
		line[sizeof( line ) - 1] = 0;
		if ( _strnicmp( line, "fallback_dir", strlen( "fallback_dir" ) ) )
			continue;

		char* start = strchr( line, '"' );
		if ( !start )
			break;

		char* end = strchr( start + 1, '"' );
		if ( !end )
			break;

		unsigned int len = (unsigned int)( end - start - 1 );
		if ( len >= sizeof( fallback ) - 1 )
			break;
		if ( !len )
			continue;

		strncpy( fallback, start + 1, len );
		fallback[len] = 0;
		if ( !_stricmp( pGameDir, fallback ) )
			break;

		if ( lowViolence )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_lv", s_pBaseDir, fallback );
			path[sizeof( path ) - 1] = 0;
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME_FALLBACK" );
		}

		if ( BEnableAddonsFolder() )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_addon", s_pBaseDir, fallback );
			path[sizeof( path ) - 1] = 0;
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME_FALLBACK" );
		}

		if ( bLanguage && pLanguage )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_%s", s_pBaseDir, fallback, pLanguage );
			path[sizeof( path ) - 1] = 0;
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPath( path, "GAME_FALLBACK" );

			if ( !COM_CheckParm( "-steam" ) )
			{
				Q_strncpy( baseDir, s_pBaseDir, sizeof( baseDir ) - 1 );
				baseDir[sizeof( baseDir ) - 1] = 0;
				char* gamePath = Q_strstr( baseDir, "\\game" );
				if ( gamePath )
				{
					*gamePath = 0;
					snprintf( path, sizeof( path ) - 1, "%s\\localization\\%s_%s", baseDir, fallback, pLanguage );
					path[sizeof( path ) - 1] = 0;
					COM_FixSlashes( path );
					g_pFileSystem->AddSearchPath( path, "GAME_FALLBACK" );
				}
			}
		}

		if ( BEnabledHDAddon() )
		{
			snprintf( path, sizeof( path ) - 1, "%s/%s_hd", s_pBaseDir, fallback );
			path[sizeof( path ) - 1] = 0;
			COM_FixSlashes( path );
			g_pFileSystem->AddSearchPathNoWrite( path, "GAME_FALLBACK" );
		}

		snprintf( path, sizeof( path ) - 1, "%s/%s", s_pBaseDir, fallback );
		path[sizeof( path ) - 1] = 0;
		COM_FixSlashes( path );
		g_pFileSystem->AddSearchPath( path, "GAME_FALLBACK" );

		if ( strcmp( fallback, "valve" ) )
		{
			char* fileName = new char[128];
			snprintf( fileName, 127, "Resource/%s_%%language%%.txt", fallback );
			fileName[127] = 0;
			g_fallbackLocalizationFiles.InsertBefore( g_fallbackLocalizationFiles.Count(), fileName );
			CheckLiblistForFallbackDir( fallback, bLanguage, pLanguage, lowViolence );
		}
		break;
	}

	FS_Close( file );
}

char* GetBaseDirectory()
{
	return s_pBaseDir;
}

CreateInterfaceFn GetFileSystemFactory()
{
	return g_FileSystemFactory;
}

int FileSystem_AddFallbackGameDir( const char* pGameDir )
{
	char language[128];
#if defined( _WIN32 ) && !defined( SWDS_DLL )
	char path[MAX_PATH];
#else
	char path[4096];
#endif
	const char* steamLanguage = NULL;

	language[0] = '\0';
	if ( SteamApps() )
		steamLanguage = SteamApps()->GetCurrentGameLanguage();
	if ( steamLanguage )
		strncpy( language, steamLanguage, sizeof( language ) );
	else
		strncpy( language, "english", sizeof( language ) );

	if ( language[0] && _stricmp( language, "english" ) )
	{
		snprintf( path, sizeof( path ), "%s/%s_%s", s_pBaseDir, pGameDir, language );
		path[sizeof( path ) - 1] = '\0';
		g_pFileSystem->AddSearchPath( path, "GAME" );
	}

	g_pFileSystem->AddSearchPath( pGameDir, "GAME" );

	return 1;
}

static void SetGameContentOption( const char* option, bool ( *isEnabled )() )
{
	if ( cls.state == ca_dedicated || Cmd_Argc() != 2 )
		return;

	bool previous = registry->ReadInt( option, 0 ) > 0;
	registry->WriteInt( option, !_stricmp( Cmd_Argv( 1 ), "1" ) );

	if ( previous != isEnabled() )
		COM_SetupDirectories();
}

void Host_SetHDModels_f()
{
	SetGameContentOption( "hdmodels", BEnabledHDAddon );
}

void Host_SetAddonsFolder_f()
{
	SetGameContentOption( "addons_folder", BEnableAddonsFolder );
}
