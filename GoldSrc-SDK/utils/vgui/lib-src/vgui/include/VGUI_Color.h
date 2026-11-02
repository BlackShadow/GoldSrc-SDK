#ifndef VGUI_COLOR_H
#define VGUI_COLOR_H

#include "VGUI.h"
#include "VGUI_Scheme.h"

namespace vgui
{

class VGUIAPI Color
{
public:
	Color();
	Color( int r, int g, int b, int a );
	Color( Scheme::SchemeColor sc );

private:
	virtual void init();

public:
	virtual void setColor( int r, int g, int b, int a );
	virtual void setColor( Scheme::SchemeColor sc );
	virtual void getColor( int& r, int& g, int& b, int& a );
	virtual void getColor( Scheme::SchemeColor& sc );

	virtual int operator[]( int index );

private:
	unsigned char		_color[4];
	Scheme::SchemeColor	_schemeColor;

public:
	Color& operator=( const Color& ) = default;
};

}

#endif
