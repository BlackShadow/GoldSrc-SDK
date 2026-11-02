/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/


#ifndef BSPFILE_H
#define BSPFILE_H

#include "HLTV/common/types.h"

typedef struct
{
	int		fileofs;
	int		filelen;
} lump_t;

typedef struct
{
	float	mins[3];
	float	maxs[3];
	float	origin[3];

	int		headnode[4];
	int		visleafs; // not including the solid leaf 0
	int		firstface;
	int		numfaces;
} dmodel_t;

typedef struct
{
	int		version;
	lump_t	lumps[15];
} dheader_t;

typedef struct
{
	int		nummiptex;
	int		dataofs[4]; // [nummiptex]
} dmiptexlump_t;

typedef struct miptex_s
{
	char			name[16];
	unsigned int	width;
	unsigned int	height;
	unsigned int	offsets[4];
} miptex_t;

typedef struct
{
	float	point[3];
} dvertex_t;

typedef struct
{
	float	normal[3];
	float	dist;
	int		type;		// PLANE_X - PLANE_ANYZ ?remove? trivial to regenerate
} dplane_t;

typedef struct
{
	int				planenum;
	short			children[2]; // negative numbers are -(leafs+1), not nodes
	short			mins[3]; // for sphere culling
	short			maxs[3];
	unsigned short	firstface;
	unsigned short	numfaces;	// counting both sides
} dnode_t;

typedef struct
{
	int		planenum;
	short	children[2]; // negative numbers are contents
} dclipnode_t;

typedef struct texinfo_s
{
	float	vecs[2][4]; // [s/t][xyz offset]
	int		miptex;
	int		flags;
} texinfo_t;

// note that edge 0 is never used, because negative edge nums are used for
// counterclockwise use of the edge in a face
typedef struct
{
	unsigned short	v[2];		// vertex numbers
} dedge_t;

typedef struct
{
	short	planenum;
	short	side;
	int		firstedge; // we must support > 64k edges
	short	numedges;
	short	texinfo;

// lighting info
	byte	styles[4];
	int		lightofs; // start of [numstyles*surfsize] samples
} dface_t;

// leaf 0 is the generic CONTENTS_SOLID leaf, used for all solid areas
// all other leafs need visibility info
typedef struct
{
	int				contents;
	int				visofs; // -1 = no visibility info
	short			mins[3]; // for frustum culling
	short			maxs[3];
	unsigned short	firstmarksurface;
	unsigned short	nummarksurfaces;
	byte			ambient_level[4];
} dleaf_t;

#endif // BSPFILE_H
