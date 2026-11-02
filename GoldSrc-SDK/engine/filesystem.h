#pragma once

#include "interface.h"
#include "../public/FileSystem.h"

#include <cstddef>

extern IFileSystem* g_pFileSystem;
extern CreateInterfaceFn g_FileSystemFactory;
extern bool bLowViolenceBuild;

int				FileSystem_Init( char* basedir, void* filesystemFactory );
void			FileSystem_Shutdown();
int				FileSystem_SetGameDirectory( const char* defaultDirectory, const char* gameDirectory );
void			CheckLiblistForFallbackDir( const char* gameDirectory, bool languageEnabled, const char* language,
					bool lowViolence );
bool			BEnabledHDAddon();
bool			BEnableAddonsFolder();
void			Host_SetHDModels_f();
void			Host_SetAddonsFolder_f();

void			FS_GetInterfaceVersion( char* version, int maxLength );
void			FS_AddSearchPath( const char* path, const char* pathID );
void			FS_CreateDirHierarchy( const char* path, const char* pathID );
void			FS_RemoveFile( const char* path, const char* pathID );
void			FS_Rename( const char* originalName, const char* newName );
void			FS_Unlink( const char* filename );
int				FS_FileExists( const char* filename );
unsigned int	FS_FileSize( const char* filename );
int32			FS_GetFileTime( const char* filename );
void			FS_GetLocalCopy( const char* filename );
const char*		FS_GetLocalPath( const char* filename, char* localPath, int localPathBufferSize );
void*			FS_LoadLibrary( const char* libraryName );
void			FS_LogLevelLoadStarted( const char* name );
void			FS_LogLevelLoadFinished( const char* name );

FileHandle_t	FS_Open( const char* filename, const char* options );
FileHandle_t	FS_OpenPathID( const char* filename, const char* options, const char* pathID );
void			FS_Close( FileHandle_t file );
int				FS_Read( void* output, int size, int count, FileHandle_t file );
int				FS_Write( const void* input, int size, int count, FileHandle_t file );
int				FS_FPrintf( FileHandle_t file, const char* format, ... );
char*			FS_ReadLine( char* output, int maxChars, FileHandle_t file );
void			FS_Seek( FileHandle_t file, int position, FileSystemSeek_t seekType );
unsigned int	FS_Tell( FileHandle_t file );
unsigned int	FS_Size( FileHandle_t file );
int				FS_EndOfFile( FileHandle_t file );
int				FS_IsOk( FileHandle_t file );
void			FS_Flush( FileHandle_t file );
int				FS_SetVBuf( FileHandle_t file, char* buffer, int mode, size_t size );
void*			FS_GetReadBuffer( FileHandle_t file, int* bufferSize );
void			FS_ReleaseReadBuffer( FileHandle_t file, void* buffer );
char*			FS_ParseFile( char* fileBytes, char* token, int* quoted );

const char*		FS_FindFirst( const char* pattern, FileFindHandle_t* handle, const char* pathID );
const char*		FS_FindNext( FileFindHandle_t handle );
void			FS_FindClose( FileFindHandle_t handle );
