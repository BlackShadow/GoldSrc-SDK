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

#pragma once

#include "effects.h"

#define MAX_WEAPONS				32

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP			-1
#define WEAPON_SUIT				31	// ?????
#define WEAPON_ALLWEAPONS		( ~( 1 << WEAPON_SUIT ) )
#define MAX_NORMAL_BATTERY		100
#define WEAPON_CROWBAR			1
#define WEAPON_HANDGRENADE		12


// weapon weight factors (for auto-switching)   (-1 = noswitch)
#define CROWBAR_WEIGHT			0
#define HANDGRENADE_WEIGHT		5
#define HANDGRENADE_DEFAULT_GIVE	5
#define HANDGRENADE_MAX_CARRY		10


// constant items
#define ITEM_HEALTHKIT			1
#define ITEM_ANTIDOTE			2
#define ITEM_SECURITY			3
#define ITEM_BATTERY				4

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define WEAPON_IS_ONTARGET			0x40

#define LOUD_GUN_VOLUME			1000
#define NORMAL_GUN_VOLUME		600
#define QUIET_GUN_VOLUME			200
#define BRIGHT_GUN_FLASH			512
#define NORMAL_GUN_FLASH			256
#define DIM_GUN_FLASH			128
#define BIG_EXPLOSION_VOLUME		2048
#define NORMAL_EXPLOSION_VOLUME	1024
#define SMALL_EXPLOSION_VOLUME	512
#define WEAPON_ACTIVITY_VOLUME	64

#define VECTOR_CONE_1DEGREES		Vector( 0.00873, 0.00873, 0.00873 )
#define VECTOR_CONE_2DEGREES		Vector( 0.01745, 0.01745, 0.01745 )
#define VECTOR_CONE_3DEGREES		Vector( 0.02618, 0.02618, 0.02618 )
#define VECTOR_CONE_4DEGREES		Vector( 0.03490, 0.03490, 0.03490 )
#define VECTOR_CONE_5DEGREES		Vector( 0.04362, 0.04362, 0.04362 )
#define VECTOR_CONE_6DEGREES		Vector( 0.05234, 0.05234, 0.05234 )
#define VECTOR_CONE_7DEGREES		Vector( 0.06105, 0.06105, 0.06105 )
#define VECTOR_CONE_8DEGREES		Vector( 0.06976, 0.06976, 0.06976 )
#define VECTOR_CONE_9DEGREES		Vector( 0.07846, 0.07846, 0.07846 )
#define VECTOR_CONE_10DEGREES		Vector( 0.08716, 0.08716, 0.08716 )
#define VECTOR_CONE_15DEGREES		Vector( 0.13053, 0.13053, 0.13053 )
#define VECTOR_CONE_20DEGREES		Vector( 0.17365, 0.17365, 0.17365 )

// bullet types
typedef enum
{
	BULLET_NONE = 0,
	BULLET_PLAYER_9MM, // glock
	BULLET_PLAYER_MP5, // mp5
	BULLET_PLAYER_357, // python
	BULLET_PLAYER_BUCKSHOT, // shotgun
	BULLET_PLAYER_CROWBAR, // crowbar swipe
	BULLET_PLAYER_TF_BUCKSHOT,
	BULLET_PLAYER_TF_ASSAULT,
	BULLET_MONSTER_9MM,
	BULLET_MONSTER_MP5,
	BULLET_MONSTER_12MM,
} Bullet;

typedef struct
{
	int			iSlot;
	int			iPosition;
	const char*	pszAmmo1;	// ammo 1 type
	int			iAmmo1;
	const char*	pszAmmo2;	// ammo 2 type
	int			iAmmo2;
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

class CBasePlayer;
class CTFFlamethrower;
class CTFGrenadeLauncher;
class CTFIncendiaryC;
class CTFRpg;

// Items that the player has in their inventory that they can use
class CBasePlayerItem : public CBaseAnimating
{
public:
	void SetObjectCollisionBox() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	virtual int AddToPlayer( CBasePlayer* pPlayer );	// return TRUE if the item you want the item added to the player inventory
	virtual int AddDuplicate( CBasePlayerItem* pItem ) { return FALSE; }	// return TRUE if you want your duplicate removed from world
	virtual int iItemSlot() { return 0; }
	virtual int GetItemInfo( ItemInfo* p ) { return 0; }
	virtual BOOL CanDeploy() { return TRUE; }
	virtual BOOL Deploy() { return TRUE; }
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
	virtual int UpdateClientData( CBasePlayer* pPlayer ) { return 0; }
	virtual CBasePlayerItem* GetWeaponPtr() { return NULL; }

	void EXPORT DestroyItem();
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	void EXPORT FallThink();
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn() override;
	void FallInit();
	void CheckRespawn();

	int iItemPosition() { return ItemInfoArray[m_iId].iPosition; }
	const char* pszAmmo1() { return ItemInfoArray[m_iId].pszAmmo1; }
	int iMaxAmmo1() { return ItemInfoArray[m_iId].iAmmo1; }
	const char* pszAmmo2() { return ItemInfoArray[m_iId].pszAmmo2; }
	int iMaxAmmo2() { return ItemInfoArray[m_iId].iAmmo2; }
	const char* pszName() { return ItemInfoArray[m_iId].pszName; }
	int iMaxClip() { return ItemInfoArray[m_iId].iMaxClip; }
	int iWeight() { return ItemInfoArray[m_iId].iWeight; }
	int iFlags() { return ItemInfoArray[m_iId].iFlags; }

	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

	CBasePlayer*		m_pPlayer;
	CBasePlayerItem*	m_pNext;
	int				m_iId;												// WEAPON_???
#ifdef _MSC_VER

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
#pragma warning( suppress : 26495 )
#endif
};


// inventory items that 
class CBasePlayerWeapon : public CBasePlayerItem
{
public:
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	int AddDuplicate( CBasePlayerItem* pItem ) override;
	virtual int ExtractAmmo( CBasePlayerWeapon* pWeapon ); //{ return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int ExtractClipAmmo( CBasePlayerWeapon* pWeapon ); // { return TRUE; };			// Return TRUE if you can add ammo to yourself when picked up
	virtual int AddWeapon() { ExtractAmmo( this ); return TRUE; }
	void UpdateItemInfo() override {}
	virtual BOOL PlayEmptySound();
	virtual void ResetEmptySound();
	virtual void SendWeaponAnim( int iAnim, int skiplocal = 1 );
	BOOL CanDeploy() override;
	virtual BOOL IsUseable();
	void ItemPostFrame() override;
	virtual void PrimaryAttack() {}
	virtual void SecondaryAttack() {}
	virtual void Reload() {}
	virtual void WeaponIdle() {}
	int UpdateClientData( CBasePlayer* pPlayer ) override;
	virtual void RetireWeapon();
	virtual BOOL ShouldWeaponIdle() { return FALSE; }
	void Holster( int skiplocal = 0 ) override;
	int PrimaryAmmoIndex() override;
	int SecondaryAmmoIndex() override;
	CBasePlayerItem* GetWeaponPtr() override { return this; }
	virtual float GetNextAttackDelay( float delay );

	// generic "shared" ammo handlers
	BOOL AddPrimaryAmmo( int iCount, char* szName, int iMaxClip, int iMaxCarry );
	BOOL AddSecondaryAmmo( int iCount, char* szName, int iMaxCarry );
	BOOL DefaultDeploy( char* szViewModel, char* szWeaponModel, int iAnim, char* szAnimExt, int skiplocal = 0 );
	int DefaultReload( int iClipSize, int iAnim, float fDelay );
	void DB_LogShots( int nShots );

	int		m_iPlayEmptySound;
	int		m_fFireOnEmpty;
	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	float	m_flNextReload;
	float	m_flPumpTime;
	float	m_fReloadTime;
	float	m_fAimedDamage;
	float	m_fNextAimBonus;
	int		m_fInZoom;
	int		m_iWeaponState;
	int		m_fInReload;
	int		m_fInSpecialReload;
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_iDefaultAmmo;

	// hle time creep vars
	float	m_flPrevPrimaryAttack;
	float	m_flLastFireTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBasePlayerAmmo : public CBaseEntity
{
public:
	void Spawn() override;
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	virtual BOOL AddAmmo( CBaseEntity* pOther ) { return TRUE; }
	CBaseEntity* Respawn() override;
	void EXPORT Materialize();
};

//=========================================================
// CWeaponBox - a single entity that can store weapons
// and ammo. 
//=========================================================
class CWeaponBox : public CBaseEntity
{
	void Precache() override;
	void Spawn() override;
	void Touch( CBaseEntity* pOther ) override;
	void KeyValue( KeyValueData* pkvd ) override;
	BOOL IsEmpty();
	int GiveAmmo( int iCount, char* szName, int iMax, int* pIndex = NULL ) override;
	void SetObjectCollisionBox() override;
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;

public:
	void EXPORT Kill();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL HasWeapon( CBasePlayerItem* pCheckItem );
	BOOL PackWeapon( CBasePlayerItem* pWeapon );
	BOOL PackAmmo( int iszName, int iCount );

	CBasePlayerItem*	m_rgpPlayerItems[MAX_ITEM_TYPES]; // one slot for each 
	float			m_flTouchableAt;
	float			m_flCreationTime;
	int				m_rgiszAmmo[MAX_AMMO_SLOTS]; // ammo names
	int				m_rgAmmo[MAX_AMMO_SLOTS]; // ammo quantities
	int				m_cAmmoTypes; // how many ammo types packed into this box (if packed by a level designer)
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	CGrenade();
	void Spawn() override;
	int BloodColor() override { return DONT_BLEED; }
	void Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib ) override;
	virtual void Explode( Vector vecSrc, Vector vecAim );
	virtual void Explode( TraceResult* pTrace, int bitsDamageType );
	virtual BOOL setBirthdayModel();
	virtual void setModel();
	virtual void BounceSound();
	BOOL InNoGrenadeZone( Vector origin );
	void EXPORT Smoke();
	void EXPORT BounceTouch( CBaseEntity* pOther );
	void EXPORT SlideTouch( CBaseEntity* pOther );
	void EXPORT ExplodeTouch( CBaseEntity* pOther );
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT DetonateUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink();

	typedef enum { SATCHEL_DETONATE = 0, SATCHEL_RELEASE } SATCHELCODE;
	static CGrenade* ShootTimed( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, float time );
	static CGrenade* ShootContact( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );
	static CGrenade* ShootSatchelCharge( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );
	static void UseSatchelCharges( entvars_t* pevOwner, SATCHELCODE code );

	BOOL	m_bDontPlaySound;
	BOOL	m_bDontSpark;
	BOOL	m_bDontSmoke;
	float	m_flExplosionScale;
	float	m_flSmokeScale;
	BOOL	m_fRegisteredSound;
	unsigned short m_usExplode;
};

class CCrowbar : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 1; }
	void EXPORT SwingAgain();
	void EXPORT Smack();
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	int Swing( int fFirst );
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;

	int			m_iSwing;
	TraceResult	m_trHit;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHandGrenade : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 5; }
	int GetItemInfo( ItemInfo* p ) override;
	BOOL CanHolster() override;
	BOOL Deploy() override;
	void PrimaryAttack() override;
	void WeaponIdle() override;
	void Holster();

	float m_flStartThrow;
	float m_flReleaseThrow;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFAxe : public CCrowbar
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 1; }
	int GetItemInfo( ItemInfo* p ) override;
	BOOL IsUseable() override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void EXPORT Smack();
	virtual BOOL AxeHit( CBaseEntity* pEntity, Vector vecDir, TraceResult* pTrace );
	virtual void PlayAnim( int anim );
	virtual void PlaySound( int sound, float volume ) {}
	virtual void PlayDecal() {}

	int			m_iSwing;
	TraceResult	m_trHit;
	BOOL		m_bHullHit;
	unsigned short m_usAxe;
	unsigned short m_usAxeDecal;
	int			classid;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFSpanner : public CTFAxe
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	BOOL AxeHit( CBaseEntity* pEntity, Vector vecDir, TraceResult* pTrace ) override;
	void PlayAnim( int anim ) override;
};

class CTFKnife : public CTFAxe
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	void PlayAnim( int anim ) override;

	unsigned short m_usKnife;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFMedikit : public CTFAxe
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	BOOL AxeHit( CBaseEntity* pEntity, Vector vecDir, TraceResult* pTrace ) override;
	void PlayAnim( int anim ) override;
	void PlaySound( int sound, float volume ) override;
	void PlayDecal() override;

	unsigned short m_usNormalShot;
	unsigned short m_usSuperShot;
	unsigned short m_usSteamShot;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFAssaultC : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	BOOL ShouldWeaponIdle() override { return TRUE; }
	void Fire();
	void StartSpin();
	void Spin();
	void WindUp();
	void WindDown( bool bFromHolster );

	float	m_flNextAnimTime;
	int		m_iShell;
	unsigned short m_usWindUp;
	unsigned short m_usWindDown;
	unsigned short m_usFire;
	unsigned short m_usStartSpin;
	unsigned short m_usSpin;
	unsigned short m_usACStart;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFAutoRifle : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireAutoRifle;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFNailgun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireNailGun;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFSuperNailgun : public CTFNailgun
{
public:
	void Precache() override;
	int iItemSlot() override { return 4; }
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;

	unsigned short m_usFireSuperNailGun;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFShotgun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void ItemPostFrame() override;

	int		m_iShell;
	int		m_iShellsReloaded;
	int		m_iMaxClipSize;
	unsigned short m_usReloadShotgun;
	unsigned short m_usPumpShotgun;
	unsigned short m_usFireShotgun;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFSuperShotgun : public CTFShotgun
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;

	unsigned short m_usFireSuperShotgun;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CLaserSpot;

class CTFSniperRifle : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	void ItemPostFrame() override;
	void UpdateSpot();

	CLaserSpot*		m_pSpot;
	int				m_iSpotActive;
	unsigned short	m_usFireSniper;
	unsigned short	m_usSniperHit;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFRailgun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireRail;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFTranq : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	BOOL Deploy() override;
	void PrimaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireTranquilizer;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFFlamethrower : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 5; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	BOOL CanHolster() override { return TRUE; }
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireFlame;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFGrenadeLauncher : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	int		m_iAnim_Deploy;
	int		m_iAnim_Holster;
	int		m_iAnim_Idle;
	int		m_iAnim_ReloadDown;
	int		m_iAnim_ReloadUp;
	unsigned short m_usFireGL;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFPipebombLauncher : public CTFGrenadeLauncher
{
public:
	void Spawn() override;
	int iItemSlot() override { return 3; }
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
};

class CTFIncendiaryC : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 4; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	BOOL CanHolster() override { return TRUE; }
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireIC;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFRpg : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 5; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	BOOL Deploy() override;
	BOOL CanHolster() override { return TRUE; }
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireRPG;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFNailgunNail : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT NailTouch( CBaseEntity* pOther );
	void EXPORT RailgunNailTouch( CBaseEntity* pOther );
	void EXPORT TranqTouch( CBaseEntity* pOther );
	void EXPORT RailgunNail_Think();
	static CTFNailgunNail* CreateNail( BOOL sendClientNail, Vector origin, Vector angles, CBaseEntity* owner,
		CBaseEntity* launcher, BOOL makeClientNail );
	static CTFNailgunNail* CreateSuperNail( Vector origin, Vector angles, CBaseEntity* owner, CBaseEntity* launcher );
	static CTFNailgunNail* CreateNailGrenNail( Vector origin, Vector angles, CBaseEntity* owner, CBaseEntity* launcher );
	static CTFNailgunNail* CreateTranqNail( Vector origin, Vector angles, CBaseEntity* owner, CBaseEntity* launcher );
	static CTFNailgunNail* CreateRailgunNail( Vector origin, Vector angles, CBaseEntity* owner, CBaseEntity* launcher );

	Vector m_vecPreviousVelocity;
};

class CTFFlamethrowerBurst : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT BurstTouch( CBaseEntity* pOther );
	static CTFFlamethrowerBurst* CreateBurst( Vector origin, Vector angles, CBaseEntity* owner,
		CTFFlamethrower* launcher );

	int m_iTrail;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFRpgRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch( CBaseEntity* pOther );
	void RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float damage, int classIgnore,
		int damageType ) override;
	static CTFRpgRocket* CreateRpgRocket( Vector origin, Vector angles, CBaseEntity* owner, CTFRpg* launcher );

	int m_iTrail;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFIncendiaryCRocket : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch( CBaseEntity* pOther );
	void RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float damage, int classIgnore,
		int damageType ) override;
	static CTFIncendiaryCRocket* CreateRpgRocket( Vector origin, Vector angles, CBaseEntity* owner,
		CTFIncendiaryC* launcher );

	int m_iTrail;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFGrenade : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void setModel() override;
	void EXPORT GrenadeTouch( CBaseEntity* pOther );
	void EXPORT PipebombDetonate();
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;
	void RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float damage, int classIgnore,
		int damageType ) override;
	static CTFGrenade* CreateTFGrenade( Vector origin, Vector angles, CBaseEntity* owner,
		CTFGrenadeLauncher* launcher );
	static CTFGrenade* CreateTFPipebomb( Vector origin, Vector angles, CBaseEntity* owner,
		CTFGrenadeLauncher* launcher );

	int		m_iTrail;
	float	m_flCreationTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDetpack : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_MACHINE; }
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;
	void EXPORT DetpackTouch( CBaseEntity* pOther );
	void Remove();
	void DetpackExplode();
	void EXPORT DetpackThink();

	BOOL		bTicked;
	BOOL		bAlternateLight;
	CSprite*	m_pBlinkSprite;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CLaserSpot : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int ObjectCaps() override { return FCAP_DONT_SAVE; }
	void EXPORT Revive();
	void Suspend( float suspendTime );
	static CLaserSpot* CreateSpot();
};

class CTFRpgAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL AddAmmo( CBaseEntity* pOther ) override;
};

enum TFTURRET_ANIM
{
	TFTURRET_ANIM_NONE = 0,
	TFTURRET_ANIM_FIRE,
	TFTURRET_ANIM_SPIN,
};

class CTFDispenser : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_MACHINE; }
	int BloodColor() override { return DONT_BLEED; }
	void EXPORT Dispenser_Explode();
	void Finished();
	void EXPORT DispenserThink();
	void EXPORT DispenserTouch( CBaseEntity* pOther );
	void Remove();
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;
	BOOL EngineerUse( CBasePlayer* player );
	BOOL RefillPlayer( CBasePlayer* player );
	void Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib ) override;
	static CTFDispenser* CreateDispenser( Vector origin, Vector angles, CBaseEntity* owner );

	int		m_iShardIndex;
	float	m_flNextUseTime;
	float	m_flInitialUseDelay;
	float	m_flNextRefillTime;
	int		m_iDestroyed;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFSentrygunBase : public CBaseEntity
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_MACHINE; }
	void Finished();
	void Remove();
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float damage, int damageType ) override;
	static CTFSentrygunBase* CreateSentryBase( Vector origin, Vector angles, CBaseEntity* owner );
};

class CTFSentrygun : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_MACHINE; }
	int BloodColor() override { return DONT_BLEED; }
	void EXPORT Sentry_Explode();
	void Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib ) override;
	void Remove();
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;
	void SetSentryAnim( TFTURRET_ANIM anim );
	void CheckShield();
	void FoundTarget();
	void Upgrade();
	void EXPORT SentryRotate();
	void EXPORT Attack();
	BOOL FInViewCone( CBaseEntity* pOther ) override;
	BOOL FindTarget();
	BOOL ValidTarget( CBaseEntity* target );
	BOOL Fire();
	BOOL CheckSentry();
	BOOL EngineerUse( CBasePlayer* player );
	int Range( CBaseEntity* target );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float damage, int damageType ) override;
	int MoveTurret();

	BOOL	m_bTurningRight;
	int		m_iShardIndex;
	float	m_flNextCheck;
	int		m_iBaseTurnRate;
	float	m_fTurnRate;
	Vector	m_vecCurAngles;
	Vector	m_vecGoalAngles;
	Vector	m_vecCurDishAngles;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFTeleporter : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_MACHINE; }
	int BloodColor() override { return DONT_BLEED; }
	void EXPORT Teleporter_Explode();
	void Finished();
	void EXPORT TeleporterTouch( CBaseEntity* pOther );
	void TeamFortress_TakeEMPBlast( entvars_t* pevAttacker ) override;
	void Remove();
	void TeleporterSend( CBasePlayer* player );
	void TeleporterReceive( CBasePlayer* player, float delay );
	void SetTeleporterRings( int state );
	void SetTeleporterParticles( int state );
	void TeleporterKilled();
	void Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib ) override;
	void TeleporterFadePlayer( int direction );
	void TeleporterProcessFade();
	void EXPORT TeleporterThink();
	BOOL EngineerUse( CBasePlayer* player );
	BOOL TeleportersReady();
	bool PlayerIsStandingOnTeleporter( CBaseEntity* player );
	float GetDamageMultiplier();
	const Vector& GetTeamColor();
	CTFTeleporter* FindMatch();
	static CTFTeleporter* CreateTeleporter( Vector origin, Vector angles, CBaseEntity* owner, int type );

	CBasePlayer*	m_pPlayer;
	float		m_flInitialUseDelay;
	int			m_iType;
	int			m_iState;
	int			m_iDestroyed;
	int			m_iShardIndex;
	float		m_flMyNextThink;
	float		m_flDamageDelay;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

typedef struct
{
	CBaseEntity*	pEntity;
	float			amount;
	int				type;
} MULTIDAMAGE;

extern MULTIDAMAGE	gMultiDamage;
extern int			gmsgWeapPickup;
extern short		g_sModelIndexLaser;
extern const char*	g_pModelNameLaser;
extern short		g_sModelIndexLaserDot;
extern short		g_sModelIndexFireball;
extern short		g_sModelIndexSmoke;
extern short		g_sModelIndexWExplosion;
extern short		g_sModelIndexBubbles;
extern short		g_sModelIndexBloodDrop;
extern short		g_sModelIndexBloodSpray;
extern short		g_sModelIndexNail;
extern short		g_sModelIndexNapalm;
extern short		g_sModelIndexPlayerFlame;
extern short		g_sModelIndexSaveMe;
extern short		g_sModelIndexGrenade;
extern short		g_sModelIndexMirvGrenade;
extern short		g_sModelIndexNapalmGrenade;
extern short		g_sModelIndexNailGrenade;
extern short		sModelIndexFlameThrowerFlame;

void	FindHullIntersection( const Vector& vecSrc, TraceResult& tr, float* mins, float* maxs, edict_t* ignore );

void	ClearMultiDamage();
void	ApplyMultiDamage( entvars_t* pevInflictor, entvars_t* pevAttacker );
void	AddMultiDamage( entvars_t* pevInflictor, CBaseEntity* pEntity, float flDamage, int bitsDamageType );
void	SpawnBlood( Vector vecSpot, int bloodColor, float flDamage );
void	RadiusDamage( Vector vecSrc, entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage,
		float flRadius, int iClassIgnore, int bitsDamageType );

void	DecalGunshot( TraceResult* pTrace, int iBulletType );
int		DamageDecal( CBaseEntity* pEntity, int bitsDamageType );
void	AddAmmoNameToAmmoRegistry( const char* name );

#ifdef CLIENT_DLL
bool	bIsMultiplayer();
void	LoadVModel( char* szViewModel, CBasePlayer* m_pPlayer );
#endif
