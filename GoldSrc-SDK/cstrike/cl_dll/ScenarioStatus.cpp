#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

DECLARE_MESSAGE(m_scenarioStatus, Scenario)

int CHudScenarioStatus::Init()
{
	HOOK_MESSAGE(Scenario);
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudScenarioStatus::VidInit()
{
	m_alpha = MIN_ALPHA;
	m_hSprite = 0;
	return 1;
}

void CHudScenarioStatus::Reset()
{
	m_iFlags &= ~HUD_ACTIVE;
	m_hSprite = 0;
	m_nextFlash = 0;
}

int CHudScenarioStatus::MsgFunc_Scenario(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	int oldFlags = m_iFlags;
	if (!READ_BYTE())
	{
		m_iFlags &= ~HUD_ACTIVE;
		return 1;
	}
	m_iFlags |= HUD_ACTIVE;
	int index = gHUD.GetSpriteIndex(READ_STRING());
	m_hSprite = gHUD.GetSprite(index);
	m_rect = gHUD.GetSpriteRect(index);
	int alpha = READ_BYTE();
	m_flashAlpha = alpha >= MIN_ALPHA ? alpha : MIN_ALPHA;
	if (oldFlags & HUD_ACTIVE)
		alpha = m_alpha;
	else
		m_alpha = alpha;
	if (alpha == MIN_ALPHA)
		return 1;
	m_flashInterval = (float)(READ_SHORT() * 0.01f);
	if (m_nextFlash == 0)
		m_nextFlash = gHUD.m_flTime;
	m_nextFlash = (float)(READ_SHORT() * 0.01f + m_nextFlash);
	return 1;
}

int CHudScenarioStatus::Draw(float time)
{
	if (!(gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) && !g_iUser1 && !gEngfuncs.IsSpectateOnly() && m_hSprite)
	{
		if (m_iFlags & HUD_ACTIVE)
		{
			int x = gHUD.m_roundTimer.m_closestRight;
			int y = ScreenHeight - 3 * gHUD.m_iFontHeight / 2
				- (m_rect.bottom - m_rect.top - gHUD.m_iFontHeight) / 2;
			int r = 255, g = 160, b = 0;
			ScaleColors(r, g, b, m_alpha);
			if (m_alpha > MIN_ALPHA)
				m_alpha = (int)(float)(m_alpha * 0.9f);
			SPR_Set(m_hSprite, r, g, b);
			SPR_DrawAdditive(0, x, y, &m_rect);
		}
		if (m_flashInterval != 0 && time >= m_nextFlash)
		{
			m_nextFlash = m_flashInterval + time;
			m_alpha = m_flashAlpha;
		}
	}
	return 1;
}
