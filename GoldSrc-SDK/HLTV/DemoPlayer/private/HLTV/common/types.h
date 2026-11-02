#ifndef HLTV_TYPES_H
#define HLTV_TYPES_H

#include "public/steam/steamtypes.h"

typedef unsigned char	byte;
typedef float			vec_t;
typedef float			vec3_t[3];
typedef uint32			CRC32_t;
typedef int				qboolean;

typedef struct client_data_s
{
	vec3_t	origin;
	vec3_t	viewangles;
	int		iWeaponBits;
	float	fov;
} client_data_t;

#endif
