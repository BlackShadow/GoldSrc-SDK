#include "tier0/dbg.h"
#include "tier0/threadtools.h"

#if defined( _WIN32 )
#include <dbghelp.h>
#include <eh.h>
#include <mbstring.h>
#include <stdio.h>
#include <time.h>

static bool s_bWritingMiniDump;
static int s_nMiniDumpsWritten;
static CThreadFastMutex s_MiniDumpMutex;

TIER0_API volatile bool g_bInException;

typedef void ( *FnMiniDump )( unsigned int, EXCEPTION_POINTERS* );
typedef BOOL ( WINAPI *MiniDumpWriteDump_t )( HANDLE, DWORD, HANDLE, MINIDUMP_TYPE,
	PMINIDUMP_EXCEPTION_INFORMATION, PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION );

extern "C" TIER0_API bool BGetMiniDumpLock()
{
	return s_MiniDumpMutex.TryLock();
}

extern "C" TIER0_API void MiniDumpUnlock()
{
	s_MiniDumpMutex.Unlock();
}

static void WriteMiniDumpUsingExceptionInfo( unsigned int, EXCEPTION_POINTERS* exceptionInfo )
{
	if ( !BGetMiniDumpLock() )
	{
		return;
	}

	HMODULE module = LoadLibraryA( "DbgHelp.dll" );
	if ( !module )
	{
		MiniDumpUnlock();
		return;
	}

	MiniDumpWriteDump_t writeDump = (MiniDumpWriteDump_t)GetProcAddress( module, "MiniDumpWriteDump" );
	if ( writeDump )
	{
		__time64_t now = _time64( NULL );
		tm* local = _localtime64( &now );
		++s_nMiniDumpsWritten;

		char executable[MAX_PATH];
		executable[0] = '\0';
		if ( !GetModuleFileNameA( NULL, executable, sizeof( executable ) ) )
		{
			FreeLibrary( module );
			MiniDumpUnlock();
			return;
		}

		unsigned char* extension = _mbsrchr( (unsigned char*)executable, '.' );
		if ( extension )
		{
			*extension = '\0';
		}

		const unsigned char* separator = _mbsrchr( (unsigned char*)executable, '\\' );
		const char* name = separator ? (const char*)separator + 1 : "unknown";

		char filename[MAX_PATH];
		_snprintf( filename, sizeof( filename ), "%s_%s_%d%.2d%.2d%.2d%.2d%.2d_%d.mdmp",
			name, s_bWritingMiniDump ? "assert" : "crash", local->tm_year + 1900,
			local->tm_mon + 1, local->tm_mday, local->tm_hour, local->tm_min, local->tm_sec,
			s_nMiniDumpsWritten );

		BOOL written = FALSE;
		HANDLE file = CreateFileA( filename, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
			CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( file )
		{
			MINIDUMP_EXCEPTION_INFORMATION info;
			info.ThreadId = GetCurrentThreadId();
			info.ExceptionPointers = exceptionInfo;
			info.ClientPointers = FALSE;
			written = writeDump( GetCurrentProcess(), GetCurrentProcessId(), file,
				MiniDumpWithDataSegs, &info, NULL, NULL );
			CloseHandle( file );
		}

		if ( !written )
		{
			char failedName[MAX_PATH];
			_snprintf( failedName, sizeof( failedName ), "(failed)%s", filename );
			rename( filename, failedName );
		}
	}

	FreeLibrary( module );
	CallFlushLogFunc();
	MiniDumpUnlock();
}

static FnMiniDump s_pfnMiniDump = WriteMiniDumpUsingExceptionInfo;

extern "C" TIER0_API FnMiniDump SetMiniDumpFunction( FnMiniDump function )
{
	FnMiniDump previous = s_pfnMiniDump;
	s_pfnMiniDump = function;
	return previous;
}

static int WriteMiniDumpFilter( EXCEPTION_POINTERS* info )
{
	s_pfnMiniDump( 0, info );
	return EXCEPTION_EXECUTE_HANDLER;
}

void WriteMiniDump( const char*, const char*, int )
{
	s_bWritingMiniDump = true;
	__try
	{
		RaiseException( 0, EXCEPTION_NONCONTINUABLE, 0, NULL );
	}
#pragma warning( suppress: 6322 )
	__except ( WriteMiniDumpFilter( GetExceptionInformation() ) )
	{
	}
	s_bWritingMiniDump = false;
}

static void TranslateException( unsigned int code, EXCEPTION_POINTERS* info )
{
	s_pfnMiniDump( code, info );
}

typedef int ( *FnWMain )( int, char** );
typedef void ( *FnVoidPtrFn )( void* );

extern "C" TIER0_API int CatchAndWriteMiniDump( FnWMain function, int argc, char** argv )
{
	if ( Plat_IsInDebugSession() )
	{
		return function( argc, argv );
	}
	_set_se_translator( TranslateException );
	try
	{
		return function( argc, argv );
	}
	catch ( ... )
	{
		g_bInException = true;
		TerminateProcess( GetCurrentProcess(), 1 );
	}
	return 0;
}

extern "C" TIER0_API void CatchAndWriteMiniDumpForVoidPtrFn( FnVoidPtrFn function, void* argument, bool bQuitOnException )
{
	if ( Plat_IsInDebugSession() )
	{
		function( argument );
		return;
	}
	_set_se_translator( TranslateException );
	try
	{
		function( argument );
	}
	catch ( ... )
	{
		g_bInException = true;
		if ( !bQuitOnException )
		{
			throw;
		}
		TerminateProcess( GetCurrentProcess(), 1 );
	}
}

#else

typedef int ( *FnWMain )( int argc, char** argv );
typedef void ( *FnVoidPtrFn )( void* pv );

void WriteMiniDump( const char*, const char*, int )
{
}

void CatchAndWriteMiniDump( FnWMain pfn, int argc, char** argv )
{
	pfn( argc, argv );
}

void CatchAndWriteMiniDumpForVoidPtrFn( FnVoidPtrFn pfn, void* pv, bool )
{
	pfn( pv );
}

#endif
