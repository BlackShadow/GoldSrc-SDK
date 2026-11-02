#ifndef MODEL_H
#define MODEL_H

#include "HLTV/common/types.h"
#include "engine/bspfile.h"
#include "engine/modelgen.h"

struct efrag_s;
struct surfcache_s;
struct msurface_s;
struct decal_s;

typedef msurface_s	msurface_t;
typedef decal_s		decal_t;

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/
typedef struct
{
	vec3_t	position;
} mvertex_t;

// plane_t structure
// !!! if this is changed, it must be changed in asm_i386.h too !!!
typedef struct mplane_s
{
	vec3_t	normal;
	float	dist;
	byte	type;			// for texture axis selection and fast side tests
	byte	signbits;		// signx + signy<<1 + signz<<1
	byte	pad[2];
} mplane_t;

typedef struct texture_s
{
	char			name[16];
	unsigned int	width;
	unsigned int	height;

	int				anim_total; // total tenths in sequence ( 0 = no)
	int				anim_min;
	int				anim_max;
	texture_s*		anim_next; // in the animation sequence
	texture_s*		alternate_anims; // bmodels in frmae 1 use these

	unsigned int	offsets[4];
	unsigned int	paloffset;
} texture_t;

typedef struct
{
	unsigned short	v[2];
	unsigned int	cachededgeoffset;
} medge_t;

typedef struct
{
	float		vecs[2][4];
	float		mipadjust;
	texture_t*	texture;
	int			flags;
} mtexinfo_t;

struct msurface_s
{
	int				visframe; // should be drawn when node is crossed
	int				dlightframe;
	int				dlightbits;

	mplane_t*		plane;
	int				flags;
	int				firstedge; // look up in model->surfedges[], negative numbers
	int				numedges; // are backwards edges

	surfcache_s*	cachespots[4];
	short			texturemins[2];
	short			extents[2];
	mtexinfo_t*		texinfo;
// lighting info
	byte			styles[4];
	color24*		samples; // [numstyles*surfsize]
	decal_t*		pdecals;
};

struct decal_s
{
	decal_t*	pnext;
	msurface_t*	psurface;

	short		dx;
	short		dy;
	short		texture;
	byte		scale;
	byte		flags;
	short		entityIndex;
};

typedef struct mnode_s
{
// common with leaf
	int				contents; // 0, to differentiate from leafs
	int				visframe; // node needs to be traversed if current
	short			minmaxs[6]; // for bounding box culling
	mnode_s*		parent;

// node specific
	mplane_t*		plane;
	mnode_s*		children[2];
	unsigned short	firstsurface;
	unsigned short	numsurfaces;
} mnode_t;

typedef struct mleaf_s
{
// common with node
	int				contents; // wil be a negative contents number
	int				visframe; // node needs to be traversed if current
	short			minmaxs[6]; // for bounding box culling
	mnode_s*		parent;

// leaf specific
	byte*			compressed_vis;
	efrag_s*		efrags;
	msurface_t**	firstmarksurface;
	int				nummarksurfaces;
	int				key; // BSP sequence number for leaf's contents
	byte			ambient_sound_level[4];
} mleaf_t;

typedef struct hull_s
{
	dclipnode_t*	clipnodes;
	mplane_t*		planes;
	int				firstclipnode;
	int				lastclipnode;

	vec3_t			clip_mins;
	vec3_t			clip_maxs;
} hull_t;

//===================================================================
//
// Whole model
//

typedef enum
{
	mod_brush,
	mod_sprite,
	mod_alias,
	mod_studio
} modtype_t;

typedef struct model_s
{
	char			name[64];
	qboolean		needload; // bmodels and sprites don't cache normally
	modtype_t		type;
	int				numframes;
	synctype_t		synctype;
	int				flags;

	vec3_t			mins;
	vec3_t			maxs;
	float			radius;
	int				firstmodelsurface;
	int				nummodelsurfaces;

	int				numsubmodels;
	dmodel_t*		submodels;
	int				numplanes;
	mplane_t*		planes;
	int				numleafs; // number of visible leafs, not counting 0
	mleaf_s*		leafs;
	int				numvertexes;
	mvertex_t*		vertexes;
	int				numedges;
	medge_t*		edges;
	int				numnodes;
	mnode_t*		nodes;
	int				numtexinfo;
	mtexinfo_t*		texinfo;
	int				numsurfaces;
	msurface_t*		surfaces;
	int				numsurfedges;
	int*			surfedges;
	int				numclipnodes;
	dclipnode_t*	clipnodes;
	int				nummarksurfaces;
	msurface_t**	marksurfaces;

	hull_t			hulls[4];
	int				numtextures;
	texture_t**		textures;
	byte*			visdata;
	color24*		lightdata;
	char*			entities;
	cache_user_t	cache;		// only access through Mod_Extradata
} model_t;

#endif	// __MODEL__
