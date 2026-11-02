#if defined( SW_RENDERER )

#include "draw.h"
#include "screen.h"
#include "r_local.h"
#include "client.h"
#include "cvar.h"
#include "enginecallbacks.h"
#include "sound.h"

extern cvar_t	crosshair;
extern sfx_t*	cl_sfx_geiger1;
extern sfx_t*	cl_sfx_geiger2;
extern sfx_t*	cl_sfx_geiger3;
extern sfx_t*	cl_sfx_geiger4;
extern sfx_t*	cl_sfx_geiger5;
extern sfx_t*	cl_sfx_geiger6;

void	DrawCrosshair( int x, int y );

double gflGeigerDelay = -1.0;
int giGeigerRange = -1;
int giHudLevel = 1;

void HudSizeUp()
{
	if ( scr_viewsize.value < 120.0f )
		Cvar_SetValue( "viewsize", scr_viewsize.value + 10.0f );
}

void HudSizeDown()
{
	if ( scr_viewsize.value > 30.0f )
		Cvar_SetValue( "viewsize", scr_viewsize.value - 10.0f );
}

void Sbar_Geiger()
{
	if ( cl.time < gflGeigerDelay )
		return;
	gflGeigerDelay = cl.time + 0.1;
	if ( giGeigerRange <= 0 || giGeigerRange >= 1000 )
		return;

	if ( giGeigerRange > 800 )
	{
		RandomFloat( 0.0f, 0.5f );
		RandomLong( 0, 127 );
		RandomLong( 0, 127 );
		return;
	}

	int pct;
	float flvol;
	int soundCount;
	sfx_t* rgsfx[3];
	if ( giGeigerRange > 600 )
	{
		pct = 2;
		flvol = 0.4f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger1;
		soundCount = 2;
	}
	else if ( giGeigerRange > 500 )
	{
		pct = 4;
		flvol = 0.5f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger2;
		soundCount = 2;
	}
	else if ( giGeigerRange > 400 )
	{
		pct = 8;
		flvol = 0.6f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger2;
		rgsfx[2] = cl_sfx_geiger3;
		soundCount = 3;
	}
	else if ( giGeigerRange > 300 )
	{
		pct = 8;
		flvol = 0.7f;
		rgsfx[0] = cl_sfx_geiger2;
		rgsfx[1] = cl_sfx_geiger3;
		rgsfx[2] = cl_sfx_geiger4;
		soundCount = 3;
	}
	else if ( giGeigerRange > 200 )
	{
		pct = 28;
		flvol = 0.78f;
		rgsfx[0] = cl_sfx_geiger2;
		rgsfx[1] = cl_sfx_geiger3;
		rgsfx[2] = cl_sfx_geiger4;
		soundCount = 3;
	}
	else if ( giGeigerRange > 150 )
	{
		pct = 40;
		flvol = 0.8f;
		rgsfx[0] = cl_sfx_geiger3;
		rgsfx[1] = cl_sfx_geiger4;
		rgsfx[2] = cl_sfx_geiger5;
		soundCount = 3;
	}
	else if ( giGeigerRange > 100 )
	{
		pct = 60;
		flvol = 0.85f;
		rgsfx[0] = cl_sfx_geiger3;
		rgsfx[1] = cl_sfx_geiger4;
		rgsfx[2] = cl_sfx_geiger5;
		soundCount = 3;
	}
	else if ( giGeigerRange > 75 )
	{
		pct = 80;
		flvol = 0.9f;
		rgsfx[0] = cl_sfx_geiger4;
		rgsfx[1] = cl_sfx_geiger5;
		rgsfx[2] = cl_sfx_geiger6;
		soundCount = 3;
	}
	else if ( giGeigerRange > 50 )
	{
		pct = 90;
		flvol = 0.95f;
		rgsfx[0] = cl_sfx_geiger5;
		rgsfx[1] = cl_sfx_geiger6;
		soundCount = 2;
	}
	else
	{
		pct = 95;
		flvol = 1.0f;
		rgsfx[0] = cl_sfx_geiger5;
		rgsfx[1] = cl_sfx_geiger6;
		soundCount = 2;
	}

	flvol = RandomFloat( 0.0f, 0.5f ) * flvol + 0.25f;
	if ( pct > RandomLong( 0, 127 ) || pct > RandomLong( 0, 127 ) )
	{
		S_StartDynamicSound( -1, CHAN_AUTO, rgsfx[RandomLong( 0, soundCount - 1 )],
			r_origin, flvol, 1.0f, 0, PITCH_NORM );
	}
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw()
{
	if ( !giHudLevel || scr_con_current == (float)vid.height )
		return;
	scr_copyeverything = 1;
	if ( !crosshair.value )
		return;

	float x = (float)( scr_vrect.x + scr_vrect.width / 2 );
	float y = (float)( scr_vrect.y + scr_vrect.height / 2 );
	vec3_t angles, forward, point, screen;
	VectorAdd( r_refdef.viewangles, cl.crosshairangle, angles );
	AngleVectors( angles, forward, NULL, NULL );
	VectorAdd( r_origin, forward, point );
	ScreenTransform( point, screen );
	float crosshairx = (float)( screen[0] * xscale + 0.5 + x );
	float crosshairy = (float)( screen[1] * yscale + 0.5 + y );
	DrawCrosshair( (int)crosshairx, (int)crosshairy );
}

void FillBackGround()
{
	if ( (unsigned int)scr_vrect.width == vid.width && (unsigned int)scr_vrect.height == vid.height )
		return;
	if ( (unsigned int)scr_vrect.height != vid.height )
	{
		Draw_Fill( 0, scr_vrect.y + scr_vrect.height, vid.width,
			vid.height - ( scr_vrect.y + scr_vrect.height ), 0 );
	}
	int width = scr_vrect.x;
	if ( width )
	{
		int height = scr_vrect.y + scr_vrect.height;
		Draw_Fill( 0, 0, width, height, 0 );
		Draw_Fill( scr_vrect.x + scr_vrect.width, 0, width, height, 0 );
	}
	if ( (unsigned int)scr_vrect.height != vid.height )
		Draw_Fill( scr_vrect.x, 0, vid.width, scr_vrect.y, 0 );
}

#else

#include "client.h"
#include "cvar.h"
#include "render.h"
#include "sound.h"
#include "vid.h"
#include "enginecallbacks.h"

extern cvar_t scr_viewsize;
extern cvar_t crosshair;
extern float scr_con_current;
extern int scr_copyeverything;
extern vrect_t scr_vrect;
extern vec3_t r_origin;
extern sfx_t* cl_sfx_geiger1;
extern sfx_t* cl_sfx_geiger2;
extern sfx_t* cl_sfx_geiger3;
extern sfx_t* cl_sfx_geiger4;
extern sfx_t* cl_sfx_geiger5;
extern sfx_t* cl_sfx_geiger6;

int ScreenTransform( vec_t* point, vec_t* screen );
void DrawCrosshair( int x, int y );

double gflGeigerDelay = -1.0;
int giGeigerRange = -1;
int giHudLevel = 1;

void HudSizeUp()
{
	if ( scr_viewsize.value < 120.0f )
		Cvar_SetValue( "viewsize", scr_viewsize.value + 10.0f );
}

void HudSizeDown()
{
	if ( scr_viewsize.value > 30.0f )
		Cvar_SetValue( "viewsize", scr_viewsize.value - 10.0f );
}

void Sbar_Geiger()
{
	int pct;
	float flvol;
	int soundCount;
	sfx_t* rgsfx[3];

	if ( cl.time < gflGeigerDelay )
		return;

	gflGeigerDelay = cl.time + 0.1;

	if ( giGeigerRange <= 0 || giGeigerRange >= 1000 )
		return;

	pct = 0;
	flvol = 0.0f;
	soundCount = 0;
	rgsfx[0] = NULL;
	rgsfx[1] = NULL;
	rgsfx[2] = NULL;

	// piecewise linear is better than continuous formula for this
	if ( giGeigerRange > 800 )
	{
		pct = 0;
	}
	else if ( giGeigerRange > 600 )
	{
		pct = 2;
		flvol = 0.4f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger1;
		soundCount = 2;
	}
	else if ( giGeigerRange > 500 )
	{
		pct = 4;
		flvol = 0.5f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger2;
		soundCount = 2;
	}
	else if ( giGeigerRange > 400 )
	{
		pct = 8;
		flvol = 0.6f;
		rgsfx[0] = cl_sfx_geiger1;
		rgsfx[1] = cl_sfx_geiger2;
		rgsfx[2] = cl_sfx_geiger3;
		soundCount = 3;
	}
	else if ( giGeigerRange > 300 )
	{
		pct = 8;
		flvol = 0.7f;
		rgsfx[0] = cl_sfx_geiger2;
		rgsfx[1] = cl_sfx_geiger3;
		rgsfx[2] = cl_sfx_geiger4;
		soundCount = 3;
	}
	else if ( giGeigerRange > 200 )
	{
		pct = 28;
		flvol = 0.78f;
		rgsfx[0] = cl_sfx_geiger2;
		rgsfx[1] = cl_sfx_geiger3;
		rgsfx[2] = cl_sfx_geiger4;
		soundCount = 3;
	}
	else if ( giGeigerRange > 150 )
	{
		pct = 40;
		flvol = 0.8f;
		rgsfx[0] = cl_sfx_geiger3;
		rgsfx[1] = cl_sfx_geiger4;
		rgsfx[2] = cl_sfx_geiger5;
		soundCount = 3;
	}
	else if ( giGeigerRange > 100 )
	{
		pct = 60;
		flvol = 0.85f;
		rgsfx[0] = cl_sfx_geiger3;
		rgsfx[1] = cl_sfx_geiger4;
		rgsfx[2] = cl_sfx_geiger5;
		soundCount = 3;
	}
	else if ( giGeigerRange > 75 )
	{
		pct = 80;
		flvol = 0.9f;
		rgsfx[0] = cl_sfx_geiger4;
		rgsfx[1] = cl_sfx_geiger5;
		rgsfx[2] = cl_sfx_geiger6;
		soundCount = 3;
	}
	else if ( giGeigerRange > 50 )
	{
		pct = 90;
		flvol = 0.95f;
		rgsfx[0] = cl_sfx_geiger5;
		rgsfx[1] = cl_sfx_geiger6;
		soundCount = 2;
	}
	else
	{
		pct = 95;
		flvol = 1.0f;
		rgsfx[0] = cl_sfx_geiger5;
		rgsfx[1] = cl_sfx_geiger6;
		soundCount = 2;
	}

	flvol = flvol * RandomFloat( 0.0f, 0.5f ) + 0.25f;

	if ( pct > RandomLong( 0, 127 ) || pct > RandomLong( 0, 127 ) )
		S_StartDynamicSound( -1, CHAN_AUTO, rgsfx[RandomLong( 0, soundCount - 1 )], r_origin, flvol, 1.0f, 0, PITCH_NORM );
}

/*
===============
Sbar_Draw
===============
*/
void Sbar_Draw()
{
	float x, y, crosshairx, crosshairy;
	vec3_t angles, forward, point, screen;

	if ( !giHudLevel )
		return;

	if ( (double)vid.height == scr_con_current )
		return; // console is full screen

	scr_copyeverything = 1;

	if ( !crosshair.value )
		return;

	x = (float)( scr_vrect.x + scr_vrect.width / 2 );
	y = (float)( scr_vrect.y + scr_vrect.height / 2 );

	VectorAdd( r_refdef.viewangles, cl.crosshairangle, angles );
	AngleVectors( angles, forward, NULL, NULL );
	VectorAdd( r_origin, forward, point );
	ScreenTransform( point, screen );

	crosshairy = (float)( 0.5 * screen[1] * scr_vrect.height + 0.5 + y );
	crosshairx = (float)( 0.5 * screen[0] * scr_vrect.width + 0.5 + x );
	DrawCrosshair( (int)crosshairx, (int)crosshairy );
}

#endif
