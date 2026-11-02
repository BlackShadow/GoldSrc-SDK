#ifndef VGUI_SCROLLPANEL_H
#define VGUI_SCROLLPANEL_H

#include "VGUI.h"
#include "VGUI_Panel.h"

namespace vgui
{

class ScrollBar;

class VGUIAPI ScrollPanel : public Panel
{
public:
	ScrollPanel( int x, int y, int wide, int tall );

	virtual void setScrollBarVisible( bool horizontal, bool vertical );
	virtual void setScrollBarAutoVisible( bool horizontal, bool vertical );

	virtual Panel* getClient();
	virtual Panel* getClientClip();
	virtual void setScrollValue( int horizontal, int vertical );
	virtual void getScrollValue( int& horizontal, int& vertical );
	virtual void recomputeClientSize();

	virtual ScrollBar* getHorizontalScrollBar();
	virtual ScrollBar* getVerticalScrollBar();
	virtual void validate();
	virtual void recomputeScroll();

protected:
	virtual void setSize( int wide, int tall ) override;

	Panel*		_clientClip;
	Panel*		_client;

	ScrollBar*	_horizontalScrollBar;
	ScrollBar*	_verticalScrollBar;
	bool		_autoVisible[2];

public:
	ScrollPanel& operator=( const ScrollPanel& ) = default;
};

}

#endif
