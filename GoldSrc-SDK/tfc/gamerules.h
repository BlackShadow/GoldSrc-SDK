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
	virtual void RefreshSkillData( void ); // fill skill data struct with proper values
	virtual void Think( void ) = 0; // GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual BOOL IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;  // Can this item spawn (eg monsters don't spawn in deathmatch).
	virtual BOOL FAllowFlashlight( void ) = 0; // Are players allowed to switch on their flashlight?
	virtual BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0; // should the player switch to this weapon?
	virtual BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon ) = 0; // I can't use this weapon anymore, get me the next best one.

// Functions to verify the single/multiplayer status of a game
	virtual BOOL IsMultiplayer( void ) = 0; // is this a multiplayer game? (either coop or deathmatch)
	virtual BOOL IsDeathmatch( void ) = 0; //is this a deathmatch game?
	virtual BOOL IsTeamplay( void ) { return FALSE; } // is this deathmatch game being played with team rules?
	virtual BOOL IsCoOp( void ) = 0; // is this a coop game?
	virtual const char *GetGameDescription( void ) { return "Half-Life"; }  // this is the game name that gets seen in the server browser
	virtual BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress,
		char szRejectReason[128] ) = 0;
	virtual void InitHUD( CBasePlayer *pPlayer ) = 0;
	virtual void ClientDisconnected( edict_t *pClient ) = 0; // a client just disconnected from the server
	virtual void UpdateGameMode( CBasePlayer *pPlayer ) {}  // the client needs to be informed of the current game mode

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0; // this client just hit the ground after a fall. How much damage?
	virtual BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker ) { return TRUE; } // can this player take damage from this attacker?
	virtual BOOL ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual BOOL FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0; // is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0; // When in the future will this player be able to spawn?
	virtual void GetPlayerSpawnSpot( CBasePlayer *pPlayer );
	virtual BOOL AllowAutoTargetCrosshair( void ) { return TRUE; }
	virtual BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd ) { return FALSE; }
	virtual void ClientUserInfoChanged( CBasePlayer *pPlayer, char *infobuffer ) {}		// the player has changed userinfo;  can change it now

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0; // how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) = 0; // Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor ) = 0; // Call this from within a GameRules class to report an obituary.
// Weapon retrieval
	virtual BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ); // The player is touching an CBasePlayerItem, do I give it to him?
	virtual void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon ) = 0; // Called each time a player picks up a weapon from the ground

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBasePlayerItem *pWeapon ) = 0; // should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBasePlayerItem *pWeapon ) = 0; // when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBasePlayerItem *pWeapon ) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon ) = 0; // where in the world should this weapon respawn?

// Item retrieval
	virtual BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0; // is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0; // call each time a player picks up an item (battery, healthkit, longjump)

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem ) = 0; // Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem ) = 0; // when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0; // where in the world should this item respawn?

// Ammo retrieval
	virtual BOOL CanHaveAmmo( CBasePlayer *pPlayer, const char *pszAmmoName, int iMaxCarry ); // can this player take more of this ammo?
	virtual void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount ) = 0; // called each time a player picks up some ammo in the world

// Ammo spawn/respawn control
	virtual int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo ) = 0; // should this ammo item respawn?
	virtual float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo ) = 0; // when should this ammo item respawn?
	virtual Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo ) = 0; // where in the world should this ammo item respawn?
																			// by default, everything spawns

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void ) = 0; // how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void ) { return 0; } // how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0; // what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0; // Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0; // what team is this entity on?
	virtual int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget ) = 0;
	virtual BOOL PlayTextureSounds( void ) { return TRUE; }
	virtual BOOL PlayFootstepSounds( CBasePlayer *pPlayer, float volume ) { return TRUE; }
	virtual void SendMOTDToClient( edict_t *client ) {}
};


//=========================================================
// CHalfLifeRules - rules for the single player Half-Life 
// game.
//=========================================================
class CHalfLifeRules : public CGameRules
{
public:
	CHalfLifeRules();
	void Think( void );
	BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	BOOL FAllowFlashlight( void ) { return TRUE; }
	BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	BOOL IsMultiplayer( void );
	BOOL IsDeathmatch( void );
	BOOL IsCoOp( void );
	BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] );
	void InitHUD( CBasePlayer *pPlayer );
	void ClientDisconnected( edict_t *pClient );
	float FlPlayerFallDamage( CBasePlayer *pPlayer );
	void PlayerSpawn( CBasePlayer *pPlayer );
	void PlayerThink( CBasePlayer *pPlayer );
	BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	BOOL AllowAutoTargetCrosshair( void );
	int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );
	BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );
	int ItemShouldRespawn( CItem *pItem );
	float FlItemRespawnTime( CItem *pItem );
	Vector VecItemRespawnSpot( CItem *pItem );
	void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );
	int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );
	float FlHealthChargerRechargeTime( void );
	int DeadPlayerWeapons( CBasePlayer *pPlayer );
	int DeadPlayerAmmo( CBasePlayer *pPlayer );
	const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget );
};

//=========================================================
// CHalfLifeMultiplay - rules for the basic half life multiplayer
// competition
//=========================================================
class CHalfLifeMultiplay : public CGameRules
{
public:
	CHalfLifeMultiplay();
	void Think( void );
	BOOL IsAllowedToSpawn( CBaseEntity *pEntity );
	BOOL FAllowFlashlight( void );
	BOOL FShouldSwitchWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	BOOL GetNextBestWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pCurrentWeapon );
	BOOL IsMultiplayer( void );
	BOOL IsDeathmatch( void );
	BOOL IsCoOp( void );
	BOOL ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char szRejectReason[128] );
	void InitHUD( CBasePlayer *pPlayer );
	void ClientDisconnected( edict_t *pClient );
	void UpdateGameMode( CBasePlayer *pPlayer );
	float FlPlayerFallDamage( CBasePlayer *pPlayer );
	BOOL FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker );
	void PlayerSpawn( CBasePlayer *pPlayer );
	void PlayerThink( CBasePlayer *pPlayer );
	BOOL FPlayerCanRespawn( CBasePlayer *pPlayer );
	float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	BOOL AllowAutoTargetCrosshair( void );
	BOOL ClientCommand( CBasePlayer *pPlayer, const char *pcmd );
	int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	void PlayerKilled( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	void DeathNotice( CBasePlayer *pVictim, entvars_t *pKiller, entvars_t *pInflictor );
	void PlayerGotWeapon( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	BOOL CanHavePlayerItem( CBasePlayer *pPlayer, CBasePlayerItem *pWeapon );
	int WeaponShouldRespawn( CBasePlayerItem *pWeapon );
	float FlWeaponRespawnTime( CBasePlayerItem *pWeapon );
	float FlWeaponTryRespawn( CBasePlayerItem *pWeapon );
	Vector VecWeaponRespawnSpot( CBasePlayerItem *pWeapon );
	BOOL CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );
	int ItemShouldRespawn( CItem *pItem );
	float FlItemRespawnTime( CItem *pItem );
	Vector VecItemRespawnSpot( CItem *pItem );
	void PlayerGotAmmo( CBasePlayer *pPlayer, char *szName, int iCount );
	int AmmoShouldRespawn( CBasePlayerAmmo *pAmmo );
	float FlAmmoRespawnTime( CBasePlayerAmmo *pAmmo );
	Vector VecAmmoRespawnSpot( CBasePlayerAmmo *pAmmo );
	float FlHealthChargerRechargeTime( void );
	float FlHEVChargerRechargeTime( void );
	int DeadPlayerWeapons( CBasePlayer *pPlayer );
	int DeadPlayerAmmo( CBasePlayer *pPlayer );
	const char *GetTeamID( CBaseEntity *pEntity ) { return ""; }
	int PlayerRelationship( CBasePlayer *pPlayer, CBaseEntity *pTarget );
	BOOL PlayTextureSounds( void ) { return FALSE; }
	BOOL PlayFootstepSounds( CBasePlayer *pPlayer, float volume );
	void SendMOTDToClient( edict_t *client );

	virtual void GoToIntermission( void );
	virtual void ChangeLevel( void );

protected:
	float m_flIntermissionEndTime;
	float m_flIntermissionStartTime;
	BOOL m_iEndIntermissionButtonHit;
};

extern CGameRules *InstallGameRules( char *pszEntity );
extern CGameRules *g_pGameRules;

#endif
