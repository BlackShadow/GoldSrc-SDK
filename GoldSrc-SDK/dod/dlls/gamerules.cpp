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
// GameRules.cpp
//=========================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "skill.h"

extern edict_t *EntSelectSpawnPoint(CBaseEntity *pPlayer, int bJustChecking);

DLL_GLOBAL CGameRules *g_pGameRules = NULL;
int g_teamplay;

void CGameRules::RefreshSkillData(void)
{
	int iSkill = (int)CVAR_GET_FLOAT("skill");
	g_iSkillLevel = iSkill;
	if (iSkill < 1)
		iSkill = 1;
	else if (iSkill > 3)
		iSkill = 3;
	gSkillData.iSkillLevel = iSkill;
	ALERT(at_console, "\nGAME SKILL LEVEL:%d\n", iSkill);
	gSkillData.turretHealth = GetSkillCvar("sk_turret_health");

	// MiniTurret
	gSkillData.miniturretHealth = GetSkillCvar("sk_miniturret_health");
	gSkillData.sentryHealth = GetSkillCvar("sk_sentry_health");

	// monster damage adj
	gSkillData.monHead = GetSkillCvar("sk_monster_head");
	gSkillData.monChest = GetSkillCvar("sk_monster_chest");
	gSkillData.monStomach = GetSkillCvar("sk_monster_stomach");
	gSkillData.monLeg = GetSkillCvar("sk_monster_leg");
	gSkillData.monArm = GetSkillCvar("sk_monster_arm");

	// player damage adj
	gSkillData.plrHead = GetSkillCvar("sk_player_head");
	gSkillData.plrChest = GetSkillCvar("sk_player_chest");
	gSkillData.plrStomach = GetSkillCvar("sk_player_stomach");
	gSkillData.plrLeg = GetSkillCvar("sk_player_leg");
	gSkillData.plrArm = GetSkillCvar("sk_player_arm");
}

edict_t *CGameRules::GetPlayerSpawnSpot(CBasePlayer *pPlayer)
{
	edict_t *pSpot = EntSelectSpawnPoint(pPlayer, FALSE);
	if (pSpot)
	{
		pPlayer->pev->origin = pSpot->v.origin + Vector(0, 0, 1);
		pPlayer->pev->velocity = g_vecZero;
		pPlayer->pev->angles = pSpot->v.angles;
		pPlayer->pev->v_angle = pPlayer->pev->angles;
		pPlayer->pev->punchangle = g_vecZero;
		pPlayer->pev->fixangle = TRUE;
		pPlayer->m_flGaityaw = pPlayer->pev->angles.y;
	}
	return pSpot;
}

BOOL CGameRules::CanHaveAmmo(CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry)
{
	if (pszAmmoName)
	{
		int iAmmoIndex = pPlayer->GetAmmoIndex(pszAmmoName);
		if (iAmmoIndex >= 0)
			return pPlayer->AmmoInventory(iAmmoIndex) < iMaxCarry;
	}
	return FALSE;
}

BOOL CGameRules::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
	// only living players can have items
	if (pPlayer->pev->deadflag != DEAD_NO)
		return FALSE;
	if (pWeapon->pszAmmo1())
	{
		if (CanHaveAmmo(pPlayer, pWeapon->pszAmmo1(), pWeapon->iMaxAmmo1()))
			return TRUE;
	}
	return !pPlayer->HasPlayerItem(pWeapon);
}

void CGameRules::GetInitialSpawnSpot(CBasePlayer *pPlayer)
{
	CBaseEntity *pSpot = UTIL_FindEntityByClassname(NULL, "info_player_observer");
	if (pSpot)
	{
		pPlayer->pev->origin = pSpot->pev->origin + Vector(0, 0, 1);
		pPlayer->pev->angles = pSpot->pev->angles;
	}
	else
	{
		ALERT(at_console, "GetInitialSpawnSpot: no info_player_observer in level - starting player at 0,0,0\n");
		pPlayer->pev->origin = g_vecZero;
		pPlayer->pev->angles = g_vecZero;
	}
	pPlayer->pev->v_angle = g_vecZero;
	pPlayer->pev->velocity = g_vecZero;
	pPlayer->pev->punchangle = g_vecZero;
	pPlayer->pev->fixangle = TRUE;
}

CGameRules *InstallGameRules(void)
{
	SERVER_COMMAND("exec game.cfg\n");
	SERVER_EXECUTE();
	g_teamplay = 1;
	return new CDoDTeamPlay;
}

void CGameRules::ChangePlayerTeam(CBasePlayer *, const char *, BOOL, BOOL)
{
}

int CGameRules::SetRoundState(void)
{
	return 0;
}
