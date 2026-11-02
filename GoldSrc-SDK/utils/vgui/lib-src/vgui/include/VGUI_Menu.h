#ifndef VGUI_MENU_H
#define VGUI_MENU_H

#include "VGUI.h"
#include "VGUI_Panel.h"

namespace vgui
{

class VGUIAPI Menu : public Panel
{
public:
	Menu( int x, int y, int wide, int tall );
	Menu( int wide, int tall );

	virtual void addMenuItem( Panel* panel );

	Menu& operator=( const Menu& ) = default;
};

}

#endif
