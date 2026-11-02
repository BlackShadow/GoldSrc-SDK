// port.h -- window handle portability

#ifndef PORT_H
#define PORT_H

#if defined( _WIN32 )
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
typedef void* HWND;
#endif

#endif
