#include "OptionsSubVoice.h"
#include "CvarSlider.h"
#include "CvarToggleCheckButton.h"
#include "GameUI_Interface.h"

#include "Button.h"
#include "CheckButton.h"
#include "ImagePanel.h"
#include "Label.h"
#include "Slider.h"
#include "vgui/KeyValues.h"
#include "wrect.h"
#include "cl_dll.h"

#include <string.h>

#ifndef _WIN32
#include <strings.h>
#define stricmp strcasecmp
#endif

#ifdef _MSC_VER
#pragma warning( suppress: 26495 )
#endif
COptionsSubVoice::COptionsSubVoice( vgui2::Panel* parent ) : vgui2::PropertyPage( parent, NULL, true )
{
	m_pVoiceTweak = engine->pVoiceTweak;
	m_pMicMeter = new vgui2::ImagePanel( this, "MicMeter" );
	m_pMicMeter2 = new vgui2::ImagePanel( this, "MicMeter2" );
	m_pReceiveSliderLabel = new vgui2::Label( this, "ReceiveLabel", "#GameUI_VoiceReceiveVolume" );
	m_pReceiveVolume = new CCvarSlider( this, "VoiceReceive", "#GameUI_ReceiveVolume",
			0.0f, 1.0f, "voice_scale", false );
	m_pMicrophoneSliderLabel = new vgui2::Label( this, "MicrophoneLabel", "#GameUI_VoiceTransmitVolume" );
	m_pMicrophoneVolume = new vgui2::Slider( this, "VoiceVolume" );
	m_pMicrophoneVolume->SetRange( 0, 100 );
	m_pMicrophoneVolume->AddActionSignalTarget( this );
	m_pVoiceEnableCheckButton = new CCvarToggleCheckButton( this, "voice_modenable",
			"#GameUI_EnableVoice", "voice_modenable" );
	m_pMicBoost = new vgui2::CheckButton( this, "MicBoost", "#GameUI_BoostMicrophone" );
	m_pMicBoost->AddActionSignalTarget( this );
	m_pTestMicrophoneButton = new vgui2::Button( this, "TestMicrophone", "#GameUI_TestMicrophone" );
	LoadControlSettings( "Resource\\OptionsSubVoice.res", NULL );

	int imageWide, imageTall;
	m_pMicMeter2->GetImageSize( imageWide, imageTall );
	m_pMicMeter2->SetImageSize( GetProportionalScaledValue( imageWide ), GetProportionalScaledValue( imageTall ) );
	m_bVoiceOn = false;
	m_pMicMeter2->SetVisible( false );
	// no voice tweak - then disable all buttons
	if ( m_pVoiceTweak )
	{
		OnResetData();
	}
	else
	{
		m_pReceiveVolume->SetEnabled( false );
		m_pMicrophoneVolume->SetEnabled( false );
		m_pVoiceEnableCheckButton->SetEnabled( false );
		m_pMicBoost->SetEnabled( false );
		m_pTestMicrophoneButton->SetEnabled( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
COptionsSubVoice::~COptionsSubVoice()
{
	if ( m_bVoiceOn )
		EndTestMicrophone();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnResetData()
{
	if ( !m_pVoiceTweak )
		return;
//	m_pVoiceTweak->SetControlFloat(OtherSpeakerScale, engine->pfnGetCvarFloat( "voice_scale" ) );

	float volume = m_pVoiceTweak->GetControlFloat( MicrophoneVolume ) * 100.0f;
	m_pMicrophoneVolume->SetValue( (int)volume, true );
	m_nMicVolumeValue = m_pMicrophoneVolume->GetValue();
	m_pMicBoost->SetSelected( m_pVoiceTweak->GetControlFloat( MicBoost ) != 0.0f );
	m_bMicBoostSelected = m_pMicBoost->IsSelected();
	m_pReceiveVolume->Reset();
	m_fReceiveVolume = m_pReceiveVolume->GetSliderValue();
	m_pVoiceEnableCheckButton->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnSliderMoved( int )
{
	if ( m_pVoiceTweak && m_pMicrophoneVolume->GetValue() != m_nMicVolumeValue )
		PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnCheckButtonChecked( int )
{
	if ( m_pVoiceTweak && m_pMicBoost->IsSelected() != m_bMicBoostSelected )
		PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnControlModified()
{
	PostActionSignal( new KeyValues( "ApplyButtonEnable" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnApplyChanges()
{
	if ( !m_pVoiceTweak )
		return;

	m_nMicVolumeValue = m_pMicrophoneVolume->GetValue();
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, m_nMicVolumeValue / 100.0f );
	m_bMicBoostSelected = m_pMicBoost->IsSelected();
	m_pVoiceTweak->SetControlFloat( MicBoost, m_bMicBoostSelected ? 1.0f : 0.0f );
	m_pReceiveVolume->ApplyChanges();
	m_fReceiveVolume = m_pReceiveVolume->GetSliderValue();
	m_pVoiceEnableCheckButton->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::UseCurrentVoiceParameters()
{
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, m_pMicrophoneVolume->GetValue() / 100.0f );
	m_pVoiceTweak->SetControlFloat( MicBoost, m_pMicBoost->IsSelected() ? 1.0f : 0.0f );
	// get where the current slider is
	m_nReceiveSliderValue = m_pReceiveVolume->GetValue();
	m_pReceiveVolume->ApplyChanges();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::StartTestMicrophone()
{
	if ( !m_pVoiceTweak || m_bVoiceOn )
		return;

	m_bVoiceOn = true;
	UseCurrentVoiceParameters();
	if ( m_pVoiceTweak->StartVoiceTweakMode() )
	{
		m_pTestMicrophoneButton->SetText( "#GameUI_StopTestMicrophone" );
		m_pReceiveVolume->SetEnabled( false );
		m_pMicrophoneVolume->SetEnabled( false );
		m_pVoiceEnableCheckButton->SetEnabled( false );
		m_pMicBoost->SetEnabled( false );
		m_pMicrophoneSliderLabel->SetEnabled( false );
		m_pReceiveSliderLabel->SetEnabled( false );
		m_pMicMeter2->SetVisible( true );
	}
	else
	{
		ResetVoiceParameters();
		m_bVoiceOn = false;
	}
}

void COptionsSubVoice::ResetVoiceParameters()
{
	m_pVoiceTweak->SetControlFloat( MicrophoneVolume, m_nMicVolumeValue / 100.0f );
	m_pVoiceTweak->SetControlFloat( MicBoost, m_bMicBoostSelected ? 1.0f : 0.0f );
	engine->Cvar_SetValue( "voice_scale", m_fReceiveVolume );
	m_pReceiveVolume->Reset();
	m_pReceiveVolume->SetValue( m_nReceiveSliderValue, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::EndTestMicrophone()
{
	if ( !m_pVoiceTweak || !m_bVoiceOn )
		return;

	m_pVoiceTweak->EndVoiceTweakMode();
	ResetVoiceParameters();
	m_pTestMicrophoneButton->SetText( "#GameUI_TestMicrophone" );
	m_bVoiceOn = false;
	m_pReceiveVolume->SetEnabled( true );
	m_pMicrophoneVolume->SetEnabled( true );
	m_pVoiceEnableCheckButton->SetEnabled( true );
	m_pMicBoost->SetEnabled( true );
	m_pMicrophoneSliderLabel->SetEnabled( true );
	m_pReceiveSliderLabel->SetEnabled( true );
	m_pMicMeter2->SetVisible( false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnCommand( const char* command )
{
	if ( stricmp( command, "TestMicrophone" ) )
	{
		BaseClass::OnCommand( command );
		return;
	}

	if ( m_bVoiceOn )
		EndTestMicrophone();
	else
		StartTestMicrophone();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnPageHide()
{
	if ( m_bVoiceOn )
		EndTestMicrophone();

	BaseClass::OnPageHide();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void COptionsSubVoice::OnThink()
{
	BaseClass::OnThink();
	if ( !m_bVoiceOn )
		return;

	int level = GetProportionalScaledValue( 160 * m_pVoiceTweak->GetSpeakingVolume() / 32768 );
	int step = GetProportionalScaledValue( 8 );
	int width = ( level + step - GetProportionalScaledValue( 1 ) ) / step * step;
	int wide, tall;
	m_pMicMeter2->GetSize( wide, tall );
	m_pMicMeter2->SetSize( width, tall );
	m_pMicMeter2->Repaint();
}
