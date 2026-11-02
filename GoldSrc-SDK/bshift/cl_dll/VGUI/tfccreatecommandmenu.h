#pragma once

#include "CreateCommandMenu.h"

class TFCCCreateCommandComboMenu : public CCreateCommandComboMenu
{
public:
	vgui2::Menu* CreateCustomButton( vgui2::Panel* parent, vgui2::Menu* menu,
		char* boundKey, char* text, char* command, bool isMenu ) override;
	bool IsCustomToken( const char* token ) override;
};
