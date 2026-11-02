/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"
#include "explode.h"
#include "monsters.h"

#include "player.h"


#define SF_TANK_ACTIVE			0x0001
#define SF_TANK_PLAYER			0x0002
#define SF_TANK_HUMANS			0x0004
#define SF_TANK_ALIENS			0x0008
#define SF_TANK_LINEOFSIGHT		0x0010
#define SF_TANK_CANCONTROL		0x0020
#define SF_TANK_SPOT				0x0040
#define SF_TANK_PLAYER_AIM		0x0080
#define SF_TANK_SOUNDON			0x8000

enum TANKBULLET
{
	TANK_BULLET_NONE = 0,
	TANK_BULLET_9MM = 1,
	TANK_BULLET_MP5 = 2,
	TANK_BULLET_12MM = 3,
};

//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank;

class CFuncTankControls : public CBaseEntity
{
public:
	int ObjectCaps() override;
	void Spawn() override;
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value ) override;
	BOOL OnControls( entvars_t* pevTest ) override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL			m_active;
	Vector			m_vecControllerUsePos;
	CBasePlayer*	m_pController;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CFuncTank : public CBaseEntity
{
public:
	void	Spawn( void );
	void	Precache( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void	EXPORT TrackThink();
	void	TrackTarget( void );
	void	UpdateSpot();
	int		IRelationship( CBaseEntity* pTarget );
	virtual CBaseEntity* BestVisibleEnemy();

	virtual void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	virtual Vector UpdateTargetPosition( CBaseEntity *pTarget )
	{
		return pTarget->BodyTarget( pev->origin );
	}

	void	StartRotSound( void );
	void	StopRotSound( void );

	// Bmodels don't go across transitions
	virtual int	ObjectCaps( void ) { return CBaseEntity :: ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	inline BOOL IsActive( void ) { return (pev->spawnflags & SF_TANK_ACTIVE)?TRUE:FALSE; }
	inline void TankActivate( void ) { pev->spawnflags |= SF_TANK_ACTIVE; pev->nextthink = pev->ltime + 0.1; m_fireLast = 0; }
	inline void TankDeactivate( void ) { pev->spawnflags &= ~SF_TANK_ACTIVE; m_fireLast = 0; StopRotSound(); }
	inline BOOL CanFire( void ) { return (gpGlobals->time - m_lastSightTime) < m_persist; }
	BOOL		InRange( float range );

	// Acquire a target.  pPlayer is a player in the PVS
	edict_t		*FindTarget( edict_t *pPlayer );

	void		TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr );

	Vector		BarrelPosition( void )
	{
		Vector forward, right, up;
		UTIL_MakeVectorsPrivate( pev->angles, forward, right, up );
		return pev->origin + (forward * m_barrelPos.x) + (right * m_barrelPos.y) + (up * m_barrelPos.z);
	}

	void		AdjustAnglesForBarrel( Vector &angles, float distance );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

	BOOL StartControl( CBasePlayer* pController, CFuncTankControls* pControls );
	void StopControl( CFuncTankControls* pControls );


protected:
	CFuncTankControls* m_pControls;
	float		m_flNextAttack;
	
	float		m_yawCenter;	// "Center" yaw
	float		m_yawRate;		// Max turn rate to track targets
	float		m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
								// Zero is full rotation
	float		m_yawTolerance;	// Tolerance angle

	float		m_pitchCenter;	// "Center" pitch
	float		m_pitchRate;	// Max turn rate on pitch
	float		m_pitchRange;	// Range of pitch motion as above
	float		m_pitchTolerance;	// Tolerance angle

	float		m_fireLast;		// Last time I fired
	float		m_fireRate;		// How many rounds/second
	float		m_lastSightTime;// Last time I saw target
	float		m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float		m_minRange;		// Minimum range to aim/track
	float		m_maxRange;		// Max range to aim/track

	Vector		m_barrelPos;	// Length of the freakin barrel
	float		m_spriteScale;	// Scale of any sprites we shoot
	int			m_iszSpriteSmoke;
	int			m_iszSpriteFlash;
	TANKBULLET	m_bulletType;	// Bullet type
	int			m_iBulletDamage; // 0 means use Bullet type's default damage
	
	Vector		m_sightOrigin;	// Last sight of target
	int			m_spread;		// firing spread
	int			m_iszMaster;	// Master entity (game_team_master or multisource)
	int			m_iTankClass;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};


TYPEDESCRIPTION	CFuncTank::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTank, m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_yawTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_pitchTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_fireRate, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_persist, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_minRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_maxRange, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spriteScale, FIELD_FLOAT ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteSmoke, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_iszSpriteFlash, FIELD_STRING ),
	DEFINE_FIELD( CFuncTank, m_bulletType, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_sightOrigin, FIELD_VECTOR ),
	DEFINE_FIELD( CFuncTank, m_spread, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_pControls, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTank, m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( CFuncTank, m_iBulletDamage, FIELD_INTEGER ),
	DEFINE_FIELD( CFuncTank, m_iszMaster, FIELD_STRING ),
};

IMPLEMENT_SAVERESTORE( CFuncTank, CBaseEntity );

static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};
#define MAX_FIRING_SPREADS ARRAYSIZE(gTankSpread)


void CFuncTank :: Spawn( void )
{
	Precache();

	pev->movetype	= MOVETYPE_PUSH;  // so it doesn't get pushed by anything
	pev->solid		= SOLID_BSP;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	m_yawCenter = pev->angles.y;
	m_pitchCenter = pev->angles.x;

	if ( IsActive() )
		pev->nextthink = pev->ltime + 1.0;

	m_sightOrigin = BarrelPosition(); // Point at the end of the barrel

	if ( m_fireRate <= 0 )
		m_fireRate = 1;
	if ( m_spread > MAX_FIRING_SPREADS )
		m_spread = 0;

	pev->oldorigin = pev->origin;
	SetThink( &CFuncTank::TrackThink );
}


void CFuncTank :: Precache( void )
{
	if ( m_iszSpriteSmoke )
		PRECACHE_MODEL( (char *)STRING(m_iszSpriteSmoke) );
	if ( m_iszSpriteFlash )
		PRECACHE_MODEL( (char *)STRING(m_iszSpriteFlash) );

	if ( pev->noise )
		PRECACHE_SOUND( (char *)STRING(pev->noise) );
}


void CFuncTank :: KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "yawrate"))
	{
		m_yawRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawrange"))
	{
		m_yawRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "yawtolerance"))
	{
		m_yawTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrange"))
	{
		m_pitchRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchrate"))
	{
		m_pitchRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "pitchtolerance"))
	{
		m_pitchTolerance = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firerate"))
	{
		m_fireRate = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrel"))
	{
		m_barrelPos.x = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrely"))
	{
		m_barrelPos.y = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "barrelz"))
	{
		m_barrelPos.z = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritescale"))
	{
		m_spriteScale = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spritesmoke"))
	{
		m_iszSpriteSmoke = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "spriteflash"))
	{
		m_iszSpriteFlash = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "rotatesound"))
	{
		pev->noise = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "persistence"))
	{
		m_persist = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "bullet"))
	{
		m_bulletType = (TANKBULLET)atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq(pkvd->szKeyName, "bullet_damage" )) 
	{
		m_iBulletDamage = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "firespread"))
	{
		m_spread = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "minRange"))
	{
		m_minRange = atof(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "maxRange"))
	{
		m_maxRange = atof( pkvd->szValue ) > 1024 ? 1024 : atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if (FStrEq(pkvd->szKeyName, "master"))
	{
		m_iszMaster = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "m_iClass" ) )
	{
		m_iTankClass = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

BOOL CFuncTank::StartControl( CBasePlayer* pController, CFuncTankControls* pControls )
{
	if ( m_pControls )
		return FALSE;
	if ( m_iszMaster && !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
		return FALSE;
	m_pControls = pControls;
	pev->nextthink = pev->ltime + 0.1;
	return TRUE;
}

void CFuncTank::StopControl( CFuncTankControls* pControls )
{
	if ( !m_pControls || m_pControls != pControls )
		return;
	StopRotSound();
	pev->nextthink = 0;
	m_pControls = NULL;
	if ( IsActive() )
		pev->nextthink = pev->ltime + 1.0;
}


void CFuncTank :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( pev->spawnflags & SF_TANK_CANCONTROL )
		pActivator->Classify();
	else
	{
		if ( !ShouldToggle( useType, IsActive() ) )
			return;

		if ( IsActive() )
			TankDeactivate();
		else
			TankActivate();
	}
}


edict_t *CFuncTank :: FindTarget( edict_t *pPlayer )
{
	return pPlayer;
}



BOOL CFuncTank :: InRange( float range )
{
	if ( range < m_minRange )
		return FALSE;
	if ( m_maxRange > 0 && range > m_maxRange )
		return FALSE;

	return TRUE;
}


void CFuncTank::TrackThink()
{
	pev->avelocity = g_vecZero;
	TrackTarget();

	if ( fabs(pev->avelocity.x) > 1 || fabs(pev->avelocity.y) > 1 )
		StartRotSound();
	else
		StopRotSound();
}

void CFuncTank::UpdateSpot()
{
	if ( !(pev->spawnflags & SF_TANK_SPOT) )
		return;
	Vector vecAiming;
	UTIL_MakeVectorsPrivate( pev->angles, vecAiming, NULL, NULL );
	Vector vecSrc = BarrelPosition();
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecSrc + vecAiming * 8192, dont_ignore_monsters, edict(), &tr );
}

int CFuncTank::IRelationship( CBaseEntity* pTarget )
{
	int targetClass = pTarget->Classify();
	if ( targetClass == CLASS_NONE )
		return R_NO;
	switch ( m_iTankClass )
	{
	case CLASS_NONE:
		return targetClass == CLASS_PLAYER ? R_HT : R_NO;
	case CLASS_HUMAN_MILITARY:
		if ( targetClass >= 0 && targetClass <= 11 && ((1 << targetClass) & 0xBA4) )
			return R_HT;
		return targetClass == CLASS_HUMAN_PASSIVE ? R_DL : R_NO;
	case CLASS_ALIEN_MILITARY:
		if ( targetClass == CLASS_PLAYER || targetClass == CLASS_HUMAN_MILITARY || targetClass == CLASS_PLAYER_ALLY )
			return R_HT;
		return targetClass == CLASS_HUMAN_PASSIVE ? R_DL : R_NO;
	case CLASS_PLAYER_ALLY:
		if ( targetClass >= 0 && targetClass <= 9 && ((1 << targetClass) & 0x3B2) )
			return R_HT;
		return R_NO;
	case 14:
		return targetClass == CLASS_PLAYER && pTarget->pev->team == 1 ? R_AL : R_HT;
	case 15:
		return targetClass == CLASS_PLAYER && pTarget->pev->team == 2 ? R_AL : R_HT;
	default:
		return R_NO;
	}
}

CBaseEntity* CFuncTank::BestVisibleEnemy()
{
	int lookDistance = m_maxRange > 512 ? (int)m_maxRange : 512;
	Vector mins = pev->origin - Vector( (float)lookDistance, (float)lookDistance, (float)lookDistance );
	Vector maxs = pev->origin + Vector( (float)lookDistance, (float)lookDistance, (float)lookDistance );
	CBaseEntity* pList[100];
	int count = UTIL_EntitiesInBox( pList, ARRAYSIZE( pList ), mins, maxs, FL_CLIENT | FL_MONSTER );
	int bestRelationship = R_NO;
	int nearest = 8192;
	CBaseEntity* pReturn = NULL;
	for ( int i = 0; i < count; ++i )
	{
		CBaseEntity* pTarget = pList[i];
		if ( !pTarget->IsAlive() )
			continue;
		if ( IRelationship( pTarget ) > bestRelationship )
		{
			bestRelationship = IRelationship( pTarget );
			nearest = (int)(pTarget->pev->origin - pev->origin).Length();
			pReturn = pTarget;
		}
		else if ( bestRelationship == IRelationship( pTarget ) )
		{
			int distance = (int)(pTarget->pev->origin - pev->origin).Length();
			if ( distance <= nearest )
			{
				nearest = distance;
				bestRelationship = IRelationship( pTarget );
				pReturn = pTarget;
			}
		}
	}
	return pReturn;
}

void CFuncTank::TrackTarget( void )
{
	TraceResult tr;
	BOOL updateTime = FALSE;
	Vector angles, direction, targetPosition, barrelEnd;
	CBaseEntity* pTarget = NULL;
	CBasePlayer* pController = m_pControls ? m_pControls->m_pController : NULL;

	// Get a position to aim for
	if ( pController )
	{
		UpdateSpot();
		pev->nextthink = pev->ltime + 0.05;
		angles = pController->pev->v_angle;
		if ( pev->spawnflags & SF_TANK_PLAYER_AIM )
		{
			Vector vecForward;
			UTIL_MakeVectorsPrivate( angles, vecForward, NULL, NULL );
			targetPosition = pController->EyePosition() + vecForward * 8192;
			if ( !pev->owner || pev->owner == pController->edict() )
				pev->owner = pController->edict();
			else
				ALERT( at_console, "func_tank has owner\n" );
			UTIL_TraceLine( pController->EyePosition(), targetPosition, missile, ignore_glass, edict(), &tr );
			direction = tr.vecEndPos - pev->origin;
			angles = UTIL_VecToAngles( direction );
			targetPosition = tr.vecEndPos;
			AdjustAnglesForBarrel( angles, direction.Length() );
		}
		else
			angles.x = -angles.x;
	}
	else
	{
		if ( IsActive() )
			pev->nextthink = pev->ltime + 0.1;
		else
		{
			pev->nextthink = 0;
			return;
		}
		UpdateSpot();
		pTarget = BestVisibleEnemy();
		if ( !pTarget || FNullEnt( pTarget->edict() ) )
		{
			if ( IsActive() )
				pev->nextthink = pev->ltime + 0.2;	// Wait 2 secs
			return;
		}

		// Calculate angle needed to aim at target
		barrelEnd = BarrelPosition();
		targetPosition = pTarget->pev->origin + pTarget->pev->view_ofs;
		float range = (targetPosition - barrelEnd).Length();
		
		if ( !InRange( range ) )
			return;

		UTIL_TraceLine( barrelEnd, targetPosition, dont_ignore_monsters, edict(), &tr );
		
		// No line of sight, don't track
		if ( tr.flFraction == 1.0 || tr.pHit == pTarget->edict() )
		{

			CBaseEntity *pInstance = pTarget;
			if ( InRange( range ) && pInstance && pInstance->IsAlive() )
			{
				updateTime = TRUE;
				m_sightOrigin = UpdateTargetPosition( pInstance );
			}
		}

		// Track sight origin

// !!! I'm not sure what i changed
		direction = m_sightOrigin - pev->origin;
//		direction = m_sightOrigin - barrelEnd;
		angles = UTIL_VecToAngles( direction );

		// Calculate the additional rotation to point the end of the barrel at the target (not the gun's center) 
		AdjustAnglesForBarrel( angles, direction.Length() );
	}

	angles.x = -angles.x;

	// Force the angles to be relative to the center position
	angles.y = m_yawCenter + UTIL_AngleDistance( angles.y, m_yawCenter );
	angles.x = m_pitchCenter + UTIL_AngleDistance( angles.x, m_pitchCenter );

	// Limit against range in y
	if ( m_yawRange < 360 && angles.y > m_yawCenter + m_yawRange )
	{
		angles.y = m_yawCenter + m_yawRange;
		updateTime = FALSE;	// Don't update if you saw the player, but out of range
	}
	else if ( m_yawRange < 360 && angles.y < (m_yawCenter - m_yawRange) )
	{
		angles.y = (m_yawCenter - m_yawRange);
		updateTime = FALSE; // Don't update if you saw the player, but out of range
	}

	if ( updateTime )
		m_lastSightTime = gpGlobals->time;

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( angles.y, pev->angles.y );
	pev->avelocity.y = distY * 10;
	if ( pev->avelocity.y > m_yawRate )
		pev->avelocity.y = m_yawRate;
	else if ( pev->avelocity.y < -m_yawRate )
		pev->avelocity.y = -m_yawRate;

	// Limit against range in x
	if ( angles.x > m_pitchCenter + m_pitchRange )
		angles.x = m_pitchCenter + m_pitchRange;
	else if ( angles.x < m_pitchCenter - m_pitchRange )
		angles.x = m_pitchCenter - m_pitchRange;

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( angles.x, pev->angles.x );
	pev->avelocity.x = distX  * 10;

	if ( pev->avelocity.x > m_pitchRate )
		pev->avelocity.x = m_pitchRate;
	else if ( pev->avelocity.x < -m_pitchRate )
		pev->avelocity.x = -m_pitchRate;

	if ( pController )
	{
		if ( m_flNextAttack <= gpGlobals->time && (pController->pev->button & IN_ATTACK) )
		{
			Vector vecForward;
			UTIL_MakeVectorsPrivate( pev->angles, vecForward, NULL, NULL );
			m_fireLast = gpGlobals->time - 1.0 / m_fireRate - 0.01;
			Fire( BarrelPosition(), vecForward, pController->pev );
			if ( pController->IsPlayer() )
				pController->m_iWeaponVolume = LOUD_GUN_VOLUME;
			m_flNextAttack = gpGlobals->time + 1.0 / m_fireRate;
		}
		return;
	}

	if ( CanFire() && ( (fabs(distX) < m_pitchTolerance && fabs(distY) < m_yawTolerance) || (pev->spawnflags & SF_TANK_LINEOFSIGHT) ) )
	{
		BOOL fire = FALSE;
		Vector forward;
		UTIL_MakeVectorsPrivate( pev->angles, forward, NULL, NULL );

		if ( pev->spawnflags & SF_TANK_LINEOFSIGHT )
		{
			float length = direction.Length();
			UTIL_TraceLine( barrelEnd, barrelEnd + forward * length, dont_ignore_monsters, edict(), &tr );
			if ( tr.pHit == pTarget->edict() )
				fire = TRUE;
		}
		else
			fire = TRUE;

		if ( fire )
		{
			Fire( BarrelPosition(), forward, pev );
		}
		else
			m_fireLast = 0;
	}
	else
		m_fireLast = 0;
}


// If barrel is offset, add in additional rotation
void CFuncTank::AdjustAnglesForBarrel( Vector &angles, float distance )
{
	float r2, d2;


	if ( m_barrelPos.y != 0 || m_barrelPos.z != 0 )
	{
		distance -= m_barrelPos.z;
		d2 = distance * distance;
		if ( m_barrelPos.y )
		{
			r2 = m_barrelPos.y * m_barrelPos.y;
			angles.y += (180.0 / M_PI) * atan2( m_barrelPos.y, sqrt( d2 - r2 ) );
		}
		if ( m_barrelPos.z )
		{
			r2 = m_barrelPos.z * m_barrelPos.z;
			angles.x += (180.0 / M_PI) * atan2( -m_barrelPos.z, sqrt( d2 - r2 ) );
		}
	}
}


// Fire targets and spawn sprites
void CFuncTank::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if ( m_fireLast != 0 )
	{
		if ( m_iszSpriteSmoke )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteSmoke), barrelEnd, TRUE );
			pSprite->AnimateAndDie( RANDOM_FLOAT( 15.0, 20.0 ) );
			pSprite->SetTransparency( kRenderTransAlpha, pev->rendercolor.x, pev->rendercolor.y, pev->rendercolor.z, 255, kRenderFxNone );
			pSprite->pev->velocity.z = RANDOM_FLOAT(40, 80);
			pSprite->SetScale( m_spriteScale );
		}
		if ( m_iszSpriteFlash )
		{
			CSprite *pSprite = CSprite::SpriteCreate( STRING(m_iszSpriteFlash), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 60 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );

			// Hack Hack, make it stick around for at least 100 ms.
			pSprite->pev->nextthink += 0.1;
		}
		SUB_UseTargets( this, USE_TOGGLE, 0 );
	}
	m_fireLast = gpGlobals->time;
}


void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, TraceResult &tr )
{
	// get circular gaussian spread
	float x, y, z;
	do {
		x = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		y = RANDOM_FLOAT(-0.5,0.5) + RANDOM_FLOAT(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);
	Vector vecDir = vecForward +
		x * vecSpread.x * gpGlobals->v_right +
		y * vecSpread.y * gpGlobals->v_up;
	Vector vecEnd;
	
	vecEnd = vecStart + vecDir * 4096;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
}

	
void CFuncTank::StartRotSound( void )
{
	if ( !pev->noise || (pev->spawnflags & SF_TANK_SOUNDON) )
		return;
	pev->spawnflags |= SF_TANK_SOUNDON;
	EMIT_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise), 0.85, ATTN_NORM);
}


void CFuncTank::StopRotSound( void )
{
	if ( pev->spawnflags & SF_TANK_SOUNDON )
		STOP_SOUND( edict(), CHAN_STATIC, (char*)STRING(pev->noise) );
	pev->spawnflags &= ~SF_TANK_SOUNDON;
}

class CFuncTankGun : public CFuncTank
{
public:
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};
LINK_ENTITY_TO_CLASS( func_tank, CFuncTankGun );

void CFuncTankGun::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if ( m_fireLast != 0 )
	{
		// FireBullets needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);

		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if ( bulletCount > 0 )
		{
			for ( i = 0; i < bulletCount; i++ )
			{
				switch( m_bulletType )
				{
				case TANK_BULLET_9MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_9MM, 1, m_iBulletDamage, pevAttacker );
					break;

				case TANK_BULLET_MP5:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_MP5, 1, m_iBulletDamage, pevAttacker );
					break;

				case TANK_BULLET_12MM:
					FireBullets( 1, barrelEnd, forward, gTankSpread[m_spread], 4096, BULLET_MONSTER_12MM, 1, m_iBulletDamage, pevAttacker );
					break;

				default:
				case TANK_BULLET_NONE:
					break;
				}
			}
			CFuncTank::Fire( barrelEnd, forward, pevAttacker );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pevAttacker );
}



class CFuncTankLaser : public CFuncTank
{
public:
	void	Activate( void );
	void	KeyValue( KeyValueData *pkvd );
	void	Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
	void	Think( void );
	CLaser *GetLaser( void );

	virtual int	Save( CSave &save );
	virtual int	Restore( CRestore &restore );
	static	TYPEDESCRIPTION m_SaveData[];

private:
	CLaser	*m_pLaser;
	float	m_laserTime;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
LINK_ENTITY_TO_CLASS( func_tanklaser, CFuncTankLaser );

TYPEDESCRIPTION	CFuncTankLaser::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTankLaser, m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankLaser, m_laserTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CFuncTankLaser, CFuncTank );

void CFuncTankLaser::Activate( void )
{
	if ( !GetLaser() )
	{
		UTIL_Remove(this);
		ALERT( at_error, "Laser tank with no env_laser!\n" );
	}
	else
	{
		m_pLaser->TurnOff();
	}
}


void CFuncTankLaser::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "laserentity"))
	{
		pev->message = ALLOC_STRING(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}


CLaser *CFuncTankLaser::GetLaser( void )
{
	if ( m_pLaser )
		return m_pLaser;

	edict_t	*pentLaser;

	pentLaser = FIND_ENTITY_BY_TARGETNAME( NULL, STRING(pev->message) );
	while ( !FNullEnt( pentLaser ) )
	{
		// Found the landmark
		if ( FClassnameIs( pentLaser, "env_laser" ) )
		{
			m_pLaser = (CLaser *)CBaseEntity::Instance(pentLaser);
			break;
		}
		else
			pentLaser = FIND_ENTITY_BY_TARGETNAME( pentLaser, STRING(pev->message) );
	}

	return m_pLaser;
}


void CFuncTankLaser::Think( void )
{
	if ( m_pLaser && (gpGlobals->time > m_laserTime) )
		m_pLaser->TurnOff();

	CBaseEntity::Think();
}


void CFuncTankLaser::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;
	TraceResult tr;

	if ( m_fireLast != 0 && GetLaser() )
	{
		// TankTrace needs gpGlobals->v_up, etc.
		UTIL_MakeAimVectors(pev->angles);

		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if ( bulletCount )
		{
			for ( i = 0; i < bulletCount; i++ )
			{
				m_pLaser->pev->origin = barrelEnd;
				TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );
				
				m_laserTime = gpGlobals->time;
				m_pLaser->TurnOn();
				m_pLaser->pev->dmgtime = gpGlobals->time - 1.0;
				m_pLaser->FireAtPoint( tr );
				m_pLaser->pev->nextthink = 0;
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
	{
		CFuncTank::Fire( barrelEnd, forward, pev );
	}
}

class CFuncTankRocket : public CFuncTank
{
public:
	void Precache( void );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};
LINK_ENTITY_TO_CLASS( func_tankrocket, CFuncTankRocket );

void CFuncTankRocket::Precache( void )
{
	UTIL_PrecacheOther( "rpg_rocket" );
	CFuncTank::Precache();
}



void CFuncTankRocket::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	int i;

	if ( m_fireLast != 0 )
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		if ( bulletCount > 0 )
		{
			for ( i = 0; i < bulletCount; i++ )
			{
				CBaseEntity::Create( "rpg_rocket", barrelEnd, pev->angles, edict() );
			}
			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}


class CFuncTankMortar : public CFuncTank
{
public:
	void KeyValue( KeyValueData *pkvd );
	void Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker );
};
LINK_ENTITY_TO_CLASS( func_tankmortar, CFuncTankMortar );


void CFuncTankMortar::KeyValue( KeyValueData *pkvd )
{
	if (FStrEq(pkvd->szKeyName, "iMagnitude"))
	{
		pev->impulse = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CFuncTank::KeyValue( pkvd );
}


void CFuncTankMortar::Fire( const Vector &barrelEnd, const Vector &forward, entvars_t *pevAttacker )
{
	if ( m_fireLast != 0 )
	{
		int bulletCount = (gpGlobals->time - m_fireLast) * m_fireRate;
		// Only create 1 explosion
		if ( bulletCount > 0 )
		{
			TraceResult tr;

			// TankTrace needs gpGlobals->v_up, etc.
			UTIL_MakeAimVectors(pev->angles);

			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );

			ExplosionCreate( tr.vecEndPos, pev->angles, edict(), pev->impulse, TRUE );

			CFuncTank::Fire( barrelEnd, forward, pev );
		}
	}
	else
		CFuncTank::Fire( barrelEnd, forward, pev );
}



//============================================================================
// FUNC TANK CONTROLS
//============================================================================
LINK_ENTITY_TO_CLASS( func_tankcontrols, CFuncTankControls );

TYPEDESCRIPTION	CFuncTankControls::m_SaveData[] = 
{
	DEFINE_FIELD( CFuncTankControls, m_active, FIELD_BOOLEAN ),
	DEFINE_FIELD( CFuncTankControls, m_pController, FIELD_CLASSPTR ),
	DEFINE_FIELD( CFuncTankControls, m_vecControllerUsePos, FIELD_VECTOR ),
};

IMPLEMENT_SAVERESTORE( CFuncTankControls, CBaseEntity );

int	CFuncTankControls :: ObjectCaps( void ) 
{ 
	return (CBaseEntity::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_IMPULSE_USE; 
}


void CFuncTankControls :: Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{ // pass the Use command onto the controls
	if ( useType != USE_OFF && !m_pController )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pActivator;
		if ( m_active || pPlayer->m_pTank )
			return;
		CBaseEntity* pEntity = NULL;
		while ( (pEntity = UTIL_FindEntityByClassname( pEntity, "func_tank" )) != NULL )
		{
			if ( FStrEq( STRING( pEntity->pev->targetname ), STRING( pev->target ) ) &&
				((CFuncTank*)pEntity)->StartControl( pPlayer, this ) )
				m_active = TRUE;
		}
		if ( m_active )
		{
			m_pController = pPlayer;
			pPlayer->m_pTank = this;
			if ( pPlayer->m_pActiveItem )
			{
				pPlayer->m_pActiveItem->Holster( 0 );
				pPlayer->pev->weaponmodel = 0;
			}
			pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS;
			m_vecControllerUsePos = pPlayer->pev->origin;
		}
	}
	else
	{
		CBaseEntity* pEntity = NULL;
		while ( (pEntity = UTIL_FindEntityByClassname( pEntity, "func_tank" )) != NULL )
		{
			if ( FStrEq( STRING( pEntity->pev->targetname ), STRING( pev->target ) ) )
				((CFuncTank*)pEntity)->StopControl( this );
		}
		if ( m_pController )
		{
			if ( m_pController->m_pActiveItem )
				m_pController->m_pActiveItem->Deploy();
			m_pController->m_iHideHUD &= ~HIDEHUD_WEAPONS;
			m_pController = NULL;
			m_active = FALSE;
		}
	}
}

BOOL CFuncTankControls::OnControls( entvars_t* pevTest )
{
	return (pev->spawnflags & SF_TANK_CANCONTROL) && (m_vecControllerUsePos - pevTest->origin).Length() < 30;
}

void CFuncTankControls::Spawn( void )
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( ENT(pev), STRING(pev->model) );

	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( this, pev->origin );
}
