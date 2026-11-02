#pragma once

class CEagleLaser;

class CEagle : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int GetItemInfo( ItemInfo* info );
	int AddToPlayer( CBasePlayer* player );
	void IncrementAmmo( CBasePlayer* player );
	BOOL Deploy();
	void Holster( int skiplocal = 0 );
	int iItemSlot() { return 2; }
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
	void UpdateLaser();

	int			m_iShell;
	CEagleLaser* m_pLaser;
	int			m_iLaserActive;
	int			m_fSpotVisible;
	unsigned short m_usFireEagle;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
