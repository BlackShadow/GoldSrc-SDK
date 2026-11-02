#include "d_local.h"
#include "r_local.h"
#include "cvar.h"

cvar_t d_subdiv16 = { "d_subdiv16", "1", 0 };
cvar_t d_mipcap = { "d_mipcap", "0", 0 };
cvar_t d_mipscale = { "d_mipscale", "1", 0 };
cvar_t d_spriteskip = { "d_spriteskip", "0", FCVAR_ARCHIVE };

qboolean d_roverwrapped;
surfcache_t* d_initial_rover;
int d_minmip;
float d_scalemip[3];
void ( *d_drawspans )( espan_t* );

/*
===============
D_Init
===============
*/
void D_Init()
{
	Cvar_RegisterVariable( &d_mipcap );
	Cvar_RegisterVariable( &d_subdiv16 );
	Cvar_RegisterVariable( &d_mipscale );
	Cvar_RegisterVariable( &d_spriteskip );

	r_drawpolys = FALSE;
	r_worldpolysbacktofront = FALSE;
	r_recursiveaffinetriangles = TRUE;
	r_pixbytes = 1;
	r_aliasuvscale = 1.0f;
	D_InitFade();
}

void D_CopyRects( vrect_t*, int )
{
}

void D_TurnZOn()
{
}

/*
===============
D_SetupFrame
===============
*/
void D_SetupFrame()
{
	if ( r_dowarp )
	{
		d_viewbuffer = r_warpbuffer;
		screenwidth = WARP_WIDTH;
	}
	else
	{
		d_viewbuffer = vid.buffer;
		screenwidth = vid.rowbytes;
	}

	d_roverwrapped = FALSE;
	d_initial_rover = sc_rover;
	d_minmip = (int)d_mipcap.value;
	if ( d_minmip > 3 )
		d_minmip = 3;
	else if ( d_minmip < 0 )
		d_minmip = 0;

	d_drawspans = D_DrawSpans8;
	d_scalemip[0] = d_mipscale.value;
	d_scalemip[1] = 0.4f * d_mipscale.value;
	d_scalemip[2] = 0.2f * d_mipscale.value;
	d_aflatcolor = 0;
}

void D_UpdateRects( vrect_t* )
{
}
