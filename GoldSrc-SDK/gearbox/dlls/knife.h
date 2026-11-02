#pragma once

class CKnife : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* info ) override;
	int iItemSlot() override { return 1; }
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	BOOL UseDecrement() override;
	int Swing( int first );
	void EXPORT SwingAgain();
	void EXPORT Smack();

	int				m_iSwing;
	TraceResult		m_trHit;
	unsigned short	m_usKnife;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
