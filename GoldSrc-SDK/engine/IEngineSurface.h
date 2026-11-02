#pragma once

#include "interface.h"
#include "vgui/VGuiVertex.h"

class IEngineSurface : public IBaseInterface
{
public:
	virtual void pushMakeCurrent( int* insets, int* absoluteExtents, int* clipRect, bool translateToScreenSpace ) = 0;
	virtual void popMakeCurrent() = 0;

	virtual void drawFilledRect( int x0, int y0, int x1, int y1 ) = 0;
	virtual void drawOutlinedRect( int x0, int y0, int x1, int y1 ) = 0;
	virtual void drawLine( int x0, int y0, int x1, int y1 ) = 0;
	virtual void drawPolyLine( int* x, int* y, int count ) = 0;
	virtual void drawTexturedPolygon( vgui2::VGuiVertex* vertices, int count ) = 0;

	virtual void drawSetTextureRGBA( int textureID, const char* rgba, int width, int height, int hardwareFilter,
		int forceReload ) = 0;
	virtual void drawSetTexture( int textureID ) = 0;
	virtual void drawTexturedRect( int x0, int y0, int x1, int y1 ) = 0;
	virtual void drawTexturedRectAdd( int x0, int y0, int x1, int y1 ) = 0;
	virtual int createNewTextureID() = 0;

	virtual void drawSetColor( int red, int green, int blue, int alpha ) = 0;
	virtual void drawSetTextColor( int red, int green, int blue, int alpha ) = 0;
	virtual void drawSetTextPos( int x, int y ) = 0;
	virtual void drawGetTextPos( int& x, int& y ) = 0;
	virtual void drawPrintChar( int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1 ) = 0;
	virtual void drawPrintCharAdd( int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1 ) = 0;

	virtual void drawSetTextureFile( int textureID, const char* filename, int hardwareFilter, bool forceReload ) = 0;
	virtual void drawGetTextureSize( int textureID, int& width, int& height ) = 0;
	virtual int isTextureIdValid( int textureID ) = 0;
	virtual void drawSetSubTextureRGBA( int textureID, int x, int y, const unsigned char* rgba, int width,
		int height ) = 0;
	virtual void drawFlushText() = 0;
	virtual void resetViewPort() = 0;
	virtual void drawSetTextureBGRA( int textureID, const char* bgra, int width, int height, int hardwareFilter,
		int forceReload ) = 0;
	virtual void drawUpdateRegionTextureBGRA( int textureID, int x, int y, const unsigned char* bgra, int width,
		int height ) = 0;
};
