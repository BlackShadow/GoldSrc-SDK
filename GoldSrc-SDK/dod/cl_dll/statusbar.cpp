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
// statusbar.cpp
//
// generic text status bar, set by game dll
// runs across bottom of screen
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "event_api.h"
#include "pmtrace.h"
#include "../../pm_shared/pm_defs.h"
#include "entity_types.h"
#include "../public/game_controls/iviewport.h"
#include <stdio.h>

extern int g_iAlive;
extern int g_iDeadFlag;
extern Vector v_angles;
extern cvar_t *cl_identiconmode;
extern float *GetClientColor( int index );

DECLARE_MESSAGE( m_StatusBar, StatusValue );

int CHudStatusBar::Init()
{
	gHUD.AddHudElem( this );
	m_iFlags |= HUD_ACTIVE;
	HOOK_MESSAGE( StatusValue );
	Reset();
	m_flNextUpdateTime = 0;
	hud_centerid = CVAR_CREATE( "hud_centerid", "1", FCVAR_ARCHIVE );
	for ( int i = 1; i < 64; ++i )
		m_TargetTalking[i] = 0;
	return 1;
}

int CHudStatusBar::VidInit()
{
	m_flNextUpdateTime = 0;
	Reset();
	m_StatusBarAllieModel = SPR_Load( "sprites/american.spr" );
	m_StatusBarBritishModel = SPR_Load( "sprites/british.spr" );
	m_StatusBarGermanModel = SPR_Load( "sprites/german.spr" );
	return 1;
}

void CHudStatusBar::Reset()
{
	m_iHealth = 100;
	m_iTargetIndex = -1;
	m_iTargetTeam = 0;
}

bool CHudStatusBar::InDeathCamMode()
{
	cl_entity_t *target = gEngfuncs.GetEntityByIndex( g_iUser2 );
	return (unsigned int)(g_iDeadFlag - 1) <= 2 && g_iUser3 > 0 && g_iUser3 <= gEngfuncs.GetMaxClients() &&
		target && g_iUser1 == 2 && target->index == gEngfuncs.GetLocalPlayer()->index;
}

void CHudStatusBar::Think()
{
	if ( m_flNextUpdateTime > gEngfuncs.GetClientTime() )
		return;
	gViewPortInterface->UpdateSpectatorPanel();
	int index = -1;
	int team = 0;
	if ( InDeathCamMode() )
	{
		cl_entity_t *target = gEngfuncs.GetEntityByIndex( g_iUser3 );
		if ( target && target->index != gEngfuncs.GetLocalPlayer()->index && (unsigned int)(target->index - 1) <= 63 )
		{
			index = target->index;
			team = g_PlayerExtraInfo[index].teamnumber;
		}
	}
	else
	{
		cl_entity_t *player = gEngfuncs.GetLocalPlayer();
		Vector height, forward, right, up;
		gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( height );
		Vector start = player->curstate.origin + height;
		gEngfuncs.pfnAngleVectors( v_angles, forward, right, up );
		Vector end = forward * 2048 + start;
		gEngfuncs.pEventAPI->EV_SetSolidPlayers( player->index - 1 );
		gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );
		pmtrace_t trace;
		gEngfuncs.pEventAPI->EV_PlayerTrace( start, end, PM_STUDIO_BOX, -1, &trace );
		int hit = gEngfuncs.pEventAPI->EV_IndexFromTrace( &trace );
		if ( hit > 0 && hit <= gEngfuncs.GetMaxClients() )
		{
			cl_entity_t *target = gEngfuncs.GetEntityByIndex( hit );
			player = gEngfuncs.GetLocalPlayer();
			gEngfuncs.pfnGetPlayerInfo( target->index, &g_PlayerInfoList[target->index] );
			gEngfuncs.pfnGetPlayerInfo( player->index, &g_PlayerInfoList[player->index] );
			int playerTeam = g_PlayerExtraInfo[player->index].teamnumber;
			if ( g_PlayerExtraInfo[target->index].teamnumber == playerTeam || playerTeam == 3 || g_iDeadFlag > 1 )
			{
				index = target->index;
				team = g_PlayerExtraInfo[index].teamnumber;
			}
		}
	}
	m_iTargetIndex = index;
	m_iTargetTeam = team;
	m_flNextUpdateTime = gEngfuncs.GetClientTime() + 0.1f;
}

int CHudStatusBar::MsgFunc_StatusValue( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	m_iHealth = READ_BYTE();
	if ( g_iUser1 )
		gViewPortInterface->UpdateSpectatorPanel();
	return 1;
}

char *CHudStatusBar::GetTargetName()
{
	if ( (unsigned int)(m_iTargetIndex - 1) <= 63 )
	{
		hud_player_info_t info;
		gEngfuncs.pfnGetPlayerInfo( m_iTargetIndex, &info );
		return info.name;
	}
	return "";
}

int CHudStatusBar::Draw( float )
{
	if ( !g_iUser1 && g_iAlive && (unsigned int)(m_iTargetIndex - 1) <= 63 )
	{
		hud_player_info_t info;
		gEngfuncs.pfnGetPlayerInfo( m_iTargetIndex, &info );
		float *color = GetClientColor( m_iTargetIndex );
		if ( m_iHealth > 0 )
		{
			char text[128];
			sprintf( text, "%s (%d)", info.name, m_iHealth );
			int width, height;
			gEngfuncs.pfnDrawConsoleStringLen( text, &width, &height );
			int x, y;
			if ( hud_centerid && hud_centerid->value > 0 )
			{
				x = max( 2, ScreenWidth - width ) >> 1;
				y = (int)(float)(hud_centerid->value * (double)height + height + ScreenHeight / 2);
			}
			else
			{
				x = ScreenWidth / 2 - width / 2;
				y = ScreenHeight - 2 * height;
			}
			gEngfuncs.pfnDrawSetTextColor( color[0], color[1], color[2] );
			gEngfuncs.pfnDrawConsoleString( x, y, text );
		}
	}
	return 1;
}

void CHudStatusBar::DrawEntitiesOverTeam()
{
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();
	for ( int i = 1; i < 64; ++i )
	{
		cl_entity_t *target = gEngfuncs.GetEntityByIndex( i );
		if ( !target || target->curstate.messagenum < player->curstate.messagenum ||
			(target->curstate.effects & EF_NODRAW) || target == player || m_TargetTalking[i] )
			continue;
		if ( target->curstate.team != player->curstate.team || !target->curstate.solid || g_PlayerExtraInfo[i].dead )
			continue;
		m_StatusBarHeadModel = target->curstate.team == 1 ?
			(gHUD.m_bBritish ? m_StatusBarBritishModel : m_StatusBarAllieModel) : m_StatusBarGermanModel;
		cl_entity_t *marker = &m_TargetHeadModel[i];
		memset( marker, 0, sizeof( *marker ) );
		marker->curstate.rendermode = kRenderTransAdd;
		marker->curstate.renderamt = 127;
		marker->curstate.frame = 0;
		marker->baseline.renderamt = 127;
		marker->curstate.renderfx = kRenderFxNoDissipation;
		marker->curstate.framerate = 1;
		marker->model = (model_t *)gEngfuncs.GetSpritePointer( m_StatusBarHeadModel );
		marker->angles = Vector( 0, 0, 0 );
		marker->origin = target->origin + Vector( 0, 0, 45 );
		float distance = (marker->origin - player->origin).Length();
		marker->curstate.scale = distance >= 378 && !g_lastFOV ? min( distance / 175 / 10, 2.0f ) : 0.2f;
		gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, marker );
	}
}

void CHudStatusBar::DrawEntitiesOverTarget()
{
	if ( m_iTargetIndex < 0 || m_iTargetIndex >= 64 )
		return;
	cl_entity_t *player = gEngfuncs.GetLocalPlayer();
	cl_entity_t *target = gEngfuncs.GetEntityByIndex( m_iTargetIndex );
	if ( !target || target->curstate.messagenum < player->curstate.messagenum ||
		(target->curstate.effects & EF_NODRAW) || target == player ||
		target->curstate.team != player->curstate.team || m_TargetTalking[m_iTargetIndex] == 1 )
		return;
	m_StatusBarHeadModel = target->curstate.team == 1 ?
		(gHUD.m_bBritish ? m_StatusBarBritishModel : m_StatusBarAllieModel) : m_StatusBarGermanModel;
	cl_entity_t *marker = &m_TargetHeadModel[0];
	memset( marker, 0, sizeof( *marker ) );
	marker->curstate.rendermode = kRenderTransAdd;
	marker->curstate.renderamt = 127;
	marker->curstate.renderfx = kRenderFxNoDissipation;
	marker->baseline.renderamt = 127;
	marker->curstate.frame = 0;
	marker->curstate.framerate = 1;
	marker->model = (model_t *)gEngfuncs.GetSpritePointer( m_StatusBarHeadModel );
	marker->angles = Vector( 0, 0, 0 );
	marker->origin = target->origin + Vector( 0, 0, 45 );
	float distance = (marker->origin - player->origin).Length();
	marker->curstate.scale = distance >= 378 && !g_lastFOV ? min( distance / 175 / 10, 2.0f ) : 0.2f;
	gEngfuncs.CL_CreateVisibleEntity( ET_NORMAL, marker );
}

void CHudStatusBar::CreateEntities()
{
	if ( m_StatusBarAllieModel && m_StatusBarBritishModel && m_StatusBarGermanModel &&
		!g_iUser1 && !(g_iEffects & EF_NODRAW) && !g_iDeadFlag )
	{
		if ( cl_identiconmode->value >= 2 )
			DrawEntitiesOverTeam();
		else if ( cl_identiconmode->value >= 1 )
			DrawEntitiesOverTarget();
	}
}
