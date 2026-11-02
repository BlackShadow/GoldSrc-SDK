#include "VGUI_DesktopIcon.h"

#include "VGUI_App.h"
#include "VGUI_Desktop.h"
#include "VGUI_Font.h"
#include "VGUI_Image.h"
#include "VGUI_InputSignal.h"
#include "VGUI_MiniApp.h"
#include "VGUI_Panel.h"
#include "VGUI_Scheme.h"

#include <string.h>

namespace vgui
{

class DesktopIcon;

}

namespace
{

using namespace vgui;

class FooHandler : public InputSignal
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	FooHandler( DesktopIcon* icon )
	{
		_desktopIcon = icon;
		_dragging = false;
	}

	virtual void cursorMoved( int x, int y, Panel* panel ) override;
	virtual void cursorEntered( Panel* panel ) override;
	virtual void cursorExited( Panel* panel ) override;

	virtual void mousePressed( MouseCode code, Panel* panel ) override;
	virtual void mouseDoublePressed( MouseCode code, Panel* panel ) override;
	virtual void mouseReleased( MouseCode code, Panel* panel ) override;
	virtual void mouseWheeled( int delta, Panel* panel ) override;

	virtual void keyPressed( KeyCode code, Panel* panel ) override;
	virtual void keyTyped( KeyCode code, Panel* panel ) override;
	virtual void keyReleased( KeyCode code, Panel* panel ) override;
	virtual void keyFocusTicked( Panel* panel ) override;

private:
	DesktopIcon*	_desktopIcon;

	bool			_dragging;
	int				_dragOrgPos[2];
	int				_dragStart[2];
};

}

namespace
{

void FooHandler::cursorMoved( int x, int y, Panel* panel )
{
	if ( _dragging )
	{
		panel->getApp()->getCursorPos( x, y );
		_desktopIcon->setPos( _dragOrgPos[0] + x - _dragStart[0], _dragOrgPos[1] + y - _dragStart[1] );

		if ( _desktopIcon->getParent() != NULL )
		{
			_desktopIcon->getParent()->repaint();
		}
	}
}

void FooHandler::cursorEntered( Panel* )
{
}

void FooHandler::cursorExited( Panel* )
{
}

void FooHandler::mousePressed( MouseCode, Panel* panel )
{
	int x;
	int y;

	panel->getApp()->getCursorPos( x, y );
	_dragging = true;
	_dragStart[0] = x;
	_dragStart[1] = y;
	_desktopIcon->getPos( _dragOrgPos[0], _dragOrgPos[1] );
	panel->getApp()->setMouseCapture( panel );
	_desktopIcon->requestFocus();
}

void FooHandler::mouseDoublePressed( MouseCode, Panel* )
{
	_desktopIcon->doActivate();
}

void FooHandler::mouseReleased( MouseCode, Panel* panel )
{
	_dragging = false;
	panel->getApp()->setMouseCapture( NULL );
}

void FooHandler::mouseWheeled( int, Panel* )
{
}

void FooHandler::keyPressed( KeyCode, Panel* )
{
}

void FooHandler::keyTyped( KeyCode, Panel* )
{
}

void FooHandler::keyReleased( KeyCode, Panel* )
{
}

void FooHandler::keyFocusTicked( Panel* )
{
}

}

namespace vgui
{

DesktopIcon::DesktopIcon( MiniApp* miniApp, Image* image ) : Panel( 0, 0, 32, 50 )
{
	_desktop = NULL;
	_miniApp = miniApp;
	setImage( image );

	if ( image != NULL )
	{
		int wide;
		int tall;

		image->getSize( wide, tall );
		Panel::setSize( wide, tall );
	}

	addInputSignal( new FooHandler( this ) );
}

void DesktopIcon::doActivate()
{
	if ( _desktop != NULL )
	{
		_desktop->iconActivated( this );
	}
}

void DesktopIcon::setImage( Image* image )
{
	int wide;
	int tall;

	_image = image;

	if ( image != NULL )
	{
		image->getSize( wide, tall );
		setSize( wide, tall );
	}
}

void DesktopIcon::setDesktop( Desktop* desktop )
{
	_desktop = desktop;
}

MiniApp* DesktopIcon::getMiniApp()
{
	return _miniApp;
}

void DesktopIcon::paintBackground()
{
	Panel::paintBackground();

	int wide;
	int tall;

	getPaintSize( wide, tall );
	if ( _image != NULL )
	{
		_image->doPaint( this );
	}

	Font* font = getApp()->getScheme()->getFont( Scheme::sf_primary2 );

	drawSetTextFont( Scheme::sf_primary1 );
	drawSetTextColor( Scheme::sc_black );
	if ( _miniApp != NULL )
	{
		char name[256];

		_miniApp->getName( name, sizeof( name ) );

		int textWide;
		int textTall;

		font->getTextSize( name, textWide, textTall );
		drawSetTextColor( 255, 255, 255, 0 );

		const int labelOffset = 20;

		drawSetTextPos( wide / 2 - textWide / 2, tall - labelOffset );
		drawPrintText( name, (int)strlen( name ) );
	}
}

}
