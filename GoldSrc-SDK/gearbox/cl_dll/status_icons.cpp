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
#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"
#include "event_api.h"

DECLARE_MESSAGE( m_StatusIcons, StatusIcon );
DECLARE_MESSAGE( m_StatusIcons, CustomIcon );

int CHudStatusIcons::Init( void )
{
	HOOK_MESSAGE( StatusIcon );
	HOOK_MESSAGE( CustomIcon );

	gHUD.AddHudElem( this );

	Reset();

	return 1;
}

int CHudStatusIcons::VidInit( void )
{

	return 1;
}

void CHudStatusIcons::Reset( void )
{
	memset( m_IconList, 0, sizeof m_IconList );
	memset( m_CustomList, 0, sizeof m_CustomList );
	m_iFlags &= ~HUD_ACTIVE;
}

// Draw status icons along the left-hand side of the screen
int CHudStatusIcons::Draw( float )
{
	// find starting position to draw from, along right-hand side of screen
	int x = 5;
	int y = ScreenHeight / 2 + 32;
	
	// loop through icon list, and draw any valid icons drawing up from the middle of screen
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( m_IconList[i].spr )
		{
			y -= ( m_IconList[i].rc.bottom - m_IconList[i].rc.top ) + 5;
			
			SPR_Set( m_IconList[i].spr, m_IconList[i].r, m_IconList[i].g, m_IconList[i].b );
			SPR_DrawAdditive( 0, x, y, &m_IconList[i].rc );
		}
	}
	
	y = ScreenHeight / 2;
	for (int i = 0; i < MAX_CUSTOMSPRITES; ++i)
	{
		if (i == 3)
			y = ScreenHeight / 2;
		icon_sprite_t& icon = m_CustomList[i];
		if (icon.spr)
		{
			SPR_Set(icon.spr, icon.r, icon.g, icon.b);
			SPR_DrawAdditive(0, i < 3 ? 100 : ScreenWidth - 100, y, &icon.rc);
			y += icon.rc.bottom - icon.rc.top + 5;
		}
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
int CHudStatusIcons::MsgFunc_StatusIcon( const char *, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	int ShouldEnable = READ_BYTE();
	char *pszIconName = READ_STRING();
	if ( ShouldEnable )
	{
		int r = READ_BYTE();
		int g = READ_BYTE();
		int b = READ_BYTE();
		EnableIcon( pszIconName, r, g, b );
		m_iFlags |= HUD_ACTIVE;
	}
	else
	{
		DisableIcon( pszIconName );
	}

	return 1;
}

// add the icon to the icon list, and set it's drawing color
void CHudStatusIcons::EnableIcon( char *pszIconName, unsigned char red, unsigned char green, unsigned char blue )
{
	int i;
	// check to see if the sprite is in the current list
	for ( i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
			break;
	}

	if ( i == MAX_ICONSPRITES )
	{
		// icon not in list, so find an empty slot to add to
		for ( i = 0; i < MAX_ICONSPRITES; i++ )
		{
			if ( !m_IconList[i].spr )
				break;
		}
	}

	// if we've run out of space in the list, overwrite the first icon
	if ( i == MAX_ICONSPRITES )
	{
		i = 0;
	}

	// Load the sprite and add it to the list
	// the sprite must be listed in hud.txt
	int spr_index = gHUD.GetSpriteIndex( pszIconName );
	m_IconList[i].spr = gHUD.GetSprite( spr_index );
	m_IconList[i].rc = gHUD.GetSpriteRect( spr_index );
	m_IconList[i].r = red;
	m_IconList[i].g = green;
	m_IconList[i].b = blue;
	strcpy( m_IconList[i].szSpriteName, pszIconName );

	// Hack: Play Timer sound when a grenade icon is played (in 0.8 seconds)
	if ( strstr(m_IconList[i].szSpriteName, "grenade") )
	{
		cl_entity_t *pthisplayer = gEngfuncs.GetLocalPlayer();
		gEngfuncs.pEventAPI->EV_PlaySound( pthisplayer->index, pthisplayer->origin, CHAN_STATIC, "weapons/timer.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
	}
}

void CHudStatusIcons::DisableIcon( char *pszIconName )
{
	// find the sprite is in the current list
	for ( int i = 0; i < MAX_ICONSPRITES; i++ )
	{
		if ( !stricmp( m_IconList[i].szSpriteName, pszIconName ) )
		{
			// clear the item from the list
			memset( &m_IconList[i], 0, sizeof(icon_sprite_t) );
			return;
		}
	}
}

void CHudStatusIcons::EnableCustomIcon(int index, char* name, unsigned char red,
	unsigned char green, unsigned char blue, wrect_t& rect)
{
	if (index <= 5)
	{
		char path[268];
		sprintf(path, "sprites/%s.spr", name);
		icon_sprite_t& icon = m_CustomList[index];
		icon.spr = SPR_Load(path);
		icon.rc = rect;
		icon.r = red;
		icon.g = green;
		icon.b = blue;
		strcpy(icon.szSpriteName, name);
	}
}

void CHudStatusIcons::DisableCustomIcon(int index)
{
	if (index <= 5)
		memset(&m_CustomList[index], 0, sizeof(m_CustomList[index]));
}

int CHudStatusIcons::MsgFunc_CustomIcon(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	int enable = READ_BYTE();
	int index = READ_BYTE();
	if (enable)
	{
		char* name = READ_STRING();
		unsigned char red = READ_BYTE();
		unsigned char green = READ_BYTE();
		unsigned char blue = READ_BYTE();
		wrect_t rect;
		rect.left = READ_BYTE();
		rect.top = READ_BYTE();
		rect.right = READ_BYTE();
		rect.bottom = READ_BYTE();
		EnableCustomIcon(index, name, red, green, blue, rect);
		m_iFlags |= HUD_ACTIVE;
	}
	else
		DisableCustomIcon(index);
	return 1;
}
