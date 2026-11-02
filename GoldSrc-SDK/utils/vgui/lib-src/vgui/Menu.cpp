#include "VGUI_Menu.h"
#include "VGUI_RaisedBorder.h"
#include "VGUI_StackLayout.h"

using namespace vgui;

void Menu::addMenuItem( Panel* panel )
{
	addChild( panel );
}

Menu::Menu( int x, int y, int wide, int tall ) : Panel( x, y, wide, tall )
{
	setBorder( new RaisedBorder() );
	setLayout( new StackLayout( 1, true ) );
}

Menu::Menu( int wide, int tall ) : Panel( 0, 0, wide, tall )
{
	setBorder( new RaisedBorder() );
	setLayout( new StackLayout( 1, true ) );
}
