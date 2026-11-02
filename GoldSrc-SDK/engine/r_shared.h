#pragma once

#include "model.h"
#include "vid.h"

struct cl_entity_s;
struct lumpinfo_s;

#define CACHE_WAD_NAME_BYTES	64

typedef struct cachepic_s
{
	char			name[CACHE_WAD_NAME_BYTES];
	cache_user_t	cache;
} cachepic_t;

typedef cachepic_t cacheentry_t;
typedef struct cachewad_s cachewad_t;
typedef void ( *PFNCACHE )( cachewad_t* wad, byte* data );

struct cachewad_s
{
	char*				name;

	cachepic_t*			cache;
	int					cacheCount;
	int					cacheMax;
	struct lumpinfo_s*	lumps;
	int					lumpCount;
	int					cacheExtra;
	PFNCACHE			pfnCacheBuild;

	int					numpaths;
	char**				basedirs;
	int*				lumppathindices;
};

typedef struct espan_s
{
	int				u;
	int				v;
	int				count;
	struct espan_s*	pnext;
} espan_t;

// FIXME: compress, make a union if that will help
// insubmodel is only 1, flags is fewer than 32, spanstate could be a byte
typedef struct surf_s
{
	struct surf_s*		next;
	struct surf_s*		prev;
	espan_t*			spans;

	int					key; // sorting key (BSP order)
	int					last_u; // set during tracing
	int					spanstate; // 0 = not in span
	// 1 = in span
	// -1 = in inverted span (end before
	//  start)
	int					flags; // currentface flags

	void*				data; // associated data like msurface_t
	struct cl_entity_s*	entity;

	float				nearzi; // nearest 1/z on surface, for mipmapping
	qboolean			insubmodel;
	float				d_ziorigin;
	float				d_zistepu;
	float				d_zistepv;
	int					pad[2]; // to 64 bytes
} surf_t;

typedef struct edge_s
{
	fixed16_t		u;
	fixed16_t		u_step;

	struct edge_s*	prev;
	struct edge_s*	next;
	unsigned short	surfs[2];
	struct edge_s*	nextremove;
	float			nearzi;
	medge_t*		owner;
} edge_t;

extern int					cachewidth;
extern pixel_t*				cacheblock;

extern int					screenwidth;
extern float				pixelAspect;
extern int					r_drawnpolycount;
extern vec3_t				vup;
extern vec3_t				base_vup;
extern vec3_t				vpn;
extern vec3_t				base_vpn;
extern vec3_t				vright;
extern vec3_t				base_vright;

extern struct cl_entity_s*	currententity;
extern vec3_t				modelorg;
extern vec3_t				base_modelorg;

extern surf_t*				surfaces;
extern surf_t*				surface_p;
extern surf_t*				surf_max;

extern float				xcenter;
extern float				ycenter;
extern float				xscale;
extern float				yscale;
extern float				xscaleinv;
extern float				yscaleinv;
extern float				xscaleshrink;
extern float				yscaleshrink;
extern int					d_lightstylevalue[256]; // 8.8 frac of base light value

void	TransformVector( vec_t* in, vec_t* out );
