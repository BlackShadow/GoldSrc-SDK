#include "hud.h"
#include "mortarpanel.h"
#include "Label.h"
#include "controls.h"
#include "../../../engine/controls.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include <stdio.h>

using namespace vgui2;

extern bool IsAttackPressed();

CMortarPanel::CMortarPanel( Panel* parent ) : BaseClass( parent, "MortarPanel", true )
{
	MakePopup( true, false );
	SetVisible( false );
	SetTitle( "", true );
	SetScheme( "ClientScheme" );
	SetMoveable( true );
	SetSizeable( true );
	SetMouseInputEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetTitleBarVisible( false );
	SetProportional( true );
	LoadControlSettings( "Resource/UI/MortarPanel.res", NULL );
	InvalidateLayout( false, false );
	m_fShowTime = -1;
	m_bAttack = false;
	m_fAimValue = 0;
	m_fAimVelocity = 0;
	m_iNumPasses = 0;
	m_iTexture = surface()->CreateNewTextureID( false );
	surface()->DrawSetTextureFile( m_iTexture, "gfx/vgui/white", false, false );
}

void CMortarPanel::ShowMortarPanel()
{
	SetVisible( true );
}

void CMortarPanel::HideMortarPanel()
{
	m_bAttack = false;
	SetVisible( false );
}

int CMortarPanel::KeyInput( int, int, const char* )
{
	return 1;
}

void CMortarPanel::ApplySchemeSettings( IScheme* scheme )
{
	SetBorder( scheme->GetBorder( "HintBorder" ) );
	SetBgColor( scheme->GetColor( "HintBackgroundColor", Color( 0, 0, 0, 128 ) ) );
	Label* label = dynamic_cast<Label*>( FindChildByName( "hintlabel", false ) );
	if ( label )
		label->SetFgColor( Color( 255, 255, 255, 255 ) );
}

void CMortarPanel::DrawRect( int x, int y, int wide, int tall )
{
	VGuiVertex vertices[4] =
	{
		{ x, y, 0, 0 },
		{ x, y + tall, 0, 1 },
		{ x + wide, y + tall, 1, 1 },
		{ x + wide, y, 1, 0 }
	};
	surface()->DrawTexturedPolygon( vertices, 4 );
}

void CMortarPanel::Paint()
{
	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );
	surface()->DrawSetTextColor( 0, 0, 255, 128 );
	surface()->DrawSetColor( 0, 0, 255, 128 );
	surface()->DrawSetTexture( m_iTexture );
	const int position = (int)( m_fAimValue / 1000 * 400 );
	DrawRect( 10, 50, 400, 20 );
	surface()->DrawSetColor( 255, 0, 0, 128 );
	DrawRect( 150, 50, 40, 20 );
	surface()->DrawSetColor( 255, 0, 0, 255 );
	DrawRect( 190, 50, 40, 20 );
	surface()->DrawSetColor( 255, 0, 0, 128 );
	DrawRect( 230, 50, 40, 20 );
	surface()->DrawSetColor( 255, 255, 0, 255 );
	DrawRect( 208, 50, 4, 20 );
	surface()->DrawSetColor( 255, 255, 255, 255 );
	DrawRect( position + 9, 51, 2, 18 );
}

void CMortarPanel::AttackDown()
{
	if ( m_bAttack )
		return;
	Label* label = dynamic_cast<Label*>( FindChildByName( "mortarlabel", false ) );
	if ( label )
	{
		char text[268];
		wchar_t wideText[256];
		sprintf( text, "Button Down! %f\n", m_fAimValue );
		localize()->ConvertANSIToUnicode( text, wideText, sizeof( wideText ) );
		label->SetText( wideText );
	}
	m_fAimValue = gEngfuncs.pfnRandomLong( 0, 1000 );
	m_fAimVelocity = 2500;
	m_iNumPasses = 0;
	if ( m_fAimValue > 500 )
		m_fAimVelocity = -2500;
	m_bAttack = true;
}

void CMortarPanel::AttackUp()
{
	if ( !m_bAttack )
		return;
	Label* label = dynamic_cast<Label*>( FindChildByName( "mortarlabel", false ) );
	if ( label )
	{
		char text[256];
		wchar_t wideText[256];
		char command[28];
		sprintf( text, "Button UP!   %.0f\n", m_fAimValue );
		localize()->ConvertANSIToUnicode( text, wideText, sizeof( wideText ) );
		label->SetText( wideText );
		sprintf( command, "maim %i", (unsigned char)(int)( m_fAimValue / 1000 * 256 ) );
		gEngfuncs.pfnServerCmd( command );
	}
	m_bAttack = false;
}

void CMortarPanel::OnTick()
{
	const double frametime = gHUD.m_flTimeDelta;
	const bool attackPressed = IsAttackPressed();
	if ( !m_bAttack )
		return;
	if ( !attackPressed )
	{
		AttackUp();
		if ( !m_bAttack )
			return;
	}
	m_fAimValue += frametime * m_fAimVelocity;
	if ( m_fAimVelocity > 0 && m_fAimValue > 1000 )
	{
		m_fAimValue = 2000 - m_fAimValue;
		m_fAimVelocity = -m_fAimVelocity;
		++m_iNumPasses;
		if ( m_iNumPasses <= 5 )
			m_fAimVelocity *= 0.9;
	}
	else if ( m_fAimVelocity < 0 && m_fAimValue < 0 )
	{
		m_fAimValue = -m_fAimValue;
		m_fAimVelocity = -m_fAimVelocity;
		if ( m_iNumPasses <= 5 )
			m_fAimVelocity *= 0.9;
	}
	Label* label = dynamic_cast<Label*>( FindChildByName( "mortarlabel", false ) );
	if ( label )
	{
		char text[256];
		wchar_t wideText[256];
		sprintf( text, "Button Down! %.0f\n", m_fAimValue );
		localize()->ConvertANSIToUnicode( text, wideText, sizeof( wideText ) );
		label->SetText( wideText );
	}
}

void CMortarPanel::PostMortarValue( float value )
{
	if ( value > 0 )
		AttackDown();
	if ( value < 0 )
	{
		m_bAttack = false;
		SetVisible( false );
	}
}
