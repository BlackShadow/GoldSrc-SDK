//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// The main debug library implementation
//=============================================================================

#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier0/validator.h"
#include "tier0/memalloc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if !defined( _WIN32 )
#include <strings.h>
#else
#include <mbstring.h>
#include <tchar.h>
#include <assert.h>
#endif
#include <math.h>
#include <stdarg.h>
#include <stdint.h>

#if defined( _WIN32 )
#define snprintf	_snprintf
#define vsnprintf	_vsnprintf
#define TIER0_DBG_SOURCE	L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\tier0\\dbg.cpp"
#else
#define _tcslen		strlen
#endif

#define DBG_SOURCE_FILE	DBG_BUILDSLAVE_PATH( "tier0\\dbg.cpp" )

#define MAX_SPEW_MESSAGE		( (size_t)5020 )
#define MAX_GROUP_NAME_LENGTH	48

struct SpewGroup_t
{
	char	m_GroupName[MAX_GROUP_NAME_LENGTH];
	int		m_Level;
	int		m_LogLevel;
};

SpewRetval_t DefaultSpewFunc( SpewType_t type, const char* pMsg );

static SpewOutputFunc_t s_SpewOutputFunc = DefaultSpewFunc;
static int s_DefaultLogLevel;
static int s_DefaultLevel;
static int s_GroupCount;
static SpewGroup_t* s_pSpewGroups;
static SpewType_t s_SpewType;
static int s_Line;
static const char* s_pFileName;
static AssertFailedNotifyFunc_t s_AssertFailedNotifyFunc;
static FlushLogFunc_t s_FlushLogFunc;

#if defined( _WIN32 )
void ValidateSpew( CValidator& validator )
{
	validator.Push( "Spew globals", NULL, "Global" );
	validator.ClaimMemory( s_pSpewGroups );
	validator.Pop();
}
#endif

//-----------------------------------------------------------------------------
// globals
//-----------------------------------------------------------------------------

SpewRetval_t DefaultSpewFunc( SpewType_t type, const char* pMsg )
{
	printf( "%s", pMsg );

	if ( type == SPEW_ASSERT )
	{
		return SPEW_DEBUGGER;
	}

	if ( type == SPEW_ERROR )
	{
		return SPEW_ABORT;
	}

	return SPEW_CONTINUE;
}

extern "C" TIER0_API void _AssertValidReadPtr( const void* ptr, int count )
{
#if defined( _WIN32 )
	AssertAt( !IsBadReadPtr( ptr, count ), 59 );
#endif
}

extern "C" TIER0_API void _AssertValidWritePtr( void* ptr, int count )
{
#if defined( _WIN32 )
	AssertAt( !IsBadWritePtr( ptr, count ), 69 );
#endif
}

extern "C" TIER0_API void _AssertValidReadWritePtr( void* ptr, int count )
{
#if defined( _WIN32 )
	AssertAt( !( IsBadWritePtr(ptr, count) || IsBadReadPtr(ptr,count)), 78 );
#endif
}

extern "C" TIER0_API void AssertValidStringPtr( const char* ptr, int maxchar )
{
#if defined( _WIN32 )
	AssertAt( !IsBadStringPtr( ptr, maxchar ), 91 );
#endif
}

void SpewOutputFunc( SpewOutputFunc_t func )
{
	if ( func )
	{
		s_SpewOutputFunc = func;
	}
	else
	{
		s_SpewOutputFunc = DefaultSpewFunc;
	}
}

SpewOutputFunc_t GetSpewOutputFunc()
{
	if ( s_SpewOutputFunc )
	{
		return s_SpewOutputFunc;
	}

	return DefaultSpewFunc;
}

extern "C" TIER0_API void SetFlushLogFunc( FlushLogFunc_t func )
{
	s_FlushLogFunc = func;
}

void CallFlushLogFunc()
{
	if ( s_FlushLogFunc )
	{
		s_FlushLogFunc();
	}
}

extern "C" TIER0_API void SetAssertFailedNotifyFunc( AssertFailedNotifyFunc_t func )
{
	s_AssertFailedNotifyFunc = func;
}

void CallAssertFailedNotifyFunc()
{
	if ( s_AssertFailedNotifyFunc )
	{
		s_AssertFailedNotifyFunc();
	}
}

void _SpewInfo( SpewType_t type, const char* pFile, int line )
{
	const char* pSlash;
	const char* pSlash2;

#if defined( _WIN32 )
	pSlash = (const char*)_mbsrchr( (const unsigned char*)pFile, '\\' );
	pSlash2 = (const char*)_mbsrchr( (const unsigned char*)pFile, '/' );
#else
	pSlash = strrchr( pFile, '\\' );
	pSlash2 = strrchr( pFile, '/' );
#endif
	if ( (uintptr_t)pSlash < (uintptr_t)pSlash2 )
	{
		pSlash = pSlash2;
	}

	s_pFileName = pSlash ? pSlash + 1 : pFile;
	s_Line = line;
	s_SpewType = type;
}

bool FindSpewGroup( const char* pGroupName, int* pInd )
{
	int low;
	int high;
	int mid;
	int cmp;

	if ( s_GroupCount == 0 )
	{
		*pInd = 0;
		return false;
	}

	low = 0;
	high = s_GroupCount - 1;

	while ( low <= high )
	{
		mid = ( low + high ) / 2;
#if defined( _WIN32 )
		cmp = _mbsicmp( (const unsigned char*)pGroupName, (const unsigned char*)s_pSpewGroups[mid].m_GroupName );
#else
		cmp = strcasecmp( pGroupName, s_pSpewGroups[mid].m_GroupName );
#endif

		if ( cmp == 0 )
		{
			*pInd = mid;
			return true;
		}

		if ( cmp < 0 )
		{
			high = mid - 1;
		}
		else
		{
			low = mid + 1;
		}
	}

	*pInd = low;
	return false;
}

//-----------------------------------------------------------------------------
// Tests to see if a particular spew is active
//-----------------------------------------------------------------------------

bool IsSpewActive( const char* pGroupName, int level )
{
	int nIndex;
	int nStored;

	if ( FindSpewGroup( pGroupName, &nIndex ) )
	{
		nStored = s_pSpewGroups[nIndex].m_Level;
	}
	else
	{
		nStored = s_DefaultLevel;
	}

	return level <= nStored;
}

bool IsLogActive( const char* pGroupName, int level )
{
	int nIndex;
	int nStored;

	if ( FindSpewGroup( pGroupName, &nIndex ) )
	{
		nStored = s_pSpewGroups[nIndex].m_LogLevel;
	}
	else
	{
		nStored = s_DefaultLogLevel;
	}

	return level <= nStored;
}

extern "C" TIER0_API SpewRetval_t _SpewMessageType( SpewType_t spewType, const char* pMsgFormat, va_list args )
{
	static CThreadMutex autoMutex;
	char pTempBuffer[MAX_SPEW_MESSAGE];
	int len;
	int val;
	SpewRetval_t retval;

	autoMutex.Lock();

#if defined( _WIN32 )
	if ( !( _tcslen( pMsgFormat ) < sizeof( pTempBuffer ) ) )
		_wassert( L"_tcslen( pMsgFormat ) < sizeof( pTempBuffer)", TIER0_DBG_SOURCE, 238 );
#endif

	if ( spewType == SPEW_ASSERT || spewType == SPEW_ERROR )
	{
		Test_SetFailed();
	}

	len = 0;
	if ( spewType == SPEW_ASSERT )
	{
		len = snprintf( pTempBuffer, sizeof( pTempBuffer ) - 1, "%s (%d) : ", s_pFileName, s_Line );
		if ( len == -1 )
		{
			autoMutex.Unlock();
			return SPEW_ABORT;
		}
	}

	val = vsnprintf( &pTempBuffer[len], sizeof( pTempBuffer ) - 1 - len, pMsgFormat, args );
	if ( val == -1 )
	{
		autoMutex.Unlock();
		return SPEW_ABORT;
	}

	len += val;
#if defined( _WIN32 )
	if ( !( (size_t)len * sizeof( *pMsgFormat ) < sizeof( pTempBuffer ) ) )
		_wassert( L"len * sizeof(*pMsgFormat) < sizeof(pTempBuffer)", TIER0_DBG_SOURCE, 264 );
#endif

	if ( spewType == SPEW_ASSERT )
	{
		len += sprintf( &pTempBuffer[len], "\n" );
	}

#if defined( _WIN32 )
	if ( !( (size_t)len < sizeof( pTempBuffer ) / sizeof( pTempBuffer[0] ) - 1 ) )
		_wassert( L"len < sizeof(pTempBuffer)/sizeof(pTempBuffer[0]) - 1", TIER0_DBG_SOURCE, 272 );
	if ( !s_SpewOutputFunc )
		_wassert( L"s_SpewOutputFunc", TIER0_DBG_SOURCE, 273 );
#endif

	retval = s_SpewOutputFunc( spewType, pTempBuffer );

	switch ( retval )
	{
// Put the break into the macro so it would occur in the right place
//	case SPEW_DEBUGGER:
//		DebuggerBreak();
//		break;
	case SPEW_ABORT:
		_DMsg( "console", 1, "Exiting on SPEW_ABORT\n" );
		exit( 0 );

	case SPEW_DEBUGGER:
		if ( spewType != SPEW_ASSERT )
		{
			DebuggerBreak();
		}
		break;

	default:
		break;
	}

	autoMutex.Unlock();
	return retval;
}

void Msg( const char* pMsgFormat, ... )
{
	va_list args;

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_MESSAGE, pMsgFormat, args );
	va_end( args );
}

void Warning( const char* pMsgFormat, ... )
{
	va_list args;

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_WARNING, pMsgFormat, args );
	va_end( args );
}

void Log( const char* pMsgFormat, ... )
{
	va_list args;

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_LOG, pMsgFormat, args );
	va_end( args );
}

void Error( const char* pMsgFormat, ... )
{
	va_list args;

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_ERROR, pMsgFormat, args );
	va_end( args );
}

SpewRetval_t _SpewMessage( const char* pMsgFormat, ... )
{
	va_list args;
	SpewRetval_t retval;

	va_start( args, pMsgFormat );
	retval = _SpewMessageType( s_SpewType, pMsgFormat, args );
	va_end( args );
	return retval;
}

extern "C" TIER0_API void _DMsg( const char* pGroupName, int level, const char* pMsgFormat, ... )
{
	va_list args;

	if ( !IsSpewActive( pGroupName, level ) )
	{
		return;
	}

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_MESSAGE, pMsgFormat, args );
	va_end( args );
}

extern "C" TIER0_API void DWarning( const char* pGroupName, int level, const char* pMsgFormat, ... )
{
	va_list args;

	if ( !IsSpewActive( pGroupName, level ) )
	{
		return;
	}

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_WARNING, pMsgFormat, args );
	va_end( args );
}

extern "C" TIER0_API void DLog( const char* pGroupName, int level, const char* pMsgFormat, ... )
{
	va_list args;

	if ( !IsSpewActive( pGroupName, level ) )
	{
		return;
	}

	va_start( args, pMsgFormat );
	_SpewMessageType( SPEW_LOG, pMsgFormat, args );
	va_end( args );
}

extern "C" TIER0_API SpewRetval_t _DSpewMessage( const char* pGroupName, int level, const char* pMsgFormat, ... )
{
	va_list args;
	SpewRetval_t retval;

	if ( !IsSpewActive( pGroupName, level ) )
	{
		return SPEW_CONTINUE;
	}

	va_start( args, pMsgFormat );
	retval = _SpewMessageType( s_SpewType, pMsgFormat, args );
	va_end( args );
	return retval;
}

extern "C" TIER0_API void _ExitOnFatalAssert( const char* pFile, int line )
{
	_SpewMessage( "Fatal assert failed: %s, line %d.  Application exiting.\n", pFile, line );

	if ( !Plat_IsInDebugSession() )
	{
		WriteMiniDump( "Fatal assert", pFile, line );
	}

	_DMsg( "console", 1, "_ExitOnFatalAssert\n" );
#if defined( _WIN32 )
	TerminateProcess( GetCurrentProcess(), 1 );
#else
	exit( 1 );
#endif
}

void SpewAndLogActivate( const char* pGroupName, int level, int logLevel )
{
	int nIndex;

	AssertAt( pGroupName, 432 );

	if ( pGroupName[0] == '*' && pGroupName[1] == '\0' )
	{
		s_DefaultLevel = level;
		s_DefaultLogLevel = logLevel;
		return;
	}

	if ( !FindSpewGroup( pGroupName, &nIndex ) )
	{
		++s_GroupCount;
		if ( s_pSpewGroups )
		{
#if defined( _WIN32 )
			s_pSpewGroups = (SpewGroup_t*)g_pMemAlloc->Realloc( s_pSpewGroups,
				s_GroupCount * sizeof( SpewGroup_t ), DBG_SOURCE_FILE, 453 );
#else
			s_pSpewGroups = (SpewGroup_t*)realloc( s_pSpewGroups, s_GroupCount * sizeof( SpewGroup_t ) );
#endif
			memmove( &s_pSpewGroups[nIndex + 1], &s_pSpewGroups[nIndex],
				( s_GroupCount - nIndex - 1 ) * sizeof( SpewGroup_t ) );
		}
		else
		{
#if defined( _WIN32 )
			s_pSpewGroups = (SpewGroup_t*)g_pMemAlloc->Alloc( s_GroupCount * sizeof( SpewGroup_t ), DBG_SOURCE_FILE, 461 );
#else
			s_pSpewGroups = (SpewGroup_t*)malloc( s_GroupCount * sizeof( SpewGroup_t ) );
#endif
		}

		AssertAt( _tcslen( pGroupName ) < MAX_GROUP_NAME_LENGTH, 463 );
		strcpy( s_pSpewGroups[nIndex].m_GroupName, pGroupName );
	}

	s_pSpewGroups[nIndex].m_Level = level;
	s_pSpewGroups[nIndex].m_LogLevel = logLevel;
}

void SpewActivate( const char* pGroupName, int level )
{
	SpewAndLogActivate( pGroupName, level, level );
}

extern "C" TIER0_API void SpewChangeIfStillDefault( const char* pGroupName, int level, int leveldefault )
{
	int nIndex;

	if ( !FindSpewGroup( pGroupName, &nIndex ) )
	{
		return;
	}

	if ( s_pSpewGroups[nIndex].m_Level == leveldefault )
	{
		SpewAndLogActivate( pGroupName, level, level );
	}
}

extern "C" TIER0_API void SpewAndLogChangeIfStillDefault( const char* pGroupName, int level, int leveldefault,
		int logLevel, int logLevelDefault )
{
	int nIndex;

	if ( !FindSpewGroup( pGroupName, &nIndex ) )
	{
		return;
	}

	if ( s_pSpewGroups[nIndex].m_Level == leveldefault && s_pSpewGroups[nIndex].m_LogLevel == logLevelDefault )
	{
		SpewAndLogActivate( pGroupName, level, logLevel );
	}
}

extern "C" TIER0_API float CrackSmokingCompiler( float a )
{
	return fabsf( a );
}

extern "C" TIER0_API void* Plat_SimpleLog( const char* file, int line )
{
	FILE* fp;

	fp = fopen( "simple.log", "at+" );
	fprintf( fp, "%s:%i\n", file, line );
	fclose( fp );
	return NULL;
}
