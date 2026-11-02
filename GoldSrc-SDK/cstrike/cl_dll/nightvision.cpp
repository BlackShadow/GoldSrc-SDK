#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "eventscripts.h"

void EV_HLDM_CreateLight(float *origin, float radius, int r, int g, int b, float time);

DECLARE_MESSAGE(m_NightVision, NVGToggle)
DECLARE_COMMAND(m_NightVision, NVGAdjustUp)
DECLARE_COMMAND(m_NightVision, NVGAdjustDown)

CHudNightVision::~CHudNightVision()
{
}

int CHudNightVision::Init()
{
	m_fOn = 0;
	m_iAlpha = 110;
	HOOK_MESSAGE(NVGToggle);
	HOOK_COMMAND("+nvgadjust", NVGAdjustUp);
	HOOK_COMMAND("-nvgadjust", NVGAdjustDown);
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}

int CHudNightVision::VidInit()
{
	return 1;
}

void CHudNightVision::Reset()
{
	m_fOn = 0;
	m_iAlpha = 110;
}

int CHudNightVision::MsgFunc_NVGToggle(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	m_fOn = READ_BYTE();
	return 1;
}

void CHudNightVision::UserCmd_NVGAdjustUp()
{
	m_iAlpha += 10;
	if (m_iAlpha > 220)
		m_iAlpha = 220;
}

void CHudNightVision::UserCmd_NVGAdjustDown()
{
	m_iAlpha -= 10;
	if (m_iAlpha < 30)
		m_iAlpha = 30;
}

int CHudNightVision::Draw(float)
{
	if (!m_fOn || gEngfuncs.IsSpectateOnly())
		return 1;
	FillRGBA(0, 0, ScreenWidth, ScreenHeight, 50, 225, 50, m_iAlpha);
	float radius = (float)(gEngfuncs.pfnRandomLong(0, 50) + 750);
	EV_HLDM_CreateLight(gHUD.m_vecOrigin, radius, 1, 20, 1, 0.1f);
	return 1;
}
