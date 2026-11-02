#include "VGUI_Desktop.h"
#include "VGUI_DesktopIcon.h"
#include "VGUI_Frame.h"
#include "VGUI_MiniApp.h"
#include "VGUI_TaskBar.h"

#define DESKTOP_TASKBAR_TALL		36
#define DESKTOP_CASCADE_ORIGIN	50

using namespace vgui;

Desktop::Desktop( int x, int y, int wide, int tall ) : Panel( x, y, wide, tall )
{
	setBgColor( 0, 128, 128, 0 );
	setPaintBorderEnabled( false );
	setPaintBackgroundEnabled( false );
	setPaintEnabled( false );
	_background = new Panel( 0, 0, wide, tall - DESKTOP_TASKBAR_TALL );
	_background->setParent( this );
	_background->setBgColor( 0, 128, 128, 0 );
	_foreground = new Panel( 0, 0, wide, tall - DESKTOP_TASKBAR_TALL );
	_foreground->setPaintBorderEnabled( false );
	_foreground->setPaintBackgroundEnabled( false );
	_foreground->setPaintEnabled( false );
	_foreground->setParent( this );
	_taskBar = new TaskBar( 0, tall - DESKTOP_TASKBAR_TALL, wide, DESKTOP_TASKBAR_TALL );
	_taskBar->setParent( this );
	_cascade[0] = DESKTOP_CASCADE_ORIGIN;
	_cascade[1] = DESKTOP_CASCADE_ORIGIN;
}

void Desktop::setSize( int wide, int tall )
{
	Panel::setSize( wide, tall );
	getPaintSize( wide, tall );
	_background->setBounds( 0, 0, wide, tall - DESKTOP_TASKBAR_TALL );
	_foreground->setBounds( 0, 0, wide, tall - DESKTOP_TASKBAR_TALL );
	_taskBar->setBounds( 0, tall - DESKTOP_TASKBAR_TALL, wide, DESKTOP_TASKBAR_TALL );
}

void Desktop::addIcon( DesktopIcon* icon )
{
	const int iconInset = 10;

	icon->setDesktop( this );
	icon->setParent( _foreground );
	icon->setPos( iconInset, iconInset );
	_desktopIconDar.addElement( icon );
}

void Desktop::iconActivated( DesktopIcon* icon )
{
	Frame* frame = icon->getMiniApp()->createInstance();

	if ( frame == NULL )
	{
		return;
	}

	frame->setPos( _cascade[0], _cascade[1] );
	frame->setParent( _foreground );
	_taskBar->addFrame( frame );
	frame->requestFocus();

	const int cascadeStepX = 25;
	const int cascadeStepY = 50;
	const int cascadeLimitY = 400;

	_cascade[0] += cascadeStepX;
	_cascade[1] += cascadeStepY;
	if ( _cascade[1] > cascadeLimitY )
	{
		_cascade[0] = DESKTOP_CASCADE_ORIGIN;
		_cascade[1] = DESKTOP_CASCADE_ORIGIN;
	}
}

void Desktop::arrangeIcons()
{
	const int firstX = 15;
	const int firstY = 10;
	const int columnStep = 50;
	const int rowStep = 60;
	const int lastY = 340;
	int x = firstX;
	int y = firstY;

	for ( int i = 0; i < _desktopIconDar.getCount(); ++i )
	{
		_desktopIconDar[i]->setPos( x, y );
		y += rowStep;
		if ( y > lastY )
		{
			x += columnStep;
			y = firstY;
		}
	}
}

Panel* Desktop::getBackground()
{
	return _background;
}

Panel* Desktop::getForeground()
{
	return _foreground;
}
