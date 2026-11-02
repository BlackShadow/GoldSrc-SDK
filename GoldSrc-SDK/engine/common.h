// comndef.h  -- general definitions
#pragma once

#include "const.h"
#include "mathlib.h"
#include "quakedef.h"
#include "filesystem.h"
#include "../common/usercmd.h"

#include <cstddef>
#include <cstdint>

#define MAX_NUM_ARGVS 50
#define CMDLINE_LENGTH 256
#define GAME_DIRECTORY_BYTES 260
#define VERSION_STRING_BYTES 32
#define SERVERINFO_BUFFER_SIZE 256
#define COM_TOKEN_SIZE 2048

#define FSB_ALLOWOVERFLOW 1
#define FSB_OVERFLOWED 2

typedef struct sizebuf_s
{
	const char* buffername;
	unsigned short flags;
	byte* data;
	int maxsize;
	int cursize;
} sizebuf_t;

void			SZ_Alloc( const char* name, sizebuf_t* buffer, int startSize );
void			SZ_Clear( sizebuf_t* buffer );
qboolean		SZ_HasSpace( sizebuf_t* buffer, int length );
void*			SZ_GetSpace( sizebuf_t* buffer, int length );
void			SZ_Write( sizebuf_t* buffer, const void* data, int length );
void			SZ_Print( sizebuf_t* buffer, const char* text ); // strcats onto the sizebuf

//============================================================================

extern qboolean bigendien;

extern short ( *BigShort )( short value );
extern short ( *LittleShort )( short value );
extern int ( *BigLong )( int value );
extern int ( *LittleLong )( int value );
extern float ( *BigFloat )( float value );
extern float ( *LittleFloat )( float value );

short			ShortSwap( short value );
short			ShortNoSwap( short value );
int				LongSwap( int value );
int				LongNoSwap( int value );
float			FloatSwap( float value );
float			FloatNoSwap( float value );

extern int msg_readcount;
extern qboolean msg_badread; // set if a read goes beyond end of message

byte*			MSG_WriteChar( sizebuf_t* buffer, int value );
void			MSG_WriteByte( sizebuf_t* buffer, int value );
void			MSG_WriteShort( sizebuf_t* buffer, int value );
void			MSG_WriteWord( sizebuf_t* buffer, int value );
void			MSG_WriteLong( sizebuf_t* buffer, int value );
void			MSG_WriteFloat( sizebuf_t* buffer, float value );
void			MSG_WriteString( sizebuf_t* buffer, const char* text );
void			MSG_WriteBuf( sizebuf_t* buffer, int length, const void* data );
void			MSG_WriteAngle( sizebuf_t* buffer, float value );
void			MSG_WriteHiresAngle( sizebuf_t* buffer, float value );
void			MSG_WriteCoord( sizebuf_t* buffer, float value );
void			MSG_WriteVec3Coord( sizebuf_t* buffer, const vec_t* coordinates );
void			MSG_WriteUsercmd( sizebuf_t* buffer, usercmd_t* to, usercmd_t* from );

void			MSG_BeginReading();
int				MSG_ReadChar();
int				MSG_ReadByte();
int				MSG_ReadShort();
int				MSG_ReadWord();
int				MSG_ReadLong();
float			MSG_ReadFloat();
char*			MSG_ReadString();
char*			MSG_ReadStringLine();
int				MSG_ReadBuf( int size, void* buffer );
float			MSG_ReadAngle();
float			MSG_ReadHiresAngle();
float			MSG_ReadCoord( sizebuf_t* buffer );
void			MSG_ReadVec3Coord( sizebuf_t* buffer, vec_t* coordinates );
void			MSG_ReadUsercmd( usercmd_t* to, usercmd_t* from );
void			MSG_SkipString();

void			MSG_StartBitWriting( sizebuf_t* buffer );
qboolean		MSG_IsBitWriting();
void			MSG_EndBitWriting( sizebuf_t* buffer );
void			MSG_WriteOneBit( int value );
void			MSG_WriteBits( unsigned int data, int numBits );
void			MSG_WriteSBits( int data, int numBits );
void			MSG_WriteBitAngle( float angle, int numBits );
void			MSG_WriteBitData( void* source, int length );
void			MSG_WriteBitString( const char* text );
void			MSG_WriteBitCoord( float coordinate );
void			MSG_WriteBitVec3Coord( const vec_t* coordinates );

qboolean		MSG_IsBitReading();
void			MSG_StartBitReading( sizebuf_t* buffer );
void			MSG_EndBitReading( sizebuf_t* buffer );
int				MSG_CurrentBit();
int				MSG_ReadOneBit();
unsigned int	MSG_ReadBits( int numBits );
unsigned int	MSG_ReadBits_0( int numBits );
unsigned int	MSG_PeekBits( int numBits );
int				MSG_ReadSBits( int numBits );
float			MSG_ReadBitAngle( int numBits );
int				MSG_ReadBitData( void* destination, int length );
char*			MSG_ReadBitString();
float			MSG_ReadBitCoord();
void			MSG_ReadBitVec3Coord( vec_t* coordinates );

//============================================================================

void			Q_memset( void* destination, int value, int count );
void			Q_memcpy( void* destination, const void* source, int count );
int				Q_memcmp( const void* first, const void* second, int count );
void			Q_strcpy( char* destination, const char* source );
void			Q_strncpy( char* destination, const char* source, int count );
void			Q_strncpy_s( char* destination, const char* source, int count );
int				Q_strlen( const char* text );
char*			Q_strrchr( char* text, char character );
void			Q_strcat( char* destination, const char* source );
int				Q_strcmp( const char* first, const char* second );
int				Q_strncmp( const char* first, const char* second, int count );
int				Q_strcasecmp( const char* first, const char* second );
int				Q_strncasecmp( const char* first, const char* second, size_t count );
int				Q_stricmp( const char* first, const char* second );
int				Q_strnicmp( const char* first, const char* second, int count );
char*			Q_strstr( const char* text, const char* substring );
const char*		Q_stristr( const char* text, const char* substring );
char*			Q_strlwr( char* text );
qboolean		Q_striprefix( const char* text, const char* prefix );
int				Q_FileNameCmp( char* first, char* second );
int				Q_atoi( const char* text );
float			Q_atof( const char* text );
uint64_t		Q_strtoull( char* text );
int				memsearch( byte* start, int count, int search );

//============================================================================

extern char com_token[COM_TOKEN_SIZE];
extern qboolean com_ignorecolons;

extern int com_argc;
extern char** com_argv;
extern char com_cmdline[CMDLINE_LENGTH];

extern char gpszVersionString[VERSION_STRING_BYTES];
extern char gpszProductString[VERSION_STRING_BYTES];
extern char com_gamedir[GAME_DIRECTORY_BYTES];
extern char com_clientfallback[GAME_DIRECTORY_BYTES];
extern char serverinfo[SERVERINFO_BUFFER_SIZE];

extern const char com_empty_string[];

const char*		COM_Parse( const char* data );
const char*		COM_ParseLine( const char* data );
void			COM_UngetToken();
int				COM_TokenWaiting( char* buffer );

void			COM_Init( char* basedir );
void			COM_Shutdown();
void			COM_InitArgv( int count, char** arguments );
int				COM_CheckParm( const char* parameter );
void			COM_ParseDirectoryFromCmd( const char* command, char* directory, int length,
					const char* defaultDirectory );
qboolean		COM_SetupDirectories();
void			COM_AddAppDirectory( char* baseDir, const char* appName );

char*			COM_SkipPath( char* pathname );
void			COM_StripExtension( const char* in, char* out );
char*			COM_FileExtension( const char* filename );
char*			COM_LastFileExtension( char* path );
char*			COM_FileBase( const char* filename, char* base );
char*			COM_FileBase_s( const char* filename, char* base, int maxBytes );
void			COM_DefaultExtension( char* path, char* extension );
void			COM_AddExtension( char* path, const char* extension, int size );
qboolean		COM_HasFileExtension( const char* path, const char* extension );
void			COM_AddDefaultDir( char* directory );
void			COM_FixSlashes( char* filename );
void			COM_StripTrailingSlash( char* path );
void			COM_CreatePath( char* path );
int				COM_ExpandFilename( char* filename );
void			COM_GetGameDir( char* gameDirectory );

int				COM_FileSize( char* filename );
byte*			COM_LoadFile( char* path, int usehunk, int* length );
byte*			COM_LoadHunkFile( char* filename );
void			COM_LoadCacheFile( char* path, cache_user_t* user );
byte*			COM_LoadFileForMe( char* filename, int* length );
byte*			COM_LoadStackFile( char* path, void* buffer, int bufferSize, int* length );
byte*			COM_LoadFileLimit( char* path, int position, int maxBytes, int* bytesRead, FileHandle_t* file );
void			COM_FreeFile( void* buffer );
void			COM_WriteFile( char* filename, void* data, int length );
void			COM_CopyFile( char* source, char* destination );
void			COM_CopyFileChunk( FileHandle_t destination, FileHandle_t source, int size );
int				COM_CompareFileTime( char* firstFilename, char* secondFilename, int* comparison );
void			COM_ListMaps( char* substring );
unsigned int	COM_GetApproxWavePlayLength( const char* filepath );

char*			va( const char* format, ... );
char*			vstr( vec_t* vector );
char*			COM_BinPrintf( byte* buffer, int length );
void			COM_HexConvert( const char* text, int length, byte* output );
void			COM_NormalizeAngles( vec_t* angles );
int				COM_EntsForPlayerSlots( int maxClients );
void			COM_ExplainDisconnection( qboolean print, const char* format, ... );
void			COM_ExtendedExplainDisconnection( qboolean print, const char* format, ... );

void			COM_Munge( byte* data, int length, int sequence );
void			COM_UnMunge( byte* data, int length, int sequence );
void			COM_Munge2( byte* data, int length, int sequence );
void			COM_UnMunge2( byte* data, int length, int sequence );
void			COM_Munge3( byte* data, int length, int sequence );
void			COM_UnMunge3( byte* data, int length, int sequence );

void			LoadBMP8( FileHandle_t file, byte** palette, int* paletteCount, byte** image, int* width, int* height );
