#include "r_local.h"
#include "client.h"

#include <cmath>

int r_dlightframecount;
int r_dlightchanged;
int r_dlightactive;

/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight()
{
	int time = (int)( cl.time * 10.0 );
	for ( int i = 0; i < MAX_LIGHTSTYLES; i++ )
	{
		if ( !cl_lightstyle[i].length )
			d_lightstylevalue[i] = 256;
		else
			d_lightstylevalue[i] = 22 * ( cl_lightstyle[i].map[time % cl_lightstyle[i].length] - 'a' );
	}
}

/*
=============
R_MarkLights
=============
*/
void R_MarkLights( dlight_t* light, int bit, mnode_t* node )
{
	if ( node->contents < 0 )
		return;

	float dist = DotProduct( light->origin, node->plane->normal ) - node->plane->dist;
	if ( dist > light->radius )
	{
		R_MarkLights( light, bit, node->children[0] );
		return;
	}
	if ( dist < -light->radius )
	{
		R_MarkLights( light, bit, node->children[1] );
		return;
	}

	msurface_t* surf = cl.worldmodel->surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ )
	{
		float rad = (float)( light->radius - fabs( (double)dist ) );
		if ( light->minlight > rad )
			continue;

		mtexinfo_t* tex = surf->texinfo;
		int minlight = (int)( rad - light->minlight );
		int s = (int)( DotProduct( light->origin, tex->vecs[0] ) + tex->vecs[0][3] - surf->texturemins[0] );
		int t = (int)( DotProduct( light->origin, tex->vecs[1] ) + tex->vecs[1][3] - surf->texturemins[1] );
		if ( s <= -minlight || t <= -minlight || s > minlight + surf->extents[0] || t > minlight + surf->extents[1] )
			continue;

		if ( surf->dlightframe != r_dlightframecount )
			surf->dlightframe = r_dlightframecount;
		surf->dlightbits |= bit;
	}

	R_MarkLights( light, bit, node->children[0] );
	R_MarkLights( light, bit, node->children[1] );
}

/*
=============
R_PushDlights
=============
*/
void R_PushDlights()
{
	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	for ( int i = 0; i < MAX_DLIGHTS; i++ )
	{
		dlight_t* light = &cl_dlights[i];
		if ( cl.time > light->die || !light->radius )
			continue;
		R_MarkLights( light, (int)( 1u << i ), cl.worldmodel->nodes );
	}
}

colorVec RecursiveLightPoint( mnode_t* node, vec_t* start, vec_t* end )
{
	colorVec color = {};
	if ( node->contents < 0 )
		return color;

	mplane_t* plane = node->plane;
	float front = DotProduct( start, plane->normal ) - plane->dist;
	float back = DotProduct( end, plane->normal ) - plane->dist;
	int side = front < 0;
	if ( ( back < 0 ) == side )
		return RecursiveLightPoint( node->children[side], start, end );

	float frac = front / ( front - back );
	vec3_t mid;
	for ( int i = 0; i < 3; i++ )
		mid[i] = start[i] + ( end[i] - start[i] ) * frac;
	color = RecursiveLightPoint( node->children[side], start, mid );
	if ( color.r || color.g || color.b )
		return color;

	msurface_t* surf = cl.worldmodel->surfaces + node->firstsurface;
	for ( int i = 0; i < node->numsurfaces; i++, surf++ )
	{
		if ( surf->flags & SURF_DRAWTILED )
			continue;	// no lightmaps

		mtexinfo_t* tex = surf->texinfo;
		int s = (int)( DotProduct( mid, tex->vecs[0] ) + tex->vecs[0][3] );
		if ( s < surf->texturemins[0] )
			continue;
		int t = (int)( DotProduct( mid, tex->vecs[1] ) + tex->vecs[1][3] );
		if ( t < surf->texturemins[1] )
			continue;
		int ds = s - surf->texturemins[0];
		int dt = t - surf->texturemins[1];
		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;
		if ( !surf->samples )
			return color;

		int width = ( surf->extents[0] >> 4 ) + 1;
		int size = width * ( ( surf->extents[1] >> 4 ) + 1 );
		color24* lightmap = surf->samples + ( ds >> 4 ) + ( dt >> 4 ) * width;
		for ( int maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++, lightmap += size )
		{
			unsigned int scale = d_lightstylevalue[surf->styles[maps]];
			color.r += scale * lightmap->r;
			color.g += scale * lightmap->g;
			color.b += scale * lightmap->b;
		}
		color.r >>= 8;
		color.g >>= 8;
		color.b >>= 8;
		if ( !color.r )
			color.r = 1;
		return color;
	}

// go down back side
	return RecursiveLightPoint( node->children[front >= 0], mid, end );
}

colorVec R_LightVec( vec_t* start, vec_t* end )
{
	colorVec color = {};
	if ( !cl.worldmodel->lightdata )
	{
		color.r = color.g = color.b = 255;
		return color;
	}

	color = RecursiveLightPoint( cl.worldmodel->nodes, start, end );
	color.r += r_refdef.ambientlight.r;
	color.g += r_refdef.ambientlight.g;
	color.b += r_refdef.ambientlight.b;
	if ( color.r > 255 )
		color.r = 255;
	if ( color.g > 255 )
		color.g = 255;
	if ( color.b > 255 )
		color.b = 255;
	return color;
}

colorVec R_LightPoint( vec_t* p )
{
	vec3_t end;
	end[0] = p[0];
	end[1] = p[1];
	end[2] = p[2] - 2048.0f;
	return R_LightVec( p, end );
}
