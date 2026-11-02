/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Battery, Battery )

int CHudBattery::Init()
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	HOOK_MESSAGE( Battery );
	gHUD.AddHudElem( this );
	return 1;
}

int CHudBattery::VidInit()
{
	int empty = gHUD.GetSpriteIndex( "suit_empty" );
	int full = gHUD.GetSpriteIndex( "suit_full" );
	m_hSprite1 = 0;
	m_hSprite2 = 0;
	m_prc1 = &gHUD.GetSpriteRect( empty );
	m_prc2 = &gHUD.GetSpriteRect( full );
	m_fFade = 0;
	m_iHeight = m_prc2->bottom - m_prc1->top;
	return 1;
}

int CHudBattery::MsgFunc_Battery( const char*, int size, void* buffer )
{
	m_iFlags |= HUD_ACTIVE;
	BEGIN_READ( buffer, size );
	int battery = READ_SHORT();
	if ( battery != m_iBat )
	{
		m_fFade = 100;
		m_iBat = battery;
	}
	return 1;
}

int CHudBattery::Draw( float )
{
	if ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH )
		return 1;
	wrect_t rect = *m_prc2;
	double missing = m_iBat > 99 ? 0.0 : ( 100 - m_iBat ) * 0.01;
	rect.top = (int)( rect.top + m_iHeight * missing );
	int r, g, b;
	if ( m_iBat > 25 )
	{
		r = 64;
		g = 255;
		b = 64;
	}
	else
	{
		r = 255;
		g = 16;
		b = 16;
	}
	if ( gHUD.m_iWeaponBits >= 0 )
		return 1;
	int alpha = 200;
	if ( m_fFade != 0 )
	{
		if ( m_fFade > 100 )
			m_fFade = 100;
		float fade = (float)( m_fFade - 20 * gHUD.m_flTimeDelta );
		if ( fade <= 0 )
			m_fFade = 0;
		else
		{
			m_fFade = fade;
			alpha = (int)( fade / 100 * 128 + 200 );
		}
	}
	ScaleColors( r, g, b, alpha );
	int offset = ( m_prc1->bottom - m_prc1->top ) / 6 - 4;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	int x = ScreenWidth / 5;

	// make sure we have the right sprite handles
	if ( !m_hSprite1 )
		m_hSprite1 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_empty" ) );
	if ( !m_hSprite2 )
		m_hSprite2 = gHUD.GetSprite( gHUD.GetSpriteIndex( "suit_full" ) );
	SPR_Set( m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0, x, y - offset, m_prc1 );
	if ( rect.bottom > rect.top )
	{
		SPR_Set( m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, x, rect.top + y - offset - m_prc2->top, &rect );
	}
	gHUD.DrawHudNumber( m_prc1->right + x - m_prc1->left, y, DHN_DRAWZERO | DHN_3DIGITS, m_iBat, r, g, b );
	return 1;
}
