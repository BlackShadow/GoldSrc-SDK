#ifndef VGUI_LINEBORDER_H
#define VGUI_LINEBORDER_H

#include "VGUI_Border.h"
#include "VGUI_Color.h"

namespace vgui
{

class Panel;

class VGUIAPI LineBorder : public Border
{
public:
	LineBorder();
	LineBorder( int thickness );
	LineBorder( Color color );
	LineBorder( int thickness, Color color );

	void setLineColor( int r, int g, int b, int a );

private:
	virtual void init( int thickness, Color color );

protected:
	virtual void paint( Panel* panel ) override;

private:
	Color	_color;

public:
	LineBorder& operator=( const LineBorder& ) = default;
};

}

#endif
