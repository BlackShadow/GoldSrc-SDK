#pragma once

class CDisplacer : public CBasePlayerWeapon
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

	void Attack();
	void EXPORT SpinupThink();
	void EXPORT AltSpinupThink();
	void EXPORT FireThink();
	void EXPORT AltFireThink();
	void EXPORT InXenThink();
	void EXPORT ReturnThink();
	void CreateChargeEffect();
	void DestroyChargeEffect();

	float		m_flSoundDelay;
	int			m_bMode;
	float		m_flStartTime;
	int			m_iSoundState;
	int			m_iImplodeCounter;
	CBeam*		m_pNoseBeam[4];
	Vector		m_vReturnPoint;
	float		m_flXenTime;
	int			m_iSpriteTexture;
	BOOL		m_fInZoom;
	unsigned short m_usDisplacer;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
