#include "r_local.h"
#include "d_local.h"
#include "client.h"
#include "sound.h"

#include <limits.h>
#include <stdint.h>
#include <string.h>

edge_t* auxedges;
edge_t* r_edges;
edge_t* edge_p;
edge_t* edge_max;
surf_t* surfaces;
surf_t* surface_p;
surf_t* surf_max;
// surfaces are generated in back to front order by the bsp, so if a surf
// pointer is greater than another one, it should be drawn in front
// surfaces[1] is the background, and is used as the active surface stack

edge_t* newedges[MAXHEIGHT];
edge_t* removeedges[MAXHEIGHT];
espan_t* span_p;
espan_t* max_span_p;
int r_currentkey;
int current_iv;
int edge_head_u_shift20;
int edge_tail_u_shift20;
edge_t edge_head;
edge_t edge_tail;
edge_t edge_aftertail;
edge_t edge_sentinel;
float fv;

static void ( *pdrawfunc )();

/*
==============
R_BeginEdgeFrame
==============
*/
void R_BeginEdgeFrame()
{
	edge_p = r_edges;
	edge_max = r_edges + r_numallocatededges;
	surface_p = surfaces + 2;
	//  surface 0 is a dummy
	surfaces[1].spans = NULL;	// no background spans yet
	surfaces[1].flags = SURF_DRAWBACKGROUND;

// put the background behind everything in the world
	if ( !r_draworder.value )
	{
		pdrawfunc = R_GenerateSpans;
		surfaces[1].key = INT_MAX;
		r_currentkey = 0;
	}
	else
	{
		pdrawfunc = R_GenerateSpansBackward;
		surfaces[1].key = 0;
		r_currentkey = 1;
	}

	if ( r_refdef.vrect.y < r_refdef.vrectbottom )
	{
		size_t count = (size_t)( r_refdef.vrectbottom - r_refdef.vrect.y );
		memset( &removeedges[r_refdef.vrect.y], 0, count * sizeof( removeedges[0] ) );
		memset( &newedges[r_refdef.vrect.y], 0, count * sizeof( newedges[0] ) );
	}
}

void R_InsertNewEdges( edge_t* edgestoadd, edge_t* edgelist )
{
	do
	{
		edge_t* next = edgestoadd->next;
		while ( edgelist->u < edgestoadd->u )
			edgelist = edgelist->next;

		edgestoadd->next = edgelist;
		edgestoadd->prev = edgelist->prev;
		edgelist->prev->next = edgestoadd;
		edgelist->prev = edgestoadd;
		edgestoadd = next;
	}
	while ( edgestoadd );
}

/*
==============
R_RemoveEdges
==============
*/
void R_RemoveEdges( edge_t* pedge )
{
	do
	{
		pedge->next->prev = pedge->prev;
		pedge->prev->next = pedge->next;
		pedge = pedge->nextremove;
	}
	while ( pedge );
}

/*
==============
R_StepActiveU
==============
*/
void R_StepActiveU( edge_t* pedge )
{
	for ( ;; )
	{
		pedge->u += pedge->u_step;
		if ( pedge->u >= pedge->prev->u )
		{
			pedge = pedge->next;
			continue;
		}

		if ( pedge == &edge_aftertail )
			return;

		edge_t* pnext_edge = pedge->next;
		pnext_edge->prev = pedge->prev;
		pedge->prev->next = pnext_edge;

		edge_t* pwedge = pedge->prev->prev;
		while ( pwedge->u > pedge->u )
			pwedge = pwedge->prev;

		pedge->prev = pwedge;
		pedge->next = pwedge->next;
		pwedge->next->prev = pedge;
		pwedge->next = pedge;

		if ( pnext_edge == &edge_tail )
			return;
		pedge = pnext_edge;
	}
}

/*
==============
R_TrailingEdge
==============
*/
void R_TrailingEdge( surf_t* surf, edge_t* edge )
{
	if ( --surf->spanstate != 0 )
		return;

	if ( surf->insubmodel )
		--r_bmodelactive;

	if ( surf == surfaces[1].next )
	{
		int iu = edge->u >> 20;
		if ( iu > surf->last_u )
		{
			espan_t* span = span_p++;
			span->u = surf->last_u;
			span->count = iu - surf->last_u;
			span->v = current_iv;
			span->pnext = surf->spans;
			surf->spans = span;
		}
		// set last_u on the surface below
		surf->next->last_u = iu;
	}

	surf->prev->next = surf->next;
	surf->next->prev = surf->prev;
}

/*
==============
R_LeadingEdge
==============
*/
void R_LeadingEdge( edge_t* edge )
{
	if ( !edge->surfs[1] )
		return;

	surf_t* surf = &surfaces[edge->surfs[1]];
	if ( ++surf->spanstate != 1 )
		return;

	surf_t* surf2 = surfaces[1].next;
	if ( surf->insubmodel )
	{
		++r_bmodelactive;
		if ( surf->key < surf2->key )
			goto newtop;

		if ( surf->key == surf2->key )
		{
			double fu = (float)( edge->u - 0xFFFFF ) * ( 1.0 / 0x100000 );
			double newzi = (double)( fv * surf->d_zistepv + surf->d_ziorigin ) + surf->d_zistepu * fu;
			double testzi = (double)( fv * surf2->d_zistepv + surf2->d_ziorigin ) + surf2->d_zistepu * fu;
			if ( newzi * 0.99 >= testzi ||
				( newzi * 1.01 >= testzi && surf->d_zistepu >= surf2->d_zistepu ) )
				goto newtop;
		}

		for ( ;; )
		{
			do
				surf2 = surf2->next;
			while ( surf->key > surf2->key );

			if ( surf->key != surf2->key )
				break;

			double fu = (float)( edge->u - 0xFFFFF ) * ( 1.0 / 0x100000 );
			double newzi = (double)( fv * surf->d_zistepv + surf->d_ziorigin ) + surf->d_zistepu * fu;
			double testzi = (double)( fv * surf2->d_zistepv + surf2->d_ziorigin ) + surf2->d_zistepu * fu;
			if ( newzi * 0.99 >= testzi ||
				( newzi * 1.01 >= testzi && surf->d_zistepu >= surf2->d_zistepu ) )
				break;
		}
	}
	else
	{
		if ( surf->key < surf2->key )
			goto newtop;

		do
			surf2 = surf2->next;
		while ( surf2->key <= surf->key );
	}
	goto gotposition;

newtop:
	{
		int iu = edge->u >> 20;
		if ( iu > surf2->last_u )
		{
			espan_t* span = span_p++;
			span->u = surf2->last_u;
			span->count = iu - surf2->last_u;
			span->v = current_iv;
			span->pnext = surf2->spans;
			surf2->spans = span;
		}
		// set last_u on the new span
		surf->last_u = iu;
	}

gotposition:
	// insert before surf2
	surf->next = surf2;
	surf->prev = surf2->prev;
	surf2->prev->next = surf;
	surf2->prev = surf;
}

/*
==============
R_LeadingEdgeBackwards
==============
*/
void R_LeadingEdgeBackwards( edge_t* edge )
{
	surf_t* surf = &surfaces[edge->surfs[1]];
	if ( ++surf->spanstate != 1 )
		return;

	surf_t* surf2 = surfaces[1].next;
	if ( surf->key > surf2->key || ( surf->insubmodel && surf->key == surf2->key ) )
	{
		int iu = edge->u >> 20;
		if ( iu > surf2->last_u )
		{
			espan_t* span = span_p++;
			span->u = surf2->last_u;
			span->count = iu - surf2->last_u;
			span->v = current_iv;
			span->pnext = surf2->spans;
			surf2->spans = span;
		}
		surf->last_u = iu;
	}
	else
	{
		do
		{
			do
				surf2 = surf2->next;
			while ( surf->key < surf2->key );
		}
		while ( surf->key == surf2->key && !surf->insubmodel );
	}

	// insert before surf2
	surf->next = surf2;
	surf->prev = surf2->prev;
	surf2->prev->next = surf;
	surf2->prev = surf;
}

/*
==============
R_CleanupSpan
==============
*/
void R_CleanupSpan()
{
	surf_t* surf = surfaces[1].next;
	if ( edge_tail_u_shift20 > surf->last_u )
	{
		espan_t* span = span_p++;
		span->u = surf->last_u;
		span->count = edge_tail_u_shift20 - surf->last_u;
		span->v = current_iv;
		span->pnext = surf->spans;
		surf->spans = span;
	}

// reset spanstate for all surfaces in the surface stack
	do
	{
		surf->spanstate = 0;
		surf = surf->next;
	}
	while ( surf != &surfaces[1] );
}

/*
==============
R_GenerateSpans
==============
*/
void R_GenerateSpans()
{
	r_bmodelactive = 0;
	surfaces[1].next = &surfaces[1];
	surfaces[1].prev = &surfaces[1];
	surfaces[1].last_u = edge_head_u_shift20;

// generate spans
	for ( edge_t* edge = edge_head.next; edge != &edge_tail; edge = edge->next )
	{
		if ( edge->surfs[0] )
			R_TrailingEdge( &surfaces[edge->surfs[0]], edge );
		if ( edge->surfs[1] )
			R_LeadingEdge( edge );
	}

	R_CleanupSpan();
}

/*
==============
R_GenerateSpansBackward
==============
*/
void R_GenerateSpansBackward()
{
	r_bmodelactive = 0;
	surfaces[1].next = &surfaces[1];
	surfaces[1].prev = &surfaces[1];
	surfaces[1].last_u = edge_head_u_shift20;

// generate spans
	for ( edge_t* edge = edge_head.next; edge != &edge_tail; edge = edge->next )
	{
		if ( edge->surfs[0] )
			R_TrailingEdge( &surfaces[edge->surfs[0]], edge );
		if ( edge->surfs[1] )
			R_LeadingEdgeBackwards( edge );
	}

	R_CleanupSpan();
}

/*
==============
R_DrawCulledPolys
==============
*/
void R_DrawCulledPolys()
{
	currententity = cl_entities;
	if ( r_worldpolysbacktofront )
	{
		for ( surf_t* surf = surface_p - 1; surf > &surfaces[1]; --surf )
		{
			if ( surf->spans && !( surf->flags & SURF_DRAWBACKGROUND ) )
				R_RenderPoly( (msurface_t*)surf->data, 15 );
		}
	}
	else
	{
		for ( surf_t* surf = surfaces + 1; surf < surface_p; ++surf )
		{
			if ( surf->spans && !( surf->flags & SURF_DRAWBACKGROUND ) )
				R_RenderPoly( (msurface_t*)surf->data, 15 );
		}
	}
}

void R_ScanEdges()
{
	byte basespans[MAXSPANS * sizeof( espan_t ) + CACHE_SIZE];
	espan_t* basespan_p = (espan_t*)( (uintptr_t)&basespans[CACHE_SIZE - 1] & ~(uintptr_t)( CACHE_SIZE - 1 ) );
	span_p = basespan_p;
	max_span_p = basespan_p + MAXSPANS - r_refdef.vrect.width;

// clear active edges to just the background edges around the whole screen
// FIXME: most of this only needs to be set up once
	edge_head.u = r_refdef.vrect.x << 20;
	edge_head_u_shift20 = edge_head.u >> 20;
	edge_head.u_step = 0;
	edge_head.prev = NULL;
	edge_head.next = &edge_tail;
	edge_head.surfs[0] = 0;
	edge_head.surfs[1] = 1;
	edge_tail.u = ( r_refdef.vrectright << 20 ) + 0xFFFFF;
	edge_tail_u_shift20 = edge_tail.u >> 20;
	edge_tail.u_step = 0;
	edge_tail.prev = &edge_head;
	edge_tail.next = &edge_aftertail;
	edge_tail.surfs[0] = 1;
	edge_tail.surfs[1] = 0;
	edge_aftertail.u = -1;		// force a move
	edge_aftertail.u_step = 0;
	edge_aftertail.prev = &edge_tail;
	edge_aftertail.next = &edge_sentinel;
	edge_sentinel.u = -805306368;
	edge_sentinel.prev = &edge_aftertail;

	int bottom = r_refdef.vrectbottom - 1;
	int iv = r_refdef.vrect.y;
	for ( ; iv < bottom; ++iv )
	{
		current_iv = iv;
		fv = (float)iv;
		// mark that the head (background start) span is pre-included
		surfaces[1].spanstate = 1;
		if ( newedges[iv] )
			R_InsertNewEdges( newedges[iv], edge_head.next );
		pdrawfunc();

		if ( span_p >= max_span_p )
		{
			S_ExtraUpdate(); // don't let sound get messed up if going slow
			if ( r_drawculledpolys )
				R_DrawCulledPolys();
			else
				D_DrawSurfaces();

			for ( surf_t* surf = surfaces + 1; surf < surface_p; ++surf )
				surf->spans = NULL;
			span_p = basespan_p;
		}

		if ( removeedges[iv] )
			R_RemoveEdges( removeedges[iv] );
		if ( edge_head.next != &edge_tail )
			R_StepActiveU( edge_head.next );
	}

// do the last scan (no need to step or sort or remove on the last scan)

	current_iv = iv;
	fv = (float)iv;
// mark that the head (background start) span is pre-included
	surfaces[1].spanstate = 1;
	if ( newedges[iv] )
		R_InsertNewEdges( newedges[iv], edge_head.next );
	pdrawfunc();

// draw whatever's left in the span list
	if ( r_drawculledpolys )
		R_DrawCulledPolys();
	else
		D_DrawSurfaces();
}
