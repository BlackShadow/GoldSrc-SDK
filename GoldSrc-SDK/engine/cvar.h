#pragma once

#include "const.h"
#include "filesystem.h"
#include "../common/cvardef.h"

#include <cstddef>

typedef void ( *pfnCvar_HookVariable_t )( cvar_t* variable );

typedef struct cvarhook_s
{
	pfnCvar_HookVariable_t hook;
	cvar_t* cvar;
	struct cvarhook_s* next;
} cvarhook_t;

extern cvar_t* cvar_vars;
extern const char* cvar_null_string;
extern cvarhook_t* cvar_hooks;

extern cvar_t sv_cheats;
extern cvar_t rate;
extern cvar_t cl_name;
extern cvar_t developer;

void			Cvar_Init();
void			Cvar_ResetList();

void			Cvar_RegisterVariable( cvar_t* variable );
// registers a cvar that allready has the name, string, and optionally the
// archive elements set.
void			Cvar_UnlinkExternals();
void			Cvar_RemoveHudCvars();
qboolean		Cvar_HookVariable( const char* name, cvarhook_t* hook );

void			Cvar_Set( const char* name, const char* value );
// equivelant to "<name> <variable>" typed at the console
void			Cvar_SetValue( char* name, float value );
// expands value to a string and calls Cvar_Set
void			Cvar_DirectSet( cvar_t* variable, const char* value );

float			Cvar_VariableValue( const char* name );
// returns 0 if not defined or non numeric

const char*		Cvar_VariableString( const char* name );
// returns an empty string if not defined

char*			Cvar_CompleteVariable( char* search, int forward );
// attempts to match a partial variable name for command line completion
// returns NULL if nothing fits

cvar_t*			Cvar_FindVar( const char* name );

qboolean		Cvar_Command();
// called by Cmd_ExecuteString when Cmd_Argv(0) doesn't match a known
// command.  Returns true if the command was a variable reference that
// was handled. (print or change)
qboolean		Cvar_CommandWithPrivilegeCheck( qboolean privileged );

void			Cvar_WriteVariables( FileHandle_t file );
// Writes lines containing "set variable value" for all variables
// with the archive flag set to true.

void			Cmd_CvarList_f();
void			Cmd_CvarListPrintCvar( cvar_t* variable, FileHandle_t file );
