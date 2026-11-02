#include "d_local.h"
#include "r_local.h"
#include "engine_color.h"
#include "client.h"
#include "enginecallbacks.h"

static int miplevel;
int ubasestep;
int errorterm;
int erroradjustup;
int erroradjustdown;
float scale_for_mip;
vec3_t transformed_modelorg;

/*
==============
D_DrawPoly

==============
*/
void D_DrawPoly()
{
// this driver takes spans, not polygons
}

/*
=============
D_MipLevelForScale
=============
*/
int D_MipLevelForScale( float scale )
{
	int lmiplevel;

	if ( scale >= d_scalemip[0] )
		lmiplevel = 0;
	else if ( scale >= d_scalemip[1] )
		lmiplevel = 1;
	else if ( scale >= d_scalemip[2] )
		lmiplevel = 2;
	else
		lmiplevel = 3;

	if ( lmiplevel < d_minmip )
		lmiplevel = d_minmip;
	return lmiplevel;
}

/*
==============
D_DrawSolidSurface
==============
*/
void D_DrawSolidSurface( surf_t* surf, int color )
{
	for ( espan_t* span = surf->spans; span; span = span->pnext )
	{
		unsigned short* pdest = (unsigned short*)( d_viewbuffer + screenwidth * span->v );
		int u2 = span->u + span->count;

		for ( int u = span->u; u < u2; u++ )
			pdest[u] = (unsigned short)color;
	}
}

/*
==============
D_CalcGradients
==============
*/
void D_CalcGradients( msurface_t* pface )
{
#if defined( _WIN32 )
	float mipscale = (float)( 1.0 / (float)( 1 << miplevel ) );
	vec3_t p_temp1, p_saxis, p_taxis;

	TransformVector( pface->texinfo->vecs[0], p_saxis );
	TransformVector( pface->texinfo->vecs[1], p_taxis );

	float t = xscaleinv * mipscale;
	d_sdivzstepu = p_saxis[0] * t;
	d_tdivzstepu = p_taxis[0] * t;

	t = yscaleinv * mipscale;
	d_sdivzstepv = -p_saxis[1] * t;
	d_tdivzstepv = -p_taxis[1] * t;

	d_sdivzorigin = p_saxis[2] * mipscale - xcenter * d_sdivzstepu - ycenter * d_sdivzstepv;
	d_tdivzorigin = p_taxis[2] * mipscale - xcenter * d_tdivzstepu - ycenter * d_tdivzstepv;

	VectorScale( transformed_modelorg, mipscale, p_temp1 );

	t = 0x10000 * mipscale;
	sadjust = (int)( (float)( (int)( DotProduct( p_temp1, p_saxis ) * 0x10000 + 0.5 )
		- ( ( pface->texturemins[0] << 16 ) >> miplevel ) ) + pface->texinfo->vecs[0][3] * t );
	tadjust = (int)( (float)( (int)( DotProduct( p_temp1, p_taxis ) * 0x10000 + 0.5 )
		- ( ( pface->texturemins[1] << 16 ) >> miplevel ) ) + pface->texinfo->vecs[1][3] * t );
#else
	float mipscale = 1.0f / ( 1 << miplevel );
	vec3_t p_temp1, p_saxis, p_taxis;

	TransformVector( pface->texinfo->vecs[0], p_saxis );
	TransformVector( pface->texinfo->vecs[1], p_taxis );

	double sstepu = p_saxis[0] * ( (double)xscaleinv * mipscale );
	double tstepu = p_taxis[0] * ( (double)xscaleinv * mipscale );
	double sstepv = -p_saxis[1] * ( (double)yscaleinv * mipscale );
	double tstepv = -p_taxis[1] * ( (double)yscaleinv * mipscale );
	d_sdivzstepu = (float)sstepu;
	d_tdivzstepu = (float)tstepu;
	d_sdivzstepv = (float)sstepv;
	d_tdivzstepv = (float)tstepv;
	d_sdivzorigin = (float)( p_saxis[2] * (double)mipscale - sstepu * xcenter - sstepv * ycenter );
	d_tdivzorigin = (float)( p_taxis[2] * (double)mipscale - tstepu * xcenter - tstepv * ycenter );

	VectorScale( transformed_modelorg, mipscale, p_temp1 );
	double s = (double)p_temp1[0] * p_saxis[0] + (double)p_temp1[1] * p_saxis[1]
		+ (double)p_temp1[2] * p_saxis[2];
	double t = (double)p_temp1[0] * p_taxis[0] + (double)p_temp1[1] * p_taxis[1]
		+ (double)p_temp1[2] * p_taxis[2];
	float sadjustment = (float)( (int)( s * 65536.0 + 0.5 )
		- ( ( pface->texturemins[0] << 16 ) >> miplevel )
		+ pface->texinfo->vecs[0][3] * ( mipscale * 65536.0 ) );
	float tadjustment = (float)( (int)( t * 65536.0 + 0.5 )
		- ( ( pface->texturemins[1] << 16 ) >> miplevel )
		+ pface->texinfo->vecs[1][3] * ( mipscale * 65536.0 ) );
	sadjust = (int)sadjustment;
	tadjust = (int)tadjustment;
#endif
//
// -1 (-epsilon) so we never wander off the edge of the texture
//
	bbextents = ( ( pface->extents[0] << 16 ) >> miplevel ) - 1;
	bbextentt = ( ( pface->extents[1] << 16 ) >> miplevel ) - 1;
}

int LeafId( msurface_t* psurface )
{
	for ( int i = 0; i < cl.worldmodel->numleafs; i++ )
	{
		mleaf_t* leaf = &cl.worldmodel->leafs[i];
		for ( int j = 0; j < leaf->nummarksurfaces; j++ )
		{
			if ( leaf->firstmarksurface[j] == psurface )
				return i;
		}
	}

	int surfIndex = (int)( ( psurface - cl.worldmodel->surfaces ) / sizeof( msurface_t ) );
	for ( int i = 0; i < cl.worldmodel->numnodes; i++ )
	{
		mnode_t* node = &cl.worldmodel->nodes[i];
		if ( surfIndex >= node->firstsurface && surfIndex < node->firstsurface + node->numsurfaces )
			return cl.worldmodel->numleafs + i;
	}
	return 1;
}

/*
==============
D_DrawSurfaces
==============
*/
void D_DrawSurfaces()
{
	vec3_t world_transformed_modelorg;
	vec3_t local_modelorg;
	static word flatcolors[256 * 4];

	currententity = cl_entities;
	TransformVector( modelorg, transformed_modelorg );

// TODO: could preset a lot of this at mode set time
	if ( r_drawflat.value )
	{
		if ( !flatcolors[1] )
		{
			for ( int i = 0; i < 256; i++ )
			{
				do
				{
					flatcolors[i * 4] = (word)RandomLong( 0, 255 );
					flatcolors[i * 4 + 1] = (word)RandomLong( 0, 255 );
					flatcolors[i * 4 + 2] = (word)RandomLong( 0, 255 );
				}
				while ( flatcolors[i * 4] + flatcolors[i * 4 + 1] + flatcolors[i * 4 + 2] <= 255 );

				flatcolors[0] = 0;
				flatcolors[1] = 255;
				flatcolors[2] = 255;
				flatcolors[3] = 0;
				flatcolors[4] = 255;
				flatcolors[5] = 255;
				flatcolors[6] = 255;
				flatcolors[7] = 0;
			}
		}

		for ( surf_t* s = surfaces + 1; s < surface_p; s++ )
		{
			if ( !s->spans || ( r_intentities && ( s->flags & SURF_DRAWBACKGROUND ) ) )
				continue;

			d_zistepu = s->d_zistepu;
			d_zistepv = s->d_zistepv;
			d_ziorigin = s->d_ziorigin;

			if ( s->flags & SURF_DRAWBACKGROUND )
			{
				D_DrawSolidSurface( s, 0 );
			}
			else if ( r_drawflat.value == 2.0f )
			{
				int id = 0;
				if ( !s->insubmodel )
				{
					id = LeafId( (msurface_t*)s->data );
					if ( id > 2 )
					{
						id %= 255;
						if ( id <= 1 )
							id += 2;
					}
				}
				D_DrawSolidSurface( s, hlRGB( flatcolors, id ) );
			}
			else
			{
				D_DrawSolidSurface( s, hlRGB( flatcolors, ( (int)s->data >> 4 ) % 255 ) );
			}
			D_DrawZSpans( s->spans );
		}
		return;
	}

	VectorCopy( transformed_modelorg, world_transformed_modelorg );
	for ( surf_t* s = surfaces + 1; s < surface_p; s++ )
	{
		if ( !s->spans )
			continue;

		r_drawnpolycount++;
		d_zistepu = s->d_zistepu;
		d_zistepv = s->d_zistepv;
		d_ziorigin = s->d_ziorigin;

		if ( s->flags & SURF_DRAWSKY )
		{
			currententity = s->entity;
			if ( currententity->index <= 0 )
			{
				// R_RotateBmodel ()
				VectorSubtract( r_origin, currententity->origin, local_modelorg );
				TransformVector( local_modelorg, transformed_modelorg );
				msurface_t* pface = (msurface_t*)s->data;
				cacheblock = (pixel_t*)pface->samples;
				cachewidth = pface->extents[0];
				miplevel = 0;
				D_CalcGradients( pface );
				d_drawspans( s->spans );
				d_zistepu = 0.0f;
				d_zistepv = 0.0f;
				d_ziorigin = -0.9f;
				D_DrawZSpans( s->spans );
			}
			continue;
		}

		if ( s->flags & SURF_DRAWBACKGROUND )
		{
			if ( !r_intentities )
			{
				d_zistepu = 0.0f;
				d_zistepv = 0.0f;
				d_ziorigin = -0.9f;
				D_DrawSolidSurface( s, 0 );
				D_DrawZSpans( s->spans );
			}
			continue;
		}

		msurface_t* pface = (msurface_t*)s->data;
		if ( s->insubmodel )
		{
			currententity = s->entity;
			VectorSubtract( r_origin, currententity->origin, local_modelorg );
			TransformVector( local_modelorg, transformed_modelorg );
			R_RotateBmodel(); // FIXME: don't mess with the frustum,
										// make entity passed in
		}

		if ( s->flags & SURF_DRAWTILED )
		{
			if ( s->flags & SURF_DRAWTURB )
			{
				miplevel = 0;
				texture_t* texture = pface->texinfo->texture;
				cacheblock = (pixel_t*)texture + texture->offsets[0];
				r_palette = (word*)( (byte*)texture + texture->paloffset );
				D_CalcGradients( pface );
				WaterTextureUpdate( r_palette, 0.1f, texture );
			}
			else
			{
#if defined( _WIN32 )
				float scale = s->nearzi * scale_for_mip * pface->texinfo->mipadjust;
#else
				double scale = (double)s->nearzi * scale_for_mip * pface->texinfo->mipadjust;
#endif
				if ( scale >= d_scalemip[0] )
					miplevel = 0;
				else if ( scale >= d_scalemip[1] )
					miplevel = 1;
				else if ( scale >= d_scalemip[2] )
					miplevel = 2;
				else
					miplevel = 3;
				if ( miplevel < d_minmip )
					miplevel = d_minmip;
				surfcache_t* pcurrentcache = D_CacheSurface( pface, miplevel );
				cacheblock = pcurrentcache->data;
				cachewidth = pcurrentcache->width;
				D_CalcGradients( pface );
				texture_t* texture = pface->texinfo->texture;
				int width = texture->width >> miplevel;
				int height = texture->height >> miplevel;
				TilingSetup( width - 1, height - 1, Q_log2( width ) );
				float speed = ( ( currententity->curstate.rendercolor.g << 8 )
					+ currententity->curstate.rendercolor.b ) * 0.0625f;
				if ( !currententity->curstate.rendercolor.r )
					speed = -speed;
#ifdef _MSC_VER
#pragma warning( suppress: 6297 )
#endif
				sadjust = (int)( speed * cl.time * 65536.0 * ( 1.0 / ( 1 << miplevel ) ) + sadjust );
			}

			if ( r_intentities )
			{
				D_DrawTiled8Trans( s->spans );
			}
			else
			{
				D_DrawTiled8( s->spans );
				D_DrawZSpans( s->spans );
			}

			if ( s->insubmodel )
			{
				VectorCopy( world_transformed_modelorg, transformed_modelorg );
				VectorCopy( base_vpn, vpn );
				VectorCopy( base_vup, vup );
				VectorCopy( base_vright, vright );
				currententity = cl_entities;
				VectorCopy( base_modelorg, modelorg );
				R_TransformFrustum();
			}
		}
		else
		{
#if defined( _WIN32 )
			float scale = s->nearzi * scale_for_mip * pface->texinfo->mipadjust;
#else
			double scale = (double)s->nearzi * scale_for_mip * pface->texinfo->mipadjust;
#endif
			if ( scale >= d_scalemip[0] )
				miplevel = 0;
			else if ( scale >= d_scalemip[1] )
				miplevel = 1;
			else if ( scale >= d_scalemip[2] )
				miplevel = 2;
			else
				miplevel = 3;
			if ( miplevel < d_minmip )
				miplevel = d_minmip;
			surfcache_t* pcurrentcache = D_CacheSurface( pface, miplevel );
			cacheblock = pcurrentcache->data;
			cachewidth = pcurrentcache->width;
			D_CalcGradients( pface );
			d_drawspans( s->spans );
			if ( !r_intentities )
				D_DrawZSpans( s->spans );

			if ( s->insubmodel )
			{
				VectorCopy( world_transformed_modelorg, transformed_modelorg );
				VectorCopy( base_vpn, vpn );
				VectorCopy( base_vup, vup );
				VectorCopy( base_vright, vright );
				VectorCopy( base_modelorg, modelorg );
				R_TransformFrustum();
				currententity = cl_entities;
			}
		}
	}
}
