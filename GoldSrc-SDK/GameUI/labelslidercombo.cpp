#include "labelslidercombo.h"

#include "TextEntry.h"
#include "vgui/KeyValues.h"

#include <wchar.h>

LabelSliderCombo::LabelSliderCombo( vgui2::Panel* parent, const char* sliderName, const char* textEntryName )
	: vgui2::Slider( parent, sliderName )
{
	m_currentValueShown = -1;
	m_pTextEntry = new vgui2::TextEntry( parent, textEntryName );
	m_pTextEntry->SetEnabled( false );
	SetValue( 0, true );
}

LabelSliderCombo::~LabelSliderCombo()
{
}

void LabelSliderCombo::SetVisible( bool state )
{
	vgui2::Slider::SetVisible( state );
	if ( m_pTextEntry )
		m_pTextEntry->SetVisible( state );
}

void LabelSliderCombo::OnCursorMoved( int x, int y )
{
	vgui2::Slider::OnCursorMoved( x, y );
	UpdateTextEntry( true );
}

void LabelSliderCombo::SetValue( int value, bool triggerChangeMessage )
{
	vgui2::Slider::SetValue( value, triggerChangeMessage );
	UpdateTextEntry( triggerChangeMessage );
}

void LabelSliderCombo::UpdateTextEntry( bool triggerChangeMessage )
{
	int value = GetValue();
	if ( value == m_currentValueShown )
		return;

	if ( triggerChangeMessage )
		PostActionSignal( new KeyValues( "ControlModified" ) );
	wchar_t text[64];
	swprintf( text, sizeof( text ) / sizeof( text[0] ), L"%d", value );
	if ( m_pTextEntry )
	{
		m_pTextEntry->SetText( text );
		m_currentValueShown = value;
	}
}
