#include "extdll.h"
#include "util.h"
#include "debug.h"
#include "shared_util.h"

struct DebugOutputLevel
{
	const char* name;
	DebugOutputType value;
};

static DebugOutputLevel outputLevel[] =
{
	{ "bot", DEBUG_BOT },
	{ "career", DEBUG_CAREER },
	{ "tutor", DEBUG_TUTOR },
	{ "stats", DEBUG_STATS },
	{ "hostage", DEBUG_HOSTAGE },
	{ "all", DEBUG_ALL }
};

static unsigned int theDebugOutputTypes;
static char theDebugBuffer[1024];

void UTIL_DPrintf( DebugOutputType outputType, char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (outputType & theDebugOutputTypes) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_DPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_BotDPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (theDebugOutputTypes & DEBUG_BOT) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_CareerDPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (theDebugOutputTypes & DEBUG_CAREER) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_TutorDPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (theDebugOutputTypes & DEBUG_TUTOR) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_StatsDPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (theDebugOutputTypes & DEBUG_STATS) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void UTIL_HostageDPrintf( char* format, ... )
{
	va_list arguments;
	va_start( arguments, format );
	if ( CVAR_GET_FLOAT( "developer" ) > 0 && (theDebugOutputTypes & DEBUG_HOSTAGE) )
	{
		vsprintf( theDebugBuffer, format, arguments );
		g_engfuncs.pfnServerPrint( theDebugBuffer );
	}
	va_end( arguments );
}

void PrintDebugFlag( const char* flag )
{
	for ( int i = 0; flag && i < ARRAYSIZE( outputLevel ); ++i )
	{
		if ( FStrEq( flag, outputLevel[i].name ) && outputLevel[i].value != DEBUG_NONE )
		{
			g_engfuncs.pfnServerPrint( SharedVarArgs( "mp_debug: %s is %s\n", flag,
				(outputLevel[i].value & theDebugOutputTypes) ? "on" : "off" ) );
			return;
		}
	}
	g_engfuncs.pfnServerPrint( SharedVarArgs( "mp_debug: unknown variable '%s'\n", flag ) );
}

static void SetDebugFlag( const char* flag, bool state )
{
	for ( int i = 0; flag && i < ARRAYSIZE( outputLevel ); ++i )
	{
		if ( FStrEq( flag, outputLevel[i].name ) && outputLevel[i].value != DEBUG_NONE )
		{
			if ( state )
				theDebugOutputTypes |= outputLevel[i].value;
			else
				theDebugOutputTypes &= ~outputLevel[i].value;
			g_engfuncs.pfnServerPrint( SharedVarArgs( "mp_debug: %s is now %s\n", flag, state ? "on" : "off" ) );
			return;
		}
	}
	g_engfuncs.pfnServerPrint( SharedVarArgs( "mp_debug: unknown variable '%s'\n", flag ) );
}

void UTIL_SetDprintfFlags( const char* flag )
{
	if ( !(CVAR_GET_FLOAT( "developer" ) > 0) )
		return;
	if ( flag && *flag )
	{
		if ( *flag == '+' )
			SetDebugFlag( flag + 1, true );
		else if ( *flag == '-' )
			SetDebugFlag( flag + 1, false );
		else
			PrintDebugFlag( flag );
		return;
	}
	int remaining = sizeof( theDebugBuffer );
	theDebugBuffer[0] = 0;
	char* output = BufPrintf( theDebugBuffer, remaining, "mp_debug:\n" );
	for ( int i = 0; i < ARRAYSIZE( outputLevel ) - 1; ++i )
		output = BufPrintf( output, remaining, "  %s: %s\n", outputLevel[i].name,
			(outputLevel[i].value & theDebugOutputTypes) ? "on" : "off" );
	g_engfuncs.pfnServerPrint( theDebugBuffer );
}
