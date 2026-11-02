#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "animation.h"
#include "soundent.h"
#include "decals.h"

#define SF_OP4MORTAR_ACTIVE		1
#define SF_OP4MORTAR_LINEOFSIGHT	16
#define SF_OP4MORTAR_TRIGGERABLE	32

class CMortarShell : public CGrenade
{
public:
	void Spawn();
	void Precache();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	void EXPORT BurnThink();
	void EXPORT FlyThink();
	void EXPORT MortarExplodeTouch( CBaseEntity* pOther );
	static CMortarShell* CreateMortarShell( Vector origin, Vector angles, CBaseEntity* pOwner, int velocity );
	static TYPEDESCRIPTION m_SaveData[];

	int		m_iTrail;
	float	m_flIgniteTime;
	int		m_velocity;
	int		m_iSoundedOff;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COp4Mortar : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	int ObjectCaps() { return 0; }
	int Save( CSave& save );
	int Restore( CRestore& restore );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT MortarThink();
	void PlaySound();
	void UpdatePosition( int direction, int controller );
	void AIUpdatePosition();
	void EXPORT Off();
	CBaseEntity* FindTarget();
	static TYPEDESCRIPTION m_SaveData[];

	int		d_x;
	int		d_y;
	float	m_lastupdate;
	int		m_playsound;
	int		m_updated;
	int		m_direction;
	Vector	m_start;
	Vector	m_end;
	int		m_velocity;
	int		m_hmin;
	int		m_hmax;
	float	m_fireLast;
	float	m_maxRange;
	float	m_minRange;
	int		m_iEnemyType;
	float	m_fireDelay;
	float	m_trackDelay;
	BOOL	m_tracking;
	float	m_zeroYaw;
	Vector	m_vGunAngle;
	Vector	m_vIdealGunVector;
	Vector	m_vIdealGunAngle;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COp4MortarController : public CBaseToggle
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int ObjectCaps() { return FCAP_CONTINUOUS_USE; }
	int Save( CSave& save );
	int Restore( CRestore& restore );
	void EXPORT Reverse();
	static TYPEDESCRIPTION m_SaveData[];

	int		m_direction;
	int		m_controller;
	float	m_lastpush;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( op4mortar, COp4Mortar );
LINK_ENTITY_TO_CLASS( func_op4mortarcontroller, COp4MortarController );
LINK_ENTITY_TO_CLASS( mortar_shell, CMortarShell );

TYPEDESCRIPTION COp4Mortar::m_SaveData[] =
{
	DEFINE_FIELD( COp4Mortar, d_x, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, d_y, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_lastupdate, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_playsound, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_updated, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_direction, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_start, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_end, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_velocity, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_hmin, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_hmax, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_fireLast, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_iEnemyType, FIELD_INTEGER ),
	DEFINE_FIELD( COp4Mortar, m_fireDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_trackDelay, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_tracking, FIELD_BOOLEAN ),
	DEFINE_FIELD( COp4Mortar, m_zeroYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COp4Mortar, m_vGunAngle, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_vIdealGunVector, FIELD_VECTOR ),
	DEFINE_FIELD( COp4Mortar, m_vIdealGunAngle, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( COp4Mortar, CBaseToggle );

TYPEDESCRIPTION COp4MortarController::m_SaveData[] =
{
	DEFINE_FIELD( COp4MortarController, m_direction, FIELD_INTEGER ),
	DEFINE_FIELD( COp4MortarController, m_controller, FIELD_INTEGER ),
	DEFINE_FIELD( COp4MortarController, m_lastpush, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COp4MortarController, CBaseToggle );

TYPEDESCRIPTION CMortarShell::m_SaveData[] =
{
	DEFINE_FIELD( CMortarShell, m_velocity, FIELD_INTEGER ),
	DEFINE_FIELD( CMortarShell, m_flIgniteTime, FIELD_TIME ),
	DEFINE_FIELD( CMortarShell, m_iSoundedOff, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CMortarShell, CBaseMonster );

void COp4MortarController::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_PUSH;
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), STRING( pev->model ) );
	m_direction = 1;
	m_lastpush = gpGlobals->time;
}

void COp4MortarController::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "mortar_axis" ) )
	{
		m_controller = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseToggle::KeyValue( pkvd );
}

void COp4MortarController::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( gpGlobals->time - m_lastpush > 0.5 )
		m_direction = -m_direction;

	edict_t* pent = NULL;
	while ( !FNullEnt( pent = FIND_ENTITY_BY_TARGETNAME( pent, STRING( pev->target ) ) ) )
	{
		if ( FClassnameIs( pent, "op4mortar" ) )
		{
			COp4Mortar* pMortar = (COp4Mortar*)CBaseEntity::Instance( pent );
			if ( pMortar )
				pMortar->UpdatePosition( m_direction, m_controller );
		}
	}
	m_lastpush = gpGlobals->time;
}

void COp4MortarController::Reverse()
{
	m_direction = -m_direction;
	SetThink( NULL );
}

void COp4Mortar::Precache()
{
	PRECACHE_MODEL( "models/mortar.mdl" );
	UTIL_PrecacheOther( "mortar_shell" );
	PRECACHE_SOUND( "weapons/mortarhit.wav" );
	PRECACHE_SOUND( "player/pl_grate1.wav" );
}

void COp4Mortar::Spawn()
{
	Precache();
	UTIL_SetOrigin( pev, pev->origin );
	SET_MODEL( ENT( pev ), "models/mortar.mdl" );
	pev->health = 1;
	pev->sequence = LookupSequence( "idle" );
	ResetSequenceInfo();
	pev->frame = 0;
	pev->framerate = 1;
	m_tracking = FALSE;
	if ( !( m_fireDelay >= 0.5 ) )
		m_fireDelay = 5;
	if ( m_minRange == 0 )
		m_minRange = 128;
	if ( m_maxRange == 0 )
		m_maxRange = 2048;
	InitBoneControllers();
	m_vGunAngle = g_vecZero;
	m_lastupdate = gpGlobals->time;
	m_zeroYaw = UTIL_AngleMod( pev->angles.y + 180 );
	m_fireLast = gpGlobals->time;
	m_trackDelay = gpGlobals->time;
	m_hEnemy = NULL;
	pev->nextthink = gpGlobals->time + 0.01;
	SetThink( &COp4Mortar::MortarThink );
}

void COp4Mortar::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "h_max" ) )
		m_hmax = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "h_min" ) )
		m_hmin = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "mortar_velocity" ) )
		m_velocity = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "mindist" ) )
		m_minRange = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "maxdist" ) )
		m_maxRange = atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "enemytype" ) )
		m_iEnemyType = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "firedelay" ) )
		m_fireDelay = (float)atoi( pkvd->szValue );
	else
	{
		CBaseToggle::KeyValue( pkvd );
		return;
	}
	pkvd->fHandled = TRUE;
}

int COp4Mortar::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0, bitsDamageType );
}

void COp4Mortar::PlaySound()
{
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "player/pl_grate1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void COp4Mortar::Off()
{
	m_playsound = 1;
}

void COp4Mortar::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_TOGGLE && ( !pActivator || pActivator->Classify() == CLASS_MACHINE ) &&
		!( pev->spawnflags & SF_OP4MORTAR_ACTIVE ) && ( pev->spawnflags & SF_OP4MORTAR_TRIGGERABLE ) )
	{
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/mortarhit.wav", 1, ATTN_NONE, 0, PITCH_NORM );
		UTIL_ScreenShake( pev->origin, 12, 100, 2, 1000 );
		Vector pos, angle;
		GetAttachment( 0, pos, angle );
		angle = m_vGunAngle;
		angle.y = UTIL_AngleMod( pev->angles.y + m_vGunAngle.y );
		if ( CMortarShell::CreateMortarShell( pos, angle, pActivator ? pActivator : this, m_velocity ) )
		{
			pev->sequence = LookupSequence( "fire" );
			pev->frame = 0;
			ResetSequenceInfo();
		}
		return;
	}

	if ( !ShouldToggle( useType, pev->spawnflags & SF_OP4MORTAR_ACTIVE ) )
		return;
	pev->spawnflags ^= SF_OP4MORTAR_ACTIVE;
	m_fireLast = 0;
	m_hEnemy = NULL;
}

void COp4Mortar::UpdatePosition( int direction, int controller )
{
	if ( !( gpGlobals->time - m_lastupdate >= 0.09 ) )
		return;
	if ( controller == 0 )
		d_x = 3 * direction;
	else if ( controller == 1 )
		d_y = 3 * direction;

	double pitch = (double)m_vGunAngle.x + d_x;
	double yaw = (double)m_vGunAngle.y + d_y;
	m_vGunAngle.x = (float)pitch;
	m_vGunAngle.y = (float)yaw;
	if ( m_hmin > yaw )
	{
		m_vGunAngle.y = (float)m_hmin;
		yaw = m_hmin;
		d_y = 0;
	}
	if ( yaw > m_hmax )
	{
		m_vGunAngle.y = (float)m_hmax;
		d_y = 0;
	}
	if ( pitch < 10 )
	{
		m_vGunAngle.x = 10;
		d_x = 0;
	}
	else if ( !( pitch <= 90 ) )
	{
		m_vGunAngle.x = 90;
		d_x = 0;
	}
	if ( d_x || d_y )
		PlaySound();
	SetBoneController( 0, m_vGunAngle.x );
	SetBoneController( 1, m_vGunAngle.y );
	d_x = 0;
	d_y = 0;
	m_lastupdate = gpGlobals->time;
}

void COp4Mortar::AIUpdatePosition()
{
	if ( fabs( m_vGunAngle.x - m_vIdealGunAngle.x ) >= 3 )
	{
		float diff = UTIL_AngleDiff( m_vGunAngle.x, m_vIdealGunAngle.x );
		if ( diff > 0 )
			d_x = fabs( diff ) <= 3 ? (int)( -fabs( diff ) ) : -3;
		else if ( !( diff >= 0 ) )
			d_x = fabs( diff ) <= 3 ? (int)fabs( diff ) : 3;
	}

	if ( !( fabs( UTIL_AngleMod( m_zeroYaw + m_vGunAngle.y ) - m_vIdealGunAngle.y ) < 3 ) )
	{
		float diff = UTIL_AngleDiff( UTIL_AngleMod( m_zeroYaw + m_vGunAngle.y ), m_vIdealGunAngle.y );
		if ( diff > 0 )
			d_y = fabs( diff ) > 3 ? -3 : (int)( -fabs( diff ) );
		else if ( !( diff >= 0 ) )
			d_y = fabs( diff ) > 3 ? 3 : (int)fabs( diff );
	}

	double pitch = (double)m_vGunAngle.x + d_x;
	double yaw = (double)m_vGunAngle.y + d_y;
	m_vGunAngle.x = (float)pitch;
	m_vGunAngle.y = (float)yaw;
	if ( m_hmin > yaw )
	{
		m_vGunAngle.y = (float)m_hmin;
		yaw = m_hmin;
		d_y = 0;
	}
	if ( yaw > m_hmax )
	{
		m_vGunAngle.y = (float)m_hmax;
		d_y = 0;
	}
	if ( pitch < 10 )
	{
		m_vGunAngle.x = 10;
		d_x = 0;
	}
	else if ( pitch > 90 )
	{
		m_vGunAngle.x = 90;
		d_x = 0;
	}
	if ( d_x || d_y )
		PlaySound();
	SetBoneController( 0, m_vGunAngle.x );
	SetBoneController( 1, m_vGunAngle.y );
	d_x = 0;
	d_y = 0;
}

CBaseEntity* COp4Mortar::FindTarget()
{
	float flIdealDist = m_maxRange;
	CBaseEntity* pPlayerTarget = UTIL_FindEntityByClassname( NULL, "player" );
	if ( !pPlayerTarget )
		return NULL;

	m_pLink = NULL;
	Vector barrelEnd, barrelAngle;
	GetAttachment( 0, barrelEnd, barrelAngle );
	CBaseEntity* pIdealTarget = NULL;
	if ( pPlayerTarget->IsAlive() )
	{
		float distance = ( pPlayerTarget->pev->origin - pev->origin ).Length();
		if ( !( distance < m_minRange ) && m_maxRange >= distance )
		{
			TraceResult tr;
			UTIL_TraceLine( barrelEnd, pPlayerTarget->pev->origin + pPlayerTarget->pev->view_ofs,
				dont_ignore_monsters, ENT( pev ), &tr );
			if ( !( pev->spawnflags & SF_OP4MORTAR_LINEOFSIGHT ) || tr.pHit == ENT( pPlayerTarget->pev ) )
			{
				if ( m_iEnemyType == 0 )
					return pPlayerTarget;
				flIdealDist = distance;
				pIdealTarget = pPlayerTarget;
			}
		}
	}

	Vector mins = pev->origin - Vector( m_maxRange, m_maxRange, m_maxRange );
	Vector maxs = pev->origin + Vector( m_maxRange, m_maxRange, m_maxRange );
	CBaseEntity* pList[100];
	int count = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), mins, maxs, FL_MONSTER | FL_CLIENT );
	for ( int i = 0; i < count; i++ )
	{
		CBaseEntity* pEntity = pList[i];
		if ( pEntity == this || ( pEntity->pev->spawnflags & SF_MONSTER_PRISONER ) || pEntity->pev->health <= 0 )
			continue;
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if ( !pMonster || pMonster->IRelationship( pPlayerTarget ) != R_AL ||
			( pEntity->pev->flags & FL_NOTARGET ) || !FVisible( pEntity ) )
			continue;

		if ( pEntity->IsPlayer() && ( pev->spawnflags & SF_OP4MORTAR_ACTIVE ) )
		{
			if ( !pEntity->MyMonsterPointer()->FInViewCone( this ) )
				continue;
			pev->spawnflags &= ~SF_OP4MORTAR_ACTIVE;
		}
		pEntity->m_pLink = m_pLink;
		m_pLink = pEntity;
	}

	for ( CBaseEntity* pTarget = m_pLink; pTarget; pTarget = pTarget->m_pLink )
	{
		float distance = ( pTarget->pev->origin - pev->origin ).Length();
		if ( distance >= m_minRange && m_maxRange >= distance && ( !pIdealTarget || flIdealDist > distance ) )
		{
			TraceResult tr;
			UTIL_TraceLine( barrelEnd, pTarget->pev->origin + pTarget->pev->view_ofs,
				dont_ignore_monsters, ENT( pev ), &tr );
			if ( !( pev->spawnflags & SF_OP4MORTAR_LINEOFSIGHT ) || tr.pHit == ENT( pTarget->pev ) )
			{
				flIdealDist = distance;
				pIdealTarget = pTarget;
			}
		}
	}
	return pIdealTarget;
}

void COp4Mortar::MortarThink()
{
	float interval = StudioFrameAdvance();
	if ( m_fSequenceFinished && pev->sequence != LookupSequence( "idle" ) )
	{
		pev->frame = 0;
		pev->sequence = LookupSequence( "idle" );
		ResetSequenceInfo();
	}
	DispatchAnimEvents( interval );
	UpdateShockEffect();
	pev->nextthink = gpGlobals->time + 0.1;
	if ( !( pev->spawnflags & SF_OP4MORTAR_ACTIVE ) )
		return;

	if ( m_hEnemy == NULL )
		m_hEnemy = FindTarget();
	CBaseEntity* pTarget = m_hEnemy;
	if ( !pTarget )
		return;

	float distance = ( pTarget->pev->origin - pev->origin ).Length();
	if ( !pTarget->IsAlive() || !( m_minRange <= distance ) || !( distance <= m_maxRange ) )
	{
		m_hEnemy = NULL;
		return;
	}

	Vector pos, angle;
	if ( gpGlobals->time - m_trackDelay > 0.5 )
	{
		GetAttachment( 0, pos, angle );
		m_vIdealGunVector = VecCheckThrow( pev, pos, pTarget->pev->origin, (float)( m_velocity / 2 ), 1 );
		m_vIdealGunAngle = UTIL_VecToAngles( m_vIdealGunVector );
		m_trackDelay = gpGlobals->time;
	}
	AIUpdatePosition();
	float speed = m_vIdealGunVector.Length();
	if ( speed > 1 && gpGlobals->time - m_fireLast > m_fireDelay )
	{
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/mortarhit.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		UTIL_ScreenShake( pev->origin, 12, 100, 2, 1000 );
		GetAttachment( 0, pos, angle );
		angle = m_vGunAngle;
		angle.y = UTIL_AngleMod( pev->angles.y + m_vGunAngle.y );
		float shellSpeed = m_vIdealGunVector.Length();
		if ( CMortarShell::CreateMortarShell( pos, angle, this, (int)shellSpeed ) )
		{
			pev->sequence = LookupSequence( "fire" );
			pev->frame = 0;
			ResetSequenceInfo();
		}
		m_fireLast = gpGlobals->time;
	}
	else if ( speed <= 1 )
		m_fireLast = gpGlobals->time;
}

void CMortarShell::Precache()
{
	PRECACHE_MODEL( "models/mortarshell.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/wep_smoke_01.spr" );
	PRECACHE_SOUND( "weapons/gauss2.wav" );
	PRECACHE_SOUND( "weapons/ofmortar.wav" );
}

void CMortarShell::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/mortarshell.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "mortar_shell" );
	SetThink( &CMortarShell::BurnThink );
	SetTouch( &CMortarShell::MortarExplodeTouch );
	UTIL_MakeVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * (float)m_velocity;
	pev->velocity = -pev->velocity;
	pev->gravity = 1;
	pev->dmg = gSkillData.plrDmgRPG + gSkillData.plrDmgRPG;
	pev->nextthink = gpGlobals->time + 0.01;
	m_flIgniteTime = gpGlobals->time;
	m_iSoundedOff = 0;
}

CMortarShell* CMortarShell::CreateMortarShell( Vector origin, Vector angles, CBaseEntity* pOwner, int velocity )
{
	CMortarShell* pShell = GetClassPtr( (CMortarShell*)NULL );
	UTIL_SetOrigin( pShell->pev, origin );
	pShell->pev->angles = angles;
	pShell->m_velocity = velocity;
	pShell->Spawn();
	pShell->pev->owner = ENT( pOwner->pev );
	return pShell;
}

void CMortarShell::BurnThink()
{
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.x -= 90;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 0 );
		WRITE_COORD( 0 );
		WRITE_COORD( 1 );
		WRITE_SHORT( m_iTrail );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 12 );
		WRITE_BYTE( 120 );
	MESSAGE_END();
	if ( gpGlobals->time > m_flIgniteTime + 0.2 )
		SetThink( &CMortarShell::FlyThink );
	pev->nextthink = gpGlobals->time + 0.01;
}

void CMortarShell::FlyThink()
{
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.x -= 90;
	if ( pev->velocity.z < 20 && m_iSoundedOff == 0 )
	{
		m_iSoundedOff = 1;
		float volume = RANDOM_FLOAT( 0.8, 0.9 );
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/ofmortar.wav", volume, ATTN_NONE, 0, PITCH_NORM );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CMortarShell::MortarExplodeTouch( CBaseEntity* pOther )
{
	pev->enemy = ENT( pOther->pev );
	Vector vecSpot = pev->origin - pev->velocity.Normalize() * 32;
	Vector vecEnd = vecSpot + pev->velocity.Normalize() * 64;
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecEnd, ignore_monsters, ENT( pev ), &tr );
	pev->model = 0;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	if ( tr.flFraction != 1 )
		pev->origin = tr.vecEndPos + ( pev->dmg - 24 ) * tr.vecPlaneNormal * 0.6f;

	int contents = UTIL_PointContents( pev->origin );
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_EXPLOSION );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_SHORT( contents == CONTENTS_WATER ? g_sModelIndexWExplosion : g_sModelIndexFireball );
		WRITE_BYTE( (int)( ( pev->dmg - 50 ) * 5 ) );
		WRITE_BYTE( 10 );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 3 );
	entvars_t* pevOwner = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	RadiusDamage( pev, pevOwner, pev->dmg, CLASS_NONE, DMG_BLAST );
	if ( RANDOM_FLOAT( 0, 1 ) >= 0.5 )
		UTIL_DecalTrace( &tr, DECAL_SCORCH2 );
	else
		UTIL_DecalTrace( &tr, DECAL_SCORCH1 );
	RANDOM_FLOAT( 0, 1 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/debris1.wav", 0.55, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 1:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/debris2.wav", 0.55, ATTN_NORM, 0, PITCH_NORM );
		break;
	case 2:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/debris3.wav", 0.55, ATTN_NORM, 0, PITCH_NORM );
		break;
	}
	pev->effects |= EF_NODRAW;
	SetThink( &CMortarShell::Smoke );
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.3;
	if ( contents != CONTENTS_WATER )
	{
		int count = RANDOM_LONG( 0, 3 );
		for ( int i = 0; i < count; i++ )
			Create( "spark_shower", pev->origin, tr.vecPlaneNormal, NULL );
	}
}
