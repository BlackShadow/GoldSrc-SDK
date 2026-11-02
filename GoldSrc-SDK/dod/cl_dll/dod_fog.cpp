#include "hud.h"
#include "cl_util.h"
#include "triangleapi.h"
#include "r_studioint.h"
#include <GL/gl.h>

extern engine_studio_api_t IEngineStudio;
extern int g_iOnlyClientDraw;
extern cvar_t* cl_fog;
extern cvar_t* cl_fog_start;
extern cvar_t* cl_fog_end;
extern cvar_t* cl_fog_red;
extern cvar_t* cl_fog_green;
extern cvar_t* cl_fog_blue;
extern cvar_t* cl_fog_density;

void RenderDoDFog()
{
	if ( gHUD.GetWaterLevel() > 2 )
		return;
	int mode = ( g_iOnlyClientDraw <= 0 && gHUD.GetMinimapState() != 0 ) ? 0 : (int)cl_fog->value;
	float start = cl_fog_start->value;
	float end = cl_fog_end->value;
	if ( !( start >= 0 && end >= 0 ) )
		return;
	float color[3] = { cl_fog_red->value, cl_fog_green->value, cl_fog_blue->value };
	if ( mode == 1 )
	{
		if ( IEngineStudio.IsHardware() == 1 )
		{
			GLfloat fogColor[4] = { 0.5f, 0.5f, 0.5f, 1 };
			glFogi( GL_FOG_MODE, GL_LINEAR );
			glFogfv( GL_FOG_COLOR, fogColor );
			glFogf( GL_FOG_DENSITY, cl_fog_density->value );
			glHint( GL_FOG_HINT, GL_NICEST );
			glFogf( GL_FOG_START, cl_fog_start->value );
			glFogf( GL_FOG_END, cl_fog_end->value );
			glEnable( GL_FOG );
		}
	}
	else
	{
		gEngfuncs.pTriAPI->Fog( color, start, end, mode == 2 );
		if ( mode != 2 && IEngineStudio.IsHardware() == 1 )
			glDisable( GL_FOG );
	}
}
