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

#ifndef WEAPONS_H
#define WEAPONS_H

#include "weapontype.h"

class CBasePlayer;


// constant items
#define ITEM_HEALTHKIT		1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY			4

#define MAX_WEAPONS			32
#define MAX_AMMO_SLOTS		32
#define MAX_ITEM_TYPES		6
#define WEAPON_SUIT			31	// ?????

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP		-1

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon

#define WPNSTATE_USP_SILENCED		1
#define WPNSTATE_GLOCK18_BURST_MODE	2
#define WPNSTATE_M4A1_SILENCED		4
#define WPNSTATE_ELITE_LEFT		8
#define WPNSTATE_FAMAS_BURST_MODE	16
#define WPNSTATE_SHIELD_DRAWN		32

#define LOUD_GUN_VOLUME		1000
#define NORMAL_GUN_VOLUME	600
#define QUIET_GUN_VOLUME		200
#define BRIGHT_GUN_FLASH		512
#define NORMAL_GUN_FLASH		256
#define DIM_GUN_FLASH		128
#define BIG_EXPLOSION_VOLUME	2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512
#define WEAPON_ACTIVITY_VOLUME	64

#define VECTOR_CONE_1DEGREES	Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES	Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES	Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES	Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES	Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES	Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES	Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES	Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES	Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES	Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES	Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES	Vector( 0.17365, 0.17365, 0.17365 )

enum grenade_e
{
	GRENADE_IDLE = 0,
	GRENADE_PINPULL,
	GRENADE_THROW,
	GRENADE_DRAW,
	GRENADE_SHIELD_IDLE,
	GRENADE_SHIELD_UP,
	GRENADE_SHIELD_DOWN
};

// bullet types
typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe
	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
	BULLET_PLAYER_45ACP,
	BULLET_PLAYER_338MAG,
	BULLET_PLAYER_762MM,
	BULLET_PLAYER_556MM,
	BULLET_PLAYER_50AE,
	BULLET_PLAYER_57MM,
	BULLET_PLAYER_357SIG
} Bullet;

typedef struct
{
	int			iSlot;
	int			iPosition;
	const char*	pszAmmo1;	// ammo 1 type
	int			iMaxAmmo1;		// max ammo 1
	const char*	pszAmmo2;	// ammo 2 type
	int			iMaxAmmo2;		// max ammo 2
	const char*	pszName;
	int			iMaxClip;
	int			iId;
	int			iFlags;
	int			iWeight; // this value used to determine this weapon's importance in autoselection.
} ItemInfo;

typedef struct
{
	const char*	pszName;
	int			iId;
} AmmoInfo;

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	void SetObjectCollisionBox();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer( CBasePlayer* pPlayer );
	virtual int AddDuplicate( CBasePlayerItem* ) { return FALSE; }
	void EXPORT DestroyItem();
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	void EXPORT FallThink();
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn();
	void FallInit();
	void CheckRespawn();
	virtual int GetItemInfo( ItemInfo* ) { return 0; }
	virtual BOOL CanDeploy() { return TRUE; }
	virtual BOOL CanDrop() { return TRUE; }
	virtual BOOL Deploy()								// returns is deploy was successful
	{
#ifdef CLIENT_DLL
		return FALSE;
#else
		return TRUE;
#endif
	}
	virtual BOOL IsWeapon() { return FALSE; }
	virtual BOOL CanHolster() { return TRUE; }
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo() {}
	virtual void ItemPreFrame() {}
	virtual void ItemPostFrame() {}
	virtual void Drop();
	virtual void Kill();
	virtual void AttachToPlayer( CBasePlayer* pPlayer );
	virtual int PrimaryAmmoIndex() { return -1; }
	virtual int SecondaryAmmoIndex() { return -1; }
	virtual int UpdateClientData( CBasePlayer* ) { return 0; }
	virtual CBasePlayerItem* GetWeaponPtr() { return NULL; }
	virtual float GetMaxSpeed() { return 260; }

	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

	CBasePlayer*		m_pPlayer;
	CBasePlayerItem*	m_pNext;
	int				m_iId;												// WEAPON_???

	virtual int iItemSlot() { return 0; }
	int iItemPosition() { return ItemInfoArray[m_iId].iPosition; }
	const char* pszAmmo1() { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1() { return ItemInfoArray[m_iId].iMaxAmmo1; }
	const char* pszAmmo2() { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2() { return ItemInfoArray[m_iId].iMaxAmmo2; }
	const char* pszName() { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip() { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight() { return ItemInfoArray[m_iId].iWeight; }
	int iFlags() { return ItemInfoArray[m_iId].iFlags; }

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};


// inventory items that 
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];
	int AddToPlayer( CBasePlayer* pPlayer );
	int AddDuplicate( CBasePlayerItem* pItem );
	virtual int ExtractAmmo( CBasePlayerWeapon* pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon* pWeapon ); // { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int AddWeapon() { ExtractAmmo( this ); return TRUE; }	// Return TRUE if you want to add yourself to the player

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, char* szName, int iMaxClip, int iMaxCarry );
	BOOL AddSecondaryAmmo( int iCount, char* szName, int iMaxCarry );
	void UpdateItemInfo() {}

	int		m_iPlayEmptySound;
	int		m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the

	virtual BOOL PlayEmptySound();
	virtual void ResetEmptySound();
	virtual void SendWeaponAnim( int iAnim, int skiplocal = 1 );
	BOOL CanDeploy();
	virtual BOOL IsUseable();
	BOOL DefaultDeploy( char* szViewModel, char* szWeaponModel, int iAnim, char* szAnimExt, int skiplocal = 0 );
	BOOL DefaultReload( int iClipSize, int iAnim, float fDelay );
	void ItemPostFrame();
	virtual void PrimaryAttack() {}
	virtual void SecondaryAttack() {}
	virtual void Reload() {}
	virtual void WeaponIdle() {}
	int UpdateClientData( CBasePlayer* pPlayer );
	virtual void RetireWeapon();
	virtual BOOL ShouldWeaponIdle() { return FALSE; }
	void Holster( int skiplocal = 0 );
	virtual BOOL UseDecrement() { return FALSE; }
	BOOL IsWeapon() { return TRUE; }
	int PrimaryAmmoIndex();
	int SecondaryAmmoIndex();
	CBasePlayerItem* GetWeaponPtr() { return this; }

	void FireRemaining( int& shotsFired, float& shootTime, BOOL bIsGlock );
	void KickBack( float up_base, float lateral_base, float up_modifier, float lateral_modifier,
		float up_max, float lateral_max, int direction_change );
	void EjectBrassLate();
	void MakeBeam();
	void BeamUpdate();
	void ReloadSound();
	float GetNextAttackDelay( float delay );
	bool HasSecondaryAttack();
	BOOL IsPistol();
	void SetPlayerShieldAnim();
	void ResetPlayerShieldAnim();
	bool ShieldSecondaryFire( int iUpAnim, int iDownAnim );
	void HandleInfiniteAmmo();

	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;
	int		m_fInSpecialReload;
	int		m_iDefaultAmmo;
	int		m_iShellId;
	float	m_fMaxSpeed;
	bool	m_bDelayFire;
	int		m_iDirection;
	bool	m_bSecondarySilencerOn;
	float	m_flAccuracy;
	float	m_flLastFire;
	int		m_iShotsFired;
	Vector	m_vVecAiming;
	string_t model_name;
	float	m_flGlock18Shoot;
	int		m_iGlock18ShotsFired;
	float	m_flFamasShoot;
	int		m_iFamasShotsFired;
	float	m_fBurstSpread;
	int		m_iWeaponState;
	float	m_flNextReload;
	float	m_flDecreaseShotsFired;
	unsigned short m_usFireGlock18;
	unsigned short m_usFireFamas;
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	void Spawn();
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	virtual BOOL AddAmmo( CBaseEntity* ) { return TRUE; }
	CBaseEntity* Respawn();
	void EXPORT Materialize();
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache();
	void Spawn();
	void Touch( CBaseEntity* pOther );
	void KeyValue( KeyValueData* pkvd );
	BOOL IsEmpty();
	int GiveAmmo( int iCount, char* szName, int iMax, int* pIndex = NULL );
	void SetObjectCollisionBox();

public:
	void EXPORT Kill();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];
	BOOL HasWeapon( CBasePlayerItem* pCheckItem );
	BOOL PackWeapon( CBasePlayerItem* pWeapon );
	BOOL PackAmmo( int iszName, int iCount );
	void EXPORT BombThink();

	CBasePlayerItem* m_rgpPlayerItems[MAX_ITEM_TYPES]; // one slot for each 
	int m_rgiszAmmo[MAX_AMMO_SLOTS]; // ammo names
	int m_rgAmmo[MAX_AMMO_SLOTS]; // ammo quantities
	int m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
	bool m_bIsBomb;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	void Spawn();
	enum SATCHELCODE { SATCHEL_DETONATE = 0, SATCHEL_RELEASE };
	static CGrenade* ShootTimed( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade* ShootTimed2( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity,
		float time, int iTeam, unsigned short usEvent );
	static CGrenade* ShootContact( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade* ShootSmokeGrenade( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity,
		float time, unsigned short usEvent );
	static CGrenade* ShootSatchelCharge( entvars_t* pevOwner, Vector vecStart, Vector vecAngles );
	static void UseSatchelCharges( entvars_t* pevOwner, SATCHELCODE code );
	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void Explode2( TraceResult* pTrace, int bitsDamageType );
	void Explode3( TraceResult* pTrace, int bitsDamageType );
	void SG_Explode( TraceResult* pTrace, int bitsDamageType );
	void EXPORT Smoke();
	void EXPORT Smoke2();
	void EXPORT Smoke3_A();
	void EXPORT Smoke3_B();
	void EXPORT Smoke3_C();
	void EXPORT SG_Smoke();
	void EXPORT BounceTouch( CBaseEntity* pOther );
	void EXPORT SlideTouch( CBaseEntity* pOther );
	void EXPORT C4Touch( CBaseEntity* pOther );
	void EXPORT ExplodeTouch( CBaseEntity* pOther );
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT SG_Detonate();
	void EXPORT Detonate2();
	void EXPORT Detonate3();
	void EXPORT DetonateUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink();
	void EXPORT SG_TumbleThink();
	void EXPORT C4Think();
	virtual void BounceSound();
	int BloodColor() { return DONT_BLEED; }
	void Killed( entvars_t* pevAttacker, int iGib );
	int ObjectCaps() { return m_bIsC4 ? FCAP_CONTINUOUS_USE : 0; }
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	bool	m_bStartDefuse;
	bool	m_bIsC4;
	EHANDLE m_pBombDefuser;
	float	m_flDefuseCountDown;
	float	m_flC4Blow;
	float	m_flNextFreqInterval;
	float	m_flNextBeep;
	float	m_flNextFreq;
	char*	m_sBeepName;
	float	m_fAttenu;
	float	m_flNextBlink;
	float	m_fNextDefuse;
	bool	m_bJustBlew;
	int		m_iTeam;
	int		m_iCurWave;
	edict_t* m_pentCurBombTarget;
	int		m_SGSmoke;
	int		m_angle;
	unsigned short m_usEvent;
	bool	m_bLightSmoke;
	bool	m_bDetonated;
	Vector	m_vSmokeDetonate;
	int		m_iBounceCount;
	BOOL	m_fRegisteredSound; // whether or not this grenade has issued its DANGER sound to the world sound list yet.
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

typedef struct
{
	CBaseEntity* pEntity;
	float		amount;
	int			type;
} MULTIDAMAGE;

class CAK47 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void AK47Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	void SecondaryAttack();
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 221; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAK47;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CAUG : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void AUGFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 240; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireAug;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFamas : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void FamasFire( float flSpread, float flCycleTime, BOOL fUseAutoAim, BOOL bFireBurst );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 240; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CGalil : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void GalilFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	void SecondaryAttack();
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 240; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireGalil;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM4A1 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void M4A1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 230; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM4A1;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSG552 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void SG552Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed();
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireSG552;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void M249Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 220; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireM249;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CMP5N : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void MP5NFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMP5N;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CMAC10 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void MAC10Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireMAC10;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CP90 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void P90Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 245; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireP90;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTMP : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void TMPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireTMP;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CUMP45 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void UMP45Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	int iShellOn;
	unsigned short m_usFireUMP45;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CP228 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void P228Fire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireP228;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CUSP : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void USPFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireUSP;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFiveSeven : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void FiveSevenFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireFiveSeven;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CELITE : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void ELITEFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireELITE_LEFT;
	unsigned short m_usFireELITE_RIGHT;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CGLOCK18 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void GLOCK18Fire( float flSpread, float flCycleTime, BOOL bFireBurst );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	bool m_bBurstFire;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CDEAGLE : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 2; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void DEAGLEFire( float flSpread, float flCycleTime, BOOL fUseSemi );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireDeagle;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CAWP : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void AWPFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed();
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireAWP;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSCOUT : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void SCOUTFire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed();
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireScout;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CG3SG1 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void G3SG1Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed();
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireG3SG1;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSG550 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void SG550Fire( float flSpread, float flCycleTime, BOOL fUseAutoAim );
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	float GetMaxSpeed();
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	unsigned short m_usFireSG550;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CM3 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	BOOL PlayEmptySound();
	float GetMaxSpeed() { return 230; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireM3;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CXM1014 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 1; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	BOOL Deploy();
	void Reload();
	void WeaponIdle();
	BOOL PlayEmptySound();
	float GetMaxSpeed() { return 240; }
	BOOL UseDecrement() { return TRUE; }

private:
	int m_iShell;
	float m_flPumpTime;
	unsigned short m_usFireXM1014;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 3; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	void WeaponAnimation( int iAnimation );
	void EXPORT SwingAgain();
	void EXPORT Smack();
	int Swing( int fFirst );
	int Stab( int fFirst );
	BOOL Deploy();
	BOOL CanDrop() { return FALSE; }
	void Holster( int skiplocal = 0 );
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	BOOL UseDecrement() { return TRUE; }
	void SetPlayerShieldAnim();
	void ResetPlayerShieldAnim();
	bool ShieldSecondaryFire( int iUpAnim, int iDownAnim );

private:
	TraceResult m_trHit;
	unsigned short m_usKnife;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CFlashbang : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 4; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	BOOL Deploy();
	BOOL CanDeploy();
	BOOL CanDrop() { return FALSE; }
	void Holster( int skiplocal = 0 );
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	virtual BOOL IsPistol() { return TRUE; }
	BOOL UseDecrement() { return TRUE; }
	void SetPlayerShieldAnim();
	void ResetPlayerShieldAnim();
	bool ShieldSecondaryFire( int iUpAnim, int iDownAnim );
};

class CHEGrenade : public CBasePlayerWeapon
{
	friend class CBasePlayer;

public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 4; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	BOOL Deploy();
	BOOL CanDeploy();
	BOOL CanDrop() { return FALSE; }
	void Holster( int skiplocal = 0 );
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	BOOL UseDecrement() { return TRUE; }
	void SetPlayerShieldAnim();
	void ResetPlayerShieldAnim();
	bool ShieldSecondaryFire( int iUpAnim, int iDownAnim );

private:
	unsigned short m_usCreateExplosion;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSmokeGrenade : public CBasePlayerWeapon
{
	friend class CBasePlayer;

public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 4; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void SecondaryAttack();
	BOOL Deploy();
	BOOL CanDeploy();
	BOOL CanDrop() { return FALSE; }
	void Holster( int skiplocal = 0 );
	void WeaponIdle();
	float GetMaxSpeed() { return m_fMaxSpeed; }
	BOOL UseDecrement() { return TRUE; }
	void SetPlayerShieldAnim();
	void ResetPlayerShieldAnim();
	bool ShieldSecondaryFire( int iUpAnim, int iDownAnim );

private:
	unsigned short m_usCreateSmoke;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CC4 : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int iItemSlot() { return 5; }
	int GetItemInfo( ItemInfo* p );
	void PrimaryAttack();
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void KeyValue( KeyValueData* pkvd );
	BOOL Deploy();
	void Holster( int skiplocal = 0 );
	void WeaponIdle();
	float GetMaxSpeed() { return 250; }
	BOOL UseDecrement() { return TRUE; }

	bool m_bStartedArming;
	bool m_bBombPlacedAnimation;
	float m_fArmedTime;
	bool m_bHasShield;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

extern MULTIDAMAGE gMultiDamage;
extern short g_sModelIndexLaser;
extern const char* g_pModelNameLaser;
extern short g_sModelIndexLaserDot;
extern short g_sModelIndexFireball;
extern short g_sModelIndexSmoke;
extern short g_sModelIndexWExplosion;
extern short g_sModelIndexBubbles;
extern short g_sModelIndexBloodDrop;
extern short g_sModelIndexBloodSpray;
extern short g_sModelIndexSmokePuff;
extern short g_sModelIndexFireball2;
extern short g_sModelIndexFireball3;
extern short g_sModelIndexFireball4;
extern short g_sModelIndexRadio;
extern short g_sModelIndexCTGhost;
extern short g_sModelIndexTGhost;
extern short g_sModelIndexC4Glow;

BOOL	CanAttack( float attack_time, float curtime, BOOL isPredicted );
void	ClearMultiDamage();
void	ApplyMultiDamage( entvars_t* pevInflictor, entvars_t* pevAttacker );
void	AddMultiDamage( entvars_t* pevInflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType );
void	DecalGunshot( TraceResult* pTrace, int iBulletType, bool ClientOnly, entvars_t* pShooter, bool bHitMetal );
void	SpawnBlood( Vector vecSpot, int bloodColor, float flDamage );
int		DamageDecal( CBaseEntity* pEntity, int bitsDamageType );
void	RadiusDamage( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, float flRadius, int iClassIgnore, int bitsDamageType );
void	RadiusFlash( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int iClassIgnore, int bitsDamageType );
void	EjectBrass( const Vector& vecOrigin, const Vector& vecLeft, const Vector& vecVelocity,
	float rotation, int model, int soundtype, int entityIndex );
void	EjectBrass2( const Vector& vecOrigin, const Vector& vecVelocity, float rotation,
	int model, int soundtype, entvars_t* pev );
int		MaxAmmoCarry( int iszName );
void	W_Precache();

#endif
