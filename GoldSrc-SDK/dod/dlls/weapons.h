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

#pragma once

#include "effects.h"
#include "../cl_dll/dod_shared.h"

#define MAX_WEAPONS			64

// the maximum amount of ammo each weapon's clip can hold
#define WEAPON_NOCLIP		-1
#define WEAPON_SUIT			31	// ?????
#define WEAPON_ALLWEAPONS	( ~( 1 << WEAPON_SUIT ) )
#define MAX_NORMAL_BATTERY	100


// constant items
#define ITEM_HEALTHKIT		1
#define ITEM_ANTIDOTE		2
#define ITEM_SECURITY		3
#define ITEM_BATTERY			4

#define ITEM_FLAG_SELECTONEMPTY		1
#define ITEM_FLAG_NOAUTORELOAD		2
#define ITEM_FLAG_NOAUTOSWITCHEMPTY	4
#define ITEM_FLAG_LIMITINWORLD		8
#define ITEM_FLAG_EXHAUSTIBLE		16 // A player can totally exhaust their ammo supply and lose this weapon
#define ITEM_FLAG_NOAUTOSWITCHTO		32
#define ITEM_FLAG_PRIMARYWEAPON		128
#define WEAPON_IS_ONTARGET			0x40

#define WEAPON_FLAG_OVERHEAT		2048
#define WEAPON_CLASS_MACHINEGUN	256

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

typedef enum
{
	G_MILLSGRENADE = 0,
	G_HANDGRENADE,
	G_STICKGRENADE,
} grenType;

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
	int			iBulletId;
	float		flSpread;
	float		flRecoil;
} ItemInfo;

typedef struct
{
	const char*	pszName;
	int			iId;
} AmmoInfo;

class CBasePlayer;

// Contact Grenade / Timed grenade / Satchel Charge
class CGrenade : public CBaseMonster
{
public:
	typedef enum
	{
		SATCHEL_DETONATE = 0,
		SATCHEL_RELEASE,
	} SATCHELCODE;

	void Spawn() override;
	static CGrenade* ShootTimed( entvars_t* pevOwner, Vector start, Vector velocity, float time );
	static CGrenade* ShootPickup( entvars_t* pevOwner, Vector start, Vector velocity, float time,
		grenType type, float damageTime );
	static CGrenade* ShootContact( entvars_t* pevOwner, Vector start, Vector velocity );
	static void UseSatchelCharges( entvars_t* pevOwner, SATCHELCODE code );
	void Explode( Vector vecSrc, Vector vecAim );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void EXPORT Smoke();
	void EXPORT BounceTouch( CBaseEntity* pOther );
	void EXPORT SlideTouch( CBaseEntity* pOther );
	void EXPORT ExplodeTouch( CBaseEntity* pOther );
	void EXPORT RocketExplodeTouch( CBaseEntity* pOther );
	void MortarShellExplodeTouch( CBaseEntity* pOther );
	void EXPORT DangerSoundThink();
	void EXPORT PreDetonate();
	void EXPORT Detonate();
	void EXPORT DetonateUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT PickupUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT TumbleThink();
	void RemoveMe();
	void Precache() override;
	virtual void BounceSound();
	int BloodColor() override { return DONT_BLEED; }
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	int ObjectCaps() override { return CBaseEntity :: ObjectCaps() | FCAP_IMPULSE_USE; }

	grenType	g_GrenType;
	int			m_flClipTime;
	int			m_iOldMoveType;
	int			m_iBodyGibs;
	BOOL		m_fRegisteredSound; // whether or not this grenade has issued its DANGER sound to the world sound list yet.
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

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
	void EXPORT DestroyItem();
	void EXPORT DefaultTouch( CBaseEntity* pOther );
	void EXPORT PickupUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT FallThink();
	void EXPORT Materialize();
	void EXPORT AttemptToMaterialize();
	CBaseEntity* Respawn() override;
	void FallInit();
	void CheckRespawn();
	virtual int GetItemInfo( ItemInfo* p ) { return 0; }
	virtual BOOL CanDeploy() { return TRUE; }
	virtual BOOL Deploy() { return TRUE; }
	virtual BOOL SpawnDeploy() { return TRUE; }
	int ObjectCaps() override { return CBaseEntity :: ObjectCaps() | FCAP_IMPULSE_USE; }
	virtual BOOL CanDrop() { return CanHolster(); }
	virtual void SetDmgTime( float time ) {}
	virtual void DropGren() {}
	virtual BOOL CanHolster() { return TRUE; }
	virtual void Holster( int skiplocal = 0 );
	virtual void UpdateItemInfo() {}
	virtual void ItemPreFrame() {}
	virtual void ItemPostFrame() {}
	virtual void Drop();
	virtual void Kill();
	void SpawnReplacement();
	virtual void AttachToPlayer( CBasePlayer* pPlayer );
	virtual int PrimaryAmmoIndex() { return -1; }
	virtual int SecondaryAmmoIndex() { return -1; }
	virtual int UpdateClientData( CBasePlayer* pPlayer ) { return 0; }
	virtual CBasePlayerItem* GetWeaponPtr() { return NULL; }
	void RoundRespawn() override;
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
	static ItemInfo ItemInfoArray[MAX_WEAPONS];
	static AmmoInfo AmmoInfoArray[MAX_AMMO_SLOTS];

	CBasePlayer*		m_pPlayer;
	CBasePlayerItem*	m_pNext;
	int				m_iId;												// WEAPON_???
	BOOL			m_bReadyToRespawn;
	Vector			m_vRespawnOrigin;
	Vector			m_vRespawnAngles;
	BOOL			m_bTriggerSpawnOnDie;

	// int		m_iIdPrimary;										// Unique Id for primary ammo
	// int		m_iIdSecondary;										// Unique Id for secondary ammo
#ifdef _MSC_VER
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
	BOOL AddPrimaryAmmo( int count, char* name, int maxClip, int maxCarry );
	BOOL AddSecondaryAmmo( int count, char* name, int maxCarry );
	void UpdateItemInfo() override {}
	virtual BOOL PlayEmptySound();
	virtual void ResetEmptySound();
	virtual void SendWeaponAnim( int anim, int skiplocal = 1 );
	void PostMortarValue( float value );
	void SendMortarFireCommand( char* command );
	BOOL CanDeploy() override;
	virtual BOOL IsUseable();
	BOOL DefaultDeploy( char* viewModel, char* weaponModel, int anim, char* animExt, char* animExt2,
		int skiplocal = 0 );
	BOOL DefaultReload( int clipSize, int anim, float delay );
	void ItemPostFrame() override;
	virtual Vector Aim( float recoil, CBasePlayer* player, int bulletType );
	virtual float flAim( float spread, CBasePlayer* player );
	virtual void RemoveStamina( float stamina, CBasePlayer* player );
	virtual int ChangeFOV( int fov );
	virtual int ZoomOut();
	virtual int ZoomIn();
	virtual int GetFOV();
	virtual bool PlayerIsWaterSniping();
	void EXPORT ThinkZoomOut();
	void EXPORT ThinkZoomIn();
	virtual void UpdateZoomSpeed();
	virtual void PrimaryAttack() {}
	virtual void SecondaryAttack() {}
	virtual void Reload() {}
	virtual void Special() {}
	virtual void WeaponIdle() {}
	int UpdateClientData( CBasePlayer* pPlayer ) override;
	virtual void RetireWeapon();
	virtual BOOL ShouldWeaponIdle() { return TRUE; }
	void Holster( int skiplocal = 0 ) override;
	virtual BOOL UseDecrement() { return FALSE; }
	int PrimaryAmmoIndex() override;
	int SecondaryAmmoIndex() override;
	void PrintState();
	CBasePlayerItem* GetWeaponPtr() override { return this; }
	BOOL TimedDeploy( char* viewModel, char* weaponModel, int anim, char* animExt, char* animExt2,
		float deployTime, float idleTime, int skiplocal = 0 );
	int Classify() override;
	int GetRoundState();

	int		m_iPlayEmptySound;
	int		m_fFireOnEmpty;		// True when the gun is empty and the player is still holding down the
	int		m_fInSpecialReload;
	float	m_flNextPrimaryAttack;								// soonest time ItemPostFrame will call PrimaryAttack
	float	m_flNextSecondaryAttack;							// soonest time ItemPostFrame will call SecondaryAttack
	float	m_flTimeWeaponIdle;									// soonest time ItemPostFrame will call WeaponIdle
	int		m_iPrimaryAmmoType;									// "primary" ammo index into players m_rgAmmo[]
	int		m_iSecondaryAmmoType;								// "secondary" ammo index into players m_rgAmmo[]
	int		m_iClip;											// number of shots left in the primary weapon clip, -1 it not used
	int		m_iClientClip;										// the last version of m_iClip sent to hud dll
	int		m_iClientWeaponState;								// the last version of the weapon state sent to hud dll (is current weapon, is on target)
	int		m_fInReload;										// Are we in the middle of a reload;
	int		m_iDefaultAmmo; // how much ammo you get when you pick up this weapon as placed by a level designer.
	int		m_fInAttack;
	int*	current_ammo;
	int		m_iWeaponState;
	float	m_flWeaponHeat;
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
	static bool GiveGenericAmmo( CBaseEntity* pOther );
};

class CDodAmmoBox : public CBaseEntity
{
	void Precache() override;
	void Spawn() override;
	void Touch( CBaseEntity* pOther ) override;

public:
	void SetOwner( CBaseEntity* pOwner );
	void EXPORT KillThink();

	EHANDLE m_hOwner;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CWeaponBox : public CBaseEntity
{
	void Precache() override;
	void Spawn() override;
	void Touch( CBaseEntity* pOther ) override;
	void KeyValue( KeyValueData* pkvd ) override;
	BOOL IsEmpty();
	int GiveAmmo( int count, char* name, int maxCarry, int* index = NULL );
	void SetObjectCollisionBox() override;

public:
	void EXPORT Kill();
	void EXPORT ActivateThink();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	BOOL HasWeapon( CBasePlayerItem* pItem );
	BOOL PackWeapon( CBasePlayerItem* pWeapon );
	BOOL PackAmmo( int name, int count );
	void RoundRespawn() override;

	CBasePlayerItem*	m_rgpPlayerItems[MAX_ITEM_TYPES];
	int				m_rgiszAmmo[MAX_AMMO_SLOTS];
	int				m_rgAmmo[MAX_AMMO_SLOTS];
	int				m_cAmmoTypes;
	BOOL			m_bDontTouch;
	BOOL			m_bAmmoBox;
	BOOL			m_bMGAmmoBoxTeam;
	BOOL			m_bTouch;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CMeleeWeapon : public CBasePlayerWeapon
{
public:
	void Spawn( int weaponId );
	void Precache() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override {}
	BOOL Deploy() override;
	void WeaponIdle() override;
	void EXPORT SwingAgain();
	void EXPORT Smack();
	int Swing( int first );
	int Stab( int first );
	void Holster( int skiplocal = 0 ) override;
	int iItemSlot() override { return 0; }
	BOOL UseDecrement() override { return TRUE; }
	int Classify() override { return 1; }
	BOOL CanDrop() override { return FALSE; }
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	virtual int GetIdleAnim() { return 0; }
	virtual int GetDrawAnim() { return 0; }
	virtual int GetSlashAnim( int swing ) { return 0; }

	int			m_iSwing;
	TraceResult	m_trHit;
	int			m_iFireEvent;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPistol : public CBasePlayerWeapon
{
public:
	void Spawn( int weaponId );
	void PrimaryAttack() override;
	void SecondaryAttack() override {}
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int iItemSlot() override { return 1; }
	BOOL UseDecrement() override { return TRUE; }
	int Classify() override { return 8; }
	BOOL CanDrop() override { return FALSE; }
	virtual int GetIdleAnim() { return 0; }
	virtual int GetDrawAnim() { return 0; }
	virtual int GetReloadAnim() { return 0; }

	int m_iFireEvent;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBipodWeapon : public CBasePlayerWeapon
{
public:
	void Spawn( int weaponId );
	void Precache() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void Holster( int skiplocal = 0 ) override;
	void ForceUndeploy();
	int iItemSlot() override { return 2; }
	BOOL UseDecrement() override { return TRUE; }
	BOOL CanDrop() override { return CanHolster(); }
	virtual int GetIdleAnim() { return 0; }
	virtual int GetDrawAnim() { return 0; }
	virtual int GetReloadAnim() { return 0; }
	virtual int GetUpToDownAnim() { return 0; }
	virtual int GetDownToUpAnim() { return 0; }
	float GetBipodSpread();
	bool IsDeployed();
	void EXPORT CoolThink();

	int m_iFireEvent;
	int m_iOverheatEvent;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CRocketWeapon : public CBasePlayerWeapon
{
public:
	int iItemSlot() override { return 2; }
	BOOL UseDecrement() override { return TRUE; }
	int Classify() override { return 512; }
	virtual void ReSlow() {}
};

class CDoDGrenade : public CBasePlayerWeapon
{
public:
	int iItemSlot() override { return 4; }
	BOOL UseDecrement() override { return TRUE; }
	int Classify() override { return 2; }
	BOOL CanDrop() override { return FALSE; }
	BOOL ShouldWeaponIdle() override { return FALSE; }

	float m_flStartThrow;
	float m_flReleaseThrow;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDoDGrenadeEx : public CDoDGrenade
{
public:
	int Classify() override { return 4; }
};

class CHandGrenade : public CDoDGrenade
{
public:
	void Spawn() override;
	int GetItemInfo( ItemInfo* p ) override;
	void Precache() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void StartThrow( BOOL underhand );
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	void DropGren() override;
	BOOL CanDeploy() override;

	Vector	angThrow;
	Vector	vecSrc;
	Vector	vecThrow;
	float	flVel;
	BOOL	m_bUnderhand;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CHandGrenadeEx : public CDoDGrenadeEx
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void StartThrow( BOOL underhand );
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	BOOL CanDeploy() override;
	void DropGren() override;
	void SetDmgTime( float time ) override;

	float	m_flTimeToExplode;
	BOOL	m_bUnderhand;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CStickGrenade : public CDoDGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void StartThrow( BOOL underhand );
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	void DropGren() override;
	BOOL CanDeploy() override;

	Vector	angThrow;
	Vector	vecSrc;
	Vector	vecThrow;
	float	flVel;
	float	m_flLeaveHand;
	BOOL	m_bUnderhand;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CStickGrenadeEx : public CDoDGrenadeEx
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void StartThrow( BOOL underhand );
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	void DropGren() override;
	void SetDmgTime( float time ) override;
	BOOL CanDeploy() override;

	float	m_flTimeToExplode;
	BOOL	m_bUnderhand;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CAmerKnife : public CMeleeWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int GetSlashAnim( int swing ) override;
	int GetDrawAnim() override;
	int GetIdleAnim() override;
};

class CGerKnife : public CMeleeWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int GetSlashAnim( int swing ) override;
	int GetDrawAnim() override;
	int GetIdleAnim() override;
};

class CSpade : public CMeleeWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int GetSlashAnim( int swing ) override;
	int GetDrawAnim() override;
	int GetIdleAnim() override;
};

class CCOLT : public CPistol
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int GetReloadAnim() override;
	int GetDrawAnim() override;
	int GetIdleAnim() override;
};

class CLUGER : public CPistol
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireLuger;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CWEBLEY : public CPistol
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;

	unsigned short m_usFireWebley;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CM1Carbine : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL SpawnDeploy() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireM1Carbine;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CGarand : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireGarand;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CScopedKar : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void EXPORT ThinkZoomOutIn();
	void Reload() override;
	void Holster( int skiplocal = 0 ) override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireScopedKar;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CThompson : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireThompson;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CMP44 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireMP44;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CSPRING : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void EXPORT ThinkZoomOutIn();
	void EXPORT ThinkZoomOut();
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireSpring;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CKAR : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireKar;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBAR : public CBipodWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int Classify() override;
	int GetIdleAnim() override;
	int GetDrawAnim() override;
	int GetReloadAnim() override;
	int GetUpToDownAnim() override;
	int GetDownToUpAnim() override;

	unsigned short m_usFireBAR;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CMP40 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireMP40;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CMG42 : public CBipodWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int Classify() override;
	int GetIdleAnim() override;
	int GetDrawAnim() override;
	int GetReloadAnim() override;
	int GetUpToDownAnim() override;
	int GetDownToUpAnim() override;

	unsigned short m_usFireMG42;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CMG34 : public CBipodWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int Classify() override;
	int GetIdleAnim() override;
	int GetDrawAnim() override;
	int GetReloadAnim() override;
	int GetUpToDownAnim() override;
	int GetDownToUpAnim() override;

	unsigned short m_usFireMG34;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class C30CAL : public CBipodWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int Classify() override;
	int GetIdleAnim() override;
	int GetDrawAnim() override;
	int GetReloadAnim() override;
	int GetUpToDownAnim() override;
	int GetDownToUpAnim() override;

	unsigned short m_usFire30CAL;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CGreaseGun : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	BOOL Deploy() override;
	BOOL SpawnDeploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireGreaseGun;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CFG42 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void Holster( int skiplocal = 0 ) override;
	BOOL CanHolster() override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }
	BOOL IsDeployed();

	unsigned short m_usFireFG42;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CK43 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireK43;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CENFIELD : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void EXPORT ThinkZoomOutIn();
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireEnfield;
	unsigned short m_usFireScopedEnfield;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CSTEN : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int AddToPlayer( CBasePlayer* pPlayer ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Reload() override;
	void WeaponIdle() override;
	int Classify() override;
	BOOL UseDecrement() override { return TRUE; }

	unsigned short m_usFireSten;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBREN : public CBipodWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int iItemSlot() override { return 2; }
	int GetItemInfo( ItemInfo* p ) override;
	int Classify() override;
	int GetIdleAnim() override;
	int GetDrawAnim() override;
	int GetReloadAnim() override;
	int GetUpToDownAnim() override;
	int GetDownToUpAnim() override;

	unsigned short m_usFireBren;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBazooka : public CRocketWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	void Raise();
	void Lower();
	void UnSlow();
	void ReSlow() override;

	unsigned short m_usFireBazooka;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPschreck : public CRocketWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	void Raise();
	void Lower();
	void UnSlow();
	void ReSlow() override;

	unsigned short m_usFirePschreck;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CPIAT : public CRocketWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* p ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	BOOL Deploy() override;
	BOOL CanHolster() override;
	void Holster( int skiplocal = 0 ) override;
	void Reload() override;
	void WeaponIdle() override;
	void Raise();
	void Lower();
	void UnSlow();
	void ReSlow() override;

	unsigned short m_usFirePIAT;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CBazookaShell : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch( CBaseEntity* pOther );
	static CBazookaShell* CreateRocket( Vector origin, Vector angles, CBaseEntity* pOwner );
};

class CPschreckShell : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch( CBaseEntity* pOther );
	static CPschreckShell* CreateRocket( Vector origin, Vector angles, CBaseEntity* pOwner );
};

class CPIATShell : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RocketTouch( CBaseEntity* pOther );
	static CPIATShell* CreateRocket( Vector origin, Vector angles, CBaseEntity* pOwner );
};

class GenericAmmoAmerican : public CBasePlayerAmmo
{
	void Spawn() override
	{
		Precache( );
		SET_MODEL( ENT(pev), "models/allied_ammo.mdl" );
		CBasePlayerAmmo::Spawn( );
		pev->team = 1;
	}

	BOOL AddAmmo( CBaseEntity* pOther ) override
	{
		BOOL bResult = GiveGenericAmmo( pOther );

		if ( bResult )
		{
			EMIT_SOUND( ENT(pev), CHAN_ITEM, "items/ammopickup.wav", 1, ATTN_NORM );
		}

		return bResult;
	}

protected:
	void Precache() override
	{
		PRECACHE_MODEL( "models/allied_ammo.mdl" );
		PRECACHE_MODEL( "models/axis_ammo.mdl" );
		PRECACHE_SOUND( "items/ammopickup.wav" );
	}
};

class GenericAmmoGerman : public GenericAmmoAmerican
{
	void Spawn() override
	{
		Precache( );
		SET_MODEL( ENT(pev), "models/axis_ammo.mdl" );
		CBasePlayerAmmo::Spawn( );
		pev->team = 2;
	}
};

class GenericAmmoBritish : public GenericAmmoAmerican
{
	void Spawn() override
	{
		Precache( );
		SET_MODEL( ENT(pev), "models/allied_ammo.mdl" );
		CBasePlayerAmmo::Spawn( );
		pev->team = 1;
	}
};

typedef struct
{
	CBaseEntity*	pEntity;
	float		amount;
	int			type;
} MULTIDAMAGE;

extern MULTIDAMAGE	gMultiDamage;
extern int			giAmmoIndex;
extern short		g_sModelIndexLaser;
extern const char*	g_pModelNameLaser;
extern short		g_sModelIndexLaserDot;
extern short		g_sModelIndexFireball;
extern short		g_sModelIndexSmoke;
extern short		g_sModelIndexBubbles;
extern short		g_sModelIndexBloodDrop;
extern short		g_sModelIndexBloodSpray;
extern unsigned short m_usPlayWhizz;
extern char*		szRespawnWeapons[3];

#ifdef CLIENT_DLL
extern float		flBoltHideXHair;
#endif

void	ClearMultiDamage();
void	ApplyMultiDamage( entvars_t* pevInflictor, entvars_t* pevAttacker );
void	AddMultiDamage( entvars_t* pevInflictor, CBaseEntity* pEntity, float damage, int damageType );
void	SpawnBlood( Vector spot, int bloodColor, float damage );
int		DamageDecal( CBaseEntity* pEntity, int damageType );
void	DecalGunshot( TraceResult* pTrace, int bulletType );
int		MaxAmmoCarry( int ammoName );
void	EjectBrass( const Vector& origin, const Vector& velocity, float rotation, int model, int sound );
void	UTIL_PrecacheOtherWeapon( const char* classname );
void	AddAmmoNameToAmmoRegistry( const char* name );
void	W_Precache();
BOOL	CanAttack( float attackTime, float currentTime, int isPredicted );

void	FindHullIntersection( const Vector& vecSrc, TraceResult& tr, float* mins, float* maxs, edict_t* ignore );

#ifdef CLIENT_DLL
void	ClientSetSensitivity( int level );
#endif
