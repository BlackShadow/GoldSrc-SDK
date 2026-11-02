#include "VGUI_MenuItem.h"

namespace vgui
{

MenuItem::MenuItem( const char* text )
	: Button( text, 0, 0 )
{
	setButtonBorderEnabled( false );
	_subMenu = NULL;
}

MenuItem::MenuItem( const char* text, Menu* subMenu )
	: Button( text, 0, 0 )
{
	setButtonBorderEnabled( false );
	_subMenu = subMenu;
}

}
