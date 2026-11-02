#pragma once

#include "IEngineSurface.h"

class EngineSurfaceRenderPlat;

class EngineSurface : public IEngineSurface
{
public:
	EngineSurface();
	~EngineSurface() override;

	void pushMakeCurrent( int* insets, int* absoluteExtents, int* clipRect, bool translateToScreenSpace ) override;
	void popMakeCurrent() override;

	void drawFilledRect( int x0, int y0, int x1, int y1 ) override;
	void drawOutlinedRect( int x0, int y0, int x1, int y1 ) override;
	void drawLine( int x0, int y0, int x1, int y1 ) override;
	void drawPolyLine( int* x, int* y, int count ) override;
	void drawTexturedPolygon( vgui2::VGuiVertex* vertices, int count ) override;

	void drawSetTextureRGBA( int textureID, const char* rgba, int width, int height, int hardwareFilter,
		int forceReload ) override;
	void drawSetTexture( int textureID ) override;
	void drawTexturedRect( int x0, int y0, int x1, int y1 ) override;
	void drawTexturedRectAdd( int x0, int y0, int x1, int y1 ) override;
	int createNewTextureID() override;

	void drawSetColor( int red, int green, int blue, int alpha ) override;
	void drawSetTextColor( int red, int green, int blue, int alpha ) override;
	void drawSetTextPos( int x, int y ) override;
	void drawGetTextPos( int& x, int& y ) override;
	void drawPrintChar( int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1 ) override;
	void drawPrintCharAdd( int x0, int y0, int x1, int y1, float s0, float t0, float s1, float t1 ) override;

	void drawSetTextureFile( int textureID, const char* filename, int hardwareFilter, bool forceReload ) override;
	void drawGetTextureSize( int textureID, int& width, int& height ) override;
	int isTextureIdValid( int textureID ) override;
	void drawSetSubTextureRGBA( int textureID, int x, int y, const unsigned char* rgba, int width,
		int height ) override;
	void drawFlushText() override;
	void resetViewPort() override;
	void drawSetTextureBGRA( int textureID, const char* bgra, int width, int height, int hardwareFilter,
		int forceReload ) override;
	void drawUpdateRegionTextureBGRA( int textureID, int x, int y, const unsigned char* bgra, int width,
		int height ) override;

	static void freeEngineSurface();

private:
	void createRenderPlat();
	void deleteRenderPlat();

	static EngineSurface* s_pEngineSurface;

	EngineSurfaceRenderPlat*	_renderPlat;
	int							_drawColor[4];
	int							_drawTextColor[4];
	int							_drawTextPos[2];
};
