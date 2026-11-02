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
// ammo_secondary.cpp
//
// implementation of CHudAmmoSecondary class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE(m_AmmoSecondary, SecAmmoVal)
DECLARE_MESSAGE(m_AmmoSecondary, SecAmmoIcon)

int CHudAmmoSecondary::Init()
{
	HOOK_MESSAGE(SecAmmoVal);
	HOOK_MESSAGE(SecAmmoIcon);
	gHUD.AddHudElem(this);
	m_HUD_ammoicon = 0;
	for (int i = 0; i < 4; i++)
		m_iAmmoAmounts[i] = -1;  // -1 means don't draw this value
	Reset();
	return 1;
}

int CHudAmmoSecondary::VidInit()
{
	return 1;
}

void CHudAmmoSecondary::Reset()
{
	m_fFade = 0;
}

int CHudAmmoSecondary::MsgFunc_SecAmmoVal(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	unsigned int index = (unsigned int)READ_BYTE();
	if (index < 4)
	{
		m_iAmmoAmounts[index] = READ_BYTE();
		int count = 0;
		for (int i = 0; i < 4; i++)
		{
			if (m_iAmmoAmounts[i] > 0)
				count += m_iAmmoAmounts[i];
		}
		m_iFlags |= HUD_ACTIVE;
		if (count)
			m_fFade = 200;
		else
			m_iFlags &= ~HUD_ACTIVE;
	}
	return 1;
}

int CHudAmmoSecondary::MsgFunc_SecAmmoIcon(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	m_HUD_ammoicon = gHUD.GetSpriteIndex(READ_STRING());
	return 1;
}

int CHudAmmoSecondary::Draw(float)
{
	if (gHUD.m_iHideHUDDisplay & (HIDEHUD_WEAPONS | HIDEHUD_ALL))
		return 1;
	int r = 255, g = 160, b = 0;
	int alpha = m_fFade > MIN_ALPHA ? (int)m_fFade : MIN_ALPHA;
	if (m_fFade > 0)
		m_fFade = (float)(m_fFade - 20.0 * gHUD.m_flTimeDelta);
	ScaleColors(r, g, b, alpha);
	wrect_t &number = gHUD.GetSpriteRect(gHUD.m_HUD_number_0);
	int width = number.right - number.left;
	int x = ScreenWidth - width;
	int y = ScreenHeight - 4 * gHUD.m_iFontHeight;
	if (m_HUD_ammoicon)
	{
		wrect_t &icon = gHUD.GetSpriteRect(m_HUD_ammoicon);
		x += icon.left - icon.right;
		y += icon.bottom - icon.top;
		SPR_Set(gHUD.GetSprite(m_HUD_ammoicon), r, g, b);
		SPR_DrawAdditive(0, x, y, &icon);
	}
	else
	{  // move the cursor by the '0' char instead, since we don't have an icon to work with
		x -= width;
		y += number.bottom - number.top;
	}

	// draw the ammo counts, in reverse order, from right to left
	for (int i = 3; i >= 0; i--)
	{
		if (m_iAmmoAmounts[i] >= 0)
		{
			int numberX = width / -2 + x - width * gHUD.GetNumWidth(m_iAmmoAmounts[i], DHN_DRAWZERO);
			gHUD.DrawHudNumber(numberX, y, DHN_DRAWZERO, m_iAmmoAmounts[i], r, g, b);
			if (i == 0)
				return 1;
			x = width / -2 + numberX;
			FillRGBA(x, y, width / 10, gHUD.m_iFontHeight, r, g, b, alpha);
		}
	}
	return 1;
}
