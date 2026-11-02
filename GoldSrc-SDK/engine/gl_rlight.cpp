#include "glquake.h"
#include "client.h"
#include "cvar.h"
#include "gl_model.h"
#include "mathlib.h"
#include "render.h"

#include <cmath>

//-----------------------------------------------------------------------------
// Globals associated with dynamic lighting
//-----------------------------------------------------------------------------
int r_dlightframecount;
int r_dlightchanged;
int r_dlightactive;

extern int d_lightstylevalue[];
extern int r_framecount;
extern vec3_t r_origin, vpn, vright, vup;
extern float v_blend[4];
extern cvar_t gl_flashblend;

//-----------------------------------------------------------------------------
// Displacements to test against for R_LightVec
//-----------------------------------------------------------------------------
/*
==================
R_AnimateLight
==================
*/
void R_AnimateLight()
{
	int i, j, k;

	//
	// light animations
	// 'm' is normal light, 'a' is no light, 'z' is double bright
	i = (int)( cl.time * 10.0 );
	for ( j = 0; j < MAX_LIGHTSTYLES; j++ )
	{
		if ( !cl_lightstyle[j].length )
		{
			d_lightstylevalue[j] = 256;
			continue;
		}
		k = i % cl_lightstyle[j].length;
		k = cl_lightstyle[j].map[k] - 'a';
		d_lightstylevalue[j] = k * 22;
	}
}

/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend( float r, float g, float b, float a2 )
{
	float a, inv;

	v_blend[3] = a = (float)( 1.0 - v_blend[3] ) * a2 + v_blend[3];

	a2 = a2 / a;
	inv = (float)( 1.0 - a2 );

	v_blend[0] = v_blend[1] * inv + r * a2;
	v_blend[1] = v_blend[1] * inv + g * a2;
	v_blend[2] = v_blend[2] * inv + b * a2;
}

void R_RenderDlight( dlight_t* light )
{
	int i, j;
	float a;
	double c, s;
	vec3_t v;
	float rad;

	rad = (float)( light->radius * 0.35 );

	VectorSubtract( light->origin, r_origin, v );
	if ( !( rad <= Length( v ) ) )
	{
		// view is inside the dlight
		AddLightBlend( 1, 0.5f, 0, (float)( light->radius * 0.0003 ) );
		return;
	}

	qglBegin( GL_TRIANGLE_FAN );
	qglColor3f( 0.2f, 0.1f, 0 );
	for ( i = 0; i < 3; i++ )
		v[i] = light->origin[i] - vpn[i] * rad;
	qglVertex3fv( v );
	qglColor3f( 0, 0, 0 );
	for ( i = 16; i >= 0; i-- )
	{
		a = (float)( i / 16.0 * M_PI * 2 );
		c = cos( (double)a );
		s = sin( (double)a );
		for ( j = 0; j < 3; j++ )
			v[j] = (float)( light->origin[j] + vright[j] * c * rad + vup[j] * s * rad );
		qglVertex3fv( v );
	}
	qglEnd();
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights()
{
	int i;
	dlight_t* l;

	if ( !gl_flashblend.value )
		return;

	r_dlightframecount = r_framecount + 1;	// because the count hasn't
											//  advanced yet for this frame
	qglDepthMask( GL_FALSE );
	qglDisable( GL_TEXTURE_2D );
	qglShadeModel( GL_SMOOTH );
	qglEnable( GL_BLEND );
	qglBlendFunc( GL_ONE, GL_ONE );

	l = cl_dlights;
	for ( i = 0; i < MAX_DLIGHTS; i++, l++ )
	{
		if ( !( cl.time <= l->die ) || !l->radius )
			continue;
		R_RenderDlight( l );
	}

	qglColor3f( 1, 1, 1 );
	qglDisable( GL_BLEND );
	qglEnable( GL_TEXTURE_2D );
	qglBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	qglDepthMask( GL_TRUE );
}

/*
=============================================================================

DYNAMIC LIGHTS

=============================================================================
*/

/*
=============
R_MarkLights
=============
*/
void R_MarkLights( dlight_t* light, int bit, mnode_t* node )
{
	mplane_t* splitplane;
	float dist;
	msurface_t* surf;
	int i;

	if ( node->contents < 0 )
		return;

	splitplane = node->plane;
	dist = DotProduct( light->origin, splitplane->normal ) - splitplane->dist;

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

	// mark the polygons
	surf = cl.worldmodel->surfaces + node->firstsurface;
	for ( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		float rad;
		int s, t, minlight;
		mtexinfo_t* tex;

		rad = (float)( light->radius - fabs( (double)dist ) );
		if ( !( light->minlight <= rad ) )
			continue;

		tex = surf->texinfo;

		minlight = (int)( rad - light->minlight );

		// Project light center into texture coordinates
		s = (int)( DotProduct( light->origin, tex->vecs[0] ) + tex->vecs[0][3] - surf->texturemins[0] );
		t = (int)( DotProduct( light->origin, tex->vecs[1] ) + tex->vecs[1][3] - surf->texturemins[1] );

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
	int i;
	dlight_t* l;

	if ( gl_flashblend.value )
		return;

	r_dlightframecount = r_framecount + 1;

	//  advanced yet for this frame
	l = cl_dlights;
	for ( i = 0; i < MAX_DLIGHTS; i++, l++ )
	{
		if ( !( cl.time <= l->die ) || !l->radius )
			continue;
		R_MarkLights( l, (int)( 1u << i ), cl.worldmodel->nodes );
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

mplane_t* lightplane;
vec3_t lightspot;

colorVec RecursiveLightPoint( mnode_t* node, vec_t* start, vec_t* end )
{
	colorVec c = {};
	float front, back, frac;
	int side;
	mplane_t* plane;
	vec3_t mid;
	msurface_t* surf;
	int s, t, ds, dt;
	int i;
	mtexinfo_t* tex;
	color24* lightmap;
	unsigned scale;
	int maps, size;

	if ( node->contents < 0 )
		return c; // didn't hit anything

	// calculate mid point
	plane = node->plane;
	front = DotProduct( start, plane->normal ) - plane->dist;
	back = DotProduct( end, plane->normal ) - plane->dist;
	side = front < 0;

	if ( ( back < 0 ) == side )
		return RecursiveLightPoint( node->children[side], start, end );

	frac = front / ( front - back );
	for ( i = 0; i < 3; i++ )
		mid[i] = start[i] + ( end[i] - start[i] ) * frac;

	// go down front side
	c = RecursiveLightPoint( node->children[side], start, mid );
	if ( c.r || c.g || c.b )
		return c; // hit something

	// check for impact on this node
	VectorCopy( mid, lightspot );
	lightplane = plane;

	surf = cl.worldmodel->surfaces + node->firstsurface;
	for ( i = 0; i < node->numsurfaces; i++, surf++ )
	{
		if ( surf->flags & SURF_DRAWTILED )
			continue; // no lightmaps

		tex = surf->texinfo;

		s = (int)( DotProduct( mid, tex->vecs[0] ) + tex->vecs[0][3] );
		if ( s < surf->texturemins[0] )
			continue;

		t = (int)( DotProduct( mid, tex->vecs[1] ) + tex->vecs[1][3] );
		if ( t < surf->texturemins[1] )
			continue;

		ds = s - surf->texturemins[0];
		dt = t - surf->texturemins[1];

		if ( ds > surf->extents[0] || dt > surf->extents[1] )
			continue;

		if ( !surf->samples )
			return c;

		ds >>= 4;
		dt >>= 4;

		size = ( ( surf->extents[0] >> 4 ) + 1 ) * ( ( surf->extents[1] >> 4 ) + 1 );
		lightmap = surf->samples + ds + dt * ( ( surf->extents[0] >> 4 ) + 1 );
		for ( maps = 0; maps < MAXLIGHTMAPS && surf->styles[maps] != 255; maps++ )
		{
			scale = d_lightstylevalue[surf->styles[maps]];
			c.r += scale * lightmap->r;
			c.g += scale * lightmap->g;
			c.b += scale * lightmap->b;
			lightmap += size; // skip to next lightmap
		}

		c.r >>= 8;
		c.g >>= 8;
		c.b >>= 8;

		if ( !c.r )
			c.r = 1;

		return c;
	}

	return RecursiveLightPoint( node->children[front >= 0], mid, end );
}

colorVec R_LightVec( vec_t* start, vec_t* end )
{
	colorVec c = {};

	if ( !cl.worldmodel->lightdata )
	{
		c.r = c.g = c.b = 255;
		return c;
	}

	c = RecursiveLightPoint( cl.worldmodel->nodes, start, end );

	c.r += r_refdef.ambientlight.r;
	c.g += r_refdef.ambientlight.g;
	c.b += r_refdef.ambientlight.b;

	if ( c.r > 255 )
		c.r = 255;
	if ( c.g > 255 )
		c.g = 255;
	if ( c.b > 255 )
		c.b = 255;

	return c;
}

colorVec R_LightPoint( vec_t* p )
{
	vec3_t end;

	end[0] = p[0];
	end[1] = p[1];
	end[2] = (float)( p[2] - 2048.0 );

	return R_LightVec( p, end );
}
