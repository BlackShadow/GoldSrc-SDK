#ifndef HLTV_TYPES_H
#define HLTV_TYPES_H

typedef unsigned char	byte;
typedef float			vec_t;
typedef vec_t			vec3_t[3];
typedef unsigned int	CRC32_t;
typedef int				qboolean;

typedef struct
{
	byte	r;
	byte	g;
	byte	b;
} color24;

typedef struct
{
	vec3_t	normal;
	float	dist;
} plane_t;

typedef struct
{
	qboolean	allsolid;
	qboolean	startsolid;
	qboolean	inopen;
	qboolean	inwater;

	float		fraction;
	vec3_t		endpos;
	plane_t		plane;
	int			hitgroup;
} trace_t;

typedef struct cache_user_s
{
	void*	data;
} cache_user_t;

typedef struct client_data_s
{
	vec3_t	origin;
	vec3_t	viewangles;
	int		iWeaponBits;
	float	fov;
} client_data_t;

typedef void*	pfnUserMsgHook;

#endif
