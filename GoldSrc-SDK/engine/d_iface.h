#pragma once

#include "model.h"
#include "vid.h"
#include "render.h"
#include "r_shared.h"

#include "../common/cvardef.h"
#include "../common/particledef.h"

typedef struct
{
	float	u;
	float	v;
	float	s;
	float	t;
	float	zi;
} emitpoint_t;

typedef struct polyvert_s
{
	float	u;
	float	v;
	float	zi;
	float	s;
	float	t;
} polyvert_t;

typedef struct polydesc_s
{
	int			numverts;
	float		nearzi;
	msurface_t*	pcurrentface;
	polyvert_t*	pverts;
} polydesc_t;

typedef struct finalvert_s
{
	int		v[6];		// u, v, s, t, l, 1/z
	int		flags;
	float	reserved;
} finalvert_t;

typedef struct
{
	void*				pskin;
	maliasskindesc_t*	pskindesc;
	int					skinwidth;
	int					skinheight;

	mtriangle_t*		ptriangles;
	finalvert_t*		pfinalverts;

	int					numtriangles;
	int					drawtype;
	int					seamfixupX16;
} affinetridesc_t;

typedef struct
{
	int					nump;
	emitpoint_t*		pverts; // there's room for an extra element at [nump], 
	//  if the driver wants to duplicate element [0] at
	//  element [nump] to avoid dealing with wrapping
	mspriteframe_t*		pspriteframe;
	vec3_t				vup;
	vec3_t				vright;
	vec3_t				vpn;
	float				nearzi;
	float				scale;
} spritedesc_t;

typedef struct
{
	int		u;
	int		v;
	float	zi;
	int		color;
} zpointdesc_t;

//=======================================================================//

// callbacks to Quake

typedef struct
{
	pixel_t*	surfdat; // destination for generated surface
	int			rowbytes;	// destination logical width in bytes
	msurface_t*	surf; // description for surface to generate

	fixed8_t	lightadj[MAXLIGHTMAPS];
	// adjust for lightmap levels for dynamic lighting
	texture_t*	texture; // corrected for animating textures

	int			surfmip;	// mipmapped ratio of surface texels / world pixels
	int			surfwidth;	// in mipmapped texels
	int			surfheight;	// in mipmapped texels
} drawsurf_t;

extern affinetridesc_t	r_affinetridesc;
extern spritedesc_t		r_spritedesc;
extern drawsurf_t		r_drawsurf;
extern cvar_t			r_drawflat;
extern int				c_surf;
extern int				d_spanpixcount;
extern int				r_framecount; // sequence # of current frame since Quake
//  started
extern int				r_pixbytes;
extern qboolean			r_dowarp;
extern qboolean			r_drawpolys; // 1 if driver wants clipped polygons
//  rather than a span list
extern qboolean			r_drawculledpolys; // 1 if driver wants clipped polygons that
//  have been culled by the edge list
extern qboolean			r_worldpolysbacktofront; // 1 if driver wants polygons
//  delivered back to front rather
//  than front to back
extern qboolean			r_recursiveaffinetriangles; // true if a driver wants to use
//  recursive triangular subdivison
//  and vertex drawing via
//  D_PolysetDrawFinalVerts() past
//  a certain distance (normally 
//  only used by the software
//  driver)
extern float			r_aliasuvscale; // scale-up factor for screen u and v
//  on Alias vertices passed to driver
extern vec3_t			r_pright;
extern vec3_t			r_pup;
extern vec3_t			r_ppn;
extern byte*			r_warpbuffer;
extern vrect_t			scr_vrect;

void	D_Init();
void	D_TurnZOn();
void	D_SetupFrame();
void	D_UpdateRects( vrect_t* rectangles );

void	D_ViewChanged();

void	D_StartParticles();
void	D_EndParticles();
void	D_DrawParticle( particle_t* particle );

void	D_FillRect( vrect_t* rect, byte* color );

void	D_DrawPoly();

void	D_WarpScreen();

void	D_DrawSprite();

void	D_PolysetUpdateTables();
void	D_PolysetDraw();

void	R_DrawSurface();
