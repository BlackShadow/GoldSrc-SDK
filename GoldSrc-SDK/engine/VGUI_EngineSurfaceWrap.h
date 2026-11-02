#pragma once

#include <VGUI_SurfaceBase.h>
#include "vgui/VGuiVertex.h"

class IEngineSurface;

namespace vgui
{
class BitmapTGA;
}

class EngineSurfaceWrap : public vgui::SurfaceBase
{
public:
	EngineSurfaceWrap( vgui::Panel* embeddedPanel, IEngineSurface* engineSurface );
	~EngineSurfaceWrap();

	void setTitle( const char* title ) override;
	bool setFullscreenMode( int wide, int tall, int bpp ) override;
	void setWindowedMode() override;
	void setAsTopMost( bool state ) override;
	void createPopup( vgui::Panel* embeddedPanel ) override;
	bool hasFocus() override;
	bool isWithin( int x, int y ) override;
	int createNewTextureID() override;

	void GetMousePos( int& x, int& y ) override;
	void setCursor( vgui::Cursor* cursor ) override;

	virtual void AppHandler( void* event, void* userData );
	virtual void lockCursor();
	virtual void unlockCursor();

protected:
	void drawSetColor( int r, int g, int b, int a ) override;
	void drawFilledRect( int x0, int y0, int x1, int y1 ) override;
	void drawOutlinedRect( int x0, int y0, int x1, int y1 ) override;
	void drawSetTextFont( vgui::Font* font ) override;
	void drawSetTextColor( int r, int g, int b, int a ) override;
	void drawSetTextPos( int x, int y ) override;
	void drawPrintText( const char* text, int textLen ) override;
	void drawSetTextureRGBA( int id, const char* rgba, int wide, int tall ) override;
	void drawSetTexture( int id ) override;
	void drawTexturedRect( int x0, int y0, int x1, int y1 ) override;

	void invalidate( vgui::Panel* panel ) override;
	void enableMouseCapture( bool state ) override;
	void swapBuffers() override;
	void pushMakeCurrent( vgui::Panel* panel, bool useInsets ) override;
	void popMakeCurrent( vgui::Panel* panel ) override;
	void applyChanges() override;

	virtual void drawLine( int x0, int y0, int x1, int y1 );
	virtual void drawPolyLine( int* x, int* y, int count );
	virtual void drawTexturedPolygon( vgui::VGuiVertex* vertices, int count );
	virtual void drawSetTextureBGRA( int id, const char* rgba, int wide, int tall, int hardwareFilter,
		int forceReload );
	virtual void drawUpdateRegionTextureBGRA( int id, int x, int y, const unsigned char* data, int wide, int tall );
	virtual void drawGetTextPos( int& x, int& y );
	virtual void drawPrintChar( int x, int y, int wide, int tall, float s0, float t0, float s1, float t1 );
	virtual void drawPrintCharAdd( int x, int y, int wide, int tall, float s0, float t0, float s1, float t1 );

private:
	vgui::BitmapTGA*	_mouseBitmap;
	IEngineSurface*		_engineSurface;
	bool				_cursorLocked;
};
