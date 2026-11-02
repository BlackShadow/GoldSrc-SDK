#include "tfccreatecommandmenu.h"

TFCCCreateCommandComboMenu g_CreateCommandMenu;

vgui2::Menu* TFCCCreateCommandComboMenu::CreateCustomButton( vgui2::Panel* parent, vgui2::Menu* menu,
	char* boundKey, char* text, char* command, bool isMenu )
{
	return CCreateCommandComboMenu::CreateCustomButton( parent, menu, boundKey, text, command, isMenu );
}

bool TFCCCreateCommandComboMenu::IsCustomToken( const char* )
{
	return false;
}
