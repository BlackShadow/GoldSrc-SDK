#pragma once

#include "filesystem.h"
#include "const.h"
#include "cvar.h"

typedef struct
{
	char* basedir;
	char* cachedir; // for development over ISDN lines
	int argc;
	char** argv;
	void* membase;
	int memsize;
} quakeparms_t;

extern quakeparms_t host_parms;
extern double host_frametime;
extern int gHostSpawnCount;
extern qboolean g_bMajorMapChange;
extern cvar_t sys_timescale;

int			Host_Init( quakeparms_t* parms );
void		Host_InitLocal();
void		Host_Shutdown();
int			Host_Frame( float time, int state, int* stateInfo );
void		_Host_Frame( float time );
[[noreturn]] void Host_Error( const char* format, ... );
void		Host_EndGame( const char* format, ... );
void		Host_ClearMemory( qboolean clearMemory );
void		Host_ShutdownServer( qboolean crash );
void		Host_WriteConfiguration();
void		Host_WriteCustomConfig();
void		Host_Version();
qboolean	Host_IsServerActive();
qboolean	Host_IsSinglePlayerGame();
void		CL_AddVoiceToDatagram( qboolean final );
void		Info_WriteVars( FileHandle_t file );

void		Host_InitCommands();
void		Host_NextDemo();
int			Host_GetStartTime();
void		Host_Maps_f();
void		Host_Version_f();
void		Host_Interp_f();

void		R_SetStackBase();
