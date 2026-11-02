#include "VGUI_LoweredBorder.h"
#include "VGUI_Panel.h"
#include "VGUI_Scheme.h"

namespace vgui
{

void LoweredBorder::paint( Panel* panel )
{
	int wide;
	int tall;

	panel->getSize( wide, tall );

	drawSetColor( Scheme::sc_secondary2 );
	drawFilledRect( 0, 0, wide, 2 );
	drawFilledRect( 0, 2, 2, tall );

	drawSetColor( Scheme::sc_white );
	drawFilledRect( 1, tall - 2, wide, tall );
	drawFilledRect( wide - 2, 1, wide, tall - 1 );

	drawSetColor( Scheme::sc_secondary1 );
	drawFilledRect( 1, 1, wide - 1, 2 );
	drawFilledRect( 1, 2, 2, tall - 1 );
}

LoweredBorder::LoweredBorder()
{
	Border::setInset( 2, 2, 2, 2 );
}

}
