#pragma once

#include "mathlib.h"

// must match definition in spritegn.h
#ifndef SYNCTYPE_T
#define SYNCTYPE_T
typedef enum
{
	ST_SYNC = 0,
	ST_RAND
} synctype_t;
#endif

enum aliasframetype_t
{
	ALIAS_SINGLE,
	ALIAS_GROUP
};

enum aliasskintype_t
{
	ALIAS_SKIN_SINGLE,
	ALIAS_SKIN_GROUP
};

typedef struct
{
	int ident;
	int version;
	vec3_t scale;
	vec3_t scale_origin;
	float boundingradius;
	vec3_t eyeposition;
	int numskins;
	int skinwidth;
	int skinheight;
	int numverts;
	int numtris;
	int numframes;
	synctype_t synctype;
	int flags;
	float size;
} mdl_t;

// TODO: could be shorts

typedef struct
{
	int onseam;
	int s;
	int t;
} stvert_t;

typedef struct
{
	int facesfront;
	int vertindex[3];
} dtriangle_t;

typedef struct
{
	byte v[3];
	byte lightnormalindex;
} trivertx_t;

typedef struct
{
	trivertx_t bboxmin; // lightnormal isn't used
	trivertx_t bboxmax; // lightnormal isn't used
	char name[16];		// frame name from grabbing
} daliasframe_t;

typedef struct
{
	int numframes;
	trivertx_t bboxmin; // lightnormal isn't used
	trivertx_t bboxmax; // lightnormal isn't used
} daliasgroup_t;

typedef struct
{
	float interval;
} daliasinterval_t;

typedef struct
{
	aliasframetype_t type;
} daliasframetype_t;

typedef struct
{
	aliasskintype_t type;
} daliasskintype_t;

typedef struct
{
	int numskins;
} daliasskingroup_t;

typedef struct
{
	float interval;
} daliasskininterval_t;
