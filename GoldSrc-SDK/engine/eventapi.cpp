#include "client.h"
#include "common.h"
#include "gl_model.h"
#include "mathlib.h"
#include "pm_defs.h"
#include "protocol.h"
#include "sound.h"
#include "../common/event_api.h"
#include "../common/event_flags.h"

#include <cstdio>

void CL_ResetEvent( event_info_t* ei );

void EV_PlaySound( int ent, float* origin, int channel, const char* sample, float volume, float attenuation, int fFlags,
	int pitch )
{
	sfx_t sentence;
	sfx_t* sfx;

	if ( fFlags & SND_SENTENCE )
	{
		snprintf( sentence.name, sizeof( sentence.name ), "!%s", rgpszrawsentence[Q_atoi( sample + 1 )] );
		sfx = &sentence;
	}
	else
		sfx = CL_LookupSound( sample );

	if ( channel == CHAN_STATIC )
		S_StartStaticSound( ent, channel, sfx, origin, volume, attenuation, fFlags, pitch );
	else
		S_StartDynamicSound( ent, channel, sfx, origin, volume, attenuation, fFlags, pitch );
}

void EV_StopSound( int ent, int channel, const char* sample )
{
	sfx_t* sfx;

	sfx = CL_LookupSound( sample );
	if ( channel == CHAN_STATIC )
		S_StartStaticSound( ent, channel, sfx, vec3_origin, 0.0f, 0.0f, SND_STOP, PITCH_NORM );
	else
		S_StartDynamicSound( ent, channel, sfx, vec3_origin, 0.0f, 0.0f, SND_STOP, PITCH_NORM );
}

int EV_FindModelIndex( const char* pmodel )
{
	int i;

	for ( i = 1; i < cl.model_precache_count; i++ )
	{
		if ( cl.model_precache[i] && !Q_strcasecmp( pmodel, cl.model_precache[i]->name ) )
			return i;
	}

	return 0;
}

int EV_IsLocal( int playernum )
{
	return playernum == cl.playernum;
}

int EV_LocalPlayerDucking()
{
	return cl.usehull == 1;
}

void EV_LocalPlayerViewheight( float* viewheight )
{
	VectorCopy( cl.viewheight, viewheight );
}

void EV_LocalPlayerBounds( int hull, float* mins, float* maxs )
{
	if ( mins )
		VectorCopy( player_mins[hull], mins );

	if ( maxs )
		VectorCopy( player_maxs[hull], maxs );
}

int EV_IndexFromTrace( pmtrace_t* pTrace )
{
	if ( pTrace )
		return pmove->physents[pTrace->ent].info;

	return 0;
}

physent_t* EV_GetPhysent( int idx )
{
	if ( idx < 0 || idx >= pmove->numphysent )
		return NULL;

	return &pmove->physents[idx];
}

void EV_SetUpPlayerPrediction( int dopred, int bIncludeLocalClient )
{
	CL_SetUpPlayerPrediction( dopred, bIncludeLocalClient );
}

void EV_SetTraceHull( int hull )
{
	pmove->usehull = hull;
}

void EV_PlayerTrace( float* start, float* end, int traceFlags, int ignore_pe, pmtrace_t* tr )
{
	*tr = PM_PlayerTrace( start, end, traceFlags, ignore_pe );
}

void EV_WeaponAnimation( int sequence, int body )
{
	cl.weaponsequence = sequence;
	cl.weaponstarttime = 0.0f;
	cl.viewent.curstate.body = body;
}

unsigned short EV_PrecacheEvent( int, const char* psz )
{
	int i;

	for ( i = 1; cl.event_precache[i].filename; i++ )
	{
		if ( !Q_strcasecmp( cl.event_precache[i].filename, psz ) )
			return (unsigned short)i;
	}

	return 0;
}

void EV_PlaybackEvent( int flags, const edict_t*, unsigned short eventindex, float delay, float* origin, float* angles,
	float fparam1, float fparam2, int iparam1, int iparam2, int bparam1, int bparam2 )
{
	event_args_t args;

	if ( flags & FEV_SERVER )
		return;

	Q_memset( &args, 0, sizeof( args ) );

	args.entindex = flags;
	VectorCopy( origin, args.origin );
	VectorCopy( angles, args.angles );
	args.fparam1 = fparam1;
	args.fparam2 = fparam2;
	args.iparam1 = iparam1;
	args.iparam2 = iparam2;
	args.bparam1 = bparam1;
	args.bparam2 = bparam2;

	CL_QueueEvent( flags, eventindex, delay, &args );
}

void EV_KillEvents( int entnum, const char* eventname )
{
	int i;
	event_info_t* ei;

	for ( i = 0; i < MAX_EVENT_QUEUE; i++ )
	{
		ei = &cl.events.ei[i];
		if ( ei->index && ei->args.entindex == entnum
			 && !Q_strcasecmp( eventname, cl.event_precache[ei->index].filename ) )
		{
			CL_ResetEvent( ei );
		}
	}
}

event_api_t eventapi = {
	EVENT_API_VERSION,
	EV_PlaySound,
	EV_StopSound,
	EV_FindModelIndex,
	EV_IsLocal,
	EV_LocalPlayerDucking,
	EV_LocalPlayerViewheight,
	EV_LocalPlayerBounds,
	EV_IndexFromTrace,
	EV_GetPhysent,
	EV_SetUpPlayerPrediction,
	CL_PushPMStates,
	CL_PopPMStates,
	CL_SetSolidPlayers,
	EV_SetTraceHull,
	EV_PlayerTrace,
	EV_WeaponAnimation,
	EV_PrecacheEvent,
	EV_PlaybackEvent,
	PM_CL_TraceTexture,
	S_StopSound,
	EV_KillEvents,
};
