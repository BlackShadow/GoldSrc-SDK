// host.h -- engine frame dispatch

#ifndef HOST_H
#define HOST_H

#if defined( _WIN32 )
#include "../../../HL25-Engine/engine/host.h"
#else

//
// host.c
//
extern "C" int	Host_Frame( float time, int state, int* stateInfo );

#endif

#endif
