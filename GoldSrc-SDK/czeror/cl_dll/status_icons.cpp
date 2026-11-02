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
// status_icons.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "cl_entity.h"
#include "event_api.h"

DECLARE_MESSAGE( m_StatusIcons, StatusIcon )

int CHudStatusIcons::Init()
{
	HOOK_MESSAGE( StatusIcon );
	gHUD.AddHudElem( this );
	Reset();
	return 1;
}

int CHudStatusIcons::VidInit()
{
	return 1;
}

void CHudStatusIcons::Reset()
{
	memset( m_IconList, 0, sizeof( m_IconList ) );
	m_iFlags &= ~HUD_ACTIVE;
}

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float )
{
	if ( gEngfuncs.IsSpectateOnly() )
		return 1;
	int y = ScreenHeight / 2;
	for ( int i = 0; i < MAX_ICONSPRITES; ++i )
	{
		icon_sprite_t& icon = m_IconList[i];
		if ( !icon.spr )
			continue;
		y -= icon.rc.bottom - icon.rc.top + 5;
		SPR_Set( icon.spr, icon.r, icon.g, icon.b );
		SPR_DrawAdditive( 0, 5, y, &icon.rc );
	}
	return 1;
}

// Message handler for StatusIcon message
// accepts five values:
//		byte   : TRUE = ENABLE icon, FALSE = DISABLE icon
//		string : the sprite name to display
//		byte   : red
//		byte   : green
//		byte   : blue
int CHudStatusIcons::MsgFunc_StatusIcon( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int enabled = READ_BYTE();
	char* iconName = READ_STRING();
	if ( enabled )
	{
		unsigned char r = (unsigned char)READ_BYTE();
		unsigned char g = (unsigned char)READ_BYTE();
		unsigned char b = (unsigned char)READ_BYTE();
		EnableIcon( iconName, r, g, b );
		m_iFlags |= HUD_ACTIVE;
	}
	else
		DisableIcon( iconName );
	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon( char* name, unsigned char r, unsigned char g, unsigned char b )
{
	int slot;
	for ( slot = 0; slot < MAX_ICONSPRITES; ++slot )
	{
		if ( !_stricmp( m_IconList[slot].szSpriteName, name ) )
			break;
	}
	if ( slot == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for ( slot = 0; slot < MAX_ICONSPRITES; ++slot )
		{
			if ( !m_IconList[slot].spr )
				break;
		}
		if ( slot == MAX_ICONSPRITES )
			slot = 0;
	}
	int sprite = gHUD.GetSpriteIndex( name );
	icon_sprite_t& icon = m_IconList[slot];
	icon.spr = gHUD.GetSprite( sprite );
	if ( sprite >= 0 )
		icon.rc = gHUD.GetSpriteRect( sprite );
	icon.r = r;
	icon.g = g;
	icon.b = b;
	strncpy( icon.szSpriteName, name, sizeof( icon.szSpriteName ) - 1 );
	icon.szSpriteName[sizeof( icon.szSpriteName ) - 1] = 0;
	if ( strstr( icon.szSpriteName, "grenade" ) )
	{
		cl_entity_t* player = gEngfuncs.GetLocalPlayer();
		gEngfuncs.pEventAPI->EV_PlaySound( player->index, player->origin, CHAN_STATIC,
			"weapons/timer.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	}
}

void CHudStatusIcons::DisableIcon( char* name )
{
	// find the sprite is in the current list
	for ( int i = 0; i < MAX_ICONSPRITES; ++i )
	{
		if ( !_stricmp( m_IconList[i].szSpriteName, name ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof( m_IconList[i] ) );
			return;
		}
	}
}
