#pragma once

class CPenguin : public CBasePlayerWeapon
{
public:
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* info ) override;
	int iItemSlot() override { return 5; }
	BOOL Deploy() override;
	void Holster( int skiplocal = 0 ) override;
	void PrimaryAttack() override;
	void SecondaryAttack() override;
	void WeaponIdle() override;
	BOOL UseDecrement() override;

	int				m_fJustThrown;
	unsigned short	m_usPenguinFire;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

#ifndef CLIENT_DLL
class CPenguinGrenade : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	int IRelationship( CBaseEntity* target ) override;
	int BloodColor() override { return BLOOD_COLOR_YELLOW; }
	void Killed( entvars_t* attacker, int gib ) override;
	void GibMonster() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	static float m_flNextBounceSoundTime;
	void Smoke();

	float	m_flDie;
	Vector	m_vecTarget;
	float	m_flNextHunt;
	float	m_flNextHit;
	Vector	m_posPrev;
	EHANDLE	m_hOwner;
	int		m_iMyClass;

private:
	void EXPORT SuperBounceTouch( CBaseEntity* other );
	void EXPORT HuntThink();
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
#endif
