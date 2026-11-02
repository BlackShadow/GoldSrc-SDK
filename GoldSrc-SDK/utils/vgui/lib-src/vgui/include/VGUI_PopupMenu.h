#ifndef VGUI_POPUPMENU_H
#define VGUI_POPUPMENU_H

#include "VGUI_Menu.h"

namespace vgui
{

class VGUIAPI PopupMenu : public Menu
{
public:
	PopupMenu( int x, int y, int wide, int tall );
	PopupMenu( int wide, int tall );

	virtual void showModal( Panel* panel );

	PopupMenu& operator=( const PopupMenu& ) = default;
};

}

#endif
