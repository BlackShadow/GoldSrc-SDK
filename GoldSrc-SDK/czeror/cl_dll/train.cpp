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
// Train.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE( m_Train, Train )

int CHudTrain::Init()
{
	HOOK_MESSAGE( Train );
	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem( this );
	return 1;
}

int CHudTrain::VidInit()
{
	m_hSprite = 0;
	return 1;
}

int CHudTrain::MsgFunc_Train( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );

	// update Train data
	m_iPos = READ_BYTE();
	if ( m_iPos )
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;
	return 1;
}

int CHudTrain::Draw( float )
{
	if ( !m_hSprite )
		m_hSprite = LoadSprite( "sprites/%d_train.spr" );
	if ( m_iPos )
	{
		SPR_Set( m_hSprite, 64, 255, 64 );
		int y = ScreenHeight - SPR_Height( m_hSprite, 0 ) - gHUD.m_iFontHeight;
		int x = ScreenWidth / 3 + SPR_Width( m_hSprite, 0 ) / 4;
		SPR_DrawAdditive( m_iPos - 1, x, y, NULL );
	}
	return 1;
}
