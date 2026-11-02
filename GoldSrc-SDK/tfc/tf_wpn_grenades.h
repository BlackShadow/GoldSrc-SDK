#pragma once

#include "weapons.h"

class CTFPrimeGrenade : public CGrenade
{
protected:
	float	heat;
	float	blastRadius;

	void InitPriming( CBaseEntity* pOwner );
	static BOOL canPrime( CBaseEntity* pOwner );

public:
	void Spawn();
	void Precache();
	void EXPORT GrenadePrimed();
	void ExplodeInHand();
	void Throw();
	void DeadThrow();
	virtual float getPrimeTime() { return 3.0; }
	virtual float getPinTime() { return 0.8f; }
	virtual void setName() { pev->classname = MAKE_STRING( "tf_weapon_genericprimedgrenade" ); }
	virtual void setDamage() { pev->dmg = 180; }
	virtual void setRadius() { setDamage(); blastRadius = pev->dmg; }
	virtual void TakeEMPBlast( entvars_t* pevGren );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFNormalGrenade : public CTFPrimeGrenade
{
public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_normalgrenade" ); }
	void setDamage() { pev->dmg = 180; }
	void setRadius() { blastRadius = 180; }
};

class CTFEMPGrenade : public CTFPrimeGrenade
{
	static int BlastTexture;

public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_empgrenade" ); }
	void setDamage() { pev->dmg = 0; }
	void setRadius() { blastRadius = 240; }
	void setModel();
};

class CTFConcussionGrenade : public CTFPrimeGrenade
{
	static int BlastTexture;

public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_concussiongrenade" ); }
	void setDamage() { pev->dmg = 0; }
	void setRadius() { blastRadius = 280; }
	void setModel();
};

class CTFMirvGrenade : public CTFPrimeGrenade
{
public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void setName()
	{
		pev->classname = MAKE_STRING( "tf_weapon_mirvgrenade" );
		deathtype = MAKE_STRING( "mirvgrenade" );
	}
	void setDamage() { pev->dmg = 180; }
	void setRadius() { blastRadius = 180; }
	void setModel();
};

class CTFBomblet : public CTFPrimeGrenade
{
public:
	void setName()
	{
		pev->classname = MAKE_STRING( "tf_weapon_mirvbomblet" );
		deathtype = MAKE_STRING( "mirvgrenade" );
	}
	void setModel();
	void LaunchFromMirv( CTFMirvGrenade* pMirvGren );
	void Explode( TraceResult* pTrace, int bitsDamageType );
};

class CTFNailGrenade : public CTFPrimeGrenade
{
public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void EXPORT NailGrenadeNailEm();
	void EXPORT NailGrenadeLaunchNail();
	void EXPORT FinishedExplode();
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_nailgrenade" ); }
	void setModel();
};

class CTFNapalmGrenade : public CTFPrimeGrenade
{
	int		numExplosionsLeft;
	int		m_bitsDamageType;
	float	m_flEffectTime;
	float	m_flDamageTime;

public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void EXPORT Burn();
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_napalmgrenade" ); }
	void setDamage() { pev->dmg = 20; }
	void setRadius() { blastRadius = 180; }
	void setModel();
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFGasGrenade : public CTFPrimeGrenade
{
	int gasCharges;

public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void EXPORT GasThePlace();
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_gasgrenade" ); }
	void setDamage() { pev->dmg = 10; }
	void setRadius() { blastRadius = 240; }
	void setModel();
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CTFCaltropGrenade : public CTFPrimeGrenade
{
	void InitPriming( CBaseEntity* pOwner );

public:
	void Precache();
	static CGrenade* Prime( CBaseEntity* pOwner );
	void Explode( TraceResult* pTrace, int bitsDamageType );
	void TakeEMPBlast( entvars_t* pevGren );
	void setDamage() { pev->dmg = 0; }
	float getPrimeTime() { return 0.5; }
	void setModel();
	void EXPORT CaltropGrenadePrimed();
	void setName() { pev->classname = MAKE_STRING( "tf_weapon_caltropgrenade" ); }
};

class CTFCaltrop : public CBaseEntity
{
	float m_flRemovalTime;

public:
	void Precache();
	void setModel();
	void ScatterDrop( CBaseEntity* pThrower );
	void EXPORT CaltropScatterThink();
	void EXPORT CanisterTouch( CBaseEntity* pOther );
	void EXPORT CaltropTouch( CBaseEntity* pOther );
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
