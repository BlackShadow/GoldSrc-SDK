/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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

DECLARE_MESSAGE(m_Battery, Battery)
DECLARE_MESSAGE(m_Battery, ArmorType)

int CHudBattery::Init()
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;
	m_iArmorType = 0;
	HOOK_MESSAGE(Battery);
	HOOK_MESSAGE(ArmorType);
	gHUD.AddHudElem(this);
	return 1;
}

int CHudBattery::VidInit()
{
	m_HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	m_HUD_suit_full = gHUD.GetSpriteIndex("suit_full");
	m_HUD_suithelmet_empty = gHUD.GetSpriteIndex("suithelmet_empty");
	m_HUD_suithelmet_full = gHUD.GetSpriteIndex("suithelmet_full");
	m_fFade = 0;
	return 1;
}

int CHudBattery::MsgFunc_Battery(const char *, int size, void *buffer)
{
	m_iFlags |= HUD_ACTIVE;
	BEGIN_READ(buffer, size);
	int battery = READ_SHORT();
	if (battery != m_iBat)
	{
		m_fFade = 100;
		m_iBat = battery;
		if (battery <= 0)
			m_iArmorType = 0;
	}
	return 1;
}

int CHudBattery::MsgFunc_ArmorType(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	m_iArmorType = READ_BYTE();
	return 1;
}

int CHudBattery::Draw(float)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) || g_iUser1)
		return 1;
	int empty = m_iArmorType == 1 ? m_HUD_suithelmet_empty : m_HUD_suit_empty;
	int full = m_iArmorType == 1 ? m_HUD_suithelmet_full : m_HUD_suit_full;
	wrect_t rect = gHUD.GetSpriteRect(full);
	double missing = m_iBat <= 99 ? (100 - m_iBat) * 0.01 : 0;
	rect.top = (int)(rect.top + missing * (rect.bottom - rect.top));
	if (!(gHUD.m_iWeaponBits & (1U << WEAPON_SUIT)))
		return 1;
	int alpha = MIN_ALPHA;
	if (m_fFade != 0)
	{
		if (m_fFade > 100)
			m_fFade = 100;
		m_fFade = (float)(m_fFade - 20.0 * gHUD.m_flTimeDelta);
		if (m_fFade <= 0)
			m_fFade = 0;
		else
			alpha = (int)(float)(m_fFade / 100.0 * 128.0 + MIN_ALPHA);
	}
	int r = 255, g = 160, b = 0;
	ScaleColors(r, g, b, alpha);
	wrect_t &emptyRect = gHUD.GetSpriteRect(empty);
	int offset = (emptyRect.bottom - emptyRect.top) / 6;
	int y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2 + 3 * (ScreenWidth > 639) + 1;
	int x = ScreenWidth / 5;
	SPR_Set(gHUD.GetSprite(empty), r, g, b);
	SPR_DrawAdditive(0, x, y - offset, &emptyRect);
	if (rect.bottom > rect.top)
	{
		SPR_Set(gHUD.GetSprite(full), r, g, b);
		SPR_DrawAdditive(0, x, rect.top + y - offset - gHUD.GetSpriteRect(full).top, &rect);
	}
	gHUD.DrawHudNumber(emptyRect.right + x - emptyRect.left, y - (ScreenWidth > 639 ? 4 : 1),
		DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);
	return 1;
}
