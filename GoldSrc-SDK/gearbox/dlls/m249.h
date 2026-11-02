#pragma once

class CM249 : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;

	int GetItemInfo( ItemInfo* info ) override;
	int AddToPlayer( CBasePlayer* player ) override;
	void IncrementAmmo( CBasePlayer* player ) override;

	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;

	int iItemSlot() override { return 4; }
	int SecondaryAmmoIndex() override;

	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void Reload() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;

#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif

	unsigned short	m_usFireM249;
	float			m_flNextAnimTime;
	int				m_iShell;
	int				m_iAlternatingEject;
	int				m_iLink;
	int				m_iSmoke;
	int				m_iFire;
	int				m_iReloading;
	float			m_flReloadStartTime;
	float			m_flReloadStart;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
