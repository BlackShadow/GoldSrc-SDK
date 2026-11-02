#if !defined( _WIN32 )

#include "tier0/dbg.h"
#include "tier0/threadtools.h"

#include <sys/socket.h>

#include <errno.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef char tchar;
typedef unsigned char uint8;
typedef unsigned int uint32;

enum VCRMode
{
	VCR_Disabled	= 0,
	VCR_Record		= 1,
	VCR_Playback	= 2
};

enum VCREvent
{
	VCREvent_Sys_FloatTime					= 0,
	VCREvent_recvfrom						= 1,
	VCREvent_SyncToken						= 2,
	VCREvent_GetCursorPos					= 3,
	VCREvent_SetCursorPos					= 4,
	VCREvent_ScreenToClient					= 5,
	VCREvent_Cmd_Exec						= 6,
	VCREvent_CmdLine						= 7,
	VCREvent_RegOpenKeyEx					= 8,
	VCREvent_RegSetValueEx					= 9,
	VCREvent_RegQueryValueEx				= 10,
	VCREvent_RegCreateKeyEx					= 11,
	VCREvent_RegCloseKey					= 12,
	VCREvent_PeekMessage					= 13,
	VCREvent_GameMsg						= 14,
	VCREvent_GetNumberOfConsoleInputEvents	= 15,
	VCREvent_ReadConsoleInput				= 16,
	VCREvent_GetKeyState					= 17,
	VCREvent_recv							= 18,
	VCREvent_send							= 19,
	VCREvent_Generic						= 20,
	VCREvent_CreateThread					= 21,
	VCREvent_WaitForSingleObject			= 22,
	VCREvent_EnterCriticalSection			= 23,
	VCREvent_Time							= 24,
	VCREvent_LocalTime						= 25,
	VCREvent_GenericString					= 26,

	VCREvent_NUMEVENTS						= 27
};

struct tagMSG;
struct tagPOINT;
struct sockaddr;

class IVCRHelpers
{
public:
	virtual void ErrorMessage( char* pMsg ) = 0;
};

class IVCRTrace
{
public:
	virtual VCREvent ReadEvent() = 0;
	virtual void Read( void* pDest, int size ) = 0;
};

class CVCRTrace : public IVCRTrace
{
public:
	virtual VCREvent ReadEvent() override;
	virtual void Read( void* pDest, int size ) override;
};

struct VCR_t
{
	int				( *Start )( const tchar* pFilename, bool bRecord, IVCRHelpers* pHelpers );
	void			( *End )();
	IVCRTrace*		( *GetVCRTraceInterface )();
	VCRMode			( *GetMode )();
	void			( *SetEnabled )( int bEnabled );
	void			( *SyncToken )( const tchar* pToken );

	double			( *Hook_Sys_FloatTime )( double flTime );
	int				( *Hook_PeekMessage )( tagMSG* msg, void* hWnd, unsigned int wMsgFilterMin,
						unsigned int wMsgFilterMax, unsigned int wRemoveMsg );
	void			( *Hook_RecordGameMsg )( unsigned int uMsg, unsigned int wParam, int lParam );
	void			( *Hook_RecordEndGameMsg )();
	bool			( *Hook_PlaybackGameMsg )( unsigned int* uMsg, unsigned int* wParam, int* lParam );
	int				( *Hook_recvfrom )( int s, char* buf, int len, int flags, sockaddr* from, int* fromlen );
	void			( *Hook_GetCursorPos )( tagPOINT* pt );
	void			( *Hook_ScreenToClient )( void* hWnd, tagPOINT* pt );
	void			( *Hook_Cmd_Exec )( tchar** f );
	tchar*			( *Hook_GetCommandLine )();
	int				( *Hook_RegOpenKeyEx )( void* hKey, const tchar* lpSubKey, unsigned int ulOptions,
						unsigned int samDesired, void* phkResult );
	int				( *Hook_RegSetValueEx )( void* hKey, const tchar* lpValueName, unsigned int Reserved,
						unsigned int dwType, const uint8* lpData, unsigned int cbData );
	int				( *Hook_RegQueryValueEx )( void* hKey, const tchar* lpValueName, unsigned int* lpReserved,
						unsigned int* lpType, uint8* lpData, unsigned int* lpcbData );
	int				( *Hook_RegCreateKeyEx )( void* hKey, const tchar* lpSubKey, unsigned int Reserved,
						tchar* lpClass, unsigned int dwOptions, unsigned int samDesired, void* lpSecurityAttributes,
						void* phkResult, unsigned int* lpdwDisposition );
	void			( *Hook_RegCloseKey )( void* hKey );
	int				( *Hook_GetNumberOfConsoleInputEvents )( void* hInput, unsigned int* pNumEvents );
	int				( *Hook_ReadConsoleInput )( void* hInput, void* pRecs, int nMaxRecs, unsigned int* pNumRead );
	void			( *Hook_LocalTime )( tm* today );
	short			( *Hook_GetKeyState )( int nVirtKey );
	int				( *Hook_recv )( int s, char* buf, int len, int flags );
	int				( *Hook_send )( int s, const char* buf, int len, int flags );

	void			( *GenericRecord )( const tchar* pEventName, const void* pData, int len );
	int				( *GenericPlayback )( const tchar* pEventName, void* pOutData, int maxLen, bool bForceSameLen );
	void			( *GenericValue )( const tchar* pEventName, void* pData, int maxLen );
	double			( *GetPercentCompleted )();

	void*			( *Hook_CreateThread )( void* pSecAttr, unsigned int cbStack, void* pStart, void* pParam,
						unsigned int dwFlags, unsigned int* pThreadId );
	unsigned int	( *Hook_WaitForSingleObject )( void* handle, unsigned int dwMilliseconds );
	void			( *Hook_EnterCriticalSection )( void* pCS );
	void			( *Hook_Time )( time_t* today );
	void			( *GenericString )( const char* pEventName, const char* pString );
	void			( *GenericValueVerify )( const tchar* pEventName, const void* pData, int maxLen );
	unsigned int	( *Hook_WaitForMultipleObjects )( uint32 nCount, const void** pHandles, int bWaitAll,
						uint32 dwMilliseconds );
};

static int			VCR_Start( const char* pFilename, bool bRecord, IVCRHelpers* pHelpers );
static void			VCR_End();
static IVCRTrace*	VCR_GetVCRTraceInterface();
static VCRMode		VCR_GetMode();
static void			VCR_SetEnabled( int bEnabled );
static void			VCR_SyncToken( const char* pToken );
static double		VCR_Hook_Sys_FloatTime( double flTime );
static int			VCR_Hook_PeekMessage( tagMSG* msg, void* hWnd, unsigned int wMsgFilterMin,
						unsigned int wMsgFilterMax, unsigned int wRemoveMsg );
void				VCR_Hook_RecordGameMsg( unsigned int uMsg, unsigned int wParam, int lParam );
void				VCR_Hook_RecordEndGameMsg();
bool				VCR_Hook_PlaybackGameMsg( unsigned int* uMsg, unsigned int* wParam, int* lParam );
static int			VCR_Hook_recvfrom( int s, char* buf, int len, int flags, sockaddr* from, int* fromlen );
static void			VCR_Hook_GetCursorPos( tagPOINT* pt );
static void			VCR_Hook_ScreenToClient( void* hWnd, tagPOINT* pt );
static void			VCR_Hook_Cmd_Exec( char** f );
static char*		VCR_Hook_GetCommandLine();
static int			VCR_Hook_RegOpenKeyEx( void* hKey, const char* lpSubKey, unsigned int ulOptions,
						unsigned int samDesired, void* phkResult );
static int			VCR_Hook_RegSetValueEx( void* hKey, const char* lpValueName, unsigned int Reserved,
						unsigned int dwType, const uint8* lpData, unsigned int cbData );
static int			VCR_Hook_RegQueryValueEx( void* hKey, const char* lpValueName, unsigned int* lpReserved,
						unsigned int* lpType, uint8* lpData, unsigned int* lpcbData );
static int			VCR_Hook_RegCreateKeyEx( void* hKey, const char* lpSubKey, unsigned int Reserved,
						char* lpClass, unsigned int dwOptions, unsigned int samDesired, void* lpSecurityAttributes,
						void* phkResult, unsigned int* lpdwDisposition );
static void			VCR_Hook_RegCloseKey( void* hKey );
int					VCR_Hook_GetNumberOfConsoleInputEvents( void* hInput, unsigned int* pNumEvents );
int					VCR_Hook_ReadConsoleInput( void* hInput, void* pRecs, int nMaxRecs, unsigned int* pNumRead );
void				VCR_Hook_LocalTime( tm* today );
short				VCR_Hook_GetKeyState( int nVirtKey );
static int			VCR_Hook_recv( int s, char* buf, int len, int flags );
static int			VCR_Hook_send( int s, const char* buf, int len, int flags );

void				VCR_GenericRecord( const char* pEventName, const void* pData, int len );
int					VCR_GenericPlayback( const char* pEventName, void* pOutData, int maxLen, bool bForceSameLen );
void				VCR_GenericValue( const char* pEventName, void* pData, int maxLen );

static void			VCR_Error( const char* pFormat, ... );
static void			VCR_Read( void* pDest, int size );

extern "C"
{

VCR_t g_VCR =
{
	VCR_Start,
	VCR_End,
	VCR_GetVCRTraceInterface,
	VCR_GetMode,
	VCR_SetEnabled,
	VCR_SyncToken,
	VCR_Hook_Sys_FloatTime,
	VCR_Hook_PeekMessage,
	VCR_Hook_RecordGameMsg,
	VCR_Hook_RecordEndGameMsg,
	VCR_Hook_PlaybackGameMsg,
	VCR_Hook_recvfrom,
	VCR_Hook_GetCursorPos,
	VCR_Hook_ScreenToClient,
	VCR_Hook_Cmd_Exec,
	VCR_Hook_GetCommandLine,
	VCR_Hook_RegOpenKeyEx,
	VCR_Hook_RegSetValueEx,
	VCR_Hook_RegQueryValueEx,
	VCR_Hook_RegCreateKeyEx,
	VCR_Hook_RegCloseKey,
	VCR_Hook_GetNumberOfConsoleInputEvents,
	VCR_Hook_ReadConsoleInput,
	VCR_Hook_LocalTime,
	VCR_Hook_GetKeyState,
	VCR_Hook_recv,
	VCR_Hook_send,
	VCR_GenericRecord,
	VCR_GenericPlayback,
	VCR_GenericValue,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

VCR_t*			g_pVCR = &g_VCR;
VCREvent		g_LastReadEvent = (VCREvent)-1;
VCRMode			g_OldVCRMode = (VCRMode)-1;
bool			g_bTraceRead;
int				g_FileLen;
int				g_CurFilePos;
int				g_iCurEvent;
VCRMode			g_VCRMode;
FILE*			g_pVCRFile;
IVCRHelpers*	g_pHelpers;

}

static CVCRTrace g_VCRTrace;

static IVCRTrace* VCR_GetVCRTraceInterface()
{
	return &g_VCRTrace;
}

static VCRMode VCR_GetMode()
{
	return g_VCRMode;
}

static int VCR_Hook_PeekMessage( tagMSG*, void*, unsigned int, unsigned int, unsigned int )
{
	return -1;
}

static void VCR_Hook_ScreenToClient( void*, tagPOINT* )
{
}

static int VCR_Hook_RegOpenKeyEx( void*, const char*, unsigned int, unsigned int, void* )
{
	return 0;
}

static int VCR_Hook_RegSetValueEx( void*, const char*, unsigned int, unsigned int, const uint8*, unsigned int )
{
	return 0;
}

static int VCR_Hook_RegQueryValueEx( void*, const char*, unsigned int*, unsigned int*, uint8*, unsigned int* )
{
	return 0;
}

static int VCR_Hook_RegCreateKeyEx( void*, const char*, unsigned int, char*, unsigned int, unsigned int,
		void*, void*, unsigned int* )
{
	return 0;
}

static void VCR_Hook_RegCloseKey( void* )
{
}

int VCR_Hook_ReadConsoleInput( void*, void*, int, unsigned int* )
{
	return 0;
}

short VCR_Hook_GetKeyState( int )
{
	return 0;
}

static void VCR_Error( const char* pFormat, ... )
{
	char str[256];
	va_list marker;

	va_start( marker, pFormat );
	snprintf( str, sizeof( str ), pFormat, marker );
	va_end( marker );

	g_pHelpers->ErrorMessage( str );
	g_pVCR->End();
}

static void VCR_Read( void* pDest, int size )
{
	if ( g_pVCRFile != NULL )
	{
		fread( pDest, 1, size, g_pVCRFile );
		g_CurFilePos += size;
		if ( g_CurFilePos > g_FileLen )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "g_CurFilePos <= g_FileLen" );
		}
		if ( g_CurFilePos >= g_FileLen )
		{
			g_pVCR->End();
		}
	}
	else
	{
		memset( pDest, 0, size );
	}
}

void VCR_Hook_LocalTime( tm* today )
{
	time_t ltime;

	time( &ltime );
	*today = *localtime( &ltime );
}

static void VCR_End()
{
	if ( g_pVCRFile != NULL )
	{
		fclose( g_pVCRFile );
		g_pVCRFile = NULL;
	}
	g_VCRMode = VCR_Disabled;
}

static void VCR_SetEnabled( int bEnabled )
{
	if ( bEnabled )
	{
		if ( g_OldVCRMode == (VCRMode)-1 )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "g_OldVCRMode != (VCRMode)-1" );
		}
		g_VCRMode = g_OldVCRMode;
		g_OldVCRMode = (VCRMode)-1;
	}
	else
	{
		if ( g_OldVCRMode != (VCRMode)-1 )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "g_OldVCRMode == (VCRMode)-1" );
		}
		g_OldVCRMode = g_VCRMode;
		g_VCRMode = VCR_Disabled;
	}
}

static void VCR_Hook_GetCursorPos( tagPOINT* )
{
}

static int VCR_Start( const char* pFilename, bool bRecord, IVCRHelpers* pHelpers )
{
	unsigned int version;

	g_pHelpers = pHelpers;
	g_pVCR->End();
	g_OldVCRMode = (VCRMode)-1;
	if ( bRecord )
	{
		g_pVCRFile = fopen( pFilename, "wb" );
		if ( g_pVCRFile != NULL )
		{
			version = 2;
			fwrite( &version, 1, 4, g_pVCRFile );
			fflush( g_pVCRFile );
			g_VCRMode = VCR_Record;
			return 1;
		}
		return 0;
	}

	g_pVCRFile = fopen( pFilename, "rb" );
	if ( g_pVCRFile == NULL )
	{
		return 0;
	}

	fseek( g_pVCRFile, 0, SEEK_END );
	g_FileLen = ftell( g_pVCRFile );
	fseek( g_pVCRFile, 0, SEEK_SET );
	g_CurFilePos = 0;
	VCR_Read( &version, 4 );
	if ( version == 2 )
	{
		g_VCRMode = VCR_Playback;
		return 1;
	}
	else
	{
		g_pVCR->End();
		return 0;
	}
}

char* GetCommandLine();

static void VCR_SyncEvent( VCREvent type )
{
	signed char eventByte;

	if ( g_VCRMode == VCR_Record )
	{
		eventByte = (signed char)type;
		g_bTraceRead = false;
		fwrite( &eventByte, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
	}
	else
	{
		g_bTraceRead = true;
		VCR_Read( &eventByte, 1 );
		g_LastReadEvent = (VCREvent)eventByte;
		if ( g_LastReadEvent != type )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "currentEvent == type" );
		}
	}
}

void VCR_GenericRecord( const char* pEventName, const void* pData, int len )
{
	unsigned char ucNameLen;
	int nameLen;

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_Generic );
	}

	if ( g_VCRMode != VCR_Record )
	{
		Error( "VCR_GenericRecord( %s ): not recording a VCR file", pEventName );
	}

	if ( pEventName == NULL )
	{
		ucNameLen = 0xFF;
	}
	else
	{
		nameLen = (int)strlen( pEventName ) + 1;
		if ( nameLen > 254 )
		{
			VCR_Error( "VCR_GenericRecord( %s ): nameLen too long (%d)", pEventName, nameLen );
			return;
		}
		ucNameLen = (unsigned char)nameLen;
	}

	fwrite( &ucNameLen, 1, 1, g_pVCRFile );
	fflush( g_pVCRFile );
	fwrite( pEventName, 1, ucNameLen, g_pVCRFile );
	fflush( g_pVCRFile );
	fwrite( &len, 1, sizeof( len ), g_pVCRFile );
	fflush( g_pVCRFile );
	fwrite( pData, 1, len, g_pVCRFile );
	fflush( g_pVCRFile );
}

int VCR_GenericPlayback( const char* pEventName, void* pOutData, int maxLen, bool bForceSameLen )
{
	unsigned char nameLen;
	int dataLen;
	char testName[512];

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_Generic );
	}

	if ( g_VCRMode != VCR_Playback )
	{
		Error( "VCR_Playback( %s ): not playing back a VCR file", pEventName );
	}

	VCR_Read( &nameLen, 1 );
	if ( nameLen != 0xFF )
	{
		VCR_Read( testName, nameLen );
		if ( strcmp( pEventName, testName ) != 0 )
		{
			VCR_Error( "VCR_GenericPlayback( %s ) - event name does not match '%s'", pEventName, testName );
			return 0;
		}
	}

	VCR_Read( &dataLen, (int)sizeof( dataLen ) );
	if ( dataLen > maxLen )
	{
		VCR_Error( "VCR_GenericPlayback( %s ) - generic data too long (greater than maxLen: %d)", pEventName, maxLen );
		return 0;
	}
	if ( dataLen != maxLen && bForceSameLen )
	{
		VCR_Error( "VCR_GenericPlayback( %s ) - data size in file (%d) different than desired (%d)",
			pEventName, dataLen, maxLen );
		return 0;
	}

	VCR_Read( pOutData, dataLen );
	return dataLen;
}

void VCR_GenericValue( const char* pEventName, void* pData, int maxLen )
{
	if ( g_VCRMode == VCR_Record )
	{
		VCR_GenericRecord( pEventName, pData, maxLen );
	}
	else if ( g_VCRMode == VCR_Playback )
	{
		VCR_GenericPlayback( pEventName, pData, maxLen, true );
	}
}

void VCR_Hook_RecordEndGameMsg()
{
	unsigned char val;

	if ( g_VCRMode == VCR_Record )
	{
		val = (unsigned char)VCREvent_GameMsg;
		++g_iCurEvent;
		g_bTraceRead = false;
		fwrite( &val, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
		val = 0;
		fwrite( &val, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
	}
}

static double VCR_Hook_Sys_FloatTime( double flTime )
{
	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_Sys_FloatTime );

		if ( g_VCRMode == VCR_Record )
		{
			fwrite( &flTime, 1, sizeof( flTime ), g_pVCRFile );
			fflush( g_pVCRFile );
		}
		else if ( g_VCRMode == VCR_Playback )
		{
			VCR_Read( &flTime, (int)sizeof( flTime ) );
		}
	}

	return flTime;
}

static char* VCR_Hook_GetCommandLine()
{
	char* pLine;
	int len;

	pLine = GetCommandLine();
	if ( g_VCRMode == VCR_Disabled )
	{
		return pLine;
	}

	++g_iCurEvent;
	VCR_SyncEvent( VCREvent_CmdLine );

	if ( g_VCRMode == VCR_Playback )
	{
		char* pRecorded;

		VCR_Read( &len, (int)sizeof( len ) );
		pRecorded = new char[len];
		VCR_Read( pRecorded, len );
		return pRecorded;
	}

	if ( g_VCRMode == VCR_Record )
	{
		len = (int)strlen( pLine ) + 1;
		fwrite( &len, 1, sizeof( len ), g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( pLine, 1, len, g_pVCRFile );
		fflush( g_pVCRFile );
	}

	return pLine;
}

void VCR_Hook_RecordGameMsg( unsigned int uMsg, unsigned int wParam, int lParam )
{
	unsigned char val;

	if ( g_VCRMode == VCR_Record )
	{
		val = (unsigned char)VCREvent_GameMsg;
		++g_iCurEvent;
		g_bTraceRead = false;
		fwrite( &val, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
		val = 1;
		fwrite( &val, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( &uMsg, 1, sizeof( uMsg ), g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( &wParam, 1, sizeof( wParam ), g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( &lParam, 1, sizeof( lParam ), g_pVCRFile );
		fflush( g_pVCRFile );
	}
}

int VCR_Hook_GetNumberOfConsoleInputEvents( void*, unsigned int* pNumEvents )
{
	signed char ret;

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_GetNumberOfConsoleInputEvents );

		if ( g_VCRMode == VCR_Playback )
		{
			VCR_Read( &ret, 1 );
			VCR_Read( pNumEvents, (int)sizeof( *pNumEvents ) );
			return ret;
		}

		ret = 1;
		if ( g_VCRMode == VCR_Record )
		{
			fwrite( &ret, 1, 1, g_pVCRFile );
			fflush( g_pVCRFile );
			fwrite( pNumEvents, 1, sizeof( *pNumEvents ), g_pVCRFile );
			fflush( g_pVCRFile );
		}
		return ret;
	}

	return 1;
}

static void VCR_Hook_Cmd_Exec( char** f )
{
	int len;

	if ( g_VCRMode == VCR_Disabled )
	{
		return;
	}

	++g_iCurEvent;
	VCR_SyncEvent( VCREvent_Cmd_Exec );

	if ( g_VCRMode == VCR_Playback )
	{
		VCR_Read( &len, (int)sizeof( len ) );
		if ( len == -1 )
		{
			*f = NULL;
		}
		else
		{
			*f = (char*)malloc( len );
			VCR_Read( *f, len );
		}
		return;
	}

	if ( g_VCRMode == VCR_Record )
	{
		if ( *f != NULL )
		{
			len = (int)strlen( *f ) + 1;
			fwrite( &len, 1, sizeof( len ), g_pVCRFile );
			fflush( g_pVCRFile );
			fwrite( *f, 1, len, g_pVCRFile );
		}
		else
		{
			len = -1;
			fwrite( &len, 1, sizeof( len ), g_pVCRFile );
		}
		fflush( g_pVCRFile );
	}
}

static void VCR_SyncToken( const char* pToken )
{
	unsigned char len;
	char test[256];

	if ( g_VCRMode == VCR_Disabled )
	{
		return;
	}

	++g_iCurEvent;
	VCR_SyncEvent( VCREvent_SyncToken );

	if ( g_VCRMode == VCR_Record )
	{
		len = (unsigned char)strlen( pToken );
		fwrite( &len, 1, 1, g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( pToken, 1, len, g_pVCRFile );
		fflush( g_pVCRFile );
	}
	else if ( g_VCRMode == VCR_Playback )
	{
		VCR_Read( &len, 1 );
		VCR_Read( test, len );
		if ( len != (unsigned char)strlen( pToken ) )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "len == (unsigned char)strlen(pToken)" );
		}
		if ( memcmp( pToken, test, len ) != 0 )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "memcmp(pToken, test, len) == 0" );
		}
	}
}

static int VCR_Hook_send( int s, const char* buf, int len, int flags )
{
	int ret;
	int err;

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_send );
	}

	if ( g_VCRMode == VCR_Playback )
	{
		VCR_Read( &ret, (int)sizeof( ret ) );
		if ( ret == -1 )
		{
			VCR_Read( &err, (int)sizeof( err ) );
			errno = err;
		}
		return ret;
	}

	ret = send( s, buf, len, flags );
	if ( g_VCRMode == VCR_Record )
	{
		fwrite( &ret, 1, sizeof( ret ), g_pVCRFile );
		fflush( g_pVCRFile );
		if ( ret == -1 )
		{
			fwrite( &errno, 1, sizeof( errno ), g_pVCRFile );
			fflush( g_pVCRFile );
		}
	}
	return ret;
}

static int VCR_Hook_recv( int s, char* buf, int len, int flags )
{
	int ret;
	int err;

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_recv );
	}

	if ( g_VCRMode == VCR_Playback )
	{
		VCR_Read( &ret, (int)sizeof( ret ) );
		if ( ret == -1 )
		{
			VCR_Read( &err, (int)sizeof( err ) );
			errno = err;
		}
		else
		{
			VCR_Read( buf, ret );
		}
		return ret;
	}

	ret = recv( s, buf, len, flags );
	if ( g_VCRMode == VCR_Record )
	{
		fwrite( &ret, 1, sizeof( ret ), g_pVCRFile );
		fflush( g_pVCRFile );
		if ( ret == -1 )
		{
			fwrite( &errno, 1, sizeof( errno ), g_pVCRFile );
		}
		else
		{
			fwrite( buf, 1, ret, g_pVCRFile );
		}
		fflush( g_pVCRFile );
	}
	return ret;
}

static int VCR_Hook_recvfrom( int s, char* buf, int len, int flags, sockaddr* from, int* fromlen )
{
	int ret;
	int err;
	unsigned char bFrom;

	if ( g_VCRMode != VCR_Disabled )
	{
		++g_iCurEvent;
		VCR_SyncEvent( VCREvent_recvfrom );

		if ( g_VCRMode == VCR_Playback )
		{
			VCR_Read( &ret, (int)sizeof( ret ) );
			if ( ret != -1 )
			{
				VCR_Read( buf, ret );
				VCR_Read( &bFrom, 1 );
				if ( bFrom != 0 )
				{
					VCR_Read( from, *fromlen );
				}
				return ret;
			}

			VCR_Read( &err, (int)sizeof( err ) );
			errno = err;
			return ret;
		}
	}

	ret = recvfrom( s, buf, len, flags, from, (socklen_t*)fromlen );
	if ( g_VCRMode != VCR_Record )
	{
		return ret;
	}

	fwrite( &ret, 1, sizeof( ret ), g_pVCRFile );
	fflush( g_pVCRFile );
	if ( ret == -1 )
	{
		fwrite( &errno, 1, sizeof( errno ), g_pVCRFile );
		fflush( g_pVCRFile );
		return ret;
	}

	fwrite( buf, 1, ret, g_pVCRFile );
	fflush( g_pVCRFile );
	bFrom = ( from != NULL ) ? 1 : 0;
	fwrite( &bFrom, 1, 1, g_pVCRFile );
	fflush( g_pVCRFile );
	if ( bFrom == 0 )
	{
		return ret;
	}

	fwrite( from, 1, *fromlen, g_pVCRFile );
	fflush( g_pVCRFile );
	return ret;
}

static char linuxCmdline[519];

bool VCR_Hook_PlaybackGameMsg( unsigned int* uMsg, unsigned int* wParam, int* lParam )
{
	VCREvent currentEvent;
	unsigned char bMsg;

	if ( g_VCRMode != VCR_Playback )
	{
		return false;
	}

	++g_iCurEvent;
	currentEvent = g_VCRTrace.ReadEvent();
	if ( currentEvent != VCREvent_GameMsg )
	{
		VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "currentEvent == type" );
	}

	if ( g_pVCRFile == NULL )
	{
		return false;
	}

	VCR_Read( &bMsg, sizeof( bMsg ) );
	if ( bMsg == 0 )
	{
		return false;
	}

	VCR_Read( uMsg, sizeof( *uMsg ) );
	VCR_Read( wParam, sizeof( *wParam ) );
	VCR_Read( lParam, sizeof( *lParam ) );
	return true;
}

void OutputDebugStringFormat( const char* pMsg, ... )
{
	char msg[4096];
	va_list marker;
	int len;

	va_start( marker, pMsg );
	vsnprintf( msg, sizeof( msg ) - 1, pMsg, marker );
	va_end( marker );

	len = (int)strlen( msg );
	if ( g_bTraceRead )
	{
		unsigned int tempLen;
		char tempData[4096];

		VCR_Read( &tempLen, sizeof( tempLen ) );
		if ( tempLen > sizeof( tempData ) )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "tempLen <= sizeof( tempData )" );
		}

		VCR_Read( tempData, tempLen );
		tempData[tempLen] = 0;
		fwrite( "FILE: ", 1, sizeof( "FILE: " ) - 1, stderr );
		fputs( tempData, stderr );
		if ( memcmp( msg, tempData, len ) != 0 )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "memcmp( msg, tempData, len ) == 0" );
		}
	}
	else
	{
		fwrite( &len, 1, sizeof( len ), g_pVCRFile );
		fflush( g_pVCRFile );
		fwrite( msg, 1, len, g_pVCRFile );
		fflush( g_pVCRFile );
	}
}

void BuildCmdLine( int argc, char** argv )
{
	int i;
	int len;

	if ( argc <= 0 )
	{
		linuxCmdline[0] = 0;
	}
	else
	{
		len = 0;
		for ( i = 0; i < argc; i++ )
		{
			len += (int)strlen( argv[i] );
		}

		if ( len > 512 )
		{
			printf( "command line too long, %i max\n", 512 );
			exit( -1 );
		}

		linuxCmdline[0] = 0;
		for ( i = 0; i < argc; i++ )
		{
			strcat( linuxCmdline, argv[i] );
			if ( i + 1 < argc )
			{
				strcat( linuxCmdline, " " );
			}
		}
	}

	strcat( linuxCmdline, " -steam" );
}

char* GetCommandLine()
{
	return linuxCmdline;
}

VCREvent CVCRTrace::ReadEvent()
{
	char curEvent;

	g_bTraceRead = true;
	if ( g_pVCRFile != NULL )
	{
		fread( &curEvent, 1, 1, g_pVCRFile );
		++g_CurFilePos;
		if ( g_CurFilePos > g_FileLen )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "g_CurFilePos <= g_FileLen" );
		}
		if ( g_CurFilePos >= g_FileLen )
		{
			g_pVCR->End();
		}

		g_LastReadEvent = (VCREvent)curEvent;
		return (VCREvent)curEvent;
	}

	g_LastReadEvent = VCREvent_Sys_FloatTime;
	return VCREvent_Sys_FloatTime;
}

void CVCRTrace::Read( void* pDest, int size )
{
	if ( g_pVCRFile != NULL )
	{
		fread( pDest, 1, size, g_pVCRFile );
		g_CurFilePos += size;
		if ( g_CurFilePos > g_FileLen )
		{
			VCR_Error( "*** VCR ASSERT FAILED: %s ***\n", "g_CurFilePos <= g_FileLen" );
		}
		if ( g_CurFilePos >= g_FileLen )
		{
			g_pVCR->End();
		}
	}
	else
	{
		memset( pDest, 0, size );
	}
}

#endif
