#pragma once

class CShockBeam : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; }
	static CShockBeam* CreateShockBeam( Vector origin, Vector angles, CBaseEntity* owner );

private:
	void EXPORT FlyThink();
	void EXPORT ExplodeThink();
	void EXPORT WaterExplodeThink();
	void EXPORT BallTouch( CBaseEntity* other );

public:
	CBeam*	m_pBeam1;
	CBeam*	m_pBeam2;
	CSprite*	m_pSprite;
	int		m_iBeams;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CDisplacerBall : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override { return CLASS_NONE; }
	static CDisplacerBall* CreateDisplacerBall( Vector origin, Vector angles, CBaseEntity* owner );
	int ClassifyTarget( CBaseEntity* target );
	void ArmBeam( int side );
	void ClearBeams();
	void InitBeams();

private:
	void EXPORT BallTouch( CBaseEntity* other );
	void EXPORT KillThink();
	void EXPORT ExplodeThink();
	void EXPORT FizzleThink();
	void EXPORT FlyThink();
	void EXPORT FlyThink2();

public:
	int				m_iTrail;
	CBeam*			m_pBeam[24];
	CBaseEntity*	m_pDisplacedTarget;
	int				m_iBeams;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CSpore : public CGrenade
{
public:
	void Spawn() override;
	void Precache() override;
	void BounceSound() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	static CSpore* CreateSpore( Vector origin, Vector angles, CBaseEntity* owner, int type, BOOL ai, BOOL puked );
	void EXPORT FlyThink();
	void EXPORT IgniteThink();
	void EXPORT GibThink();
	void EXPORT RocketTouch( CBaseEntity* other );
	void EXPORT MyBounceTouch( CBaseEntity* other );

	CSprite*	m_pSprite;
	int			m_iTrail;
	int			m_iBlow;
	int			m_iBlowSmall;
	float		m_flIgniteTime;
	int			m_iSporeType;
	int			m_iSpitSprite;
	int			m_iGibCount;
	BOOL		m_bIsAI;
	BOOL		m_bPuked;
	float		m_flSoundDelay;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};
