#pragma once

class CSporeLauncher : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int EXPORT GetItemInfo( ItemInfo* info ) override;
	int EXPORT iItemSlot() override { return 4; }
	int EXPORT AddToPlayer( CBasePlayer* player ) override;
	BOOL EXPORT Deploy() override;
	void EXPORT Holster( int skiplocal = 0 ) override;
	void EXPORT PrimaryAttack() override;
	void EXPORT SecondaryAttack() override;
	void Reload() override;
	void EXPORT WeaponIdle() override;
	BOOL EXPORT ShouldWeaponIdle() override { return TRUE; }
	BOOL UseDecrement() override;
	void IncrementAmmo( CBasePlayer* player ) override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	int				m_fInReload;
	float			m_flNextReload;
	int				m_cActiveRockets;
	unsigned short	m_usFireSpore;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

#ifndef CLIENT_DLL
class CSporeAmmo : public CBasePlayerAmmo
{
public:
	void Spawn() override;
	void Precache() override;
	BOOL AddAmmo( CBaseEntity* other ) override;
	int EXPORT TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	void EXPORT Idling();
	void EXPORT SporeTouch( CBaseEntity* other );

	float	m_flRegrowTime;
	float	m_flAnimTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
#endif
