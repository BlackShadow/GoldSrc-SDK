#include "VGUI_MenuSeparator.h"
#include "VGUI_Scheme.h"

namespace vgui
{

MenuSeparator::MenuSeparator( const char* text )
	: Label( text )
{
	setFont( Scheme::sf_primary3 );
}

void MenuSeparator::paintBackground()
{
	int wide;
	int tall;
	int textWide;
	int textTall;
	int mid;
	int left;
	int right;

	getPaintSize( wide, tall );
	drawSetColor( Scheme::sc_secondary3 );
	drawFilledRect( 0, 0, wide, tall );

	getTextSize( textWide, textTall );
	if ( textWide > 0 )
	{
		textWide += 4;
	}

	mid = textTall / 2;
	left = wide / 2 - textWide / 2;
	right = textWide / 2 + wide / 2;

	drawSetColor( Scheme::sc_secondary1 );
	drawFilledRect( 2, mid - 1, left, mid );
	drawFilledRect( right, mid - 1, wide - 2, mid );

	drawSetColor( Scheme::sc_white );
	drawFilledRect( 2, mid, left, mid + 1 );
	drawFilledRect( right, mid, wide - 2, mid + 1 );
}

}
