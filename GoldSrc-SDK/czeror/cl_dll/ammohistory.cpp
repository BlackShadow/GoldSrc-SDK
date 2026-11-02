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
//  ammohistory.cpp
//


#include "hud.h"
#include "cl_util.h"
#include "ammohistory.h"

HistoryResource gHR;
int HISTORY_DRAW_TIME = 5;

void HistoryResource::AddToHistory( int type, int id, int accessory, int count )
{
	if ( type == HISTSLOT_AMMO && !count )
		return;
	int slot = iCurrentHistorySlot;
	if ( ScreenHeight - 99 <= slot * ( gHR.iHistoryGap + 5 ) + 2 * gHR.iHistoryGap + 32 || slot > 11 )
		slot = 0;
	iCurrentHistorySlot = slot + 1;
	HISTORY_DRAW_TIME = (int)CVAR_GET_FLOAT( "hud_drawhistory_time" );
	HIST_ITEM& item = rgAmmoHistory[slot];
	item.type = type;
	item.iId = id;
	item.iCount = count;
	item.iIsAccessory = accessory;
	item.DisplayTime = HISTORY_DRAW_TIME + gHUD.m_flTime;
}

void HistoryResource::AddToHistory( int type, const char* name, int count )
{
	if ( type != HISTSLOT_ITEM )
		return;
	int slot = iCurrentHistorySlot;
	if ( ScreenHeight - 99 <= slot * ( gHR.iHistoryGap + 5 ) + 2 * gHR.iHistoryGap + 32 || slot > 11 )
		slot = 0;
	iCurrentHistorySlot = slot + 1;
	int sprite = gHUD.GetSpriteIndex( name );
	if ( sprite == -1 )
		return;
	HIST_ITEM& item = rgAmmoHistory[slot];
	item.iId = sprite;
	item.type = HISTSLOT_ITEM;
	item.iCount = count;
	HISTORY_DRAW_TIME = (int)CVAR_GET_FLOAT( "hud_drawhistory_time" );
	item.DisplayTime = HISTORY_DRAW_TIME + gHUD.m_flTime;
}

void HistoryResource::CheckClearHistory()
{
	for ( int i = 0; i < MAX_HISTORY; ++i )
	{
		if ( rgAmmoHistory[i].type )
			return;
	}
	iCurrentHistorySlot = 0;
}

int HistoryResource::DrawAmmoHistory( float time )
{
	for ( int i = 0; i < MAX_HISTORY; ++i )
	{
		HIST_ITEM& item = rgAmmoHistory[i];
		if ( !item.type )
			continue;
		if ( item.DisplayTime > HISTORY_DRAW_TIME + gHUD.m_flTime )
			item.DisplayTime = HISTORY_DRAW_TIME + gHUD.m_flTime;
		if ( !( time < item.DisplayTime ) )
		{
			memset( &item, 0, sizeof( item ) );
			CheckClearHistory();
			continue;
		}
		int r = 64, g = 255, b = 64;
		int alpha = (int)(float)( ( item.DisplayTime - time ) * 80.0 );
		if ( alpha > 255 )
			alpha = 255;
		int y = ScreenHeight - ( i * ( gHR.iHistoryGap + 5 ) + 2 * gHR.iHistoryGap + 32 );
		switch ( item.type )
		{
		case HISTSLOT_AMMO:
		{
			wrect_t rect;
			HSPRITE* sprite = gWR.GetAmmoPicFromWeapon( item.iId, rect, item.iIsAccessory );
			ScaleColors( r, g, b, alpha );
			if ( sprite && *sprite )
			{
				SPR_Set( *sprite, r, g, b );
				SPR_DrawAdditive( 0, ScreenWidth - 24, y, &rect );
			}
			gHUD.DrawHudNumberString( ScreenWidth - 34, y, ScreenWidth - 124, item.iCount, r, g, b );
			break;
		}
		case HISTSLOT_WEAP:
		{
			WEAPON* weapon = gWR.GetItem( item.iId, item.iIsAccessory != 0 );
			if ( !weapon )
				return 1;
			if ( !gWR.HasAmmo( weapon ) &&
				( !weapon->bIsAccessory || weapon->iId != ACCESSORY_RADIOCONTROLLEDBOMB ) )
			{
				r = 255;
				g = 16;
				b = 16;
			}
			ScaleColors( r, g, b, alpha );
			SPR_Set( weapon->hInactive, r, g, b );
			SPR_DrawAdditive( 0, ScreenWidth - ( weapon->rcInactive.right - weapon->rcInactive.left ), y, &weapon->rcInactive );
			break;
		}
		case HISTSLOT_ITEM:
			if ( item.iId )
			{
				wrect_t rect = gHUD.GetSpriteRect( item.iId );
				ScaleColors( r, g, b, alpha );
				SPR_Set( gHUD.GetSprite( item.iId ), r, g, b );
				SPR_DrawAdditive( 0, ScreenWidth - ( rect.right - rect.left ), y, &rect );
			}
			break;
		}
	}
	return 1;
}
