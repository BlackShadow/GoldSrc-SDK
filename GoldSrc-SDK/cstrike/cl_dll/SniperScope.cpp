#include "hud.h"
#include "cl_util.h"
#include "pm_shared.h"
#include "r_studioint.h"
#include "vgui_controls/Controls.h"
#include "vgui/VGuiVertex.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"

extern engine_studio_api_t IEngineStudio;

#define SCOPE_UV_MIN 0.001953125f
#define SCOPE_UV_MAX 0.998046875f

CHudSniperScope::~CHudSniperScope()
{
}

int CHudSniperScope::Init()
{
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem(this);
	return 1;
}

void CHudSniperScope::Reset()
{
}

int CHudSniperScope::VidInit()
{
	if (IEngineStudio.IsHardware())
	{
		m_iScopeTextureID = vgui2::surface()->CreateNewTextureID(false);
		vgui2::surface()->DrawSetTextureFile(m_iScopeTextureID, "sprites/scope_arc", 1, false);
	}
	else
	{
		m_iScopeNWTextureID = vgui2::surface()->CreateNewTextureID(false);
		m_iScopeNETextureID = vgui2::surface()->CreateNewTextureID(false);
		m_iScopeSWTextureID = vgui2::surface()->CreateNewTextureID(false);
		m_iScopeSETextureID = vgui2::surface()->CreateNewTextureID(false);
		vgui2::surface()->DrawSetTextureFile(m_iScopeNWTextureID, "sprites/scope_arc_nw", 1, false);
		vgui2::surface()->DrawSetTextureFile(m_iScopeNETextureID, "sprites/scope_arc_ne", 1, false);
		vgui2::surface()->DrawSetTextureFile(m_iScopeSWTextureID, "sprites/scope_arc_sw", 1, false);
		vgui2::surface()->DrawSetTextureFile(m_iScopeSETextureID, "sprites/scope_arc", 1, false);
	}
	return 1;
}

int CHudSniperScope::Draw(float)
{
	if ((g_iUser1 & ~OBS_IN_EYE) || gHUD.m_iFOV > 44)
		return 1;
	int bottom = ScreenHeight - ScreenHeight / 16;
	int left = (ScreenWidth - ScreenHeight) / 2 + ScreenHeight / 16;
	int top = ScreenHeight / 16;
	int right = ScreenWidth - left;
	int centerX = ScreenWidth / 2;
	int centerY = ScreenHeight / 2;
	if (IEngineStudio.IsHardware())
	{
		vgui2::surface()->DrawSetTexture(m_iScopeTextureID);
		vgui2::surface()->DrawSetColor(0, 0, 0, 255);
		vgui2::VGuiVertex vertices[4] =
		{
			{ centerX, centerY, SCOPE_UV_MIN, SCOPE_UV_MIN },
			{ right, centerY, SCOPE_UV_MAX, SCOPE_UV_MIN },
			{ right, bottom, SCOPE_UV_MAX, SCOPE_UV_MAX },
			{ centerX, bottom, SCOPE_UV_MIN, SCOPE_UV_MAX }
		};
		vgui2::surface()->DrawTexturedPolygon(vertices, 4);
		vertices[0] = { centerX - 1, top, SCOPE_UV_MIN, SCOPE_UV_MAX };
		vertices[1] = { right, top, SCOPE_UV_MAX, SCOPE_UV_MAX };
		vertices[2] = { right, centerY + 1, SCOPE_UV_MAX, SCOPE_UV_MIN };
		vertices[3] = { centerX - 1, centerY + 1, SCOPE_UV_MIN, SCOPE_UV_MIN };
		vgui2::surface()->DrawTexturedPolygon(vertices, 4);
		vertices[0] = { left, centerY, SCOPE_UV_MAX, SCOPE_UV_MIN };
		vertices[1] = { centerX, centerY, SCOPE_UV_MIN, SCOPE_UV_MIN };
		vertices[2] = { centerX, bottom, SCOPE_UV_MIN, SCOPE_UV_MAX };
		vertices[3] = { left, bottom, SCOPE_UV_MAX, SCOPE_UV_MAX };
		vgui2::surface()->DrawTexturedPolygon(vertices, 4);
		vertices[0] = { left, top, SCOPE_UV_MAX, SCOPE_UV_MAX };
		vertices[1] = { centerX, top, SCOPE_UV_MIN, SCOPE_UV_MAX };
		vertices[2] = { centerX, centerY, SCOPE_UV_MIN, SCOPE_UV_MIN };
		vertices[3] = { left, centerY, SCOPE_UV_MAX, SCOPE_UV_MIN };
		vgui2::surface()->DrawTexturedPolygon(vertices, 4);
	}
	else
	{
		vgui2::surface()->DrawSetColor(0, 0, 0, 255);
		vgui2::surface()->DrawSetTexture(m_iScopeNWTextureID);
		vgui2::surface()->DrawTexturedRect(left, top, centerX, centerY);
		vgui2::surface()->DrawSetTexture(m_iScopeNETextureID);
		vgui2::surface()->DrawTexturedRect(centerX, top, right, centerY);
		vgui2::surface()->DrawSetTexture(m_iScopeSWTextureID);
		vgui2::surface()->DrawTexturedRect(left, centerY, centerX, bottom);
		vgui2::surface()->DrawSetTexture(m_iScopeSETextureID);
		vgui2::surface()->DrawTexturedRect(centerX, centerY, right, bottom);
	}
	if (IEngineStudio.IsHardware() == 1)
	{
		int opening = vgui2::scheme()->GetProportionalScaledValue(10);
		vgui2::surface()->DrawFilledRect(centerX, 0, centerX + 1, centerY - opening);
		vgui2::surface()->DrawFilledRect(centerX, centerY + opening, centerX + 1, ScreenHeight);
		vgui2::surface()->DrawFilledRect(0, centerY + 1, centerX - opening, centerY + 2);
		vgui2::surface()->DrawFilledRect(centerX + opening, centerY + 1, ScreenWidth, centerY + 2);
	}
	else if (!IEngineStudio.IsHardware())
	{
		int opening = vgui2::scheme()->GetProportionalScaledValue(10);
		vgui2::surface()->DrawFilledRect(centerX - 1, 0, centerX, centerY - opening);
		vgui2::surface()->DrawFilledRect(centerX - 1, centerY + opening, centerX, ScreenHeight);
		vgui2::surface()->DrawFilledRect(0, centerY + 1, centerX - opening, centerY + 2);
		vgui2::surface()->DrawFilledRect(centerX + opening, centerY + 1, ScreenWidth, centerY + 2);
	}
	vgui2::surface()->DrawFilledRect(0, 0, ScreenWidth, top + 1);
	vgui2::surface()->DrawFilledRect(0, bottom - 1, ScreenWidth, ScreenHeight);
	vgui2::surface()->DrawFilledRect(0, top, left + 1, ScreenHeight);
	vgui2::surface()->DrawFilledRect(right - 1, top, ScreenWidth, ScreenHeight);
	return 1;
}
