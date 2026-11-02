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
//  hud_msg.cpp
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#pragma warning( push )
#pragma warning( disable: 4018 )
#include "particleman.h"
#pragma warning( pop )
#include "../public/game_controls/iviewport.h"
#include "vgui/idodviewport.h"

extern int g_iAlive;
extern IParticleMan *g_pParticleMan;
extern struct beam_s *pBeam;
extern struct beam_s *pBeam2;
extern void DoD_LoadClientEnts( const char *map );

int CHud::MsgFunc_ResetHUD( const char *, int, void * )
{
	for ( HUDLIST *element = m_pHudList; element; element = element->pNext )
	{
		if ( element->p )
			element->p->Reset();
	}
	if ( g_iAlive )
	{
		m_iFOV = 0;
		g_lastFOV = 0;
		m_flMouseSensitivity = 0;
		for ( int i = 0; i < 64; ++i )
			m_PlayerFOV[i] = 90;
		if ( g_pParticleMan )
			g_pParticleMan->ResetParticles();
	}
	return 1;
}

int CHud::MsgFunc_YouDied( const char *, int, void * )
{
	for ( HUDLIST *element = m_pHudList; element; element = element->pNext )
	{
		if ( element->p )
			element->p->PlayerDied();
	}
	m_flMouseSensitivity = 0;
	if ( !g_iVuser1z && gDoDViewPortInterface->isWidescreenVisible() )
		return 1;
	m_iFOV = 0;
	g_lastFOV = 0;
	return 1;
}

void CHud::MsgFunc_InitHUD( const char *, int, void * )
{
	if ( gViewPortInterface )
		gViewPortInterface->OnLevelChange();
	for ( HUDLIST *element = m_pHudList; element; element = element->pNext )
	{
		if ( element->p )
			element->p->InitHUDData();
	}
	pBeam2 = NULL;
	pBeam = NULL;
	DoD_LoadClientEnts( gEngfuncs.pfnGetLevelName() );
}

int CHud::MsgFunc_Damage( const char *, int size, void *buffer )
{
	BEGIN_READ( buffer, size );
	READ_BYTE();
	READ_BYTE();
	READ_COORD();
	READ_COORD();
	READ_COORD();
	return 1;
}
