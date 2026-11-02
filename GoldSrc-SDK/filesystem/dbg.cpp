#include "basefilesystem.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#define TRACE_ENTRY_COUNT	64
#define TRACE_ENTRY_LENGTH	256

#ifndef _WIN32
#define ERROR_LOG_DIRECTORY_MODE	( (mode_t)02167 )
#endif

static char g_LogEntries[TRACE_ENTRY_COUNT][TRACE_ENTRY_LENGTH];
static int g_iCurrentEntry;
static int g_iLogNumber;

#ifndef _WIN32
extern "C" int __wrap_rmdir( const char* pathname )
{
	char* path;
	char pszOutBuf[FS_PATH_MATCH_BUFFER_SIZE];
	int result;

	path = NULL;
	pathmatch( pathname, &path, false, pszOutBuf, (unsigned int)sizeof( pszOutBuf ) );
	if ( path == NULL )
	{
		path = (char*)pathname;
	}

	result = rmdir( path );
	if ( pathname != path && path != pszOutBuf )
	{
		free( path );
	}
	return result;
}
#endif

void _LogFunctionTrace( const char* pFunctionName, const char* param )
{
	int entry;
	char* dest;
	int next;

	entry = g_iCurrentEntry;
	dest = g_LogEntries[entry];
	snprintf( dest, sizeof( g_LogEntries[0] ), "%s%s", pFunctionName, param != NULL ? param : "" );

	next = entry + 1;
	if ( next >= TRACE_ENTRY_COUNT )
	{
		next = 0;
	}
	g_iCurrentEntry = next;
}

long double _StartFunctionTimer()
{
	CPerformanceCounter counter;

	return counter.GetCurTime();
}

void ClearErrorLogs()
{
}

extern "C" void Error( const char* pMsg, ... )
{
	va_list va;
	char logName[FS_MAX_PATH_LENGTH];
	FILE* existing;
	FILE* fp;
	int i;
	int slot;

	va_start( va, pMsg );
#ifdef _WIN32
	if ( _mkdir( "errorlogs" ) != 0 && errno != EEXIST )
		fprintf( stderr, "Cannot create errorlogs directory: %d\n", errno );
#else
	if ( __wrap_mkdir( "errorlogs", ERROR_LOG_DIRECTORY_MODE ) != 0 && errno != EEXIST )
		fprintf( stderr, "Cannot create errorlogs directory: %d\n", errno );
#endif

	if ( g_iLogNumber <= 0 )
	{
		while ( true )
		{
			g_iLogNumber = g_iLogNumber + 1;
			sprintf( logName, "errorlogs/errorlog%.5d.txt", g_iLogNumber );
			existing = fopen( logName, "rb" );
			if ( existing == NULL )
			{
				break;
			}
			fclose( existing );
		}
	}
	else
	{
		sprintf( logName, "errorlogs/errorlog%.5d.txt", ++g_iLogNumber );
	}

	fp = fopen( logName, "wt" );
	if ( fp != NULL )
	{
		fwrite( "Error:", 1, sizeof( "Error:" ) - 1, fp );
		vfprintf( fp, pMsg, va );
		fwrite( "\n\nFunction trace:\n", 1u, sizeof( "\n\nFunction trace:\n" ) - 1, fp );
		if ( g_iCurrentEntry > 0 )
		{
			i = g_iCurrentEntry - 1;
			while ( g_LogEntries[i][0] != 0 )
			{
				slot = i;
				g_LogEntries[slot][TRACE_ENTRY_LENGTH - 1] = 0;
				fprintf( fp, "%s\n", g_LogEntries[slot] );
				if ( i == 0 )
				{
					i = TRACE_ENTRY_COUNT - 1;
				}
				else
				{
					i = i - 1;
				}
				g_LogEntries[slot][0] = 0;
			}
		}
		fclose( fp );
	}
	va_end( va );
}

void _LogFunctionTraceMaxTime( const char* pFunctionName, double startTime, double maxTime )
{
	CPerformanceCounter counter;
	double elapsed;
	int next;

	elapsed = counter.GetCurTime() - startTime;
	if ( elapsed > maxTime )
	{
		Error( "Function (%s) took too long, %.3f (max %.3f)\n", pFunctionName, (float)elapsed, (float)maxTime );
		return;
	}

	next = g_iCurrentEntry + 1;
	strncpy( g_LogEntries[g_iCurrentEntry], pFunctionName, ( sizeof( g_LogEntries[0] ) - 1 ) );
	if ( next >= TRACE_ENTRY_COUNT )
	{
		next = 0;
	}
	g_iCurrentEntry = next;
}

#ifndef _WIN32
extern "C" int __wrap___lxstat( int __ver, const char* __filename, struct stat* __stat_buf );
extern "C" int __xmknod( int __ver, const char* path, mode_t mode, dev_t* dev );
#endif

CPerformanceCounter::CPerformanceCounter()
{
}

void CPerformanceCounter::InitializePerformanceCounter()
{
}

double CPerformanceCounter::GetCurTime()
{
#ifdef _WIN32
	static const LARGE_INTEGER frequency = []()
	{
		LARGE_INTEGER value;
		QueryPerformanceFrequency( &value );
		return value;
	}();
	LARGE_INTEGER counter;
	QueryPerformanceCounter( &counter );
	return (double)counter.QuadPart / frequency.QuadPart;
#else
	timeval tp;

	static int secbase;

	gettimeofday( &tp, NULL );
	if ( secbase != 0 )
	{
		return (long double)( tp.tv_sec - secbase ) + (long double)tp.tv_usec / 1000000.0;
	}
	else
	{
		secbase = tp.tv_sec;
		return (long double)tp.tv_usec / 1000000.0;
	}
#endif
}
#ifndef _WIN32

extern "C" int __stat( const char* __filename, struct stat* __stat_buf )
{
	return __wrap___xstat( FS_LINUX_STAT_VERSION, __filename, __stat_buf );
}

extern "C" int __lstat( const char* __filename, struct stat* __stat_buf )
{
	return __wrap___lxstat( FS_LINUX_STAT_VERSION, __filename, __stat_buf );
}

extern "C" int mknod( const char* path, mode_t mode, dev_t dev )
{
	return __xmknod( FS_LINUX_DEVICE_VERSION, path, mode, &dev );
}
#endif
