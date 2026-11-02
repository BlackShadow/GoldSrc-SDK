#pragma once

class CPipewrench : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* info ) override;
	int iItemSlot() override { return 1; }
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;
#ifndef CLIENT_DLL
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
#endif
	int Swing( int first );
	void EXPORT SwingAgain();
	void EXPORT Smack();
	void EXPORT BigSwing();

	int				m_iSwing;
	TraceResult		m_trHit;
	int				m_iSwingMode;
	float			m_flBigSwingStart;
	int				m_iBigSwingHit;
	unsigned short	m_usPipeWrench;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
