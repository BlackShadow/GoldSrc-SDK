#include "d_local.h"
#include "r_local.h"
#include "conprint.h"
#include "sys.h"

#include <stddef.h>
#include <stdio.h>

float surfscale;
qboolean r_cache_thrash; // set if surface cache is thrashing
int sc_size;
surfcache_t* sc_base;
surfcache_t* sc_rover;
qboolean is15bit;

void D_CheckCacheGuard()
{
	byte* s = (byte*)sc_base + sc_size;

	for ( int i = 0; i < 4; i++ )
	{
		if ( s[i] != i )
			Sys_Error( "D_CheckCacheGuard: failed" );
	}
}

void D_ClearCacheGuard()
{
	byte* s = (byte*)sc_base + sc_size;

	for ( int i = 0; i < 4; i++ )
		s[i] = (byte)i;
}

/*
================
D_InitCaches

================
*/
void D_InitCaches()
{
	Con_Printf( "%ik surface cache\n", vid_surfcachesize / 1024 );

	sc_size = vid_surfcachesize - 4;
	sc_base = (surfcache_t*)vid_surfcache;
	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;

	D_ClearCacheGuard();
}

/*
==================
D_FlushCaches
==================
*/
void D_FlushCaches()
{
	if ( !sc_base )
		return;

	for ( surfcache_t* c = sc_base; c; c = c->next )
	{
		if ( c->owner )
			*c->owner = NULL;
	}

	sc_rover = sc_base;
	sc_base->next = NULL;
	sc_base->owner = NULL;
	sc_base->size = sc_size;
	gWaterLastPalette = NULL;
}

/*
=================
D_SCAlloc
=================
*/
surfcache_t* D_SCAlloc( int width, int size )
{
	if ( (unsigned int)width > 256 )
		Sys_Error( "D_SCAlloc: bad cache width %d\n", width );
	if ( (unsigned int)size - 1 > 0x1FFFF )
		Sys_Error( "D_SCAlloc: bad cache size %d\n", size );

	size = ( (int)offsetof( surfcache_t, data ) + size + 3 ) & ~3;
	if ( size > sc_size )
		Sys_Error( "D_SCAlloc: %i > cache size", size );

	qboolean wrapped_this_time = FALSE;
	if ( sc_rover && (byte*)sc_rover - (byte*)sc_base > sc_size - size )
	{
		wrapped_this_time = TRUE;
		sc_rover = sc_base;
	}
	if ( !sc_rover )
		sc_rover = sc_base;

	surfcache_t* new_surf = sc_rover;
	if ( new_surf->owner )
		*new_surf->owner = NULL;

	while ( new_surf->size < size )
	{
		// free another
		sc_rover = sc_rover->next;
		if ( !sc_rover )
			Sys_Error( "D_SCAlloc: hit the end of memory" );
		if ( sc_rover->owner )
			*sc_rover->owner = NULL;

		new_surf->size += sc_rover->size;
		new_surf->next = sc_rover->next;
	}

// create a fragment out of any leftovers
	if ( new_surf->size - size > 256 )
	{
		sc_rover = (surfcache_t*)( (byte*)new_surf + size );
		sc_rover->size = new_surf->size - size;
		sc_rover->next = new_surf->next;
		sc_rover->width = 0;
		sc_rover->owner = NULL;
		new_surf->next = sc_rover;
		new_surf->size = size;
	}
	else
	{
		sc_rover = new_surf->next;
	}

	new_surf->width = width;
	if ( width > 0 )
		new_surf->height = ( size - offsetof( surfcache_t, data ) ) / (unsigned int)width;
	new_surf->owner = NULL; // should be set properly after return

	if ( d_roverwrapped )
	{
		if ( wrapped_this_time || sc_rover >= d_initial_rover )
			r_cache_thrash = TRUE;
	}
	else if ( wrapped_this_time )
	{
		d_roverwrapped = TRUE;
	}

	D_CheckCacheGuard(); // DEBUG
	return new_surf;
}

//=============================================================================

// if the num is not a power of 2, assume it will not repeat

int MaskForNum( int num )
{
	switch ( num )
	{
	case 128:
		return 127;
	case 64:
		return 63;
	case 32:
		return 31;
	case 16:
		return 15;
	default:
		return 255;
	}
}

int D_log2( int num )
{
	int count = 0;

	while ( num >>= 1 )
		count++;

	return count;
}

/*
================
D_CacheSurface
================
*/
//=============================================================================
surfcache_t* D_CacheSurface( msurface_t* surface, int miplevel )
{
//
// if the surface is animating or flashing, flush the cache
//
	r_drawsurf.texture = R_TextureAnimation( surface->texinfo->texture );
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
		r_drawsurf.lightadj[i] = d_lightstylevalue[surface->styles[i]];

	surfcache_t* cache = surface->cachespots[miplevel];
	if ( cache )
	{
		if ( cache->dlight >= surface->dlightframe - 1 )
			cache->dlight = surface->dlightframe;

		if ( cache->dlight == surface->dlightframe && !( surface->dlightbits & r_dlightchanged ) &&
				cache->texture == r_drawsurf.texture &&
				cache->lightadj[0] == r_drawsurf.lightadj[0] &&
				cache->lightadj[1] == r_drawsurf.lightadj[1] &&
				cache->lightadj[2] == r_drawsurf.lightadj[2] &&
				cache->lightadj[3] == r_drawsurf.lightadj[3] )
			return cache;
	}

	r_drawsurf.surfmip = miplevel;
//
// determine shape of surface
//
	surfscale = 1.0f / ( 1 << miplevel );
	r_drawsurf.surfwidth = surface->extents[0] >> miplevel;
	r_drawsurf.rowbytes = 2 * r_drawsurf.surfwidth;
	r_drawsurf.surfheight = surface->extents[1] >> miplevel;

//
// allocate memory if needed
//
	if ( !cache ) // if a texture just animated, don't reallocate it
	{
		cache = D_SCAlloc( r_drawsurf.surfwidth, r_drawsurf.rowbytes * r_drawsurf.surfheight );
		surface->cachespots[miplevel] = cache;
		cache->owner = &surface->cachespots[miplevel];
		cache->mipscale = surfscale;
	}

	surface->dlightbits &= r_dlightactive;
	int dlightframe = surface->dlightframe;
	c_surf++;
	cache->dlight = dlightframe;
	r_drawsurf.surfdat = cache->data;
	cache->texture = r_drawsurf.texture;
	r_drawsurf.surf = surface;
	for ( int i = 0; i < MAXLIGHTMAPS; i++ )
		cache->lightadj[i] = r_drawsurf.lightadj[i];

	R_DrawSurface();
	return surface->cachespots[miplevel];
}
