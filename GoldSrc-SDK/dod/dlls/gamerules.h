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
// GameRules
//=========================================================

//#include "weapons.h"
//#include "items.h"

#ifndef GAMERULES_H
#define GAMERULES_H

class CBaseEntity;
class CBasePlayer;
class CBasePlayerItem;
class CBasePlayerAmmo;
class CItem;

// weapon respawning return codes
enum
{
	GR_NONE = 0,
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,
	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,
	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL
};

class CGameRules
{
public:
	virtual void Init(void) = 0;
	virtual void RefreshSkillData(void);
	virtual void Think(void) = 0; // GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual BOOL IsAllowedToSpawn(CBaseEntity *pEntity) = 0;  // Can this item spawn (eg monsters don't spawn in deathmatch).
	virtual BOOL FAllowFlashlight(void) = 0; // Are players allowed to switch on their flashlight?
	virtual BOOL FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) = 0; // should the player switch to this weapon?
	virtual BOOL GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) = 0; // I can't use this weapon anymore, get me the next best one.

// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer(void) = 0; // is this a multiplayer game? (either coop or deathmatch)
	virtual BOOL IsDeathmatch(void) = 0; //is this a deathmatch game?
	virtual BOOL IsTeamplay(void) { return FALSE; } // is this deathmatch game being played with team rules?
	virtual BOOL IsCoOp(void) = 0; // is this a coop game?
	virtual const char *GetGameDescription(void) { return "Day of Defeat"; }  // this is the game name that gets seen in the server browser

// Client connection/disconnection
	virtual BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128]) = 0; // a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD(CBasePlayer *pPlayer) = 0;		// the client dll is ready for updating
	virtual void ClientDisconnected(edict_t *pClient) = 0; // a client just disconnected from the server
	virtual float FlPlayerFallDamage(CBasePlayer *pPlayer) = 0;
	virtual BOOL FPlayerCanTakeDamage(CBasePlayer *, CBaseEntity *) { return TRUE; }
	virtual BOOL ShouldAutoAim(CBasePlayer *, edict_t *) { return TRUE; }

// Client spawn/respawn control
	virtual void PlayerSpawn(CBasePlayer *pPlayer) = 0; // called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink(CBasePlayer *pPlayer) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual BOOL FPlayerCanRespawn(CBasePlayer *pPlayer) = 0; // is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime(CBasePlayer *pPlayer) = 0; // When in the future will this player be able to spawn?
	virtual edict_t *GetPlayerSpawnSpot(CBasePlayer *pPlayer); // Place this player on their spawnspot and face them the proper direction.
	void GetInitialSpawnSpot(CBasePlayer *pPlayer);
	virtual BOOL AllowAutoTargetCrosshair(void) { return TRUE; }
	virtual BOOL ClientCommand(CBasePlayer *, const char *) { return FALSE; }
	virtual void ClientUserInfoChanged(CBasePlayer *, char *) {}
	virtual void PlayerDeathThink(CBasePlayer *pPlayer) = 0;

// Client kills/scoring
	virtual int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled) = 0; // how many points do I award whoever kills this player?
	virtual void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iWeapon) = 0;
	virtual void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iWeapon) = 0;
// Weapon retrieval
	virtual BOOL CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon); // The player is touching an CBasePlayerItem, do I give it to him?
	virtual void PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon) = 0; // Called each time a player picks up a weapon from the ground

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn(CBasePlayerItem *pWeapon) = 0; // should this weapon respawn?
	virtual float FlWeaponRespawnTime(CBasePlayerItem *pWeapon) = 0; // when may this weapon respawn?
	virtual float FlWeaponTryRespawn(CBasePlayerItem *pWeapon) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot(CBasePlayerItem *pWeapon) = 0; // where in the world should this weapon respawn?

// Item retrieval
	virtual BOOL CanHaveItem(CBasePlayer *pPlayer, CItem *pItem) = 0; // is this player allowed to take this item?
	virtual void PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem) = 0; // call each time a player picks up an item (battery, healthkit, longjump)

// Item spawn/respawn control
	virtual int ItemShouldRespawn(CItem *pItem) = 0; // Should this item respawn?
	virtual float FlItemRespawnTime(CItem *pItem) = 0; // when may this item respawn?
	virtual Vector VecItemRespawnSpot(CItem *pItem) = 0; // where in the world should this item respawn?

// Ammo retrieval
	virtual BOOL CanHaveAmmo(CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry); // can this player take more of this ammo?
	virtual void PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount) = 0; // called each time a player picks up some ammo in the world

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn(CBasePlayerAmmo *pAmmo) = 0; // should this ammo item respawn?
	virtual float FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo) = 0; // when should this ammo item respawn?
	virtual Vector VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo) = 0; // where in the world should this ammo item respawn?
																			// by default, everything spawns

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime(void) = 0; // how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime(void) { return 0; } // how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons(CBasePlayer *pPlayer) = 0; // what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo(CBasePlayer *pPlayer) = 0; // Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID(CBaseEntity *pEntity) = 0; // what team is this entity on?
	virtual int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget) = 0; // What is the player's relationship with this entity?
	virtual int GetTeamIndex(const char *) { return -1; }
	virtual const char *GetIndexedTeamName(int) { return ""; }
	virtual BOOL IsValidTeam(const char *) { return TRUE; }
	virtual void ChangePlayerTeam(CBasePlayer *pPlayer, const char *pTeamName, BOOL bKill, BOOL bGib);
	virtual const char *SetDefaultPlayerTeam(CBasePlayer *) { return ""; }

// Sounds
	virtual BOOL PlayTextureSounds(void) { return TRUE; }
	virtual BOOL PlayFootstepSounds(CBasePlayer *, float) { return TRUE; }

// Monsters
	virtual BOOL FAllowMonsters(void) = 0; //are monsters allowed
	virtual void EndMultiplayerGame(void) {}
	virtual int RoundState(void) = 0;
	virtual void VoteKick(CBasePlayer *, int, bool) {}
	virtual void SetRoundState(int) {}
	virtual int SetRoundState(void);
	virtual void SetRestartTime(float) {}
	virtual void SendTeamScores(int) {}
	virtual float GetMaxWaveTime(int iTeam) = 0;
	virtual float getWaveTime(int iTeam) = 0;
	virtual void setWaveTime(int, float) {}
	virtual int TeamScores(int iTeam) = 0;
	virtual void SetTeamScores(int, int) {}
	virtual void AddToTeamScores(int, int) {}
	virtual void setMapTime(void) {}
	virtual void RestartRound(void) {}
	virtual float getMapTime(void) = 0;
	virtual bool IsRoundFrozen(void) { return false; }
	virtual void SetClanMatch(bool) {}
	virtual bool IsClanMatch(void) { return false; }
	virtual bool IsClanMatchActive(void) { return false; }
	virtual unsigned short Camera(void) = 0;
	virtual unsigned short usVoice(void) = 0;
	virtual unsigned short bodyDamage(void) = 0;
	virtual unsigned short roundRestartSound(void) = 0;
	virtual unsigned short gerVoice(void) = 0;
	virtual unsigned short Pain(void) = 0;
	virtual unsigned short Smoke(void) = 0;
	virtual unsigned short Prone(void) = 0;
	virtual unsigned short BloodSprite(void) = 0;
	virtual unsigned short BloodStream(void) = 0;
	virtual unsigned short BulletTracers(void) = 0;
	virtual unsigned short SparkShower(void) = 0;
	virtual unsigned short ScreenFades(void) = 0;
	virtual unsigned short BubbleTrails(void) = 0;
	virtual unsigned short Bubbles(void) = 0;
	virtual unsigned short Shards(void) = 0;
	virtual unsigned short Explosion(void) = 0;
	virtual unsigned short PopHelmet(void) = 0;
	virtual unsigned short RoundReset(void) = 0;
	virtual unsigned short StartTrail(void) = 0;
	virtual void SetCamera(short) {}
	virtual void SetusVoice(short) {}
	virtual void SetbodyDamage(short) {}
	virtual void SetroundRestartSound(short) {}
	virtual void SetgerVoice(short) {}
	virtual void SetPain(short) {}
	virtual void SetSmoke(short) {}
	virtual void SetProne(short) {}
	virtual void SetBloodSprite(short) {}
	virtual void SetBloodStream(short) {}
	virtual void SetBulletTracers(short) {}
	virtual void SetSparkShower(short) {}
	virtual void SetScreenFades(short) {}
	virtual void SetBubbleTrails(short) {}
	virtual void SetBubbles(short) {}
	virtual void SetShards(short) {}
	virtual void SetExplosion(short) {}
	virtual void SetPopHelmet(short) {}
	virtual void SetRoundReset(short) {}
	virtual void SetRocketTrail(short) {}
	virtual int ReturnGameRules(void) = 0;
	virtual void SetMapChangeFromTime(int) {}
};

class CSPDoDRules : public CGameRules
{
public:
	CSPDoDRules(void);
	void Init(void);
	void Think(void);
	BOOL IsAllowedToSpawn(CBaseEntity *pEntity);
	BOOL FAllowFlashlight(void);
	BOOL FShouldSwitchWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	BOOL GetNextBestWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	BOOL IsMultiplayer(void);
	BOOL IsDeathmatch(void);
	BOOL IsCoOp(void);
	BOOL ClientConnected(edict_t *pEntity, const char *pszName, const char *pszAddress, char *szRejectReason);
	void InitHUD(CBasePlayer *pPlayer);
	void ClientDisconnected(edict_t *pClient);
	float FlPlayerFallDamage(CBasePlayer *pPlayer);
	void PlayerSpawn(CBasePlayer *pPlayer);
	void PlayerThink(CBasePlayer *pPlayer);
	BOOL FPlayerCanRespawn(CBasePlayer *pPlayer);
	float FlPlayerSpawnTime(CBasePlayer *pPlayer);
	BOOL AllowAutoTargetCrosshair(void);
	int IPointsForKill(CBasePlayer *pAttacker, CBasePlayer *pKilled);
	void PlayerKilled(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iWeapon);
	void DeathNotice(CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor, int iWeapon);
	void PlayerGotWeapon(CBasePlayer *pPlayer, CBasePlayerItem *pWeapon);
	int WeaponShouldRespawn(CBasePlayerItem *pWeapon);
	float FlWeaponRespawnTime(CBasePlayerItem *pWeapon);
	float FlWeaponTryRespawn(CBasePlayerItem *pWeapon);
	Vector VecWeaponRespawnSpot(CBasePlayerItem *pWeapon);
	void PlayerDeathThink(CBasePlayer *pPlayer);
	BOOL CanHaveItem(CBasePlayer *pPlayer, CItem *pItem);
	void PlayerGotItem(CBasePlayer *pPlayer, CItem *pItem);
	int ItemShouldRespawn(CItem *pItem);
	float FlItemRespawnTime(CItem *pItem);
	Vector VecItemRespawnSpot(CItem *pItem);
	void PlayerGotAmmo(CBasePlayer *pPlayer, char *szName, int iCount);
	int AmmoShouldRespawn(CBasePlayerAmmo *pAmmo);
	float FlAmmoRespawnTime(CBasePlayerAmmo *pAmmo);
	Vector VecAmmoRespawnSpot(CBasePlayerAmmo *pAmmo);
	float FlHealthChargerRechargeTime(void);
	int DeadPlayerWeapons(CBasePlayer *pPlayer);
	int DeadPlayerAmmo(CBasePlayer *pPlayer);
	BOOL FAllowMonsters(void);
	const char *GetTeamID(CBaseEntity *pEntity);
	int PlayerRelationship(CBaseEntity *pPlayer, CBaseEntity *pTarget);
	int RoundState(void);
	void VoteKick(CBasePlayer *pPlayer, int iID, bool bUserID);
	void SetRoundState(int iState);
	void SetRestartTime(float flTime);
	void SendTeamScores(int iTeam);
	float GetMaxWaveTime(int iTeam);
	float getWaveTime(int iTeam);
	void setWaveTime(int iTeam, float flTime);
	int TeamScores(int iTeam);
	void SetTeamScores(int iTeam, int iScore);
	void AddToTeamScores(int iTeam, int iScore);
	BOOL CanHavePlayerItem(CBasePlayer *pPlayer, CBasePlayerItem *pItem);
	void setMapTime(void);
	float getMapTime(void);
	bool IsRoundFrozen(void);
	void SetClanMatch(bool bMatch);
	bool IsClanMatch(void);
	bool IsClanMatchActive(void);
	unsigned short Camera(void);
	unsigned short usVoice(void);
	unsigned short bodyDamage(void);
	unsigned short roundRestartSound(void);
	unsigned short gerVoice(void);
	unsigned short Pain(void);
	unsigned short Smoke(void);
	unsigned short BloodSprite(void);
	unsigned short BloodStream(void);
	unsigned short BulletTracers(void);
	unsigned short SparkShower(void);
	unsigned short ScreenFades(void);
	unsigned short BubbleTrails(void);
	unsigned short Bubbles(void);
	unsigned short Shards(void);
	unsigned short Explosion(void);
	unsigned short PopHelmet(void);
	unsigned short RoundReset(void);
	unsigned short Prone(void);
	unsigned short StartTrail(void);
	void SetCamera(short sPrecache);
	void SetusVoice(short sPrecache);
	void SetbodyDamage(short sPrecache);
	void SetroundRestartSound(short sPrecache);
	void SetgerVoice(short sPrecache);
	void SetPain(short sPrecache);
	void SetSmoke(short sPrecache);
	void SetProne(short sPrecache);
	void SetBloodSprite(short sPrecache);
	void SetBloodStream(short sPrecache);
	void SetBulletTracers(short sPrecache);
	void SetSparkShower(short sPrecache);
	void SetScreenFades(short sPrecache);
	void SetBubbleTrails(short sPrecache);
	void SetBubbles(short sPrecache);
	void SetShards(short sPrecache);
	void SetExplosion(short sPrecache);
	void SetPopHelmet(short sPrecache);
	void SetRoundReset(short sPrecache);
	void SetRocketTrail(short sPrecache);
	int ReturnGameRules(void);
	void SetMapChangeFromTime(int iTime);

protected:
	unsigned short m_Camera;
	unsigned short m_usVoice;
	unsigned short m_bodyDamage;
	unsigned short m_roundRestartSound;
	unsigned short m_gerVoice;
	unsigned short m_Pain;
	unsigned short m_Smoke;
	unsigned short m_BloodSprite;
	unsigned short m_Prone;
	unsigned short m_BloodStream;
	unsigned short m_BulletTracers;
	unsigned short m_SparkShower;
	unsigned short m_ScreenFades;
	unsigned short m_BubbleTrails;
	unsigned short m_Bubbles;
	unsigned short m_Shards;
	unsigned short m_Explosion;
	unsigned short m_DeadBodies;
	unsigned short m_PopHelmet;
	unsigned short m_RoundReset;
	unsigned short m_RocketTrail;
};

extern CGameRules *g_pGameRules;
extern CGameRules *InstallGameRules(void);

#endif // GAMERULES_H
