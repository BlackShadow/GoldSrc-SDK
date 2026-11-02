#include "BitmapButton.h"

#include "controls.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"

#include <stdlib.h>

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 26495 )
#endif
CBitmapButton::CBitmapButton( vgui2::Panel* parent, const char* panelName, const char* text,
	const char* normalImage, const char* armedImage, const char* clickedImage, const char* disabledImage )
	: vgui2::Button( parent, panelName, text )
{
	const char* images[4] = { normalImage, armedImage, clickedImage, disabledImage ? disabledImage : normalImage };
	for ( int i = 0; i < 4; i++ )
	{
		bimage_t& image = m_ImageID[i];
		image.imageID = vgui2::surface()->CreateNewTextureID();
		vgui2::surface()->DrawSetTextureFile( image.imageID, images[i] ? images[i] : "", 1, false );
		vgui2::surface()->DrawGetTextureSize( image.imageID, image.width, image.height );
	}

	m_shouldFlash = false;
	m_isFlashColor = false;
	m_transitionsLeft = 0;
	m_totalTransitions = 0;
	m_lastTransition = 0.0f;
}
#ifdef _MSC_VER
#pragma warning( pop )
#endif

CBitmapButton::~CBitmapButton()
{
}

void CBitmapButton::Flash()
{
	m_shouldFlash = true;
	m_transitionsLeft = -1;
}

void CBitmapButton::PaintBackground()
{
	int state;
	if ( !IsEnabled() )
		state = 3;
	else if ( IsDepressed() )
		state = 2;
	else
		state = IsArmed() ? 1 : 0;

	if ( m_shouldFlash && m_isFlashColor && state == 0 )
		state = 1;

	int wide, tall;
	GetSize( wide, tall );
	vgui2::surface()->DrawSetColor( 255, 255, 255, 255 );
	vgui2::surface()->DrawSetTexture( m_ImageID[state].imageID );
	vgui2::surface()->DrawTexturedRect( 0, 0, wide, tall );
}

void CBitmapButton::OnThink()
{
	if ( !m_shouldFlash )
		return;

	if ( m_transitionsLeft < 0 && m_totalTransitions )
		m_transitionsLeft = m_totalTransitions;

	float now = (float)vgui2::system()->GetCurrentTime();
	if ( now >= m_lastTransition + m_interval && m_transitionsLeft > 0 )
	{
		m_isFlashColor = !m_isFlashColor;
		m_lastTransition = now;
		m_transitionsLeft--;
	}
	else if ( m_transitionsLeft <= 0 )
	{
		m_isFlashColor = false;
	}
}

void CBitmapButton::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Button::ApplySchemeSettings( scheme );
	SetBorder( NULL );
	SetDefaultBorder( NULL );
	SetDepressedBorder( NULL );
	SetKeyFocusBorder( NULL );
	m_interval = (float)atof( scheme->GetResourceString( "JumpButtonFlashPeriod" ) );
	m_totalTransitions = atoi( scheme->GetResourceString( "JumpButtonNumFlashes" ) );
}

void CBitmapButton::DrawFocusBorder( int, int, int, int )
{
}
