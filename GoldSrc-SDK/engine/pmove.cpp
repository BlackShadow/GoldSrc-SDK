#include "pm_defs.h"
#include "common.h"
#include "conprint.h"
#include "info.h"
#include "sys.h"
#include "enginecallbacks.h"
#include "tmessage.h"
#include "../pm_shared/pm_movevars.h"

movevars_t movevars;
playermove_t* pmove;

vec3_t player_mins[MAX_PLAYER_HULLS] = { { -16, -16, -36 }, { -16, -16, -18 }, { 0, 0, 0 }, { -32, -32, -32 } };
vec3_t player_maxs[MAX_PLAYER_HULLS] = { { 16, 16, 36 }, { 16, 16, 18 }, { 0, 0, 0 }, { 32, 32, 32 } };

void CL_Particle( float* origin, int color, float life, int zpos, int zvel );

void PM_Init( playermove_t* ppmove )
{
	int i;

	PM_InitBoxHull();

	for ( i = 0; i < MAX_PLAYER_HULLS; i++ )
	{
		VectorCopy( player_mins[i], ppmove->player_mins[i] );
		VectorCopy( player_maxs[i], ppmove->player_maxs[i] );
	}

	ppmove->movevars = &movevars;

	ppmove->PM_Info_ValueForKey = Info_ValueForKey;
	ppmove->PM_Particle = CL_Particle;
	ppmove->PM_TestPlayerPosition = PM_TestPlayerPosition;
	ppmove->Con_NPrintf = Con_NPrintf;
	ppmove->Con_DPrintf = Con_DPrintf;
	ppmove->Con_Printf = Con_Printf;
	ppmove->Sys_FloatTime = Sys_FloatTime;
	ppmove->PM_StuckTouch = PM_StuckTouch;
	ppmove->PM_PointContents = PM_PointContents;
	ppmove->PM_TruePointContents = PM_TruePointContents;
	ppmove->PM_HullPointContents = PM_HullPointContents;
	ppmove->PM_PlayerTrace = PM_PlayerTrace;
	ppmove->PM_TraceLine = PM_TraceLine;
	ppmove->PM_TraceModel = PM_TraceModel;
	ppmove->RandomLong = RandomLong;
	ppmove->RandomFloat = RandomFloat;
	ppmove->PM_GetModelType = PM_GetModelType;
	ppmove->PM_GetModelBounds = PM_GetModelBounds;
	ppmove->PM_HullForBsp = PM_HullForBsp;
	ppmove->COM_FileSize = COM_FileSize;
	ppmove->COM_LoadFile = COM_LoadFile;
	ppmove->COM_FreeFile = COM_FreeFile;
	ppmove->memfgets = memfgets;
	ppmove->PM_PlayerTraceEx = PM_PlayerTraceEx;
	ppmove->PM_TestPlayerPositionEx = PM_TestPlayerPositionEx;
	ppmove->PM_TraceLineEx = PM_TraceLineEx;
}

static qboolean PM_AddToTouched( pmtrace_t tr, float* impactvelocity )
{
	int i;

	for ( i = 0; i < pmove->numtouch; i++ )
	{
		if ( pmove->touchindex[i].ent == tr.ent )
			break;
	}

	if ( i != pmove->numtouch ) // Already in list.
		return FALSE;

	VectorCopy( impactvelocity, tr.deltavelocity );

	if ( pmove->numtouch >= MAX_PHYSENTS )
		pmove->Con_DPrintf( "Too many entities were touched!\n" );

	pmove->touchindex[pmove->numtouch++] = tr;
	return TRUE;
}

void PM_StuckTouch( int hitent, pmtrace_t* ptraceresult )
{
	if ( !pmove->server )
		return;

	EDICT_NUM( pmove->physents[hitent].info );

	PM_AddToTouched( *ptraceresult, pmove->velocity );
}
