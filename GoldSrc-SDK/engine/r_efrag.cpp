#include "r_local.h"
#include "client.h"
#include "conprint.h"
#include "../common/entity_types.h"
#include "sys.h"

vec3_t r_emins, r_emaxs;
mnode_t* r_pefragtopnode;
//===========================================================================

/*
===============================================================================

					ENTITY FRAGMENT FUNCTIONS

===============================================================================
*/

efrag_t** lastlink;
cl_entity_t* r_addent;

/*
================
R_RemoveEfrags

Call when removing an object from the world or moving it to another position
================
*/
void R_RemoveEfrags( cl_entity_t* ent )
{
	efrag_t* ef = ent->efrag;
	while ( ef )
	{
		efrag_t** prev = &ef->leaf->efrags;
		while ( *prev )
		{
			if ( *prev == ef )
			{	// remove this fragment
				*prev = ef->leafnext;
				break;
			}
			prev = &( *prev )->leafnext;
		}

		efrag_t* old = ef;
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
	if ( node->contents == CONTENTS_SOLID )
		return;

// add an efrag if the node is a leaf

	if ( node->contents < 0 )
	{
		if ( !r_pefragtopnode )
			r_pefragtopnode = node;
		efrag_t* ef = cl.free_efrags;
		if ( !ef )
		{
			Con_Printf( "Too many efrags!\n" );
			return;		// no free fragments...
		}

		cl.free_efrags = ef->entnext;
		ef->entity = r_addent;
// add the entity link	
		*lastlink = ef;
		lastlink = &ef->entnext;
		ef->entnext = NULL;
		mleaf_t* leaf = (mleaf_t*)node;
// set the leaf links
		ef->leaf = leaf;
		ef->leafnext = leaf->efrags;
		leaf->efrags = ef;
		return;
	}

	mplane_t* plane = node->plane;
	int sides;
	if ( plane->type > 2 )
		sides = BoxOnPlaneSide( r_emins, r_emaxs, plane );
	else if ( plane->dist <= r_emins[plane->type] )
		sides = 1;
	else if ( !( plane->dist < r_emaxs[plane->type] ) )
		sides = 2;
	else
		sides = 3;
	if ( sides == 3 && !r_pefragtopnode )
		r_pefragtopnode = node;
// recurse down the contacted sides
	if ( sides & 1 )
		R_SplitEntityOnNode( node->children[0] );
	if ( sides & 2 )
		R_SplitEntityOnNode( node->children[1] );
}

/*
===================
R_SplitEntityOnNode2
===================
*/
void R_SplitEntityOnNode2( mnode_t* node )
{
	while ( node->visframe == r_visframecount )
	{
		if ( node->contents < 0 )
		{
			if ( node->contents != CONTENTS_SOLID )
				r_pefragtopnode = node; // we've reached a non-solid leaf, so it's
									//  visible and not BSP clipped
			return;
		}

		mplane_t* plane = node->plane;
		int sides;
		if ( plane->type > 2 )
			sides = BoxOnPlaneSide( r_emins, r_emaxs, plane );
		else if ( plane->dist <= r_emins[plane->type] )
			sides = 1;
		else if ( !( plane->dist < r_emaxs[plane->type] ) )
			sides = 2;
		else
			sides = 3;
		if ( sides == 3 )
		{
			// remember first splitter
			r_pefragtopnode = node;
			return;
		}
		node = node->children[( sides & 1 ) == 0];
	}
}

/*
===========
R_AddEfrags
===========
*/
void R_AddEfrags( cl_entity_t* ent )
{
	model_t* model = ent->model;
	if ( !model || ent == cl_entities )
		return;		// never add the world

	lastlink = &ent->efrag;
	r_addent = ent;
	r_pefragtopnode = NULL;
	VectorAdd( model->mins, ent->origin, r_emins );
	VectorAdd( model->maxs, ent->origin, r_emaxs );
	R_SplitEntityOnNode( cl.worldmodel->nodes );
	ent->topnode = r_pefragtopnode;
}

void R_StoreEfrags( efrag_t** ppefrag )
{
	for ( efrag_t* ef = *ppefrag; ef; ef = ef->leafnext )
	{
		cl_entity_t* ent = ef->entity;
		if ( (unsigned int)ent->model->type > mod_studio )
			Sys_Error( "R_StoreEfrags: Bad entity type %d\n", ent->model->type );
		if ( ent->visframe != r_framecount && ClientDLL_AddEntity( ET_FRAGMENTED, ent ) )
			ent->visframe = r_framecount;
	}
}
