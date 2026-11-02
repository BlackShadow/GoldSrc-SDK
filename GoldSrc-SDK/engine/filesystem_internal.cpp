#include "filesystem.h"

#if defined( _WIN32 )
#include <Windows.h>
#include <io.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"engine\\filesystem_internal.cpp" )

#ifdef GetCurrentDirectory
#undef GetCurrentDirectory
#endif

IFileSystem* g_pFileSystem;

void FS_RemoveAllSearchPaths()
{
	g_pFileSystem->RemoveAllSearchPaths();
}

void FS_AddSearchPath( const char* path, const char* pathID )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 26 );
#endif
	g_pFileSystem->AddSearchPath( path, pathID );
}

int FS_RemoveSearchPath( const char* pPath )
{
	return g_pFileSystem->RemoveSearchPath( pPath );
}

void FS_RemoveFile( const char* relativePath, const char* pathID )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 38 );
#endif
	g_pFileSystem->RemoveFile( relativePath, pathID );
}

void FS_CreateDirHierarchy( const char* path, const char* pathID )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 44 );
#endif
	g_pFileSystem->CreateDirHierarchy( path, pathID );
}

int FS_FileExists( const char* fileName )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 50 );
#endif
	return g_pFileSystem->FileExists( fileName );
}

int FS_IsDirectory( const char* pFileName )
{
	return g_pFileSystem->IsDirectory( pFileName );
}

FileHandle_t FS_Open( const char* fileName, const char* options )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 62 );
#endif
	return g_pFileSystem->Open( fileName, options, NULL );
}

FileHandle_t FS_OpenPathID( const char* fileName, const char* options, const char* pathID )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 68 );
#endif
	return g_pFileSystem->Open( fileName, options, pathID );
}

void FS_Close( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 74 );
#endif
	g_pFileSystem->Close( file );
}

void FS_Seek( FileHandle_t file, int position, FileSystemSeek_t seekType )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 80 );
#endif
	g_pFileSystem->Seek( file, position, seekType );
}

unsigned int FS_Tell( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 86 );
#endif
	return g_pFileSystem->Tell( file );
}

unsigned int FS_Size( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 92 );
#endif
	return g_pFileSystem->Size( file );
}

unsigned int FS_FileSize( const char* fileName )
{
	return g_pFileSystem->Size( fileName );
}

int32 FS_GetFileTime( const char* fileName )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 103 );
#endif
	return g_pFileSystem->GetFileTime( fileName );
}

void FS_FileTimeToString( char* pStrip, int maxCharsIncludingTerminator, int32 fileTime )
{
	g_pFileSystem->FileTimeToString( pStrip, maxCharsIncludingTerminator, fileTime );
}

int FS_IsOk( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 115 );
#endif
	return g_pFileSystem->IsOk( file );
}

void FS_Flush( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 121 );
#endif
	g_pFileSystem->Flush( file );
}

int FS_EndOfFile( FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 127 );
#endif
	return g_pFileSystem->EndOfFile( file );
}

int FS_Read( void* output, int size, int count, FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 133 );
	ASSERT_AT( count == 1, 134 );
#endif
	( void )count;
	return g_pFileSystem->Read( output, size, file );
}

int FS_Write( const void* input, int size, int count, FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 140 );
	ASSERT_AT( count == 1, 141 );
#endif
	( void )count;
	return g_pFileSystem->Write( input, size, file );
}

char* FS_ReadLine( char* output, int maxChars, FileHandle_t file )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 147 );
#endif
	return g_pFileSystem->ReadLine( output, maxChars, file );
}

int FS_FPrintf( FileHandle_t file, const char* format, ... )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 153 );
#endif
	char buffer[8192];
	va_list argptr;

	va_start( argptr, format );
	_vsnprintf( buffer, sizeof( buffer ), format, argptr );
	buffer[sizeof( buffer ) - 1] = 0;
	va_end( argptr );

	return g_pFileSystem->FPrintf( file, "%s", buffer );
}

void* FS_GetReadBuffer( FileHandle_t file, int* outputSize )
{
	return g_pFileSystem->GetReadBuffer( file, outputSize, true );
}

void FS_ReleaseReadBuffer( FileHandle_t file, void* buffer )
{
	g_pFileSystem->ReleaseReadBuffer( file, buffer );
}

const char* FS_FindFirst( const char* wildCard, FileFindHandle_t* handle, const char* pathID )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 166 );
#endif
	return g_pFileSystem->FindFirst( wildCard, handle, pathID );
}

const char* FS_FindNext( FileFindHandle_t handle )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 172 );
#endif
	return g_pFileSystem->FindNext( handle );
}

int FS_FindIsDirectory( FileFindHandle_t handle )
{
	return g_pFileSystem->FindIsDirectory( handle );
}

void FS_FindClose( FileFindHandle_t handle )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 184 );
#endif
	g_pFileSystem->FindClose( handle );
}

void FS_GetLocalCopy( const char* fileName )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 190 );
#endif
	g_pFileSystem->GetLocalCopy( fileName );
}

const char* FS_GetLocalPath( const char* fileName, char* localPath, int localPathBufferSize )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 196 );
#endif
	return g_pFileSystem->GetLocalPath( fileName, localPath, localPathBufferSize );
}

char* FS_ParseFile( char* fileBytes, char* token, int* wasQuoted )
{
#if defined( _WIN32 )
	ASSERT_AT( g_pFileSystem, 202 );
#endif
	bool quoted;
	char* next = g_pFileSystem->ParseFile( fileBytes, token, &quoted );

	if ( wasQuoted )
		*wasQuoted = quoted;

	return next;
}

int FS_FullPathToRelativePath( const char* pFullpath, char* pRelative )
{
	return g_pFileSystem->FullPathToRelativePath( pFullpath, pRelative );
}

int FS_GetCurrentDirectory( char* pDirectory, int maxlen )
{
	return g_pFileSystem->GetCurrentDirectory( pDirectory, maxlen );
}

void FS_PrintOpenedFiles()
{
	g_pFileSystem->PrintOpenedFiles();
}

void FS_SetWarningFunc( void ( *pfnWarning )( const char*, ... ) )
{
	g_pFileSystem->SetWarningFunc( pfnWarning );
}

void FS_SetWarningLevel( FileWarningLevel_t level )
{
	g_pFileSystem->SetWarningLevel( level );
}

unsigned char FS_GetCharacter( FileHandle_t f )
{
	unsigned char retval;

	g_pFileSystem->Read( &retval, 1, f );
	return retval;
}

void FS_LogLevelLoadStarted( const char* name )
{
	g_pFileSystem->LogLevelLoadStarted( name );
}

void FS_LogLevelLoadFinished( const char* name )
{
	g_pFileSystem->LogLevelLoadFinished( name );
}

int FS_SetVBuf( FileHandle_t stream, char* buffer, int mode, std::size_t size )
{
	return g_pFileSystem->SetVBuf( stream, buffer, mode, (long)size );
}

void FS_GetInterfaceVersion( char* output, int maxLength )
{
	g_pFileSystem->GetInterfaceVersion( output, maxLength );
}

void FS_Unlink( const char* fileName )
{
	char localPath[512];

	FS_GetLocalPath( fileName, localPath, sizeof( localPath ) );
#if defined( _WIN32 )
	int res = _unlink( localPath );
	ASSERT_AT( res == 0, 289 );
	( void )res;
#else
	unlink( localPath );
#endif
}

void FS_Rename( const char* originalName, const char* newName )
{
	char localPath[512];
	char newPath[512];
	char* cut;

	if ( !FS_GetLocalPath( originalName, localPath, sizeof( localPath ) ) )
		return;

	strcpy( newPath, localPath );

	cut = strstr( newPath, originalName );
#if defined( _WIN32 )
	ASSERT_AT( cut != NULL, 303 );
#else
	if ( !cut )
		return;
#endif

	*cut = '\0';
	strcat( newPath, newName );
#ifdef _MSC_VER
#pragma warning( suppress: 6031 )
#endif
	rename( localPath, newPath );
}

void* FS_LoadLibrary( const char* dllName )
{
	if ( !dllName )
		return NULL;

	FS_GetLocalCopy( dllName );

#if defined( _WIN32 )
	return LoadLibraryA( dllName );
#else
	return dlopen( dllName, RTLD_NOW );
#endif
}
