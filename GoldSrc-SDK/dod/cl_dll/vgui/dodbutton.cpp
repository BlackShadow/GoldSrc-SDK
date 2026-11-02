#include "dodbutton.h"
#include "vgui/KeyValues.h"

using namespace vgui2;

DoDButton::DoDButton( Panel* parent, const char* panelName, const char* text ) :
	BaseClass( parent, panelName, text ), m_iMouseOverCode( -1 )
{
}

DoDButton::DoDButton( Panel* parent, const char* panelName, const wchar_t* text ) :
	BaseClass( parent, panelName, text ), m_iMouseOverCode( -1 )
{
}

void DoDButton::SetDefaultColor( Color foreground, Color background )
{
	if ( m_cDefaultFgColor != foreground || m_cDefaultBgColor != background )
	{
		m_cDefaultFgColor = foreground;
		m_cDefaultBgColor = background;
		InvalidateLayout( false, false );
	}
}

void DoDButton::SetArmedColor( Color foreground, Color background )
{
	if ( m_cArmedFgColor != foreground || m_cArmedBgColor != background )
	{
		m_cArmedFgColor = foreground;
		m_cArmedBgColor = background;
		InvalidateLayout( false, false );
	}
}

void DoDButton::SetDepressedColor( Color foreground, Color background )
{
	if ( m_cDepressedFgColor != foreground || m_cDepressedBgColor != background )
	{
		m_cDepressedFgColor = foreground;
		m_cDepressedBgColor = background;
		InvalidateLayout( false, false );
	}
}

Color DoDButton::GetButtonFgColor()
{
	return IsDepressed() ? m_cDepressedFgColor : ( IsArmed() ? m_cArmedFgColor : m_cDefaultFgColor );
}

Color DoDButton::GetButtonBgColor()
{
	return IsDepressed() ? m_cDepressedBgColor : ( IsArmed() ? m_cArmedBgColor : m_cDefaultBgColor );
}

void DoDButton::ApplySchemeSettings( IScheme* scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
	m_cDefaultFgColor = GetFgColor();
	m_cDefaultBgColor = GetBgColor();
	m_cArmedFgColor = GetSchemeColor( "ButtonArmedFgColor", m_cDefaultFgColor, scheme );
	m_cArmedBgColor = GetSchemeColor( "ButtonArmedBgColor", m_cDefaultBgColor, scheme );
	m_cDepressedFgColor = GetSchemeColor( "ButtonDepressedFgColor", m_cDefaultFgColor, scheme );
	m_cDepressedBgColor = GetSchemeColor( "ButtonDepressedBgColor", m_cDefaultBgColor, scheme );
	SetKeyFocusBorder( NULL );
}

void DoDButton::OnCursorEntered()
{
	BaseClass::OnCursorEntered();
	PostActionSignal( new KeyValues( "MouseOver", "code", m_iMouseOverCode ) );
}
