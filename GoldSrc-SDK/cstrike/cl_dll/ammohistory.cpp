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
#include <string.h>

#define AMMO_PICKUP_GAP (gHR.iHistoryGap + 5)
#define AMMO_PICKUP_PICK_HEIGHT (72 + gHR.iHistoryGap * 2)
#define AMMO_PICKUP_HEIGHT_MAX (ScreenHeight - 100)

HistoryResource gHR;
int HISTORY_DRAW_TIME = 5;

void HistoryResource::AddToHistory(int type, int id, int count)
{
	if (type == HISTSLOT_AMMO && !count)
		return;
	if (AMMO_PICKUP_GAP * iCurrentHistorySlot + AMMO_PICKUP_PICK_HEIGHT > AMMO_PICKUP_HEIGHT_MAX
		|| iCurrentHistorySlot >= MAX_HISTORY)
		iCurrentHistorySlot = 0;
	HIST_ITEM *item = &rgAmmoHistory[iCurrentHistorySlot++];
	HISTORY_DRAW_TIME = (int)CVAR_GET_FLOAT("hud_drawhistory_time");
	item->type = type;
	item->iId = id;
	item->iCount = count;
	item->DisplayTime = gHUD.m_flTime + (float)HISTORY_DRAW_TIME;
}

void HistoryResource::AddToHistory(int type, const char *name, int count)
{
	if (type != HISTSLOT_ITEM)
		return;
	if (AMMO_PICKUP_GAP * iCurrentHistorySlot + AMMO_PICKUP_PICK_HEIGHT > AMMO_PICKUP_HEIGHT_MAX
		|| iCurrentHistorySlot >= MAX_HISTORY)
		iCurrentHistorySlot = 0;
	HIST_ITEM *item = &rgAmmoHistory[iCurrentHistorySlot++];
	int id = gHUD.GetSpriteIndex(name);
	if (id == -1)
		return;
	item->iId = id;
	item->type = type;
	item->iCount = count;
	HISTORY_DRAW_TIME = (int)CVAR_GET_FLOAT("hud_drawhistory_time");
	item->DisplayTime = gHUD.m_flTime + (float)HISTORY_DRAW_TIME;
}

void HistoryResource::CheckClearHistory()
{
	for (int i = 0; i < MAX_HISTORY; i++)
	{
		if (rgAmmoHistory[i].type)
			return;
	}
	iCurrentHistorySlot = 0;
}

int HistoryResource::DrawAmmoHistory(float time)
{
	for (int i = 0; i < MAX_HISTORY; i++)
	{
		HIST_ITEM &item = rgAmmoHistory[i];
		if (!item.type)
			continue;
		if (item.DisplayTime > gHUD.m_flTime + (float)HISTORY_DRAW_TIME)
			item.DisplayTime = gHUD.m_flTime + (float)HISTORY_DRAW_TIME;
		if (!(time < item.DisplayTime))
		{
			memset(&item, 0, sizeof(item));
			CheckClearHistory();
			continue;
		}
		int y = ScreenHeight - (AMMO_PICKUP_PICK_HEIGHT + AMMO_PICKUP_GAP * i);
		if (item.type == HISTSLOT_AMMO)
		{
			wrect_t rect;
			HSPRITE *sprite = gWR.GetAmmoPicFromWeapon(item.iId, rect);
			int r = 255, g = 160, b = 0;
			int alpha = (int)(float)((item.DisplayTime - time) * 80.0);
			ScaleColors(r, g, b, min(alpha, 255));
			int x = ScreenWidth - 24;
			if (sprite && *sprite)
			{
				SPR_Set(*sprite, r, g, b);
				SPR_DrawAdditive(0, x, y, &rect);
			}
			gHUD.DrawHudNumberString(x, y, ScreenWidth - 124, item.iCount, r, g, b);
		}
		else if (item.type == HISTSLOT_WEAP)
		{
			WEAPON *weapon = gWR.GetWeapon(item.iId);
			if (!weapon)
				return 1;
			int r = 255, g = 160, b = 0;
			if (!gWR.HasAmmo(weapon))
			{
				g = 16;
				b = 16;
			}
			int alpha = (int)(float)((item.DisplayTime - time) * 80.0);
			ScaleColors(r, g, b, min(alpha, 255));
			int x = ScreenWidth - (weapon->rcInactive.right - weapon->rcInactive.left);
			SPR_Set(weapon->hInactive, r, g, b);
			SPR_DrawAdditive(0, x, y, &weapon->rcInactive);
		}
		else if (item.type == HISTSLOT_ITEM && item.iId)
		{
			wrect_t rect = gHUD.GetSpriteRect(item.iId);
			int r = 255, g = 160, b = 0;
			int alpha = (int)(float)((item.DisplayTime - time) * 80.0);
			ScaleColors(r, g, b, min(alpha, 255));
			int x = ScreenWidth - (rect.right - rect.left) - 10;
			SPR_Set(gHUD.GetSprite(item.iId), r, g, b);
			SPR_DrawAdditive(0, x, y, &rect);
		}
	}
	return 1;
}
