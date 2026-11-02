#pragma once

class CSniperRifle : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int GetItemInfo( ItemInfo* info );
	int AddToPlayer( CBasePlayer* player );
	void IncrementAmmo( CBasePlayer* player );
	BOOL Deploy();
	void Holster( int skiplocal = 0 );
	int iItemSlot() { return 4; }
	void PrimaryAttack();
	void SecondaryAttack();
	void Reload();
	void WeaponIdle();
	BOOL UseDecrement();
#ifndef CLIENT_DLL
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];
#endif
	void SetZoom();
	void EXPORT Reload2();

	int			m_iDummy;
	int			m_fInZoom;
	float		m_flReloadStart;
	int			m_iReloading;
	unsigned short m_usSniper;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
