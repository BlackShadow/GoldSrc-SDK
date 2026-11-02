#include "server.h"
#include "pm_defs.h"
#include "../common/event_flags.h"
#include "world.h"
#include "host.h"
#include "conprint.h"
#include "sys.h"
#include "eiface.h"

#include <cmath>
#include <cstring>

#define STOP_EPSILON 0.1
#define MAX_CLIP_PLANES 5

void SV_SetGlobalTrace( trace_t* trace );
qboolean SV_CheckBottom( edict_t* ent );
int RandomLong( int low, int high );

static vec3_t vec_origin;

/*


pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects 

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.

*/

cvar_t sv_maxvelocity = { "sv_maxvelocity", "2000" };
cvar_t sv_gravity = { "sv_gravity", "800", FCVAR_SERVER };
cvar_t sv_stepsize = { "sv_stepsize", "18", FCVAR_SERVER };
cvar_t sv_bounce = { "sv_bounce", "1", FCVAR_SERVER };
cvar_t sv_friction = { "sv_friction", "4", FCVAR_SERVER };
cvar_t sv_stopspeed = { "sv_stopspeed", "100", FCVAR_SERVER };
cvar_t sv_airaccelerate = { "sv_airaccelerate", "10", FCVAR_SERVER };
cvar_t sv_wateraccelerate = { "sv_wateraccelerate", "10", FCVAR_SERVER };
cvar_t sv_waterfriction = { "sv_waterfriction", "1", FCVAR_SERVER };
cvar_t sv_spectatormaxspeed = { "sv_spectatormaxspeed", "500" };
cvar_t sv_zmax = { "sv_zmax", "4096", FCVAR_SPONLY };
cvar_t sv_wateramp = { "sv_wateramp", "0" };
cvar_t sv_skyname = { "sv_skyname", "desert" };

vec3_t* g_moved_from;
edict_t** g_moved_edict;

/*
================
SV_CheckAllEnts
================
*/
void SV_CheckAllEnts()
{
	int e;
	edict_t* check;

	// see if any solid entities are inside the final position
	for ( e = 1; e < sv.num_edicts; e++ )
	{
		check = &sv.edicts[e];
		if ( check->free )
			continue;
		if ( check->v.movetype == MOVETYPE_NONE || check->v.movetype == MOVETYPE_PUSH
			 || check->v.movetype == MOVETYPE_NOCLIP || check->v.movetype == MOVETYPE_FOLLOW )
			continue;

		if ( SV_TestEntityPosition( check ) )
			Con_Printf( "entity in invalid position\n" );
	}
}

/*
=============
SV_RunThink

Runs thinking code if time.  There is some play in the exact time the think
function will be called, because it is called before any movement is done
in a frame.  Not used for pushmove objects, because they must be exact.
Returns false if the entity removed itself.
=============
*/
qboolean SV_RunThink( edict_t* ent )
{
	float thinktime;

	if ( !( ent->v.flags & FL_KILLME ) )
	{
		thinktime = ent->v.nextthink;
		if ( thinktime <= 0 || thinktime > sv.time + host_frametime )
			return TRUE;

		if ( thinktime < sv.time )
			thinktime = (float)sv.time; // don't let things stay in the past.
								// it is possible to start that way
								// by a trigger with a local time.

		// it is possible to start that way
		// by a trigger with a local time.
		ent->v.nextthink = 0;
		gGlobalVariables.time = thinktime;
		gEntityInterface.pfnThink( ent );
	}

	if ( ent->v.flags & FL_KILLME )
		ED_Free( ent );

	return !ent->free;
}

static qboolean IsInvalidVectorComponent( float value )
{
	int bits;

	memcpy( &bits, &value, sizeof( bits ) );
	return ( bits & nanmask ) == nanmask;
}

/*
================
SV_CheckVelocity
================
*/
void SV_CheckVelocity( edict_t* ent )
{
	int i;

	// bound velocity
	for ( i = 0; i < 3; i++ )
	{
		if ( IsInvalidVectorComponent( ent->v.velocity[i] ) )
		{
			Con_Printf( "Got a NaN velocity on %s\n", &pr_strings[ent->v.classname] );
			ent->v.velocity[i] = 0;
		}
		if ( IsInvalidVectorComponent( ent->v.origin[i] ) )
		{
			Con_Printf( "Got a NaN origin on %s\n", &pr_strings[ent->v.classname] );
			ent->v.origin[i] = 0;
		}

		if ( ent->v.velocity[i] > sv_maxvelocity.value )
		{
			Con_DPrintf( "Got a velocity too high on %s\n", &pr_strings[ent->v.classname] );
			ent->v.velocity[i] = sv_maxvelocity.value;
		}
		else if ( ent->v.velocity[i] < -sv_maxvelocity.value )
		{
			Con_DPrintf( "Got a velocity too low on %s\n", &pr_strings[ent->v.classname] );
			ent->v.velocity[i] = -sv_maxvelocity.value;
		}
	}
}

/*
============
SV_AddGravity

============
*/
void SV_AddGravity( edict_t* ent )
{
	float ent_gravity;

	if ( ent->v.gravity )
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	ent->v.velocity[2] = (float)( ent->v.velocity[2] - ent_gravity * sv_gravity.value * host_frametime );
	ent->v.velocity[2] = (float)( ent->v.velocity[2] + ent->v.basevelocity[2] * host_frametime );
	ent->v.basevelocity[2] = 0;

	SV_CheckVelocity( ent );
}

void SV_AddCorrectGravity( edict_t* ent )
{
	float ent_gravity;

	if ( ent->v.gravity )
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.
	ent->v.velocity[2] = (float)( ent->v.velocity[2] - ent_gravity * sv_gravity.value * 0.5 * host_frametime );
	ent->v.velocity[2] = (float)( ent->v.velocity[2] + ent->v.basevelocity[2] * host_frametime );
	ent->v.basevelocity[2] = 0;

	SV_CheckVelocity( ent );
}

void SV_FixupGravityVelocity( edict_t* ent )
{
	float ent_gravity;

	if ( ent->v.gravity )
		ent_gravity = ent->v.gravity;
	else
		ent_gravity = 1.0f;

	// Get the correct velocity for the end of the dt 
	ent->v.velocity[2] = (float)( ent->v.velocity[2] - ent_gravity * sv_gravity.value * host_frametime * 0.5 );

	SV_CheckVelocity( ent );
}

/*
==================
ClipVelocity

Slide off of the impacting object
returns the blocked flags (1 = floor, 2 = step / wall)
==================
*/
int ClipVelocity( float* in, float* normal, float* out, float overbounce )
{
	float backoff;
	int i, blocked;

	blocked = 0;
	if ( normal[2] > 0 )
		blocked |= 1; // floor
	if ( !normal[2] )
		blocked |= 2; // step

	backoff = DotProduct( in, normal ) * overbounce;

	for ( i = 0; i < 3; i++ )
	{
		out[i] = in[i] - normal[i] * backoff;
		if ( out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON )
			out[i] = 0;
	}

	return blocked;
}

/*
==================
SV_Impact

Two entities have touched, so run their touch functions
==================
*/
void SV_Impact( edict_t* e1, edict_t* e2, trace_t* ptrace )
{
	int groups;

	gGlobalVariables.time = (float)sv.time;

	if ( ( e1->v.flags & FL_KILLME ) || ( e2->v.flags & FL_KILLME ) )
		return;

	if ( e1->v.groupinfo && e2->v.groupinfo )
	{
		groups = e1->v.groupinfo & e2->v.groupinfo;
		if ( ( g_groupop == 0 && !groups ) || ( g_groupop == 1 && groups ) )
			return;
	}

	if ( e1->v.solid != SOLID_NOT )
	{
		SV_SetGlobalTrace( ptrace );
		gEntityInterface.pfnTouch( e1, e2 );
	}

	if ( e2->v.solid != SOLID_NOT )
	{
		SV_SetGlobalTrace( ptrace );
		gEntityInterface.pfnTouch( e2, e1 );
	}
}

/*
===============================================================================

PUSHMOVE

===============================================================================
*/
/*
============
SV_PushEntity

Does not change the entities velocity at all
============
*/
trace_t SV_PushEntity( edict_t* ent, float* push )
{
	trace_t trace;
	vec3_t end;
	qboolean monsterClip;
	int moveType;

	VectorAdd( ent->v.origin, push, end );

	if ( ent->v.movetype == MOVETYPE_FLYMISSILE )
		moveType = MOVE_MISSILE;
	else if ( ent->v.solid == SOLID_NOT || ent->v.solid == SOLID_TRIGGER )
		moveType = MOVE_NOMONSTERS; // only clip against bmodels
	else
		moveType = MOVE_NORMAL;

	monsterClip = ( ent->v.flags & FL_MONSTERCLIP ) ? TRUE : FALSE;

	trace = SV_Move( ent->v.origin, ent->v.mins, ent->v.maxs, end, moveType, ent, monsterClip );

	if ( trace.fraction != 0 )
		VectorCopy( trace.endpos, ent->v.origin );

	SV_LinkEdict( ent, TRUE );

	if ( trace.ent )
		SV_Impact( ent, trace.ent, &trace );

	return trace;
}

static qboolean IsWaterContents( int contents )
{
	return contents <= CONTENTS_WATER && contents > CONTENTS_TRANSLUCENT;
}

qboolean SV_CheckWater( edict_t* ent )
{
	static const vec3_t current_table[] = {
		{ 1, 0, 0 }, { 0, 1, 0 }, { -1, 0, 0 }, { 0, -1, 0 }, { 0, 0, 1 }, { 0, 0, -1 } };
	vec3_t point;
	int cont;

	point[0] = ( ent->v.absmin[0] + ent->v.absmax[0] ) * 0.5f;
	point[1] = ( ent->v.absmin[1] + ent->v.absmax[1] ) * 0.5f;
	point[2] = ent->v.absmin[2] + 1;

	ent->v.waterlevel = 0;
	ent->v.watertype = CONTENTS_EMPTY;

	g_groupmask = ent->v.groupinfo;
	cont = SV_PointContents( point );
	if ( IsWaterContents( cont ) )
	{
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;

		if ( ent->v.absmin[2] == ent->v.absmax[2] )
		{
			ent->v.waterlevel = 3;
		}
		else
		{
			g_groupmask = ent->v.groupinfo;
			point[2] = ( ent->v.absmin[2] + ent->v.absmax[2] ) * 0.5f;
			if ( IsWaterContents( SV_PointContents( point ) ) )
			{
				ent->v.waterlevel = 2;

				VectorAdd( point, ent->v.view_ofs, point );

				g_groupmask = ent->v.groupinfo;
				if ( IsWaterContents( SV_PointContents( point ) ) )
					ent->v.waterlevel = 3; // In over our eyes
			}
		}

		if ( cont <= CONTENTS_CURRENT_0 && cont >= CONTENTS_CURRENT_DOWN )
		{
			VectorMA( ent->v.basevelocity, (float)( ent->v.waterlevel * 50.0 ), current_table[CONTENTS_CURRENT_0 - cont],
				ent->v.basevelocity );
		}
	}

	return ent->v.waterlevel > 1;
}

static void PlayWadeSound( edict_t* ent )
{
	const char* sample;

	switch ( RandomLong( 0, 3 ) )
	{
	case 0:
		sample = "player/pl_wade1.wav";
		break;
	case 1:
		sample = "player/pl_wade2.wav";
		break;
	case 2:
		sample = "player/pl_wade3.wav";
		break;
	case 3:
		sample = "player/pl_wade4.wav";
		break;
	default:
		return;
	}

	SV_StartSound( 0, ent, CHAN_BODY, sample, 255, ATTN_NORM, 0, PITCH_NORM );
}

void PF_WaterMove( edict_t* ent )
{
	int flags;
	int waterlevel;
	int watertype;
	float drownlevel;

	if ( ent->v.movetype == MOVETYPE_NOCLIP )
	{
		ent->v.air_finished = (float)( sv.time + 12 );
		return;
	}

	if ( ent->v.health < 0 )
		return;

	drownlevel = ( ent->v.deadflag == DEAD_NO ) ? 3.0f : 1.0f;
	waterlevel = ent->v.waterlevel;
	watertype = ent->v.watertype;
	flags = ent->v.flags;

	if ( !( flags & ( FL_GODMODE | FL_IMMUNE_WATER ) ) )
	{
		if ( ( flags & FL_SWIM ) || waterlevel >= drownlevel )
		{
			if ( ent->v.air_finished < sv.time && ent->v.pain_finished < sv.time )
			{
				ent->v.dmg += 2;
				if ( ent->v.dmg > 15 )
					ent->v.dmg = 10;

				ent->v.pain_finished = (float)( sv.time + 1 );
			}
		}
		else
		{
			ent->v.dmg = 2;
			ent->v.air_finished = (float)( sv.time + 12 );
		}
	}

	if ( waterlevel == 0 )
	{
		if ( flags & FL_INWATER )
		{
			// play leave water sound
			PlayWadeSound( ent );
			ent->v.flags = flags & ~FL_INWATER;
		}

		ent->v.air_finished = (float)( sv.time + 12 );
		return;
	}

	if ( watertype == CONTENTS_LAVA )
	{
		if ( !( flags & ( FL_GODMODE | FL_IMMUNE_LAVA ) ) && ent->v.dmgtime < sv.time )
		{
			if ( ent->v.radsuit_finished < sv.time )
				ent->v.dmgtime = (float)( sv.time + 0.2 );
			else
				ent->v.dmgtime = (float)( sv.time + 1 );
		}
	}
	else if ( watertype == CONTENTS_SLIME )
	{
		if ( !( flags & ( FL_GODMODE | FL_IMMUNE_SLIME ) ) && ent->v.dmgtime < sv.time
			 && ent->v.radsuit_finished < sv.time )
			ent->v.dmgtime = (float)( sv.time + 1 );
	}

	if ( !( flags & FL_INWATER ) )
	{
		// player enter water sound
		if ( watertype == CONTENTS_WATER )
			PlayWadeSound( ent );

		ent->v.dmgtime = 0;
		ent->v.flags = flags | FL_INWATER;
	}

	if ( !( flags & FL_WATERJUMP ) )
		VectorMA( ent->v.velocity, (float)( -0.8 * ent->v.waterlevel * host_frametime ), ent->v.velocity, ent->v.velocity );
}

/*
=============
SV_CheckWaterTransition

=============
*/
void SV_CheckWaterTransition( edict_t* ent )
{
	vec3_t point;
	int cont;

	point[0] = (float)( ( ent->v.absmin[0] + ent->v.absmax[0] ) * 0.5 );
	point[1] = (float)( ( ent->v.absmin[1] + ent->v.absmax[1] ) * 0.5 );
	point[2] = ent->v.absmin[2] + 1;

	g_groupmask = ent->v.groupinfo;
	cont = SV_PointContents( point );

	if ( !ent->v.watertype )
	{
		// just spawned here
		ent->v.watertype = cont;
		ent->v.waterlevel = 1;
		return;
	}

	if ( !IsWaterContents( cont ) )
	{
		if ( ent->v.watertype != CONTENTS_EMPTY )
		{
			// just crossed out of water
			SV_StartSound( 0, ent, CHAN_AUTO, "player/pl_wade2.wav", 255, 1.0f, 0, PITCH_NORM );
		}
		ent->v.watertype = CONTENTS_EMPTY;
		ent->v.waterlevel = 0;
		return;
	}

	if ( ent->v.watertype == CONTENTS_EMPTY )
	{
		// just crossed into water
		SV_StartSound( 0, ent, CHAN_AUTO, "player/pl_wade1.wav", 255, 1.0f, 0, PITCH_NORM );
		ent->v.velocity[2] = (float)( ent->v.velocity[2] * 0.5 );
	}

	ent->v.watertype = cont;
	ent->v.waterlevel = 1;

	if ( ent->v.absmin[2] == ent->v.absmax[2] )
	{
		ent->v.waterlevel = 3;
		return;
	}

	g_groupmask = ent->v.groupinfo;
	point[2] = (float)( ( ent->v.absmin[2] + ent->v.absmax[2] ) * 0.5 );
	if ( IsWaterContents( SV_PointContents( point ) ) )
	{
		ent->v.waterlevel = 2;

		VectorAdd( point, ent->v.view_ofs, point );

		g_groupmask = ent->v.groupinfo;
		if ( IsWaterContents( SV_PointContents( point ) ) )
			ent->v.waterlevel = 3;
	}
}

float SV_RecursiveWaterLevel( float* center, float out, float in, int count )
{
	vec3_t test;
	float offset;

	offset = (float)( ( out - in ) * 0.5 + in );
	count++;

	if ( count >= 6 )
		return offset;

	VectorCopy( center, test );
	test[2] += offset;

	if ( SV_PointContents( test ) == CONTENTS_WATER )
		return SV_RecursiveWaterLevel( center, out, offset, count );

	return SV_RecursiveWaterLevel( center, offset, in, count );
}

float SV_Submerged( edict_t* ent )
{
	int i;
	float bottom;
	vec3_t center;
	vec3_t point;

	for ( i = 0; i < 3; i++ )
		center[i] = (float)( ( ent->v.absmin[i] + ent->v.absmax[i] ) * 0.5 );

	bottom = ent->v.absmin[2] - center[2];

	switch ( ent->v.waterlevel )
	{
	case 1:
		return SV_RecursiveWaterLevel( center, 0, bottom, 0 ) - bottom;
	case 2:
		return SV_RecursiveWaterLevel( center, ent->v.absmax[2] - center[2], 0, 0 ) - bottom;
	case 3:
		point[0] = center[0];
		point[1] = center[1];
		point[2] = ent->v.absmax[2];

		g_groupmask = ent->v.groupinfo;
		if ( SV_PointContents( point ) == CONTENTS_WATER )
			return ent->v.maxs[2] - ent->v.mins[2];

		return SV_RecursiveWaterLevel( center, ent->v.absmax[2] - center[2], 0, 0 ) - bottom;
	}

	return 0;
}

/*
============
SV_FlyMove

The basic solid body movement clip that slides along multiple planes
Returns the clipflags if the velocity was modified (hit something solid)
1 = floor
2 = wall / step
4 = dead stop
If steptrace is not NULL, the trace of any vertical wall hit will be stored
============
*/
int SV_FlyMove( edict_t* ent, float time, trace_t* steptrace )
{
	int bumpcount, numbumps;
	vec3_t dir;
	float d;
	int planeCount;
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t primal_velocity, original_velocity, new_velocity;
	int i, j;
	trace_t trace, stuck;
	vec3_t end;
	float time_left;
	int blocked;
	qboolean monsterClip;

	numbumps = 4;

	blocked = 0;
	VectorCopy( ent->v.velocity, original_velocity );
	VectorCopy( ent->v.velocity, primal_velocity );
	planeCount = 0;

	time_left = time;

	monsterClip = ( ent->v.flags & FL_MONSTERCLIP ) ? TRUE : FALSE;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		if ( !ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2] )
			break;

		for ( i = 0; i < 3; i++ )
			end[i] = ent->v.origin[i] + time_left * ent->v.velocity[i];

		trace = SV_Move( ent->v.origin, ent->v.mins, ent->v.maxs, end, MOVE_NORMAL, ent, monsterClip );

		if ( trace.allsolid )
		{
			// entity is trapped in another solid
			VectorCopy( vec3_origin, ent->v.velocity );
			return 4;
		}

		if ( trace.fraction > 0 )
		{	// actually covered some distance
			stuck = SV_Move( trace.endpos, ent->v.mins, ent->v.maxs, trace.endpos, MOVE_NORMAL, ent, monsterClip );
			if ( !stuck.allsolid )
			{
				VectorCopy( trace.endpos, ent->v.origin );
				VectorCopy( ent->v.velocity, original_velocity );
				planeCount = 0;
			}
		}

		if ( trace.fraction == 1 )
			break; // moved the entire distance

		if ( !trace.ent )
			Sys_Error( "SV_FlyMove: !trace.ent" );

		if ( trace.plane.normal[2] > 0.7 )
		{
			blocked |= 1; // floor
			if ( trace.ent->v.solid == SOLID_BSP || trace.ent->v.solid == SOLID_SLIDEBOX
				 || trace.ent->v.movetype == MOVETYPE_PUSHSTEP || ( ent->v.flags & FL_CLIENT ) )
			{
				ent->v.flags |= FL_ONGROUND;
				ent->v.groundentity = trace.ent;
			}
		}
		if ( !trace.plane.normal[2] )
		{
			blocked |= 2; // step
			if ( steptrace )
				*steptrace = trace; // save for player extrafriction
		}

		// run the impact function
		SV_Impact( ent, trace.ent, &trace );
		if ( ent->free )
			break; // removed by the impact function

		time_left -= time_left * trace.fraction;

		// cliped to another plane
		if ( planeCount >= MAX_CLIP_PLANES )
		{
			// this shouldn't really happen
			VectorCopy( vec3_origin, ent->v.velocity );
			return blocked;
		}

		VectorCopy( trace.plane.normal, planes[planeCount] );
		planeCount++;

		if ( planeCount == 1 && ent->v.movetype == MOVETYPE_WALK
			 && ( !( ent->v.flags & FL_ONGROUND ) || ent->v.friction != 1.0f ) )
		{
			if ( planes[0][2] <= 0.7 )
				d = ( 1.0f - ent->v.friction ) * sv_bounce.value + 1.0f;
			else
				d = 1.0f;

			ClipVelocity( original_velocity, planes[0], new_velocity, d );
//				Con_Printf ("clip velocity, numplanes == %i\n",numplanes);
			VectorCopy( new_velocity, ent->v.velocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
//
			// modify original_velocity so it parallels all of the clip planes
//
			for ( i = 0; i < planeCount; i++ )
			{
				ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
				for ( j = 0; j < planeCount; j++ )
				{
					if ( j != i && DotProduct( new_velocity, planes[j] ) < 0 )
						break; // not ok
				}
				if ( j == planeCount )
					break;
			}

			if ( i != planeCount )
			{
				// go along this plane
				VectorCopy( new_velocity, ent->v.velocity );
			}
			else
			{
				// go along the crease
				if ( planeCount != 2 )
					return blocked;

				CrossProduct( planes[0], planes[1], dir );
				d = DotProduct( dir, ent->v.velocity );
				VectorScale( dir, d, ent->v.velocity );
			}

			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			if ( DotProduct( primal_velocity, ent->v.velocity ) <= 0 )
			{
				VectorCopy( vec3_origin, ent->v.velocity );
				return blocked;
			}
		}
	}

	return blocked;
}

/*
=============
SV_Physics_Step

Monsters freefall when they don't have a ground entity, otherwise
all movement is done with discrete steps.

This is also used for objects that have become still on the ground, but
will fall if the floor is pulled out from under them.
FIXME: is this true?
=============
*/
void SV_Physics_Step( edict_t* ent )
{
	qboolean wasonground;
	qboolean inwater;
	qboolean monsterClip;
	float speed, newspeed, control;
	float friction;
	float buoyancy;
	vec3_t mins, maxs, point;
	int x, y;
	trace_t trace;

	PF_WaterMove( ent );

	SV_CheckVelocity( ent );

	wasonground = ( ent->v.flags & FL_ONGROUND ) ? TRUE : FALSE;
	inwater = SV_CheckWater( ent );

	if ( ( ent->v.flags & FL_FLOAT ) && ent->v.waterlevel > 0 )
	{
		buoyancy = (float)( (float)ent->v.skin * SV_Submerged( ent ) * host_frametime );

		SV_AddGravity( ent );
		ent->v.velocity[2] += buoyancy;
	}

	// add gravity except:
	//   flying monsters
	//   swimming monsters who are in the water
	if ( !wasonground && !( ent->v.flags & FL_FLY ) && ( !( ent->v.flags & FL_SWIM ) || ent->v.waterlevel <= 0 )
		 && !inwater )
		SV_AddGravity( ent );

	if ( !VectorCompare( ent->v.velocity, vec_origin ) || !VectorCompare( ent->v.basevelocity, vec_origin ) )
	{
		ent->v.flags &= ~FL_ONGROUND;

		// apply friction
		// let dead monsters who aren't completely onground slide
		if ( wasonground && ( !( ent->v.health <= 0 ) || SV_CheckBottom( ent ) ) )
		{
			speed = (float)sqrt(
				(double)( ent->v.velocity[0] * ent->v.velocity[0] + ent->v.velocity[1] * ent->v.velocity[1] ) );
			if ( speed )
			{
				friction = sv_friction.value * ent->v.friction;
				ent->v.friction = 1.0f;

				control = ( speed < sv_stopspeed.value ) ? sv_stopspeed.value : speed;
				newspeed = (float)( speed - control * host_frametime * friction );
				if ( newspeed < 0 )
					newspeed = 0;

				newspeed = newspeed / speed;

				ent->v.velocity[0] *= newspeed;
				ent->v.velocity[1] *= newspeed;
			}
		}

		VectorAdd( ent->v.velocity, ent->v.basevelocity, ent->v.velocity );
		SV_CheckVelocity( ent );

		SV_FlyMove( ent, (float)host_frametime, NULL );
		SV_CheckVelocity( ent );

		VectorSubtract( ent->v.velocity, ent->v.basevelocity, ent->v.velocity );
		SV_CheckVelocity( ent );

		// determine if it's on solid ground at all
		VectorAdd( ent->v.origin, ent->v.mins, mins );
		VectorAdd( ent->v.origin, ent->v.maxs, maxs );

		point[2] = mins[2] - 1;

		for ( x = 0; x <= 1; x++ )
		{
			for ( y = 0; y <= 1; y++ )
			{
				point[0] = x ? maxs[0] : mins[0];
				point[1] = y ? maxs[1] : mins[1];

				g_groupmask = ent->v.groupinfo;
				if ( SV_PointContents( point ) == CONTENTS_SOLID )
				{
					ent->v.flags |= FL_ONGROUND;
					break;
				}
			}
		}

		SV_LinkEdict( ent, TRUE );
	}
	else if ( gGlobalVariables.force_retouch != 0 )
	{
		monsterClip = ( ent->v.flags & FL_MONSTERCLIP ) ? TRUE : FALSE;
		trace = SV_Move( ent->v.origin, ent->v.mins, ent->v.maxs, ent->v.origin, MOVE_NORMAL, ent, monsterClip );

		if ( ( trace.fraction < 1 || trace.startsolid ) && trace.ent )
			SV_Impact( ent, trace.ent, &trace );
	}

	// regular thinking
	SV_RunThink( ent );

	SV_CheckWaterTransition( ent );
}

/*
============
SV_PushMove

============
*/
void SV_PushMove( edict_t* pusher, float movetime )
{
	int i, e;
	edict_t* check;
	vec3_t mins, maxs, move;
	vec3_t entorig, pushorig;
	int num_moved;

	if ( !pusher->v.velocity[0] && !pusher->v.velocity[1] && !pusher->v.velocity[2] )
	{
		pusher->v.ltime += movetime;
		return;
	}

	for ( i = 0; i < 3; i++ )
	{
		move[i] = pusher->v.velocity[i] * movetime;
		mins[i] = pusher->v.absmin[i] + move[i];
		maxs[i] = pusher->v.absmax[i] + move[i];
	}

	VectorCopy( pusher->v.origin, pushorig );

	// move the pusher to it's final position
	VectorAdd( pusher->v.origin, move, pusher->v.origin );
	pusher->v.ltime += movetime;
	SV_LinkEdict( pusher, FALSE );

	if ( pusher->v.solid == SOLID_NOT )
		return;

	// see if any solid entities are inside the final position
	num_moved = 0;
	for ( e = 1; e < sv.num_edicts; e++ )
	{
		check = &sv.edicts[e];
		if ( check->free )
			continue;
		if ( check->v.movetype == MOVETYPE_PUSH || check->v.movetype == MOVETYPE_NONE
			 || check->v.movetype == MOVETYPE_FOLLOW || check->v.movetype == MOVETYPE_NOCLIP )
			continue;

		// if the entity is standing on the pusher, it will definately be moved
		if ( !( check->v.flags & FL_ONGROUND ) || check->v.groundentity != pusher )
		{
			if ( check->v.absmin[0] >= maxs[0] || check->v.absmin[1] >= maxs[1] || check->v.absmin[2] >= maxs[2]
				 || check->v.absmax[0] <= mins[0] || check->v.absmax[1] <= mins[1] || check->v.absmax[2] <= mins[2] )
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if ( !SV_TestEntityPosition( check ) )
				continue;
		}

		// remove the onground flag for non-players
		if ( check->v.movetype != MOVETYPE_WALK )
			check->v.flags &= ~FL_ONGROUND;

		VectorCopy( check->v.origin, entorig );
		VectorCopy( check->v.origin, g_moved_from[num_moved] );
		g_moved_edict[num_moved] = check;
		num_moved++;

		// try moving the contacted entity
		pusher->v.solid = SOLID_NOT;
		SV_PushEntity( check, move );
		pusher->v.solid = SOLID_BSP;

		// if it is still inside the pusher, block
		if ( !SV_TestEntityPosition( check ) || check->v.mins[0] == check->v.maxs[0] )
			continue;

		if ( check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER )
		{
			// corpse
			check->v.mins[0] = check->v.mins[1] = 0;
			check->v.maxs[0] = check->v.maxs[1] = 0;
			check->v.maxs[2] = check->v.mins[2];
			continue;
		}

		VectorCopy( entorig, check->v.origin );
		SV_LinkEdict( check, TRUE );

		VectorCopy( pushorig, pusher->v.origin );
		SV_LinkEdict( pusher, FALSE );

		pusher->v.ltime -= movetime;

		gEntityInterface.pfnBlocked( pusher, check );

		// move back any entities we already moved
		for ( i = 0; i < num_moved; i++ )
		{
			VectorCopy( g_moved_from[i], g_moved_edict[i]->v.origin );
			SV_LinkEdict( g_moved_edict[i], FALSE );
		}

		return;
	}
}

int SV_PushRotate( edict_t* pusher, float movetime )
{
	int i, e;
	edict_t* check;
	vec3_t move, amove, entamove;
	vec3_t entorig, pushorig;
	int num_moved;
	vec3_t org, start, end;
	vec3_t forward, right, up;
	vec3_t forwardNow, rightNow, upNow;
	vec3_t push;

	if ( !pusher->v.avelocity[0] && !pusher->v.avelocity[1] && !pusher->v.avelocity[2] )
	{
		pusher->v.ltime += movetime;
		return TRUE;
	}

	for ( i = 0; i < 3; i++ )
		amove[i] = pusher->v.avelocity[i] * movetime;

	AngleVectors( pusher->v.angles, forward, right, up );
	VectorCopy( pusher->v.angles, pushorig );

	// move the pusher to it's final position
	VectorAdd( pusher->v.angles, amove, pusher->v.angles );

	AngleVectorsTranspose( pusher->v.angles, forwardNow, rightNow, upNow );

	pusher->v.ltime += movetime;
	SV_LinkEdict( pusher, FALSE );

	if ( pusher->v.solid == SOLID_NOT )
		return TRUE;

	entamove[0] = 0;
	entamove[1] = amove[1];
	entamove[2] = 0;

	// see if any solid entities are inside the final position
	num_moved = 0;
	for ( e = 1; e < sv.num_edicts; e++ )
	{
		check = &sv.edicts[e];
		if ( check->free )
			continue;
		if ( check->v.movetype == MOVETYPE_PUSH || check->v.movetype == MOVETYPE_NONE
			 || check->v.movetype == MOVETYPE_FOLLOW || check->v.movetype == MOVETYPE_NOCLIP )
			continue;

		// if the entity is standing on the pusher, it will definately be moved
		if ( !( check->v.flags & FL_ONGROUND ) || check->v.groundentity != pusher )
		{
			if ( check->v.absmin[0] >= pusher->v.absmax[0] || check->v.absmin[1] >= pusher->v.absmax[1]
				 || check->v.absmin[2] >= pusher->v.absmax[2] || check->v.absmax[0] <= pusher->v.absmin[0]
				 || check->v.absmax[1] <= pusher->v.absmin[1] || check->v.absmax[2] <= pusher->v.absmin[2] )
				continue;

			// see if the ent's bbox is inside the pusher's final position
			if ( !SV_TestEntityPosition( check ) )
				continue;
		}

		// remove the onground flag for non-players
		if ( check->v.movetype != MOVETYPE_WALK )
			check->v.flags &= ~FL_ONGROUND;

		VectorCopy( check->v.origin, entorig );
		VectorCopy( check->v.origin, g_moved_from[num_moved] );
		g_moved_edict[num_moved] = check;
		num_moved++;

		if ( num_moved >= sv.max_edicts )
			Sys_Error( "Out of edicts in simulator!\n" );

		if ( check->v.movetype == MOVETYPE_PUSHSTEP )
		{
			for ( i = 0; i < 3; i++ )
				org[i] = ( check->v.absmin[i] + check->v.absmax[i] ) * 0.5f;
			VectorSubtract( org, pusher->v.origin, start );
		}
		else
		{
			VectorSubtract( check->v.origin, pusher->v.origin, start );
		}

		// try moving the contacted entity
		pusher->v.solid = SOLID_NOT;

		move[0] = DotProduct( start, forward );
		move[1] = -DotProduct( start, right );
		move[2] = DotProduct( start, up );
		end[0] = DotProduct( move, forwardNow );
		end[1] = DotProduct( move, rightNow );
		end[2] = DotProduct( move, upNow );
		VectorSubtract( end, start, push );

		SV_PushEntity( check, push );
		pusher->v.solid = SOLID_BSP;

		if ( check->v.movetype != MOVETYPE_PUSHSTEP )
		{
			if ( check->v.flags & FL_CLIENT )
			{
				check->v.fixangle = 2;
				check->v.avelocity[1] += amove[1];
			}
			else
			{
				// try moving the contacted entity 
				VectorAdd( check->v.angles, entamove, check->v.angles );
			}
		}

		// if it is still inside the pusher, block
		if ( !SV_TestEntityPosition( check ) || check->v.mins[0] == check->v.maxs[0] )
			continue;

		if ( check->v.solid == SOLID_NOT || check->v.solid == SOLID_TRIGGER )
		{
			// corpse
			check->v.mins[0] = check->v.mins[1] = 0;
			check->v.maxs[0] = check->v.maxs[1] = 0;
			check->v.maxs[2] = check->v.mins[2];
			continue;
		}

		VectorCopy( entorig, check->v.origin );
		SV_LinkEdict( check, TRUE );

		VectorCopy( pushorig, pusher->v.angles );
		SV_LinkEdict( pusher, FALSE );

		pusher->v.ltime -= movetime;

		gEntityInterface.pfnBlocked( pusher, check );

		// move back any entities we already moved
		for ( i = 0; i < num_moved; i++ )
		{
			VectorCopy( g_moved_from[i], g_moved_edict[i]->v.origin );

			if ( g_moved_edict[i]->v.flags & FL_CLIENT )
				g_moved_edict[i]->v.avelocity[1] = 0;
			else if ( g_moved_edict[i]->v.movetype != MOVETYPE_PUSHSTEP )
				VectorSubtract( g_moved_edict[i]->v.angles, entamove, g_moved_edict[i]->v.angles );

			SV_LinkEdict( g_moved_edict[i], FALSE );
		}

		return FALSE;
	}

	return TRUE;
}

/*
=============
SV_Physics_None

Non moving objects can only think
=============
*/
void SV_Physics_None( edict_t* ent )
{
	// regular thinking
	SV_RunThink( ent );
}

/*
=============
SV_Physics_Noclip

A moving object that doesn't obey physics
=============
*/
void SV_Physics_Noclip( edict_t* ent )
{
	// regular thinking
	if ( !SV_RunThink( ent ) )
		return;

	VectorMA( ent->v.angles, (float)host_frametime, ent->v.avelocity, ent->v.angles );
	VectorMA( ent->v.origin, (float)host_frametime, ent->v.velocity, ent->v.origin );

	SV_LinkEdict( ent, FALSE );
}

void SV_Physics_Follow( edict_t* ent )
{
	// regular thinking
	if ( !SV_RunThink( ent ) )
		return;

	if ( !ent->v.aiment )
	{
		Con_DPrintf( "%s movetype FOLLOW with NULL aiment\n", &pr_strings[ent->v.classname] );
		ent->v.movetype = MOVETYPE_NONE;
		return;
	}

	VectorAdd( ent->v.aiment->v.origin, ent->v.v_angle, ent->v.origin );
	VectorCopy( ent->v.aiment->v.angles, ent->v.angles );

	SV_LinkEdict( ent, TRUE );
}

/*
================
SV_Physics_Pusher

================
*/
void SV_Physics_Pusher( edict_t* ent )
{
	int i;
	float thinktime;
	float oldltime;
	float movetime;
	float savetime;

	oldltime = ent->v.ltime;

	thinktime = ent->v.nextthink;
	if ( thinktime < oldltime + host_frametime )
	{
		movetime = thinktime - oldltime;
		if ( movetime < 0 )
			movetime = 0;
	}
	else
	{
		movetime = (float)host_frametime;
	}

	if ( movetime )
	{
		if ( !ent->v.avelocity[0] && !ent->v.avelocity[1] && !ent->v.avelocity[2] )
		{
			SV_PushMove( ent, movetime );
		}
		else if ( !ent->v.velocity[0] && !ent->v.velocity[1] && !ent->v.velocity[2] )
		{
			SV_PushRotate( ent, movetime );
		}
		else if ( SV_PushRotate( ent, movetime ) )
		{
			savetime = ent->v.ltime;

			// Reset the local time to what it was before we rotated
			ent->v.ltime = oldltime;
			SV_PushMove( ent, movetime );

			if ( ent->v.ltime < savetime )
				ent->v.ltime = savetime;
		}
	}

	for ( i = 0; i < 3; i++ )
	{
		if ( ent->v.angles[i] < -3600 || ent->v.angles[i] > 3600 )
			ent->v.angles[i] = (float)fmod( (double)ent->v.angles[i], 3600.0 );
	}

	if ( thinktime > oldltime && ( ( ent->v.flags & FL_ALWAYSTHINK ) || thinktime <= ent->v.ltime ) )
	{
		ent->v.nextthink = 0;
		gGlobalVariables.time = (float)sv.time;
		gEntityInterface.pfnThink( ent );
	}
}

/*
=============
SV_Physics_Toss

Toss, bounce, and fly movement.  When onground, do nothing.
=============
*/
void SV_Physics_Toss( edict_t* ent )
{
	trace_t trace;
	vec3_t move;
	float backoff;
	float vel;
	float scale;

	SV_CheckWater( ent );

	// regular thinking
	if ( !SV_RunThink( ent ) )
		return;

	if ( ent->v.velocity[2] > 0 || !ent->v.groundentity
		 || ( ent->v.groundentity->v.flags & ( FL_CLIENT | FL_MONSTER ) ) )
		ent->v.flags &= ~FL_ONGROUND;

	if ( ( ent->v.flags & FL_ONGROUND ) && VectorCompare( ent->v.velocity, vec_origin ) )
	{
		VectorCopy( vec3_origin, ent->v.avelocity );

//@@
		if ( VectorCompare( ent->v.basevelocity, vec_origin ) )
			return;
	}

	SV_CheckVelocity( ent );

	// add gravity
	if ( ent->v.movetype != MOVETYPE_FLY && ent->v.movetype != MOVETYPE_FLYMISSILE
		 && ent->v.movetype != MOVETYPE_BOUNCEMISSILE )
		SV_AddGravity( ent );

	// move angles
	VectorMA( ent->v.angles, (float)host_frametime, ent->v.avelocity, ent->v.angles );

	// move origin
	VectorAdd( ent->v.velocity, ent->v.basevelocity, ent->v.velocity );
	SV_CheckVelocity( ent );

	VectorScale( ent->v.velocity, (float)host_frametime, move );
	VectorSubtract( ent->v.velocity, ent->v.basevelocity, ent->v.velocity );

	trace = SV_PushEntity( ent, move );
	SV_CheckVelocity( ent );

	if ( trace.allsolid )
	{
		// entity is trapped in another solid
		VectorCopy( vec3_origin, ent->v.velocity );
		VectorCopy( vec3_origin, ent->v.avelocity );
		return;
	}

	if ( trace.fraction != 1 )
	{
		if ( ent->free )
			return;

		if ( ent->v.movetype == MOVETYPE_BOUNCE )
			backoff = 2.0f - ent->v.friction;
		else if ( ent->v.movetype == MOVETYPE_BOUNCEMISSILE )
			backoff = 2.0f; // NOTE: A backoff of 2.0f is a reflection
		else
			backoff = 1.0f;

		ClipVelocity( ent->v.velocity, trace.plane.normal, ent->v.velocity, backoff );

		// stop if on ground
		if ( trace.plane.normal[2] > 0.7 )
		{
			// Get the total velocity (player + conveyors, etc.)
			VectorAdd( ent->v.velocity, ent->v.basevelocity, move );
			vel = DotProduct( move, move );

			// Are we on the ground?
			if ( move[2] < sv_gravity.value * host_frametime )
			{
				ent->v.flags |= FL_ONGROUND;
				ent->v.groundentity = trace.ent;
				ent->v.velocity[2] = 0;
			}

			if ( vel < 30 * 30
				 || ( ent->v.movetype != MOVETYPE_BOUNCE && ent->v.movetype != MOVETYPE_BOUNCEMISSILE ) )
			{
				ent->v.flags |= FL_ONGROUND;
				ent->v.groundentity = trace.ent;
				VectorCopy( vec3_origin, ent->v.velocity );
				VectorCopy( vec3_origin, ent->v.avelocity );
			}
			else
			{
				scale = (float)( ( 1.0 - trace.fraction ) * host_frametime * 0.9 );
				VectorScale( ent->v.velocity, scale, move );
				VectorMA( move, scale, ent->v.basevelocity, move );
				SV_PushEntity( ent, move );
			}
		}
	}

	// check for in water
	SV_CheckWaterTransition( ent );
}

void SV_Physics()
{
	int i;
	edict_t* ent;
	edict_t* groundentity;

	// let the progs know that a new frame has started
	gGlobalVariables.time = (float)sv.time;

	gEntityInterface.pfnStartFrame();

	// iterate through all entities and have them think or simulate
	for ( i = 0; i < sv.num_edicts; i++ )
	{
		ent = &sv.edicts[i];
		if ( ent->free )
			continue;

		// force retouch even for stationary
		if ( gGlobalVariables.force_retouch != 0 )
			SV_LinkEdict( ent, TRUE ); // force retouch even for stationary

		if ( i > 0 && i <= svs.maxclients )
			continue;		// clients are run directly from packets

		if ( ent->v.flags & FL_ONGROUND )
		{
			groundentity = ent->v.groundentity;
			if ( groundentity && ( groundentity->v.flags & FL_CONVEYOR ) )
			{
				if ( ent->v.flags & FL_BASEVELOCITY )
					VectorMA( ent->v.basevelocity, groundentity->v.speed, groundentity->v.movedir, ent->v.basevelocity );
				else
					VectorScale( groundentity->v.movedir, groundentity->v.speed, ent->v.basevelocity );
				ent->v.flags |= FL_BASEVELOCITY;
			}
		}

		if ( !( ent->v.flags & FL_BASEVELOCITY ) )
		{
			// Apply momentum (add in half of the previous frame of velocity first)
			VectorMA( ent->v.velocity, (float)( 1.0 + host_frametime * 0.5 ), ent->v.basevelocity, ent->v.velocity );
			VectorCopy( vec3_origin, ent->v.basevelocity );
		}

		ent->v.flags &= ~FL_BASEVELOCITY;

		switch ( ent->v.movetype )
		{
		case MOVETYPE_NONE:
			SV_RunThink( ent );
			break;
		case MOVETYPE_PUSH:
			SV_Physics_Pusher( ent );
			break;
		case MOVETYPE_FOLLOW:
			SV_Physics_Follow( ent );
			break;
		case MOVETYPE_NOCLIP:
			SV_Physics_Noclip( ent );
			break;
		case MOVETYPE_STEP:
		case MOVETYPE_PUSHSTEP:
			SV_Physics_Step( ent );
			break;
		case MOVETYPE_TOSS:
		case MOVETYPE_BOUNCE:
		case MOVETYPE_FLY:
		case MOVETYPE_FLYMISSILE:
		case MOVETYPE_BOUNCEMISSILE:
			SV_Physics_Toss( ent );
			break;
		default:
			Sys_Error( "SV_Physics: %s bad movetype %d", &pr_strings[ent->v.classname], ent->v.movetype );
			break;
		}

		if ( ent->v.flags & FL_KILLME )
			ED_Free( ent );
	}

	if ( gGlobalVariables.force_retouch != 0 )
		gGlobalVariables.force_retouch -= 1;
}

trace_t SV_Trace_Toss( edict_t* ent, edict_t* ignore )
{
	edict_t tempent, *tent;
	trace_t trace;
	vec3_t move;
	vec3_t end;
	double save_frametime;
//	extern particle_t	*active_particles, *free_particles;
//	particle_t	*p;

	save_frametime = host_frametime;
	host_frametime = 0.05;

	Q_memcpy( &tempent, ent, sizeof( tempent ) );
	tent = &tempent;

	do
	{
		SV_CheckVelocity( tent );
		SV_AddGravity( tent );
		VectorMA( tent->v.angles, (float)host_frametime, tent->v.avelocity, tent->v.angles );
		VectorScale( tent->v.velocity, (float)host_frametime, move );
		VectorAdd( tent->v.origin, move, end );
		trace = SV_Move( tent->v.origin, tent->v.mins, tent->v.maxs, end, MOVE_NORMAL, tent, FALSE );
		VectorCopy( trace.endpos, tent->v.origin );
	} while ( !trace.ent || trace.ent == ignore );

	host_frametime = save_frametime;
	return trace;
}
