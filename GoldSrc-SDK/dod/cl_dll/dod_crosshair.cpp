#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "dod_shared.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;
extern cvar_t *cl_dynamic_xhair;
extern cvar_t *cl_xhair_style;
extern int g_iDeadFlag;
extern int g_ihidexhair;

float flBoltHideXHair;

DECLARE_MESSAGE(m_DoDCrossHair, ClanTimer);

int CHudDoDCrossHair::Init()
{
	HOOK_MESSAGE(ClanTimer);
	gHUD.AddHudElem(this);
	m_iYPos = ScreenHeight/2;
	m_iXPos = ScreenWidth/2;
	m_iFlags |= HUD_ACTIVE;
	m_fMoveTime = 0;
	return 1;
}

void CHudDoDCrossHair::Reset()
{
}

int CHudDoDCrossHair::VidInit()
{
	int horiz = gHUD.GetSpriteIndex("cross_style_2_horiz");
	int vert = gHUD.GetSpriteIndex("cross_style_2_vert");
	int dot = gHUD.GetSpriteIndex("cross_style_2_dot");
	CrossSprite2horiz = gHUD.GetSprite(horiz);
	CrossArea2horiz = &gHUD.GetSpriteRect(horiz);
	CrossSprite2vert = gHUD.GetSprite(vert);
	CrossArea2vert = &gHUD.GetSpriteRect(vert);
	CrossSprite2dot = gHUD.GetSprite(dot);
	CrossArea2dot = &gHUD.GetSpriteRect(dot);
	flBoltHideXHair = 0;
	m_fClanTimer = 0;
	m_hCrosshair = SPR_Load("sprites/crosshairs.spr");
	memset(&m_crosshairRect, 0, sizeof(m_crosshairRect));
	m_hCustomCrosshair = SPR_Load("sprites/customXHair.spr");
	memset(&m_customCrosshairRect, 0, sizeof(m_customCrosshairRect));
	return 1;
}

int CHudDoDCrossHair::MsgFunc_ClanTimer( const char *, int size, void *buffer )
{
	BEGIN_READ(buffer, size);
	float time = gHUD.m_flTime;
	m_fClanTimer = READ_BYTE()+time;
	return 1;
}

void CHudDoDCrossHair::DrawCustomCrossHair( int style )
{
	style = min(style, 16)-1;
	m_customCrosshairRect.left = style%4*64;
	m_customCrosshairRect.top = style/4*64;
	m_customCrosshairRect.right = (style%4+1)*64;
	m_customCrosshairRect.bottom = (style/4+1)*64;
	wrect_t area = m_customCrosshairRect;
	SPR_Set(m_hCustomCrosshair, 255,255,255);
	SPR_DrawHoles(0, ScreenWidth/2-(area.right-area.left)/2,
		ScreenHeight/2-(area.bottom-area.top)/2, &area);
}

float CHudDoDCrossHair::GetCurrentWeaponAccuracy()
{
	int weapon = gHUD.GetCurrentWeaponId();
	float accuracy = P_WpnInfo[weapon].base_accuracy;
	if ( gHUD.IsInMGDeploy() )
		accuracy = P_WpnInfo[weapon].base_accuracy2;
	if ( gHUD.m_vecVelocity.Length() > 105 )
		accuracy += P_WpnInfo[weapon].accuracy_penalty;
	return accuracy;
}

int CHudDoDCrossHair::GetCrossHairWidth()
{
	if ( cl_dynamic_xhair != NULL && !(cl_dynamic_xhair->value >= 1) )
		return 4;
	switch ( gHUD.GetCurrentWeaponId() )
	{
	case WEAPON_HANDGRENADE:
	case WEAPON_STICKGRENADE:
	case WEAPON_STICKGRENADE_EX:
	case WEAPON_HANDGRENADE_EX:
	case WEAPON_BAZOOKA:
	case WEAPON_PSCHRECK:
	case WEAPON_PIAT:
		return 4;
	}
	float width = GetCurrentWeaponAccuracy()*200;
	int result = (int)width+2;
	if ( result < m_iLastXHairWidth )
	{
		if ( result < m_iLastXHairWidth-2 )
			result = m_iLastXHairWidth-3;
		m_iLastXHairWidth = result;
	}
	else if ( result > m_iLastXHairWidth )
	{
		if ( result > m_iLastXHairWidth+2 )
			result = m_iLastXHairWidth+3;
		m_iLastXHairWidth = result;
	}
	return result;
}

void CHudDoDCrossHair::DrawDynamicCrossHair()
{
	int gap = GetCrossHairWidth();
	int x = ScreenWidth/2, y = ScreenHeight/2;
	SPR_Set(CrossSprite2dot, 255,255,255);
	SPR_DrawHoles(0,x-1,y-1,CrossArea2dot);
	int width = CrossArea2horiz->right-CrossArea2horiz->left;
	int height = CrossArea2horiz->bottom-CrossArea2horiz->top;
	SPR_Set(CrossSprite2horiz,255,255,255);
	SPR_DrawHoles(0,x-gap-width+2,y-height/2,CrossArea2horiz);
	SPR_DrawHoles(0,x+gap,y-height/2,CrossArea2horiz);
	height = CrossArea2vert->bottom-CrossArea2vert->top;
	SPR_Set(CrossSprite2vert,255,255,255);
	SPR_DrawHoles(0,x-1,y-gap-height+2,CrossArea2vert);
	SPR_DrawHoles(0,x-1,y+gap,CrossArea2vert);
}

bool CHudDoDCrossHair::ShouldDrawCrossHair()
{
	if ( (gHUD.m_iHideHUDDisplay & (HIDEHUD_ALL|HIDEHUD_WEAPONS)) || gHUD.m_Icons.m_iHealth <= 0 || g_iDeadFlag
		|| g_iVuser1z || gHUD.GetMinimapState() == 1 )
		return false;
	int weapon = gHUD.GetCurrentWeaponId();
	if ( weapon == -1 || (!gHUD.m_iWeaponBits && !g_iWeaponBits2) || !(flBoltHideXHair <= 0) || g_ihidexhair )
		return false;
	if ( (unsigned int)weapon <= 2 || weapon == WEAPON_SPADE || weapon == WEAPON_MORTAR
		|| (weapon == WEAPON_FG42 && gHUD.m_iFOV <= 89) )
		return false;
	if ( (weapon == WEAPON_MG42 || weapon == WEAPON_30CAL || weapon == WEAPON_MG34) && !gHUD.IsInMGDeploy() )
		return false;
	bool scoped = weapon == WEAPON_SPRING || weapon == WEAPON_BINOCULARS || weapon == WEAPON_SCOPEDKAR
		|| (weapon == WEAPON_ENFIELD && (g_iWeaponFlags & 1));
	bool software = !IEngineStudio.IsHardware();
	return (software && scoped && gHUD.m_iFOV <= 89) || !scoped;
}

void CHudDoDCrossHair::DrawSpectatorCrossHair()
{
	if ( (unsigned int)(gHUD.m_iFOV-1) <= 88 )
		return;
	m_crosshairRect.left = 24;
	m_crosshairRect.right = 48;
	m_crosshairRect.top = 0;
	m_crosshairRect.bottom = 24;
	wrect_t area = m_crosshairRect;
	SPR_Set(m_hCrosshair,255,255,255);
	SPR_DrawHoles(0,ScreenWidth/2-12,ScreenHeight/2-12,&area);
}

void CHudDoDCrossHair::DrawClanTimer( float )
{
	if ( !(m_fClanTimer > gHUD.m_flTime) )
		return;
	float remaining = m_fClanTimer-gHUD.m_flTime;
	int minutes = (int)remaining/60;
	int seconds = (int)remaining-minutes*60;
	char message[256];
	if ( seconds > 1 || minutes > 0 )
	{
		strcpy(message,"#Clan_time_remaining");
		float color = seconds <= 5 || (!(seconds & 1) && seconds <= 20) ? 0.0f : 1.0f;
		char mins[6], secs[18];
		sprintf(mins,"%d",minutes);
		sprintf(secs,"%d",seconds);
		gHUD.m_VGUI2Print.VGUI2HudPrintArgs(message,mins,secs,NULL,NULL,-1,ScreenHeight/3,1,color,color);
	}
	else
	{
		strcpy(message,"#Clan_match_live");
		gHUD.m_VGUI2Print.VGUI2HudPrintArgs(message,NULL,NULL,NULL,NULL,-1,ScreenHeight/3,1,0,0);
	}
}

int CHudDoDCrossHair::Draw( float time )
{
	DrawClanTimer(time);
	if ( !g_iUser1 )
	{
		if ( !g_iUser2 && ShouldDrawCrossHair() )
		{
			int style = (int)cl_xhair_style->value;
			if ( style ) DrawCustomCrossHair(style);
			else DrawDynamicCrossHair();
		}
	}
	else if ( g_iUser1 == 3 || g_iUser1 == 4 )
		DrawSpectatorCrossHair();
	return 1;
}
