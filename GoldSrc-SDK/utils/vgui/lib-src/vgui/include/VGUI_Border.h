#ifndef VGUI_BORDER_H
#define VGUI_BORDER_H

#include "VGUI_Image.h"

namespace vgui
{

class Panel;

class VGUIAPI Border : public Image
{
public:
	Border();
	Border( int left, int top, int right, int bottom );

	virtual void setInset( int left, int top, int right, int bottom );
	virtual void getInset( int& left, int& top, int& right, int& bottom );

protected:
	virtual void drawFilledRect( int x0, int y0, int x1, int y1 ) override;
	virtual void drawOutlinedRect( int x0, int y0, int x1, int y1 ) override;

	virtual void drawSetTextPos( int x, int y ) override;
	virtual void drawPrintText( int x, int y, const char* str, int strlen ) override;
	virtual void drawPrintChar( int x, int y, char ch ) override;

	int		_inset[4];
	Panel*	_panel;

public:
	Border& operator=( const Border& ) = default;
};

}

#endif
