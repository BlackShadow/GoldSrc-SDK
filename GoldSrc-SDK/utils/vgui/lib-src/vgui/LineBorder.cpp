#include "VGUI_LineBorder.h"
#include "VGUI_Panel.h"

namespace vgui
{

void LineBorder::setLineColor( int r, int g, int b, int a )
{
	_color = Color( r, g, b, a );
}

void LineBorder::init( int thickness, Color color )
{
	setInset( thickness, thickness, thickness, thickness );
	_color = color;
}

void LineBorder::paint( Panel* panel )
{
	int wide;
	int tall;
	int r;
	int g;
	int b;
	int a;

	panel->getSize( wide, tall );
	_color.getColor( r, g, b, a );
	drawSetColor( r, g, b, a );
	drawFilledRect( 0, 0, wide, _inset[1] );
	drawFilledRect( 0, tall - _inset[3], wide, tall );
	drawFilledRect( 0, _inset[1], _inset[0], tall - _inset[3] );
	drawFilledRect( wide - _inset[2], _inset[1], wide, tall - _inset[3] );
}

LineBorder::LineBorder()
{
	init( 1, Color( 0, 0, 0, 0 ) );
}

LineBorder::LineBorder( int thickness )
{
	init( thickness, Color( 0, 0, 0, 0 ) );
}

LineBorder::LineBorder( Color color )
{
	init( 1, color );
}

LineBorder::LineBorder( int thickness, Color color )
{
	init( thickness, color );
}

}
