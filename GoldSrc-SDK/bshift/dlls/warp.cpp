#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "effects.h"
#include "weapons.h"

#define SF_WARPBALL_ONCE		0x0001
#define SF_WARPBALL_DAMAGE	0x0002

class CWarpBall : public CBaseEntity
{
public:
	virtual void Spawn();
	virtual void Precache();
	virtual void KeyValue( KeyValueData* pkvd );
	virtual int Save( CSave& save );
	virtual int Restore( CRestore& restore );
	virtual int Classify();

	void EXPORT BallThink();
	void EXPORT WarpBallUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );

	static CWarpBall* CreateWarpBall( Vector vecOrigin );
	static TYPEDESCRIPTION m_SaveData[];

	CLightning*	m_pBeams;
	CSprite*	m_pSprite;
	int			m_iBeams;
	float		m_flLastTime;
	float		m_flMaxFrame;
	float		m_flBeamRadius;
	int			m_iszWarpTarget;
	float		m_flWarpStart;
	float		m_flDamageDelay;
	float		m_flTargetDelay;
	BOOL		m_fPlaying;
	BOOL		m_fDamageApplied;
	BOOL		m_fBeamsCleared;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( env_warpball, CWarpBall );

TYPEDESCRIPTION CWarpBall::m_SaveData[] =
{
	DEFINE_FIELD( CWarpBall, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( CWarpBall, m_flLastTime, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flMaxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flBeamRadius, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_iszWarpTarget, FIELD_STRING ),
	DEFINE_FIELD( CWarpBall, m_flWarpStart, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flDamageDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_flTargetDelay, FIELD_FLOAT ),
	DEFINE_FIELD( CWarpBall, m_fPlaying, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fDamageApplied, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_fBeamsCleared, FIELD_BOOLEAN ),
	DEFINE_FIELD( CWarpBall, m_pBeams, FIELD_CLASSPTR ),
	DEFINE_FIELD( CWarpBall, m_pSprite, FIELD_CLASSPTR ),
};

IMPLEMENT_SAVERESTORE( CWarpBall, CBaseEntity );

void CWarpBall::Precache()
{
	PRECACHE_MODEL( "sprites/Fexplo1.spr" );
	PRECACHE_MODEL( "sprites/XFlare1.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_SOUND( "debris/alien_teleport.wav" );
}

int CWarpBall::Classify()
{
	return CLASS_NONE;
}

void CWarpBall::BallThink()
{
	pev->frame = ( gpGlobals->time - m_flLastTime ) * pev->framerate + pev->frame;
	if ( pev->frame > m_flMaxFrame )
	{
		SET_MODEL( edict(), "" );
		SetThink( NULL );
		if ( pev->spawnflags & SF_WARPBALL_ONCE )
			UTIL_Remove( this );
		if ( m_pSprite != NULL )
			m_pSprite->TurnOff();
		m_fPlaying = FALSE;
		return;
	}

	if ( ( pev->spawnflags & SF_WARPBALL_DAMAGE ) && !m_fDamageApplied &&
		gpGlobals->time - m_flWarpStart >= m_flDamageDelay )
	{
		RadiusDamage( pev->origin, pev, pev, 300.0, 48.0, CLASS_NONE, DMG_SHOCK );
		m_fDamageApplied = TRUE;
	}

	if ( m_pBeams != NULL && pev->frame >= m_flMaxFrame - 4.0 )
	{
		m_pBeams->SetThink( NULL );
		m_pBeams->pev->nextthink = gpGlobals->time;
	}

	pev->nextthink = gpGlobals->time + 0.1;
	m_flLastTime = gpGlobals->time;
}

void CWarpBall::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->rendermode = kRenderGlow;
	pev->renderamt = 255;
	pev->renderfx = kRenderFxNoDissipation;
	pev->framerate = 10;
	m_pSprite = CSprite::SpriteCreate( "sprites/Fexplo1.spr", pev->origin, TRUE );
	m_pSprite->TurnOff();
	SetUse( &CWarpBall::WarpBallUse );
}

void CWarpBall::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "radius" ) )
	{
		m_flBeamRadius = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "warp_target" ) )
	{
		m_iszWarpTarget = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "damage_delay" ) )
	{
		m_flDamageDelay = (float)atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		pkvd->fHandled = FALSE;
	}
}

void CWarpBall::WarpBallUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( m_fPlaying )
		return;

	if ( m_iszWarpTarget != 0 )
	{
		edict_t* pTarget = FIND_ENTITY_BY_TARGETNAME( NULL, STRING( m_iszWarpTarget ) );
		if ( pTarget != NULL )
			UTIL_SetOrigin( pev, VARS( pTarget )->origin );
	}

	SET_MODEL( edict(), "sprites/XFlare1.spr" );
	m_flMaxFrame = MODEL_FRAMES( pev->modelindex ) - 1.0;
	pev->rendercolor.x = 77;
	pev->rendercolor.y = 210;
	pev->rendercolor.z = 130;
	pev->scale = 1.2f;
	pev->frame = 0;

	if ( m_pSprite != NULL )
	{
		m_pSprite->SetTransparency( kRenderGlow, 77, 210, 130, 255, kRenderFxNoDissipation );
		m_pSprite->pev->scale = 1;
		m_pSprite->pev->framerate = 10;
		m_pSprite->TurnOn();
	}

	if ( m_pBeams == NULL )
	{
		m_pBeams = CLightning::LightningCreate( "sprites/lgtning.spr", 18 );
		if ( m_pBeams != NULL )
		{
			m_pBeams->m_iszSpriteName = (int)MAKE_STRING( "sprites/lgtning.spr" );
			m_pBeams->pev->origin = pev->origin;
			UTIL_SetOrigin( m_pBeams->pev, pev->origin );
			m_pBeams->m_restrike = -0.5;
			m_pBeams->m_noiseAmplitude = 65;
			m_pBeams->m_boltWidth = 18;
			m_pBeams->m_life = 0.5;

			m_pBeams->pev->rendercolor.x = 0;
			m_pBeams->pev->rendercolor.y = 255;
			m_pBeams->pev->rendercolor.z = 0;
			m_pBeams->pev->spawnflags |= SF_BEAM_SPARKEND;
			m_pBeams->pev->spawnflags |= SF_BEAM_TOGGLE;

			m_pBeams->m_radius = m_flBeamRadius;
			m_pBeams->m_iszStartEntity = pev->targetname;
			m_pBeams->BeamUpdateVars();
		}
	}

	if ( m_pBeams != NULL )
	{
		m_pBeams->pev->solid = SOLID_NOT;
		m_pBeams->Precache();
		m_pBeams->SetThink( &CLightning::StrikeThink );
		m_pBeams->pev->nextthink = gpGlobals->time + 0.1;
	}

	SetThink( &CWarpBall::BallThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flLastTime = gpGlobals->time;
	m_fBeamsCleared = FALSE;
	m_fPlaying = TRUE;
	if ( m_flDamageDelay == 0 )
	{
		RadiusDamage( pev->origin, pev, pev, 300.0, 48.0, CLASS_NONE, DMG_SHOCK );
		m_fDamageApplied = TRUE;
	}
	else
	{
		m_fDamageApplied = FALSE;
	}

	SUB_UseTargets( this, USE_TOGGLE, 0 );
	UTIL_ScreenShake( pev->origin, 4.0, 100.0, 2.0, 1000.0 );
	m_flWarpStart = gpGlobals->time;
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "debris/alien_teleport.wav", 1.0, ATTN_NORM, 0, PITCH_NORM );
}

CWarpBall* CWarpBall::CreateWarpBall( Vector vecOrigin )
{
	CWarpBall* pBall = GetClassPtr( (CWarpBall*)NULL );
	UTIL_SetOrigin( pBall->pev, vecOrigin );
	pBall->pev->classname = (string_t)MAKE_STRING( "env_warpball" );
	pBall->Spawn();
	return pBall;
}
