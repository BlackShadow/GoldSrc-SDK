#include "VGUI_StackLayout.h"
#include "VGUI_Panel.h"

namespace vgui
{

StackLayout::StackLayout( int vgap, bool fitWide )
	: Layout(),
	_vgap( vgap ),
	_fitWide( fitWide )
{
}

void StackLayout::performLayout( Panel* panel )
{
	int nextY = 0;

	for ( int i = 0; i < panel->getChildCount(); ++i )
	{
		Panel* child = panel->getChild( i );
		int x;
		int y;
		int wide;
		int tall;

		child->getBounds( x, y, wide, tall );
		child->setPos( x, nextY );
		if ( _fitWide )
		{
			int paintWide;
			int paintTall;

			panel->getPaintSize( paintWide, paintTall );
			child->setSize( paintWide, tall );
		}

		nextY += tall + _vgap;
	}
}

}
