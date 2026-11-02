#include "client.h"
#include "conprint.h"
#include "gl_model.h"
#include "sys.h"
#include "entity_types.h"

extern int r_framecount;

mnode_t* r_pefragtopnode;

//===========================================================================
/*
===============================================================================

					ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

efrag_t** lastlink;

vec3_t r_emins, r_emaxs;

cl_entity_t* r_addent;

static int Efrag_BoxOnPlaneSide( mplane_t* plane )
{
	if ( plane->type > 2 )
		return BoxOnPlaneSide( r_emins, r_emaxs, plane );

	if ( plane->dist <= r_emins[plane->type] )
		return 1;
	if ( plane->dist >= r_emaxs[plane->type] )
		return 2;
	return 3;
}

/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position
================
*/
void R_RemoveEfrags( cl_entity_t* ent )
{
	efrag_t *ef, *old, **prev;

	ef = ent->efrag;

	while ( ef )
	{
		prev = &ef->leaf->efrags;
		while ( *prev )
		{
			if ( *prev == ef )
			{
				// remove this fragment
				*prev = ef->leafnext;
				break;
			}
			prev = &( *prev )->leafnext;
		}

		old = ef;
		ef = ef->entnext;

		// put it on the free list
		old->entnext = cl.free_efrags;
		cl.free_efrags = old;
	}

	ent->efrag = NULL;
}

/*
===================
R_SplitEntityOnNode
===================
*/
void R_SplitEntityOnNode( mnode_t* node )
{
	efrag_t* ef;
	mleaf_t* leaf;
	int sides;

	if ( node->contents == CONTENTS_SOLID )
		return;

	// add an efrag if the node is a leaf
	if ( node->contents < 0 )
	{
		if ( !r_pefragtopnode )
			r_pefragtopnode = node;

		leaf = (mleaf_t*)node;

		// grab an efrag off the free list
		ef = cl.free_efrags;
		if ( !ef )
		{
			Con_Printf( "Too many efrags!\n" );
			return; // no free fragments...
		}
		cl.free_efrags = ef->entnext;

		ef->entity = r_addent;

		// add the entity link
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;

		// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;
		return;
	}

	// NODE_MIXED
	sides = Efrag_BoxOnPlaneSide( node->plane );

	// split on this plane
	// if this is the first splitter of this bmodel, remember it
	if ( sides == 3 && !r_pefragtopnode )
		r_pefragtopnode = node;

	// recurse down the contacted sides
	if ( sides & 1 )
		R_SplitEntityOnNode( node->children[0] );
	if ( sides & 2 )
		R_SplitEntityOnNode( node->children[1] );
}

/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags( cl_entity_t* ent )
{
	model_t* entmodel;
	int i;

	entmodel = ent->model;
	if ( !entmodel )
		return;

	lastlink = &ent->efrag;
	r_addent = ent;
	r_pefragtopnode = NULL;

	for ( i = 0; i < 3; i++ )
	{
		r_emins[i] = ent->origin[i] + entmodel->mins[i];
		r_emaxs[i] = ent->origin[i] + entmodel->maxs[i];
	}

	R_SplitEntityOnNode( cl.worldmodel->nodes );

	ent->topnode = r_pefragtopnode;
}

/*
================
R_StoreEfrags

// FIXME: a lot of this goes away with edge-based
================
*/
void R_StoreEfrags( efrag_t** ppefrag )
{
	cl_entity_t* pent;
	efrag_t* pefrag;

	for ( pefrag = *ppefrag; pefrag; pefrag = pefrag->leafnext )
	{
		pent = pefrag->entity;
		if ( pent->model->type != mod_brush && pent->model->type != mod_sprite
			 && pent->model->type != mod_alias )
			Sys_Error( "R_StoreEfrags: Bad entity type %d\n", pent->model->type );

		if ( pent->visframe != r_framecount && cl_numvisedicts < MAX_VISEDICTS
			 && ClientDLL_AddEntity( ET_FRAGMENTED, pent ) )
		{
			// mark that we've recorded this entity for this frame
			pent->visframe = r_framecount;
		}
	}
}
