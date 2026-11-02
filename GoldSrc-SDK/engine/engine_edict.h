#pragma once

#include "mathlib.h"
#include "../common/const.h"
#include "edict.h"

#include <cstddef>

extern "C" globalvars_t gGlobalVariables;

extern "C" edict_t*	ED_Alloc();
extern "C" void		ED_Free( edict_t* entity );
void				ED_ClearEdict( edict_t* entity );
extern "C" void		ED_LoadFromFile( char* data );
extern "C" edict_t*	EDICT_NUM( int index );
extern "C" int		NUM_FOR_EDICT( const edict_t* entity );
int					IndexOfEdict( const edict_t* entity );

void				InitEntityDLLFields( edict_t* entity );
void				ReleaseEntityDLLFields( edict_t* entity );
void				FreeEntPrivateData( edict_t* entity );
void				FreeAllEntPrivateData();

void				SV_LinkEdict( edict_t* entity, qboolean touchTriggers );
// Needs to be called any time an entity changes origin, mins, maxs, or solid
// sets ent->v.absmin and ent->v.absmax
// if touchtriggers, calls prog functions for the intersected triggers

void				SV_UnlinkEdict( edict_t* entity );
// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself
