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
#include <stdlib.h>
#include <time.h>

DECLARE_MESSAGE(m_Geiger, Geiger)

int CHudGeiger::Init()
{
	HOOK_MESSAGE(Geiger);
	m_iGeigerRange = 0;
	m_iFlags = 0;
	gHUD.AddHudElem(this);
	srand((unsigned int)time(NULL));
	return 1;
}

int CHudGeiger::VidInit()
{
	return 1;
}

int CHudGeiger::MsgFunc_Geiger(const char *, int size, void *buffer)
{
	BEGIN_READ(buffer, size);
	int range = READ_BYTE();
	m_iFlags |= HUD_ACTIVE;
	m_iGeigerRange = 4 * range;
	return 1;
}

int CHudGeiger::Draw(float)
{
	if (m_iGeigerRange <= 0 || m_iGeigerRange >= 1000)
		return 1;
	// peicewise linear is better than continuous formula for this
	if (m_iGeigerRange > 800)
	{
		(void)rand();
		(void)rand();
		(void)rand();
		return 1;
	}
	int variants = 3;
	int probability;
	float volume;
	if (m_iGeigerRange > 600)
	{
		variants = 2;
		probability = 2;
		volume = 0.4f;
	}
	else if (m_iGeigerRange > 500)
	{
		variants = 2;
		probability = 4;
		volume = 0.5f;
	}
	else if (m_iGeigerRange > 400)
	{
		probability = 8;
		volume = 0.6f;
	}
	else if (m_iGeigerRange > 300)
	{
		probability = 8;
		volume = 0.7f;
	}
	else if (m_iGeigerRange > 200)
	{
		probability = 28;
		volume = 0.78f;
	}
	else if (m_iGeigerRange > 150)
	{
		probability = 40;
		volume = 0.8f;
	}
	else if (m_iGeigerRange > 100)
	{
		probability = 60;
		volume = 0.85f;
	}
	else if (m_iGeigerRange > 75)
	{
		probability = 80;
		volume = 0.9f;
	}
	else
	{
		variants = 2;
		if (m_iGeigerRange > 50)
		{
			probability = 90;
			volume = 0.95f;
		}
		else
		{
			probability = 95;
			volume = 1;
		}
	}
	float soundVolume = (volume * (rand() & 127) / 255) + 0.25;
	if (probability > (rand() & 127) || probability > (rand() & 127))
	{
		int sample = rand() & 1;
		if (variants == 3)
			sample += rand() & 1;
		char name[256];
		sprintf(name, "player/geiger%d.wav", sample + 1);
		gEngfuncs.pfnPlaySoundByName(name, soundVolume);
	}
	return 1;
}
