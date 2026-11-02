#include "tier0/platform.h"

#if defined( _WIN32 )
#include "tier0/threadtools.h"
#include "tier0/memalloc.h"

#include <stdlib.h>

static LARGE_INTEGER s_PerformanceFrequency;
static LARGE_INTEGER s_PerformanceStart;
static LONGLONG s_PerformanceTicksPerMillisecond;

static CRITICAL_SECTION s_AllocationMutex;
static struct CAllocationMutexInit
{
	CAllocationMutexInit()
	{
		InitializeCriticalSection( &s_AllocationMutex );
	}
} s_AllocationMutexInit;

void* Plat_Alloc( unsigned int size )
{
	EnterCriticalSection( &s_AllocationMutex );
	void* pRet = g_pMemAlloc->Alloc( size );
	LeaveCriticalSection( &s_AllocationMutex );
	return pRet;
}

void* Plat_Realloc( void* ptr, unsigned int size )
{
	EnterCriticalSection( &s_AllocationMutex );
	void* pRet = g_pMemAlloc->Realloc( ptr, size );
	LeaveCriticalSection( &s_AllocationMutex );
	return pRet;
}

void Plat_Free( void* ptr )
{
	EnterCriticalSection( &s_AllocationMutex );
	free( ptr );
	LeaveCriticalSection( &s_AllocationMutex );
}

bool Plat_VerifyHardwareKey() { return true; }
bool Plat_FastVerifyHardwareKey() { return true; }
bool Plat_VerifyHardwareKeyDriver() { return true; }
bool Plat_VerifyHardwareKeyPrompt() { return true; }

bool Is64BitWindows()
{
	typedef BOOL ( WINAPI *IsWow64Process_t )( HANDLE, PBOOL );
	static IsWow64Process_t pfnIsWow64Process = (IsWow64Process_t)GetProcAddress(
		GetModuleHandleA( "kernel32" ), "IsWow64Process" );
	static bool bChecked;
	static BOOL bWow64;
	if ( !bChecked )
	{
		bChecked = true;
		if ( pfnIsWow64Process )
			return pfnIsWow64Process( GetCurrentProcess(), &bWow64 ) && bWow64;
		return false;
	}
	return bWow64 == TRUE;
}

static void InitPerformanceCounter()
{
	if ( s_PerformanceFrequency.QuadPart == 0 )
	{
		const int MILLISECONDS_PER_SECOND = 1000;
		QueryPerformanceFrequency( &s_PerformanceFrequency );
		s_PerformanceTicksPerMillisecond = s_PerformanceFrequency.QuadPart / MILLISECONDS_PER_SECOND;
		QueryPerformanceCounter( &s_PerformanceStart );
	}
}

double Plat_FloatTime()
{
	InitPerformanceCounter();
	LARGE_INTEGER now;
	QueryPerformanceCounter( &now );
	return (double)( now.QuadPart - s_PerformanceStart.QuadPart ) / (double)s_PerformanceFrequency.QuadPart;
}

unsigned int Plat_MSTime()
{
	InitPerformanceCounter();
	LARGE_INTEGER now;
	QueryPerformanceCounter( &now );
	return (unsigned int)( ( now.QuadPart - s_PerformanceStart.QuadPart ) / s_PerformanceTicksPerMillisecond );
}

const char* Plat_GetCommandLine()
{
	return GetCommandLineA();
}

bool Plat_IsInDebugSession()
{
	return IsDebuggerPresent() != FALSE;
}

bool vtune( bool resume )
{
	typedef void ( *VTuneFunc_t )();
	static bool initialized;
	static VTuneFunc_t pauseFunction;
	static VTuneFunc_t resumeFunction;

	if ( !initialized )
	{
		initialized = true;
		HMODULE module = LoadLibraryA( "vtuneapi.dll" );
		if ( module )
		{
			resumeFunction = (VTuneFunc_t)GetProcAddress( module, "VTResume" );
			pauseFunction = (VTuneFunc_t)GetProcAddress( module, "VTPause" );
		}
	}

	VTuneFunc_t function = resume ? resumeFunction : pauseFunction;
	if ( !function )
	{
		return false;
	}

	function();
	return true;
}

#else
#include <stdlib.h>
#include <sys/time.h>

extern "C"
{

typedef void ( *Plat_AllocErrorFn )( unsigned int size );

enum VCRMode
{
	VCR_Disabled	= 0,
	VCR_Record		= 1,
	VCR_Playback	= 2
};

struct IVCRHelpers;
struct IVCRTrace;

struct VCR_t
{
	int			( *Start )( const char* pFilename, bool bRecord, IVCRHelpers* pHelpers );
	void		( *End )();
	IVCRTrace*	( *GetVCRTraceInterface )();
	VCRMode		( *GetMode )();
	void		( *SetEnabled )( int bEnable );
	void		( *SyncToken )( const char* pToken );
	double		( *Hook_Sys_FloatTime )( double flTime );
};

extern VCRMode	g_VCRMode;
extern VCR_t*	g_pVCR;

void Plat_DefaultAllocErrorFn( unsigned int )
{
}

Plat_AllocErrorFn g_AllocError = Plat_DefaultAllocErrorFn;

double Plat_FloatTime()
{
	static int secbase = 0;
	timeval tp;

	gettimeofday( &tp, NULL );
	if ( secbase != 0 )
	{
		if ( g_VCRMode != VCR_Disabled )
		{
			return g_pVCR->Hook_Sys_FloatTime( (double)( tp.tv_sec - secbase ) + (double)tp.tv_usec / 1000000.0 );
		}
		else
		{
			return (double)( tp.tv_sec - secbase ) + (double)tp.tv_usec / 1000000.0;
		}
	}
	else
	{
		secbase = tp.tv_sec;
		return (double)tp.tv_usec / 1000000.0;
	}
}

unsigned int Plat_MSTime()
{
	static int secbase = 0;
	timeval tp;

	gettimeofday( &tp, NULL );
	if ( secbase != 0 )
	{
		return (unsigned int)(long long)( (double)( 1000 * ( tp.tv_sec - secbase ) ) + (double)tp.tv_usec / 1000.0 );
	}
	else
	{
		double flSeconds = (double)tp.tv_usec / 1000000.0;
		secbase = tp.tv_sec;
		return (unsigned int)(long long)flSeconds;
	}
}

bool vtune( bool )
{
	return false;
}

void* Plat_Alloc( unsigned int size )
{
	void* pRet;

	pRet = malloc( size );
	if ( pRet == NULL )
	{
		g_AllocError( size );
		return NULL;
	}
	return pRet;
}

void* Plat_Realloc( void* ptr, unsigned int size )
{
	void* pRet;

	pRet = realloc( ptr, size );
	if ( pRet == NULL )
	{
		g_AllocError( size );
		return NULL;
	}
	return pRet;
}

void Plat_Free( void* ptr )
{
	free( ptr );
}

void Plat_SetAllocErrorFn( Plat_AllocErrorFn fn )
{
	g_AllocError = fn;
}

}

#include <string.h>

static char g_CmdLine[2048];

extern "C"
{

void Plat_SetCommandLine( const char* cmdLine )
{
	strncpy( g_CmdLine, cmdLine, sizeof( g_CmdLine ) );
	g_CmdLine[sizeof( g_CmdLine ) - 1] = 0;
}

void Plat_SetCommandLineArgs( char** argv, int argc )
{
	int i;

	g_CmdLine[0] = 0;
	for ( i = 0; i < argc; i++ )
	{
		strncat( g_CmdLine, argv[i], sizeof( g_CmdLine ) - strlen( g_CmdLine ) );
		strncat( g_CmdLine, " ", sizeof( g_CmdLine ) - strlen( g_CmdLine ) );
	}
	g_CmdLine[sizeof( g_CmdLine ) - 1] = 0;
}

const char* Plat_GetCommandLine()
{
	return g_CmdLine;
}

int Is64BitOS()
{
	return 0;
}

int GetInstalledRAM()
{
	return 0;
}

unsigned int GetAvailableRAM()
{
	return 0;
}

const char *GetCRunTimeVersion()
{
	return "unknown";
}

}

bool Plat_IsInDebugSession()
{
	return false;
}

#endif
