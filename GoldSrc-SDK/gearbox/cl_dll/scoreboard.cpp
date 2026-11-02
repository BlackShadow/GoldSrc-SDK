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
// Scoreboard.cpp
//
// implementation of CHudScoreboard class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>
#include "vgui_CTFViewport.h"

team_info_t g_TeamInfo[MAX_PLAYERS + 1];
int g_IsSpectator[MAX_PLAYERS + 1];
icon_sprite_t g_PlayerSpriteList[MAX_PLAYERS + 1][6];

DECLARE_COMMAND( m_Scoreboard, ShowScores );
DECLARE_COMMAND( m_Scoreboard, HideScores );

DECLARE_MESSAGE( m_Scoreboard, ScoreInfo );
DECLARE_MESSAGE( m_Scoreboard, TeamInfo );
DECLARE_MESSAGE( m_Scoreboard, TeamScore );
DECLARE_MESSAGE( m_Scoreboard, PlayerIcon );
DECLARE_MESSAGE( m_Scoreboard, CTFScore );

int CHudScoreboard :: Init( void )
{
	gHUD.AddHudElem( this );

	// Hook messages & commands here
	//HOOK_COMMAND( "+showscores", ShowScores );
	//HOOK_COMMAND( "-showscores", HideScores );

	HOOK_MESSAGE( ScoreInfo );
	HOOK_MESSAGE( TeamScore );
	HOOK_MESSAGE( TeamInfo );
	HOOK_MESSAGE( PlayerIcon );
	HOOK_MESSAGE( CTFScore );

	InitHUDData();

	cl_showpacketloss = CVAR_CREATE( "cl_showpacketloss", "0", FCVAR_ARCHIVE );

	return 1;
}


int CHudScoreboard :: VidInit( void )
{
	// Load sprites here

	return 1;
}

void CHudScoreboard :: InitHUDData( void )
{
	memset( g_PlayerExtraInfo, 0, sizeof g_PlayerExtraInfo );
	m_iLastKilledBy = 0;
	m_fLastKillTime = 0;
	m_iPlayerNum = 0;
	m_iNumTeams = 0;
	memset( g_TeamInfo, 0, sizeof g_TeamInfo );
	memset( g_PlayerSpriteList, 0, sizeof g_PlayerSpriteList );

	m_iFlags &= ~HUD_ACTIVE;  // starts out inactive

	m_iFlags |= HUD_INTERMISSION; // is always drawn during an intermission
}

/* The scoreboard
We have a minimum width of 1-320 - we could have the field widths scale with it?
*/

// X positions
// relative to the side of the scoreboard
#define NAME_RANGE_MIN  20
#define NAME_RANGE_MAX  145
#define KILLS_RANGE_MIN 130
#define KILLS_RANGE_MAX 170
#define DIVIDER_POS		180
#define DEATHS_RANGE_MIN  185
#define DEATHS_RANGE_MAX  210
#define PING_RANGE_MIN	245
#define PING_RANGE_MAX	295
#define PL_RANGE_MIN 315
#define PL_RANGE_MAX 375

int SCOREBOARD_WIDTH = 320;
		

// Y positions
#define ROW_GAP  13
#define ROW_RANGE_MIN 15
#define ROW_RANGE_MAX ( ScreenHeight - 50 )

int CHudScoreboard :: Draw( float )
{
	int i, j;
	int can_show_packetloss = 0;
	int FAR_RIGHT;

	if ( !m_iShowscoresHeld && gHUD.m_Health.m_iHealth > 0 && !gHUD.m_iIntermission )
		return 1;
	if (gHUD.m_iIntermission && gHUD.m_Teamplay == 2)
		return 1;

	GetAllPlayersInfo();

	//  Packetloss removed on Kelly 'shipping nazi' Bailey's orders
	if ( cl_showpacketloss && cl_showpacketloss->value && ( ScreenWidth >= 400 ) )
	{
		can_show_packetloss = 1;
		SCOREBOARD_WIDTH = 400;
	}
	else
	{
		SCOREBOARD_WIDTH = 320;
	}

	// just sort the list on the fly
	// list is sorted first by frags, then by deaths
	float list_slot = 0;
	int xpos_rel = (ScreenWidth - SCOREBOARD_WIDTH) / 2;

	// print the heading line
	int ypos = ROW_RANGE_MIN + (list_slot * ROW_GAP);
	int	xpos = NAME_RANGE_MIN + xpos_rel;

	if ( !gHUD.m_Teamplay ) 
		gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, "Player", 255, 140, 0 );
	else
		gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, "Teams", 255, 140, 0 );

	gHUD.DrawHudStringReverse( KILLS_RANGE_MAX + xpos_rel, ypos, 0, "kills", 255, 140, 0 );
	gHUD.DrawHudString( DIVIDER_POS + xpos_rel, ypos, ScreenWidth, "/", 255, 140, 0 );
	gHUD.DrawHudString( DEATHS_RANGE_MIN + xpos_rel + 5, ypos, ScreenWidth, "deaths", 255, 140, 0 );
	gHUD.DrawHudString( (gHUD.m_Teamplay == 2 ? 310 : 260) + xpos_rel, ypos, ScreenWidth, "latency", 255, 140, 0 );
	if (gHUD.m_Teamplay == 2)
	{
		gHUD.DrawHudString(240 + xpos_rel, ypos, ScreenWidth, "/", 255, 140, 0);
		gHUD.DrawHudString(255 + xpos_rel, ypos, ScreenWidth, "scores", 255, 140, 0);
	}

	if ( can_show_packetloss )
	{
		gHUD.DrawHudString( (gHUD.m_Teamplay == 2 ? 355 : 340) + xpos_rel, ypos, ScreenWidth, "pkt loss", 255, 140, 0 );
	}

	FAR_RIGHT = can_show_packetloss ? PL_RANGE_MAX : PING_RANGE_MAX;
	FAR_RIGHT += 5;
	if (gHUD.m_Teamplay == 2)
		FAR_RIGHT = can_show_packetloss ? 395 : 350;

	list_slot += 1.2;
	ypos = ROW_RANGE_MIN + (list_slot * ROW_GAP);
	xpos = NAME_RANGE_MIN + xpos_rel;
	FillRGBA( xpos - 5, ypos, FAR_RIGHT, 1, 255, 140, 0, 255);  // draw the seperator line
	
	list_slot += 0.8;

	if ( !gHUD.m_Teamplay )
	{
		// it's not teamplay,  so just draw a simple player list
		DrawPlayers( xpos_rel, list_slot );
		return 1;
	}

	// clear out team scores
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( gHUD.m_Teamplay != 2 && !g_TeamInfo[i].scores_overriden )
			g_TeamInfo[i].frags = g_TeamInfo[i].deaths = 0;
		g_TeamInfo[i].ping = g_TeamInfo[i].packetloss = 0;
	}

	// recalc the team scores, then draw them
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue; // empty player slot, skip

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// find what team this player is in
		for ( j = 1; j <= m_iNumTeams; j++ )
		{
			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}
		if ( j > m_iNumTeams )  // player is not in a team, skip to the next guy
			continue;

		if ( gHUD.m_Teamplay != 2 && !g_TeamInfo[j].scores_overriden )
		{
			g_TeamInfo[j].frags += g_PlayerExtraInfo[i].frags;
			g_TeamInfo[j].deaths += g_PlayerExtraInfo[i].deaths;
		}

		g_TeamInfo[j].ping += g_PlayerInfoList[i].ping;
		g_TeamInfo[j].packetloss += g_PlayerInfoList[i].packetloss;

		if ( g_PlayerInfoList[i].thisplayer )
			g_TeamInfo[j].ownteam = TRUE;
		else
			g_TeamInfo[j].ownteam = FALSE;
	}

	// find team ping/packetloss averages
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].already_drawn = FALSE;

		if ( g_TeamInfo[i].players > 0 )
		{
			g_TeamInfo[i].ping /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
			g_TeamInfo[i].packetloss /= g_TeamInfo[i].players;  // use the average ping of all the players in the team as the teams ping
		}
	}

	// Draw the teams
	while ( 1 )
	{
		int highest_frags = -99999; int lowest_deaths = 99999;
		int best_team = 0;

		for ( i = 1; i <= m_iNumTeams; i++ )
		{
			if (gHUD.m_Teamplay == 2 && (i > 2 || g_TeamInfo[i].players <= 0))
				continue;
			if ( g_TeamInfo[i].players < 0 )
				continue;

			if ( !g_TeamInfo[i].already_drawn && g_TeamInfo[i].frags >= highest_frags )
			{
				if ( g_TeamInfo[i].frags > highest_frags || g_TeamInfo[i].deaths < lowest_deaths )
				{
					best_team = i;
					lowest_deaths = g_TeamInfo[i].deaths;
					highest_frags = g_TeamInfo[i].frags;
				}
			}
		}

		// draw the best team on the scoreboard
		if ( !best_team )
			break;

		// draw out the best team
		team_info_t *team_info = &g_TeamInfo[best_team];

		ypos = ROW_RANGE_MIN + (list_slot * ROW_GAP);

		// check we haven't drawn too far down
		if ( ypos > ROW_RANGE_MAX )  // don't draw to close to the lower border
			break;

		xpos = NAME_RANGE_MIN + xpos_rel;
		int r = 255, g = 225, b = 55; // draw the stuff kinda yellowish
		
		if ( team_info->ownteam ) // if it is their team, draw the background different color
		{
			// overlay the background in blue,  then draw the score text over it
			FillRGBA( NAME_RANGE_MIN + xpos_rel - 5, ypos, FAR_RIGHT, ROW_GAP, 0, 0, 255, 70 );
		}

		if (gHUD.m_Teamplay == 2)
		{
			char score[64];
			sprintf(score, "%s Team Score:", team_info->name);
			gHUD.DrawHudString(xpos, ypos, xpos_rel + 205, score, r, g, b);
			gHUD.DrawHudNumberString(xpos_rel + 275, ypos, xpos_rel + 250, team_info->frags, r, g, b);
		}
		else
		{
		// draw their name (left to right)
		gHUD.DrawHudString( xpos, ypos, NAME_RANGE_MAX + xpos_rel, team_info->name, r, g, b );

		// draw kills (right to left)
		xpos = KILLS_RANGE_MAX + xpos_rel;
		gHUD.DrawHudNumberString( xpos, ypos, KILLS_RANGE_MIN + xpos_rel, team_info->frags, r, g, b );

		// draw divider
		xpos = DIVIDER_POS + xpos_rel;
		gHUD.DrawHudString( xpos, ypos, xpos + 20, "/", r, g, b );

		// draw deaths
		xpos = DEATHS_RANGE_MAX + xpos_rel;
		gHUD.DrawHudNumberString( xpos, ypos, DEATHS_RANGE_MIN + xpos_rel, team_info->deaths, r, g, b );
		}

		// draw ping
		// draw ping & packetloss
		static char buf[64];
		sprintf( buf, "%d", team_info->ping );
		xpos = xpos_rel + (gHUD.m_Teamplay == 2 ? 345 : 295);
		r = giR;
		g = giG;
		b = giB;
		gHUD.DrawHudStringReverse( xpos, ypos, xpos - 50, buf, r, g, b );

	//  Packetloss removed on Kelly 'shipping nazi' Bailey's orders
		if ( can_show_packetloss )
		{
			xpos = ((PL_RANGE_MAX - PL_RANGE_MIN) / 2) + PL_RANGE_MIN + xpos_rel + 25;
		
			sprintf( buf, "  %d", team_info->packetloss );
			gHUD.DrawHudString( xpos, ypos, xpos+50, buf, r, g, b );
		}

		team_info->already_drawn = TRUE;  // set the already_drawn to be TRUE, so this team won't get drawn again
		list_slot++;

		// draw all the players that belong to this team, indented slightly
		list_slot = DrawPlayers( xpos_rel, list_slot, 10, team_info->name );
	}

	// draw all the players who are not in a team
	list_slot += 0.5;
	DrawPlayers( xpos_rel, list_slot, 0, "" );

	return 1;
}

// returns the ypos where it finishes drawing
int CHudScoreboard :: DrawPlayers( int xpos_rel, float list_slot, int nameoffset, char *team )
{
	int can_show_packetloss = 0;
	int FAR_RIGHT;

	//  Packetloss removed on Kelly 'shipping nazi' Bailey's orders
	if ( cl_showpacketloss && cl_showpacketloss->value && ( ScreenWidth >= 400 ) )
	{
		can_show_packetloss = 1;
		SCOREBOARD_WIDTH = 400;
	}
	else
	{
		SCOREBOARD_WIDTH = 320;
	}

	FAR_RIGHT = can_show_packetloss ? PL_RANGE_MAX : PING_RANGE_MAX;
	FAR_RIGHT += 5;
	if (gHUD.m_Teamplay == 2)
		FAR_RIGHT = can_show_packetloss ? 395 : 350;

	// draw the players, in order,  and restricted to team if set
	while ( 1 )
	{
		// Find the top ranking player
		int highest_frags = -99999;	int lowest_deaths = 99999;
		int best_player = 0;

		for ( int i = 1; i < MAX_PLAYERS; i++ )
		{
			if ( g_PlayerInfoList[i].name && g_PlayerExtraInfo[i].frags + g_PlayerExtraInfo[i].flagcaptures >= highest_frags )
			{
				if ( !(team && stricmp(g_PlayerExtraInfo[i].teamname, team)) )  // make sure it is the specified team
				{
					extra_player_info_t *pl_info = &g_PlayerExtraInfo[i];
					if ( pl_info->frags + pl_info->flagcaptures > highest_frags || pl_info->deaths < lowest_deaths )
					{
						best_player = i;
						lowest_deaths = pl_info->deaths;
						highest_frags = pl_info->frags + pl_info->flagcaptures;
					}
				}
			}
		}

		if ( !best_player )
			break;

		// draw out the best player
		hud_player_info_t *pl_info = &g_PlayerInfoList[best_player];

		int ypos = ROW_RANGE_MIN + (list_slot * ROW_GAP);

		// check we haven't drawn too far down
		if ( ypos > ROW_RANGE_MAX )  // don't draw to close to the lower border
			break;
		int iconX = xpos_rel;
		for (int i = 0; i < 6; ++i)
		{
			icon_sprite_t& icon = g_PlayerSpriteList[best_player][i];
			if (icon.spr)
			{
				SPR_Set(icon.spr, icon.r, icon.g, icon.b);
				SPR_DrawAdditive(0, iconX, ypos, &icon.rc);
				iconX -= icon.rc.right - icon.rc.left + 5;
			}
		}

		int xpos = NAME_RANGE_MIN + xpos_rel;
		int r = 255, g = 255, b = 255;
		if ( best_player == m_iLastKilledBy && m_fLastKillTime && m_fLastKillTime > gHUD.m_flTime )
		{
			if ( pl_info->thisplayer )
			{  // green is the suicide color? i wish this could do grey...
				FillRGBA( NAME_RANGE_MIN + xpos_rel - 5, ypos, FAR_RIGHT, ROW_GAP, 80, 155, 0, 70 );
			}
			else
			{  // Highlight the killers name - overlay the background in red,  then draw the score text over it
				FillRGBA( NAME_RANGE_MIN + xpos_rel - 5, ypos, FAR_RIGHT, ROW_GAP, 255, 0, 0, ((float)15 * (float)(m_fLastKillTime - gHUD.m_flTime)) );
			}
		}
		else if ( pl_info->thisplayer ) // if it is their name, draw it a different color
		{
			// overlay the background in blue,  then draw the score text over it
			FillRGBA( NAME_RANGE_MIN + xpos_rel - 5, ypos, FAR_RIGHT, ROW_GAP, 0, 0, 255, 70 );
		}

		// draw their name (left to right)
		gHUD.DrawHudString( xpos + nameoffset, ypos, NAME_RANGE_MAX + xpos_rel, pl_info->name, r, g, b );

		// draw kills (right to left)
		xpos = KILLS_RANGE_MAX + xpos_rel;
		gHUD.DrawHudNumberString( xpos, ypos, KILLS_RANGE_MIN + xpos_rel, g_PlayerExtraInfo[best_player].frags, r, g, b );

		// draw divider
		xpos = DIVIDER_POS + xpos_rel;
		gHUD.DrawHudString( xpos, ypos, xpos + 20, "/", r, g, b );

		// draw deaths
		xpos = (gHUD.m_Teamplay == 2 ? 230 : DEATHS_RANGE_MAX) + xpos_rel;
		gHUD.DrawHudNumberString( xpos, ypos, DEATHS_RANGE_MIN + xpos_rel, g_PlayerExtraInfo[best_player].deaths, r, g, b );
		if (gHUD.m_Teamplay == 2)
		{
			gHUD.DrawHudString(xpos_rel + 240, ypos, xpos_rel + 260, "/", r, g, b);
			gHUD.DrawHudNumberString(xpos_rel + 275, ypos, xpos_rel + 250, g_PlayerExtraInfo[best_player].flagcaptures, r, g, b);
		}

		// draw ping & packetloss
		static char buf[64];
		sprintf( buf, "%d", g_PlayerInfoList[best_player].ping );
		xpos = xpos_rel + (gHUD.m_Teamplay == 2 ? 345 : 295);
		gHUD.DrawHudStringReverse( xpos, ypos, xpos - 50, buf, r, g, b );

		//  Packetloss removed on Kelly 'shipping nazi' Bailey's orders
		if ( can_show_packetloss )
		{
			if ( g_PlayerInfoList[best_player].packetloss >= 63 )
			{
				UnpackRGB( r, g, b, RGB_REDISH );
				sprintf( buf, " !!!!" );
			}
			else
			{
				sprintf( buf, "  %d", g_PlayerInfoList[best_player].packetloss );
			}

			xpos = xpos_rel + (gHUD.m_Teamplay == 2 ? 385 : 370);
		
			gHUD.DrawHudString( xpos, ypos, xpos+50, buf, r, g, b );
		}
	
		pl_info->name = NULL;  // set the name to be NULL, so this client won't get drawn again
		list_slot++;
	}

	return list_slot;
}


void CHudScoreboard :: GetAllPlayersInfo( void )
{
	for ( int i = 1; i < MAX_PLAYERS; i++ )
	{
		gEngfuncs.pfnGetPlayerInfo( i, &g_PlayerInfoList[i] );

		if ( g_PlayerInfoList[i].thisplayer )
			m_iPlayerNum = i;  // !!!HACK: this should be initialized elsewhere... maybe gotten from the engine
	}
}

int CHudScoreboard :: MsgFunc_ScoreInfo( const char *, int iSize, void *pbuf )
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	short frags = READ_SHORT();
	short deaths = READ_SHORT();
	short playerclass = READ_SHORT();
	short teamnumber = READ_SHORT();

	if ( cl > 0 && cl <= MAX_PLAYERS )
	{
		g_PlayerExtraInfo[cl].frags = frags;
		g_PlayerExtraInfo[cl].deaths = deaths;
		g_PlayerExtraInfo[cl].playerclass = playerclass;
		g_PlayerExtraInfo[cl].teamnumber = teamnumber;

		gViewPort->UpdateOnPlayerInfo();
	}

	return 1;
}

int CHudScoreboard::MsgFunc_CTFScore(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	int player = READ_BYTE();
	short captures = (short)READ_BYTE();
	if (player > 0 && player <= MAX_PLAYERS)
		g_PlayerExtraInfo[player].flagcaptures = captures;
	return 1;
}

int CHudScoreboard::MsgFunc_PlayerIcon(const char*, int size, void* buffer)
{
	BEGIN_READ(buffer, size);
	int player = READ_BYTE();
	int enable = READ_BYTE();
	int index = READ_BYTE();
	unsigned char flags = (unsigned char)READ_BYTE();
	if (player < 1 || player > MAX_PLAYERS)
		return 1;
	icon_sprite_t* icons = g_PlayerSpriteList[player];
	if (!enable)
	{
		for (int i = 0; i < 6; ++i)
			if (icons[i].bFlags & flags)
				memset(&icons[i], 0, sizeof(icons[i]));
		return 1;
	}
	if (!flags)
	{
		memset(&icons[index], 0, sizeof(icons[index]));
		return 1;
	}
	if (flags & 3)
	{
		int sprite = gHUD.GetSpriteIndex("score_flag");
		icons[0].spr = gHUD.GetSprite(sprite);
		icons[0].rc = gHUD.GetSpriteRect(sprite);
		icons[0].r = (flags & 1) ? 255 : 0;
		icons[0].g = 160;
		icons[0].b = 0;
		icons[0].bFlags = flags;
		strcpy(icons[index].szSpriteName, "score_flag");
		return 1;
	}
	for (int i = 1; i < 6; ++i)
		if (icons[i].bFlags & flags)
			return 1;
	int slot;
	for (slot = 1; slot < 6 && icons[slot].spr; ++slot)
	{
	}
	if (slot < 6)
	{
		const char* name = NULL;
		unsigned char red = 0, green = 0, blue = 0;
		if (flags & 4) { name = "score_ctfljump"; red = 255; green = 160; }
		else if (flags & 8) { name = "score_ctfphev"; red = 128; green = 160; blue = 255; }
		else if (flags & 128) { name = "score_ctfregen"; green = 255; }
		else if (flags & 32) { name = "score_ctfaccel"; red = 255; }
		else if (flags & 16) { name = "score_ctfbpack"; red = green = 255; }
		if (name)
		{
			int sprite = gHUD.GetSpriteIndex(name);
			icons[slot].spr = gHUD.GetSprite(sprite);
			icons[slot].rc = gHUD.GetSpriteRect(sprite);
			icons[slot].r = red;
			icons[slot].g = green;
			icons[slot].b = blue;
			icons[slot].bFlags = flags;
			strcpy(icons[slot].szSpriteName, name);
		}
	}
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

// Message handler for TeamInfo message
// accepts two values:
//		byte: client number
//		string: client team name
int CHudScoreboard :: MsgFunc_TeamInfo( const char *, int iSize, void *pbuf )
{
	int i, j;
	BEGIN_READ( pbuf, iSize );
	short cl = READ_BYTE();
	
	if ( cl > 0 && cl <= MAX_PLAYERS )
	{  // set the players team
		strncpy( g_PlayerExtraInfo[cl].teamname, READ_STRING(), MAX_TEAM_NAME );
	}

	// rebuild the list of teams

	// clear out player counts from teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		g_TeamInfo[i].players = 0;
	}

	// rebuild the team list
	GetAllPlayersInfo();
	m_iNumTeams = 0;
	for ( i = 1; i < MAX_PLAYERS; i++ )
	{
		if ( g_PlayerInfoList[i].name == NULL )
			continue;

		if ( g_PlayerExtraInfo[i].teamname[0] == 0 )
			continue; // skip over players who are not in a team

		// is this player in an existing team?
		for ( j = 1; j <= m_iNumTeams; j++ )
		{
			if ( g_TeamInfo[j].name[0] == '\0' )
				break;

			if ( !stricmp( g_PlayerExtraInfo[i].teamname, g_TeamInfo[j].name ) )
				break;
		}

		if ( j > m_iNumTeams )
		{ // they aren't in a listed team, so make a new one
			// search through for an empty team slot
			for ( j = 1; j <= m_iNumTeams; j++ )
			{
				if ( g_TeamInfo[j].name[0] == '\0' )
					break;
			}
			m_iNumTeams = max( j, m_iNumTeams );

			strncpy( g_TeamInfo[j].name, g_PlayerExtraInfo[i].teamname, MAX_TEAM_NAME );
			g_TeamInfo[j].players = 0;
		}

		g_TeamInfo[j].players++;
	}

	// clear out any empty teams
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( g_TeamInfo[i].players < 1 )
			memset( &g_TeamInfo[i], 0, sizeof(team_info_t) );
	}

	return 1;
}

// Message handler for TeamScore message
// accepts three values:
//		string: team name
//		short: teams kills
//		short: teams deaths 
// if this message is never received, then scores will simply be the combined totals of the players.
int CHudScoreboard :: MsgFunc_TeamScore( const char *, int iSize, void *pbuf )
{
	int i;
	BEGIN_READ( pbuf, iSize );
	char *TeamName = READ_STRING();

	// find the team matching the name
	for ( i = 1; i <= m_iNumTeams; i++ )
	{
		if ( !stricmp( TeamName, g_TeamInfo[i].name ) )
			break;
	}
	if ( i > m_iNumTeams )
		return 1;

	// use this new score data instead of combined player scores
	g_TeamInfo[i].scores_overriden = TRUE;
	g_TeamInfo[i].frags = READ_SHORT();
	g_TeamInfo[i].deaths = READ_SHORT();
	
	return 1;
}

void CHudScoreboard :: DeathMsg( int killer, int victim )
{
	// if we were the one killed,  or the world killed us, set the scoreboard to indicate suicide
	if ( victim == m_iPlayerNum || killer == 0 )
	{
		m_iLastKilledBy = killer ? killer : m_iPlayerNum;
		m_fLastKillTime = gHUD.m_flTime + 10;	// display who we were killed by for 10 seconds

		if ( killer == m_iPlayerNum )
			m_iLastKilledBy = m_iPlayerNum;
	}
}



void CHudScoreboard :: UserCmd_ShowScores( void )
{
	m_iShowscoresHeld = TRUE;
}

void CHudScoreboard :: UserCmd_HideScores( void )
{
	m_iShowscoresHeld = FALSE;
}
