/***
*
*	Copyright (c) 1999, Valve LLC. All rights reserved.
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
// hud_redraw.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "event_api.h"
#include "pm_shared.h"
#include "../public/game_controls/iviewport.h"
#include <stdio.h>

extern Vector v_origin;
extern int g_iDeadFlag;
extern cvar_t *sensitivity;
extern float HUD_GetFOV();
extern void ToggleScores();

int grgLogoFrame[56] =
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31
};

Queue g_RubbleQueue;
char cmd[50];
int i_dodmusic = 1;
int g_SpecScoreboardActive;

Queue::~Queue()
{
	Update( 99999 );
}

void CHud::Think()
{
	m_scrinfo.iSize = sizeof( m_scrinfo );
	gEngfuncs.pfnGetScreenInfo( &m_scrinfo );
	if ( flBoltHideXHair > 0 )
		flBoltHideXHair = (float)(flBoltHideXHair - gHUD.m_flTimeDelta);
	if ( flBoltHideXHair < 0 )
		flBoltHideXHair = 0;
	if ( !g_iDeadFlag && m_flTime > m_flPlaySprintSoundTime && g_fStamina < 60 && !gEngfuncs.IsSpectateOnly() )
	{
		gEngfuncs.pEventAPI->EV_PlaySound( -1, v_origin, CHAN_STATIC, "player/sprintgrunts.wav",
			g_fStamina >= 25 ? 0.3f : 0.8f, 0, 0, PITCH_NORM );
		m_flPlaySprintSoundTime = gHUD.m_flTime + 1.4f;
	}
	if ( g_iDeadFlag > 0 && m_flPlaySprintSoundTime > gHUD.m_flTime )
		gEngfuncs.pEventAPI->EV_StopSound( -1, CHAN_STATIC, "player/sprintgrunts.wav" );
	if ( g_iDeadFlag || g_iUser1 )
		gHUD.m_iSensLevel = 0;
	if ( !gEngfuncs.IsSpectateOnly() )
	{
		if ( i_dodmusic == 1 && gHUD.m_flTime > gHUD.i_MusicFadeCounter )
		{
			gEngfuncs.pfnPlaySoundByName( "player/prewarmup.wav", 1 );
			i_dodmusic = 0;
		}
		if ( gHUD.i_MusicFadeCounter - gHUD.m_flTime > 10 )
			gHUD.i_MusicFadeCounter = gHUD.m_flTime - 1;
	}
	for ( HUDLIST *element = m_pHudList; element; element = element->pNext )
	{
		if ( element->p->m_iFlags & HUD_ACTIVE )
			element->p->Think();
	}
	int fov = (int)HUD_GetFOV();
	if ( !fov )
		fov = 90;
	if ( m_iFOV != fov )
		m_iFOV = fov;
	if ( fov == 90 )
		m_flMouseSensitivity = 0;
	else
		m_flMouseSensitivity = (float)((zoom_sensitivity_ratio ? zoom_sensitivity_ratio->value : 1.0) *
			(fov / 90.0 * sensitivity->value));
	if ( gEngfuncs.IsSpectateOnly() )
	{
		if ( g_iUser1 == OBS_IN_EYE && g_iUser2 )
			m_iFOV = m_PlayerFOV[g_iUser2] ? m_PlayerFOV[g_iUser2] : 90;
		else
			m_iFOV = 90;
	}
	g_RubbleQueue.Update( m_flTime );
	if ( gViewPortInterface )
		gViewPortInterface->OnTick();
}

int CHud::Redraw( float time, int intermission )
{
	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = time;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;
	static float m_flShotTime;
	if ( gViewPortInterface )
	{
		if ( gHUD.m_Spectator.m_scoreboard->value != g_SpecScoreboardActive )
		{
			ToggleScores();
			g_SpecScoreboardActive = (int)gHUD.m_Spectator.m_scoreboard->value;
		}
		if ( m_iIntermission && !intermission )
		{
			m_iIntermission = intermission;
			gViewPortInterface->HideAllVGUIMenu();
			gViewPortInterface->HideScoreBoard();
		}
		else if ( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			gViewPortInterface->HideAllVGUIMenu();
			gViewPortInterface->ShowScoreBoard();
			if ( hud_takesshots && hud_takesshots->value )
				m_flShotTime = time + 1;
		}
	}
	if ( m_flShotTime && m_flShotTime < time )
	{
		gEngfuncs.pfnClientCmd( "snapshot\n" );
		m_flShotTime = 0;
	}
	m_iIntermission = intermission;

	// if no redrawing is necessary
	// return 0;

	// draw all registered HUD elements
	if ( m_pCvarDraw->value )
	{
		for ( HUDLIST *element = m_pHudList; element; element = element->pNext )
		{
			if ( intermission )
			{
				if ( element->p->m_iFlags & HUD_INTERMISSION )
					element->p->Draw( time );
			}
			else if ( (element->p->m_iFlags & HUD_ACTIVE) && !(m_iHideHUDDisplay & HIDEHUD_ALL) )
				element->p->Draw( time );
		}
	}
	if ( r_drawentities && r_drawentities->value != 1 )
	{
		char things[6];
		strcpy( things, "quit\n" );
		gEngfuncs.pfnClientCmd( "r_drawentities 1" );
		gEngfuncs.pfnConsolePrint( "r_drawentities is not a valid command. Do not use it.\n" );
		gEngfuncs.pfnClientCmd( things );
	}
	if ( cl_lw && cl_lw->value != 1 )
	{
		char things[6];
		strcpy( things, "quit\n" );
		gEngfuncs.pfnClientCmd( "cl_lw 1" );
		gEngfuncs.pfnConsolePrint( "cl_lw 0 is not a valid command. Do not use it.\n" );
		gEngfuncs.pfnClientCmd( things );
	}
	if ( cl_pitchdown && (cl_pitchdown->value > 89 || cl_pitchdown->value < 0) )
		gEngfuncs.pfnClientCmd( "cl_pitchdown 89" );
	if ( cl_pitchup && (cl_pitchup->value > 89 || cl_pitchup->value < 0) )
		gEngfuncs.pfnClientCmd( "cl_pitchup 89" );
	if ( crosshair && crosshair->value > 0 )
		gEngfuncs.pfnClientCmd( "crosshair 0" );
	g_RubbleQueue.maxelements = max_rubble ? (int)max_rubble->value : 0;
	return 1;
}

void ScaleColors( int &red, int &green, int &blue, int alpha )
{
	double scale = alpha / 255.0;
	red = (int)(float)(red * scale);
	green = (int)(float)(green * scale);
	blue = (int)(float)(blue * scale);
}

int CHud::DrawHudString( int x, int y, int maxX, char *text, int red, int green, int blue )
{
	for ( char *character = text; *character && *character != '\n'; ++character )
	{
		int width = gHUD.m_scrinfo.charWidths[(int)*character];
		if ( x + width > maxX )
			break;
		gEngfuncs.pfnDrawCharacter( x, y, *character, red, green, blue );
		x += width;
	}
	return x;
}

int CHud::DrawHudStringReverse( int x, int y, int minX, char *text, int red, int green, int blue )
{
	for ( int i = (int)strlen( text ) - 1; i >= 0; --i )
	{
		int width = gHUD.m_scrinfo.charWidths[(int)text[i]];
		if ( x - width < minX )
			break;
		x -= width;
		gEngfuncs.pfnDrawCharacter( x, y, text[i], red, green, blue );
	}
	return x;
}

int CHud::DrawHudNumberString( int x, int y, int minX, int number, int red, int green, int blue )
{
	char text[32];
	sprintf( text, "%d", number );
	return DrawHudStringReverse( x, y, minX, text, red, green, blue );
}

int CHud::DrawHudNumber( int x, int y, int flags, int number, int red, int green, int blue )
{
	int width = GetSpriteRect( m_HUD_number_0 ).right - GetSpriteRect( m_HUD_number_0 ).left;
	if ( number > 0 )
	{
		// SPR_Draw 100's
		if ( number >= 100 )
		{
			int digit = m_HUD_number_0 + number / 100;
			SPR_Set( GetSprite( digit ), red, green, blue );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( digit ) );
			x += width;
		}
		else if ( flags & DHN_3DIGITS )
			x += width;
		if ( number >= 10 )
		{
			int digit = m_HUD_number_0 + number % 100 / 10;
			SPR_Set( GetSprite( digit ), red, green, blue );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( digit ) );
			x += width;
		}
		else if ( flags & (DHN_3DIGITS | DHN_2DIGITS) )
			x += width;
		int digit = m_HUD_number_0 + number % 10;
		SPR_Set( GetSprite( digit ), red, green, blue );
		SPR_DrawAdditive( 0, x, y, &GetSpriteRect( digit ) );
		x += width;
	}
	else if ( flags & DHN_DRAWZERO )
	{
		SPR_Set( GetSprite( m_HUD_number_0 ), red, green, blue );
		if ( flags & DHN_3DIGITS )
			x += width;
		if ( flags & (DHN_3DIGITS | DHN_2DIGITS) )
			x += width;

		// SPR_Draw ones

		SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
		x += width;
	}
	return x;
}

int CHud::GetNumWidth( int number, int flags )
{
	if ( flags & DHN_3DIGITS )
		return 3;
	if ( flags & DHN_2DIGITS )
		return 2;
	if ( number <= 0 )
		return flags & DHN_DRAWZERO;
	if ( number < 10 )
		return 1;
	return number < 100 ? 2 : 3;
}
