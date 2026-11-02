#ifndef PORT_H
#define PORT_H

#include <string.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#define strncasecmp	_strnicmp
#else
#include <strings.h>
#endif

#endif // PORT_H
