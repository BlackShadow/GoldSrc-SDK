/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
// teamplay_gamerules.cpp
//
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "items.h"
#include "gamerules.h"
#include "skill.h"
#include "../cl_dll/dod_shared.h"

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CSPDoDRules::CSPDoDRules()
{
	CGameRules::RefreshSkillData();
}

void CSPDoDRules::Init(void)
{
}

void CSPDoDRules::Think(void)
{
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::IsMultiplayer(void)
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::IsDeathmatch(void)
{
	return FALSE;
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::IsCoOp(void)
{
	return FALSE;
}

BOOL CSPDoDRules::FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *)
{
	return !pPlayer->m_pActiveItem || pPlayer->m_pActiveItem->CanHolster();
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::GetNextBestWeapon(CBasePlayer *, CBasePlayerItem *)
{
	return FALSE;
}

BOOL CSPDoDRules::ClientConnected(edict_t *, const char *, const char *, char *)
{
	return TRUE;
}

void CSPDoDRules::InitHUD(CBasePlayer *)
{
}

//=========================================================
//=========================================================
void CSPDoDRules::ClientDisconnected(edict_t *)
{
}

//=========================================================
//=========================================================
float CSPDoDRules::FlPlayerFallDamage(CBasePlayer *pPlayer)
{
	pPlayer->m_flFallVelocity -= 500;
	return pPlayer->m_flFallVelocity * 100 / 400;
}

BOOL CSPDoDRules::AllowAutoTargetCrosshair(void)
{
	return g_iSkillLevel == 1;
}

//=========================================================
//=========================================================
void CSPDoDRules::PlayerThink(CBasePlayer *)
{
}


//=========================================================
//=========================================================
BOOL CSPDoDRules::FPlayerCanRespawn(CBasePlayer *)
{
	return TRUE;
}

//=========================================================
//=========================================================
float CSPDoDRules::FlPlayerSpawnTime(CBasePlayer *)
{
	return gpGlobals->time; //now!
}

//=========================================================
// IPointsForKill - how many points awarded to anyone
// that kills this player?
//=========================================================
int CSPDoDRules::IPointsForKill(CBasePlayer *, CBasePlayer *)
{
	return 1;
}

//=========================================================
// PlayerKilled - someone/something killed this player
//=========================================================
void CSPDoDRules::PlayerKilled(CBasePlayer *, entvars_t *, entvars_t *, int)
{
}

//=========================================================
// Deathnotice
//=========================================================
void CSPDoDRules::DeathNotice(CBasePlayer *, entvars_t *, entvars_t *, int)
{
}

//=========================================================
// PlayerGotWeapon - player has grabbed a weapon that was
// sitting in the world
//=========================================================
void CSPDoDRules::PlayerGotWeapon(CBasePlayer *, CBasePlayerItem *)
{
}

//=========================================================
// FlWeaponRespawnTime - what is the time in the future
// at which this weapon may spawn?
//=========================================================
float CSPDoDRules::FlWeaponRespawnTime(CBasePlayerItem *)
{
	return -1;
}

//=========================================================
// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
// now,  otherwise it returns the time at which it can try
// to spawn again.
//=========================================================
float CSPDoDRules::FlWeaponTryRespawn(CBasePlayerItem *)
{
	return 0;
}

//=========================================================
// VecWeaponRespawnSpot - where should this weapon spawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CSPDoDRules::VecWeaponRespawnSpot(CBasePlayerItem *pWeapon)
{
	return pWeapon->pev->origin;
}

//=========================================================
// WeaponShouldRespawn - any conditions inhibiting the
// respawning of this weapon?
//=========================================================
int CSPDoDRules::WeaponShouldRespawn(CBasePlayerItem *)
{
	return GR_WEAPON_RESPAWN_NO;
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::CanHaveItem(CBasePlayer *, CItem *)
{
	return TRUE;
}

//=========================================================
//=========================================================
void CSPDoDRules::PlayerGotItem(CBasePlayer *, CItem *)
{
}

//=========================================================
//=========================================================
int CSPDoDRules::ItemShouldRespawn(CItem *)
{
	return GR_ITEM_RESPAWN_NO;
}


//=========================================================
// At what time in the future may this Item respawn?
//=========================================================
float CSPDoDRules::FlItemRespawnTime(CItem *)
{
	return -1;
}

//=========================================================
// Where should this item respawn?
// Some game variations may choose to randomize spawn locations
//=========================================================
Vector CSPDoDRules::VecItemRespawnSpot(CItem *pItem)
{
	return pItem->pev->origin;
}

//=========================================================
//=========================================================
BOOL CSPDoDRules::IsAllowedToSpawn(CBaseEntity *)
{
	return TRUE;
}

//=========================================================
//=========================================================
void CSPDoDRules::PlayerGotAmmo(CBasePlayer *, char *, int)
{
}

//=========================================================
//=========================================================
int CSPDoDRules::AmmoShouldRespawn(CBasePlayerAmmo *)
{
	return GR_AMMO_RESPAWN_NO;
}

//=========================================================
//=========================================================
float CSPDoDRules::FlAmmoRespawnTime(CBasePlayerAmmo *)
{
	return -1;
}

//=========================================================
//=========================================================
Vector CSPDoDRules::VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo)
{
	return pAmmo->pev->origin;
}

//=========================================================
//=========================================================
float CSPDoDRules::FlHealthChargerRechargeTime(void)
{
	return 0; // don't recharge
}

//=========================================================
//=========================================================
int CSPDoDRules::DeadPlayerWeapons(CBasePlayer *)
{
	return GR_PLR_DROP_GUN_NO;
}

//=========================================================
//=========================================================
int CSPDoDRules::DeadPlayerAmmo(CBasePlayer *)
{
	return GR_PLR_DROP_AMMO_NO;
}

//=========================================================
//=========================================================
int CSPDoDRules::PlayerRelationship(CBaseEntity *, CBaseEntity *)
{
	// why would a single player in half life need this? 
	return GR_NOTTEAMMATE;
}

BOOL CSPDoDRules::FAllowMonsters(void)
{
	return TRUE;
}

int CSPDoDRules::RoundState(void)
{
	return RND_RUNNING;
}

void CSPDoDRules::SetRoundState(int)
{
}

void CSPDoDRules::SetRestartTime(float)
{
}

int CSPDoDRules::TeamScores(int)
{
	return 1;
}

void CSPDoDRules::SetTeamScores(int, int)
{
}

void CSPDoDRules::AddToTeamScores(int, int)
{
}

unsigned short CSPDoDRules::Camera(void)
{
	return m_Camera;
}

unsigned short CSPDoDRules::usVoice(void)
{
	return m_usVoice;
}

unsigned short CSPDoDRules::bodyDamage(void)
{
	return m_bodyDamage;
}

unsigned short CSPDoDRules::roundRestartSound(void)
{
	return m_roundRestartSound;
}

unsigned short CSPDoDRules::gerVoice(void)
{
	return m_gerVoice;
}

unsigned short CSPDoDRules::Pain(void)
{
	return m_Pain;
}

unsigned short CSPDoDRules::Smoke(void)
{
	return m_Smoke;
}

unsigned short CSPDoDRules::Prone(void)
{
	return m_Prone;
}

unsigned short CSPDoDRules::BloodSprite(void)
{
	return m_BloodSprite;
}

unsigned short CSPDoDRules::BloodStream(void)
{
	return m_BloodStream;
}

unsigned short CSPDoDRules::BulletTracers(void)
{
	return m_BulletTracers;
}

unsigned short CSPDoDRules::SparkShower(void)
{
	return m_SparkShower;
}

unsigned short CSPDoDRules::ScreenFades(void)
{
	return m_ScreenFades;
}

unsigned short CSPDoDRules::BubbleTrails(void)
{
	return m_BubbleTrails;
}

unsigned short CSPDoDRules::Bubbles(void)
{
	return m_Bubbles;
}

unsigned short CSPDoDRules::Shards(void)
{
	return m_Shards;
}

unsigned short CSPDoDRules::Explosion(void)
{
	return m_Explosion;
}

unsigned short CSPDoDRules::PopHelmet(void)
{
	return m_PopHelmet;
}

unsigned short CSPDoDRules::RoundReset(void)
{
	return m_RoundReset;
}

unsigned short CSPDoDRules::StartTrail(void)
{
	return m_RocketTrail;
}

void CSPDoDRules::SetCamera(short sPrecache)
{
	m_Camera = sPrecache;
}

void CSPDoDRules::SetusVoice(short sPrecache)
{
	m_usVoice = sPrecache;
}

void CSPDoDRules::SetbodyDamage(short sPrecache)
{
	m_bodyDamage = sPrecache;
}

void CSPDoDRules::SetroundRestartSound(short sPrecache)
{
	m_roundRestartSound = sPrecache;
}

void CSPDoDRules::SetgerVoice(short sPrecache)
{
	m_gerVoice = sPrecache;
}

void CSPDoDRules::SetPain(short sPrecache)
{
	m_Pain = sPrecache;
}

void CSPDoDRules::SetSmoke(short sPrecache)
{
	m_Smoke = sPrecache;
}

void CSPDoDRules::SetProne(short sPrecache)
{
	m_Prone = sPrecache;
}

void CSPDoDRules::SetBloodSprite(short sPrecache)
{
	m_BloodSprite = sPrecache;
}

void CSPDoDRules::SetBloodStream(short sPrecache)
{
	m_BloodStream = sPrecache;
}

void CSPDoDRules::SetBulletTracers(short sPrecache)
{
	m_BulletTracers = sPrecache;
}

void CSPDoDRules::SetSparkShower(short sPrecache)
{
	m_SparkShower = sPrecache;
}

void CSPDoDRules::SetScreenFades(short sPrecache)
{
	m_ScreenFades = sPrecache;
}

void CSPDoDRules::SetBubbleTrails(short sPrecache)
{
	m_BubbleTrails = sPrecache;
}

void CSPDoDRules::SetBubbles(short sPrecache)
{
	m_Bubbles = sPrecache;
}

void CSPDoDRules::SetShards(short sPrecache)
{
	m_Shards = sPrecache;
}

void CSPDoDRules::SetExplosion(short sPrecache)
{
	m_Explosion = sPrecache;
}

int CSPDoDRules::ReturnGameRules(void)
{
	return 0;
}

void CSPDoDRules::SetPopHelmet(short sPrecache)
{
	m_PopHelmet = sPrecache;
}

void CSPDoDRules::SetRoundReset(short sPrecache)
{
	m_RoundReset = sPrecache;
}

void CSPDoDRules::SetRocketTrail(short sPrecache)
{
	m_RocketTrail = sPrecache;
}

void CSPDoDRules::SetMapChangeFromTime(int)
{
}

float CSPDoDRules::GetMaxWaveTime(int)
{
	return 0;
}

float CSPDoDRules::getWaveTime(int)
{
	return 0;
}

void CSPDoDRules::SendTeamScores(int)
{
}

void CSPDoDRules::VoteKick(CBasePlayer *, int, bool)
{
}

void CSPDoDRules::setWaveTime(int, float)
{
}

void CSPDoDRules::setMapTime(void)
{
}

void CSPDoDRules::PlayerDeathThink(CBasePlayer *)
{
}

void CSPDoDRules::PlayerSpawn(CBasePlayer *pPlayer)
{
	CBaseEntity *pEquip = NULL;
	while ((pEquip = UTIL_FindEntityByClassname(pEquip, "game_player_equip")) != NULL)
		pEquip->Touch(pPlayer);
	pPlayer->RemoveAllWeaponFlags();
	pPlayer->m_iClassSpeed = 600;
	pPlayer->SetSpeed(600);
}

BOOL CSPDoDRules::CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon)
{
	int iId = pWeapon->m_iId;
	if (iId == WEAPON_LUGER || iId == WEAPON_KAR || iId == WEAPON_SCOPEDKAR ||
		iId == WEAPON_MP44 || iId == WEAPON_MP40 || iId == WEAPON_STICKGRENADE || iId == WEAPON_MG42)
		return FALSE;
	int iHeldFlags = 0;
	for (int i = 0; i < MAX_ITEM_TYPES; ++i)
	{
		for (CBasePlayerItem *pItem = pPlayer->m_rgpPlayerItems[i]; pItem; pItem = pItem->m_pNext)
		{
			iHeldFlags |= CBasePlayerItem::ItemInfoArray[pItem->m_iId].iFlags;
			if (iId == pItem->m_iId)
				return FALSE;
		}
	}
	return (CBasePlayerItem::ItemInfoArray[iId].iFlags & iHeldFlags & (0x20 | 0x40 | 0x80)) == 0;
}

BOOL CSPDoDRules::FAllowFlashlight(void)
{
	return TRUE;
}

const char *CSPDoDRules::GetTeamID(CBaseEntity *)
{
	return "";
}

float CSPDoDRules::getMapTime(void)
{
	return 0;
}

bool CSPDoDRules::IsRoundFrozen(void)
{
	return false;
}

void CSPDoDRules::SetClanMatch(bool)
{
}

bool CSPDoDRules::IsClanMatch(void)
{
	return false;
}

bool CSPDoDRules::IsClanMatchActive(void)
{
	return false;
}
