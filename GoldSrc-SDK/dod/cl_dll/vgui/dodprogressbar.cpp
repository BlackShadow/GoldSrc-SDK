#include "dodprogressbar.h"

using namespace vgui2;

DoDProgressBar::DoDProgressBar( Panel* parent, const char* panelName ) : BaseClass( parent, panelName )
{
}

void DoDProgressBar::ApplySchemeSettings( IScheme* scheme )
{
	Panel::ApplySchemeSettings( scheme );
	SetFgColor( GetSchemeColor( "ClassMenuLight", scheme ) );
	SetBgColor( GetSchemeColor( "ClassMenuDark", scheme ) );
	SetBarInset( 0 );
	SetSegmentInfo( 0, 1 );
	SetBorder( NULL );
}
