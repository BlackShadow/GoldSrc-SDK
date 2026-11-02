#include "VGUI_Color.h"

#include "VGUI.h"
#include "VGUI_Scheme.h"
#include "VGUI_App.h"

using namespace vgui;

void Color::init()
{
	_color[0] = 0;
	_color[1] = 0;
	_color[2] = 0;
	_color[3] = 0;
	_schemeColor = Scheme::sc_user;
}

void Color::setColor( int r, int g, int b, int a )
{
	_schemeColor = Scheme::sc_user;
	_color[0] = (unsigned char)r;
	_color[1] = (unsigned char)g;
	_color[2] = (unsigned char)b;
	_color[3] = (unsigned char)a;
}

void Color::setColor( Scheme::SchemeColor sc )
{
	_schemeColor = sc;
}

void Color::getColor( Scheme::SchemeColor& sc )
{
	sc = _schemeColor;
}

void Color::getColor( int& r, int& g, int& b, int& a )
{
	if ( _schemeColor != Scheme::sc_user )
	{
		App::getInstance()->getScheme()->getColor( _schemeColor, r, g, b, a );
	}
	else
	{
		r = _color[0];
		g = _color[1];
		b = _color[2];
		a = _color[3];
	}
}

int Color::operator[]( int index )
{
	int co[4];

	getColor( co[0], co[1], co[2], co[3] );
	return co[index];
}

Color::Color()
{
	init();
}

Color::Color( int r, int g, int b, int a )
{
	init();
	setColor( r, g, b, a );
}

Color::Color( Scheme::SchemeColor sc )
{
	init();
	setColor( sc );
}
