#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <stdio.h>
#include <string.h>

DECLARE_MESSAGE(m_PlayerBrowse, PlyrBrowse);

int CHudPlayerBrowse::Init()
{
	gHUD.AddHudElem(this);
	HOOK_MESSAGE(PlyrBrowse);
	InitHUDData();
	return 1;
}

int CHudPlayerBrowse::VidInit()
{
	return 1;
}

void CHudPlayerBrowse::InitHUDData()
{
	memset(m_szLineBuffer, 0, sizeof(m_szLineBuffer));
	memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));
	m_flDelayFade = 0;
}

int CHudPlayerBrowse::MsgFunc_PlyrBrowse(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	m_fFriendly = READ_BYTE() != 0;
	m_iNewTeamNum = READ_BYTE();
	m_szNewLineBuffer[0] = 0;
	if (m_iNewTeamNum > 0)
		strcpy(m_szNewLineBuffer, m_fFriendly ? "(Friendly) " : " (Enemy) ");
	strncat(m_szNewLineBuffer, READ_STRING(), size - 3);
	if (!m_szNewLineBuffer[0])
	{
		m_flDelayFade = 200;
		m_flDelayFadeSprite = 255;
		return 1;
	}
	unsigned char powerups = READ_BYTE();
	if (powerups)
	{
		const char* name = NULL;
		int red = 0, green = 0, blue = 0;
		if (powerups & 4) { name = "score_ctfljump"; red = 255; green = 160; }
		else if (powerups & 8) { name = "score_ctfphev"; red = 128; green = 160; blue = 255; }
		else if (powerups & 128) { name = "score_ctfregen"; green = 255; }
		else if (powerups & 32) { name = "score_ctfaccel"; red = 255; }
		else if (powerups & 16) { name = "score_ctfbpack"; red = green = 255; }
		if (name)
		{
			int index = gHUD.GetSpriteIndex(name);
			m_PowerupSprite.spr = gHUD.GetSprite(index);
			m_PowerupSprite.rc = gHUD.GetSpriteRect(index);
			m_PowerupSprite.r = red;
			m_PowerupSprite.g = green;
			m_PowerupSprite.b = blue;
		}
	}
	else
		memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));
	m_iHealth = READ_BYTE();
	m_iArmor = READ_BYTE();
	if (m_fFriendly || m_iNewTeamNum == 0)
	{
		char stats[64];
		sprintf(stats, " (%d/%d)", m_iHealth, m_iArmor);
		strcat(m_szNewLineBuffer, stats);
	}
	strcpy(m_szLineBuffer, m_szNewLineBuffer);
	m_iFlags |= HUD_ACTIVE;
	m_flDelayFade = m_flDelayFadeSprite = 0;
	m_iTeamNum = m_iNewTeamNum;
	return 1;
}

int CHudPlayerBrowse::Draw(float)
{
	int red = 192, green = 192, blue = 192;
	if (m_iTeamNum == 1) { red = 255; green = 160; blue = 0; }
	else if (m_iTeamNum == 2) { red = 0; green = 160; blue = 0; }
	if (m_flDelayFade > 0)
	{
		if (m_flDelayFade < 40)
		{
			m_flDelayFade = 0;
			m_szLineBuffer[0] = 0;
			m_iTeamNum = m_iNewTeamNum;
			memset(&m_PowerupSprite, 0, sizeof(m_PowerupSprite));
			m_iFlags &= ~HUD_ACTIVE;
		}
		else
		{
			ScaleColors(red, green, blue, (int)m_flDelayFade);
			m_flDelayFade -= (float)(85 * gHUD.m_flTimeDelta);
		}
	}
	if (m_szLineBuffer[0])
	{
		int y = (int)(ScreenHeight * 0.75);
		if (m_PowerupSprite.spr)
		{
			if (m_flDelayFadeSprite > 0)
			{
				ScaleColors(m_PowerupSprite.r, m_PowerupSprite.g, m_PowerupSprite.b,
					max(40, (int)m_flDelayFadeSprite));
				m_flDelayFadeSprite -= (float)(10 * gHUD.m_flTimeDelta);
			}
			SPR_Set(m_PowerupSprite.spr, m_PowerupSprite.r, m_PowerupSprite.g, m_PowerupSprite.b);
			SPR_DrawAdditive(0, ScreenWidth / 2 - (m_PowerupSprite.rc.right - m_PowerupSprite.rc.left) - 105,
				y, &m_PowerupSprite.rc);
		}
		gHUD.DrawHudString(ScreenWidth / 2 - 100, y, ScreenWidth, m_szLineBuffer, red, green, blue);
	}
	return 1;
}
