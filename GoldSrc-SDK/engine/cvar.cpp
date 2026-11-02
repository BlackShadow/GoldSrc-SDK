#include "cvar.h"
#include "engine_apiproxy.h"
#include "cmd.h"
#include "common.h"
#include "conprint.h"
#include "client.h"
#include "filesystem.h"
#include "info.h"
#include "protocol.h"
#include "server.h"
#include "sys.h"
#include "unicode_strtools.h"
#include "zone.h"

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined( _WIN32 )
#ifndef _snprintf
#define _snprintf snprintf
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#endif

#if ( defined( HL25_HW_ENGINE ) || defined( SW_RENDERER ) ) && defined( _WIN32 )
#include <xmmintrin.h>
#endif

const char* Cvar_IsMultipleTokens( const char* varname );

cvar_t* cvar_vars;
cvarhook_t* cvar_hooks;
const char* cvar_null_string = com_empty_string;

void Cmd_CvarListPrintCvar( cvar_t* variable, FileHandle_t file )
{
	constexpr int CVAR_LIST_BUFFER_BYTES = 256;
	constexpr int CVAR_LIST_SUFFIX_BYTES = sizeof( ", a, sv, i\n" ) - 1;
	char output[CVAR_LIST_BUFFER_BYTES];
	float value = variable->value;
#if ( defined( HL25_HW_ENGINE ) || defined( SW_RENDERER ) ) && defined( _WIN32 )
	int integerValue = _mm_cvtt_ss2si( _mm_set_ss( value ) );
#else
	int integerValue = static_cast<int>( value );
#endif
	if ( value == static_cast<float>( integerValue ) )
		_snprintf( output, sizeof( output ) - CVAR_LIST_SUFFIX_BYTES, "%-15s : %8i", variable->name, integerValue );
	else
		_snprintf( output, sizeof( output ) - CVAR_LIST_SUFFIX_BYTES, "%-15s : %8.3f", variable->name,
			static_cast<double>( value ) );
	if ( variable->flags & FCVAR_ARCHIVE )
		Q_strcat( output, ", a" );
	if ( variable->flags & FCVAR_SERVER )
		Q_strcat( output, ", sv" );
	if ( variable->flags & FCVAR_USERINFO )
		Q_strcat( output, ", i" );
	Q_strcat( output, "\n" );
	// Print to console
	Con_Printf( "%s", output );
	if ( file )
		FS_FPrintf( file, "%s", output );
}

void Cmd_CvarList_f()
{
	constexpr int CVAR_LIST_FILENAME_BYTES = 256;
	constexpr int CVAR_LIST_MAX_LOG_FILES = 100;
	const char* partial = nullptr;
	int partialLength = 0;
	int count = 0; // Length of the partial cvar
	bool archiveOnly = false;
	bool serverOnly = false;
	FileHandle_t file = nullptr;
	char filename[CVAR_LIST_FILENAME_BYTES];
	int argumentCount = Cmd_Argc();
	if ( argumentCount >= 2 )
	{
		if ( Q_strcasecmp( Cmd_Argv( 1 ), "?" ) == 0 )
		{
			Con_Printf( "CvarList           : List all cvars\n"
						"CvarList [Partial] : List cvars starting with 'Partial'\n"
						"CvarList log [Partial] : Logs cvars to file \"cvarlist.txt\" in the gamedir.\n" );
			return;
		}
		if ( Q_strcasecmp( Cmd_Argv( 1 ), "log" ) == 0 )
		{
			int index = 0;
			for ( ; index < CVAR_LIST_MAX_LOG_FILES; ++index )
			{
				_snprintf( filename, sizeof( filename ), "cvarlist%02d.txt", index );
				COM_FixSlashes( filename );
				FileHandle_t existing = FS_Open( filename, "r" );
				if ( !existing )
					break;
				FS_Close( existing );
			}
			if ( index == CVAR_LIST_MAX_LOG_FILES )
			{
				Con_Printf( "Can't cvarlist! Too many existing cvarlist output files in the gamedir!\n" );
				return;
			}
			file = FS_Open( filename, "wt" );
			if ( !file )
			{
				Con_Printf( "Couldn't open [%s] for writing!\n", filename );
				return;
			}
			if ( argumentCount == 3 )
			{
				partial = Cmd_Argv( 2 );
				partialLength = Q_strlen( partial );
			}
		}
		else if ( Q_strcasecmp( Cmd_Argv( 1 ), "-a" ) == 0 )
			archiveOnly = true;
		else if ( Q_strcasecmp( Cmd_Argv( 1 ), "-s" ) == 0 )
			serverOnly = true;
		else
		{
			partial = Cmd_Argv( 1 ); // Save pointer
			partialLength = Q_strlen( partial );
		}
	}

	Con_Printf( "CVar List\n--------------\n" );
	for ( cvar_t* variable = cvar_vars; variable; variable = variable->next )
	{
		if ( archiveOnly && !( variable->flags & FCVAR_ARCHIVE ) )
			continue;
		if ( serverOnly && !( variable->flags & FCVAR_SERVER ) )
			continue;
		if ( partial && Q_strnicmp( variable->name, partial, partialLength ) != 0 )
			continue;
		Cmd_CvarListPrintCvar( variable, file );
		++count;
	}
	if ( argumentCount == 2 && partial && *partial )
		Con_Printf( "--------------\n%3i CVars for [%s]\nCvarList ? for syntax\n", count, partial );
	else
		Con_Printf( "--------------\n%3i Total CVars\nCvarList ? for syntax\n", count );
	if ( file )
	{
		FS_Close( file );
		Con_Printf( "cvarlist logged to %s\n", filename );
	}
}

void Cvar_Init()
{
}

qboolean Cvar_CommandWithPrivilegeCheck( qboolean privileged )
{
	const char* firstToken = Cvar_IsMultipleTokens( Cmd_Argv( 0 ) );
	if ( firstToken )
	{
		cvar_t* variable = Cvar_FindVar( firstToken );
		if ( !variable )
			return 0;
		Con_Printf( "\"%s\" is \"%s\"\n", variable->name, variable->string );
		return 1;
	}

	cvar_t* variable = Cvar_FindVar( Cmd_Argv( 0 ) );
	if ( !variable )
		return 0;
	if ( !privileged )
	{
		if ( variable->flags & FCVAR_PRIVILEGED )
		{
			Con_Printf( "%s is a privileged variable\n", variable->name );
			return 1;
		}
		cvar_t* filter = Cvar_FindVar( "cl_filterstuffcmd" );
		if ( filter && Q_atof( filter->string ) > 0.0f )
		{
			if ( variable->flags & FCVAR_FILTERSTUFFTEXT )
			{
				Con_Printf( "%s is a privileged variable\n", variable->name );
				return 1;
			}
			const char* const filterPrefixes[] = { "cl_", "gl_", "m_", "r_", "hud_" };
			for ( const char* prefix : filterPrefixes )
			{
				const char* match = Q_stristr( variable->name, prefix );
				if ( match == variable->name )
				{
					Con_Printf( "%s is a privileged variable\n", variable->name );
					return 1;
				}
			}
		}
	}

	if ( Cmd_Argc() == 1 )
		Con_Printf( "\"%s\" is \"%s\"\n", variable->name, variable->string );
	else if ( !( variable->flags & FCVAR_SPONLY ) || cls.state == ca_dedicated || cls.state == ca_disconnected
			  || ( ( variable->flags & FCVAR_SERVER ) && Host_IsServerActive() ) || cl.maxclients <= 1 )
	{
		const char* value = Cmd_Argv( 1 );
		Cvar_Set( variable->name, value );
	}
	else
		Con_Printf( "Can't set %s in multiplayer\n", variable->name );
	return 1;
}

/*
============
Cvar_FindVar
============
*/
cvar_t* Cvar_FindVar( const char* name )
{
	g_engdstAddrs.pfnGetCvarPointer( &name );

	for ( cvar_t* var = cvar_vars; var; var = var->next )
	{
		if ( Q_strcasecmp( name, var->name ) == 0 )
			return var;
	}

	return nullptr;
}

void Cvar_RegisterVariable( cvar_t* variable )
{
// first check to see if it has allready been defined
	if ( Cvar_FindVar( variable->name ) )
	{
		Con_Printf( "Can't register variable %s, already defined\n", variable->name );
		return;
	}

// check for overlap with a command
	if ( Cmd_Exists( variable->name ) )
	{
		Con_Printf( "Cvar_RegisterVariable: %s is a command\n", variable->name );
		return;
	}

	char* oldString = variable->string;
	std::uint32_t stringBytes = static_cast<std::uint32_t>( Q_strlen( oldString ) );
	++stringBytes;
	int allocationSize = static_cast<int>( stringBytes );
	variable->string = static_cast<char*>( Z_Malloc( allocationSize ) );
	Q_strcpy( variable->string, oldString );
	variable->value = Q_atof( variable->string );

	cvar_t dummy;
	dummy.next = cvar_vars;
	cvar_t* previous = &dummy;
	cvar_t* current = dummy.next;
	while ( current && _stricmp( current->name, variable->name ) <= 0 )
	{
		previous = current;
		current = current->next;
	}

	previous->next = variable;
	variable->next = current;
	cvar_vars = dummy.next;
}

const char* Cvar_VariableString( const char* name )
{
	cvar_t* var = Cvar_FindVar( name );
	if ( !var )
		return cvar_null_string;

	return var->string;
}

float Cvar_VariableValue( const char* name )
{
	cvar_t* var = Cvar_FindVar( name );
	if ( !var )
		return 0.0f;

	return Q_atof( var->string );
}

void Cvar_ResetList()
{
	cvar_vars = nullptr;
}

void Cvar_UnlinkExternals()
{
	cvar_t** link = &cvar_vars;
	for ( cvar_t* var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & FCVAR_EXTDLL )
			*link = var->next;
		else
			link = &var->next;
	}
}

void Cvar_RemoveHudCvars()
{
	cvar_t* retained = nullptr;
	for ( cvar_t* var = cvar_vars; var; )
	{
		cvar_t* next = var->next;
		if ( var->flags & FCVAR_CLIENTDLL )
		{
			Z_Free( var->string );
			Z_Free( var );
		}
		else
		{
			var->next = retained;
			retained = var;
		}
		var = next;
	}

	cvar_vars = nullptr;
	while ( retained )
	{
		cvar_t* next = retained->next;
		retained->next = cvar_vars;
		cvar_vars = retained;
		retained = next;
	}
}

void Cvar_WriteVariables( FileHandle_t file )
{
	for ( cvar_t* var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & FCVAR_ARCHIVE )
			FS_FPrintf( file, "%s \"%s\"\n", var->name, var->string );
	}
}

qboolean Cvar_HookVariable( const char* name, cvarhook_t* hook )
{
	if ( !hook || !hook->hook || hook->cvar || hook->next )
		return 0;

	cvar_t* var = Cvar_FindVar( name );
	if ( !var )
		return 0;

	hook->cvar = var;
	if ( !cvar_hooks )
	{
		cvar_hooks = hook;
		return 1;
	}

	cvarhook_t* last = cvar_hooks;
	while ( last->next )
		last = last->next;
	last->next = hook;
	return 1;
}

void Cvar_Set( const char* name, const char* value )
{
	cvar_t* var = Cvar_FindVar( name );
	if ( !var )
	{
		Con_DPrintf( "Cvar_Set: variable %s not found\n", name );
		return;
	}

	Cvar_DirectSet( var, value );
	for ( cvarhook_t* hook = cvar_hooks; hook; hook = hook->next )
	{
		if ( hook->cvar == var )
		{
			hook->hook( var );
			return;
		}
	}
}

void Cvar_SetValue( char* name, float value )
{
	constexpr std::size_t CVAR_VALUE_BUFFER_SIZE = 32;
	constexpr double CVAR_INTEGER_EPSILON = 0.000001;

	g_engdstAddrs.Cvar_SetValue( &name, &value );
#if ( defined( HL25_HW_ENGINE ) || defined( SW_RENDERER ) ) && defined( _WIN32 )
	const int integerValue = _mm_cvtt_ss2si( _mm_set_ss( value ) );
#else
	const int integerValue = static_cast<int>( value );
#endif
	const float fractionalPart = value - static_cast<float>( integerValue );
	char text[CVAR_VALUE_BUFFER_SIZE];
	if ( std::fabs( static_cast<double>( fractionalPart ) ) < CVAR_INTEGER_EPSILON )
		_snprintf( text, sizeof( text ), "%d", integerValue );
	else
		_snprintf( text, sizeof( text ), "%f", static_cast<double>( value ) );

	Cvar_Set( name, text );
}

void Cvar_DirectSet( cvar_t* variable, const char* value )
{
	constexpr int CVAR_FILTER_BUFFER_SIZE = 1024;
	constexpr int CVAR_SERVERINFO_SIZE = 512;

	if ( !variable || !value )
		return;
	if ( ( variable->flags & FCVAR_NOBADPATHS )
		 && ( *value == '/' || Q_strstr( value, ":" ) || Q_strstr( value, ".." ) || Q_strstr( value, "\\" ) ) )
	{
		Con_Printf( "Refusing to set %s to %s (invalid path)\n", variable->name, value );
		return;
	}

	char filtered[CVAR_FILTER_BUFFER_SIZE];
	const char* newValue = value;
	if ( variable->flags & FCVAR_PRINTABLEONLY )
	{
		filtered[0] = '\0';
		char* cursor = filtered;
		if ( Q_UnicodeValidate( value ) )
			Q_strncpy_s( filtered, value, sizeof( filtered ) );
		else
		{
			for ( const char* input = value; *input; ++input )
			{
				if ( *input >= ' ' && *input <= '~' )
					*cursor++ = *input;
			}
			*cursor = '\0';
		}

		if ( !Q_UnicodeValidate( cursor ) )
			Q_UnicodeRepair( cursor );
		if ( !Q_strlen( filtered ) )
			Q_strcpy( filtered, "empty" );
		newValue = filtered;
	}
	if ( variable->flags & FCVAR_NOEXTRAWHITEPACE )
	{
		if ( newValue != filtered )
			Q_strncpy_s( filtered, newValue, sizeof( filtered ) );
		Q_StripUnprintableAndSpace( filtered );
		newValue = filtered;
	}
	if ( variable->flags & FCVAR_FILTERCHARS )
	{
		if ( newValue != filtered )
			Q_strncpy_s( filtered, newValue, sizeof( filtered ) );
		filtered[std::strcspn( filtered, ";\n" )] = '\0';
		newValue = filtered;
	}

	const qboolean changed = Q_strcmp( variable->string, newValue );
	if ( variable->flags & FCVAR_USERINFO )
	{
		if ( cls.state == ca_dedicated )
		{
			Info_SetValueForKey( Info_Serverinfo(), variable->name, newValue, CVAR_SERVERINFO_SIZE );
			SV_BroadcastCommand( "fullserverinfo \"%s\"\n", Info_Serverinfo() );
		}
		if ( cls.state != ca_dedicated )
		{
			Info_SetValueForKey( cls.userinfo, variable->name, newValue, sizeof( cls.userinfo ) );
			if ( changed && cls.state >= ca_connected )
			{
				MSG_WriteByte( &cls.netchan.message, clc_stringcmd );
				SZ_Print( &cls.netchan.message, va( "setinfo \"%s\" \"%s\"\n", variable->name, newValue ) );
			}
		}
	}
	if ( ( variable->flags & FCVAR_SERVER ) && changed )
	{
		if ( !( variable->flags & FCVAR_UNLOGGED ) )
		{
			const char* displayValue = variable->flags & FCVAR_PROTECTED ? "***PROTECTED***" : newValue;
			Log_Printf( "Server cvar \"%s\" = \"%s\"\n", variable->name, displayValue );
			SV_BroadcastPrintf( "\"%s\" changed to \"%s\"\n", variable->name, displayValue );
		}
		const char* reportedValue = newValue;
		if ( variable->flags & FCVAR_PROTECTED )
			reportedValue = Q_strlen( newValue ) > 0 && Q_strcasecmp( newValue, "none" ) != 0 ? "1" : "0";
		Steam_SetCVar( variable->name, reportedValue );
	}

	Z_Free( variable->string );
	std::uint32_t stringBytes = static_cast<std::uint32_t>( Q_strlen( newValue ) );
	++stringBytes;
	variable->string = static_cast<char*>( Z_Malloc( static_cast<int>( stringBytes ) ) );
	Q_strcpy( variable->string, newValue );
	variable->value = Q_atof( variable->string );
}

void Cvar_Shutdown()
{
	Cvar_ResetList();
}

cvar_t* Cvar_FindPrevVar( char* var_name )
{
	cvar_t* previous = cvar_vars;
	cvar_t* next = cvar_vars->next;
	if ( !next )
		return nullptr;
	while ( Q_stricmp( var_name, next->name ) != 0 )
	{
		previous = previous->next;
		next = previous->next;
		if ( !next )
			return nullptr;
	}
	return previous;
}

int Cvar_VariableInt( char* var_name )
{
	cvar_t* var = Cvar_FindVar( var_name );
	return var ? Q_atoi( var->string ) : 0;
}

char* Cvar_CompleteVariable( char* search, int forward )
{
	constexpr int CVAR_PARTIAL_BYTES = 256;
	static char lastPartial[CVAR_PARTIAL_BYTES];
	char partial[CVAR_PARTIAL_BYTES];
	Q_strncpy( partial, search, CVAR_PARTIAL_BYTES - 1 );
	partial[CVAR_PARTIAL_BYTES - 1] = 0;

	const int length = Q_strlen( partial );
	if ( length == 0 )
		return nullptr;

	int prefixLength = length - 1;
	if ( partial[length - 1] == ' ' )
	{
		while ( true )
		{
			partial[prefixLength] = 0;
			if ( partial[prefixLength - 1] != ' ' )
				break;
			--prefixLength;
		}
	}
	else
		prefixLength = length;

	cvar_t* next = nullptr;
	if ( Q_stricmp( partial, lastPartial ) == 0 )
	{
		cvar_t* found = Cvar_FindVar( partial );
		if ( !cvar_vars )
			return nullptr;

		if ( found )
		{
			if ( forward == 1 )
				next = found->next;
			else if ( cvar_vars->next )
				next = Cvar_FindPrevVar( found->name );
		}

		if ( next )
		{
			Q_strncpy( lastPartial, next->name, CVAR_PARTIAL_BYTES - 1 );
			lastPartial[CVAR_PARTIAL_BYTES - 1] = 0;
			return next->name;
		}
	}

	next = cvar_vars;
	if ( !next )
		return nullptr;
	while ( Q_strncmp( partial, next->name, prefixLength ) != 0 )
	{
		next = next->next;
		if ( !next )
			return nullptr;
	}

	Q_strncpy( lastPartial, next->name, CVAR_PARTIAL_BYTES - 1 );
	lastPartial[CVAR_PARTIAL_BYTES - 1] = 0;
	return next->name;
}

const char* Cvar_IsMultipleTokens( const char* varname )
{
	constexpr int CVAR_FIRST_TOKEN_BYTES = 516;
	static char firstToken[CVAR_FIRST_TOKEN_BYTES];
	int tokenCount = 0;
	firstToken[0] = 0;
	while ( true )
	{
		varname = COM_Parse( varname );
		if ( Q_strlen( com_token ) <= 0 )
			break;
		if ( tokenCount != 0 )
			++tokenCount;
		else
		{
			tokenCount = 1;
			Q_strncpy( firstToken, com_token, CVAR_FIRST_TOKEN_BYTES - 1 );
			firstToken[CVAR_FIRST_TOKEN_BYTES - 1] = 0;
		}
	}
	if ( tokenCount == 1 )
		return nullptr;
	return firstToken;
}

qboolean Cvar_Command()
{
	return Cvar_CommandWithPrivilegeCheck( 1 );
}

int Cvar_CountServerVariables()
{
	int count = 0;
	for ( cvar_t* var = cvar_vars; var; var = var->next )
	{
		if ( var->flags & FCVAR_SERVER )
			++count;
	}
	return count;
}

void Cvar_CmdInit()
{
	Cmd_AddCommand( const_cast<char*>( "cvarlist" ), Cmd_CvarList_f );
// TODO, additional cleanup
}
