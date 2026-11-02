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
// flashlight.cpp
//
// implementation of CHudFlashlight class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui/iczeroviewport.h"
#include "pm_shared.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

extern bool g_bRenderFog;
extern int g_iFogSkybox;
extern float g_flFogDensity;
extern float g_iFogColor[3];
extern float g_flFogStartDistance;
extern float g_flFogStopDistance;

DECLARE_MESSAGE( m_Flash, Flashlight )
DECLARE_MESSAGE( m_Flash, FlashBat )
DECLARE_MESSAGE( m_NightVision, NightVision )
DECLARE_MESSAGE( m_SniperLaser, SniperLaser )
DECLARE_MESSAGE( m_FiberCamera, FiberCamera )
DECLARE_MESSAGE( m_HudIcons, HudIcons )
DECLARE_MESSAGE( m_progressBar, BarTime )
DECLARE_MESSAGE( m_levelLocks, LevelLocks )

int CHudFlashlight::Init()
{
	m_fFade = 0;
	m_fOn = 0;
	m_iconAlpha = 225;
	m_blinkIcon = false;
	HOOK_MESSAGE( Flashlight );
	HOOK_MESSAGE( FlashBat );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudFlashlight::Reset()
{
	m_fFade = 0;
	m_fOn = 0;
	m_iconAlpha = 225;
	m_blinkIcon = false;
}

int CHudFlashlight::VidInit()
{
	int empty = gHUD.GetSpriteIndex( "flash_empty" );
	int full = gHUD.GetSpriteIndex( "flash_full" );
	int beam = gHUD.GetSpriteIndex( "flash_beam" );
	m_hSprite1 = gHUD.GetSprite( empty );
	m_hSprite2 = gHUD.GetSprite( full );
	m_hBeam = gHUD.GetSprite( beam );
	m_prc1 = &gHUD.GetSpriteRect( empty );
	m_prc2 = &gHUD.GetSpriteRect( full );
	m_prcBeam = &gHUD.GetSpriteRect( beam );
	m_iWidth = m_prc2->right - m_prc2->left;
	return 1;
}

int CHudFlashlight::MsgFunc_FlashBat( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_iBat = READ_BYTE();
	m_flBat = (float)( m_iBat / 100.0 );
	return 1;
}

int CHudFlashlight::MsgFunc_Flashlight( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_fOn = READ_BYTE();
	m_iBat = READ_BYTE();
	m_flBat = (float)( m_iBat / 100.0 );
	return 1;
}

int CHudFlashlight::Draw( float )
{
	if ( gHUD.m_iWeaponBits >= 0 )
		return 1;
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_FLASHLIGHT | HIDEHUD_ALL ) )
	{
		if ( gHUD.m_iHideHUDDisplay & HIDEHUD_FLASHLIGHT )
			m_blinkIcon = true;
		return 1;
	}
	int alpha = 0;
	if ( m_blinkIcon )
	{
		m_nextBlinkTime = gEngfuncs.GetClientTime() + 0.25f;
		m_blinkStopTime = gEngfuncs.GetClientTime() + 2;
		m_blinkIcon = false;
	}
	float now = gEngfuncs.GetClientTime();
	if ( now <= m_blinkStopTime )
	{
		if ( now > m_nextBlinkTime )
		{
			if ( m_iconAlpha <= 0 )
			{
				m_iconAlpha = m_fOn ? 225.0f : 200.0f;
				alpha = (int)m_iconAlpha;
			}
			else
				m_iconAlpha = 0;
			m_nextBlinkTime = now + 0.25f;
		}
		else
			alpha = (int)m_iconAlpha;
	}
	else
	{
		m_iconAlpha = m_fOn ? 225.0f : 200.0f;
		alpha = (int)m_iconAlpha;
	}
	int r, g, b;
	if ( m_flBat >= 0.2 )
	{
		r = 64;
		g = 255;
		b = 64;
	}
	else
	{
		r = 255;
		g = 16;
		b = 16;
	}
	ScaleColors( r, g, b, alpha );
	int y = ( m_prc1->bottom - m_prc2->top ) / 2;
	int x = ScreenWidth - m_iWidth - m_iWidth / 2;

	// Draw the flashlight casing
	SPR_Set( m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0, x, y, m_prc1 );
	if ( m_fOn )
	{  // draw the flashlight beam
		SPR_Set( m_hBeam, r, g, b );
		SPR_DrawAdditive( 0, ScreenWidth - m_iWidth / 2, y, m_prcBeam );
	}
	int missing = (int)( ( 1.0 - m_flBat ) * m_iWidth );
	if ( m_iWidth > missing )
	{
		wrect_t rect = *m_prc2;
		rect.left += missing;
		SPR_Set( m_hSprite2, r, g, b );
		SPR_DrawAdditive( 0, ScreenWidth - m_iWidth - m_iWidth / 2 + missing, y, &rect );
	}
	return 1;
}

int CHudNightVision::Init()
{
	m_Fade = 0;
	m_On = false;
	m_iconAlpha = 225;
	m_blinkIcon = true;
	HOOK_MESSAGE( NightVision );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudNightVision::Reset()
{
	m_Fade = 0;
	m_On = false;
	m_iconAlpha = 225;
	m_blinkIcon = true;
}

int CHudNightVision::VidInit()
{
	int sprite = gHUD.GetSpriteIndex( "icon_nightvision" );
	m_hSprite1 = gHUD.GetSprite( sprite );
	m_prc1 = &gHUD.GetSpriteRect( sprite );
	return 1;
}

int CHudNightVision::MsgFunc_NightVision( const char*, int size, void* buffer )
{
	static bool oldRenderFog = g_bRenderFog;
	static int oldFogSkybox = g_iFogSkybox;
	static float oldFogDensity = g_flFogDensity;
	static float oldFogColorRed = g_iFogColor[0];
	static float oldFogColorGreen = g_iFogColor[1];
	static float oldFogColorBlue = g_iFogColor[2];
	static float oldFogStartDistance = g_flFogStartDistance;
	static float oldFogStopDistance = g_flFogStopDistance;
	BEGIN_READ( buffer, size );
	int enabled = READ_BYTE();
	if ( enabled == 2 )
		return 1;
	m_On = enabled != 0;
	if ( enabled )
	{
		oldRenderFog = g_bRenderFog;
		oldFogSkybox = g_iFogSkybox;
		oldFogDensity = g_flFogDensity;
		oldFogColorRed = g_iFogColor[0];
		oldFogColorGreen = g_iFogColor[1];
		oldFogColorBlue = g_iFogColor[2];
		oldFogStartDistance = g_flFogStartDistance;
		oldFogStopDistance = g_flFogStopDistance;
		g_bRenderFog = true;
		g_flFogDensity = 0.0025f;
		g_iFogSkybox = 1;
		g_iFogColor[0] = 0;
		g_iFogColor[1] = 1;
		g_iFogColor[2] = 0;
		g_flFogStartDistance = 200;
		g_flFogStopDistance = 350;
	}
	else
	{
		g_bRenderFog = oldRenderFog;
		g_flFogDensity = oldFogDensity;
		g_iFogColor[0] = oldFogColorRed;
		g_iFogColor[1] = oldFogColorGreen;
		g_iFogColor[2] = oldFogColorBlue;
		g_flFogStartDistance = oldFogStartDistance;
		g_flFogStopDistance = oldFogStopDistance;
		g_iFogSkybox = oldFogSkybox;
	}
	gEngfuncs.pfnSetFilterMode( enabled );
	gEngfuncs.pfnSetFilterColor( 0.1f, 0.9f, 0.1f );
	gEngfuncs.pfnSetFilterBrightness( 0.65f );
	return 1;
}

int CHudNightVision::Draw( float )
{
	if ( gHUD.m_iHideHUDDisplay & ( HIDEHUD_ALL | HIDEHUD_NIGHTVISION ) )
	{
		m_blinkIcon = true;
		return 1;
	}
	int alpha = 0;
	if ( m_blinkIcon )
	{
		m_nextBlinkTime = gEngfuncs.GetClientTime() + 0.25f;
		m_blinkStopTime = gEngfuncs.GetClientTime() + 2;
		m_blinkIcon = false;
	}
	float now = gEngfuncs.GetClientTime();
	if ( now > m_blinkStopTime )
	{
		m_iconAlpha = 225;
		alpha = 225;
	}
	else if ( now <= m_nextBlinkTime )
		alpha = (int)m_iconAlpha;
	else
	{
		m_iconAlpha = m_iconAlpha <= 0 ? 225.0f : 0.0f;
		alpha = (int)m_iconAlpha;
		m_nextBlinkTime = now + 0.25f;
	}
	int r = 64, g = 255, b = 64;
	ScaleColors( r, g, b, alpha );
	int width = m_prc1->right - m_prc1->left;
	int x = ScreenWidth - width - width / 2 + 11;
	int y = (int)( ( m_prc1->bottom - m_prc1->top ) / 2 + 30.0f );
	SPR_Set( m_hSprite1, r, g, b );
	SPR_DrawAdditive( 0, x, y, m_prc1 );
	return 1;
}

int CHudSniperLaser::Init()
{
	m_Fade = 0;
	m_On = false;
	HOOK_MESSAGE( SniperLaser );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudSniperLaser::Reset()
{
	m_Fade = 0;
	m_On = false;
}

int CHudSniperLaser::VidInit()
{
	return 1;
}

int CHudSniperLaser::Draw( float )
{
	return 1;
}

int CHudSniperLaser::MsgFunc_SniperLaser( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int enabled = READ_BYTE();
	m_On = enabled != 0;
	gEngfuncs.pfnSetFilterMode( enabled );
	gEngfuncs.pfnSetFilterColor( 0.9f, 0.1f, 0.1f );
	gEngfuncs.pfnSetFilterBrightness( 0.65f );
	return 1;
}

int CHudLevelLocks::Init()
{
	HOOK_MESSAGE( LevelLocks );
	return 1;
}

int CHudLevelLocks::MsgFunc_LevelLocks( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	gHUD.m_levelLockBits = 0;
	int bits = READ_LONG();
	gHUD.m_levelLockBits |= (int)CVAR_GET_FLOAT( "cl_levellocks" ) | bits;
	gEngfuncs.Cvar_SetValue( "cl_levellocks", (float)gHUD.m_levelLockBits );
	int current = 0;
	for ( int i = 13; i > 0; --i )
	{
		if ( gHUD.m_levelLockBits & ( 1 << i ) )
		{
			current = i;
			break;
		}
	}
	const char* missions[] =
	{
		"cz_train", "cz_recoil", "cz_lostcause", "cz_silo", "cz_brecon",
		"cz_druglab", "cz_motor", "cz_thinice", "cz_downed", "cz_hankagai",
		"cz_turncrank", "cz_alamo", "cz_hr", "cz_truth", "cz_run",
		"cz_miami", "cz_fastline", "cz_pipedream", "cz_sandstorm",
	};
	for ( int i = 1; i < 32; ++i )
	{
		if ( i > ARRAYSIZE( missions ) )
			continue;
		if ( ( gHUD.m_levelLockBits >> i ) & 1 )
		{
			if ( current == i )
				gCzeroViewPortInterface->CurrentMission( missions[i - 1] );
			else
				gCzeroViewPortInterface->UnlockMission( missions[i - 1] );
		}
		else
			gCzeroViewPortInterface->LockMission( missions[i - 1] );
	}
	return 1;
}

int CHudProgressBar::Init()
{
	HOOK_MESSAGE( BarTime );
	gHUD.AddHudElem( this );
	Reset();
	return 1;
}

int CHudProgressBar::VidInit()
{
	return 1;
}

void CHudProgressBar::Reset()
{
	m_iFlags |= HUD_ACTIVE;
	m_tmEnd = 0;
	m_tmNewPeriod = 0;
	m_height = 10;
	m_width = (int)( ScreenWidth * 0.5f );
	m_x0 = m_width / 2;
	m_x1 = m_width + m_x0 - 1;
	m_y0 = (int)( ScreenHeight * 0.5f + 64 );
	m_y1 = m_y0 + 9;
}

int CHudProgressBar::MsgFunc_BarTime( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	int period = READ_SHORT();
	m_tmNewPeriod = period ? (float)period : -1.0f;
	return 1;
}

int CHudProgressBar::Draw( float time )
{
	if ( m_tmNewPeriod == 0 )
	{
		if ( time > m_tmEnd || m_tmStart > time )
			return 1;
	}
	else
	{
		m_tmEnd = m_tmNewPeriod + time;
		m_tmNewPeriod = 0;
		m_tmStart = time;
		if ( time > m_tmEnd )
			return 1;
	}
	FillRGBA( m_x0, m_y0, 1, m_height, 64, 255, 64, 80 );
	FillRGBA( m_x1, m_y0, 1, m_height, 64, 255, 64, 80 );
	FillRGBA( m_x0 + 1, m_y0, m_width - 2, 1, 64, 255, 64, 80 );
	FillRGBA( m_x0 + 1, m_y1, m_width - 2, 1, 64, 255, 64, 80 );
	float progress = ( m_width - 4 ) * ( time - m_tmStart ) / ( m_tmEnd - m_tmStart );
	int width = (int)progress < m_width - 3 ? (int)progress : m_width - 4;
	FillRGBA( m_x0 + 2, m_y0 + 2, width, m_height - 4, 64, 255, 64, 160 );
	return 1;
}

int CHudIcons::Init()
{
	m_icons = 0;
	m_iconsCorrectWeapon = 0;
	m_iconsInPosition = 0;
	HOOK_MESSAGE( HudIcons );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudIcons::Reset()
{
	m_icons = 0;
}

int CHudIcons::MsgFunc_HudIcons( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_icons = READ_SHORT();
	m_iconsCorrectWeapon = READ_SHORT();
	m_iconsInPosition = READ_SHORT();
	return 1;
}

int CHudIcons::VidInit()
{
	const char* names[] =
	{
		"icon_blowtorch", "icon_briefcase", "icon_camera", "icon_fibercamera",
		"icon_radio", "icon_radiocontrolledbomb", "icon_stealth", "icon_syringe",
		"icon_zipline", "icon_bombdefuse", "icon_hostagezone",
	};
	HSPRITE* sprites[] =
	{
		&m_hBlowtorchSprite, &m_hBriefcaseSprite, &m_hCameraSprite, &m_hFiberCameraSprite,
		&m_hRadioSprite, &m_hRadioBombSprite, &m_hStealthSprite, &m_hSyringeSprite,
		&m_hZiplineSprite, &m_hBombDefuseSprite, &m_hHostageZoneSprite,
	};
	wrect_t** rects[] =
	{
		&m_hBlowtorchSpriteRect, &m_hBriefcaseSpriteRect, &m_hCameraSpriteRect, &m_hFiberCameraSpriteRect,
		&m_hRadioSpriteRect, &m_hRadioBombSpriteRect, &m_hStealthSpriteRect, &m_hSyringeSpriteRect,
		&m_hZiplineSpriteRect, &m_hBombDefuseSpriteRect, &m_hHostageZoneSpriteRect,
	};
	for ( int i = 0; i < ARRAYSIZE( names ); ++i )
		*sprites[i] = gHUD.GetSprite( gHUD.GetSpriteIndex( names[i] ) );
	for ( int i = 0; i < ARRAYSIZE( names ); ++i )
		*rects[i] = &gHUD.GetSpriteRect( gHUD.GetSpriteIndex( names[i] ) );
	return 1;
}

int CHudIcons::Draw( float )
{
	if ( ( gHUD.m_iHideHUDDisplay & HIDEHUD_ALL ) || !HasIcons() )
		return 1;
	const int bits[] = { 1, 1024, 512, 2, 4, 8, 16, 32, 64, 128, 256 };
	HSPRITE* sprites[] =
	{
		&m_hBlowtorchSprite, &m_hHostageZoneSprite, &m_hBombDefuseSprite, &m_hBriefcaseSprite,
		&m_hCameraSprite, &m_hFiberCameraSprite, &m_hRadioSprite, &m_hRadioBombSprite,
		&m_hStealthSprite, &m_hSyringeSprite, &m_hZiplineSprite,
	};
	wrect_t** rects[] =
	{
		&m_hBlowtorchSpriteRect, &m_hHostageZoneSpriteRect, &m_hBombDefuseSpriteRect, &m_hBriefcaseSpriteRect,
		&m_hCameraSpriteRect, &m_hFiberCameraSpriteRect, &m_hRadioSpriteRect, &m_hRadioBombSpriteRect,
		&m_hStealthSpriteRect, &m_hSyringeSpriteRect, &m_hZiplineSpriteRect,
	};
	int y = 145;
	for ( int i = 0; i < ARRAYSIZE( bits ); ++i )
	{
		int bit = bits[i];
		if ( !( m_icons & bit ) )
			continue;
		int r = 255, g = 0, b = 0;
		if ( bit == 1024 )
		{
			r = 0;
			g = 255;
		}
		else if ( bit == 64 )
			r = g = b = 128;
		else if ( bit == 128 || bit == 256 )
			r = g = b = 255;
		else if ( ( m_iconsCorrectWeapon & bit ) && ( m_iconsInPosition & bit ) )
		{
			r = 0;
			g = 255;
		}
		else if ( ( ( m_iconsCorrectWeapon & bit ) != 0 ) != ( ( m_iconsInPosition & bit ) != 0 ) )
			g = 128;
		ScaleColors( r, g, b, 255 );
		SPR_Set( *sprites[i], r, g, b );
		SPR_DrawAdditive( 0, 20, y, *rects[i] );
		y += 65;
	}
	Reset();
	return 1;
}

int CHudFiberCamera::Init()
{
	m_On = false;
	HOOK_MESSAGE( FiberCamera );
	m_iFlags |= HUD_ACTIVE;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudFiberCamera::Reset()
{
	m_On = false;
}

int CHudFiberCamera::MsgFunc_FiberCamera( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_On = READ_BYTE() != 0;
	if ( m_On )
	{
		g_iUser2 = READ_SHORT();
		viewOrigin.x = READ_COORD();
		viewOrigin.y = READ_COORD();
		viewOrigin.z = READ_COORD();
		viewAngles.x = READ_COORD();
		viewAngles.y = READ_COORD();
		viewAngles.z = READ_COORD();
		if ( viewAngles.x > 180 )
			viewAngles.x -= 360;
		if ( viewAngles.x < -180 )
			viewAngles.x += 360;
		gHUD.m_Spectator.SetModes( OBS_IN_EYE, -1 );
		firstTime = true;
	}
	else
	{
		g_iUser1 = 0;
		g_iUser2 = 0;
	}
	return 1;
}

CHudFiberCamera::~CHudFiberCamera()
{
	delete m_hTL;
	delete m_hT;
	delete m_hTR;
	delete m_hL;
	delete m_hR;
	delete m_hBL;
	delete m_hB;
	delete m_hBR;
	delete m_hTL2;
	delete m_hT2;
	delete m_hTR2;
	delete m_hL2;
	delete m_hR2;
	delete m_hBL2;
	delete m_hB2;
	delete m_hBR2;
	delete m_hTL3;
	delete m_hT3;
	delete m_hTR3;
	delete m_hL3;
	delete m_hR3;
	delete m_hBL3;
	delete m_hB3;
	delete m_hBR3;
	delete m_hHor;
	delete m_hVer;
}

int CHudFiberCamera::VidInit()
{
	wrect_t** rects[3][8] =
	{
		{ &m_hTL, &m_hT, &m_hTR, &m_hL, &m_hR, &m_hBL, &m_hB, &m_hBR },
		{ &m_hTL2, &m_hT2, &m_hTR2, &m_hL2, &m_hR2, &m_hBL2, &m_hB2, &m_hBR2 },
		{ &m_hTL3, &m_hT3, &m_hTR3, &m_hL3, &m_hR3, &m_hBL3, &m_hB3, &m_hBR3 },
	};
	HSPRITE* sprites[3][8] =
	{
		{ &m_hTopLeft, &m_hTop, &m_hTopRight, &m_hLeft, &m_hRight, &m_hBottomLeft, &m_hBottom, &m_hBottomRight },
		{ &m_hTopLeft2, &m_hTop2, &m_hTopRight2, &m_hLeft2, &m_hRight2, &m_hBottomLeft2, &m_hBottom2, &m_hBottomRight2 },
		{ &m_hTopLeft3, &m_hTop3, &m_hTopRight3, &m_hLeft3, &m_hRight3, &m_hBottomLeft3, &m_hBottom3, &m_hBottomRight3 },
	};
	const char* names[3][8] =
	{
		{ "top_left", "top", "top_right", "left", "right", "bottom_left", "bottom", "bottom_right" },
		{ "top_left2", "top2", "top_right2", "left2", "right2", "bottom_left2", "bottom2", "bottom_right2" },
		{ "top_left3", "top3", "top_right3", "left3", "right3", "bottom_left3", "bottom3", "bottom_right3" },
	};
	const char* paths[3][8] =
	{
		{ "sprites/top_left.spr", "sprites/top.spr", "sprites/top_right.spr", "sprites/left.spr",
			"sprites/right.spr", "sprites/bottom_left.spr", "sprites/bottom.spr", "sprites/bottom_right.spr" },
		{ "sprites/top_left2.spr", "sprites/top2.spr", "sprites/top_right2.spr", "sprites/left2.spr",
			"sprites/right2.spr", "sprites/bottom_left2.spr", "sprites/bottom2.spr", "sprites/bottom_right2.spr" },
		{ "sprites/top_left3.spr", "sprites/top3.spr", "sprites/top_right3.spr", "sprites/left3.spr",
			"sprites/right3.spr", "sprites/bottom_left3.spr", "sprites/bottom3.spr", "sprites/bottom_right3.spr" },
	};
	for ( int group = 0; group < 3; ++group )
	{
		for ( int i = 0; i < 8; ++i )
			*rects[group][i] = new wrect_t;
	}
	m_hHor = new wrect_t;
	m_hVer = new wrect_t;
	m_hTopLeftSmall = SPR_Load( "sprites/top_left_small.spr" );
	m_hTopRightSmall = SPR_Load( "sprites/top_right_small.spr" );
	m_hBottomLeftSmall = SPR_Load( "sprites/bottom_left_small.spr" );
	m_hBottomRightSmall = SPR_Load( "sprites/bottom_right_small.spr" );
	for ( int group = 0; group < 3; ++group )
	{
		for ( int i = 0; i < 8; ++i )
			gHUD.GetSpriteIndex( names[group][i] );
		for ( int i = 0; i < 8; ++i )
		{
			wrect_t& rect = **rects[group][i];
			rect.left = 0;
			rect.top = 0;
			rect.right = group == 0 && i != 1 && i != 6 ? 192 : 256;
			rect.bottom = group == 2 || i == 3 || i == 4 ? 256 : group == 0 ? 112 : 172;
		}
		for ( int i = 0; i < 8; ++i )
			*sprites[group][i] = SPR_Load( paths[group][i] );
	}
	gHUD.GetSpriteIndex( "horizontal" );
	gHUD.GetSpriteIndex( "vertical" );
	m_hHor->left = 0;
	m_hHor->top = 0;
	m_hHor->right = 256;
	m_hHor->bottom = 32;
	m_hVer->left = 0;
	m_hVer->top = 0;
	m_hVer->right = 40;
	m_hVer->bottom = 256;
	m_hHorizontal = SPR_Load( "sprites/horizontal.spr" );
	m_hVertical = SPR_Load( "sprites/vertical.spr" );
	return 1;
}

int CHudFiberCamera::Draw( float )
{
	if ( !IEngineStudio.IsHardware() || !m_On )
		return 1;
	int centerX = ScreenWidth / 2;
	int centerY = ScreenHeight / 2;
	if ( ScreenWidth < 640 )
	{
		HSPRITE sprites[] = { m_hTopLeftSmall, m_hTopRightSmall, m_hBottomLeftSmall, m_hBottomRightSmall };
		const char* names[] = { "top_left", "top_right", "bottom_left", "bottom_right" };
		for ( int i = 0; i < 4; ++i )
		{
			int x = i & 1 ? centerX : ScreenWidth <= 511 ? -48 : 0;
			int y = i >= 2 ? centerY - ( ScreenWidth <= 511 ? 32 : 0 ) : 0;
			SPR_Set( sprites[i], 255, 255, 255 );
			SPR_DrawHoles( 0, x, y, &gHUD.GetSpriteRect( gHUD.GetSpriteIndex( names[i] ) ) );
		}
	}
	else
	{
		HSPRITE sprites[3][8] =
		{
			{ m_hTopLeft, m_hTop, m_hTopRight, m_hLeft, m_hRight, m_hBottomLeft, m_hBottom, m_hBottomRight },
			{ m_hTopLeft2, m_hTop2, m_hTopRight2, m_hLeft2, m_hRight2, m_hBottomLeft2, m_hBottom2, m_hBottomRight2 },
			{ m_hTopLeft3, m_hTop3, m_hTopRight3, m_hLeft3, m_hRight3, m_hBottomLeft3, m_hBottom3, m_hBottomRight3 },
		};
		wrect_t* rects[3][8] =
		{
			{ m_hTL, m_hT, m_hTR, m_hL, m_hR, m_hBL, m_hB, m_hBR },
			{ m_hTL2, m_hT2, m_hTR2, m_hL2, m_hR2, m_hBL2, m_hB2, m_hBR2 },
			{ m_hTL3, m_hT3, m_hTR3, m_hL3, m_hR3, m_hBL3, m_hB3, m_hBR3 },
		};
		int group = ScreenWidth > 1023 ? 2 : ScreenWidth > 799 ? 1 : 0;
		int left = group == 0 ? -320 : -384;
		int top = group == 0 ? -240 : group == 1 ? -300 : -384;
		const int columns[] = { 0, 1, 2, 0, 2, 0, 1, 2 };
		const int rows[] = { 0, 0, 0, 1, 1, 2, 2, 2 };
		int x[] = { centerX + left, centerX - 128, centerX + 128 };
		int y[] = { centerY + top, centerY - 128, centerY + 128 };
		for ( int i = 0; i < 8; ++i )
		{
			SPR_Set( sprites[group][i], 255, 255, 255 );
			SPR_DrawHoles( 0, x[columns[i]], y[rows[i]], rects[group][i] );
		}
	}
	if ( ScreenWidth <= 640 )
		return 1;
	int leftMask = centerX - 320;
	int rightMask = centerX + 320;
	if ( ScreenWidth > 799 )
	{
		leftMask = centerX - 384;
		rightMask = centerX + 384;
	}
	if ( ScreenWidth > 1024 )
	{
		int columns = ScreenWidth / 256 + 1;
		for ( int y = centerY - 416; y > 0; y -= 32 )
		{
			for ( int i = 0; i < columns; ++i )
			{
				SPR_Set( m_hHorizontal, 255, 255, 255 );
				SPR_Draw( 0, i * 256, y, m_hHor );
			}
		}
		for ( int i = 0; i < columns; ++i )
		{
			SPR_Set( m_hHorizontal, 255, 255, 255 );
			SPR_Draw( 0, i * 256, 0, m_hHor );
		}
		for ( int y = centerY + 384; y < ScreenHeight; y += 32 )
		{
			for ( int i = 0; i < columns; ++i )
			{
				SPR_Set( m_hHorizontal, 255, 255, 255 );
				SPR_Draw( 0, i * 256, y, m_hHor );
			}
		}
	}
	int rows = ScreenHeight / 256 + 1;
	for ( int x = leftMask - 40; x > 0; x -= 40 )
	{
		for ( int i = 0; i < rows; ++i )
		{
			SPR_Set( m_hVertical, 255, 255, 255 );
			SPR_Draw( 0, x, i * 256, m_hVer );
		}
	}
	for ( int i = 0; i < rows; ++i )
	{
		SPR_Set( m_hVertical, 255, 255, 255 );
		SPR_Draw( 0, 0, i * 256, m_hVer );
	}
	for ( int x = rightMask; x < ScreenWidth; x += 40 )
	{
		for ( int i = 0; i < rows; ++i )
		{
			SPR_Set( m_hVertical, 255, 255, 255 );
			SPR_Draw( 0, x, i * 256, m_hVer );
		}
	}
	return 1;
}
