#include "pm_defs.h"
#include "gl_model.h"
#include "sys.h"
#include "engine_apiproxy.h"
#include "common.h"
#include "conprint.h"
#include "world.h"

// Values for flags parameter of PM_TraceLine
#define PM_TRACELINE_PHYSENTSONLY 0
#define PM_TRACELINE_ANYVISIBLE 1

hull_t box_hull;
dclipnode_t box_clipnodes[6];
mplane_t box_planes[6];

int g_contentsresult;

int _PM_TestPlayerPosition( float* pos, pmtrace_t* trace, int ( *ignore )( physent_t* ) );
pmtrace_t _PM_PlayerTrace( float* start, float* end, int flags, int numphysent, physent_t* physents, int ignore_pe,
	int ( *ignore )( physent_t* ) );
hull_t* R_StudioHull( model_t* model, float frame, int sequence, const float* angles, const float* origin,
	const float* size, const byte* controller, const byte* blending, int* numhulls, const edict_t* entity,
	int skipShield );
int SV_HitgroupForStudioHull( int index );
qboolean PM_RecursiveHullCheck( hull_t* hull, int num, float p1f, float p2f, float* p1, float* p2, pmtrace_t* trace );

/*
===================
PM_InitBoxHull

Set up the planes and clipnodes so that the six floats of a bounding box
can just be stored out and get a proper hull_t structure.
===================
*/
void PM_InitBoxHull()
{
	int i;
	int side;

	box_hull.clipnodes = box_clipnodes;
	box_hull.planes = box_planes;
	box_hull.firstclipnode = 0;
	box_hull.lastclipnode = 5;

	for ( i = 0; i < 6; i++ )
	{
		box_clipnodes[i].planenum = i;

		side = i & 1;

		box_clipnodes[i].children[side] = CONTENTS_EMPTY;
		if ( i != 5 )
			box_clipnodes[i].children[side ^ 1] = (short)( i + 1 );
		else
			box_clipnodes[i].children[side ^ 1] = CONTENTS_SOLID;

		box_planes[i].type = (byte)( i >> 1 );
		box_planes[i].normal[i >> 1] = 1;
	}
}

int PM_GetModelType( model_t* mod )
{
	return mod->type;
}

void PM_GetModelBounds( model_t* mod, float* mins, float* maxs )
{
	VectorCopy( mod->mins, mins );
	VectorCopy( mod->maxs, maxs );
}

/*
==================
PM_HullPointContents

==================
*/
int PM_HullPointContents( hull_t* hull, int num, float* p )
{
	float d;
	dclipnode_t* node;
	mplane_t* plane;

	if ( hull->firstclipnode >= hull->lastclipnode )
		return CONTENTS_EMPTY;

	while ( num >= 0 )
	{
		if ( num < hull->firstclipnode || num > hull->lastclipnode )
			Sys_Error( "PM_HullPointContents: bad node number" );

		node = &hull->clipnodes[num];
		plane = &hull->planes[node->planenum];

		if ( plane->type < 3 )
			d = p[plane->type] - plane->dist;
		else
			d = DotProduct( plane->normal, p ) - plane->dist;

		if ( d < 0 )
			num = node->children[1];
		else
			num = node->children[0];
	}

	return num;
}

int PM_LinkContents( float* p, int* pIndex )
{
	int i;
	int cont;
	physent_t* pe;
	hull_t* hull;
	vec3_t test;

	for ( i = 1; i < pmove->numphysent; i++ )
	{
		pe = &pmove->physents[i];

		if ( pe->solid != SOLID_NOT || !pe->model )
			continue;

		VectorSubtract( p, pe->origin, test );

		hull = pe->model->hulls;
		cont = PM_HullPointContents( hull, hull->firstclipnode, test );
		if ( cont != CONTENTS_EMPTY )
		{
			if ( pIndex )
				*pIndex = pe->info;

			return pe->skin;
		}
	}

	return CONTENTS_EMPTY;
}

/*
==================
PM_PointContents

==================
*/
int PM_PointContents( float* p, int* truecontents )
{
	hull_t* hull;
	int cont, entityContents;

	g_engdstAddrs.PM_PointContents( &p, &truecontents );

	hull = pmove->physents[0].model->hulls;
	if ( !hull )
	{
		if ( truecontents )
			*truecontents = CONTENTS_EMPTY;
		return CONTENTS_SOLID;
	}

	entityContents = PM_HullPointContents( hull, hull->firstclipnode, p );
	if ( truecontents )
		*truecontents = entityContents;

	if ( entityContents <= CONTENTS_CURRENT_0 && entityContents >= CONTENTS_CURRENT_DOWN )
		entityContents = CONTENTS_WATER;

	if ( entityContents == CONTENTS_SOLID )
		return entityContents;

	cont = PM_LinkContents( p, NULL );
	if ( cont != CONTENTS_EMPTY )
		return cont;

	return entityContents;
}

int PM_TruePointContents( float* p )
{
	hull_t* hull;

	hull = pmove->physents[0].model->hulls;
	if ( hull )
		return PM_HullPointContents( hull, hull->firstclipnode, p );

	return CONTENTS_EMPTY;
}

hull_t* PM_HullForBsp( physent_t* pe, float* offset )
{
	hull_t* hull;

	switch ( pmove->usehull )
	{
	case 1:
		hull = &pe->model->hulls[3];
		break;
	case 2:
		hull = &pe->model->hulls[0];
		break;
	case 3:
		hull = &pe->model->hulls[2];
		break;
	default:
		hull = &pe->model->hulls[1];
		break;
	}

	VectorSubtract( hull->clip_mins, player_mins[pmove->usehull], offset );
	VectorAdd( offset, pe->origin, offset );

	return hull;
}

float PM_TraceModel( physent_t* pe, float* start, float* end, trace_t* trace )
{
	hull_t* hull;
	int oldhull;
	vec3_t offset, start_l, end_l;

	oldhull = pmove->usehull;
	pmove->usehull = 2;
	hull = PM_HullForBsp( pe, offset );
	pmove->usehull = oldhull;

	VectorSubtract( start, offset, start_l );
	VectorSubtract( end, offset, end_l );

	SV_RecursiveHullCheck( hull, hull->firstclipnode, 0.0f, 1.0f, start_l, end_l, trace );
	trace->ent = NULL;

	return trace->fraction;
}

int PM_TestPlayerPosition( float* pos, pmtrace_t* ptrace )
{
	return _PM_TestPlayerPosition( pos, ptrace, NULL );
}

int PM_TestPlayerPositionEx( float* pos, pmtrace_t* ptrace, int ( *ignore )( physent_t* ) )
{
	return _PM_TestPlayerPosition( pos, ptrace, ignore );
}

/*
===================
PM_HullForBox

To keep everything totally uniform, bounding boxes are turned into small
BSP trees instead of being compared directly.
===================
*/
hull_t* PM_HullForBox( float* mins, float* maxs )
{
	box_planes[0].dist = maxs[0];
	box_planes[1].dist = mins[0];
	box_planes[2].dist = maxs[1];
	box_planes[3].dist = mins[1];
	box_planes[4].dist = maxs[2];
	box_planes[5].dist = mins[2];

	return &box_hull;
}

hull_t* PM_HullForStudioModel( model_t* pModel, float* offset, float frame, int sequence, const float* angles,
	const float* origin, const byte* pcontroller, const byte* pblending, int* pNumHulls )
{
	vec3_t size;

	VectorSubtract( player_maxs[pmove->usehull], player_mins[pmove->usehull], size );
	VectorScale( size, 0.5f, size );
	VectorClear( offset );

	return R_StudioHull( pModel, frame, sequence, angles, origin, size, pcontroller, pblending, pNumHulls, NULL, 0 );
}

pmtrace_t PM_PlayerTrace( float* start, float* end, int traceFlags, int ignore_pe )
{
	return _PM_PlayerTrace( start, end, traceFlags, pmove->numphysent, pmove->physents, ignore_pe, NULL );
}

pmtrace_t PM_PlayerTraceEx( float* start, float* end, int traceFlags, int ( *ignore )( physent_t* ) )
{
	return _PM_PlayerTrace( start, end, traceFlags, pmove->numphysent, pmove->physents, -1, ignore );
}

pmtrace_t* PM_TraceLine( float* start, float* end, int flags, int usehull, int ignore_pe )
{
	static pmtrace_t tr;
	int oldhull;

	g_engdstAddrs.PM_TraceLine( &start, &end, &flags, &usehull, &ignore_pe );

	oldhull = pmove->usehull;
	pmove->usehull = usehull;

	if ( flags == PM_TRACELINE_PHYSENTSONLY )
		tr = _PM_PlayerTrace( start, end, PM_NORMAL, pmove->numphysent, pmove->physents, ignore_pe, NULL );
	else if ( flags == PM_TRACELINE_ANYVISIBLE )
		tr = _PM_PlayerTrace( start, end, PM_NORMAL, pmove->numvisent, pmove->visents, ignore_pe, NULL );

	pmove->usehull = oldhull;

	return &tr;
}

pmtrace_t* PM_TraceLineEx( float* start, float* end, int flags, int usehull, int ( *ignore )( physent_t* ) )
{
	static pmtrace_t tr;
	int oldhull;

	oldhull = pmove->usehull;
	pmove->usehull = usehull;

	if ( flags )
		tr = _PM_PlayerTrace( start, end, PM_NORMAL, pmove->numvisent, pmove->visents, -1, ignore );
	else
		tr = _PM_PlayerTrace( start, end, PM_NORMAL, pmove->numphysent, pmove->physents, -1, ignore );

	pmove->usehull = oldhull;

	return &tr;
}

int _PM_TestPlayerPosition( float* pos, pmtrace_t* ptrace, int ( *ignore )( physent_t* ) )
{
	int i, j;
	int numhulls;
	physent_t* pe;
	hull_t* hull;
	vec3_t mins, maxs, test, offset;
	pmtrace_t tr;

	tr = _PM_PlayerTrace( pmove->origin, pmove->origin, PM_NORMAL, pmove->numphysent, pmove->physents, -1, NULL );
	if ( ptrace )
		*ptrace = tr;

	for ( i = 0; i < pmove->numphysent; i++ )
	{
		pe = &pmove->physents[i];

		if ( ignore && ignore( pe ) )
			continue;

		if ( pe->model && pe->solid == SOLID_NOT && pe->skin )
			continue;

		VectorCopy( pe->origin, offset );
		numhulls = 1;

		// get the clipping hull
		if ( pe->model )
		{
			hull = PM_HullForBsp( pe, offset );
		}
		else if ( pe->studiomodel && pe->studiomodel->type == mod_studio
				  && ( ( pe->studiomodel->flags & STUDIO_TRACE_HITBOX ) || pmove->usehull == 2 ) )
		{
			hull = PM_HullForStudioModel( pe->studiomodel, offset, pe->frame, pe->sequence, pe->angles, pe->origin,
				pe->controller, pe->blending, &numhulls );
		}
		else
		{
			VectorSubtract( pe->mins, player_maxs[pmove->usehull], mins );
			VectorSubtract( pe->maxs, player_mins[pmove->usehull], maxs );
			hull = PM_HullForBox( mins, maxs );
		}

		VectorSubtract( pos, offset, test );

		if ( pe->solid == SOLID_BSP && ( pe->angles[0] != 0 || pe->angles[1] != 0 || pe->angles[2] != 0 ) )
		{
			vec3_t forward, right, up;
			vec3_t temp;

			AngleVectors( pe->angles, forward, right, up );

			VectorCopy( test, temp );
			test[0] = DotProduct( forward, temp );
			test[1] = -DotProduct( right, temp );
			test[2] = DotProduct( up, temp );
		}

		for ( j = 0; j < numhulls; j++ )
		{
			g_contentsresult = PM_HullPointContents( &hull[j], hull[j].firstclipnode, test );
			if ( g_contentsresult == CONTENTS_SOLID )
				return i;
		}
	}

	return -1;
}

/*
===============================================================================

LINE TESTING IN HULLS

===============================================================================
*/

/*
==================
PM_RecursiveHullCheck

==================
*/
qboolean PM_RecursiveHullCheck( hull_t* hull, int num, float p1f, float p2f, float* p1, float* p2, pmtrace_t* trace )
{
	dclipnode_t* node;
	mplane_t* plane;
	float t1, t2;
	float frac;
	int i;
	vec3_t mid;
	int side;
	float midf;

	// check for empty
	if ( num < 0 )
	{
		if ( num != CONTENTS_SOLID )
		{
			trace->allsolid = FALSE;
			if ( num == CONTENTS_EMPTY )
				trace->inopen = TRUE;
			else
				trace->inwater = TRUE;
		}
		else
			trace->startsolid = TRUE;
		return TRUE; // empty
	}

	if ( hull->firstclipnode >= hull->lastclipnode )
	{
		trace->allsolid = FALSE;
		trace->inopen = TRUE;
		return TRUE;
	}

	//
	// find the point distances
	//
	node = &hull->clipnodes[num];
	plane = &hull->planes[node->planenum];

	if ( plane->type < 3 )
	{
		t1 = p1[plane->type] - plane->dist;
		t2 = p2[plane->type] - plane->dist;
	}
	else
	{
		t1 = DotProduct( plane->normal, p1 ) - plane->dist;
		t2 = DotProduct( plane->normal, p2 ) - plane->dist;
	}

	if ( t1 >= 0 && t2 >= 0 )
		return PM_RecursiveHullCheck( hull, node->children[0], p1f, p2f, p1, p2, trace );
	if ( t1 < 0 && t2 < 0 )
		return PM_RecursiveHullCheck( hull, node->children[1], p1f, p2f, p1, p2, trace );

	// put the crosspoint DIST_EPSILON pixels on the near side
	side = t1 < 0;
	if ( side )
		frac = (float)( ( t1 + DIST_EPSILON ) / ( t1 - t2 ) );
	else
		frac = (float)( ( t1 - DIST_EPSILON ) / ( t1 - t2 ) );

	if ( frac < 0 )
		frac = 0;
	if ( frac > 1 )
		frac = 1;

	midf = p1f + ( p2f - p1f ) * frac;
	for ( i = 0; i < 3; i++ )
		mid[i] = p1[i] + frac * ( p2[i] - p1[i] );

	// move up to the node
	if ( !PM_RecursiveHullCheck( hull, node->children[side], p1f, midf, p1, mid, trace ) )
		return FALSE;

	if ( PM_HullPointContents( hull, node->children[side ^ 1], mid ) != CONTENTS_SOLID )
	{
		// go past the node
		return PM_RecursiveHullCheck( hull, node->children[side ^ 1], midf, p2f, mid, p2, trace );
	}

	if ( trace->allsolid )
		return FALSE; // never got out of the solid area

	//==================
	// the other side of the node is solid, this is the impact point
	//==================
	if ( !side )
	{
		VectorCopy( plane->normal, trace->plane.normal );
		trace->plane.dist = plane->dist;
	}
	else
	{
		VectorSubtract( vec3_origin, plane->normal, trace->plane.normal );
		trace->plane.dist = -plane->dist;
	}

	while ( PM_HullPointContents( hull, hull->firstclipnode, mid ) == CONTENTS_SOLID )
	{
		// shouldn't really happen, but does occasionally
		frac = (float)( frac - 0.05 );
		if ( frac < 0 )
		{
			trace->fraction = midf;
			VectorCopy( mid, trace->endpos );
			Con_DPrintf( "Trace backed up past 0.0.\n" );
			return FALSE;
		}

		midf = p1f + ( p2f - p1f ) * frac;
		for ( i = 0; i < 3; i++ )
			mid[i] = p1[i] + frac * ( p2[i] - p1[i] );
	}

	trace->fraction = midf;
	VectorCopy( mid, trace->endpos );

	return FALSE;
}

pmtrace_t _PM_PlayerTrace( float* start, float* end, int flags, int numphysent, physent_t* physents, int ignore_pe,
	int ( *ignore )( physent_t* ) )
{
	int i, j;
	int numhulls, closest;
	pmtrace_t trace, total, testtrace;
	vec3_t offset;
	vec3_t start_l, end_l;
	vec3_t mins, maxs;
	vec3_t forward, right, up, temp;
	hull_t* hull;
	physent_t* pe;
	qboolean rotated;
	qboolean startsolid;

	// fill in a default trace
	Q_memset( &total, 0, sizeof( total ) );
	total.fraction = 1;
	total.ent = -1;
	VectorCopy( end, total.endpos );

	for ( i = 0; i < numphysent; i++ )
	{
		if ( i > 0 && ( flags & PM_WORLD_ONLY ) )
			break;

		pe = &physents[i];

		if ( ignore )
		{
			if ( ignore( pe ) )
				continue;
		}
		else if ( i == ignore_pe )
			continue;

		if ( pe->model && pe->solid == SOLID_NOT && pe->skin )
			continue;

		if ( ( flags & PM_GLASS_IGNORE ) && pe->rendermode != kRenderNormal )
			continue;

		VectorCopy( pe->origin, offset );
		numhulls = 1;

		// get the clipping hull
		if ( pe->model )
		{
			hull = PM_HullForBsp( pe, offset );
		}
		else if ( pe->studiomodel && ( flags & PM_STUDIO_IGNORE ) )
		{
			continue;
		}
		else if ( pe->studiomodel && pe->studiomodel->type == mod_studio
				  && ( ( pe->studiomodel->flags & STUDIO_TRACE_HITBOX )
					   || ( pmove->usehull == 2 && !( flags & PM_STUDIO_BOX ) ) ) )
		{
			hull = PM_HullForStudioModel( pe->studiomodel, offset, pe->frame, pe->sequence, pe->angles, pe->origin,
				pe->controller, pe->blending, &numhulls );
		}
		else
		{
			VectorSubtract( pe->mins, player_maxs[pmove->usehull], mins );
			VectorSubtract( pe->maxs, player_mins[pmove->usehull], maxs );
			hull = PM_HullForBox( mins, maxs );
		}

		VectorSubtract( start, offset, start_l );
		VectorSubtract( end, offset, end_l );

		// rotate start and end into the models frame of reference
		rotated = pe->solid == SOLID_BSP && ( pe->angles[0] != 0 || pe->angles[1] != 0 || pe->angles[2] != 0 );
		if ( rotated )
		{
			AngleVectors( pe->angles, forward, right, up );

			VectorCopy( start_l, temp );
			start_l[0] = DotProduct( forward, temp );
			start_l[1] = -DotProduct( right, temp );
			start_l[2] = DotProduct( up, temp );

			VectorCopy( end_l, temp );
			end_l[0] = DotProduct( forward, temp );
			end_l[1] = -DotProduct( right, temp );
			end_l[2] = DotProduct( up, temp );
		}

		// fill in a default trace
		Q_memset( &trace, 0, sizeof( trace ) );
		trace.fraction = 1;
		trace.allsolid = TRUE;
		VectorCopy( end, trace.endpos );

		if ( numhulls <= 0 )
		{
			trace.allsolid = FALSE;
		}
		else if ( numhulls == 1 )
		{
			// trace a line through the apropriate clipping hull
			PM_RecursiveHullCheck( hull, hull->firstclipnode, 0, 1, start_l, end_l, &trace );
		}
		else
		{
			closest = 0;
			for ( j = 0; j < numhulls; j++ )
			{
				Q_memset( &testtrace, 0, sizeof( testtrace ) );
				testtrace.fraction = 1;
				testtrace.allsolid = TRUE;
				VectorCopy( end, testtrace.endpos );

				PM_RecursiveHullCheck( &hull[j], hull[j].firstclipnode, 0, 1, start_l, end_l, &testtrace );

				if ( j == 0 || testtrace.allsolid || testtrace.startsolid || testtrace.fraction < trace.fraction )
				{
					startsolid = trace.startsolid;
					trace = testtrace;
					if ( startsolid )
						trace.startsolid = TRUE;
					closest = j;
				}

				trace.hitgroup = SV_HitgroupForStudioHull( closest );
			}
		}

		if ( trace.allsolid )
			trace.startsolid = TRUE;
		if ( trace.startsolid )
			trace.fraction = 0;

		// fix trace up by the offset
		if ( trace.fraction != 1 )
		{
			if ( rotated )
			{
				AngleVectorsTranspose( pe->angles, forward, right, up );

				VectorCopy( trace.plane.normal, temp );
				trace.plane.normal[0] = DotProduct( forward, temp );
				trace.plane.normal[1] = DotProduct( right, temp );
				trace.plane.normal[2] = DotProduct( up, temp );
			}

			for ( j = 0; j < 3; j++ )
				trace.endpos[j] = start[j] + trace.fraction * ( end[j] - start[j] );
		}

		if ( !( total.fraction <= trace.fraction ) )
		{
			total = trace;
			total.ent = i;
		}
	}

	return total;
}

int PM_WaterEntity( float* p )
{
	hull_t* hull;
	int cont, entityIndex;

	g_engdstAddrs.PM_WaterEntity( &p );

	entityIndex = -1;
	hull = pmove->physents[0].model->hulls;
	cont = PM_HullPointContents( hull, hull->firstclipnode, p );
	if ( cont < CONTENTS_SOLID )
		entityIndex = 0;

	if ( cont != CONTENTS_SOLID )
		PM_LinkContents( p, &entityIndex );

	return entityIndex;
}
