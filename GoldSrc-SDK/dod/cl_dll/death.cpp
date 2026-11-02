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
// death notice
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "../public/game_controls/iviewport.h"
#include <string.h>

DECLARE_MESSAGE( m_DeathNotice, DeathMsg );

struct DeathNoticeItem
{
	char szKiller[MAX_PLAYER_NAME_LENGTH * 2];
	char szVictim[MAX_PLAYER_NAME_LENGTH * 2];
	int iId;	// the index number of the associated sprite
	int iSuicide;
	int iTeamKill;
	int iNonPlayerKill;
	float flDisplayTime;
	float* KillerColor;
	float* VictimColor;
};

#define MAX_DEATHNOTICES 4

char deathicons[44][32] =
{
	"", "d_amerknife", "d_gerknife", "d_colt", "d_luger", "d_garand", "d_scopedkar", "d_thompson",
	"d_mp44", "d_spring", "d_kar", "d_bar", "d_mp40", "d_grenade", "d_stick", "d_stick", "d_grenade",
	"d_mg42", "d_30cal", "d_spade", "d_m1carbine", "d_mg34", "d_greasegun", "d_fg42", "d_k43",
	"d_enfield", "d_sten", "d_bren", "d_webley", "d_bazooka", "d_pschreck", "d_piat", "d_mortar",
	"d_binoculars", "d_satchel", "d_scopedfg42", "d_fcarbine", "d_bayonet", "d_scopedenfield",
	"d_britgrenade", "d_britknife", "d_mortar", "d_garandbutt", "d_enfbayonet"
};

static int DEATHNOTICE_DISPLAY_TIME = 6;
DeathNoticeItem rgDeathNoticeList[MAX_DEATHNOTICES + 1];
float g_ColorBlue[3] = { 0.6f, 0.8f, 1 };
float g_ColorRed[3] = { 1, 0.25f, 0.25f };
float g_ColorGreen[3] = { 0.6f, 1, 0.6f };
float g_ColorYellow[3] = { 1, 0.7f, 0 };
float g_ColorDoDGreen[3] = { 0.3f, 0.4f, 0.3f };

float* GetClientColor( int client )
{
	switch ( g_PlayerExtraInfo[client].teamnumber )
	{
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	case 4: return g_ColorGreen;
	}
	return g_ColorDoDGreen;
}

float* GetTeamColor( int team )
{
	switch ( team )
	{
	case 1: return g_ColorDoDGreen;
	case 2: return g_ColorRed;
	case 3: return g_ColorYellow;
	}
	return g_ColorGreen;
}

int CHudDeathNotice::Init()
{
	gHUD.AddHudElem( this );
	HOOK_MESSAGE( DeathMsg );
	hud_deathnotice_time = CVAR_CREATE( "hud_deathnotice_time", "6", FCVAR_ARCHIVE );
	return 1;
}

void CHudDeathNotice::InitHUDData()
{
	memset( rgDeathNoticeList, 0, sizeof( rgDeathNoticeList ) );
}

int CHudDeathNotice::VidInit()
{
	m_HUD_d_skull = gHUD.GetSpriteIndex( "d_skull" );
	return 1;
}

int CHudDeathNotice::Draw( float time )
{
	int mapX, mapY, mapWidth, mapHeight;
	gHUD.GetMapBounds( mapX, mapY, mapWidth, mapHeight );
	int y;
	if ( gHUD.GetMinimapState() == 2 )
		y = mapHeight + mapY + (int)(ScreenHeight / 480.0 * 2 + 0.5);
	else
		y = 20 + (g_iUser1 ? (int)(ScreenHeight / 480.0 * 42 + 0.5) : 0);

	for ( int i = 0; i < MAX_DEATHNOTICES && rgDeathNoticeList[i].iId; i++ )
	{
		DeathNoticeItem* notice = &rgDeathNoticeList[i];
		if ( notice->flDisplayTime < time )
		{
			memmove( notice, notice + 1, sizeof( *notice ) * (MAX_DEATHNOTICES - i) );
			i--;  // continue on the next item;  stop the counter getting incremented
			continue;
		}
		notice->flDisplayTime = min( notice->flDisplayTime, gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME );
		if ( !gViewPortInterface || !gViewPortInterface->AllowedToPrintText() )
			continue;

		int id = notice->iId == -1 ? m_HUD_d_skull : notice->iId;
		wrect_t rect = gHUD.GetSpriteRect( id );
		int x = ScreenWidth - ConsoleStringLen( notice->szVictim ) - (rect.right - rect.left);
		if ( !notice->iSuicide )
		{
			x -= ConsoleStringLen( notice->szKiller ) + 5;
			if ( notice->KillerColor )
				gEngfuncs.pfnDrawSetTextColor( notice->KillerColor[0], notice->KillerColor[1], notice->KillerColor[2] );
			x = DrawConsoleString( x, y, notice->szKiller ) + 5;
		}
		SPR_Set( gHUD.GetSprite( id ), 255, 255, 255 );
		SPR_DrawHoles( 0, x, y, &rect );
		if ( !notice->iNonPlayerKill )
		{
			if ( notice->VictimColor )
				gEngfuncs.pfnDrawSetTextColor( notice->VictimColor[0], notice->VictimColor[1], notice->VictimColor[2] );
			DrawConsoleString( x + rect.right - rect.left, y, notice->szVictim );
		}
		y += rect.bottom - rect.top;
	}
	return 1;
}

int CHudDeathNotice::MsgFunc_DeathMsg( const char*, int size, void* data )
{
	m_iFlags |= HUD_ACTIVE;
	BEGIN_READ( data, size );
	int killer = READ_BYTE();
	int victim = READ_BYTE();
	int weapon = READ_BYTE();
	char killedwith[60];
	strcpy( killedwith, weapon >= 1 && weapon <= 43 ? deathicons[weapon] : "d_world" );
	gViewPortInterface->DeathMsg( killer, victim );
	gHUD.m_Spectator.DeathMessage( victim );

	int i;
	for ( i = 0; i < MAX_DEATHNOTICES; i++ )
	{
		if ( !rgDeathNoticeList[i].iId )
			break;
	}
	if ( i == MAX_DEATHNOTICES )
	{ // move the rest of the list forward to make room for this item
		memmove( rgDeathNoticeList, rgDeathNoticeList + 1, sizeof( DeathNoticeItem ) * MAX_DEATHNOTICES );
		i = MAX_DEATHNOTICES - 1;
	}
	DeathNoticeItem* notice = &rgDeathNoticeList[i];
	gEngfuncs.pfnGetPlayerInfo( killer, &g_PlayerInfoList[killer] );
	gEngfuncs.pfnGetPlayerInfo( victim, &g_PlayerInfoList[victim] );
	if ( g_PlayerInfoList[killer].name )
	{
		notice->KillerColor = GetClientColor( killer );
		strncpy( notice->szKiller, g_PlayerInfoList[killer].name, MAX_PLAYER_NAME_LENGTH );
		notice->szKiller[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}
	else
		notice->szKiller[0] = 0;
	if ( victim != 255 && g_PlayerInfoList[victim].name )
	{
		notice->VictimColor = GetClientColor( victim );
		strncpy( notice->szVictim, g_PlayerInfoList[victim].name, MAX_PLAYER_NAME_LENGTH );
		notice->szVictim[MAX_PLAYER_NAME_LENGTH - 1] = 0;
	}
	else
		notice->szVictim[0] = 0;
	if ( victim == 255 )
	{
		notice->iNonPlayerKill = TRUE;
		strcpy( notice->szVictim, killedwith + 2 );
	}
	else if ( killer == victim || killer == 0 )
		notice->iSuicide = TRUE;
	else if ( victim && g_PlayerExtraInfo[killer].teamnumber == g_PlayerExtraInfo[victim].teamnumber )
		notice->iTeamKill = TRUE;

	notice->iId = gHUD.GetSpriteIndex( killedwith );
	DEATHNOTICE_DISPLAY_TIME = hud_deathnotice_time ? (int)hud_deathnotice_time->value : 6;
	notice->flDisplayTime = gHUD.m_flTime + DEATHNOTICE_DISPLAY_TIME;
	if ( notice->iNonPlayerKill )
	{
		ConsolePrint( notice->szKiller );
		ConsolePrint( " killed a " );
		ConsolePrint( notice->szVictim );
	}
	else if ( notice->iSuicide )
	{
		ConsolePrint( notice->szVictim );
		ConsolePrint( victim == killer ? " killed self" : " died" );
	}
	else
	{
		ConsolePrint( notice->szKiller );
		ConsolePrint( notice->iTeamKill ? " killed his teammate " : " killed " );
		ConsolePrint( notice->szVictim );
		ConsolePrint( " with " );
		ConsolePrint( killedwith + 2 ); // skip over the "d_" part
	}
	ConsolePrint( "\n" );
	return 1;
}
