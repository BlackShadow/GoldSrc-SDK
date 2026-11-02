#include "r_local.h"
#include "../common/cl_entity.h"

#include <cmath>

unsigned int	cacheoffset;
int				c_faceclip; // number of faces clipped
zpointdesc_t		r_zpointdesc;
polydesc_t		r_polydesc;
clipplane_t*	entity_clipplanes;
clipplane_t		view_clipplanes[4];
clipplane_t		world_clipplanes[16];
medge_t*		r_pedge;
qboolean		r_leftclipped;
qboolean		r_rightclipped;
qboolean		r_nearzionly;
mvertex_t		r_leftenter;
mvertex_t		r_leftexit;
mvertex_t		r_rightenter;
mvertex_t		r_rightexit;
int				r_emitted;
float			r_nearzi;
float			r_u1;
float			r_v1;
float			r_lzi1;
int				r_ceilv1;
qboolean		r_lastvertvalid;

static qboolean	makeleftedge;
static qboolean	makerightedge;

/*
================
R_EmitEdge
================
*/
void R_EmitEdge( mvertex_t* pv0, mvertex_t* pv1 )
{
	float u0, v0, lzi0, scale;
	int ceilv0;
	vec3_t local, transformed;

	if ( r_lastvertvalid )
	{
		u0 = r_u1;
		v0 = r_v1;
		lzi0 = r_lzi1;
		ceilv0 = r_ceilv1;
	}
	else
	{
		VectorSubtract( pv0->position, modelorg, local );
		TransformVector( local, transformed );
		if ( transformed[2] < 0.01 )
			transformed[2] = 0.01f;
		lzi0 = (float)( 1.0 / transformed[2] );
		// FIXME: build x/yscale into transform?
		scale = xscale * lzi0;
		u0 = scale * transformed[0] + xcenter;
		if ( u0 < r_refdef.fvrectx_adj )
			u0 = r_refdef.fvrectx_adj;
		if ( u0 > r_refdef.fvrectright_adj )
			u0 = r_refdef.fvrectright_adj;
		scale = yscale * lzi0;
		v0 = ycenter - scale * transformed[1];
		if ( v0 < r_refdef.fvrecty_adj )
			v0 = r_refdef.fvrecty_adj;
		if ( v0 > r_refdef.fvrectbottom_adj )
			v0 = r_refdef.fvrectbottom_adj;
		ceilv0 = (int)ceil( v0 );
	}

	VectorSubtract( pv1->position, modelorg, local );
	TransformVector( local, transformed );
	if ( transformed[2] < 0.01 )
		transformed[2] = 0.01f;
	r_lzi1 = (float)( 1.0 / transformed[2] );
	scale = xscale * r_lzi1;
	r_u1 = scale * transformed[0] + xcenter;
	if ( r_u1 < r_refdef.fvrectx_adj )
		r_u1 = r_refdef.fvrectx_adj;
	if ( r_u1 > r_refdef.fvrectright_adj )
		r_u1 = r_refdef.fvrectright_adj;
	scale = yscale * r_lzi1;
	r_v1 = ycenter - scale * transformed[1];
	if ( r_v1 < r_refdef.fvrecty_adj )
		r_v1 = r_refdef.fvrecty_adj;
	if ( r_v1 > r_refdef.fvrectbottom_adj )
		r_v1 = r_refdef.fvrectbottom_adj;

	if ( r_lzi1 > lzi0 )
		lzi0 = r_lzi1;
	if ( lzi0 > r_nearzi ) // for mipmap finding
		r_nearzi = lzi0;

	// for right edges, all we want is the effect on 1/z
	if ( r_nearzionly )
		return;

	r_emitted = 1;
	r_ceilv1 = (int)ceil( r_v1 );

	// create the edge
	if ( ceilv0 == r_ceilv1 )
	{
		// we cache unclipped horizontal edges as fully clipped
		if ( cacheoffset != 0x7FFFFFFF )
			cacheoffset = r_framecount | 0x80000000;
		return; // horizontal edge
	}

	edge_t* edge = edge_p++;
	edge->owner = r_pedge;
	edge->nearzi = lzi0;
	int lastv;
	float ustep, ustart;
	if ( ceilv0 <= r_ceilv1 )
	{
		// trailing edge (go from p1 to p2)
		lastv = r_ceilv1 - 1;
		edge->surfs[0] = (unsigned short)( surface_p - surfaces );
		edge->surfs[1] = 0;
		ustep = ( r_u1 - u0 ) / ( r_v1 - v0 );
		ustart = ( (float)ceilv0 - v0 ) * ustep + u0;
	}
	else
	{
		// leading edge (go from p2 to p1)
		lastv = ceilv0 - 1;
		ceilv0 = r_ceilv1;
		edge->surfs[0] = 0;
		edge->surfs[1] = (unsigned short)( surface_p - surfaces );
		ustep = ( u0 - r_u1 ) / ( v0 - r_v1 );
		ustart = ( (float)ceilv0 - r_v1 ) * ustep + r_u1;
	}
	edge->u_step = (int)( ustep * 0x100000 );
	edge->u = (int)( ustart * 0x100000 + 0xFFFFF );
// we need to do this to avoid stepping off the edges if a very nearly
// horizontal edge is less than epsilon above a scan, and numeric error causes
// it to incorrectly extend to the scan, and the extension of the line goes off
// the edge of the screen
// FIXME: is this actually needed?
	if ( edge->u < r_refdef.vrect_x_adj_shift20 )
		edge->u = r_refdef.vrect_x_adj_shift20;
	if ( edge->u > r_refdef.vrectright_adj_shift20 )
		edge->u = r_refdef.vrectright_adj_shift20;

	fixed16_t sortu = edge->u + ( edge->surfs[0] != 0 );
	edge_t* next = newedges[ceilv0];
	if ( next && sortu > next->u )
	{
		while ( next->next && sortu > next->next->u )
			next = next->next;
		edge->next = next->next;
		next->next = edge;
	}
	else
	{
		edge->next = next;
		newedges[ceilv0] = edge;
	}
	edge->nextremove = removeedges[lastv];
	removeedges[lastv] = edge;
}

/*
================
R_ClipEdge
================
*/
void R_ClipEdge( mvertex_t* pv0, mvertex_t* pv1, clipplane_t* clip )
{
	for ( ; clip; clip = clip->next )
	{
		float d0 = DotProduct( pv0->position, clip->normal ) - clip->dist;
		float d1 = DotProduct( pv1->position, clip->normal ) - clip->dist;
		if ( d0 < 0.0f )
		{
			if ( d1 < 0.0f )
			{
				if ( !r_leftclipped )
					cacheoffset = r_framecount | 0x80000000;
				return;
			}
		}
		else if ( !( d1 < 0.0f ) )
			continue;

		mvertex_t vert;
		float frac = d0 / ( d0 - d1 );
		cacheoffset = 0x7FFFFFFF;
		for ( int i = 0; i < 3; i++ )
			vert.position[i] = ( pv1->position[i] - pv0->position[i] ) * frac + pv0->position[i];
		if ( !( d0 < 0.0f ) )
		{
			if ( clip->leftedge )
			{
				r_leftclipped = TRUE;
				r_leftexit = vert;
			}
			else if ( clip->rightedge )
			{
				r_rightclipped = TRUE;
				r_rightexit = vert;
			}
			R_ClipEdge( pv0, &vert, clip->next );
		}
		else
		{
			r_lastvertvalid = FALSE;
			if ( clip->leftedge )
			{
				r_leftclipped = TRUE;
				r_leftenter = vert;
			}
			else if ( clip->rightedge )
			{
				r_rightclipped = TRUE;
				r_rightenter = vert;
			}
			R_ClipEdge( &vert, pv1, clip->next );
		}
		return;
	}
// add the edge
	R_EmitEdge( pv0, pv1 );
}

/*
================
R_EmitCachedEdge
================
*/
void R_EmitCachedEdge()
{
	edge_t* edge = (edge_t*)( (byte*)r_edges + r_pedge->cachededgeoffset );
	if ( edge->surfs[0] )
		edge->surfs[1] = (unsigned short)( surface_p - surfaces );
	else
		edge->surfs[0] = (unsigned short)( surface_p - surfaces );
	if ( edge->nearzi > r_nearzi ) // for mipmap finding
		r_nearzi = edge->nearzi;
	r_emitted = 1;
}

/*
================
R_RenderFace
================
*/
void R_RenderFace( msurface_t* fa, int clipflags )
{
	if ( surface_p >= surf_max )
	{
		++r_outofsurfaces;
		return;
	}
	if ( edge_max <= edge_p + fa->numedges + 4 )
	{
		r_outofedges += fa->numedges;
		return;
	}
	if ( fa->flags & SURF_DRAWSKY )
	{
		R_DrawSkyChain( fa );
		return;
	}

	++c_faceclip;
	clipplane_t* clip = NULL;
	for ( int i = 3; i >= 0; i-- )
	{
		if ( clipflags & ( 1 << i ) )
		{
			view_clipplanes[i].next = clip;
			clip = &view_clipplanes[i];
		}
	}
	makeleftedge = FALSE;
	makerightedge = FALSE;
// push the edges through
	r_emitted = 0;
	r_nearzionly = FALSE;
	r_nearzi = 0;
	r_lastvertvalid = FALSE;

	for ( int i = 0; i < fa->numedges; i++ )
	{
		model_t* model = currententity->model;
		int index = model->surfedges[fa->firstedge + i];
		r_pedge = &model->edges[index > 0 ? index : -index];
		// if the edge is cached, we can just reuse the edge
		if ( !insubmodel )
		{
			unsigned int offset = r_pedge->cachededgeoffset;
			if ( offset & 0x80000000 )
			{
				if ( ( offset & 0x7FFFFFFF ) == (unsigned int)r_framecount )
				{
					r_lastvertvalid = FALSE;
					continue;
				}
			}
			else if ( offset < (unsigned int)( (byte*)edge_p - (byte*)r_edges )
				&& ( (edge_t*)( (byte*)r_edges + offset ) )->owner == r_pedge )
			{
				R_EmitCachedEdge();
				r_lastvertvalid = FALSE;
				continue;
			}
		}

		cacheoffset = (unsigned int)( (byte*)edge_p - (byte*)r_edges );
		r_leftclipped = FALSE;
		r_rightclipped = FALSE;
		int side = index <= 0;
		R_ClipEdge( &r_pcurrentvertbase[r_pedge->v[side]], &r_pcurrentvertbase[r_pedge->v[side ^ 1]], clip );
		r_pedge->cachededgeoffset = cacheoffset;
		if ( r_leftclipped )
			makeleftedge = TRUE;
		if ( r_rightclipped )
			makerightedge = TRUE;
		r_lastvertvalid = TRUE;
	}

	medge_t tedge;
// if there was a clip off the left edge, add that edge too
// FIXME: faster to do in screen space?
// FIXME: share clipped edges?
	if ( makeleftedge )
	{
		r_pedge = &tedge;
		r_lastvertvalid = FALSE;
		R_ClipEdge( &r_leftexit, &r_leftenter, clip->next );
	}
// if there was a clip off the right edge, get the right r_nearzi
	if ( makerightedge )
	{
		r_pedge = &tedge;
		r_lastvertvalid = FALSE;
		r_nearzionly = TRUE;
		R_ClipEdge( &r_rightexit, &r_rightenter, view_clipplanes[1].next );
	}
// if no edges made it out, return without posting the surface
	if ( !r_emitted )
		return;

	++r_polycount;
	surface_p->nearzi = r_nearzi;
	surface_p->data = fa;
	surface_p->spanstate = 0;
	surface_p->spans = NULL;
	surface_p->flags = fa->flags;
	surface_p->insubmodel = insubmodel;
	surface_p->entity = currententity;
	surface_p->key = r_currentkey++;
	mplane_t* plane = fa->plane;
	vec3_t normal;
	TransformVector( plane->normal, normal );
	float distinv = (float)( 1.0 / ( plane->dist - DotProduct( modelorg, plane->normal ) ) );
	surface_p->d_zistepu = normal[0] * xscaleinv * distinv;
	surface_p->d_zistepv = -normal[1] * yscaleinv * distinv;
	surface_p->d_ziorigin = normal[2] * distinv - xcenter * surface_p->d_zistepu - ycenter * surface_p->d_zistepv;
	++surface_p;
}

/*
================
R_RenderBmodelFace
================
*/
void R_RenderBmodelFace( bedge_t* pedges, msurface_t* psurf )
{
	if ( surface_p >= surf_max )
		return;
	if ( edge_max <= edge_p + psurf->numedges + 4 )
		return;

	++c_faceclip;
	medge_t tedge;
// this is a dummy to give the caching mechanism someplace to write to
	r_pedge = &tedge;
	clipplane_t* clip = NULL;
	for ( int i = 3; i >= 0; i-- )
	{
		if ( r_clipflags & ( 1 << i ) )
		{
			view_clipplanes[i].next = clip;
			clip = &view_clipplanes[i];
		}
	}
	makeleftedge = FALSE;
	makerightedge = FALSE;
// push the edges through
	r_emitted = 0;
	r_nearzi = 0;
	r_nearzionly = FALSE;
	r_lastvertvalid = FALSE;

	for ( bedge_t* edge = pedges; edge; edge = edge->pnext )
	{
		r_leftclipped = FALSE;
		r_rightclipped = FALSE;
		R_ClipEdge( edge->v[0], edge->v[1], clip );
		if ( r_leftclipped )
			makeleftedge = TRUE;
		if ( r_rightclipped )
			makerightedge = TRUE;
	}
	if ( pedges )
	{
// if there was a clip off the left edge, add that edge too
// FIXME: faster to do in screen space?
// FIXME: share clipped edges?
		if ( makeleftedge )
		{
			r_pedge = &tedge;
			R_ClipEdge( &r_leftexit, &r_leftenter, clip->next );
		}
// if there was a clip off the right edge, get the right r_nearzi
		if ( makerightedge )
		{
			r_pedge = &tedge;
			r_nearzionly = TRUE;
			R_ClipEdge( &r_rightexit, &r_rightenter, view_clipplanes[1].next );
		}
	}
// if no edges made it out, return without posting the surface
	if ( !r_emitted )
		return;

	++r_polycount;
	surface_p->nearzi = r_nearzi;
	surface_p->data = psurf;
	surface_p->insubmodel = TRUE;
	surface_p->spanstate = 0;
	surface_p->spans = NULL;
	surface_p->flags = psurf->flags;
	surface_p->entity = currententity;
	surface_p->key = r_currentbkey;
	mplane_t* plane = psurf->plane;
	vec3_t normal;
	TransformVector( plane->normal, normal );
	float distinv = (float)( 1.0 / ( plane->dist - DotProduct( modelorg, plane->normal ) ) );
	surface_p->d_zistepu = normal[0] * xscaleinv * distinv;
	surface_p->d_zistepv = -normal[1] * yscaleinv * distinv;
	surface_p->d_ziorigin = normal[2] * distinv - xcenter * surface_p->d_zistepu - ycenter * surface_p->d_zistepv;
	++surface_p;
}

/*
================
R_RenderPoly
================
*/
void R_RenderPoly( msurface_t* fa, int clipflags )
{
	clipplane_t* clip = NULL;
	for ( int i = 3; i >= 0; i-- )
	{
		if ( clipflags & ( 1 << i ) )
		{
			view_clipplanes[i].next = clip;
			clip = &view_clipplanes[i];
		}
	}

	mvertex_t verts[2][100]; //FIXME: do real number
	polyvert_t pverts[100]; //FIXME: do real number, safely
	model_t* model = currententity->model;
	int numverts = fa->numedges;
	for ( int i = 0; i < numverts; i++ )
	{
		int index = model->surfedges[fa->firstedge + i];
		r_pedge = &model->edges[index > 0 ? index : -index];
		verts[0][i] = r_pcurrentvertbase[r_pedge->v[index <= 0]];
	}
	int page = 0;
	for ( ; clip; clip = clip->next )
	{
		mvertex_t* in = verts[page];
		mvertex_t* out = verts[page ^ 1];
		int count = 0;
		qboolean visible = FALSE;
		int last = numverts - 1;
		float lastdist = DotProduct( in[last].position, clip->normal ) - clip->dist;
		for ( int i = 0; i < numverts; last = i++ )
		{
			float dist = DotProduct( in[i].position, clip->normal ) - clip->dist;
			if ( ( lastdist > 0.0f ) != ( dist > 0.0f ) )
			{
				float frac = dist / ( dist - lastdist );
				for ( int j = 0; j < 3; j++ )
					out[count].position[j] = in[i].position[j] + ( in[last].position[j] - in[i].position[j] ) * frac;
				++count;
			}
			if ( !( dist < 0.0f ) )
			{
				out[count++] = in[i];
				visible = TRUE;
			}
			lastdist = dist;
		}
		if ( !visible || count <= 2 )
			return;
		page ^= 1;
		numverts = count;
	}

	int saxis = 0, taxis = 0;
	switch ( fa->plane->type )
	{
	case 0:
	case 3:
		saxis = 1;
		taxis = 2;
		break;
	case 1:
	case 4:
		taxis = 2;
		break;
	case 2:
	case 5:
		taxis = 1;
		break;
	}
	r_nearzi = 0;
	for ( int i = 0; i < numverts; i++ )
	{
		vec3_t local, transformed;
		mvertex_t* vert = &verts[page][i];
		VectorSubtract( vert->position, modelorg, local );
		TransformVector( local, transformed );
		if ( transformed[2] < 0.01 )
			transformed[2] = 0.01f;
		float zi = (float)( 1.0 / transformed[2] );
		if ( zi > r_nearzi ) // for mipmap finding
			r_nearzi = zi;
		pverts[i].s = vert->position[saxis];
		pverts[i].t = vert->position[taxis];
		float u = zi * xscale * transformed[0] + xcenter;
		if ( u < r_refdef.fvrectx_adj )
			u = r_refdef.fvrectx_adj;
		if ( u > r_refdef.fvrectright_adj )
			u = r_refdef.fvrectright_adj;
		float v = ycenter - zi * yscale * transformed[1];
		if ( v < r_refdef.fvrecty_adj )
			v = r_refdef.fvrecty_adj;
		if ( v > r_refdef.fvrectbottom_adj )
			v = r_refdef.fvrectbottom_adj;
		pverts[i].u = u;
		pverts[i].v = v;
		pverts[i].zi = zi;
	}
	r_polydesc.nearzi = r_nearzi;
	r_polydesc.numverts = numverts;
	r_polydesc.pcurrentface = fa;
	r_polydesc.pverts = pverts;
// draw the polygon
	D_DrawPoly();
}

/*
================
R_ZDrawSubmodelPolys
================
*/
void R_ZDrawSubmodelPolys( model_t* pmodel )
{
	msurface_t* surf = pmodel->surfaces + pmodel->firstmodelsurface;
	for ( int i = 0; i < pmodel->nummodelsurfaces; i++, surf++ )
	{
		mplane_t* plane = surf->plane;
		double dot = DotProduct( modelorg, plane->normal ) - plane->dist;
		if ( ( surf->flags & SURF_PLANEBACK ) ? dot < -0.01 : dot > 0.01 )
			R_RenderPoly( surf, 15 );
	}
}
