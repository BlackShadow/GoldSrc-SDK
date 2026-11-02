// sys.h -- system startup, timing and game state

#ifndef SYS_H
#define SYS_H

#if defined( _WIN32 )
#include "../../../HL25-Engine/engine/sys.h"
#include "../../../HL25-Engine/common/dll_state.h"

#define DLL_RESTARTING	5
#else

#define DLL_INACTIVE	0
#define DLL_ACTIVE		1
#define DLL_PAUSED		2
#define DLL_CLOSE		3
#define DLL_TRANS		4
#define DLL_RESTARTING	5

#define ENG_NORMAL		( 1 << 0 )

//
// sys_dll2.cpp
//
int		Sys_InitGame( char* cmdline, char* basedir, void* windowAddress, int dedicated );
void	Sys_ShutdownGame();

//
// sys_dll.c
//
extern "C" double	Sys_FloatTime();

//
// sys_linuxwind.cpp / sys_sdlwind.cpp
//
extern "C" void		GameSetState( int state );
extern "C" void		GameSetSubState( int state );

#endif

#endif
