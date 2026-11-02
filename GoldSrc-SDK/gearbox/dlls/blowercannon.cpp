#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "op4weap.h"

class CBlowerCannon : public CBaseToggle
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	void EXPORT BlowerCannonStart( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonStop( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT BlowerCannonThink();
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flDelay;
	int		m_iZOffset;
	int		m_iWeaponType;
	int		m_iFireType;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( env_blowercannon, CBlowerCannon );

TYPEDESCRIPTION CBlowerCannon::m_SaveData[] =
{
	DEFINE_FIELD( CBlowerCannon, m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CBlowerCannon, m_iZOffset, FIELD_INTEGER ),
	DEFINE_FIELD( CBlowerCannon, m_iWeaponType, FIELD_INTEGER ),
	DEFINE_FIELD( CBlowerCannon, m_iFireType, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CBlowerCannon, CBaseEntity );

void CBlowerCannon::Spawn()
{
	SetThink( NULL );
	SetUse( &CBlowerCannon::BlowerCannonStart );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( m_flDelay <= 0 )
		m_flDelay = 1;
	Precache();
}

void CBlowerCannon::Precache()
{
	UTIL_PrecacheOther( "displacer_ball" );
	UTIL_PrecacheOther( "spore" );
	UTIL_PrecacheOther( "shock_beam" );
}

void CBlowerCannon::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "delay" ) )
		m_flDelay = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "weaptype" ) )
		m_iWeaponType = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "firetype" ) )
		m_iFireType = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "zoffset" ) )
		m_iZOffset = atoi( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}

	pkvd->fHandled = TRUE;
}

void CBlowerCannon::BlowerCannonStart( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStop );
	SetThink( &CBlowerCannon::BlowerCannonThink );
	pev->nextthink = gpGlobals->time + m_flDelay;
}

void CBlowerCannon::BlowerCannonStop( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	SetUse( &CBlowerCannon::BlowerCannonStart );
	SetThink( NULL );
}

void CBlowerCannon::BlowerCannonThink()
{
	CBaseEntity* pTarget = GetNextTarget();
	Vector vecDir( pTarget->pev->origin.x - pev->origin.x, pTarget->pev->origin.y - pev->origin.y,
		m_iZOffset + pTarget->pev->origin.z - pev->origin.z );
	Vector angles = UTIL_VecToAngles( vecDir );
	angles.x = -angles.x;
	switch ( m_iWeaponType )
	{
	case 1:
	case 2:
		CSpore::CreateSpore( pev->origin, angles, this, m_iWeaponType, FALSE, FALSE );
		break;
	case 3:
		CShockBeam::CreateShockBeam( pev->origin, angles, this );
		break;
	case 4:
		CDisplacerBall::CreateDisplacerBall( pev->origin, angles, this );
		break;
	}

	if ( m_iFireType == 2 )
	{
		SetUse( &CBlowerCannon::BlowerCannonStart );
		SetThink( NULL );
	}
	pev->nextthink = gpGlobals->time + m_flDelay;
}
