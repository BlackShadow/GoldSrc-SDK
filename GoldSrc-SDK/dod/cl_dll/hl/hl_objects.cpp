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

#include "hud.h"
#include "cl_util.h"
#include "cl_entity.h"
#include "event_api.h"
#include "pm_defs.h"
#include "r_efx.h"
#include "beamdef.h"

extern BEAM* pBeam;
extern BEAM* pBeam2;

void HUD_GetLastOrg( float* origin );

void UpdateBeams()
{
	int index = gEngfuncs.GetLocalPlayer()->index;
	Vector angles, origin, forward, right, up;
	gEngfuncs.GetViewAngles( angles );
	HUD_GetLastOrg( origin );
	gEngfuncs.pfnAngleVectors( angles, forward, right, up );
	Vector end = origin + forward * 2048;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

	// Store off the old count
	gEngfuncs.pEventAPI->EV_PushPMStates();

	// Now add in all of the players.
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( index - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	pmtrace_t trace;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, end, PM_STUDIO_BOX, -1, &trace );
	gEngfuncs.pEventAPI->EV_PopPMStates();
	if ( pBeam )
	{
		pBeam->target = trace.endpos;
		pBeam->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
	if ( pBeam2 )
	{
		pBeam2->target = trace.endpos;
		pBeam2->die = gEngfuncs.GetClientTime() + 0.1; // We keep it alive just a little bit forward in the future, just in case.
	}
}

/*
=====================
Game_AddObjects

Add game specific, client-side objects here
=====================
*/
void Game_AddObjects()
{
	if ( pBeam && pBeam2 )
		UpdateBeams();
}
