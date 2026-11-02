#include "vgui_EngineSurface.h"
#include "glquake.h"

EngineSurface* EngineSurface::s_pEngineSurface = NULL;

EXPOSE_SINGLE_INTERFACE( EngineSurface, IEngineSurface, "EngineSurface007" );

EngineSurface::EngineSurface()
{
	_renderPlat = NULL;
	createRenderPlat();
}

EngineSurface::~EngineSurface()
{
	deleteRenderPlat();
	_renderPlat = NULL;
	s_pEngineSurface = NULL;
}

void EngineSurface::freeEngineSurface()
{
	if ( s_pEngineSurface )
	{
		delete s_pEngineSurface;
		s_pEngineSurface = NULL;
	}
}

int EngineSurface::createNewTextureID()
{
	return (int)GL_GenTexture();
}

void EngineSurface::drawSetColor( int red, int green, int blue, int alpha )
{
	_drawColor[0] = red;
	_drawColor[1] = green;
	_drawColor[2] = blue;
	_drawColor[3] = alpha;
}

void EngineSurface::drawSetTextColor( int red, int green, int blue, int alpha )
{
	if ( _drawTextColor[0] == red && _drawTextColor[1] == green && _drawTextColor[2] == blue
		 && _drawTextColor[3] == alpha )
		return;

	drawFlushText();
	_drawTextColor[0] = red;
	_drawTextColor[1] = green;
	_drawTextColor[2] = blue;
	_drawTextColor[3] = alpha;
}

void EngineSurface::drawSetTextPos( int x, int y )
{
	_drawTextPos[0] = x;
	_drawTextPos[1] = y;
}

void EngineSurface::drawGetTextPos( int& x, int& y )
{
	x = _drawTextPos[0];
	y = _drawTextPos[1];
}
