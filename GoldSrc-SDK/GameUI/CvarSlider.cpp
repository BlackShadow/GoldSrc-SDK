#include "CvarSlider.h"

#include "GameUI_Interface.h"
#include "wrect.h"
#include "cl_dll.h"
#include "vgui/KeyValues.h"

#include <stdio.h>
#include <string.h>

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCvarSlider::CCvarSlider( vgui2::Panel* parent, const char* panelName, const char*, float minValue,
		float maxValue, const char* cvarName, bool allowOutOfRange ) : BaseClass( parent, panelName )
{
	AddActionSignalTarget( this );
	float min = minValue * 100.0f;
	float max = maxValue * 100.0f;
	SetRange( (int)min, (int)max );

	char minText[32];
	char maxText[32];
	sprintf( minText, "%.2f", minValue );
	sprintf( maxText, "%.2f", maxValue );
	SetTickCaptions( minText, maxText );
	strcpy( m_szCvarName, cvarName );
	m_bModifiedOnce = false;
	m_bAllowOutOfRange = allowOutOfRange;
	m_fStartValue = engine->pfnGetCvarFloat( m_szCvarName );
	m_fCurrentValue = m_fStartValue;
	float sliderValue = m_fStartValue * 100.0f;
	SetValue( (int)sliderValue, true );
	m_iStartValue = GetValue();
	m_iLastSliderValue = m_iStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CCvarSlider::~CCvarSlider()
{
}

void CCvarSlider::Paint()
{
	float value = engine->pfnGetCvarFloat( m_szCvarName );
	if ( value != m_fStartValue )
	{
		m_fStartValue = value;
		m_fCurrentValue = value;
		float sliderValue = value * 100.0f;
		SetValue( (int)sliderValue, true );
		m_iStartValue = GetValue();
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : position - 
//-----------------------------------------------------------------------------
void CCvarSlider::OnSliderMoved()
{
	if ( !HasBeenModified() )
		return;

	if ( m_iLastSliderValue != GetValue() )
	{
		m_iLastSliderValue = GetValue();
		m_fCurrentValue = (float)m_iLastSliderValue / 100.0f;
	}

	// tell parent that we've been modified
	PostActionSignal( new KeyValues( "ControlModified" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::ApplyChanges()
{
	if ( !m_bModifiedOnce )
		return;

	m_iStartValue = GetValue();
	if ( m_bAllowOutOfRange )
		m_fStartValue = m_fCurrentValue;
	else
		m_fStartValue = (float)m_iStartValue / 100.0f;

	engine->Cvar_SetValue( m_szCvarName, m_fStartValue );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
float CCvarSlider::GetSliderValue()
{
	if ( m_bAllowOutOfRange )
		return m_fCurrentValue;

	return (float)GetValue() / 100.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCvarSlider::SetSliderValue( float value )
{
	float sliderValue = value * 100.0f;
	SetValue( (int)sliderValue, false );
	// remember this slider value
	m_iLastSliderValue = GetValue();
	if ( value != m_fCurrentValue )
	{
		m_bModifiedOnce = true;
		m_fCurrentValue = value;
	}
}

void CCvarSlider::Reset()
{
	m_fStartValue = engine->pfnGetCvarFloat( m_szCvarName );
	m_fCurrentValue = m_fStartValue;
	float sliderValue = m_fStartValue * 100.0f;
	SetValue( (int)sliderValue, true );
	m_iStartValue = GetValue();
	m_iLastSliderValue = m_iStartValue;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CCvarSlider::HasBeenModified()
{
	if ( GetValue() != m_iStartValue )
		m_bModifiedOnce = true;

	return m_bModifiedOnce;
}
