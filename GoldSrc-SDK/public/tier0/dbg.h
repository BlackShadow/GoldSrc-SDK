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
// The main debug library interfaces
//=============================================================================


#ifndef DBG_H
#define DBG_H

#include "tier0/platform.h"

#include <stdarg.h>
#include <stdio.h>

enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,
	SPEW_INPUT,

	SPEW_TYPE_COUNT
};

enum SpewRetval_t
{
	SPEW_DEBUGGER = 0,
	SPEW_CONTINUE,
	SPEW_ABORT
};

/* type of externally defined function used to display debug spew */
typedef SpewRetval_t ( *SpewOutputFunc_t )( SpewType_t type, const char* pMessage );
typedef void ( *FlushLogFunc_t )();
typedef void ( *AssertFailedNotifyFunc_t )();

extern "C"
{
	TIER0_API void				Msg( const char* pFormat, ... );
	TIER0_API void				Warning( const char* pFormat, ... );
	TIER0_API void				Error( const char* pFormat, ... );
	TIER0_API void				Log( const char* pFormat, ... );
	TIER0_API void				_DMsg( const char* pGroupName, int level, const char* pMsgFormat, ... );

	TIER0_API void				SpewOutputFunc( SpewOutputFunc_t func );
	TIER0_API SpewOutputFunc_t	GetSpewOutputFunc();
	TIER0_API bool				IsSpewActive( const char* pGroupName, int level );
	TIER0_API bool				IsLogActive( const char* pGroupName, int level );
	TIER0_API void				SpewActivate( const char* pGroupName, int level );
	TIER0_API void				SpewAndLogActivate( const char* pGroupName, int level, int logLevel );

	TIER0_API void				_SpewInfo( SpewType_t type, const char* pFile, int line );
	TIER0_API SpewRetval_t		_SpewMessage( const char* pFormat, ... );
	TIER0_API void				CallFlushLogFunc();
	TIER0_API void				CallAssertFailedNotifyFunc();
	TIER0_API void				_ExitOnFatalAssert( const char* pFile, int line );

	TIER0_API bool				IsInAssert();
	TIER0_API void				SetInAssert( bool state );
	TIER0_API bool				ShouldUseNewAssertDialog();
	TIER0_API bool				DoNewAssertDialog( const char* pFilename, int line, const char* pExpression );

	TIER0_API void				WriteMiniDump( const char* pMessage, const char* pFilename, int line );

	TIER0_API void				Test_SetFailed();
}

class CDbgFmtMsg
{
public:
	CDbgFmtMsg( const char* pszFormat, ... )
	{
		va_list arg_ptr;

		va_start( arg_ptr, pszFormat );
#if defined( _WIN32 )
		_vsnprintf( m_szBuf, sizeof( m_szBuf ) - 1, pszFormat, arg_ptr );
#else
		vsnprintf( m_szBuf, sizeof( m_szBuf ) - 1, pszFormat, arg_ptr );
#endif
		va_end( arg_ptr );

		m_szBuf[sizeof( m_szBuf ) - 1] = 0;
	}

	operator const char*() const
	{
		return m_szBuf;
	}

private:
	char m_szBuf[256];
};

#define _AssertMsgAt( _exp, _msg, _bFatal, _file, _line ) \
	do \
	{ \
		if ( !( _exp ) ) \
		{ \
			if ( !IsInAssert() ) \
			{ \
				SetInAssert( true ); \
				_SpewInfo( SPEW_ASSERT, _file, _line ); \
				const SpewRetval_t ret = _SpewMessage( "%s", _msg ); \
				CallFlushLogFunc(); \
				CallAssertFailedNotifyFunc(); \
				if ( ret == SPEW_DEBUGGER && ShouldUseNewAssertDialog() && DoNewAssertDialog( _file, _line, _msg ) ) \
					DebuggerBreak(); \
				if ( ret == SPEW_DEBUGGER && _bFatal ) \
					_ExitOnFatalAssert( _file, _line ); \
				do \
				{ \
					static bool fDumped = false; \
					if ( !fDumped && !Plat_IsInDebugSession() ) \
					{ \
						WriteMiniDump( _msg, _file, _line ); \
						fDumped = true; \
					} \
				} while ( false ); \
				SetInAssert( false ); \
			} \
		} \
	} while ( false )

#define _AssertMsg( _exp, _msg, _bFatal )	_AssertMsgAt( _exp, _msg, _bFatal, __FILE__, __LINE__ )

#define Assert( expression )	_AssertMsg( expression, "Assertion Failed: " #expression, false )

#define _AssertMsgText( msg )			( (const char*)CDbgFmtMsg( "Assertion Failed: %s", (const char*)( msg ) ) )
#define _AssertMsgFormat( msg, a1 )		( (const char*)CDbgFmtMsg( msg, a1 ) )

#if defined( _WIN32 )
#define DBG_BUILDSLAVE_PATH( relative )	"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\" relative
#define _AssertMsgIn( _exp, _msg, _bFatal, _file, _line )	_AssertMsgAt( _exp, _msg, _bFatal, _file, _line )
#else
#define DBG_BUILDSLAVE_PATH( relative )	relative
#define _AssertMsgIn( _exp, _msg, _bFatal, _file, _line )	_AssertMsg( _exp, _msg, _bFatal )
#endif

#define AssertAtFile( expression, file, line )		_AssertMsgIn( expression, "Assertion Failed: " #expression, false, file, line )
#define AssertMsgAtFile( exp, msg, file, line )		_AssertMsgIn( exp, _AssertMsgText( msg ), false, file, line )

#define AssertAt( expression, line )				_AssertMsgIn( expression, "Assertion Failed: " #expression, false, DBG_SOURCE_FILE, line )
#define AssertMsgAt( exp, msg, line )				AssertMsgAtFile( exp, msg, DBG_SOURCE_FILE, line )
#define AssertMsg1At( exp, msg, a1, line )			AssertMsgAtFile( exp, _AssertMsgFormat( msg, a1 ), DBG_SOURCE_FILE, line )
#define AssertFatalMsg1At( exp, msg, a1, line )		_AssertMsgIn( exp, _AssertMsgText( _AssertMsgFormat( msg, a1 ) ), true, DBG_SOURCE_FILE, line )

#endif /* DBG_H */
