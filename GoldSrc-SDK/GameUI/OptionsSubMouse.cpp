#include "OptionsSubMouse.h"
#include "CvarNegateCheckButton.h"
#include "CvarToggleCheckButton.h"
#include "CvarSlider.h"
#include "GameUI_Interface.h"

#include "TextEntry.h"
#include "vgui/IScheme.h"
#include "vgui/KeyValues.h"
#include "wrect.h"
#include "cl_dll.h"

#include <stdio.h>
#include <stdlib.h>

COptionsSubMouse::COptionsSubMouse( vgui2::Panel* parent ) : BaseClass( parent, NULL, true )
{
	m_pReverseMouseCheckBox = new CCvarNegateCheckButton( this, "ReverseMouse", "#GameUI_ReverseMouse", "m_pitch" );
	m_pLowLatencyCheckBox = new CCvarToggleCheckButton( this, "LowLatency", "#GameUI_LowLatency", "gl_lowlatency" );
	m_pMouseFilterCheckBox = new CCvarToggleCheckButton( this, "MouseFilter", "#GameUI_MouseFilter", "m_filter" );
	m_pMouseSensitivitySlider = new CCvarSlider( this, "Slider", "#GameUI_MouseSensitivity",
			0.2f, 20.0f, "sensitivity", true );
	m_pMouseRawInputCheckBox = new CCvarToggleCheckButton( this, "RawInput", "#GameUI_RawInput", "m_rawinput" );
	m_pMouseSensitivityLabel = new vgui2::TextEntry( this, "SensitivityLabel" );
	m_pMouseSensitivityLabel->AddActionSignalTarget( this );
	m_pAutoAimCheckBox = new CCvarToggleCheckButton( this, "Auto-Aim", "#GameUI_AutoAim", "sv_aim" );
	LoadControlSettings( "Resource\\OptionsSubMouse.res", NULL );

	char buf[64];
#ifdef _WIN32
	_snprintf( buf, sizeof( buf ) - 1, " %.1f", engine->pfnGetCvarFloat( "sensitivity" ) );
#else
	snprintf( buf, sizeof( buf ) - 1, " %.1f", engine->pfnGetCvarFloat( "sensitivity" ) );
#endif
	buf[sizeof( buf ) - 1] = 0;
	m_pMouseSensitivityLabel->SetText( buf );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubMouse::~COptionsSubMouse()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnResetData()
{
	m_pReverseMouseCheckBox->Reset();
	if ( m_pLowLatencyCheckBox )
		m_pLowLatencyCheckBox->Reset();
	m_pMouseFilterCheckBox->Reset();
	m_pMouseSensitivitySlider->Reset();
	m_pMouseRawInputCheckBox->Reset();
	m_pAutoAimCheckBox->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnApplyChanges()
{
	m_pReverseMouseCheckBox->ApplyChanges();
	if ( m_pLowLatencyCheckBox )
		m_pLowLatencyCheckBox->ApplyChanges();
	m_pMouseFilterCheckBox->ApplyChanges();
	m_pMouseSensitivitySlider->ApplyChanges();
	m_pAutoAimCheckBox->ApplyChanges();
	m_pMouseRawInputCheckBox->ApplyChanges();
}

void COptionsSubMouse::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	BaseClass::ApplySchemeSettings( scheme );
//	m_pMouseSensitivityLabel->SetFgColor(GetSchemeColor("WindowDisabledFgColor", GetFgColor()));
//	m_pMouseSensitivityLabel->SetBgColor(GetSchemeColor("WindowDisabledBgColor", GetBgColor()));
	m_pMouseSensitivityLabel->SetBorder( scheme->GetBorder( "ButtonDepressedBorder" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnControlModified( vgui2::Panel* panel )
{
	PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
	if ( panel == m_pMouseSensitivitySlider && m_pMouseSensitivitySlider->HasBeenModified() )
		UpdateSensitivityLabel();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::OnTextChanged( vgui2::Panel* panel )
{
	if ( panel != m_pMouseSensitivityLabel )
		return;

	char buf[64];
	m_pMouseSensitivityLabel->GetText( buf, sizeof( buf ) );
	float value = (float)atof( buf );
	if ( value >= 1.0f )
	{
		m_pMouseSensitivitySlider->SetSliderValue( value );
		PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubMouse::UpdateSensitivityLabel()
{
	char buf[64];
	sprintf( buf, " %.1f", m_pMouseSensitivitySlider->GetSliderValue() );
	m_pMouseSensitivityLabel->SetText( buf );
}
