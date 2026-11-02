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
#include "event_api.h"
#include "event_args.h"
#include "r_efx.h"
#include "dlight.h"
#include "com_model.h"
#include "tri.h"
#include "pm_shared.h"

extern cvar_t* cl_dynamiclights;

static int speeds[8] = { 0, 0, -4, 4, -6, 6, -8, 8 };

cl_entity_t* GetEntity( int idx )
{
	return gEngfuncs.GetEntityByIndex( idx );
}

cl_entity_t* GetViewEntity()
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

qboolean EV_IsPlayer( int idx )
{
	return idx > 0 && idx <= gEngfuncs.GetMaxClients();
}

qboolean EV_IsLocal( int idx )
{
	if ( g_iUser1 == OBS_IN_EYE )
		return g_iUser2 == idx;
	return gEngfuncs.pEventAPI->EV_IsLocal( idx - 1 ) != 0;
}

void EV_GetGunPosition( event_args_t* args, float* pos, float* origin )
{
	Vector offset( 0, 0, 22 );
	if ( EV_IsPlayer( args->entindex ) )
	{
		if ( EV_IsLocal( args->entindex ) && g_iUser1 != OBS_IN_EYE )
			gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( offset );
		else if ( args->ducking == 1 )
			offset.z = 18;
	}
	VectorAdd( origin, offset, pos );
}

void EV_EjectBrass( float* origin, float* velocity, float rotation, int model, int soundtype )
{
	Vector angles( 0.0f, rotation, 0.0f );
	gEngfuncs.pEfxAPI->R_TempModel( origin, velocity, angles, 2.5f, model, soundtype );
}

void EV_GetDefaultShellInfo( event_args_t*, float*, float*, float*, float*, float*, float*, float*, float, float, float )
{
}

void EV_MuzzleFlash( int idx, int guntype )
{
	Vector origin;
	if ( EV_IsLocal( idx ) )
	{
		cl_entity_t* ent = gEngfuncs.GetViewModel();
		VectorCopy( ent->attachment[0], origin );
		if ( cl_dynamiclights->value <= 0 )
			return;
	}
	else
	{
		cl_entity_t* ent = gEngfuncs.GetEntityByIndex( idx );
		cl_entity_t* local = gEngfuncs.GetLocalPlayer();
		if ( !ent || ent->curstate.messagenum < local->curstate.messagenum || !( cl_dynamiclights->value > 0 ) )
			return;
		VectorCopy( ent->curstate.origin, origin );
	}
	dlight_t* light = gEngfuncs.pEfxAPI->CL_AllocDlight( 0 );
	light->origin = origin;
	light->radius = (float)( 50 * guntype + 50 );
	light->color.r = 248;
	light->color.g = 255;
	light->color.b = 120;
	light->decay = (float)( 50 * guntype + 600 );
	light->die = gHUD.m_flTime + 1;
}

void EV_BazookaSmoke( cl_entity_t* ent )
{
	if ( !ent )
		return;
	Vector angles( ent->angles.x, ent->angles.y, -ent->angles.z );
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	model_t* sprite = (model_t*)gEngfuncs.GetSpritePointer( gEngfuncs.pfnSPR_Load( "sprites/bazookapuff.spr" ) );
	if ( !sprite )
	{
		gEngfuncs.Con_DPrintf( "Couldn't load Sprite: %s\n", "sprites/bazookapuff.spr" );
		return;
	}
	int ccw = 1;
	for ( int i = 0; i < 8; ++i )
	{
		Vector origin;
		if ( i & 1 )
			origin = forward * 30 + up * 16 + ent->origin;
		else
		{
			ccw = 1 - ccw;
			origin = up * 16 + ent->origin - forward * 30;
		}
		float variance = gEngfuncs.pfnRandomFloat( -1, 1 );
		CDoDParticle* particle = CDoDParticle::Create( origin, Vector( 0, 0, 0 ), sprite, 90, 80, "dod_particle", true );
		if ( !particle )
			break;
		particle->SetCollisionFlags( TRI_ANIMATEDIE | TRI_COLLIDEWORLD );
		particle->m_iRendermode = kRenderTransAdd;
		particle->SetLightFlag( LIGHT_NONE );
		particle->SetCullFlag( CULL_FRUSTUM_PLANE | CULL_PVS );
		particle->m_flFadeSpeed = 0.7f;
		particle->m_flScaleSpeed = 1.1f;
		particle->m_flDampingTime = 0.5f;
		particle->m_iFrame = 0;
		particle->m_iFramerate = 2;
		particle->m_vColor = Vector( 200, 200, 200 );
		particle->m_vVelocity = forward * (float)speeds[i] + up * variance;
		particle->m_flDieTime = gEngfuncs.GetClientTime() + 5;
		particle->m_vAVelocity.z = ccw ? 1.0f : -1.0f;
		particle->AddGlobalWind();
	}
}

void EV_PSchreckSmoke( cl_entity_t* ent )
{
	EV_BazookaSmoke( ent );
}
