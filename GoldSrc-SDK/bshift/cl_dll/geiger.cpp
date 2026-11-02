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
// Geiger.cpp
//
// implementation of CHudAmmo class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

DECLARE_MESSAGE( m_Geiger, Geiger )

int CHudGeiger::Init()
{
	HOOK_MESSAGE( Geiger );
	m_iGeigerRange = 0;
	m_iFlags = 0;
	gHUD.AddHudElem( this );
	srand( (unsigned)time( NULL ) );
	return 1;
}

int CHudGeiger::VidInit()
{
	return 1;
}

int CHudGeiger::MsgFunc_Geiger( const char*, int iSize, void* pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_iGeigerRange = READ_BYTE() << 2;
	m_iFlags |= HUD_ACTIVE;
	return 1;
}

int CHudGeiger::Draw( float )
{
	if ( m_iGeigerRange <= 0 || m_iGeigerRange >= 1000 )
		return 1;
	// peicewise linear is better than continuous formula for this

	if ( m_iGeigerRange > 800 )
	{
		(void)rand();
		(void)rand();
		(void)rand();
		return 1;
	}

	int pct;
	int count;
	float volume;

	if ( m_iGeigerRange > 600 )
	{
		pct = 2;
		volume = 0.4f;
		count = 2;
	}
	else if ( m_iGeigerRange > 500 )
	{
		pct = 4;
		volume = 0.5f;
		count = 2;
	}
	else if ( m_iGeigerRange > 400 )
	{
		pct = 8;
		volume = 0.6f;
		count = 3;
	}
	else if ( m_iGeigerRange > 300 )
	{
		pct = 8;
		volume = 0.7f;
		count = 3;
	}
	else if ( m_iGeigerRange > 200 )
	{
		pct = 28;
		volume = 0.78f;
		count = 3;
	}
	else if ( m_iGeigerRange > 150 )
	{
		pct = 40;
		volume = 0.80f;
		count = 3;
	}
	else if ( m_iGeigerRange > 100 )
	{
		pct = 60;
		volume = 0.85f;
		count = 3;
	}
	else if ( m_iGeigerRange > 75 )
	{
		pct = 80;
		volume = 0.9f;
		count = 3;
	}
	else if ( m_iGeigerRange > 50 )
	{
		pct = 90;
		volume = 0.95f;
		count = 2;
	}
	else
	{
		pct = 95;
		volume = 1.0f;
		count = 2;
	}

	volume = ( volume * ( rand() & 127 ) / 255 ) + 0.25; // UTIL_RandomFloat(0.25, 0.5);
	if ( ( rand() & 127 ) < pct || ( rand() & 127 ) < pct )
	{
		int sound = rand() & 1;
		if ( count > 2 )
			sound += rand() & 1;

		char name[256];
		sprintf( name, "player/geiger%d.wav", sound + 1 );
		PlaySound( name, volume );
	}
	return 1;
}
