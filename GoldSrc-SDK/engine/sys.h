// sys.h -- non-portable functions
#pragma once

#include "const.h"

#define ENG_ESCAPEEXITS ( 1 << 4 )
#define STATE_ENDLOGO 2

#define DISCONNECT_REASON_BYTES 256

extern qboolean host_initialized; // true if into command execution
extern double realtime;
extern int host_framecount; // incremented every frame, never reset

extern int giActive;
extern int giStateInfo;
extern int giSubState;

extern qboolean gfExtendedError;
extern char gszDisconnectReason[DISCONNECT_REASON_BYTES];
extern char gszExtendedDisconnectReason[DISCONNECT_REASON_BYTES];

extern const char* szReslistsBaseDir;
extern const char* szReslistsExt;
extern const char* szCommonPreloads;

extern void ( *Console_Printf )( char* format, ... );

void				Sys_Init();
int					Sys_InitGame( char* cmdline, char* basedir, void* windowAddress, int dedicated );
void				Sys_ShutdownGame();
void				Sys_InitFloatTime();
void				Sys_ShutdownFloatTime();
double				Sys_FloatTime();

// an error will cause the entire program to exit
[[noreturn]] void	Sys_Error( const char* format, ... );

// send text to the console
void				Sys_Printf( char* format, ... );
// send text to the console
void				Sys_Warning( const char* format, ... );

const char*			Sys_FindFirst( const char* path, char* basename );
const char*			Sys_FindFirstPathID( const char* path, char* pathID );
const char*			Sys_FindNext( char* basename );
void				Sys_FindClose();

void				Sys_GetCDKey( char* key, int* length, int* dedicated );
void				Sys_SetRegKeyValue( const char* subkey, const char* valueName, const char* value );

void				GameSetState( int state );
void				GameSetSubState( int state );
void				ForceReloadProfile();
qboolean			Host_IsServerActive();
const char*			GetCurrentSteamAppName();
void				ClearIOStates();
