#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "event_api.h"
#include "in_buttons.h"
#include "screenfade.h"
#include "shake.h"
#include "dod_shared.h"

extern int i_dodmusic;
extern int g_iDeadFlag;
extern client_textmessage_t g_pCustomMessage;
extern bool ShowHudElement( int element );
extern int HUD_GetWeaponAnim();
extern void CreateCorpse( Vector origin, Vector angles, const char *model, float animtime, int sequence, int body );

char *szCreditNames[14] =
{
	"",
	"Matt Boone",
	"Tim Holt",
	"Jakob Jungels",
	"John Morello II",
	"Brandon Russell",
	"Travis Smith",
	"Kelly Thornton",
	"Special Thanks to",
	"All the mappers, artists and voice actors who worked on Day of Defeat",
	"Everyone at Valve Software",
	"Our patient and loving families",
	"The DoD community - many thanks for your continued support",
	"We couldn't have done it without you all!"
};

DECLARE_MESSAGE(m_Icons, Health);
DECLARE_MESSAGE(m_Icons, Object);
DECLARE_MESSAGE(m_Icons, ClientAreas);
DECLARE_MESSAGE(m_Icons, ClCorpse);

void Credits()
{
	gHUD.m_Icons.StartDrawingCredits();
}

int CHudDodIcons::Init()
{
	HOOK_MESSAGE(Health);
	HOOK_MESSAGE(Object);
	HOOK_MESSAGE(ClientAreas);
	HOOK_MESSAGE(ClCorpse);
	m_iFlags |= HUD_ACTIVE;
	for ( int i = 0; i < 128; ++i )
	{
		m_Areas[i].nStatus = -1;
		m_Areas[i].hSpr = 0;
	}
	gHUD.AddHudElem(this);
	gEngfuncs.pfnAddCommand("credits", Credits);
	return 1;
}

void CHudDodIcons::Reset()
{
	m_iHealth = 100;
	m_hObjectSpr = 0;
	screenfade_t fade;
	fade.fader = fade.fadeg = fade.fadeb = fade.fadealpha = 0;
	fade.fadeFlags = FFADE_OUT | FFADE_STAYOUT;
	gEngfuncs.pfnSetScreenFade(&fade);
	m_iFlags |= HUD_ACTIVE;
	m_fLastMapMarkerTime = 0;
	i_dodmusic = 0;
}

int CHudDodIcons::VidInit()
{
	int sprite = gHUD.GetSpriteIndex("icon_mg_deploy");
	IconMGDeploy = gHUD.GetSprite(sprite);
	IconMGDeployArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_main");
	MainHUD = gHUD.GetSprite(sprite);
	MainHUDArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_health_overlay");
	HealthOverlayHUD = gHUD.GetSprite(sprite);
	HealthOverlayHUDArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_objectives");
	ObjectivesHUD = gHUD.GetSprite(sprite);
	ObjectivesHUDArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_staminabar");
	StaminaBarHUD = gHUD.GetSprite(sprite);
	StaminaBarHUDArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_heart");
	HeartHUD = gHUD.GetSprite(sprite);
	HeartHUDArea = &gHUD.GetSpriteRect(sprite);
	i_HeartFrame = 0;
	f_HeartSpeed = 0.07f;
	f_HeartTime = gHUD.m_flTime;
	sprite = gHUD.GetSpriteIndex("hud_healthbar");
	HealthBarHUD = gHUD.GetSprite(sprite);
	HealthBarHUDArea = &gHUD.GetSpriteRect(sprite);
	sprite = gHUD.GetSpriteIndex("hud_reinforcements");
	ReinforcementsHUD = gHUD.GetSprite(sprite);
	ReinforcementsHUDArea = &gHUD.GetSpriteRect(sprite);
	m_iIconHeight = IconMGDeployArea->bottom-IconMGDeployArea->top;
	m_iIconWidth = IconMGDeployArea->right-IconMGDeployArea->left;
	sprite = gHUD.GetSpriteIndex("hud_mapmarker");
	MapMarkerSprite = gHUD.GetSprite(sprite);
	MapMarkerArea = &gHUD.GetSpriteRect(sprite);
	int brit = gHUD.GetSpriteIndex("hint_brithead");
	int us = gHUD.GetSpriteIndex("hint_ushead");
	int german = gHUD.GetSpriteIndex("hint_gerhead");
	m_hHintHeads[0] = gHUD.GetSprite(brit);
	m_rectHintHeads[0] = &gHUD.GetSpriteRect(brit);
	m_hHintHeads[1] = gHUD.GetSprite(us);
	m_rectHintHeads[1] = &gHUD.GetSpriteRect(us);
	m_hHintHeads[2] = gHUD.GetSprite(german);
	m_rectHintHeads[2] = &gHUD.GetSpriteRect(german);
	int horiz = gHUD.GetSpriteIndex("hint_horiz");
	int vert = gHUD.GetSpriteIndex("hint_vert");
	m_hHorizHintBacking = gHUD.GetSprite(horiz);
	m_rectHorizHintBacking = &gHUD.GetSpriteRect(horiz);
	m_hVertHintBacking = gHUD.GetSprite(vert);
	m_rectVertHintBacking = &gHUD.GetSpriteRect(vert);
	m_iMsgX = m_iMsgY = 100;
	m_iCreditName = -1;
	m_flHintDieTime = 0;
	m_flCreditChangeTime = -1;
	for ( int i = 0; i < 128; ++i )
	{
		m_Areas[i].nStatus = -1;
		m_Areas[i].hSpr = 0;
	}
	m_hsprSelectedMarker = 0;
	m_flDrawSelectedMarkerTime = 0;
	return 1;
}

void CHudDodIcons::PlayerDied()
{
	m_iHealth = 0;
	if ( !g_iVuser1z )
	{
		gHUD.m_iFOV = 0;
		g_lastFOV = 0;
		gHUD.m_flMouseSensitivity = 0;
	}
	m_hObjectSpr = 0;
}

int CHudDodIcons::MsgFunc_Health( const char *, int size, void *buffer )
{
	BEGIN_READ(buffer,size);
	int health = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	m_iHealth = health;
	return 1;
}

int CHudDodIcons::MsgFunc_Object( const char *, int size, void *buffer )
{
	BEGIN_READ(buffer,size);
	m_szObjectIcon = READ_STRING();
	m_hObjectSpr = m_szObjectIcon[0] ? SPR_Load(m_szObjectIcon) : 0;
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

int CHudDodIcons::MsgFunc_ClientAreas( const char *, int size, void *buffer )
{
	BEGIN_READ(buffer,size);
	int index = READ_BYTE();
	int status = READ_BYTE();
	if ( index < 0 || index >= 128 )
		return 1;
	if ( status > 253 )
	{
		char *sprite = READ_STRING();
		if ( sprite[0] )
		{
			m_Areas[index].hSpr = SPR_Load(sprite);
			m_Areas[index].nStatus = 0;
		}
	}
	else if ( status == 0 || status == 1 )
		m_Areas[index].nStatus = status;
	else if ( status == 2 )
	{
		for ( int i = 0; i < 128; ++i )
			m_Areas[i].nStatus = 0;
	}
	return 1;
}

int CHudDodIcons::MsgFunc_ClCorpse( const char *, int size, void *buffer )
{
	BEGIN_READ(buffer,size);
	const char *player = READ_STRING();
	Vector origin, angles;
	origin.x = READ_COORD(); origin.y = READ_COORD(); origin.z = READ_COORD();
	angles.x = READ_ANGLE(); angles.y = READ_ANGLE(); angles.z = READ_ANGLE();
	int sequence = READ_BYTE();
	int body = READ_SHORT();
	READ_BYTE();
	char model[64];
	_snprintf(model,sizeof(model),"models/player/%s/%s.mdl",player,player);
	model[sizeof(model)-1] = 0;
	CreateCorpse(origin,angles,model,0,sequence,body);
	return 1;
}

void CHudDodIcons::DrawMarkerIcon( int marker )
{
	float time = gEngfuncs.GetClientTime();
	m_hsprSelectedMarker = marker;
	m_flDrawSelectedMarkerTime = time+1.0;
}

void CHudDodIcons::MapMarkerPosted()
{
	m_fLastMapMarkerTime = gEngfuncs.GetClientTime();
}

void CHudDodIcons::ActivateHintBacking( int team, float duration )
{
	m_iHintTeam = team == 2 ? 2 : !gHUD.m_bBritish;
	m_flHintDieTime = duration+gHUD.m_flTime;
}

void CHudDodIcons::GetHintMessageLocation( int &x, int &y )
{
	x = m_iMsgX;
	y = m_iMsgY;
}

void ShowHideWeapons( cl_entity_t *player )
{
	int weapon = gHUD.GetCurrentWeaponId();
	int animation = weapon == WEAPON_THOMPSON ? 6 : -1;
	if ( player->curstate.iuser3 <= 0 )
		gEngfuncs.pEventAPI->EV_WeaponAnimation(weapon == WEAPON_THOMPSON ? 0 : -1, 2);
	else if ( gHUD.m_iKeyBits & IN_FORWARD )
	{
		HUD_GetWeaponAnim();
		gEngfuncs.pEventAPI->EV_WeaponAnimation(animation, 2);
	}
}

void CHudDodIcons::StartDrawingCredits()
{
	m_iCreditName = 0;
	m_flCreditChangeTime = gHUD.m_flTime;
}

void CHudDodIcons::DrawCredits( float )
{
	if ( m_iCreditName < 0 || !(gHUD.m_flTime > m_flCreditChangeTime) )
		return;
	if ( ++m_iCreditName > 13 )
	{
		m_iCreditName = -1;
		return;
	}
	g_pCustomMessage.pName = "asdf";
	g_pCustomMessage.pMessage = szCreditNames[m_iCreditName];
	g_pCustomMessage.effect = 1;
	g_pCustomMessage.r1 = 18; g_pCustomMessage.g1 = 115; g_pCustomMessage.b1 = 11; g_pCustomMessage.a1 = 255;
	g_pCustomMessage.r2 = 0; g_pCustomMessage.g2 = 60; g_pCustomMessage.b2 = 0; g_pCustomMessage.a2 = 255;
	g_pCustomMessage.x = gEngfuncs.pfnRandomFloat(0.2,0.4);
	g_pCustomMessage.y = gEngfuncs.pfnRandomFloat(0.4,0.6);
	g_pCustomMessage.fadein = 0.7f;
	g_pCustomMessage.fadeout = 0.7f;
	g_pCustomMessage.fxtime = 0.07f;
	g_pCustomMessage.holdtime = 3;
	gHUD.m_Message.MessageAdd(&g_pCustomMessage);
	m_flCreditChangeTime = gHUD.m_flTime+5;
}

int CHudDodIcons::Draw( float time )
{
	DrawCredits(time);
	if ( (gHUD.m_iHideHUDDisplay & HIDEHUD_ALL) || g_iDeadFlag || g_iUser1 || g_iUser2 || g_iVuser1z )
		return 1;
	int r = 255, g = 255, b = 255;
	ScaleColors(r,g,b,100);
	r = g = b = 255;
	wrect_t area;
	if ( m_hObjectSpr )
	{
		area.left = area.top = 0; area.right = area.bottom = 64;
		SPR_Set(m_hObjectSpr,r,g,b);
		SPR_DrawAdditive(0,m_iIconWidth,(int)(m_iIconHeight+ScreenHeight/2-m_iIconHeight*1.5),&area);
	}
	int y = ScreenHeight+MainHUDArea->top-MainHUDArea->bottom;
	if ( ShowHudElement(1) )
	{
		SPR_Set(MainHUD,r,g,b);
		SPR_DrawHoles(0,0,y,MainHUDArea);
		SPR_Set(StaminaBarHUD,r,g,b);
		area = *StaminaBarHUDArea;
		float stamina = (double)(area.bottom-area.top)*g_fStamina/100.0;
		area.top = area.bottom-(int)stamina;
		SPR_DrawHoles(0,8,ScreenHeight-(int)stamina,&area);
		y = ScreenHeight+MainHUDArea->top-MainHUDArea->bottom;
		area = *HealthBarHUDArea;
		float health = (float)((double)(area.bottom-area.top)*m_iHealth/100.0);
		int offset = area.bottom-(area.top+(int)health+1);
		area.bottom = area.top+(int)health+1;
		SPR_Set(HealthBarHUD,r,g,b);
		SPR_DrawHoles(0,99,y+offset+28,&area);
		area = *HealthOverlayHUDArea;
		float overlay = (area.bottom-area.top)*(1.0-(double)m_iHealth/100.0);
		area.bottom = area.top+(int)overlay+1;
		if ( area.top < area.bottom )
		{
			SPR_Set(HealthOverlayHUD,r,g,b);
			SPR_DrawHoles(0,54,y+27,&area);
		}
	}
	y -= 64;
	if ( g_iVuser1x > 0 && IconMGDeploy && IconMGDeployArea != NULL )
	{
		SPR_Set(IconMGDeploy,220,220,220);
		SPR_DrawAdditive(0,10,y+IconMGDeployArea->top-IconMGDeployArea->bottom,IconMGDeployArea);
		y += IconMGDeployArea->top-IconMGDeployArea->bottom;
	}
	area.left = area.top = 0; area.right = area.bottom = 64;
	for ( int i = 0; i < 128; ++i )
	{
		if ( m_Areas[i].nStatus > 0 && m_Areas[i].hSpr )
		{
			SPR_Set(m_Areas[i].hSpr,r,g,b);
			SPR_DrawAdditive(0,10,y+area.top-area.bottom,&area);
		}
	}
	if ( ShowHudElement(2) )
	{
		SPR_Set(ReinforcementsHUD,r,g,b);
		SPR_DrawHoles(0,113,ScreenHeight+ReinforcementsHUDArea->top-ReinforcementsHUDArea->bottom,ReinforcementsHUDArea);
	}
	if ( ShowHudElement(6) )
	{
		float now = gEngfuncs.GetClientTime();
		if ( m_fLastMapMarkerTime != 0 && m_fLastMapMarkerTime+1.0 > now )
		{
			int red = 255, green = 255, blue = 255;
			float alpha = (1.0-(now-m_fLastMapMarkerTime))*255.0;
			ScaleColors(red,green,blue,(int)alpha);
			float offset = (double)ScreenWidth/640.0+(double)ScreenWidth/640.0+0.5;
			int x = MapMarkerArea->left+ScreenWidth-(int)offset-MapMarkerArea->right;
			int markerY = ScreenHeight/2+2*(MapMarkerArea->top-MapMarkerArea->bottom);
			SPR_Set(MapMarkerSprite,red,green,blue);
			SPR_DrawAdditive(0,x,markerY,MapMarkerArea);
		}
	}
	if ( m_flDrawSelectedMarkerTime > gEngfuncs.GetClientTime() && m_hsprSelectedMarker )
	{
		gEngfuncs.GetClientTime();
		int width = ScreenWidth;
		int markerY = ScreenHeight/2;
		SPR_Set(m_hsprSelectedMarker,r,g,b);
		area.left = area.top = 0; area.right = area.bottom = 64;
		float offset = (double)width/640.0+(double)width/640.0+0.5;
		SPR_DrawHoles(0,width-(int)offset-96,markerY-64,&area);
		width = ScreenWidth;
		offset = (double)width/640.0+(double)width/640.0+0.5;
		gHUD.m_VGUI2Print.DrawVGUI2StringReverse("Dod_current_marker",width-(int)offset,markerY,1,1,1);
	}
	return 1;
}
