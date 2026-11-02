#include "r_local.h"
#include "client.h"
#include "conprint.h"
#include "sys.h"
#include "../common/cl_entity.h"

#include <cmath>

#define MAX_BMODEL_VERTS	500			// 6K
#define MAX_BMODEL_EDGES	1000		// 12K
#define MAX_BTOF_POLYS	5000

//
// current entity info
//
qboolean		insubmodel;
cl_entity_t*	currententity;
vec3_t			modelorg;
vec3_t			base_modelorg;
// modelorg is the viewpoint reletive to
// the currently rendering entity
vec3_t			r_entorigin;	// the currently rendering entity in world
								// coordinates
float			entity_rotation[3][3];
vec3_t			r_worldmodelorg;
int				r_currentbkey;

static mvertex_t*	pbverts;
static bedge_t*		pbedges;
static int			numbverts;
static int			numbedges;
static mvertex_t*	pfrontenter;
static mvertex_t*	pfrontexit;
static qboolean		makeclippededge;

/*
================
R_EntityRotate
================
*/
void R_EntityRotate( vec_t* vec )
{
	double x = vec[0];
	double y = vec[1];
	double z = vec[2];

	vec[0] = (float)( entity_rotation[0][0] * x + entity_rotation[0][1] * y + entity_rotation[0][2] * z );
	vec[1] = (float)( entity_rotation[1][0] * x + entity_rotation[1][1] * y + entity_rotation[1][2] * z );
	vec[2] = (float)( x * entity_rotation[2][0] + y * entity_rotation[2][1] + z * entity_rotation[2][2] );
}

/*
================
R_RotateBmodel
================
*/
void R_RotateBmodel()
{
	float angle = (float)( M_PI / 180.0 * currententity->angles[YAW] );
	float sy = (float)sin( (double)angle );
	float cy = (float)cos( (double)angle );
	angle = (float)( M_PI / 180.0 * currententity->angles[PITCH] );
	float sp = (float)sin( (double)angle );
	float cp = (float)cos( (double)angle );
	angle = (float)( M_PI / 180.0 * currententity->angles[ROLL] );
	float sr = (float)sin( (double)angle );
	float cr = (float)cos( (double)angle );

	entity_rotation[0][0] = cp * cy;
	entity_rotation[0][1] = cp * sy;
	entity_rotation[0][2] = -sp;
	entity_rotation[1][0] = sr * sp * cy - cr * sy;
	entity_rotation[1][1] = sr * sp * sy + cr * cy;
	entity_rotation[1][2] = sr * cp;
	entity_rotation[2][0] = cr * sp * cy + sy * sr;
	entity_rotation[2][1] = cr * sp * sy - sr * cy;
	entity_rotation[2][2] = cr * cp;

	vec_t* vectors[4] = { modelorg, vpn, vright, vup };
	for ( int i = 0; i < 4; i++ )
	{
		vec_t* vec = vectors[i];
		vec3_t temp;
		VectorCopy( vec, temp );
		vec[0] = DotProduct( entity_rotation[0], temp );
		vec[1] = DotProduct( entity_rotation[1], temp );
		vec[2] = DotProduct( entity_rotation[2], temp );
	}
	R_TransformFrustum();
}

/*
================
R_RecursiveClipBPoly
================
*/
void R_RecursiveClipBPoly( bedge_t* pedges, mnode_t* pnode, msurface_t* psurf )
{
	bedge_t* psideedges[2] = { NULL, NULL };
	mplane_t* plane = pnode->plane;
	float dist = plane->dist - DotProduct( plane->normal, r_entorigin );
	float normal[3];
	normal[0] = DotProduct( plane->normal, entity_rotation[0] );
	normal[1] = DotProduct( plane->normal, entity_rotation[1] );
	normal[2] = DotProduct( plane->normal, entity_rotation[2] );
	makeclippededge = FALSE;

	for ( bedge_t* edge = pedges; edge; )
	{
		bedge_t* next = edge->pnext;
		mvertex_t* vert0 = edge->v[0];
		mvertex_t* vert1 = edge->v[1];
		float d0 = DotProduct( vert0->position, normal ) - dist;
		float d1 = DotProduct( vert1->position, normal ) - dist;
		int side0 = d0 <= 0.0f;
		int side1 = d1 <= 0.0f;

		if ( side0 == side1 )
		{
			edge->pnext = psideedges[side0];
			psideedges[side0] = edge;
		}
		else
		{
			if ( numbverts >= MAX_BMODEL_VERTS )
			{
				Con_Printf( "Out of verts for bmodel\n" );
				return;
			}

			float frac = d0 / ( d0 - d1 );
			mvertex_t* vert = &pbverts[numbverts++];
			for ( int i = 0; i < 3; i++ )
				vert->position[i] = ( vert1->position[i] - vert0->position[i] ) * frac + vert0->position[i];

			// split into two edges, one on each side, and remember entering
			// and exiting points
			// FIXME: share the clip edge by having a winding direction flag?
			if ( numbedges >= MAX_BMODEL_EDGES - 1 )
			{
				Con_Printf( "Out of edges for bmodel\n" );
				return;
			}

			bedge_t* clipped = &pbedges[numbedges];
			numbedges += 2;
			clipped[0].v[0] = vert0;
			clipped[0].v[1] = vert;
			clipped[0].pnext = psideedges[side0];
			psideedges[side0] = &clipped[0];
			clipped[1].v[0] = vert;
			clipped[1].v[1] = vert1;
			clipped[1].pnext = psideedges[side1];
			psideedges[side1] = &clipped[1];
			makeclippededge = TRUE;
			if ( side1 )
				pfrontexit = vert;
			else
				pfrontenter = vert;
		}
		edge = next;
	}

// if anything was clipped, reconstitute and add the edges along the clip
// plane to both sides (but in opposite directions)
	if ( makeclippededge )
	{
		if ( numbedges >= MAX_BMODEL_EDGES - 2 )
		{
			Con_Printf( "Out of edges for bmodel\n" );
			return;
		}

		bedge_t* edge = &pbedges[numbedges];
		numbedges += 2;
		edge[0].v[0] = pfrontexit;
		edge[0].v[1] = pfrontenter;
		edge[0].pnext = psideedges[0];
		psideedges[0] = &edge[0];
		edge[1].v[0] = pfrontenter;
		edge[1].v[1] = pfrontexit;
		edge[1].pnext = psideedges[1];
		psideedges[1] = &edge[1];
	}

	for ( int side = 0; side < 2; side++ )
	{
		if ( !psideedges[side] )
			continue;
		mnode_t* child = pnode->children[side];
		if ( child->visframe != r_visframecount )
			continue;
		if ( child->contents >= 0 )
			R_RecursiveClipBPoly( psideedges[side], child, psurf );
		else if ( child->contents != CONTENTS_SOLID )
		{
			r_currentbkey = ( (mleaf_t*)child )->key;
			R_RenderBmodelFace( psideedges[side], psurf );
		}
	}
}

/*
================
R_DrawSolidClippedSubmodelPolygons
================
*/
void R_DrawSolidClippedSubmodelPolygons( model_t* pmodel )
{
	mvertex_t bverts[MAX_BMODEL_VERTS];
	bedge_t bedges[MAX_BMODEL_EDGES];
	msurface_t* surf = pmodel->surfaces + pmodel->firstmodelsurface;
	for ( int i = 0; i < pmodel->nummodelsurfaces; i++, surf++ )
	{
		mplane_t* plane = surf->plane;
		if ( ( surf->flags & SURF_DRAWTURB )
			&& ( plane->type != 2 || pmodel->mins[2] + 1.0f >= plane->dist ) )
			continue;

		double dot = DotProduct( modelorg, plane->normal ) - plane->dist;
		qboolean visible = ( surf->flags & SURF_PLANEBACK ) ? dot < -0.01 : dot > 0.01;
		if ( !visible && !( surf->flags & SURF_DRAWTURB ) )
			continue;

		// copy the edges to bedges, flipping if necessary so always
		// clockwise winding
		// FIXME: if edges and vertices get caches, these assignments must move
		// outside the loop, and overflow checking must be done here
		pbverts = bverts;
		pbedges = bedges;
		numbverts = 0;
		numbedges = surf->numedges;
		if ( visible && numbedges <= 0 )
		{
			Sys_Error( "no edges in bmodel" );
			continue;
		}

		for ( int j = 0; j < numbedges; j++ )
		{
			int index = pmodel->surfedges[surf->firstedge + ( visible ? j : numbedges - 1 - j )];
			medge_t* edge = &pmodel->edges[index > 0 ? index : -index];
			int side = ( index <= 0 ) ^ !visible;
			bedges[j].v[0] = &r_pcurrentvertbase[edge->v[side]];
			bedges[j].v[1] = &r_pcurrentvertbase[edge->v[side ^ 1]];
			bedges[j].pnext = &bedges[j + 1];
		}
		bedges[numbedges - 1].pnext = NULL; // mark end of edges
		R_RecursiveClipBPoly( bedges, currententity->topnode, surf );
	}
}

void R_RenderBmodelFaceReverse( model_t* pmodel, msurface_t* psurf )
{
	mvertex_t bverts[32];
	bedge_t bedges[32];
	pbedges = bedges;
	pbverts = bverts;
	numbverts = 0;
	numbedges = psurf->numedges;
	if ( numbedges > 32 )
	{
		Con_Printf( "Out of edges!\n" );
		return;
	}

	for ( int i = 0; i < numbedges; i++ )
	{
		int index = pmodel->surfedges[psurf->firstedge + numbedges - 1 - i];
		medge_t* edge = &pmodel->edges[index > 0 ? index : -index];
		int side = index > 0;
		bedges[i].v[0] = &r_pcurrentvertbase[edge->v[side]];
		bedges[i].v[1] = &r_pcurrentvertbase[edge->v[side ^ 1]];
		bedges[i].pnext = &bedges[i + 1];
	}
	bedges[numbedges - 1].pnext = NULL;
	R_RenderBmodelFace( bedges, psurf );
}

/*
================
R_DrawSubmodelPolygons
================
*/
void R_DrawSubmodelPolygons( model_t* pmodel, int clipflags )
{
	msurface_t* surf = pmodel->surfaces + pmodel->firstmodelsurface;
	for ( int i = 0; i < pmodel->nummodelsurfaces; i++, surf++ )
	{
		mplane_t* plane = surf->plane;
		if ( ( surf->flags & SURF_DRAWTURB )
			&& ( plane->type != 2 || pmodel->mins[2] + 1.0f >= plane->dist ) )
			continue;
		double dot = DotProduct( modelorg, plane->normal ) - plane->dist;
		if ( ( surf->flags & SURF_PLANEBACK ) ? dot < -0.01 : dot > 0.01 )
		{
			r_currentkey = ( (mleaf_t*)currententity->topnode )->key;
			R_RenderFace( surf, clipflags );
		}
		else if ( surf->flags & SURF_DRAWTURB )
		{
			r_currentbkey = ( (mleaf_t*)currententity->topnode )->key;
			r_clipflags = clipflags;
			R_RenderBmodelFaceReverse( pmodel, surf );
		}
	}
}

/*
================
R_RecursiveWorldNode
================
*/
void R_RecursiveWorldNode( mnode_t* node, int clipflags )
{
	if ( node->contents == CONTENTS_SOLID || node->visframe != r_visframecount )
		return;

	for ( int i = 0; i < 4; i++ )
	{
		if ( !( clipflags & ( 1 << i ) ) )
			continue; // don't need to clip against it
		int* indexes = pfrustum_indexes[i];
		clipplane_t* clip = &view_clipplanes[i];
		double dot = (double)( node->minmaxs[indexes[0]] * clip->normal[0] + node->minmaxs[indexes[1]] * clip->normal[1]
							  + node->minmaxs[indexes[2]] * clip->normal[2] )
			- clip->dist;
		if ( dot <= 0.0 )
			return;
		dot = (double)( node->minmaxs[indexes[3]] * clip->normal[0] + node->minmaxs[indexes[4]] * clip->normal[1]
					   + node->minmaxs[indexes[5]] * clip->normal[2] )
			- clip->dist;
		if ( dot >= 0.0 )
			clipflags &= ~( 1 << i ); // node is entirely on screen
	}

// if a leaf node, draw stuff
	if ( node->contents < 0 )
	{
		mleaf_t* leaf = (mleaf_t*)node;
		for ( int i = 0; i < leaf->nummarksurfaces; i++ )
			leaf->firstmarksurface[i]->visframe = r_framecount;
		if ( leaf->efrags )
			R_StoreEfrags( &leaf->efrags );
		leaf->key = r_currentkey++;
		return;
	}

	mplane_t* plane = node->plane;
	double dot;
	if ( plane->type < 3 )
		dot = modelorg[plane->type] - plane->dist;
	else
		dot = DotProduct( modelorg, plane->normal ) - plane->dist;
	int side = dot < 0.0;
	// recurse down the children, front side first
	R_RecursiveWorldNode( node->children[side], clipflags );
	msurface_t* surf = cl.worldmodel->surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ )
	{
		if ( surf->visframe != r_framecount )
			continue;
		if ( !( ( surf->flags & SURF_PLANEBACK ) ? dot < -0.01 : dot > 0.01 ) )
			continue;
		if ( !r_drawpolys )
			R_RenderFace( surf, clipflags );
		else if ( !r_worldpolysbacktofront )
			R_RenderPoly( surf, clipflags );
		else if ( numbtofpolys < MAX_BTOF_POLYS )
		{
			pbtofpolys[numbtofpolys].clipflags = clipflags;
			pbtofpolys[numbtofpolys++].psurf = surf;
		}
	}
	if ( node->numsurfaces )
		++r_currentkey;
	// recurse down the back side
	R_RecursiveWorldNode( node->children[side ^ 1], clipflags );
}

/*
================
R_RenderWorld
================
*/
void R_RenderWorld()
{
	static btofpoly_t btofpolys[MAX_BTOF_POLYS];
	R_ClearSkyBox();
	currententity = cl_entities;
	VectorCopy( r_origin, modelorg );
	r_pcurrentvertbase = currententity->model->vertexes;
	pbtofpolys = btofpolys;
	R_RecursiveWorldNode( currententity->model->nodes, 15 );
	R_DrawSkyBox( r_currentkey );
// if the driver wants the polygons back to front, play the visible ones back
// in that order
	if ( r_worldpolysbacktofront )
	{
		for ( int i = numbtofpolys - 1; i >= 0; i-- )
			R_RenderPoly( pbtofpolys[i].psurf, pbtofpolys[i].clipflags );
	}
}
