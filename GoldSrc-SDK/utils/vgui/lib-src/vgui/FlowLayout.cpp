#include "VGUI_FlowLayout.h"
#include "VGUI_Panel.h"

namespace vgui
{

FlowLayout::FlowLayout( int hgap )
	: Layout(),
	_hgap( hgap )
{
}

void FlowLayout::performLayout( Panel* panel )
{
	int xx;

	xx = 0;
	for ( int i = 0; i < panel->getChildCount(); i++ )
	{
		Panel* child;
		int x;
		int y;
		int wide;
		int tall;

		child = panel->getChild( i );
		child->getBounds( x, y, wide, tall );
		child->setPos( xx, y );
		xx += _hgap + wide;
	}
}

}
