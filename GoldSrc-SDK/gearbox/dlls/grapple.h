#pragma once

class CGrappleTip : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	static CGrappleTip* CreateGrappleTip();
	void SetPosition( Vector origin, Vector angles, CBaseEntity* owner );
	int ClassifyTarget( CBaseEntity* target );
	void EXPORT FlyThink();
	void EXPORT OffsetThink();
	void EXPORT TongueTouch( CBaseEntity* other );

	int				m_iTrail;
	int				m_iGrappleType;
	bool			m_bIsStuck;
	bool			m_bMissed;
	CBaseEntity*	m_pGrappleTarget;
	edict_t*		m_TargetEdict;
	Vector			m_OriginOffset;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CGrapple : public CBasePlayerWeapon
{
public:
	enum GRAPPLE_FIRESTATE
	{
		FIRE_OFF,
		FIRE_CHARGE
	};
	void Spawn() override;
	void Precache() override;
	int GetItemInfo( ItemInfo* info ) override;
	int iItemSlot() override { return 1; }
	BOOL Deploy() override;
	int AddToPlayer( CBasePlayer* player ) override;
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
	void EndAttack();
	void Fire( const Vector& source, const Vector& direction );
	void CreateEffect();
	void UpdateEffect( const Vector& start, const Vector& end, float blend );
	void DestroyEffect();

	float		m_flDamageTime;
	float		m_shootTime;
	CBeam*		m_pBeam;
	CGrappleTip*	m_pTip;
	GRAPPLE_FIRESTATE m_fireState;
	int			m_MomentaryStuck;
	int			m_TipIndex;
	bool		m_bMissed;
	bool		m_bGrappling;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
