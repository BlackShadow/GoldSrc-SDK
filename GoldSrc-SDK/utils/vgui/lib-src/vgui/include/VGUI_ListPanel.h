#ifndef VGUI_LISTPANEL_H
#define VGUI_LISTPANEL_H

#include "VGUI.h"
#include "VGUI_Panel.h"

namespace vgui
{

class ScrollBar;

class VGUIAPI ListPanel : public Panel
{
public:
	ListPanel( int x, int y, int wide, int tall );

public:
	virtual void addString( const char* str );
	virtual void setSize( int wide, int tall ) override;
	virtual void addItem( Panel* panel );

	virtual void setPixelScroll( int value );
	virtual void translatePixelScroll( int delta );

protected:
	virtual void performLayout() override;
	virtual void paintBackground() override;

protected:
	Panel*		_vpanel;
	ScrollBar*	_scroll;

public:
	ListPanel& operator=( const ListPanel& ) = default;
};

}

#endif
