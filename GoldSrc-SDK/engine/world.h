#pragma once

#include "gl_model.h"
#include "engine_edict.h"

#define MOVE_NORMAL 0
#define MOVE_NOMONSTERS 1
#define MOVE_MISSILE 2

// 1/32 epsilon to keep floating point happy
#define DIST_EPSILON ( 0.03125 )

typedef struct areanode_s
{
	int axis; // -1 = leaf node
	float dist;
	struct areanode_s* children[2];
	link_t trigger_edicts;
	link_t solid_edicts;
} areanode_t;

int			SV_PointContents( const float* point );
// returns the CONTENTS_* value from the world at the given point.
// does not check any entities at all
int			SV_HullPointContents( hull_t* hull, int num, const float* point );
int			SV_LinkContents( areanode_t* node, const float* pos );

hull_t*		SV_HullForBsp( edict_t* entity, const float* mins, const float* maxs, float* offset );
hull_t*		SV_HullForEntity( edict_t* entity, const float* mins, const float* maxs, float* offset );
edict_t*	SV_TestEntityPosition( edict_t* entity );

trace_t		SV_Move( const float* start, const float* mins, const float* maxs, const float* end, int type,
				edict_t* passedEntity, qboolean monsterClip );
// mins and maxs are reletive

// if the entire move stays in a solid volume, trace.allsolid will be set

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// nomonsters is used for line of sight or edge testing, where mosnters
// shouldn't be considered solid objects

// passedict is explicitly excluded from clipping checks (normally NULL)
qboolean	SV_RecursiveHullCheck( hull_t* hull, int num, float p1f, float p2f, float* p1, float* p2,
				trace_t* trace );
