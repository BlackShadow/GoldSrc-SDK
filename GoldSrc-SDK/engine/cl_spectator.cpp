#include "client.h"
#include "cvar.h"
#include "common.h"
#include "conprint.h"
#include "gl_model.h"
#include "pm_defs.h"
#include "render.h"
#include "../common/in_buttons.h"
#include "../pm_shared/pm_movevars.h"

overviewInfo_t gDevOverview;
local_state_t spectatorState;

cvar_t dev_overview = { "dev_overview", "0" };

int CL_IsSpectateOnly( void )
{
	g_engdstAddrs.IsSpectateOnly();

	return cls.spectator ? TRUE : FALSE;
}

qboolean CL_IsDevOverviewMode( void )
{
	return dev_overview.value && ( sv_cheats.value || cls.spectator );
}

void CL_CalculateDevOverviewParameters( void )
{
	int i, j;
	vec3_t mins, maxs;
	float* v;
	float xScale, yScale, zoomX, zoomY;

	for ( i = 0; i < 3; i++ )
	{
		mins[i] = 32000;
		maxs[i] = -32000;
	}

	for ( i = 0; i < cl.worldmodel->numvertexes; i++ )
	{
		v = cl.worldmodel->vertexes[i].position;
		for ( j = 0; j < 3; j++ )
		{
			if ( v[j] > maxs[j] )
				maxs[j] = v[j];
			if ( v[j] < mins[j] )
				mins[j] = v[j];
		}
	}

	gDevOverview.z_min = maxs[2] + 1;
	gDevOverview.z_max = mins[2] - 1;

	for ( i = 0; i < 3; i++ )
		gDevOverview.origin[i] = ( mins[i] + maxs[i] ) * 0.5f;

	xScale = maxs[0] - mins[0];
	yScale = maxs[1] - mins[1];

	if ( yScale > xScale )
	{
		xScale *= 1.3333f;
		gDevOverview.rotated = FALSE;
	}
	else
	{
		yScale *= 1.3333f;
		gDevOverview.rotated = TRUE;
	}

	zoomY = 8192 / yScale;
	zoomX = 8192 / xScale;
	gDevOverview.zoom = zoomY > zoomX ? zoomX : zoomY;
}

void CL_InitSpectator( void )
{
	Q_memset( &spectatorState, 0, sizeof( spectatorState ) );
	Q_memset( &gDevOverview, 0, sizeof( gDevOverview ) );

	CL_CalculateDevOverviewParameters();

	if ( !cls.spectator )
		return;

	spectatorState.playerstate.number = cl.playernum + 1;
	spectatorState.playerstate.friction = 1.0f;
	spectatorState.playerstate.usehull = 1;
	spectatorState.playerstate.movetype = MOVETYPE_NOCLIP;
	spectatorState.playerstate.gravity = 1.0f;
	spectatorState.client.maxspeed = movevars.spectatormaxspeed;
}

qboolean CL_AddEntityToPhysList( int entIndex )
{
	cl_entity_t* pEnt;

	if ( pmove->numphysent >= MAX_PHYSENTS )
	{
		Con_DPrintf( "CL_AddEntityToPhysList:  pmove->numphysent >= MAX_PHYSENTS\n" );
		return FALSE;
	}

	pEnt = &cl_entities[entIndex];
	if ( !pEnt->curstate.modelindex )
		return FALSE;

	CL_AddStateToEntlist( &pmove->physents[pmove->numphysent++], &pEnt->curstate );
	return TRUE;
}

void CL_MoveSpectatorCamera( void )
{
	double time;

	if ( cls.state != ca_active || !cls.spectator )
		return;

	time = cl.time;

	CL_SetUpPlayerPrediction( FALSE, TRUE );
	CL_SetSolidPlayers( cl.playernum );
	CL_RunUsercmd( &spectatorState, &spectatorState, &cl.cmd, TRUE, &time, (unsigned int)(long long)( 100.0 * time ) );

	VectorCopy( spectatorState.client.velocity, cl.simvel );
	VectorCopy( spectatorState.playerstate.origin, cl.simorg );
	VectorCopy( spectatorState.client.punchangle, cl.punchangle );
	VectorCopy( spectatorState.client.view_ofs, cl.viewheight );
}

void CL_SetDevOverView( refdef_t* refdef )
{
	gDevOverview.origin[0] -= 0.0078125f * cl.cmd.sidemove;
	gDevOverview.origin[1] -= 0.0078125f * cl.cmd.forwardmove;

	if ( cl.cmd.upmove > 0 )
		gDevOverview.z_min += 1;
	if ( cl.cmd.upmove < 0 )
		gDevOverview.z_min -= 1;

	if ( cl.cmd.buttons & IN_DUCK )
		gDevOverview.z_max -= 1;
	if ( cl.cmd.buttons & IN_JUMP )
		gDevOverview.z_max += 1;

	if ( cl.cmd.buttons & IN_ATTACK )
		gDevOverview.zoom += 0.01f;
	if ( cl.cmd.buttons & IN_ATTACK2 )
		gDevOverview.zoom -= 0.01f;

	refdef->vieworg[0] = gDevOverview.origin[0];
	refdef->vieworg[1] = gDevOverview.origin[1];
	refdef->vieworg[2] = 16000;
	refdef->viewangles[PITCH] = 90;
	refdef->viewangles[YAW] = gDevOverview.rotated ? 90.0f : 0.0f;
	refdef->viewangles[ROLL] = 0;
	refdef->onlyClientDraws = FALSE;

	if ( dev_overview.value < 2 )
	{
		Con_Printf( " Overview: Zoom %.2f, Map Origin (%.2f, %.2f, %.2f), Z Min %.2f, Z Max %.2f, Rotated %i\n",
			gDevOverview.zoom, gDevOverview.origin[0], gDevOverview.origin[1], gDevOverview.origin[2],
			gDevOverview.z_min, gDevOverview.z_max, gDevOverview.rotated );
	}
}
