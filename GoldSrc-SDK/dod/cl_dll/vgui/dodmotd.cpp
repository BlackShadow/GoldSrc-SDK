#include "dodmotd.h"
#include "dodbutton.h"
#include "idodviewport.h"
#include "game_controls/iviewport.h"
#include <string.h>

using namespace vgui2;

CDoDMOTD::CDoDMOTD( Panel* parent ) : BaseClass( parent )
{
	LoadControlSettings( "Resource/UI/MOTD.res", NULL );
	InvalidateLayout( false, false );
}

void CDoDMOTD::OnCommand( const char* command )
{
	gViewPortInterface->HideBackGround();
	BaseClass::OnCommand( command );
}

void CDoDMOTD::Paint()
{
	gDoDViewPortInterface->PaintBackgroundPanel();
	Panel::Paint();
}

Panel* CDoDMOTD::CreateControlByName( const char* controlName )
{
	if ( stricmp( "DoDButton", controlName ) )
		return EditablePanel::CreateControlByName( controlName );
	DoDButton* button = new DoDButton( this, NULL, "" );
	button->MakeReadyForUse();
	button->SetDefaultColor( Color( 255, 255, 255, 255 ), Color( 55, 43, 29, 255 ) );
	button->SetKeyFocusBorder( NULL );
	return button;
}
