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
// Health.cpp
//
// implementation of CHudHealth class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <math.h>

DECLARE_MESSAGE( m_Health, Health )
DECLARE_MESSAGE( m_Health, Damage )
DECLARE_MESSAGE( m_Health, ReceiveW )
DECLARE_MESSAGE( m_Health, WindInfo )
DECLARE_MESSAGE( m_Health, GrassInfo )

int giDmgHeight;
int giDmgWidth;
int giDmgFlags[NUM_DMG_TYPES] =
{
	DMG_POISON,
	DMG_ACID,
	DMG_FREEZE | DMG_SLOWFREEZE,
	DMG_DROWN,
	DMG_BURN | DMG_SLOWBURN,
	DMG_NERVEGAS,
	DMG_RADIATION,
	DMG_SHOCK,
	DMG_CALTROP,
	DMG_TRANQ,
	DMG_CONCUSS,
	DMG_HALLUC,
};

int CHudHealth::Init()
{
	HOOK_MESSAGE( Health );
	HOOK_MESSAGE( Damage );
	HOOK_MESSAGE( ReceiveW );
	HOOK_MESSAGE( WindInfo );
	HOOK_MESSAGE( GrassInfo );
	m_iHealth = 100;
	m_fFade = 0;
	m_iFlags = 0;
	m_fAttackFront = 0;
	m_fAttackRear = 0;
	m_fAttackLeft = 0;
	m_fAttackRight = 0;
	m_bitsDamage = 0;
	giDmgHeight = 0;
	giDmgWidth = 0;
	memset( m_dmg, 0, sizeof( m_dmg ) );
	gHUD.AddHudElem( this );
	return 1;
}

void CHudHealth::Reset()
{
	m_fAttackFront = 0;
	m_fAttackRear = 0;
	m_fAttackLeft = 0;
	m_fAttackRight = 0;
	for ( int i = 0; i < NUM_DMG_TYPES; ++i )
		m_dmg[i].fExpire = 0;
	m_bitsDamage = 0;
}

int CHudHealth::VidInit()
{
	m_hSprite = 0;
	m_HUD_dmg_bio = gHUD.GetSpriteIndex( "dmg_bio" );
	m_HUD_cross = gHUD.GetSpriteIndex( "cross" );
	wrect_t& rect = gHUD.GetSpriteRect( m_HUD_dmg_bio );
	giDmgHeight = rect.right - rect.left;
	giDmgWidth = rect.bottom - rect.top;
	return 1;
}

int CHudHealth::MsgFunc_Health( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int health = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;

	// Only update the fade if we've changed health
	if ( health != m_iHealth )
	{
		m_fFade = 100;
		m_iHealth = health;
	}
	return 1;
}

int CHudHealth::MsgFunc_Damage( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int armor = READ_BYTE();	// armor
	int damage = READ_BYTE();
	int bits = READ_LONG();
	Vector source;
	source.x = READ_COORD();
	source.y = READ_COORD();
	source.z = READ_COORD();
	UpdateTiles( gHUD.m_flTime, bits );
	if ( damage > 0 || armor > 0 )
		CalcDamageDirection( source );
	return 1;
}

void CHudHealth::GetPainColor( int& r, int& g, int& b )
{
	if ( m_iHealth > 25 )
	{
		r = 64;
		g = 255;
		b = 64;
	}
	else
	{
		r = 250;
		g = 0;
		b = 0;
	}
}

void CHudHealth::CalcDamageDirection( Vector source )
{
	if ( source.x == 0 && source.y == 0 && source.z == 0 )
	{
		m_fAttackFront = 0;
		m_fAttackRear = 0;
		m_fAttackLeft = 0;
		m_fAttackRight = 0;
		return;
	}
	source = source - gHUD.m_vecOrigin;
	float distance = source.Length();
	source = source.Normalize();
	Vector forward, right, up;
	AngleVectors( gHUD.m_vecAngles, forward, right, up );
	if ( distance <= 50 )
	{
		m_fAttackFront = 1;
		m_fAttackRear = 1;
		m_fAttackLeft = 1;
		m_fAttackRight = 1;
		return;
	}
	float front = DotProduct( source, forward );
	float side = DotProduct( source, right );
	if ( front > 0.3 )
	{
		if ( !( front <= m_fAttackFront ) )
			m_fAttackFront = front;
	}
	else if ( -front > 0.3 )
	{
		if ( !( -front <= m_fAttackRear ) )
			m_fAttackRear = -front;
	}
	if ( side > 0.3 )
	{
		if ( !( side <= m_fAttackRight ) )
			m_fAttackRight = side;
	}
	else if ( -side > 0.3 )
	{
		if ( !( -side <= m_fAttackLeft ) )
			m_fAttackLeft = -side;
	}
}

void CHudHealth::UpdateTiles( float time, long bits )
{
	int added = bits & ~m_bitsDamage;
	for ( int i = 0; i < NUM_DMG_TYPES; ++i )
	{
		DAMAGE_IMAGE& damage = m_dmg[i];
		if ( m_bitsDamage & giDmgFlags[i] )
		{
			damage.fExpire = time + DMG_IMAGE_LIFE;
			if ( damage.fBaseline == 0 )
				damage.fBaseline = time;
		}

		// Are we just turning it on?
		if ( added & giDmgFlags[i] )
		{
			damage.x = giDmgWidth / 8;
			damage.y = ScreenHeight - 2 * giDmgHeight;
			damage.fExpire = time + DMG_IMAGE_LIFE;
			for ( int j = 0; j < NUM_DMG_TYPES; ++j )
			{
				if ( i != j && m_dmg[j].y )
					m_dmg[j].y -= giDmgHeight;
			}
		}
	}

	// damage bits are only turned on here;  they are turned off when the draw time has expired (in DrawDamage())
	m_bitsDamage |= bits;
}

int CHudHealth::DrawDamage( float time )
{
	if ( !m_bitsDamage )
		return 1;
	int r = 64, g = 255, b = 64;
	int alpha = (int)( fabs( sin( (double)( time * 2 ) ) ) * 256.0 );
	ScaleColors( r, g, b, alpha );
	for ( int i = 0; i < NUM_DMG_TYPES; ++i )
	{
		if ( ( m_bitsDamage & giDmgFlags[i] ) && m_dmg[i].fExpire >= time )
		{
			SPR_Set( gHUD.GetSprite( m_HUD_dmg_bio + i ), r, g, b );
			SPR_DrawAdditive( 0, m_dmg[i].x, m_dmg[i].y, &gHUD.GetSpriteRect( m_HUD_dmg_bio + i ) );
		}
	}


	// check for bits that should be expired
	for ( int i = 0; i < NUM_DMG_TYPES; ++i )
	{
		if ( !( m_bitsDamage & giDmgFlags[i] ) )
			continue;
		float expire = time + DMG_IMAGE_LIFE;
		if ( !( expire > m_dmg[i].fExpire ) )
			m_dmg[i].fExpire = expire;
		if ( time < m_dmg[i].fExpire || alpha > 39 )
			continue;
		m_dmg[i].fExpire = 0;
		int y = m_dmg[i].y;
		m_dmg[i].x = 0;
		m_dmg[i].y = 0;
		for ( int j = 0; j < NUM_DMG_TYPES; ++j )
		{
			if ( m_dmg[j].y && y > m_dmg[j].y )
				m_dmg[j].y += giDmgHeight;
		}
		m_bitsDamage &= ~giDmgFlags[i];  // clear the bits
	}
	return 1;
}

int CHudHealth::DrawPain( float )
{
	if ( !m_fAttackFront && !m_fAttackRear && !m_fAttackLeft && !m_fAttackRight )
		return 1;
	float fade = (float)( gHUD.m_flTimeDelta + gHUD.m_flTimeDelta );
	float* attacks[] = { &m_fAttackFront, &m_fAttackRight, &m_fAttackRear, &m_fAttackLeft };
	for ( int frame = 0; frame < 4; ++frame )
	{
		float& attack = *attacks[frame];
		bool visible = frame == 1 || frame == 2 ? !( attack <= 0.4 ) : attack > 0.4;
		if ( !visible )
		{
			attack = 0;
			continue;
		}
		int r, g, b;
		GetPainColor( r, g, b );
		int alpha = attack >= 0.5 ? (int)(float)( attack * 255.0 ) : 127;
		ScaleColors( r, g, b, alpha );
		SPR_Set( m_hSprite, r, g, b );
		int width = SPR_Width( m_hSprite, frame );
		int height = SPR_Height( m_hSprite, frame );
		int x = ScreenWidth / 2;
		int y = ScreenHeight / 2;
		switch ( frame )
		{
		case 0:
			x -= width / 2;
			y -= 3 * height;
			break;
		case 1:
			x += 2 * width;
			y -= height / 2;
			break;
		case 2:
			x -= width / 2;
			y += 2 * height;
			break;
		case 3:
			x -= 3 * width;
			y -= height / 2;
			break;
		}
		SPR_DrawAdditive( frame, x, y, NULL );
		attack -= fade;
		if ( attack <= 0 )
			attack = 0;
	}
	return 1;
}

int CHudHealth::Draw( float time )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH ) || gEngfuncs.IsSpectateOnly() )
		return 1;
	if ( !m_hSprite )
		m_hSprite = LoadSprite( "sprites/%d_pain.spr" );
	int alpha = 200;
	if ( m_fFade != 0 )
	{
		float fade = (float)( m_fFade - 20 * gHUD.m_flTimeDelta );
		if ( fade <= 0 )
			m_fFade = 0;
		else
		{
			m_fFade = fade;
			alpha = (int)( fade / 100 * 128 + 200 );
		}
	}

	// If health is getting low, make it bright red
	if ( m_iHealth <= 15 )
		alpha = 255;
	int r, g, b;
	GetPainColor( r, g, b );
	ScaleColors( r, g, b, alpha );
	if ( gHUD.m_iWeaponBits < 0 )
	{
		wrect_t& number = gHUD.GetSpriteRect( gHUD.m_HUD_number_0 );
		wrect_t& cross = gHUD.GetSpriteRect( m_HUD_cross );
		int width = number.right - number.left;
		int crossWidth = cross.right - cross.left;
		int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
		SPR_Set( gHUD.GetSprite( m_HUD_cross ), r, g, b );
		SPR_DrawAdditive( 0, crossWidth / 2, y, &cross );
		int x = gHUD.DrawHudNumber( width / 2 + crossWidth, y, DHN_DRAWZERO | DHN_3DIGITS, m_iHealth, r, g, b );
		FillRGBA( x + width / 2, y, width / 10, gHUD.m_iFontHeight, 64, 255, 64, alpha );
	}
	if ( m_bitsDamage )
		DrawDamage( time );
	return DrawPain( time );
}
