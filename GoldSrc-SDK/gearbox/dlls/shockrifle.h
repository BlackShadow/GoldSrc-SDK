#pragma once

class CShockRifle : public CBasePlayerWeapon
{
public:
	void Spawn();
	void Precache();
	int GetItemInfo( ItemInfo* info );
	int AddToPlayer( CBasePlayer* player );
	void AttachToPlayer( CBasePlayer* player );
	BOOL CanDeploy();
	BOOL Deploy();
	void Holster( int skiplocal = 0 );
	int iItemSlot() { return 4; }
	void PrimaryAttack();
	void SecondaryAttack();
	void Reload();
	void WeaponIdle();
	BOOL UseDecrement();
	BOOL HasAmmo();
	void CreateChargeEffect();
	void EXPORT DestroyChargeEffect();

	float		m_flSoundDelay;
	bool		m_bCharging;
	bool		m_bSpinning;
	float		m_flStartTime;
	int			m_iSoundState;
	int			m_iImplodeCounter;
	CBeam*		m_pNoseBeam[4];
	int			m_iSpriteTexture;
	BOOL		m_fInZoom;
	float		m_flRechargeTime;
	unsigned short m_usShockRifle;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
