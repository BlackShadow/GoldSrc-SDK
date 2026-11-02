#include "hud.h"
#include "cl_util.h"
#include "bench.h"
#include "demo.h"
#include "demo_api.h"
#include "cl_entity.h"
#include "entity_types.h"
#include "event_api.h"
#include "pm_defs.h"

extern Vector g_aimorg;
extern float g_fZAdjust;

void HUD_GetLastOrg( float* origin );
int HUD_NeedSpot( int* damage );

int HUD_CreateSniperDot( int damage, Vector viewangles, Vector origin, float* dotOrigin )
{
	static cl_entity_t dot;
	static float lastTime;
	static float zAdjust;
	memset( &dot, 0, sizeof( dot ) );
	int modelIndex;
	dot.model = gEngfuncs.CL_LoadModel( Bench_Active() ? "sprites/ppdemodot.spr" : "sprites/laserdot.spr",
		&modelIndex );
	if ( !dot.model )
		return 0;
	cl_entity_t* player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return 0;

	dot.curstate.movetype = MOVETYPE_NONE;
	dot.curstate.solid = SOLID_NOT;
	dot.curstate.modelindex = modelIndex;
	dot.curstate.rendermode = kRenderGlow;
	dot.curstate.renderfx = kRenderFxNoDissipation;
	dot.curstate.renderamt = damage;
	Vector forward, right, up;
	gEngfuncs.pfnAngleVectors( viewangles, forward, right, up );
	Vector end = origin + forward * 8192;
	gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );
	gEngfuncs.pEventAPI->EV_PushPMStates();
	gEngfuncs.pEventAPI->EV_SetSolidPlayers( player->index - 1 );
	gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
	pmtrace_t trace;
	gEngfuncs.pEventAPI->EV_PlayerTrace( origin, end, PM_STUDIO_BOX, -1, &trace );
	int result = 0;
	if ( trace.fraction != 1.0f )
	{
		int contents = gEngfuncs.PM_PointContents( trace.endpos, NULL );
		if ( contents != CONTENTS_SKY &&
			( gEngfuncs.PM_PointContents( origin, NULL ) == CONTENTS_WATER ) == ( contents == CONTENTS_WATER ) )
		{
			if ( Bench_Active() && Bench_InStage( THIRD_STAGE ) )
			{
				float lowRate, highRate;
				if ( Bench_GetPowerPlay() )
				{
					lowRate = -4.0f;
					highRate = 4.0f;
				}
				else
				{
					lowRate = -8.0f;
					highRate = 8.0f;
				}
				float lowLimit, highLimit;
				if ( Bench_GetPowerPlay() )
				{
					lowLimit = -8.0f;
					highLimit = 8.0f;
				}
				else
				{
					lowLimit = -15.0f;
					highLimit = 15.0f;
				}
				float elapsed = gHUD.m_flTime - lastTime;
				if ( elapsed > 0.0f && elapsed < 1.0f )
				{
					zAdjust += gEngfuncs.pfnRandomFloat( lowRate, highRate );
					zAdjust = min( highLimit, zAdjust );
					zAdjust = max( lowLimit, zAdjust );
					trace.endpos.z += zAdjust;
					g_fZAdjust = zAdjust;
				}
				lastTime = gHUD.m_flTime;
			}
			dot.origin = trace.endpos;
			dot.curstate.origin = trace.endpos;
			dot.prevstate = dot.curstate;
			gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, &dot );
			VectorCopy( dot.origin, dotOrigin );
			result = 1;
		}
	}
	gEngfuncs.pEventAPI->EV_PopPMStates();
	return result;
}

void Game_AddObjects()
{
	int tfc = atoi( gEngfuncs.PhysInfo_ValueForKey( "tfc" ) );
	cl_entity_t* player = gEngfuncs.GetLocalPlayer();
	if ( tfc && player && ( player->curstate.playerclass == 2 || Bench_InStage( THIRD_STAGE ) ) )
	{
		int damage;
		if ( HUD_NeedSpot( &damage ) )
		{
			Vector angles, origin, dotOrigin;
			gEngfuncs.GetViewAngles( angles );
			HUD_GetLastOrg( origin );
			if ( HUD_CreateSniperDot( Bench_InStage( THIRD_STAGE ) ? 250 : damage, angles, origin, dotOrigin ) )
				Bench_SpotPosition( dotOrigin, g_aimorg );
			if ( gEngfuncs.pDemoAPI->IsRecording() )
			{
				struct
				{
					int active;
					int damage;
					Vector angles;
					Vector origin;
				} message = { 1, damage, angles, origin };
				Demo_WriteBuffer( TYPE_SNIPERDOT, sizeof( message ), (unsigned char*)&message );
			}
		}
		else if ( gEngfuncs.pDemoAPI->IsRecording() )
		{
			int active = 0;
			Demo_WriteBuffer( TYPE_SNIPERDOT, sizeof( active ), (unsigned char*)&active );
		}
	}
	if ( gEngfuncs.pDemoAPI->IsPlayingback() && g_demosniper )
	{
		Vector dotOrigin;
		HUD_CreateSniperDot( g_demosniperdamage, g_demosniperangles, g_demosniperorg, dotOrigin );
	}
}
