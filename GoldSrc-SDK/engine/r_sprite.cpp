#include "r_local.h"
#include "client.h"
#include "cl_draw.h"
#include "common.h"
#include "sys.h"

spritedesc_t r_spritedesc;

static int clip_current;
static vec_t clip_verts[2][20][5];
static int sprite_width;
static int sprite_height;

int R_ClipSpriteFace( int nump, clipplane_t* pclipplane )
{
	float ( *src )[5] = clip_verts[clip_current ? 1 : 0];
	clip_current = !clip_current;
	float ( *dest )[5] = clip_verts[clip_current];
	float dists[21];

	if ( nump <= 0 )
	{
		Q_memcpy( src[0], src[0], sizeof( src[0] ) );
		return 0;
	}

	for ( int i = 0; i < nump; i++ )
	{
		dists[i] = DotProduct( src[i], pclipplane->normal ) - pclipplane->dist;
	}
// handle wraparound case
	dists[nump] = dists[0];
	Q_memcpy( src[nump], src[0], sizeof( src[0] ) );

	int outcount = 0;
	for ( int i = 0; i < nump; i++ )
	{
		float dist = dists[i];
		if ( dist >= 0 )
		{
			Q_memcpy( dest[outcount], src[i], sizeof( src[i] ) );
			outcount++;
		}

		if ( dist == 0 || dists[i + 1] == 0 )
			continue;
		if ( ( dist > 0 ) == ( dists[i + 1] > 0 ) )
			continue;

		float fraction = dist / ( dist - dists[i + 1] );
		for ( int j = 0; j < 5; j++ )
			dest[outcount][j] = ( src[i + 1][j] - src[i][j] ) * fraction + src[i][j];
		outcount++;
	}
	return outcount;
}

/*
================
R_SetupAndDrawSprite
================
*/
void R_SetupAndDrawSprite()
{
	if ( !( DotProduct( r_spritedesc.vpn, modelorg ) < 0.0f ) )
		return;

	vec3_t left, right, up, down;
	float scale = r_spritedesc.scale * r_spritedesc.pspriteframe->right;
	VectorScale( r_spritedesc.vright, scale, right );
	scale = r_spritedesc.scale * r_spritedesc.pspriteframe->up;
	VectorScale( r_spritedesc.vup, scale, up );
	scale = r_spritedesc.scale * r_spritedesc.pspriteframe->left;
	VectorScale( r_spritedesc.vright, scale, left );
	scale = r_spritedesc.scale * r_spritedesc.pspriteframe->down;
	VectorScale( r_spritedesc.vup, scale, down );

	for ( int i = 0; i < 3; i++ )
	{
		clip_verts[0][0][i] = up[i] + r_entorigin[i] + left[i];
		clip_verts[0][1][i] = up[i] + r_entorigin[i] + right[i];
		clip_verts[0][2][i] = down[i] + r_entorigin[i] + right[i];
		clip_verts[0][3][i] = down[i] + r_entorigin[i] + left[i];
	}
	clip_verts[0][0][3] = 0;
	clip_verts[0][0][4] = 0;
	clip_verts[0][1][3] = (float)sprite_width;
	clip_verts[0][1][4] = 0;
	clip_verts[0][2][3] = (float)sprite_width;
	clip_verts[0][2][4] = (float)sprite_height;
	clip_verts[0][3][3] = 0;
	clip_verts[0][3][4] = (float)sprite_height;
	clip_current = 0;

	int nump = 4;
	for ( int i = 0; i < 4; i++ )
	{
		nump = R_ClipSpriteFace( nump, &view_clipplanes[i] );
		if ( nump <= 2 )
			return;
		if ( nump > 19 )
			Sys_Error( "R_SetupAndDrawSprite: too many points" );
	}

	emitpoint_t outverts[21];
	r_spritedesc.nearzi = -999999.0f;
	for ( int i = 0; i < nump; i++ )
	{
		vec3_t local, transformed;
		VectorSubtract( clip_verts[clip_current][i], r_origin, local );
		TransformVector( local, transformed );
		if ( transformed[2] < 1 )
			return;

		float zi = (float)( 1.0 / transformed[2] );
		outverts[i].zi = zi;
		if ( zi > r_spritedesc.nearzi )
			r_spritedesc.nearzi = zi;
		outverts[i].s = clip_verts[clip_current][i][3];
		outverts[i].t = clip_verts[clip_current][i][4];
		outverts[i].u = zi * xscale * transformed[0] + xcenter;
		if ( outverts[i].u < 0.0f )
			outverts[i].u = 0;
		outverts[i].v = ycenter - zi * yscale * transformed[1];
		if ( outverts[i].v < 0.0f )
			outverts[i].v = 0;
	}
// draw it
	r_spritedesc.nump = nump;
	r_spritedesc.pverts = outverts;
	D_DrawSprite();
}

/*
================
R_DrawSprite
================
*/
void R_DrawSprite()
{
	msprite_t* sprite = (msprite_t*)currententity->model->cache.data;
	r_spritedesc.pspriteframe = R_GetSpriteFrame( sprite, (int)currententity->curstate.frame );
	r_spritedesc.scale = currententity->curstate.scale > 0 ? currententity->curstate.scale : 1;

	colorVec color;
	word palette[256];
	R_SpriteColor( &color, currententity, r_blend );
	UnpackPalette( palette, (word*)( (byte*)sprite + sprite->paloffset ), color.r, color.g, color.b );
	r_palette = palette;
	sprite_width = r_spritedesc.pspriteframe->width;
	sprite_height = r_spritedesc.pspriteframe->height;
	R_GetSpriteAxes( currententity, sprite->type, r_spritedesc.vpn, r_spritedesc.vright, r_spritedesc.vup );
	R_SetupAndDrawSprite();
}
