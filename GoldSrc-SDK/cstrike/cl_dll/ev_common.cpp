/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
// shared event functions

#include "hud.h"
#include "cl_util.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "event_api.h"
#include "pm_shared.h"
#include "r_efx.h"
#include "eventscripts.h"

#define IS_FIRSTPERSON_SPEC ( g_iUser1 == OBS_IN_EYE || ( g_iUser1 && gHUD.m_Spectator.m_pip->value == INSET_IN_EYE ) )

cl_entity_s* GetEntity( int index )
{
	return gEngfuncs.GetEntityByIndex( index );
}

cl_entity_s* GetViewEntity()
{
	return gEngfuncs.GetViewModel();
}

/*
=================
EV_CreateTracer

Creates a tracer effect
=================
*/
void EV_CreateTracer( float* start, float* end )
{
	gEngfuncs.pEfxAPI->R_TracerEffect( start, end );
}

qboolean EV_IsPlayer( int index )
{
	return index > 0 && index <= gEngfuncs.GetMaxClients();
}

qboolean EV_IsLocal( int index )
{
	// check if we are in some way in first person spec mode
	if ( IS_FIRSTPERSON_SPEC )
		return g_iUser2 == index;
	return gEngfuncs.pEventAPI->EV_IsLocal( index - 1 ) != 0;
}

void EV_GetGunPosition( event_args_s* args, float* position, float* origin )
{
	vec3_t viewOffset( 0, 0, 17 );
	if ( EV_IsPlayer( args->entindex ) )
	{
		if ( EV_IsLocal( args->entindex ) && !IS_FIRSTPERSON_SPEC )
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( viewOffset );
		else if ( args->ducking == 1 )
			viewOffset[2] = 12;
	}
	for ( int i = 0; i < 3; ++i )
		position[i] = origin[i] + viewOffset[i];
}

void EV_EjectBrass( float* origin, float* velocity, float rotation, int model, int soundType, int index, int angularVelocity )
{
	if ( !EV_IsLocal( index ) && g_ShellQueue.Full() )
		return;
	vec3_t angles( 0.0f, rotation, 0.0f );
	g_ShellQueue.Add( gHUD.m_flTime );
	TEMPENTITY* shell = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, angles, 2, model, soundType );
	if ( shell )
	{
		for ( int i = 0; i < 3; ++i )
			shell->entity.baseline.angles[i] *= angularVelocity;
	}
}

void EV_GetDefaultShellInfo( event_args_s* args, float* origin, float* velocity,
	float* shellVelocity, float* shellOrigin, float* forward, float* right, float* up,
	float forwardScale, float upScale, float rightScale, bool reverseDirection )
{
	vec3_t viewOffset( 0, 0, DEFAULT_VIEWHEIGHT );
	if ( EV_IsPlayer( args->entindex ) )
	{
		if ( EV_IsLocal( args->entindex ) )
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( viewOffset );
		else if ( args->ducking == 1 )
			viewOffset[2] = 12;
	}
	float direction = rightScale <= 0 ? 1.0f : -1.0f;
	float rightVelocity = gEngfuncs.pfnRandomFloat( 50, 90 ) * direction;
	float upVelocity = gEngfuncs.pfnRandomFloat( 75, 175 );
	float forwardVelocity = gEngfuncs.pfnRandomFloat( 25, 150 );
	for ( int i = 0; i < 3; ++i )
		velocity[i] *= 0.5f;
	for ( int i = 0; i < 3; ++i )
	{
		if ( reverseDirection )
			shellVelocity[i] = velocity[i] - right[i] * rightVelocity + up[i] * upVelocity + forward[i] * forwardVelocity;
		else
			shellVelocity[i] = velocity[i] + right[i] * rightVelocity + up[i] * upVelocity + forward[i] * forwardVelocity;
		shellOrigin[i] = velocity[i] / 5.0 + origin[i] + viewOffset[i] + upScale * up[i] + forwardScale * forward[i] + rightScale * right[i];
	}
}

void EV_MuzzleFlash()
{
	cl_entity_t* entity = GetViewEntity();
	if ( entity )
		entity->curstate.effects |= EF_MUZZLEFLASH;
}
