#include "extdll.h"
#include "util.h"
#include "triggers.h"
#include "monsters.h"
#include "animation.h"
#include "effects.h"
#include "weapons.h"
#include "gamerules.h"

#define SF_TRIGGER_HURT_TARGETONCE		1
#define SF_TRIGGER_HURT_START_OFF		2
#define SF_TRIGGER_HURT_NO_CLIENTS		8
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH	32

int iGeneWormSpitSprite;

class COFGeneWormSpawn : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT GeneWormSpawnTouch( CBaseEntity* pOther );
	void EXPORT GeneWormSpawnThink();
	void RunGeneWormSpawn( float frames );
	void CreateWarpBeams( int side );
	void TurnOn();
	void LaunchSpawn( const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float flBirthTime );
	static COFGeneWormSpawn* GeneWormSpawnCreate( const Vector& origin );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_lastTime;
	float	m_maxFrame;
	float	m_flBirthTime;
	float	m_flWarpTime;
	BOOL	m_bLaunched;
	BOOL	m_bWarping;
	BOOL	m_bTrooperDropped;
	CBeam*	m_pBeam[8];
	int		m_iBeams;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFGeneWormCloud : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	void EXPORT GeneWormCloudTouch( CBaseEntity* pOther );
	void EXPORT GeneWormCloudThink();
	void RunGeneWormCloud( float frames );
	void TurnOn();
	void LaunchCloud( const Vector& origin, const Vector& aim, int nSpeed, edict_t* pOwner, float fadeTime );
	static COFGeneWormCloud* GeneWormCloudCreate( const Vector& origin );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_lastTime;
	float	m_maxFrame;
	BOOL	m_bLaunched;
	float	m_fadeScale;
	float	m_fadeRender;
	float	m_damageTimer;
	BOOL	m_fSinking;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFTriggerGeneWormHit : public CBaseTrigger
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT GeneWormHitTouch( CBaseEntity* pOther );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pAttackSounds[];
	float m_flLastDamageTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFGeneWorm : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Classify() override;
	int BloodColor() override;
	void GibMonster() override;
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
		TraceResult* ptr, int bitsDamageType ) override;
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	BOOL FVisible( CBaseEntity* pEntity ) override;
	BOOL FVisible( const Vector& vecOrigin ) override;
	void SetObjectCollisionBox() override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	void EXPORT StartupThink();
	void EXPORT HuntThink();
	void EXPORT NullThink();
	void EXPORT DyingThink();
	void EXPORT HitTouch( CBaseEntity* pOther );
	void EXPORT CommandUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	BOOL ClawAttack();
	void SpewCloud();
	void TrackHead();
	void NextActivity();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pIdleSounds[];
	static const char* pHitGroundSounds[];
	static const char* pPainSounds[];
	static const char* pSpawnSounds[];
	static const char* pShootSounds[];

	float				m_flNextPainSound;
	Vector				m_posTarget;
	float				m_flLastSeen;
	float				m_flPrevSeen;
	COFGeneWormCloud*	m_pCloud;
	int					m_iWasHit;
	float				m_flTakeHitTime;
	float				m_flHitTime;
	float				m_flNextMeleeTime;
	float				m_flNextRangeTime;
	BOOL				m_fRightEyeHit;
	BOOL				m_fLeftEyeHit;
	BOOL				m_fGetMad;
	BOOL				m_fOrificeHit;
	float				m_flOrificeOpenTime;
	COFGeneWormSpawn*	m_orificeGlow;
	BOOL				m_fSpawningTrooper;
	float				m_flSpawnTrooperTime;
	int					m_iHitTimes;
	int					m_iMaxHitTimes;
	float				m_offsetBeam;
	float				m_lenBeam;
	Vector				m_posBeam;
	Vector				m_vecBeam;
	Vector				m_angleBeam;
	float				m_flBeamExpireTime;
	float				m_flBeamDir;
	BOOL				m_fSpitting;
	float				m_flSpitStartTime;
	BOOL				m_fActivated;
	float				m_flDeathStart;
	BOOL				m_fHasEntered;
	float				m_flMadDelayTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( env_genewormspawn, COFGeneWormSpawn );
LINK_ENTITY_TO_CLASS( env_genewormcloud, COFGeneWormCloud );
LINK_ENTITY_TO_CLASS( trigger_geneworm_hit, COFTriggerGeneWormHit );
LINK_ENTITY_TO_CLASS( monster_geneworm, COFGeneWorm );

TYPEDESCRIPTION COFGeneWormSpawn::m_SaveData[] =
{
	DEFINE_FIELD( COFGeneWormSpawn, m_lastTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormSpawn, m_maxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormSpawn, m_flBirthTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormSpawn, m_flWarpTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormSpawn, m_bLaunched, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWormSpawn, m_bWarping, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWormSpawn, m_bTrooperDropped, FIELD_BOOLEAN ),
	DEFINE_ARRAY( COFGeneWormSpawn, m_pBeam, FIELD_CLASSPTR, 8 ),
	DEFINE_FIELD( COFGeneWormSpawn, m_iBeams, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( COFGeneWormSpawn, CBaseEntity );

TYPEDESCRIPTION COFGeneWormCloud::m_SaveData[] =
{
	DEFINE_FIELD( COFGeneWormCloud, m_lastTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormCloud, m_maxFrame, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormCloud, m_bLaunched, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWormCloud, m_fadeScale, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormCloud, m_fadeRender, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormCloud, m_damageTimer, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWormCloud, m_fSinking, FIELD_BOOLEAN )
};

IMPLEMENT_SAVERESTORE( COFGeneWormCloud, CBaseEntity );

TYPEDESCRIPTION COFTriggerGeneWormHit::m_SaveData[] =
{
	DEFINE_FIELD( COFTriggerGeneWormHit, m_flLastDamageTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( COFTriggerGeneWormHit, CBaseTrigger );

TYPEDESCRIPTION COFGeneWorm::m_SaveData[] =
{
	DEFINE_FIELD( COFGeneWorm, m_flNextPainSound, FIELD_TIME ),
	DEFINE_FIELD( COFGeneWorm, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COFGeneWorm, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( COFGeneWorm, m_flPrevSeen, FIELD_TIME ),
	DEFINE_FIELD( COFGeneWorm, m_pCloud, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFGeneWorm, m_iWasHit, FIELD_INTEGER ),
	DEFINE_FIELD( COFGeneWorm, m_flTakeHitTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_flHitTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_flNextMeleeTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_flNextRangeTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_fRightEyeHit, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_fLeftEyeHit, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_fOrificeHit, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_flOrificeOpenTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_orificeGlow, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFGeneWorm, m_fSpawningTrooper, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_flSpawnTrooperTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFGeneWorm, m_iHitTimes, FIELD_INTEGER ),
	DEFINE_FIELD( COFGeneWorm, m_iMaxHitTimes, FIELD_INTEGER ),
	DEFINE_FIELD( COFGeneWorm, m_fSpitting, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_flSpitStartTime, FIELD_TIME ),
	DEFINE_FIELD( COFGeneWorm, m_fActivated, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_flDeathStart, FIELD_TIME ),
	DEFINE_FIELD( COFGeneWorm, m_fHasEntered, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFGeneWorm, m_flMadDelayTime, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( COFGeneWorm, CBaseMonster );

const char* COFGeneWorm::pIdleSounds[] =
{
	"geneworm/geneworm_idle1.wav", "geneworm/geneworm_idle2.wav",
	"geneworm/geneworm_idle3.wav", "geneworm/geneworm_idle4.wav"
};
const char* COFGeneWorm::pHitGroundSounds[] = { "tentacle/te_strike1.wav", "tentacle/te_strike2.wav" };
const char* COFGeneWorm::pPainSounds[] = { "X/x_pain1.wav", "X/x_pain2.wav" };
const char* COFGeneWorm::pSpawnSounds[] = { "debris/beamstart7.wav" };
const char* COFGeneWorm::pShootSounds[] = { "bullchicken/bc_idle2.wav", "bullchicken/bc_acid1.wav" };
const char* COFTriggerGeneWormHit::pAttackSounds[] =
{
	"zombie/claw_strike1.wav", "zombie/claw_strike2.wav", "zombie/claw_strike3.wav"
};

void COFGeneWormSpawn::Precache()
{
	PRECACHE_MODEL( "sprites/tele1.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/boss_glow.spr" );
}

void COFGeneWormSpawn::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = 0;
	pev->frame = 0;
	Precache();
	SET_MODEL( ENT( pev ), "sprites/boss_glow.spr" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;
	if ( pev->angles.y != 0 && pev->angles.z == 0 )
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
	m_bLaunched = FALSE;
	m_bWarping = FALSE;
	m_bTrooperDropped = FALSE;
	m_flBirthTime = gpGlobals->time;
	m_flWarpTime = gpGlobals->time;
	memset( m_pBeam, 0, sizeof( m_pBeam ) );
	m_iBeams = 0;
}

void COFGeneWormSpawn::GeneWormSpawnTouch( CBaseEntity* )
{
}

void COFGeneWormSpawn::TurnOn()
{
	pev->effects = 0;
	if ( ( pev->framerate != 0 && m_maxFrame > 1 ) || ( pev->spawnflags & SF_SPRITE_ONCE ) )
	{
		SetThink( &COFGeneWormSpawn::GeneWormSpawnThink );
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}

COFGeneWormSpawn* COFGeneWormSpawn::GeneWormSpawnCreate( const Vector& origin )
{
	COFGeneWormSpawn* pSpawn = GetClassPtr( (COFGeneWormSpawn*)NULL );
	pSpawn->Spawn();
	pSpawn->pev->origin = origin;
	pSpawn->pev->classname = MAKE_STRING( "env_genewormspawn" );
	pSpawn->pev->solid = SOLID_NOT;
	pSpawn->pev->movetype = MOVETYPE_NONE;
	pSpawn->pev->effects = 0;
	pSpawn->TurnOn();
	return pSpawn;
}

void COFGeneWormSpawn::LaunchSpawn( const Vector& origin, const Vector& aim, int nSpeed,
		edict_t* pOwner, float flBirthTime )
{
	pev->movetype = MOVETYPE_FLY;
	pev->angles = pOwner->v.angles;
	pev->owner = pOwner;
	pev->velocity = aim * (float)nSpeed;
	pev->speed = (float)nSpeed;
	pev->skin = 0;
	pev->body = 0;
	pev->aiment = NULL;
	pev->movetype = MOVETYPE_FLY;
	SET_MODEL( ENT( pev ), "sprites/tele1.spr" );
	m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;
	pev->frame = 0;
	UTIL_SetOrigin( pev, origin );
	SetTouch( &COFGeneWormSpawn::GeneWormSpawnTouch );
	m_bLaunched = TRUE;
	m_flBirthTime = flBirthTime + gpGlobals->time;
}

void COFGeneWormSpawn::CreateWarpBeams( int side )
{
	if ( m_iBeams > 7 )
		m_iBeams = 0;
	UTIL_MakeAimVectors( pev->angles );
	Vector source = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
	TraceResult best;
	best.flFraction = 1;
	for ( int i = 0; i < 3; i++ )
	{
		Vector direction = gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		direction = gpGlobals->v_right * (float)side * RANDOM_FLOAT( 0, 1 ) + direction;
		TraceResult trace;
		UTIL_TraceLine( source, direction * 512 + source, dont_ignore_monsters, ENT( pev ), &trace );
		if ( best.flFraction > trace.flFraction )
			best = trace;
	}
	if ( best.flFraction == 1 )
		return;
	if ( m_pBeam[m_iBeams] == NULL )
		m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	CBeam* pBeam = m_pBeam[m_iBeams];
	if ( pBeam == NULL )
		return;
	CBaseEntity* pHit = CBaseEntity::Instance( best.pHit );
	if ( pHit && pHit->pev->takedamage )
	{
		pBeam->EntsInit( pHit->entindex(), entindex() );
		pBeam->SetColor( 255, 255, 0 );
		pBeam->SetBrightness( 255 );
		pBeam->SetNoise( 20 );
	}
	else
	{
		pBeam->PointEntInit( best.vecEndPos, entindex() );
		pBeam->SetColor( 255, 255, 0 );
		pBeam->SetBrightness( 192 );
		pBeam->SetNoise( 80 );
	}
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + 1;
	m_iBeams++;
}

void COFGeneWormSpawn::RunGeneWormSpawn( float frames )
{
	if ( m_bLaunched )
	{
		if ( gpGlobals->time > m_flBirthTime )
		{
			if ( m_bWarping )
			{
				if ( m_bTrooperDropped )
				{
					if ( pev->scale < 0.1 )
					{
						m_bLaunched = FALSE;
						m_bWarping = FALSE;
						m_bTrooperDropped = FALSE;
						UTIL_Remove( this );
						return;
					}
					pev->scale = (float)( pev->scale - 0.075 );
				}
				else if ( m_flWarpTime - 1 < gpGlobals->time )
				{
					UTIL_MakeVectors( pev->angles );
					TraceResult trace;
					UTIL_TraceLine( pev->origin + Vector( 0, 0, 1000 ), pev->origin + Vector( 0, 0, -1000 ),
						ignore_monsters, dont_ignore_glass, ENT( pev ), &trace );
					m_bTrooperDropped = TRUE;
					CBaseEntity::Create( "monster_shocktrooper", trace.vecEndPos + Vector( 0, 0, 48 ), pev->angles, NULL );
				}
				else
				{
					RadiusDamage( pev->origin, pev, pev, 1000, 128, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
					CreateWarpBeams( 1 );
					CreateWarpBeams( -1 );
				}
			}
			else
			{
				pev->velocity = g_vecZero;
				m_bWarping = TRUE;
				m_flWarpTime = gpGlobals->time + 3;
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "debris/beamstart2.wav", 1, ATTN_NORM, 0, 100 );
			}
		}
		else
		{
			pev->scale = (float)( pev->scale + 0.1 );
		}
	}
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( 96 );
		WRITE_BYTE( 207 );
		WRITE_BYTE( m_bLaunched ? 214 : 0 );
		WRITE_BYTE( m_bLaunched ? 0 : 214 );
		WRITE_BYTE( 1 );
		WRITE_COORD( 1 );
	MESSAGE_END();
	pev->frame += frames;
	if ( pev->frame > m_maxFrame && m_maxFrame > 0 )
		pev->frame = (float)fmod( (double)pev->frame, (double)m_maxFrame );
}

void COFGeneWormSpawn::GeneWormSpawnThink()
{
	RunGeneWormSpawn( ( gpGlobals->time - m_lastTime ) * pev->framerate );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	m_lastTime = gpGlobals->time;
}

void COFGeneWormCloud::Precache()
{
	PRECACHE_MODEL( "sprites/ballsmoke.spr" );
}

int COFGeneWormCloud::Classify()
{
	return CLASS_NONE;
}

void COFGeneWormCloud::Spawn()
{
	Precache();
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_FLY;
	pev->effects = 0;
	pev->frame = 0;
	SET_MODEL( ENT( pev ), "sprites/ballsmoke.spr" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_maxFrame = (float)MODEL_FRAMES( pev->modelindex ) - 1;
	if ( pev->angles.y != 0 && pev->angles.z == 0 )
	{
		pev->angles.z = pev->angles.y;
		pev->angles.y = 0;
	}
	pev->scale = 0.01f;
	m_fadeScale = 0;
	m_bLaunched = FALSE;
	m_damageTimer = gpGlobals->time;
	m_fSinking = FALSE;
}

void COFGeneWormCloud::TurnOn()
{
	pev->effects = 0;
	if ( ( pev->framerate != 0 && m_maxFrame > 1 ) || ( pev->spawnflags & SF_SPRITE_ONCE ) )
	{
		SetThink( &COFGeneWormCloud::GeneWormCloudThink );
		pev->nextthink = gpGlobals->time;
		m_lastTime = gpGlobals->time;
	}
	pev->frame = 0;
}

COFGeneWormCloud* COFGeneWormCloud::GeneWormCloudCreate( const Vector& origin )
{
	COFGeneWormCloud* pCloud = GetClassPtr( (COFGeneWormCloud*)NULL );
	pCloud->Spawn();
	pCloud->pev->origin = origin;
	pCloud->pev->classname = MAKE_STRING( "env_genewormcloud" );
	pCloud->pev->solid = SOLID_BBOX;
	pCloud->pev->movetype = MOVETYPE_FLY;
	pCloud->pev->effects = 0;
	pCloud->TurnOn();
	pCloud->SetTouch( &COFGeneWormCloud::GeneWormCloudTouch );
	return pCloud;
}

void COFGeneWormCloud::LaunchCloud( const Vector& origin, const Vector& aim, int nSpeed,
		edict_t* pOwner, float fadeTime )
{
	pev->angles = pOwner->v.angles;
	pev->owner = pOwner;
	pev->velocity = aim * (float)nSpeed;
	m_fadeScale = 2.5f / fadeTime;
	m_fadeRender = ( pev->renderamt - 7 ) / fadeTime;
	pev->skin = 0;
	pev->body = 0;
	pev->aiment = NULL;
	pev->movetype = MOVETYPE_FLY;
	UTIL_SetOrigin( pev, origin );
	SetTouch( &COFGeneWormCloud::GeneWormCloudTouch );
	m_bLaunched = TRUE;
}

void COFGeneWormCloud::GeneWormCloudTouch( CBaseEntity* pOther )
{
	if ( ( pev->owner == NULL || pOther->pev->modelindex != pev->owner->v.modelindex )
		&& pev->modelindex != pOther->pev->modelindex )
	{
		if ( pOther->pev->takedamage )
			pOther->TakeDamage( pev, pev, gSkillData.geneWormDmgSpit, DMG_ACID );
		pev->nextthink = gpGlobals->time;
		SetThink( NULL );
		UTIL_Remove( this );
	}
}

void COFGeneWormCloud::RunGeneWormCloud( float frames )
{
	if ( m_bLaunched )
	{
		pev->scale += m_fadeScale;
		pev->renderamt -= m_fadeRender;
		if ( pev->scale >= 4.5 )
		{
			pev->scale = 0;
			UTIL_Remove( this );
			return;
		}
	}
	else if ( pev->scale < 2 )
	{
		pev->scale = (float)( pev->scale + 0.05 );
	}
	pev->frame += frames;
	if ( pev->frame > m_maxFrame && m_maxFrame > 0 )
		pev->frame = (float)fmod( (double)pev->frame, (double)m_maxFrame );
}

void COFGeneWormCloud::GeneWormCloudThink()
{
	RunGeneWormCloud( ( gpGlobals->time - m_lastTime ) * pev->framerate );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	m_lastTime = gpGlobals->time;
}

void COFTriggerGeneWormHit::Precache()
{
	for ( int i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackSounds[i] );
}

void COFTriggerGeneWormHit::Spawn()
{
	Precache();
	InitTrigger();
	SetTouch( &COFTriggerGeneWormHit::GeneWormHitTouch );
	if ( !FStringNull( pev->targetname ) )
		SetUse( &COFTriggerGeneWormHit::ToggleUse );
	else
		SetUse( NULL );
	if ( pev->spawnflags & SF_TRIGGER_HURT_START_OFF )
		pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	pev->dmg = gSkillData.geneWormDmgHit;
	m_flLastDamageTime = gpGlobals->time;
}

void COFTriggerGeneWormHit::GeneWormHitTouch( CBaseEntity* pOther )
{
	if ( !( gpGlobals->time - m_flLastDamageTime >= 2 ) || !pOther->pev->takedamage )
		return;
	if ( ( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH ) && !pOther->IsPlayer() )
		return;
	if ( ( pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS ) && pOther->IsPlayer() )
		return;
	if ( g_pGameRules->IsMultiplayer() )
	{
		if ( pev->dmgtime <= gpGlobals->time )
		{
			pev->impulse = 0;
			if ( pOther->IsPlayer() )
				pev->impulse |= 1 << ( pOther->entindex() - 1 );
		}
		else if ( gpGlobals->time != pev->pain_finished )
		{
			if ( !pOther->IsPlayer() )
				return;
			int playerBit = 1 << ( pOther->entindex() - 1 );
			if ( pev->impulse & playerBit )
				return;
			pev->impulse |= playerBit;
		}
	}
	else if ( pev->dmgtime > gpGlobals->time && gpGlobals->time != pev->pain_finished )
	{
		return;
	}
	pOther->TakeDamage( pev, pev, pev->dmg, DMG_CRUSH );
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	EMIT_SOUND_DYN( pOther->edict(), CHAN_BODY, pAttackSounds[RANDOM_LONG( 0, 2 )], 1, 0.1f, 0, pitch );
	pev->pain_finished = gpGlobals->time;
	m_flLastDamageTime = gpGlobals->time;
	if ( !FStringNull( pev->target ) && ( !( pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE ) || pOther->IsPlayer() ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		if ( pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE )
			pev->target = 0;
	}
}

void FireHurtTargets( const char* targetName, CBaseEntity* pActivator, CBaseEntity* pCaller,
		USE_TYPE useType, float value )
{
	if ( targetName == NULL )
		return;
	ALERT( at_aiconsole, "Firing: (%s)\n", targetName );
	edict_t* pentTarget = NULL;
	for ( ;; )
	{
		pentTarget = FIND_ENTITY_BY_TARGETNAME( pentTarget, targetName );
		if ( FNullEnt( pentTarget ) )
			break;
		CBaseEntity* pTarget = CBaseEntity::Instance( pentTarget );
		if ( ( useType == USE_OFF && pTarget->pev->solid == SOLID_NOT )
			|| ( useType == USE_ON && pTarget->pev->solid == SOLID_TRIGGER )
			|| ( pTarget->pev->flags & FL_KILLME ) )
			continue;
		ALERT( at_aiconsole, "Found: %s, firing (%s)\n", STRING( pTarget->pev->classname ), targetName );
		pTarget->Use( pActivator, pCaller, useType, value );
	}
}

void COFGeneWorm::Precache()
{
	PRECACHE_MODEL( "models/geneworm.mdl" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/tele1.spr" );
	PRECACHE_MODEL( "sprites/bigspit.spr" );
	PRECACHE_MODEL( "sprites/boss_glow.spr" );
	iGeneWormSpitSprite = PRECACHE_MODEL( "sprites/tinyspit.spr" );
	PRECACHE_MODEL( "sprites/xbeam3.spr" );
	UTIL_PrecacheOther( "monster_shocktrooper" );
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
	for ( int i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char*)pIdleSounds[i] );
	PRECACHE_SOUND( (char*)pSpawnSounds[0] );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
	PRECACHE_SOUND( "debris/beamstart2.wav" );
	PRECACHE_MODEL( "sprites/xspark4.spr" );
	PRECACHE_MODEL( "sprites/ballsmoke.spr" );
	PRECACHE_SOUND( "geneworm/geneworm_attack_mounted_gun.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_attack_mounted_rocket.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_beam_attack.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_big_attack_forward.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_death.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_final_pain1.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_final_pain2.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_final_pain3.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_final_pain4.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_shot_in_eye.wav" );
	PRECACHE_SOUND( "geneworm/geneworm_entry.wav" );
}

void COFGeneWorm::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	SET_MODEL( ENT( pev ), "models/geneworm.mdl" );
	UTIL_SetSize( pev, Vector( -436.67f, -720.49f, -331.74f ), Vector( 425.29f, 164.85f, 355.68f ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->effects = 0;
	pev->health = gSkillData.geneWormHealth;
	pev->view_ofs = Vector( 0, 0, 300 );
	m_bloodColor = BLOOD_COLOR_YELLOW;
	m_flFieldOfView = 0.5f;
	SetThink( &COFGeneWorm::StartupThink );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	m_iWasHit = 0;
	m_fRightEyeHit = FALSE;
	m_fLeftEyeHit = FALSE;
	m_flTakeHitTime = 0;
	m_flHitTime = 0;
	m_fGetMad = FALSE;
	m_fOrificeHit = FALSE;
	m_flOrificeOpenTime = gpGlobals->time;
	m_flSpawnTrooperTime = gpGlobals->time;
	m_iHitTimes = 0;
	m_MonsterState = MONSTERSTATE_IDLE;
	m_pCloud = NULL;
	m_iMaxHitTimes = 4;
	pev->deadflag = DEAD_NO;
	m_fSpitting = FALSE;
	m_fActivated = FALSE;
	m_flSpitStartTime = gpGlobals->time;
	UTIL_SetOrigin( pev, pev->origin );
	pev->renderamt = 0;
	pev->rendermode = kRenderTransTexture;
	m_fHasEntered = FALSE;
	m_flMadDelayTime = gpGlobals->time;
}

int COFGeneWorm::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

int COFGeneWorm::BloodColor()
{
	return BLOOD_COLOR_YELLOW;
}

void COFGeneWorm::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -436.67f, -720.49f, -331.74f );
	pev->absmax = pev->origin + Vector( 425.29f, 164.85f, 355.68f );
}

void COFGeneWorm::GibMonster()
{
}

int COFGeneWorm::TakeDamage( entvars_t*, entvars_t*, float flDamage, int )
{
	if ( flDamage >= pev->health )
		pev->health = 1;
	else
		pev->health -= flDamage;
	return 1;
}

void COFGeneWorm::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
		TraceResult* ptr, int bitsDamageType )
{
	BOOL laser = !memcmp( STRING( pevAttacker->classname ), "env_laser", 9 );
	if ( ptr->iHitgroup < 4 || ptr->iHitgroup > 6 )
	{
		if ( pev->dmgtime != gpGlobals->time || RANDOM_LONG( 0, 10 ) <= 0 )
		{
			if ( laser )
				UTIL_Sparks( ptr->vecEndPos );
			else if ( bitsDamageType & DMG_BULLET )
				UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
		return;
	}
	if ( gpGlobals->time <= m_flTakeHitTime )
		return;
	if ( !laser )
	{
		if ( ptr->iHitgroup != 6 && ( gpGlobals->time != pev->dmgtime || RANDOM_LONG( 0, 10 ) <= 0 ) )
		{
			if ( bitsDamageType & DMG_BULLET )
				UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
	}
	else if ( !m_fHasEntered )
	{
		UTIL_Sparks( ptr->vecEndPos );
	}
	else if ( !( m_fLeftEyeHit && m_fRightEyeHit ) && !m_fGetMad )
	{
		Vector bloodPos = ptr->vecEndPos - vecDir * 4;
		if ( ptr->iHitgroup == 4 )
		{
			if ( !m_fLeftEyeHit && FStrEq( STRING( pevAttacker->targetname ), "left_eye_laser" ) )
			{
				m_fLeftEyeHit = TRUE;
				if ( !m_fRightEyeHit )
				{
					pev->skin = 1;
					m_fGetMad = TRUE;
				}
				else
				{
					pev->skin = 3;
					m_flOrificeOpenTime = gpGlobals->time + 20;
				}
				m_iWasHit = 1;
				if ( m_bloodColor != DONT_BLEED )
				{
					SpawnBlood( bloodPos, m_bloodColor, 300 );
					TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
				}
			}
			else
			{
				UTIL_Sparks( ptr->vecEndPos );
			}
		}
		else if ( ptr->iHitgroup == 5 )
		{
			if ( !m_fRightEyeHit && FStrEq( STRING( pevAttacker->targetname ), "right_eye_laser" ) )
			{
				m_fRightEyeHit = TRUE;
				if ( m_fLeftEyeHit )
				{
					pev->skin = 3;
					m_flOrificeOpenTime = gpGlobals->time + 20;
				}
				else
				{
					m_fGetMad = TRUE;
					pev->skin = 2;
				}
				m_iWasHit = 1;
				if ( m_bloodColor != DONT_BLEED )
				{
					SpawnBlood( bloodPos, m_bloodColor, 300 );
					TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
				}
			}
			else
			{
				UTIL_Sparks( ptr->vecEndPos );
			}
		}
	}
	if ( ptr->iHitgroup == 6 && m_flOrificeOpenTime > gpGlobals->time && !m_fOrificeHit )
	{
		pev->health -= flDamage;
		if ( pev->health <= 0 )
		{
			m_fOrificeHit = TRUE;
			UTIL_BloodDecalTrace( ptr, m_bloodColor );
			m_iHitTimes++;
			m_iWasHit = m_iHitTimes >= m_iMaxHitTimes ? 2 : 1;
			pev->health = gSkillData.geneWormHealth;
		}
	}
}

void COFGeneWorm::Killed( entvars_t* pevAttacker, int iGib )
{
	CBaseMonster::Killed( pevAttacker, iGib );
}

void COFGeneWorm::StartupThink()
{
	Vector eyePos, eyeAngles;
	GetAttachment( 0, eyePos, eyeAngles );
	pev->view_ofs = eyePos - pev->origin;
	m_flNextMeleeTime = gpGlobals->time;
	m_flNextRangeTime = gpGlobals->time;
	pev->sequence = LookupSequence( "entry" );
	pev->frame = 0;
	SetThink( &COFGeneWorm::HuntThink );
	SetUse( &COFGeneWorm::CommandUse );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	SetTouch( &COFGeneWorm::HitTouch );
}

BOOL COFGeneWorm::FVisible( const Vector& vecOrigin )
{
	Vector origin, angles;
	TraceResult trace;
	GetAttachment( 0, origin, angles );
	UTIL_TraceLine( origin, vecOrigin, ignore_monsters, ignore_glass, ENT( pev ), &trace );
	return trace.flFraction == 1;
}

BOOL COFGeneWorm::FVisible( CBaseEntity* pEntity )
{
	if ( pEntity->pev->flags & FL_NOTARGET )
		return FALSE;
	if ( pev->waterlevel == 3 )
	{
		if ( pEntity->pev->waterlevel == 0 )
			return FALSE;
	}
	else if ( pEntity->pev->waterlevel == 3 )
	{
		return FALSE;
	}
	Vector origin, angles;
	TraceResult trace;
	GetAttachment( 0, origin, angles );
	UTIL_TraceLine( origin, pEntity->EyePosition(), ignore_monsters, ignore_glass, ENT( pev ), &trace );
	return trace.flFraction == 1;
}

void COFGeneWorm::HitTouch( CBaseEntity* pOther )
{
	TraceResult trace = UTIL_GetGlobalTrace();
	if ( pOther->pev->modelindex == pev->modelindex || !( m_flHitTime <= gpGlobals->time )
		|| trace.pHit == NULL || trace.pHit->v.modelindex != pev->modelindex )
		return;
	m_flHitTime = gpGlobals->time + 0.5f;
	float damage;
	switch ( trace.iHitgroup )
	{
	case 1: damage = 10; break;
	case 2: damage = 15; break;
	case 3: damage = 20; break;
	default: damage = pOther->pev->health; break;
	}
	pOther->TakeDamage( pev, pev, damage, DMG_CRUSH | DMG_SLASH );
	pOther->pev->punchangle.z = 15;
	pOther->pev->velocity.z += 200;
	pOther->pev->flags &= ~FL_ONGROUND;
}

void COFGeneWorm::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 0:
		m_fSpitting = TRUE;
		m_flSpitStartTime = gpGlobals->time;
		break;
	case 2:
		if ( m_orificeGlow != NULL )
		{
			Vector origin, angles;
			GetAttachment( 1, origin, angles );
			UTIL_MakeVectors( pev->angles );
			int pitch = 100 + RANDOM_LONG( -5, 5 );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pSpawnSounds[RANDOM_LONG( 0, 0 )], 1, 0.1f, 0, pitch );
			int speed = 300 + RANDOM_LONG( 0, 50 );
			m_orificeGlow->LaunchSpawn( origin, gpGlobals->v_forward, speed, edict(), 2 );
			m_orificeGlow = NULL;
		}
		break;
	case 3:
		FireHurtTargets( "GeneWormLeftSlash", this, this, USE_ON, 0 );
		break;
	case 4:
		FireHurtTargets( "GeneWormLeftSlash", this, this, USE_OFF, 0 );
		break;
	case 5:
		FireHurtTargets( "GeneWormRightSlash", this, this, USE_ON, 0 );
		break;
	case 6:
		FireHurtTargets( "GeneWormRightSlash", this, this, USE_OFF, 0 );
		break;
	case 7:
		FireHurtTargets( "GeneWormCenterSlash", this, this, USE_ON, 0 );
		break;
	case 8:
		FireHurtTargets( "GeneWormCenterSlash", this, this, USE_OFF, 0 );
		break;
	case 9:
		FireTargets( "GeneWormWallHit", this, this, USE_TOGGLE, 0 );
		UTIL_ScreenShake( pev->origin, 24, 3, 5, 2048 );
		break;
	default:
		break;
	}
}

void COFGeneWorm::CommandUse( CBaseEntity*, CBaseEntity*, USE_TYPE useType, float )
{
	if ( useType == USE_TOGGLE && !m_fActivated )
	{
		pev->sequence = LookupSequence( "entry" );
		pev->frame = 0;
		ResetSequenceInfo();
		pev->renderamt = 0;
		pev->rendermode = kRenderTransTexture;
		pev->renderfx = kRenderFxNone;
		m_fActivated = TRUE;
		pev->solid = SOLID_BBOX;
		UTIL_SetOrigin( pev, pev->origin );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_entry.wav", 1, 0.1f, 0, 100 );
	}
}

void COFGeneWorm::DyingThink()
{
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	DispatchAnimEvents( 0.1f );
	StudioFrameAdvance();
	if ( m_fSequenceFinished && pev->deadflag == DEAD_DYING )
	{
		UTIL_Remove( this );
		return;
	}
	if ( pev->deadflag == DEAD_NO )
	{
		int direction = 1;
		pev->deadflag = DEAD_DYING;
		pev->frame = 0;
		pev->sequence = FindTransition( pev->sequence, LookupSequence( "death" ), &direction );
		pev->frame = direction > 0 ? 0.0f : 255.0f;
		pev->renderfx = kRenderFxNone;
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 255;
		ResetSequenceInfo();
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_death.wav", 1, 0.1f, 0, 100 );
		m_flDeathStart = gpGlobals->time;
		FireTargets( "GeneWormDead", this, this, USE_TOGGLE, 0 );
		CBaseEntity* pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "monster_shocktrooper" ) ) != NULL )
		{
			pEntity->SetThink( &CBaseEntity::SUB_FadeOut );
			pEntity->pev->nextthink = (float)( gpGlobals->time + 0.1 );
		}
		pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "monster_shockroach" ) ) != NULL )
		{
			pEntity->SetThink( &CBaseEntity::SUB_FadeOut );
			pEntity->pev->nextthink = (float)( gpGlobals->time + 0.1 );
		}
	}
	if ( gpGlobals->time - m_flDeathStart >= 15 )
	{
		CBaseEntity* pPlayer = UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer )
		{
			CBaseEntity* pTarget = UTIL_FindEntityByTargetname( NULL, "GeneWormTeleport" );
			while ( pTarget != NULL )
			{
				pTarget->Touch( pPlayer );
				pTarget = UTIL_FindEntityByTargetname( pTarget, "GeneWormTeleport" );
				ALERT( at_console, "Touching Target GeneWormTeleport\n" );
			}
			FireTargets( "GeneWormTeleport", pPlayer, pPlayer, USE_ON, 1 );
		}
		m_flDeathStart = gpGlobals->time + 999;
	}
	if ( pev->deadflag == DEAD_DYING )
		pev->renderamt -= 1;
	if ( m_pCloud != NULL )
	{
		UTIL_Remove( m_pCloud );
		m_pCloud = NULL;
	}
	if ( m_orificeGlow != NULL )
	{
		UTIL_Remove( m_orificeGlow );
		m_orificeGlow = NULL;
	}
}

void COFGeneWorm::NullThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5f;
}

void COFGeneWorm::SpewCloud()
{
	UTIL_MakeVectors( pev->angles );
}

BOOL COFGeneWorm::ClawAttack()
{
	CBaseEntity* pEnemy = m_hEnemy;
	if ( pEnemy == NULL )
		return FALSE;
	float distance = ( pev->origin - pEnemy->pev->origin ).Length2D();
	if ( !( m_flNextMeleeTime <= gpGlobals->time ) )
		return FALSE;
	m_posTarget = pEnemy->pev->origin;
	if ( !FVisible( m_posTarget ) )
		return FALSE;
	Vector direction = ( m_posTarget - pev->origin ).Normalize();
	Vector angles = UTIL_VecToAngles( direction );
	float yaw = UTIL_AngleDiff( angles.y, pev->angles.y );
	if ( gpGlobals->time > m_flNextRangeTime )
	{
		Vector mouthPos, mouthAngles;
		GetAttachment( 0, mouthPos, mouthAngles );
		if ( yaw >= 10 )
			pev->sequence = LookupSequence( "dattack2" );
		else if ( yaw > -10 )
			pev->sequence = LookupSequence( "dattack1" );
		else
			pev->sequence = LookupSequence( "dattack3" );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_beam_attack.wav", 1, 0.1f, 0, 100 );
		m_flNextRangeTime = gpGlobals->time + RANDOM_FLOAT( 10, 15 );
	}
	else
	{
		if ( !( distance < 1280 ) )
			return FALSE;
		if ( m_posTarget.z <= pev->origin.z )
		{
			pev->sequence = LookupSequence( "melee3" );
		}
		else if ( yaw >= 10 )
		{
			pev->sequence = LookupSequence( "melee1" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_attack_mounted_rocket.wav", 1, 0.1f, 0, 100 );
		}
		else if ( yaw > -2 )
		{
			pev->sequence = LookupSequence( "melee3" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_big_attack_forward.wav", 1, 0.1f, 0, 100 );
		}
		else
		{
			pev->sequence = LookupSequence( "melee2" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_attack_mounted_gun.wav", 1, 0.1f, 0, 100 );
		}
	}
	m_flNextMeleeTime = gpGlobals->time + RANDOM_FLOAT( 3, 5 );
	return TRUE;
}

void COFGeneWorm::TrackHead()
{
	Vector mouthPos, mouthAngles;
	GetAttachment( 0, mouthPos, mouthAngles );
	Vector targetAngles = UTIL_VecToAngles( ( m_posTarget - mouthPos ).Normalize() );
	float yaw = UTIL_AngleDiff( targetAngles.y, pev->angles.y );
	SetBoneController( 0, max( -30.0f, min( yaw, 30.0f ) ) );
}

void COFGeneWorm::NextActivity()
{
	UTIL_MakeAimVectors( pev->angles );
	if ( (CBaseEntity*)m_hEnemy != NULL && !m_hEnemy->IsAlive() )
		m_hEnemy = NULL;
	if ( gpGlobals->time > m_flLastSeen + 15 && (CBaseEntity*)m_hEnemy != NULL
		&& ( pev->origin - m_hEnemy->pev->origin ).Length2D() > 700 )
		m_hEnemy = NULL;
	if ( (CBaseEntity*)m_hEnemy == NULL )
	{
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy();
	}
	if ( m_fGetMad )
	{
		pev->sequence = LookupSequence( "mad" );
		m_fGetMad = FALSE;
		return;
	}
	if ( m_fRightEyeHit && m_fLeftEyeHit )
	{
		if ( gpGlobals->time <= m_flOrificeOpenTime && !m_fOrificeHit )
		{
			pev->sequence = LookupSequence( "bigpain2" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_final_pain2.wav", 1, 0.1f, 0, 100 );
			return;
		}
		m_flOrificeOpenTime = gpGlobals->time;
		if ( !m_fSpawningTrooper )
		{
			pev->sequence = LookupSequence( "bigpain4" );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_final_pain4.wav", 1, 0.1f, 0, 100 );
			m_fSpawningTrooper = TRUE;
			return;
		}
		m_fLeftEyeHit = FALSE;
		m_fRightEyeHit = FALSE;
		m_fOrificeHit = FALSE;
		pev->skin = 0;
		m_fSpawningTrooper = FALSE;
	}
	if ( (CBaseEntity*)m_hEnemy != NULL )
	{
		if ( ClawAttack() )
			return;
		if ( m_iHitTimes > 1 && gpGlobals->time > m_flMadDelayTime
			&& RANDOM_LONG( 0, m_iMaxHitTimes - m_iHitTimes ) == 0
			&& (CBaseEntity*)m_hEnemy != NULL && FVisible( m_hEnemy ) )
		{
			pev->sequence = LookupSequence( "mad" );
			m_flMadDelayTime = gpGlobals->time + 15;
			return;
		}
		if ( m_iHitTimes == 0 )
			pev->sequence = LookupSequence( "idlepain" );
		else if ( m_iHitTimes == 1 )
			pev->sequence = LookupSequence( "idlepain2" );
		else if ( m_iHitTimes > 1 )
			pev->sequence = LookupSequence( "idlepain3" );
	}
	else
	{
		switch ( m_iHitTimes )
		{
		case 0: pev->sequence = LookupSequence( "idlepain" ); break;
		case 1: pev->sequence = LookupSequence( "idlepain2" ); break;
		case 2: pev->sequence = LookupSequence( "idlepain3" ); break;
		default: break;
		}
	}
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, pIdleSounds[RANDOM_LONG( 0, 3 )], 1, 0.1f, 0, pitch );
}

void COFGeneWorm::HuntThink()
{
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	if ( !m_fActivated )
		return;
	DispatchAnimEvents( 0.1f );
	StudioFrameAdvance();
	UpdateShockEffect();
	if ( pev->rendermode == kRenderTransTexture )
	{
		if ( pev->renderamt < 248 )
		{
			pev->renderamt += 3;
		}
		else
		{
			pev->renderamt = 255;
			pev->rendermode = kRenderNormal;
			pev->renderfx = kRenderFxNone;
		}
	}
	if ( m_iWasHit == 2 )
	{
		if ( m_fShockEffect )
		{
			pev->rendermode = m_iOldRenderMode;
			pev->renderfx = m_iOldRenderFX;
			pev->rendercolor = m_OldRenderColor;
			pev->renderamt = m_flOldRenderAmt;
			m_flShockDuration = 0;
			m_fShockEffect = FALSE;
		}
		SetThink( &COFGeneWorm::DyingThink );
		m_fSequenceFinished = TRUE;
		return;
	}
	if ( m_iWasHit == 1 )
	{
		int direction = 1;
		if ( m_pCloud != NULL )
		{
			UTIL_Remove( m_pCloud );
			m_pCloud = NULL;
		}
		if ( m_fSpitting )
			m_fSpitting = FALSE;
		if ( m_fRightEyeHit && m_fLeftEyeHit )
		{
			if ( m_fOrificeHit )
			{
				pev->sequence = FindTransition( pev->sequence, LookupSequence( "bigpain3" ), &direction );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_final_pain3.wav", 1, 0.1f, 0, 100 );
			}
			else
			{
				pev->sequence = FindTransition( pev->sequence, LookupSequence( "bigpain1" ), &direction );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_final_pain1.wav", 1, 0.1f, 0, 100 );
				Vector origin, angles;
				GetAttachment( 1, origin, angles );
				m_orificeGlow = COFGeneWormSpawn::GeneWormSpawnCreate( origin );
				m_orificeGlow->pev->rendermode = kRenderTransAdd;
				m_orificeGlow->pev->rendercolor = Vector( 255, 255, 255 );
				m_orificeGlow->pev->renderamt = 255;
				m_orificeGlow->pev->renderfx = kRenderFxNoDissipation;
				m_orificeGlow->pev->scale = 0.95f;
				m_orificeGlow->pev->framerate = 10;
				m_orificeGlow->TurnOn();
			}
		}
		else
		{
			pev->sequence = FindTransition( pev->sequence,
				LookupSequence( m_fRightEyeHit ? "eyepain2" : "eyepain1" ), &direction );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "geneworm/geneworm_shot_in_eye.wav", 1, 0.1f, 0, 100 );
		}
		pev->frame = direction > 0 ? 0.0f : 255.0f;
		ResetSequenceInfo();
		m_iWasHit = 0;
	}
	else if ( m_fSequenceFinished )
	{
		int sequence = pev->sequence;
		if ( !m_fHasEntered )
			m_fHasEntered = TRUE;
		NextActivity();
		if ( !m_fSequenceLoops || sequence != pev->sequence )
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}

	Vector origin, angles;
	for ( int i = 0; i < 2; i++ )
	{
		if ( i == 0 ? m_fRightEyeHit : m_fLeftEyeHit )
			continue;
		GetAttachment( i + 2, origin, angles );
		MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
			WRITE_BYTE( TE_ELIGHT );
			WRITE_SHORT( entindex() + ( ( i + 3 ) << 12 ) );
			WRITE_COORD( origin.x );
			WRITE_COORD( origin.y );
			WRITE_COORD( origin.z );
			WRITE_COORD( 48 );
			WRITE_BYTE( 128 );
			WRITE_BYTE( 255 );
			WRITE_BYTE( 128 );
			WRITE_BYTE( 1 );
			WRITE_COORD( i == 0 ? 2 : 1 );
		MESSAGE_END();
	}
	if ( m_orificeGlow != NULL )
	{
		GetAttachment( 1, origin, angles );
		UTIL_SetOrigin( m_orificeGlow->pev, origin );
	}
	if ( (CBaseEntity*)m_hEnemy != NULL )
	{
		if ( FVisible( m_hEnemy ) )
		{
			if ( gpGlobals->time - 5 > m_flLastSeen )
				m_flPrevSeen = gpGlobals->time;
			m_flLastSeen = gpGlobals->time;
			m_posTarget = m_hEnemy->pev->origin;
			m_posTarget.z += 24;
		}
		TrackHead();
	}
	else
	{
		SetBoneController( 0, 0 );
	}
	if ( m_fSpitting )
	{
		if ( gpGlobals->time - m_flSpitStartTime <= 2 )
		{
			GetAttachment( 0, origin, angles );
			m_pCloud = COFGeneWormCloud::GeneWormCloudCreate( origin );
			if ( (CBaseEntity*)m_hEnemy != NULL )
			{
				m_pCloud->pev->rendermode = kRenderGlow;
				m_pCloud->pev->rendercolor = Vector( 255, 255, 255 );
				m_pCloud->pev->renderamt = 255;
				m_pCloud->pev->renderfx = kRenderFxNoDissipation;
				m_pCloud->pev->rendercolor = Vector( 0, 255, 0 );
				if ( edict() != NULL )
				{
					m_pCloud->pev->skin = entindex();
					m_pCloud->pev->body = 1;
					m_pCloud->pev->aiment = edict();
					m_pCloud->pev->movetype = MOVETYPE_FOLLOW;
				}
				m_pCloud->pev->scale = 0.5f;
				m_pCloud->pev->framerate = 10;
				m_pCloud->TurnOn();
				float spread = RANDOM_FLOAT( -0.01, 0.01 );
				Vector direction = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - origin ).Normalize();
				direction.z += spread;
				m_pCloud->LaunchCloud( origin, direction, 1000, edict(), 35 );
				m_pCloud = NULL;
			}
		}
		else
		{
			m_fSpitting = FALSE;
		}
	}
}
