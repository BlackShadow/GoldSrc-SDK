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
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "r_efx.h"
#include "eventscripts.h"
#include "event_api.h"
#include "../../pm_shared/pm_shared.h"

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

void EV_GetGunPosition( event_args_s* arguments, float* position, float* origin )
{
	float viewOffset[3] = { 0, 0, DEFAULT_VIEWHEIGHT };
	if ( EV_IsPlayer( arguments->entindex ) )
	{
		if ( EV_IsLocal( arguments->entindex ) && !IS_FIRSTPERSON_SPEC )
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( viewOffset );
		else if ( arguments->ducking == 1 )
			viewOffset[2] = VEC_DUCK_VIEW;
	}
	VectorAdd( origin, viewOffset, position );
}

void EV_EjectBrass( float* origin, float* velocity, float rotation, int model, int soundType, int index, int angleVelocity )
{
	EV_IsLocal( index );
	float angles[3] = { 0, rotation, 0 };
	g_ShellQueue.Add( gHUD.m_flTime );
	TEMPENTITY* shell = gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, angles, 2.0f, model, soundType );
	if ( shell )
	{
		for ( int axis = 0; axis < 3; ++axis )
			shell->entity.baseline.angles[axis] *= angleVelocity;
	}
}

void EV_GetDefaultShellInfo( event_args_s* arguments, float* origin, float* velocity, float* shellVelocity,
	float* shellOrigin, float* forward, float* right, float* up, float forwardScale, float upScale,
	float rightScale, bool reverseDirection )
{
	float viewOffset[3] = { 0, 0, DEFAULT_VIEWHEIGHT };
	if ( EV_IsPlayer( arguments->entindex ) )
	{
		if ( EV_IsLocal( arguments->entindex ) )
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( viewOffset );
		else if ( arguments->ducking == 1 )
			viewOffset[2] = VEC_DUCK_VIEW;
	}
	float direction = rightScale <= 0 ? 1.0f : -1.0f;
	float rightVelocity = gEngfuncs.pfnRandomFloat( 50, 90 ) * direction;
	float upVelocity = gEngfuncs.pfnRandomFloat( 75, 175 );
	float forwardVelocity = gEngfuncs.pfnRandomFloat( 25, 150 );
	for ( int axis = 0; axis < 3; ++axis )
		velocity[axis] *= 0.5f;
	for ( int axis = 0; axis < 3; ++axis )
	{
		float lateral = right[axis] * rightVelocity;
		shellVelocity[axis] = velocity[axis] + ( reverseDirection ? -lateral : lateral ) +
			up[axis] * upVelocity + forward[axis] * forwardVelocity;
		shellOrigin[axis] = velocity[axis] / 5.0f + origin[axis] + viewOffset[axis] + upScale * up[axis] +
			forwardScale * forward[axis] + rightScale * right[axis];
	}
}

void EV_MuzzleFlash()
{
	cl_entity_s* entity = GetViewEntity();
	if ( entity )
		entity->curstate.effects |= EF_MUZZLEFLASH;
}
