#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include <stdio.h>
#include <string.h>

DECLARE_MESSAGE(m_FlagIcons, FlagIcon);
DECLARE_MESSAGE(m_FlagIcons, FlagTimer);

extern team_info_t g_TeamInfo[MAX_PLAYERS + 1];

int CHudFlagIcons::Init()
{
	HOOK_MESSAGE(FlagIcon);
	HOOK_MESSAGE(FlagTimer);
	gHUD.AddHudElem(this);
	memset(m_FlagList, 0, sizeof(m_FlagList));
	m_flTimeStart = m_flTimeLimit = 0;
	m_bIsTimer = m_bTimerReset = false;
	return 1;
}

int CHudFlagIcons::VidInit()
{
	return 1;
}

void CHudFlagIcons::InitHUDData()
{
	memset(m_FlagList, 0, sizeof(m_FlagList));
}

void CHudFlagIcons::EnableFlag(char* name, unsigned char team, unsigned char red,
	unsigned char green, unsigned char blue, unsigned char score)
{
	if (team <= 3)
	{
		flag_sprite_t& flag = m_FlagList[team];
		int index = gHUD.GetSpriteIndex(name);
		flag.spr = gHUD.GetSprite(index);
		flag.rc = gHUD.GetSpriteRect(index);
		flag.r = red;
		flag.g = green;
		flag.b = blue;
		flag.score = score;
		g_TeamInfo[team + 1].scores_overriden = 1;
		g_TeamInfo[team + 1].frags = score;
		strcpy(flag.szSpriteName, name);
	}
}

void CHudFlagIcons::DisableFlag(char*, unsigned char team)
{
	if (team <= 3)
		memset(&m_FlagList[team], 0, sizeof(m_FlagList[team]));
}

int CHudFlagIcons::MsgFunc_FlagIcon(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	int enable = READ_BYTE();
	char* name = READ_STRING();
	unsigned char team = READ_BYTE();
	if (enable)
	{
		unsigned char red = READ_BYTE();
		unsigned char green = READ_BYTE();
		unsigned char blue = READ_BYTE();
		unsigned char score = READ_BYTE();
		EnableFlag(name, team, red, green, blue, score);
		m_iFlags |= HUD_ACTIVE;
	}
	else if (team <= 3)
		DisableFlag(name, team);
	else
		m_iFlags &= ~HUD_ACTIVE;
	return 1;
}

int CHudFlagIcons::MsgFunc_FlagTimer(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	m_bIsTimer = READ_BYTE() != 0;
	if (m_bIsTimer)
	{
		m_flTimeLimit = (float)READ_SHORT();
		m_bTimerReset = true;
	}
	return 1;
}

int CHudFlagIcons::Draw(float)
{
	if (gEngfuncs.IsSpectateOnly() == 2)
		return 1;
	int y = ScreenHeight - 64;
	for (int i = 0; i < 4; ++i)
	{
		flag_sprite_t& flag = m_FlagList[i];
		if (!flag.spr)
			continue;
		int height = flag.rc.bottom - flag.rc.top;
		y -= height + 5;
		SPR_Set(flag.spr, flag.r, flag.g, flag.b);
		SPR_DrawAdditive(0, 5, y, &flag.rc);
		gHUD.DrawHudNumber(flag.rc.right - flag.rc.left + 10, y + height / 2 - 5,
			DHN_2DIGITS | DHN_DRAWZERO, flag.score, flag.r, flag.g, flag.b);
	}
	if (m_bIsTimer)
	{
		if (m_bTimerReset)
		{
			m_flTimeStart = gHUD.m_flTime;
			m_bTimerReset = false;
		}
		float remaining = m_flTimeLimit - (gHUD.m_flTime - m_flTimeStart);
		int minutes = max(0, (int)(float)(remaining / 60));
		int seconds = max(0, (int)(float)(remaining - 60 * minutes));
		char text[256];
		sprintf(text, "%s %d:%02d", CHudTextMessage::BufferedLocaliseTextString("#CTFTimeRemain"), minutes, seconds);
		gHUD.DrawHudString(5, ScreenHeight - 60, 200, text, giR, giG, giB);
	}
	return 1;
}
