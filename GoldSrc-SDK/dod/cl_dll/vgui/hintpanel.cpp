#include "hud.h"
#include "hintpanel.h"
#include "ImagePanel.h"
#include "Label.h"
#include "vgui/IScheme.h"
#include <wchar.h>

using namespace vgui2;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CHintPanel::CHintPanel( Panel* parent ) : BaseClass( parent, "HintPanel", true )
{
	MakePopup( true, false );
	SetVisible( false );
	SetTitle( "", true );
	SetScheme( "ClientScheme" );
	SetMoveable( false );
	SetSizeable( false );
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetTitleBarVisible( false );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/HintPanel.res", NULL );
	InvalidateLayout( false, false );
	m_fShowTime = -1;
}

void CHintPanel::Paint()
{
}

void CHintPanel::ApplySchemeSettings( IScheme* scheme )
{
	SetBorder( scheme->GetBorder( "HintBorder" ) );
	SetBgColor( scheme->GetColor( "HintBackgroundColor", Color( 0, 0, 0, 128 ) ) );
	Label* label = dynamic_cast<Label*>( FindChildByName( "hintlabel", false ) );
	if ( label )
		label->SetFgColor( Color( 255, 255, 255, 255 ) );
}

void CHintPanel::OnTick()
{
	if ( m_fShowTime > 0 && IsVisible() && gHUD.m_flTime >= m_fShowTime )
	{
		m_fShowTime = -1;
		SetVisible( false );
	}
}

void CHintPanel::SetVisibleImagePanel( const char* name, bool visible )
{
	ImagePanel* image = dynamic_cast<ImagePanel*>( FindChildByName( name, false ) );
	if ( image )
		image->SetVisible( visible );
}

void CHintPanel::ShowHintMessage( int team, const char* text )
{
	char* localized = CHudTextMessage::BufferedLocaliseTextString( text );
	SetVisibleImagePanel( "britimage", false );
	SetVisibleImagePanel( "gerimage", false );
	SetVisibleImagePanel( "amerimage", false );
	SetVisibleImagePanel( team == 0 ? "britimage" : ( team == 2 ? "gerimage" : "amerimage" ), true );
	Label* label = dynamic_cast<Label*>( FindChildByName( "hintlabel", false ) );
	if ( label )
	{
		label->SetText( localized );
		wchar_t labelText[256];
		label->GetText( labelText, 256 );
		m_fShowTime = gHUD.m_flTime + (float)wcslen( labelText ) / 20;
	}
	SetVisible( true );
}
