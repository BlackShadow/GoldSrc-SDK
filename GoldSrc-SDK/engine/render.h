// refresh.h -- public interface to refresh functions
#pragma once

#include "const.h"
#include "mathlib.h"
#include "../common/const.h"

#define NUMVERTEXNORMALS 162

#define STUDIO_RENDER 1
#define STUDIO_EVENTS 2

typedef struct vrect_s
{
	int x, y, width, height;
	vrect_s* pnext;
//  that splits bmodel, or NULL if
//  not split
} vrect_t;

typedef struct
{
	vrect_t vrect;		// subwindow in video for refresh
	// FIXME: not need vrect next field here?
	vrect_t aliasvrect; // scaled Alias version
	int vrectright, vrectbottom; // right & bottom screen coords
	int aliasvrectright, aliasvrectbottom; // scaled Alias versions
	float vrectrightedge; // rightmost right edge we care about,
	//  for use in edge list
	float fvrectx, fvrecty; // for floating-point compares
	float fvrectx_adj, fvrecty_adj; // left and top edges, for clamping
	int vrect_x_adj_shift20; // (vrect.x + 0.5 - epsilon) << 20
	int vrectright_adj_shift20; // (vrectright + 0.5 - epsilon) << 20
	float fvrectright_adj, fvrectbottom_adj; // right and bottom edges, for clamping
	float fvrectright; // rightmost edge, for Alias clamping
	float fvrectbottom; // bottommost edge, for Alias clamping
	float horizontalFieldOfView; // at Z = 1.0, this many X is visible 
	// 2.0 = 90 degrees
	float xOrigin; // should probably allways be 0.5
	float yOrigin; // between be around 0.3 to 0.5

	vec3_t vieworg;
	vec3_t viewangles;

	color24 ambientlight;
	qboolean onlyClientDraws;
} refdef_t;

extern refdef_t r_refdef;
extern int r_dointerp;
extern int r_visframecount;

#if defined( SW_RENDERER )
struct texture_s;

extern int					r_blend; // Global blending factor for the current entity
extern vec3_t				r_origin;
extern qboolean				r_cache_thrash; // set if thrashing the surface cache
extern struct texture_s*	r_notexture_mip;
extern int					( *D_SurfaceCacheForRes )( int width, int height );
#endif

void		R_ForceCVars( qboolean multiplayer );
void		R_NewMap();

void		R_AllocObjects( int count );
void		R_DestroyObjects();

int			R_GetStudioBounds( const char* name, float* mins, float* maxs );

#if defined( SW_RENDERER )
void		R_ParseParticleEffect();
#endif

void		CL_SetDevOverView( refdef_t* view );
