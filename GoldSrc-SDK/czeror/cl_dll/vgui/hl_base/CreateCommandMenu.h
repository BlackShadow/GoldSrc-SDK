#ifndef CREATECOMMANDMENU_H
#define CREATECOMMANDMENU_H

namespace vgui2
{
class Panel;
class Menu;
class ComboBox;
}

void CreateCommandComboMenu( char* menuFile, vgui2::ComboBox* parent, int playerTeam );
vgui2::Menu* CreateCommandMenu( char* menuFile, int playerTeam, vgui2::Panel* parent );

#endif
