// cmd.c -- Quake script command processing module
#include "cmd.h"
#include "engine_apiproxy.h"
#include "client.h"
#include "common.h"
#include "conprint.h"
#include "cvar.h"
#include "filesystem.h"
#include "mem.h"
#include "protocol.h"
#include "server.h"
#include "sys.h"
#include "zone.h"

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "buildslave_assert.h"
#define ASSERT_FILE BUILDSLAVE_PATH( L"engine\\cmd.c" )

#if !defined( _WIN32 )
#ifndef _snprintf
#define _snprintf snprintf
#endif
#ifndef _vsnprintf
#define _vsnprintf vsnprintf
#endif
#endif

#define CMD_FLAG_HUD 1
#define CMD_FLAG_GAME 2
#define CMD_FLAG_WRAPPER 4

sizebuf_t cmd_text;
sizebuf_t cmd_text_untrusted;
cmd_function_t* cmd_functions; // possible commands to execute
cmdalias_t* cmd_alias;
cmd_source_t cmd_source;
int cmd_argc;
char* cmd_argv[MAX_CMD_ARGS];
char* cmd_args;
const char* command_argv_empty = com_empty_string;
static qboolean s_bCurrentCommandIsPrivileged = TRUE;
qboolean cmd_wait;

/*
=============================================================================

						COMMAND BUFFER

=============================================================================
*/

/*
============
Cbuf_Init
============
*/
void Cbuf_Init()
{
	// space for commands and script files
	SZ_Alloc( "cmd_text", &cmd_text, 16384 );
	SZ_Alloc( "filteredcmd_text", &cmd_text_untrusted, 16384 );
}

static void Cbuf_AddTextToBuffer( const char* text, sizebuf_t* buf )
{
	int l;

	l = Q_strlen( text );

	if ( buf->cursize + l >= buf->maxsize )
	{
		Con_Printf( "Cbuf_AddTextToBuffer: overflow\n" );
		return;
	}

	SZ_Write( buf, text, l );
}

void Cbuf_AddTextToUntrustedBuffer( const char* text )
{
	Cbuf_AddTextToBuffer( text, &cmd_text_untrusted );
}

/*
============
Cbuf_AddText

Adds command text at the end of the buffer
============
*/
void Cbuf_AddText( const char* text )
{
	Cbuf_AddTextToBuffer( text, &cmd_text );
}

void Cbuf_AddFilteredText( char* text )
{
	Cbuf_AddTextToUntrustedBuffer( text );
}

void Cbuf_InsertText( const char* text )
{
	Cbuf_InsertTextToBuffer( text, &cmd_text, FALSE );
}

void Cbuf_InsertTextLines( char* text )
{
	Cbuf_InsertTextToBuffer( text, &cmd_text, TRUE );
}

void Cbuf_InsertTextToBuffer( const char* text, sizebuf_t* buf, qboolean addnewlines )
{
	void* temp;
	int templen;
	int l;

	templen = buf->cursize;

	l = Q_strlen( text );
	if ( addnewlines )
		l += 2;

	if ( templen + l >= buf->maxsize )
	{
		Con_Printf( "Cbuf_InsertText: overflow\n" );
		return;
	}

	// copy off any commands still remaining in the exec buffer
	temp = NULL;
	if ( templen )
	{
		temp = Z_Malloc( templen );
		Q_memcpy( temp, buf->data, templen );
		SZ_Clear( buf );
	}

	// add the entire text of the file
	if ( addnewlines )
		Cbuf_AddTextToBuffer( "\n", buf );
	Cbuf_AddTextToBuffer( text, buf );
	if ( addnewlines )
		Cbuf_AddTextToBuffer( "\n", buf );

	// add the copied off data
	if ( templen )
	{
		SZ_Write( buf, temp, templen );
		Z_Free( temp );
	}
}

void Cbuf_ExecuteCommandsFromBuffer( sizebuf_t* buf, qboolean bIsPrivileged, int maxcmds )
{
	int i, len;
	char* text;
	char line[1024];
	int quotes;
	int count;

	if ( !buf )
		return;

	count = 0;

	while ( buf->cursize )
	{
		// find a \n or ; line break
		text = (char*)buf->data;

		quotes = 0;
		for ( i = 0; i < buf->cursize; i++ )
		{
			if ( text[i] == '"' )
				quotes++;
			if ( !( quotes & 1 ) && text[i] == ';' )
				break; // don't break if inside a quoted string
			if ( text[i] == '\n' )
				break;
		}

		len = i;
		if ( len >= (int)sizeof( line ) )
			len = sizeof( line ) - 1;

		Q_memcpy( line, text, len );
		line[len] = 0;

		// delete the text from the command buffer and move remaining commands down
		// this is necessary because commands (exec, alias) can insert data at the
		// beginning of the text buffer
		if ( i == buf->cursize )
			buf->cursize = 0;
		else
		{
			i++;
			buf->cursize -= i;
			memmove( text, text + i, buf->cursize );
		}

		// execute the command line
		Cmd_ExecuteStringWithPrivilegeCheck( line, src_command, bIsPrivileged );
		count++;

		if ( cmd_wait || ( maxcmds > 0 && count >= maxcmds ) )
		{
			// skip out while text still remains in buffer, leaving it
			// for next frame
			cmd_wait = FALSE;
			return;
		}
	}
}

/*
=============================================================================

					COMMAND EXECUTION

=============================================================================
*/

/*
============
Cmd_Argc
============
*/
int Cmd_Argc()
{
	g_engdstAddrs.Cmd_Argc();

	return cmd_argc;
}

/*
============
Cmd_Args
============
*/
const char* Cmd_Args()
{
	return cmd_args;
}

/*
============
Cmd_Argv
============
*/
const char* Cmd_Argv( int arg )
{
	g_engdstAddrs.Cmd_Argv( &arg );

	if ( (unsigned int)arg >= (unsigned int)cmd_argc )
		return command_argv_empty;
	return cmd_argv[arg];
}

/*
============
Cmd_TokenizeString

Parses the given string into command line tokens.
============
*/
void Cmd_TokenizeString( const char* text )
{
	int i;
	int len;

	// clear the args from the last string
	for ( i = 0; i < cmd_argc; i++ )
		Z_Free( cmd_argv[i] );

	cmd_args = NULL;
	cmd_argc = 0;

	while ( 1 )
	{
		// skip whitespace up to a /n
		while ( *text && (unsigned char)*text <= ' ' && *text != '\n' )
			text++;

		if ( !*text || *text == '\n' )
			return;

		if ( cmd_argc == 1 )
			cmd_args = (char*)text;

		text = COM_Parse( text );
		if ( !text )
			return;

		len = Q_strlen( com_token ) + 1;
		if ( len >= 516 )
			return;

		if ( cmd_argc < MAX_CMD_ARGS )
		{
			cmd_argv[cmd_argc] = (char*)Z_Malloc( len );
			Q_strcpy( cmd_argv[cmd_argc], com_token );
			cmd_argc++;
		}
	}
}

void Cmd_ExecuteStringWithPrivilegeCheck( const char* text, cmd_source_t src, qboolean bIsPrivileged )
{
	cmd_function_t* cmd;
	cmdalias_t* a;
	qboolean bBlocked;
	int i;

	cmd_source = src;
	Cmd_TokenizeString( text );

	// execute the command line
	if ( !Cmd_Argc() )
		return; // no tokens

	// check functions
	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( Q_strcasecmp( cmd_argv[0], cmd->name ) )
			continue;

		if ( !bIsPrivileged )
		{
			bBlocked = ( cmd->flags & CMD_FLAG_PRIVILEGED ) ? TRUE : FALSE;

			if ( !bBlocked && Cvar_VariableValue( "cl_filterstuffcmd" ) > 0.0f )
			{
				const char* filtered[] = { "cl_", "gl_", "m_", "r_", "hud_" };

				bBlocked = ( cmd->flags & CMD_FLAG_FILTERED ) ? TRUE : FALSE;

				for ( i = 0; !bBlocked && i < (int)ARRAYSIZE( filtered ); i++ )
				{
					if ( Q_stristr( cmd->name, filtered[i] ) == cmd->name )
						bBlocked = TRUE;
				}
			}

			if ( bBlocked )
			{
				Con_Printf( "Could not execute privileged command %s\n", text );
				return;
			}
		}

		s_bCurrentCommandIsPrivileged = bIsPrivileged;
		cmd->function();
		s_bCurrentCommandIsPrivileged = TRUE;

		if ( cls.demorecording && ( cmd->flags & CMD_FLAG_HUD ) && !cls.spectator )
			CL_RecordHUDCommand( cmd->name );

		return;
	}

	// check alias
	for ( a = cmd_alias; a; a = a->next )
	{
		if ( !Q_strcasecmp( cmd_argv[0], a->name ) )
		{
			Cbuf_InsertTextToBuffer( a->value, bIsPrivileged ? &cmd_text : &cmd_text_untrusted, FALSE );
			return;
		}
	}

	// check cvars
	if ( Cvar_CommandWithPrivilegeCheck( bIsPrivileged ) )
		return;

	// forward the command line to the server, so the entity DLL can parse it
	if ( cls.state == ca_active || cls.state == ca_connected || cls.state == ca_uninitialized )
		Cmd_ForwardToServer();
}

void Cmd_ExecuteString( const char* text, cmd_source_t src )
{
	Cmd_ExecuteStringWithPrivilegeCheck( text, src, TRUE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *in - 
// Output : char *CopyString
//-----------------------------------------------------------------------------
char* CopyString( char* in )
{
	char* out;

	out = (char*)Z_Malloc( Q_strlen( in ) + 1 );
	Q_strcpy( out, in );
	return out;
}

/*
============
Cmd_Exists
============
*/
qboolean Cmd_Exists( const char* cmd_name )
{
	cmd_function_t* cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !Q_strcmp( cmd_name, cmd->name ) )
			return TRUE;
	}

	return FALSE;
}

cmd_function_t* Cmd_FindCmd( char* cmd_name )
{
	cmd_function_t* cmd;

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( !Q_strcmp( cmd_name, cmd->name ) )
			return cmd;
	}

	return NULL;
}

cmd_function_t* Cmd_FindCmdPrev( char* cmd_name )
{
	cmd_function_t* cmd;

	for ( cmd = cmd_functions; cmd->next; cmd = cmd->next )
	{
		if ( !Q_strcmp( cmd_name, cmd->next->name ) )
			return cmd;
	}

	return NULL;
}

char* Cmd_CompleteCommand( char* search, int forward )
{
	static char lastpartial[256];
	char partial[256];
	cmd_function_t* cmd = NULL;
	qboolean bSearch;
	int len;

	Q_strncpy( partial, search, sizeof( partial ) - 1 );
	partial[sizeof( partial ) - 1] = 0;

	len = Q_strlen( partial );
	if ( !len )
		return NULL;

	if ( partial[len - 1] == ' ' )
	{
		do
		{
			len--;
			partial[len] = 0;
		} while ( partial[len - 1] == ' ' );
	}

	bSearch = TRUE;

	if ( !Q_stricmp( partial, lastpartial ) )
	{
		if ( !cmd_functions )
			return NULL;

		cmd = Cmd_FindCmd( partial );
		if ( cmd )
		{
			if ( forward == 1 )
				cmd = cmd->next;
			else
				cmd = Cmd_FindCmdPrev( cmd->name );
		}

		if ( cmd )
			bSearch = FALSE;
	}

	if ( bSearch )
	{
		// check functions
		for ( cmd = cmd_functions; cmd; cmd = cmd->next )
		{
			if ( !Q_strncmp( partial, cmd->name, len ) )
				break;
		}

		if ( !cmd )
			return NULL;
	}

	Q_strncpy( lastpartial, cmd->name, sizeof( lastpartial ) - 1 );
	lastpartial[sizeof( lastpartial ) - 1] = 0;
	return cmd->name;
}

/*
================
Cmd_CheckParm

Returns the position (1 to argc-1) in the command's argument list
where the given parameter apears, or 0 if not present
================
*/
int Cmd_CheckParm( char* parm )
{
	int i;

	if ( !parm )
		Sys_Error( "Cmd_CheckParm: NULL" );

	for ( i = 1; i < Cmd_Argc(); i++ )
	{
		if ( !Q_strcasecmp( parm, Cmd_Argv( i ) ) )
			return i;
	}

	return 0;
}

/*
===============
Cmd_Echo_f

Just prints the rest of the line to the console
===============
*/
void Cmd_Echo_f()
{
	int i;

	for ( i = 1; i < Cmd_Argc(); i++ )
		Con_Printf( "%s ", Cmd_Argv( i ) );
	Con_Printf( "\n" );
}

void Cmd_CmdList_f()
{
	cmd_function_t* cmd;
	int iCmds;
	int iArgs;
	const char* partial;
	int ipLen;
	char szTemp[256];
	FileHandle_t f;
	qboolean bLogging;

	iCmds = 0;
	partial = NULL;
	ipLen = 0;
	f = NULL;
	bLogging = FALSE;

	iArgs = Cmd_Argc();
	if ( iArgs >= 2 )
	{
		if ( !Q_strcasecmp( Cmd_Argv( 1 ), "?" ) )
		{
			Con_Printf( "CmdList           : List all commands\n"
						"CmdList [Partial] : List commands starting with 'Partial'\n"
						"CmdList log [Partial] : Logs commands to file \"cmdlist.txt\" in the gamedir.\n" );
			return;
		}

		if ( !Q_strcasecmp( Cmd_Argv( 1 ), "log" ) )
		{
			_snprintf( szTemp, sizeof( szTemp ), "cmdlist.txt" );

			f = FS_Open( szTemp, "wt" );
			if ( !f )
			{
				Con_Printf( "Couldn't open [%s] for writing!\n", szTemp );
				return;
			}
			bLogging = TRUE;

			if ( iArgs == 3 )
			{
				partial = Cmd_Argv( 2 );
				ipLen = Q_strlen( partial );
			}
		}
		else
		{
			partial = Cmd_Argv( 1 );
			ipLen = Q_strlen( partial );
		}
	}

	// Banner
	Con_Printf( "Command List\n--------------\n" );

	for ( cmd = cmd_functions; cmd; cmd = cmd->next )
	{
		if ( partial && Q_strnicmp( cmd->name, partial, ipLen ) ) // Partial string searching?
			continue;

		Con_Printf( "%-16.16s\n", cmd->name );
		if ( bLogging )
			FS_FPrintf( f, "%-16.16s\n", cmd->name );
		iCmds++;
	}

	// Show total and syntax help...
	if ( iArgs == 2 && partial && partial[0] )
		Con_Printf( "--------------\n%3i Commands for [%s]\nCmdList ? for syntax\n", iCmds, partial );
	else
		Con_Printf( "--------------\n%3i Total Commands\nCmdList ? for syntax\n", iCmds );

	if ( bLogging )
		FS_Close( f );
}

/*
===============
Cmd_Exec_f
===============
*/
void Cmd_Exec_f()
{
	char* pszFileName;
	char* pszFileData;
	const char* pszDataPtr;
	unsigned int nSize;
	FileHandle_t f;
	int i;

	if ( Cmd_Argc() != 2 )
	{
		Con_Printf( "exec <filename> : execute a script file\n" );
		return;
	}

	pszFileName = (char*)Cmd_Argv( 1 );
	if ( !pszFileName )
		return;

	if ( Q_strstr( pszFileName, "\\" ) || Q_strstr( pszFileName, ":" ) || Q_strstr( pszFileName, "~" )
		|| Q_strstr( pszFileName, ".." ) || pszFileName[0] == '/' )
	{
		Con_Printf( "exec %s: invalid path.\n", pszFileName );
		return;
	}

	if ( strchr( pszFileName, '.' ) != strrchr( pszFileName, '.' ) )
	{
		Con_Printf( "exec %s: invalid filename.\n", pszFileName );
		return;
	}

	if ( Q_strcmp( COM_LastFileExtension( pszFileName ), "cfg" )
		&& Q_strcmp( COM_LastFileExtension( pszFileName ), "rc" ) )
	{
		Con_Printf( "exec %s: not a .cfg or .rc file\n", pszFileName );
		return;
	}

	if ( !s_bCurrentCommandIsPrivileged )
	{
#if defined( _WIN32 )
		constexpr int EXEC_MAP_CONFIG_BYTES = 260;
#else
		constexpr int EXEC_MAP_CONFIG_BYTES = 4096;
#endif
		char szMapCfg[EXEC_MAP_CONFIG_BYTES];
		char szMapName[EXEC_MAP_CONFIG_BYTES];

		Q_memset( szMapCfg, 0, sizeof( szMapCfg ) );

		const char* whitelist[] = { "mapdefault.cfg", "scout.cfg", "sniper.cfg", "soldier.cfg", "demoman.cfg",
			"medic.cfg", "hwguy.cfg", "pyro.cfg", "spy.cfg", "engineer.cfg", "civilian.cfg", szMapCfg };

		if ( COM_FileBase_s( cl.levelname, szMapName, sizeof( szMapName ) ) )
			_snprintf( szMapCfg, sizeof( szMapCfg ), "%s.cfg", szMapName );

		for ( i = 0; i < (int)ARRAYSIZE( whitelist ); i++ )
		{
			if ( !Q_strcmp( pszFileName, whitelist[i] ) )
				break;
		}

		if ( i == (int)ARRAYSIZE( whitelist ) )
		{
			Con_Printf( "exec %s: not privileged or in whitelist\n", pszFileName );
			return;
		}
	}

	f = FS_OpenPathID( pszFileName, "rb", "GAMECONFIG" );
	if ( !f )
		f = FS_OpenPathID( pszFileName, "rb", "GAME" );
	if ( !f )
		f = FS_Open( pszFileName, "rb" );

	if ( !f )
	{
		if ( !Q_strstr( pszFileName, "autoexec.cfg" ) && !Q_strstr( pszFileName, "userconfig.cfg" )
			&& !Q_strstr( pszFileName, "hw/opengl.cfg" ) && !Q_strstr( pszFileName, "joystick.cfg" )
			&& !Q_strstr( pszFileName, "game.cfg" ) )
			Con_Printf( "couldn't exec %s\n", pszFileName );
		return;
	}

	nSize = FS_Size( f );
	pszFileData = (char*)Mem_Malloc( nSize + 1 );
	if ( !pszFileData )
	{
		Con_Printf( "exec: not enough space for %s", pszFileName );
		FS_Close( f );
		return;
	}

	FS_Read( pszFileData, nSize, 1, f );
	pszFileData[nSize] = 0;
	FS_Close( f );

	Con_DPrintf( "execing %s\n", pszFileName );

	if ( (int)( nSize + cmd_text.cursize + 2 ) < cmd_text.maxsize )
	{
		Cbuf_InsertTextToBuffer( pszFileData, &cmd_text, TRUE );
	}
	else
	{
		pszDataPtr = pszFileData;
		while ( 1 )
		{
			Cbuf_Execute();

			pszDataPtr = COM_ParseLine( pszDataPtr );
			if ( Q_strlen( com_token ) <= 0 )
				break;

			Cbuf_InsertTextToBuffer( com_token, &cmd_text, TRUE );
		}
	}

	Mem_Free( pszFileData );
}

int Cmd_ForwardToServerInternal( sizebuf_t* pBuf )
{
	byte tempData[4096];
	sizebuf_t tempBuf;

	tempBuf.data = tempData;
	tempBuf.buffername = "Cmd_ForwardToServerInternal::tempBuf";
	tempBuf.flags = FSB_ALLOWOVERFLOW;
	tempBuf.maxsize = sizeof( tempData );
	tempBuf.cursize = 0;

	if ( cls.state != ca_connected && cls.state != ca_uninitialized && cls.state != ca_active )
	{
		if ( Q_strcasecmp( Cmd_Argv( 0 ), "setinfo" ) )
			Con_Printf( "Can't \"%s\", not connected\n", Cmd_Argv( 0 ) );
		return 0;
	}

	if ( cls.demoplayback || g_bIsDedicatedServer )
		return 0; // not really connected

	MSG_WriteByte( &tempBuf, clc_stringcmd );

	if ( Q_strcasecmp( Cmd_Argv( 0 ), "cmd" ) )
	{
		SZ_Print( &tempBuf, Cmd_Argv( 0 ) );
		SZ_Print( &tempBuf, " " );
	}

	if ( Cmd_Argc() > 1 )
		SZ_Print( &tempBuf, Cmd_Args() );
	else
		SZ_Print( &tempBuf, "\n" );

	if ( tempBuf.flags & FSB_OVERFLOWED )
	{
#if defined( _WIN32 )
		ASSERT_AT( !"Cmd_ForwardToServerInternal: buffer overflowed", 1347 );
#endif
	}

	if ( tempBuf.cursize + pBuf->cursize > pBuf->maxsize )
		return 0;

	SZ_Write( pBuf, tempBuf.data, tempBuf.cursize );
	return 1;
}

/*
===================
Cmd_ForwardToServer

Sends the entire command line over to the server
===================
*/
void Cmd_ForwardToServer()
{
	if ( !Q_strcasecmp( Cmd_Argv( 0 ), "cmd" ) && !Q_strcasecmp( Cmd_Argv( 1 ), "dlfile" ) )
		return;

	Cmd_ForwardToServerInternal( &cls.netchan.message );
}

int Cmd_ForwardToServerUnreliable()
{
	return Cmd_ForwardToServerInternal( &cls.datagram );
}

/*
===============
Cmd_StuffCmds_f

Adds command line parameters as script statements
Commands lead with a +, and continue until a - or another +
quake +prog jctest.qp +cmd amlev1
quake -nosound +cmd amlev1
===============
*/
//-----------------------------------------------------------------------------
// Purpose: Adds command line parameters as script statements
// Commands lead with a +, and continue until a - or another +
// Also automatically converts .dem, .bsp, and .sav files to +playdemo etc command line options
// hl2 +cmd amlev1
// hl2 -nosound +cmd amlev1
// Output : void Cmd_StuffCmds_f
//-----------------------------------------------------------------------------
void Cmd_StuffCmds_f()
{
	int i, j;
	int s;
	char* text, * build, c;

	if ( Cmd_Argc() != 1 )
	{
		Con_Printf( "stuffcmds : execute command line parameters\n" );
		return;
	}

	// build the combined string to parse from
	s = 0;
	for ( i = 1; i < com_argc; i++ )
	{
		if ( !com_argv[i] )
			continue; // NEXTSTEP nulls out -NXHost
		s += Q_strlen( com_argv[i] ) + 1;
	}
	if ( !s )
		return;

	text = (char*)Z_Malloc( s + 1 );
	text[0] = 0;
	for ( i = 1; i < com_argc; i++ )
	{
		if ( !com_argv[i] )
			continue; // NEXTSTEP nulls out -NXHost
		strncat( text, com_argv[i], s - strlen( text ) - 1 );
		if ( i != com_argc - 1 )
			Q_strcat( text, " " );
	}

	// pull out the commands
	build = (char*)Z_Malloc( s + 1 );
	build[0] = 0;

	for ( i = 0; i < s - 1; i++ )
	{
		if ( text[i] == '+' )
		{
			i++;

			for ( j = i; text[j] && text[j] != '+' && text[j] != '-'; j++ )
				;

			c = text[j];
			text[j] = 0;

			strncat( build, text + i, s - strlen( build ) - 2 );
			Q_strcat( build, "\n" );
			text[j] = c;
			i = j - 1;
		}
	}

	if ( build[0] )
		Cbuf_InsertText( build );

	Z_Free( text );
	Z_Free( build );
}

cmd_function_t* Cmd_GetFirstCmd()
{
	return cmd_functions;
}

qboolean Cmd_CurrentCommandIsPrivileged()
{
	return s_bCurrentCommandIsPrivileged;
}

void Cmd_AddCommandWithFlags( char* cmd_name, xcommand_t function, int flags )
{
	cmd_function_t* cmd;
	cmd_function_t** link;

	if ( host_initialized ) // because hunk allocation would get stomped
		Sys_Error( "Cmd_AddCommand after host_initialized" );

	// fail if the command is a variable name
	if ( Cvar_VariableString( cmd_name )[0] )
	{
		Con_Printf( "Cmd_AddCommand: %s already defined as a var\n", cmd_name );
		return;
	}

	// fail if the command already exists
	if ( Cmd_Exists( cmd_name ) )
	{
		Con_Printf( "Cmd_AddCommand: %s already defined\n", cmd_name );
		return;
	}

	cmd = (cmd_function_t*)Hunk_Alloc( sizeof( cmd_function_t ) );
	cmd->name = cmd_name;
	cmd->function = function ? function : Cmd_ForwardToServer;
	cmd->flags = flags;

	for ( link = &cmd_functions; *link && _stricmp( ( *link )->name, cmd->name ) <= 0; link = &( *link )->next )
		;

	cmd->next = *link;
	*link = cmd;
}

/*
============
Cmd_AddCommand
============
*/
void Cmd_AddCommand( char* cmd_name, xcommand_t function )
{
	Cmd_AddCommandWithFlags( cmd_name, function, 0 );
}

void Cmd_AddMallocCommand( char* cmd_name, xcommand_t function, int flags )
{
	cmd_function_t* cmd;

	// fail if the command is a variable name
	if ( Cvar_VariableString( cmd_name )[0] )
	{
		Con_Printf( "Cmd_AddCommand: %s already defined as a var\n", cmd_name );
		return;
	}

	// fail if the command already exists
	if ( Cmd_Exists( cmd_name ) )
	{
		Con_Printf( "Cmd_AddCommand: %s already defined\n", cmd_name );
		return;
	}

	cmd = (cmd_function_t*)Mem_ZeroMalloc( sizeof( cmd_function_t ) );
	cmd->name = cmd_name;
	cmd->function = function ? function : Cmd_ForwardToServer;
	cmd->flags = flags;
	cmd->next = cmd_functions;
	cmd_functions = cmd;
}

void Cmd_AddHUDCommand( char* cmd_name, xcommand_t function )
{
	Cmd_AddMallocCommand( cmd_name, function, CMD_FLAG_HUD );
}

void Cmd_AddGameCommand( char* cmd_name, xcommand_t function )
{
	Cmd_AddMallocCommand( cmd_name, function, CMD_FLAG_GAME );
}

void Cmd_AddWrapperCommand( char* cmd_name, xcommand_t function )
{
	Cmd_AddMallocCommand( cmd_name, function, CMD_FLAG_WRAPPER );
}

void Cmd_RemoveMallocedCmds( int flags )
{
	cmd_function_t* cmd;
	cmd_function_t* next;
	cmd_function_t* list;

	list = NULL;

	for ( cmd = cmd_functions; cmd; cmd = next )
	{
		next = cmd->next;

		if ( cmd->flags & flags )
		{
			free( cmd );
		}
		else
		{
			cmd->next = list;
			list = cmd;
		}
	}

	cmd_functions = list;
}

void Cmd_RemoveGameCmds()
{
	Cmd_RemoveMallocedCmds( CMD_FLAG_GAME );
}

void Cmd_RemoveHudCmds()
{
	Cmd_RemoveMallocedCmds( CMD_FLAG_HUD );
}

void Cmd_RemoveWrapperCmds()
{
	Cmd_RemoveMallocedCmds( CMD_FLAG_WRAPPER );
}

void Cmd_Shutdown()
{
	cmd_functions = NULL;
	cmd_argc = 0;
	memset( cmd_argv, 0, sizeof( cmd_argv ) );
	cmd_args = NULL;
}

void Cbuf_Execute()
{
	Cbuf_ExecuteCommandsFromBuffer( &cmd_text, TRUE, -1 );
	Cbuf_ExecuteCommandsFromBuffer( &cmd_text_untrusted, FALSE, 1 );
}

/*
============
Cmd_Wait_f

Causes execution of the remainder of the command buffer to be delayed until
next frame.  This allows commands like:
bind g "impulse 5 ; +attack ; wait ; -attack ; impulse 2"
============
*/
//=============================================================================
void Cmd_Wait_f()
{
	cmd_wait = TRUE;
}

/*
===============
Cmd_Alias_f

Creates a new command that executes a command string (possibly ; seperated)
===============
*/
void Cmd_Alias_f()
{
	cmdalias_t* a;
	char cmd[1024];
	int i, c;
	const char* s;

	if ( Cmd_Argc() == 1 )
	{
		Con_Printf( "Current alias commands:\n" );
		for ( a = cmd_alias; a; a = a->next )
			Con_Printf( "%s : %s\n", a->name, a->value );
		return;
	}

	s = Cmd_Argv( 1 );
	if ( Q_strlen( s ) >= MAX_ALIAS_NAME )
	{
		Con_Printf( "Alias name is too long\n" );
		return;
	}

	if ( Cvar_FindVar( s ) )
	{
		Con_Printf( "Alias name is invalid\n" );
		return;
	}

	SetCStrikeFlags();
	if ( g_bIsCStrike || g_bIsCZero )
	{
		const char* reserved[] = { "cl_autobuy", "cl_rebuy", "gl_ztrick", "gl_ztrick_old", "gl_d3dflip" };

		for ( i = 0; i < (int)ARRAYSIZE( reserved ); i++ )
		{
			if ( !_stricmp( s, reserved[i] ) )
			{
				Con_Printf( "Alias name is invalid\n" );
				return;
			}
		}
	}

	if ( !_stricmp( s, "_special" ) || !_stricmp( s, "special" ) )
	{
		Con_Printf( "Alias name is invalid\n" );
		return;
	}

	// copy the rest of the command line
	cmd[0] = 0; // start out with a null string
	c = Cmd_Argc();
	for ( i = 2; i < c; i++ )
	{
		strncat( cmd, Cmd_Argv( i ), sizeof( cmd ) - 2 - strlen( cmd ) );
		strncat( cmd, " ", sizeof( cmd ) - 2 - strlen( cmd ) );
	}
	Q_strcat( cmd, "\n" );

	// if the alias allready exists, reuse it
	for ( a = cmd_alias; a; a = a->next )
	{
		if ( !Q_strcmp( s, a->name ) )
			break;
	}

	if ( !a )
	{
		a = (cmdalias_t*)Z_Malloc( sizeof( cmdalias_t ) );
		a->next = cmd_alias;
		cmd_alias = a;
	}
	else
	{
		if ( !Q_strcmp( a->value, cmd ) ) // Re-alias the same thing
			return;

		Z_Free( a->value );
	}

	Q_strncpy( a->name, s, sizeof( a->name ) - 1 );
	a->name[sizeof( a->name ) - 1] = 0;

	a->value = CopyString( cmd );
}

/*
============
Cmd_Init
============
*/
void Cmd_Init()
{
	SetCStrikeFlags();

	//
	// register our commands
	//
	Cmd_AddCommandWithFlags( "stuffcmds", Cmd_StuffCmds_f, 0 );
	Cmd_AddCommandWithFlags( "exec", Cmd_Exec_f, g_bIsTFC ? 0 : CMD_FLAG_PRIVILEGED );
	Cmd_AddCommandWithFlags( "echo", Cmd_Echo_f, 0 );
	Cmd_AddCommandWithFlags( "alias", Cmd_Alias_f, CMD_FLAG_PRIVILEGED );
	Cmd_AddCommandWithFlags( "cmd", Cmd_ForwardToServer, 0 );
	Cmd_AddCommandWithFlags( "wait", Cmd_Wait_f, 0 );
	Cmd_AddCommandWithFlags( "cmdlist", Cmd_CmdList_f, 0 );
}
