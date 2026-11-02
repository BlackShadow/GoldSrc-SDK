/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
//=========================================================
// skill.h - skill level concerns
//=========================================================

#pragma once

struct skilldata_t
{
	int iSkillLevel; // game skill level

	float plrDmg9MM;
	float plrDmg357;
	float plrDmgMP5;
	float plrDmgM203Grenade;
	float plrDmgBuckshot;
	float plrDmgCrossbowClient;
	float plrDmgRPG;

// weapons shared by monsters
	float monDmg9MM;
	float monDmgMP5;
	float monDmg12MM;

// health/suit charge
	float suitchargerCapacity;
	float batteryCapacity;
	float healthchargerCapacity;
	float healthkitCapacity;
};

#define SKILL_EASY		1
#define SKILL_MEDIUM	2
#define SKILL_HARD		3

extern skilldata_t gSkillData;
extern int g_iSkillLevel;

float GetSkillCvar( char* name );
