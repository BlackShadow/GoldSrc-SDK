#include "VGUI_BorderLayout.h"
#include "VGUI_Panel.h"

namespace vgui
{

LayoutInfo* BorderLayout::createLayoutInfo( Alignment alignment )
{
	BorderLayoutInfo* info = new BorderLayoutInfo;
	info->_alignment = alignment;
	return info;
}

void BorderLayout::performLayout( Panel* panel )
{
	int wide;
	int tall;
	int maxNorth;
	int maxSouth;
	int maxEast;
	int maxWest;
	int x0;
	int y0;
	int x1;
	int y1;

	panel->getSize( wide, tall );

	maxNorth = 0;
	maxSouth = 0;
	maxEast = 0;
	maxWest = 0;

	for ( int i = 0; i < panel->getChildCount(); i++ )
	{
		BorderLayoutInfo* info = dynamic_cast<BorderLayoutInfo*>( panel->getChild( i )->getLayoutInfo() );

		if ( info != NULL )
		{
			Panel* child = panel->getChild( i );

			switch ( info->_alignment )
			{
			case a_north:
				if ( child->getTall() > maxNorth )
				{
					maxNorth = child->getTall();
				}
				break;
			case a_south:
				if ( child->getTall() > maxSouth )
				{
					maxSouth = child->getTall();
				}
				break;
			case a_east:
				if ( child->getTall() > maxEast )
				{
					maxEast = child->getWide();
				}
				break;
			case a_west:
				if ( child->getWide() > maxWest )
				{
					maxWest = child->getWide();
				}
				break;
			default:
				break;
			}
		}
	}

	x0 = _inset + maxWest;
	y0 = _inset + maxNorth;
	x1 = wide - _inset - maxEast;
	y1 = tall - _inset - maxSouth;

	for ( int i = 0; i < panel->getChildCount(); i++ )
	{
		BorderLayoutInfo* info = dynamic_cast<BorderLayoutInfo*>( panel->getChild( i )->getLayoutInfo() );

		if ( info != NULL )
		{
			Panel* child = panel->getChild( i );

			switch ( info->_alignment )
			{
			case a_center:
				child->setBounds( x0, y0, x1 - x0, y1 - y0 );
				break;
			case a_north:
				child->setBounds( 0, 0, wide, y0 );
				break;
			case a_south:
				child->setBounds( 0, y1, wide, tall - y1 );
				break;
			case a_east:
				child->setBounds( x1, y0, wide - x1, y1 - y0 );
				break;
			case a_west:
				child->setBounds( 0, y0, x0, y1 - y0 );
				break;
			default:
				break;
			}
		}
	}
}

BorderLayout::BorderLayout( int inset )
{
	_inset = inset;
}

LayoutInfo* BorderLayoutInfo::getThis()
{
	return this;
}

}
