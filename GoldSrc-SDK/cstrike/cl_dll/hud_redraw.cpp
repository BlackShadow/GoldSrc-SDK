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

#include <stdio.h>
#include <math.h>
#include "hud.h"
#include "cl_util.h"
#include "pm_shared.h"
#include "../public/game_controls/iviewport.h"

extern cvar_t* sensitivity;
extern cvar_t* cl_pitchup;
extern cvar_t* cl_pitchdown;
float HUD_GetFOV();
void ToggleScores();
char* SharedVarArgs( char* format, ... );

int grgLogoFrame[56] =
{
	1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 13, 13, 13, 13, 13, 12, 11, 10, 9, 8, 14, 15,
	16, 17, 18, 19, 20, 20, 20, 20, 20, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
	29, 29, 29, 29, 29, 28, 27, 26, 25, 24, 30, 31
};

cvar_t* cl_drawviewmodel;
cvar_t* cl_lightgamma;
cvar_t* cl_direct;
int g_SpecScoreboardActive;

Queue g_ShellQueue;
Queue g_SmokePuffQueue;

Queue::~Queue()
{
	Update( 99999 );
}

void CHud::Think()
{
	m_scrinfo.iSize = sizeof( m_scrinfo );
	gEngfuncs.pfnGetScreenInfo( &m_scrinfo );
	for ( HUDLIST* item = m_pHudList; item; item = item->pNext )
	{
		if ( item->p->m_iFlags & HUD_ACTIVE )
			item->p->Think();
	}
	int fov = (int)HUD_GetFOV();
	m_iFOV = fov ? fov : (int)default_fov->value;

	// the clients fov is actually set in the client data update section of the hud

	// Set a new sensitivity
	if ( m_iFOV == default_fov->value )
		m_flMouseSensitivity = 0;
	else
	{
		float scaled = (float)fov / default_fov->value * sensitivity->value;
		m_flMouseSensitivity = gEngfuncs.pfnGetCvarFloat( "zoom_sensitivity_ratio" ) * scaled;
	}
	if ( !m_iFOV )
		m_iFOV = default_fov->value >= 90 ? (int)default_fov->value : 90;
	if ( gEngfuncs.IsSpectateOnly() )
	{
		if ( g_iUser1 == OBS_IN_EYE && g_iUser2 )
		{
			if ( gEngfuncs.GetMaxClients() >= g_iUser2 )
				m_iFOV = m_PlayerFOV[g_iUser2];
			else
				m_iFOV = default_fov->value >= 90 ? (int)default_fov->value : 90;
		}
		else
			m_iFOV = (int)gHUD.m_Spectator.GetFOV();
	}
	g_ShellQueue.Update( m_flTime );
	g_SmokePuffQueue.Update( m_flTime );
	if ( gViewPortInterface )
		gViewPortInterface->OnTick();
}

int CHud::Redraw( float time, int intermission )
{
	static float m_flShotTime;
	static float flLastSensivity = -1;
	static float flSensitivityUpdateTime;
	m_fOldTime = m_flTime;	// save time of previous redraw
	m_flTime = time;
	m_flTimeDelta = (double)m_flTime - m_fOldTime;
	if ( !cl_drawviewmodel )
		cl_drawviewmodel = gEngfuncs.pfnGetCvarPointer( "r_drawviewmodel" );
	if ( !cl_lightgamma )
		cl_lightgamma = gEngfuncs.pfnGetCvarPointer( "lightgamma" );
	if ( !cl_direct )
		cl_direct = gEngfuncs.pfnGetCvarPointer( "direct" );

	// Clock was reset, reset delta
	if ( m_flTimeDelta < 0 )
		m_flTimeDelta = 0;
	if ( gViewPortInterface )
	{
		if ( gHUD.m_Spectator.m_scoreboard->value != g_SpecScoreboardActive )
		{
			ToggleScores();
			g_SpecScoreboardActive = (int)gHUD.m_Spectator.m_scoreboard->value;
		}
		if ( m_iIntermission && !intermission )
		{
			m_iIntermission = 0;
			gViewPortInterface->HideAllVGUIMenu();
			gViewPortInterface->HideScoreBoard();
		}
		else if ( !m_iIntermission && intermission )
		{
			m_iIntermission = intermission;
			if ( gViewPortInterface->GetCurrentMenuID() != 36 )
			{
				gViewPortInterface->HideAllVGUIMenu();
				gViewPortInterface->ShowScoreBoard();
			}
			if ( gEngfuncs.pfnGetCvarFloat( "hud_takesshots" ) )
				m_flShotTime = time + 1;
		}
	}
	if ( m_flShotTime && time > m_flShotTime )
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
		for ( HUDLIST* item = m_pHudList; item; item = item->pNext )
		{
			if ( intermission ? ( item->p->m_iFlags & HUD_INTERMISSION )
				: ( item->p->m_iFlags & HUD_ACTIVE ) && !( m_iHideHUDDisplay & HIDEHUD_ALL ) )
				item->p->Draw( time );
		}
	}
	else if ( intermission )
	{
		if ( m_NightVision.m_iFlags & HUD_INTERMISSION )
			m_NightVision.Draw( time );
		if ( m_sniperScope.m_iFlags & HUD_INTERMISSION )
			m_sniperScope.Draw( time );
	}
	else
	{
		if ( ( m_NightVision.m_iFlags & HUD_ACTIVE ) && !( m_iHideHUDDisplay & HIDEHUD_ALL ) )
			m_NightVision.Draw( time );
		if ( ( m_sniperScope.m_iFlags & HUD_ACTIVE ) && !( m_iHideHUDDisplay & HIDEHUD_ALL ) )
			m_sniperScope.Draw( time );
	}

	// are we in demo mode? do we need to draw the logo in the top corner?
	if ( m_iLogo )
	{
		if ( !m_hsprLogo )
			m_hsprLogo = LoadSprite( "sprites/%d_logo.spr" );
		SPR_Set( m_hsprLogo, 250, 250, 250 );
		float frame = time * 20;
		SPR_DrawAdditive( grgLogoFrame[(int)frame % ARRAYSIZE( grgLogoFrame )] - 1,
			ScreenWidth - SPR_Width( m_hsprLogo, 0 ), SPR_Height( m_hsprLogo, 0 ) / 2, NULL );
	}
	if ( time > m_flCheatCheckTime || m_flCheatCheckTime == -1 )
	{
		char* kill = new char[6];
		strcpy( kill, "kill\n" );
		char* disconnect = new char[12];
		strcpy( disconnect, "disconnect\n" );
		char* quit = new char[6];
		strcpy( quit, "quit\n" );
		if ( cl_drawviewmodel && cl_drawviewmodel->value != 1 )
			gEngfuncs.Cvar_SetValue( "r_drawviewmodel", 1 );
		if ( cl_pitchup->value != 89 )
		{
			gEngfuncs.Cvar_SetValue( "cl_pitchup", 89 );
			gEngfuncs.pfnClientCmd( kill );
			gEngfuncs.pfnClientCmd( disconnect );
		}
		if ( cl_pitchdown->value != 89 )
		{
			gEngfuncs.Cvar_SetValue( "cl_pitchdown", 89 );
			gEngfuncs.pfnClientCmd( kill );
			gEngfuncs.pfnClientCmd( disconnect );
		}
		if ( flLastSensivity < 0 )
			flLastSensivity = sensitivity->value;
		if ( sensitivity->value < 0.2 )
			gEngfuncs.pfnClientCmd( "sensitivity 1\n" );
		else if ( flSensitivityUpdateTime > time && fabs( sensitivity->value - flLastSensivity ) > 0.2 )
		{
			gEngfuncs.pfnClientCmd( SharedVarArgs( "sensitivity %0.2f\n", flLastSensivity ) );
			flSensitivityUpdateTime = time + 2;
		}
		if ( sensitivity->value != flLastSensivity && time > flSensitivityUpdateTime )
		{
			flLastSensivity = sensitivity->value;
			flSensitivityUpdateTime = time + 2;
		}
		if ( !m_NightVision.m_fOn )
		{
			if ( cl_lightgamma && cl_lightgamma->value != 2.5 )
			{
				gEngfuncs.Cvar_SetValue( "lightgamma", 2.5 );
				gEngfuncs.pfnClientCmd( kill );
			}
			if ( cl_direct && cl_direct->value < 0.8 )
			{
				gEngfuncs.Cvar_SetValue( "direct", 0.9 );
				gEngfuncs.pfnClientCmd( kill );
			}
		}
		float pitch = gEngfuncs.pfnGetCvarFloat( "m_pitch" );
		if ( pitch >= 0 && pitch < 0.022 )
			gEngfuncs.pfnClientCmd( "m_pitch 0.022\n" );
		else if ( pitch < 0 && pitch > -0.022 )
			gEngfuncs.pfnClientCmd( "m_pitch -0.022\n" );
		g_ShellQueue.maxelements = (int)gEngfuncs.pfnGetCvarFloat( "max_shells" );
		g_SmokePuffQueue.maxelements = (int)gEngfuncs.pfnGetCvarFloat( "max_smokepuffs" );
		m_flCheatCheckTime = time + 0.5;
		delete[] kill;
		delete[] disconnect;
		delete[] quit;
	}

	/*
	if ( g_iVisibleMouse )
	{
		void IN_GetMousePos( int *mx, int *my );
		int mx, my;

		IN_GetMousePos( &mx, &my );
		
		if (m_hsprCursor == 0)
		{
			char sz[256];
			sprintf( sz, "sprites/cursor.spr" );
			m_hsprCursor = SPR_Load( sz );
		}

		SPR_Set(m_hsprCursor, 250, 250, 250 );
		
		// Draw the logo at 20 fps
		SPR_DrawAdditive( 0, mx, my, NULL );
	}
	*/

	return 1;
}

void ScaleColors( int& r, int& g, int& b, int alpha )
{
	double scale = (double)alpha / 255;
	float value = r * scale;
	r = (int)value;
	value = g * scale;
	g = (int)value;
	value = b * scale;
	b = (int)value;
}

int CHud::DrawHudString( int x, int y, int, const char* text, int r, int g, int b )
{
	return x + gEngfuncs.pfnDrawString( x, y, text, r, g, b );
}

int CHud::DrawHudStringReverse( int x, int y, int, const char* text, int r, int g, int b )
{
	return x - gEngfuncs.pfnDrawStringReverse( x, y, text, r, g, b );
}

int CHud::DrawHudNumberString( int x, int y, int, int number, int r, int g, int b )
{
	char text[32];
	sprintf( text, "%d", number );
	return x - gEngfuncs.pfnDrawStringReverse( x, y, text, r, g, b );
}

int CHud::GetNumWidth( int number, int flags )
{
	if ( flags & DHN_3DIGITS )
		return 3;
	if ( flags & DHN_2DIGITS )
		return 2;
	if ( number <= 0 )
		return flags & DHN_DRAWZERO;
	if ( number > 99 )
		return 3;
	return number > 9 ? 2 : 1;
}

int CHud::DrawHudNumber( int x, int y, int number, int r, int g, int b )
{
	static char szBuffer[16];
	int width = GetSpriteWidth( m_HUD_number_0 );
	sprintf( szBuffer, "%5i", number );
	const char* digit = szBuffer + ( number < 0 );
	while ( *digit )
	{
		if ( (unsigned int)( *digit - '0' ) <= 9 )
		{
			int index = m_HUD_number_0 + *digit - '0';
			SPR_Set( GetSprite( index ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( index ) );
		}
		digit++;
		x += width;
	}
	return x;
}

int CHud::DrawHudNumber( int x, int y, int flags, int number, int r, int g, int b )
{
	int width = GetSpriteWidth( m_HUD_number_0 );
	if ( number > 0 )
	{
		// SPR_Draw 100's
		if ( number >= 100 )
		{
			int index = m_HUD_number_0 + number / 100;
			SPR_Set( GetSprite( index ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( index ) );
			x += width;
		}
		else if ( flags & DHN_3DIGITS )
		{
			if ( flags & DHN_FILLZERO )
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
			x += width;
		}

		// SPR_Draw 10's
		if ( number >= 10 )
		{
			int index = m_HUD_number_0 + number % 100 / 10;
			SPR_Set( GetSprite( index ), r, g, b );
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( index ) );
			x += width;
		}
		else if ( flags & ( DHN_2DIGITS | DHN_3DIGITS ) )
		{
			if ( flags & DHN_FILLZERO )
				SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
			x += width;
		}
		int index = m_HUD_number_0 + number % 10;
		SPR_Set( GetSprite( index ), r, g, b );
		SPR_DrawAdditive( 0, x, y, &GetSpriteRect( index ) );
		return x + width;
	}
	if ( !( flags & DHN_DRAWZERO ) )
		return x;
	SPR_Set( GetSprite( m_HUD_number_0 ), r, g, b );
	if ( flags & DHN_3DIGITS )
	{
		if ( flags & DHN_FILLZERO )
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
		x += width;
	}
	if ( flags & ( DHN_2DIGITS | DHN_3DIGITS ) )
	{
		if ( flags & DHN_FILLZERO )
			SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
		x += width;
	}
	SPR_DrawAdditive( 0, x, y, &GetSpriteRect( m_HUD_number_0 ) );
	return x + width;
}
