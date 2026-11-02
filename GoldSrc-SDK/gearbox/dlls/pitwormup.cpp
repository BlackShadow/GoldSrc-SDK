#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"
#include "decals.h"

class COFPitWormUp : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int ObjectCaps() override { return 0; }
	int Classify() override { return CLASS_ALIEN_MILITARY; }
	int BloodColor() override { return BLOOD_COLOR_YELLOW; }
	void SetObjectCollisionBox() override;
	void HandleAnimEvent( MonsterEvent_t* event ) override;
	void GibMonster() override;
	void Killed( entvars_t* attacker, int gib ) override;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType ) override;
	BOOL FVisible( CBaseEntity* entity ) override;
	BOOL FVisible( const Vector& origin ) override;
	void PainSound() override;
	void IdleSound() override;
	void EXPORT HitTouch( CBaseEntity* other );
	void EXPORT StartupThink();
	void EXPORT HuntThink();
	void EXPORT DyingThink();
	void EXPORT NullThink();
	void EXPORT StartupUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void EXPORT CommandUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	void LockTopLevel();
	void NextActivity();
	void ChangeLevel();
	void TrackEnemy();
	BOOL ClawAttack();
	void ShootBeam();
	void StrafeBeam();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pIdleSounds[];
	static const char* pHitGroundSounds[];
	static const char* pPainSounds[];
	static const char* pShootSounds[];
	static const char* pAttackVoiceSounds[];
	static const char* pAttackSounds[];

	float	m_flNextPainSound;
	Vector	m_vecTarget;
	Vector	m_posTarget;
	Vector	m_vecDesired;
	Vector	m_posDesired;
	float	m_offsetBeam;
	Vector	m_posBeam;
	Vector	m_vecBeam;
	Vector	m_angleBeam;
	float	m_flBeamExpireTime;
	float	m_flBeamDir;
	float	m_flTorsoYaw;
	float	m_flHeadYaw;
	float	m_flHeadPitch;
	float	m_flIdealTorsoYaw;
	float	m_flIdealHeadYaw;
	float	m_flIdealHeadPitch;
	float	m_flLevels[4];
	float	m_flTargetLevels[4];
	float	m_flLastSeen;
	float	m_flPrevSeen;
	int		m_iLevel;
	float	m_flLevelSpeed;
	CBeam*	m_pBeam;
	CSprite*	m_pSprite;
	BOOL	m_fAttacking;
	BOOL	m_fLockHeight;
	BOOL	m_fLockYaw;
	int		m_iWasHit;
	float	m_flTakeHitTime;
	float	m_flHitTime;
	float	m_flNextMeleeTime;
	float	m_flNextRangeTime;
	float	m_flDeathStartTime;
	BOOL	m_fFirstSighting;
	BOOL	m_fTopLevelLocked;
	float	m_flLastBlinkTime;
	float	m_flLastBlinkInterval;
	float	m_flLastEventTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFPitWormGib : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT GibFloat();
};

class COFPitWormGibShooter : public CBaseEntity
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* data ) override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
	void EXPORT ShootThink();
	virtual COFPitWormGib* CreateGib();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flDelay;
	int		m_iGibs;
	int		m_iGibCapacity;
	int		m_iGibMaterial;
	int		m_iGibModelIndex;
	float	m_flGibVelocity;
	float	m_flVariance;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFPitWormSteamTrigger : public CPointEntity
{
public:
	void Spawn() override;
	void Use( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value ) override;
};

LINK_ENTITY_TO_CLASS( monster_pitworm_up, COFPitWormUp );
LINK_ENTITY_TO_CLASS( pitworm_gib, COFPitWormGib );
LINK_ENTITY_TO_CLASS( pitworm_gibshooter, COFPitWormGibShooter );
LINK_ENTITY_TO_CLASS( info_pitworm_steam_lock, COFPitWormSteamTrigger );

TYPEDESCRIPTION COFPitWormUp::m_SaveData[] =
{
	DEFINE_FIELD( COFPitWormUp, m_flNextPainSound, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_posTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_vecDesired, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_posDesired, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_offsetBeam, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_posBeam, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_vecBeam, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_angleBeam, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWormUp, m_flBeamExpireTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flBeamDir, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flTorsoYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flHeadYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flHeadPitch, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flIdealTorsoYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flIdealHeadYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flIdealHeadPitch, FIELD_FLOAT ),
	DEFINE_ARRAY( COFPitWormUp, m_flLevels, FIELD_FLOAT, 4 ),
	DEFINE_ARRAY( COFPitWormUp, m_flTargetLevels, FIELD_FLOAT, 4 ),
	DEFINE_FIELD( COFPitWormUp, m_flLastSeen, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_flPrevSeen, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_iLevel, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWormUp, m_flLevelSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFPitWormUp, m_pSprite, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFPitWormUp, m_fAttacking, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFPitWormUp, m_fLockHeight, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFPitWormUp, m_fLockYaw, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFPitWormUp, m_iWasHit, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWormUp, m_flTakeHitTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_flHitTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_flNextMeleeTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_flNextRangeTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_flDeathStartTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWormUp, m_fFirstSighting, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFPitWormUp, m_fTopLevelLocked, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFPitWormUp, m_flLastBlinkTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flLastBlinkInterval, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormUp, m_flLastEventTime, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COFPitWormUp, CBaseMonster );

TYPEDESCRIPTION COFPitWormGibShooter::m_SaveData[] =
{
	DEFINE_FIELD( COFPitWormGibShooter, m_iGibs, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWormGibShooter, m_iGibCapacity, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWormGibShooter, m_iGibMaterial, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWormGibShooter, m_flGibVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWormGibShooter, m_flVariance, FIELD_FLOAT ),
};

IMPLEMENT_SAVERESTORE( COFPitWormGibShooter, CBaseEntity );

const char* COFPitWormUp::pIdleSounds[] =
{
	"pitworm/pit_worm_idle1.wav", "pitworm/pit_worm_idle2.wav", "pitworm/pit_worm_idle3.wav"
};
const char* COFPitWormUp::pHitGroundSounds[] = { "tentacle/te_strike1.wav", "tentacle/te_strike2.wav" };
const char* COFPitWormUp::pPainSounds[] = { "pitworm/pit_worm_flinch1.wav", "pitworm/pit_worm_flinch2.wav" };
const char* COFPitWormUp::pShootSounds[] = { "debris/beamstart3.wav", "debris/beamstart8.wav" };
const char* COFPitWormUp::pAttackVoiceSounds[] =
{
	"pitworm/pit_worm_attack_swipe1.wav", "pitworm/pit_worm_attack_swipe2.wav", "pitworm/pit_worm_attack_swipe3.wav"
};
const char* COFPitWormUp::pAttackSounds[] =
{
	"zombie/claw_strike1.wav", "zombie/claw_strike2.wav", "zombie/claw_strike3.wav"
};

void COFPitWormGib::Precache()
{
	PRECACHE_MODEL( "models/pit_worm_gibs.mdl" );
}

void COFPitWormGib::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->friction = 0.55f;
	pev->renderamt = 255;
	pev->rendermode = kRenderNormal;
	pev->renderfx = kRenderFxNone;
	pev->solid = SOLID_NOT;
	pev->classname = MAKE_STRING( "pitworm_gib" );
	SET_MODEL( ENT( pev ), "models/pit_worm_gibs.mdl" );
	UTIL_SetSize( pev, Vector( -8, -8, -4 ), Vector( 8, 8, 16 ) );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &COFPitWormGib::GibFloat );
}

void COFPitWormGib::GibFloat()
{
	if ( pev->waterlevel == 3 )
	{
		pev->movetype = MOVETYPE_FLY;
		pev->velocity = pev->velocity * 0.8f;
		pev->avelocity = pev->avelocity * 0.9f;
		pev->velocity.z += 8;
	}
	else
	{
		if ( !pev->waterlevel )
			pev->movetype = MOVETYPE_BOUNCE;
		pev->velocity.z -= 8;
	}
	pev->nextthink = gpGlobals->time + 0.1f;
}

void COFPitWormGibShooter::Precache()
{
	m_iGibModelIndex = PRECACHE_MODEL( "models/pit_worm_gibs.mdl" );
	UTIL_PrecacheOther( "pitworm_gib" );
}

void COFPitWormGibShooter::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	pev->effects = EF_NODRAW;
	if ( m_flDelay == 0 )
		m_flDelay = 0.1f;
	SetMovedir( pev );
	pev->body = MODEL_FRAMES( m_iGibModelIndex );
}

void COFPitWormGibShooter::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "m_iGibs" ) )
		m_iGibCapacity = m_iGibs = atoi( data->szValue );
	else if ( FStrEq( data->szKeyName, "m_flVelocity" ) )
		m_flGibVelocity = (float)atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "m_flVariance" ) )
		m_flVariance = (float)atof( data->szValue );
	else if ( FStrEq( data->szKeyName, "m_flDelay" ) )
		m_flDelay = (float)atof( data->szValue );
	else
	{
		data->fHandled = FALSE;
		return;
	}
	data->fHandled = TRUE;
}

void COFPitWormGibShooter::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( &COFPitWormGibShooter::ShootThink );
	pev->nextthink = gpGlobals->time;
}

COFPitWormGib* COFPitWormGibShooter::CreateGib()
{
	if ( !CVAR_GET_FLOAT( "violence_hgibs" ) )
		return NULL;
	COFPitWormGib* gib = GetClassPtr( (COFPitWormGib*)NULL );
	gib->Spawn();
	if ( pev->body <= 1 )
		ALERT( at_aiconsole, "GibShooter Body is <= 1!\n" );
	gib->pev->body = RANDOM_LONG( 0, pev->body - 1 );
	return gib;
}

void COFPitWormGibShooter::ShootThink()
{
	pev->nextthink = gpGlobals->time + m_flDelay;
	UTIL_MakeVectors( pev->angles );
	Vector direction = gpGlobals->v_up.Normalize();
	COFPitWormGib* gib = CreateGib();
	if ( gib )
	{
		gib->pev->origin = pev->origin;
		gib->pev->velocity = direction * m_flGibVelocity;
	}
	SetUse( NULL );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void COFPitWormSteamTrigger::Spawn()
{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->effects = EF_NODRAW;
	UTIL_SetOrigin( pev, pev->origin );
}

void COFPitWormSteamTrigger::Use( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	COFPitWormUp* worm = (COFPitWormUp*)UTIL_FindEntityByClassname( NULL, "monster_pitworm_up" );
	if ( worm )
		worm->LockTopLevel();
}

void COFPitWormUp::Precache()
{
	PRECACHE_MODEL( "models/pit_worm_up.mdl" );
	PRECACHE_MODEL( "sprites/tele1.spr" );
	for ( int i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackVoiceSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackVoiceSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pShootSounds ); i++ )
		PRECACHE_SOUND( (char*)pShootSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char*)pPainSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pHitGroundSounds ); i++ )
		PRECACHE_SOUND( (char*)pHitGroundSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char*)pIdleSounds[i] );
	PRECACHE_SOUND( "debris/beamstart7.wav" );
	PRECACHE_SOUND( "pitworm/clang1.wav" );
	PRECACHE_SOUND( "pitworm/clang2.wav" );
	PRECACHE_SOUND( "pitworm/clang3.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_alert.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_attack_eyeblast.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_attack_eyeblast_impact.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_attack_swipe1.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_attack_swipe2.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_attack_swipe3.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_death.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_flinch1.wav" );
	PRECACHE_SOUND( "pitworm/pit_worm_flinch2.wav" );
	PRECACHE_MODEL( "models/pit_worm_gibs.mdl" );
	UTIL_PrecacheOther( "pitworm_gib" );
}

void COFPitWormUp::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/pit_worm_up.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = pev->max_health = gSkillData.pitWormHealth;
	pev->view_ofs = Vector( 0, 0, 300 );
	m_bloodColor = BLOOD_COLOR_YELLOW;
	m_flFieldOfView = 0.5f;
	pev->sequence = 0;
	ResetSequenceInfo();
	m_flTorsoYaw = m_flHeadYaw = m_flHeadPitch = 0;
	m_flIdealTorsoYaw = m_flIdealHeadYaw = m_flIdealHeadPitch = 0;
	InitBoneControllers();
	SetThink( &COFPitWormUp::StartupThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	m_vecDesired = Vector( 1, 0, 0 );
	SetTouch( &COFPitWormUp::HitTouch );
	m_posDesired = pev->origin;
	m_fAttacking = m_fLockHeight = m_fFirstSighting = FALSE;
	m_flBeamExpireTime = gpGlobals->time;
	m_iLevel = 0;
	m_fLockYaw = FALSE;
	m_iWasHit = 0;
	m_flTakeHitTime = m_flHitTime = 0;
	m_flLevelSpeed = 10;
	m_fTopLevelLocked = FALSE;
	m_flLastBlinkTime = m_flLastBlinkInterval = m_flLastEventTime = gpGlobals->time;
	m_flTargetLevels[3] = pev->origin.z;
	m_flLevels[3] = pev->origin.z - 350;
	m_flTargetLevels[2] = pev->origin.z;
	m_flLevels[2] = pev->origin.z - 300;
	m_flTargetLevels[1] = pev->origin.z;
	m_flLevels[1] = pev->origin.z - 300;
	m_flTargetLevels[0] = pev->origin.z;
	m_flLevels[0] = pev->origin.z - 300;
	m_pBeam = NULL;
}

void COFPitWormUp::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -400, -400, 0 );
	pev->absmax = pev->origin + Vector( 400, 400, 850 );
}

void COFPitWormUp::StartupThink()
{
	CBaseEntity* node = UTIL_FindEntityByTargetname( NULL, "pw_tlevel3" );
	if ( node )
	{
		ALERT( at_console, "level 3 node set\n" );
		m_flTargetLevels[3] = node->pev->origin.z;
		m_flLevels[3] = node->pev->origin.z - 350;
	}
	node = UTIL_FindEntityByTargetname( NULL, "pw_tlevel2" );
	if ( node )
	{
		ALERT( at_console, "level 2 node set\n" );
		m_flTargetLevels[2] = node->pev->origin.z;
		m_flLevels[2] = node->pev->origin.z - 300;
	}
	node = UTIL_FindEntityByTargetname( NULL, "pw_tlevel1" );
	if ( node )
	{
		ALERT( at_console, "level 1 node set\n" );
		m_flTargetLevels[1] = node->pev->origin.z;
		m_flLevels[1] = node->pev->origin.z - 300;
	}
	node = UTIL_FindEntityByTargetname( NULL, "pw_tleveldead" );
	if ( node )
	{
		ALERT( at_console, "level dead node set\n" );
		m_flTargetLevels[0] = node->pev->origin.z;
		m_flLevels[0] = node->pev->origin.z - 300;
	}
	if ( pev->target )
	{
		const char* target = STRING( pev->target );
		if ( !m_fTopLevelLocked && !_stricmp( target, "pw_level3" ) )
			m_iLevel = 3;
		else if ( !_stricmp( target, "pw_level2" ) )
			m_iLevel = 2;
		else if ( !_stricmp( target, "pw_level1" ) )
			m_iLevel = 1;
		else if ( !_stricmp( target, "pw_leveldead" ) )
			m_iLevel = 0;
		m_posDesired.z = m_flLevels[m_iLevel];
	}
	Vector origin, angles;
	GetAttachment( 0, origin, angles );
	pev->view_ofs = origin - pev->origin;
	m_flNextMeleeTime = m_flNextRangeTime = gpGlobals->time;
	SetThink( &COFPitWormUp::HuntThink );
	SetUse( &COFPitWormUp::CommandUse );
	pev->nextthink = gpGlobals->time + 0.1f;
}

void COFPitWormUp::StartupUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( &COFPitWormUp::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1f;
	SetUse( &COFPitWormUp::CommandUse );
}

void COFPitWormUp::CommandUse( CBaseEntity* activator, CBaseEntity*, USE_TYPE useType, float )
{
	switch ( useType )
	{
	case USE_ON:
		if ( activator )
			CSoundEnt::InsertSound( bits_SOUND_WORLD, activator->pev->origin, 1024, 1 );
		break;
	case USE_OFF:
	case USE_TOGGLE:
		pev->takedamage = DAMAGE_NO;
		SetThink( &COFPitWormUp::DyingThink );
		break;
	default:
		break;
	}
}

void COFPitWormUp::LockTopLevel()
{
	if ( m_iLevel == 3 )
	{
		pev->health = pev->max_health;
		float time = gpGlobals->time;
		m_flTakeHitTime = RANDOM_LONG( 2, 4 ) + time;
		m_iWasHit = 1;
		m_iLevel = 2;
		m_posDesired.z = m_flLevels[2];
	}
	m_fTopLevelLocked = TRUE;
}

void COFPitWormUp::NullThink()
{
	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.5f;
}

void COFPitWormUp::GibMonster()
{
}

void COFPitWormUp::Killed( entvars_t* attacker, int gib )
{
	CBaseMonster::Killed( attacker, gib );
}

int COFPitWormUp::TakeDamage( entvars_t*, entvars_t*, float, int )
{
	PainSound();
	return 0;
}

void COFPitWormUp::PainSound()
{
	if ( m_flNextPainSound <= gpGlobals->time )
	{
		float time = gpGlobals->time;
		m_flNextPainSound = RANDOM_FLOAT( 2, 5 ) + time;
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, 1 )], 1, 0.1f, 0, 100 );
	}
}

void COFPitWormUp::IdleSound()
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, 1 )], 1, 0.1f, 0, 100 );
}

BOOL COFPitWormUp::FVisible( const Vector& origin )
{
	Vector eye, angles;
	GetAttachment( 0, eye, angles );
	TraceResult trace;
	UTIL_TraceLine( eye, origin, ignore_monsters, ignore_glass, edict(), &trace );
	return trace.flFraction == 1;
}

BOOL COFPitWormUp::FVisible( CBaseEntity* entity )
{
	if ( entity->pev->flags & FL_NOTARGET )
		return FALSE;
	if ( pev->waterlevel == 3 && !entity->pev->waterlevel )
		return FALSE;
	if ( pev->waterlevel != 3 && entity->pev->waterlevel == 3 )
		return FALSE;
	Vector eye, angles;
	GetAttachment( 0, eye, angles );
	TraceResult trace;
	UTIL_TraceLine( eye, entity->EyePosition(), ignore_monsters, ignore_glass, edict(), &trace );
	return trace.flFraction == 1;
}

void COFPitWormUp::HitTouch( CBaseEntity* other )
{
	TraceResult trace = UTIL_GetGlobalTrace();
	if ( other->pev->modelindex == pev->modelindex || !( m_flHitTime <= gpGlobals->time ) || !trace.pHit ||
		trace.pHit->v.modelindex != pev->modelindex || !other->pev->takedamage )
		return;
	other->TakeDamage( pev, pev, 20, DMG_CRUSH | DMG_SLASH );
	other->pev->punchangle.z = 15;
	other->pev->velocity = other->pev->velocity + Vector( 0, 0, 200 );
	other->pev->flags &= ~FL_ONGROUND;
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackSounds[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM, 0, pitch );
	if ( trace.iHitgroup == 2 )
		other->TakeDamage( pev, pev, 10, DMG_CRUSH | DMG_SLASH );
	m_flHitTime = gpGlobals->time + 1;
}

void COFPitWormUp::TraceAttack( entvars_t* attacker, float damage, Vector direction,
		TraceResult* trace, int damageType )
{
	if ( trace->iHitgroup != 1 )
	{
		if ( pev->dmgtime != gpGlobals->time || RANDOM_LONG( 0, 10 ) <= 0 )
		{
			UTIL_Ricochet( trace->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
		return;
	}
	if ( !( gpGlobals->time > m_flTakeHitTime ) )
		return;
	pev->health -= damage;
	if ( pev->health <= 0 )
	{
		pev->health = pev->max_health;
		float time = gpGlobals->time;
		m_flTakeHitTime = RANDOM_LONG( 2, 4 ) + time;
		m_iWasHit = 1;
	}
	if ( m_bloodColor != DONT_BLEED )
	{
		SpawnBlood( trace->vecEndPos - direction * 4, m_bloodColor, damage * 10 );
		TraceBleed( damage, direction, trace, damageType );
		if ( attacker && m_hEnemy == NULL )
		{
			CBaseEntity* enemy = CBaseEntity::Instance( attacker );
			if ( enemy && enemy->MyMonsterPointer() )
			{
				m_hEnemy = enemy;
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "pitworm/pit_worm_alert.wav", 1, 0.1f, 0, 100 );
				if ( !m_fFirstSighting )
				{
					pev->sequence = 3;
					m_fFirstSighting = TRUE;
					return;
				}
			}
		}
	}
	if ( !pev->skin )
	{
		pev->skin = 1;
		m_flLastBlinkInterval = m_flLastBlinkTime = gpGlobals->time;
	}
}

void COFPitWormUp::HandleAnimEvent( MonsterEvent_t* event )
{
	switch ( event->event )
	{
	case 1:
	{
		int pitch = 100 + RANDOM_LONG( -5, 5 );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pHitGroundSounds[RANDOM_LONG( 0, 1 )], 1, ATTN_NORM, 0, pitch );
		if ( pev->sequence == 2 )
			UTIL_ScreenShake( pev->origin, 12, 100, 2, 1000 );
		else
			UTIL_ScreenShake( pev->origin, 4, 3, 1, 750 );
		break;
	}
	case 2:
		if ( gpGlobals->time - m_flLastEventTime >= 1.1 )
		{
			m_posBeam = m_hEnemy->pev->origin;
			m_posBeam.z += 24;
			Vector eye, angles;
			GetAttachment( 0, eye, angles );
			m_vecBeam = ( m_posBeam - eye ).Normalize();
			m_angleBeam = UTIL_VecToAngles( m_vecBeam );
			ShootBeam();
			m_fLockYaw = TRUE;
		}
		break;
	case 4:
		m_fLockYaw = TRUE;
		break;
	default:
		break;
	}
}

void COFPitWormUp::ShootBeam()
{
	if ( m_hEnemy == NULL )
		return;
	m_flBeamDir = m_flHeadYaw > 0 ? -1.0f : 1.0f;
	m_offsetBeam = -m_flBeamDir * 80;
	Vector eye, angles;
	GetAttachment( 0, eye, angles );
	m_vecBeam = ( m_posBeam - eye ).Normalize();
	m_angleBeam = UTIL_VecToAngles( m_vecBeam );
	UTIL_MakeVectors( m_angleBeam );
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;
	Vector end = eye + m_vecBeam * 1280 + gpGlobals->v_right * m_offsetBeam;
	TraceResult trace;
	UTIL_TraceLine( eye, end, dont_ignore_monsters, edict(), &trace );
	m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
	if ( !m_pBeam )
		return;
	m_pBeam->PointEntInit( trace.vecEndPos, entindex() );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->SetColor( 0, 255, 32 );
	m_pBeam->SetBrightness( 128 );
	m_pBeam->SetWidth( 32 );
	m_pBeam->pev->spawnflags |= SF_BEAM_SPARKEND;
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	if ( hit && hit->pev->takedamage )
	{
		ClearMultiDamage();
		hit->TraceAttack( pev, gSkillData.pitWormDmgBeam, m_vecBeam, &trace, DMG_ENERGYBEAM );
		hit->TakeDamage( pev, pev, gSkillData.pitWormDmgBeam, DMG_ENERGYBEAM );
	}
	else if ( trace.flFraction != 1 )
	{
		UTIL_DecalTrace( &trace, RANDOM_LONG( 0, 4 ) );
		m_pBeam->DoSparks( trace.vecEndPos, trace.vecEndPos );
	}
	m_pBeam->DoSparks( eye, eye );
	m_flBeamExpireTime = gpGlobals->time + 0.9f;
	float headYaw = m_flHeadYaw - m_flBeamDir * 25;
	if ( headYaw >= -45 && headYaw <= 45 )
		m_flHeadYaw = headYaw;
	m_flIdealHeadYaw += m_flBeamDir * 50;
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pShootSounds[RANDOM_LONG( 0, 1 )], 1, ATTN_NORM, 0, pitch );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( eye.x );
		WRITE_COORD( eye.y );
		WRITE_COORD( eye.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 1 );
		WRITE_COORD( 2 );
	MESSAGE_END();
	m_pSprite = CSprite::SpriteCreate( "sprites/tele1.spr", eye, TRUE );
	if ( m_pSprite )
	{
		m_pSprite->SetTransparency( kRenderGlow, 255, 255, 255, 255, kRenderFxNoDissipation );
		m_pSprite->SetColor( 0, 255, 0 );
		m_pSprite->SetAttachment( edict(), 1 );
		m_pSprite->SetScale( 0.75f );
		m_pSprite->pev->framerate = 10;
		m_pSprite->TurnOn();
	}
}

void COFPitWormUp::StrafeBeam()
{
	m_offsetBeam += 20 * m_flBeamDir;
	Vector eye, angles;
	GetAttachment( 0, eye, angles );
	m_vecBeam = ( m_posBeam - eye ).Normalize();
	m_angleBeam = UTIL_VecToAngles( m_vecBeam );
	UTIL_MakeVectors( m_angleBeam );
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;
	Vector end = eye + m_vecBeam * 1280 + gpGlobals->v_right * m_offsetBeam;
	TraceResult trace;
	UTIL_TraceLine( eye, end, dont_ignore_monsters, edict(), &trace );
	m_pBeam->DoSparks( eye, eye );
	m_pBeam->pev->origin = trace.vecEndPos;
	CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
	if ( hit && hit->pev->takedamage )
	{
		ClearMultiDamage();
		hit->TraceAttack( pev, gSkillData.pitWormDmgBeam, m_vecBeam, &trace, DMG_ENERGYBEAM );
		hit->TakeDamage( pev, pev, gSkillData.pitWormDmgBeam, DMG_ENERGYBEAM );
	}
	else if ( trace.flFraction != 1 )
	{
		UTIL_DecalTrace( &trace, RANDOM_LONG( 0, 4 ) );
		m_pBeam->DoSparks( trace.vecEndPos, trace.vecEndPos );
	}
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_ELIGHT );
		WRITE_SHORT( entindex() + 0x1000 );
		WRITE_COORD( eye.x );
		WRITE_COORD( eye.y );
		WRITE_COORD( eye.z );
		WRITE_COORD( 128 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 128 );
		WRITE_BYTE( 1 );
		WRITE_COORD( 2 );
	MESSAGE_END();
}

BOOL COFPitWormUp::ClawAttack()
{
	CBaseEntity* enemy = m_hEnemy;
	if ( m_hEnemy == NULL || pev->origin.z != m_posDesired.z )
		return FALSE;
	float distance = ( pev->origin - enemy->pev->origin ).Length2D();
	if ( !( m_flNextMeleeTime <= gpGlobals->time ) )
		return FALSE;
	if ( !FVisible( m_posTarget ) )
	{
		if ( distance >= 600 )
			return FALSE;
		Vector angles = UTIL_VecToAngles( ( m_posTarget - pev->origin ).Normalize() );
		float yaw = UTIL_AngleDiff( angles.y, pev->angles.y );
		if ( m_iLevel == 2 )
		{
			if ( yaw < 30 )
				return FALSE;
			pev->sequence = 5;
			m_flIdealHeadYaw = 0;
		}
		else if ( m_iLevel == 1 )
		{
			if ( yaw <= -30 )
				pev->sequence = 6;
			else if ( yaw >= 30 )
				pev->sequence = 7;
			else
				return FALSE;
			m_flIdealHeadYaw = 0;
		}
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAttackVoiceSounds[RANDOM_LONG( 0, 2 )], 1, 0.1f, 0, 100 );
		m_fLockHeight = m_fLockYaw = m_fAttacking = TRUE;
		return TRUE;
	}
	m_fLockYaw = FALSE;
	switch ( m_iLevel )
	{
	case 1:
	{
		Vector angles = UTIL_VecToAngles( ( m_posTarget - pev->origin ).Normalize() );
		float yaw = UTIL_AngleDiff( angles.y, pev->angles.y );
		if ( yaw >= 50 )
		{
			pev->sequence = 7;
			m_flIdealHeadYaw = 0;
			m_fLockYaw = TRUE;
		}
		else if ( yaw <= -30 )
			pev->sequence = 6;
		else
			pev->sequence = distance > 425 ? 8 : 4;
		break;
	}
	case 2:
	{
		Vector angles = UTIL_VecToAngles( ( m_posTarget - pev->origin ).Normalize() );
		float yaw = UTIL_AngleDiff( angles.y, pev->angles.y );
		if ( yaw >= 30 )
		{
			pev->sequence = 5;
			m_flIdealHeadYaw = 0;
			m_fLockYaw = TRUE;
		}
		else
			pev->sequence = distance > 425 || yaw <= -50 ? 8 : 2;
		break;
	}
	case 3:
		pev->sequence = distance > 425 ? 8 : 10 - ( RANDOM_LONG( 0, 1 ) != 0 );
		break;
	default:
		return FALSE;
	}
	if ( pev->sequence == 8 )
	{
		m_posBeam = m_posTarget;
		m_vecBeam = m_vecTarget;
		m_angleBeam = UTIL_VecToAngles( m_vecBeam );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "pitworm/pit_worm_attack_eyeblast.wav", 1, 0.1f, 0, 100 );
	}
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAttackVoiceSounds[RANDOM_LONG( 0, 2 )], 1, 0.1f, 0, 100 );
	m_fAttacking = m_fLockHeight = TRUE;
	return TRUE;
}

void COFPitWormUp::NextActivity()
{
	UTIL_MakeAimVectors( pev->angles );
	( m_posDesired - pev->origin ).Length();
	if ( m_hEnemy != NULL && !m_hEnemy->IsAlive() )
	{
		m_hEnemy = NULL;
		m_flIdealHeadYaw = 0;
	}
	if ( gpGlobals->time > m_flLastSeen + 15 && (CBaseEntity*)m_hEnemy != NULL &&
		( pev->origin - m_hEnemy->pev->origin ).Length2D() > 700 )
		m_hEnemy = NULL;
	if ( m_hEnemy == NULL )
	{
		Look( 4096 );
		m_hEnemy = BestVisibleEnemy();
		if ( m_hEnemy != NULL )
		{
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "pitworm/pit_worm_alert.wav", 1, 0.1f, 0, 100 );
			if ( !m_fFirstSighting )
			{
				pev->sequence = 3;
				m_fFirstSighting = TRUE;
				return;
			}
		}
	}
	if ( m_iWasHit == 1 )
	{
		pev->sequence = 12 - ( RANDOM_LONG( 0, 1 ) != 0 );
		m_iWasHit = 0;
		PainSound();
		m_fLockHeight = m_fLockYaw = m_fAttacking = FALSE;
	}
	else if ( pev->origin.z == m_posDesired.z )
	{
		float yaw = m_flIdealTorsoYaw - m_flTorsoYaw;
		if ( abs( (int)yaw ) > 10 || !ClawAttack() )
		{
			if ( RANDOM_LONG( 0, 2 ) == 0 )
				IdleSound();
			pev->sequence = 1;
			m_fLockHeight = m_fLockYaw = m_fAttacking = FALSE;
		}
	}
	else
	{
		if ( RANDOM_LONG( 0, 2 ) == 0 )
			IdleSound();
		pev->sequence = 0;
		m_fLockHeight = m_fLockYaw = m_fAttacking = FALSE;
	}
}

void COFPitWormUp::ChangeLevel()
{
	if ( pev->origin.z != m_posDesired.z )
	{
		if ( m_posDesired.z > pev->origin.z )
			pev->origin.z += min( m_posDesired.z - pev->origin.z, m_flLevelSpeed );
		else
			pev->origin.z -= min( pev->origin.z - m_posDesired.z, m_flLevelSpeed );
	}
	if ( m_flTorsoYaw != m_flIdealTorsoYaw )
	{
		if ( m_flIdealTorsoYaw > m_flTorsoYaw )
			m_flTorsoYaw += min( m_flIdealTorsoYaw - m_flTorsoYaw, 5.0f );
		else
			m_flTorsoYaw -= min( m_flTorsoYaw - m_flIdealTorsoYaw, 5.0f );
		SetBoneController( 2, m_flTorsoYaw );
	}
	if ( m_flHeadYaw != m_flIdealHeadYaw )
	{
		if ( m_flIdealHeadYaw > m_flHeadYaw )
			m_flHeadYaw += min( m_flIdealHeadYaw - m_flHeadYaw, 5.0f );
		else
			m_flHeadYaw -= min( m_flHeadYaw - m_flIdealHeadYaw, 5.0f );
		SetBoneController( 0, -m_flHeadYaw );
	}
	if ( m_flHeadPitch != m_flIdealHeadPitch )
	{
		if ( m_flIdealHeadPitch > m_flHeadPitch )
			m_flHeadPitch += min( m_flIdealHeadPitch - m_flHeadPitch, 5.0f );
		else
			m_flHeadPitch -= min( m_flHeadPitch - m_flIdealHeadPitch, 5.0f );
		SetBoneController( 1, m_flHeadPitch );
	}
}

void COFPitWormUp::TrackEnemy()
{
	CBaseEntity* enemy = m_hEnemy;
	Vector eye, angles;
	GetAttachment( 0, eye, angles );
	eye.x = pev->origin.x;
	eye.y = pev->origin.y;
	Vector target = enemy->pev->origin + enemy->pev->view_ofs - eye;
	Vector goal = UTIL_VecToAngles( target );
	float pitch = goal.x - pev->angles.x;
	if ( pitch < 0 )
		pitch += 360;
	pitch = -pitch;
	if ( pitch > 180 )
		pitch -= 360;
	if ( pitch < -180 )
		pitch += 360;
	m_flIdealHeadPitch = max( -45.0f, min( pitch, 45.0f ) );
	float yaw = VecToYaw( enemy->pev->origin + enemy->pev->view_ofs - eye ) - pev->angles.y;
	if ( yaw < 0 )
		yaw += 360;
	if ( yaw > 180 )
		yaw -= 360;
	if ( yaw < -180 )
		yaw += 360;
	float torsoYaw = m_flTorsoYaw;
	if ( !m_fLockYaw )
	{
		torsoYaw = yaw;
		if ( yaw < 0 )
			torsoYaw = max( yaw, m_iLevel == 1 ? -30.0f : -50.0f );
		if ( yaw > 0 )
			torsoYaw = min( yaw, m_iLevel == 2 ? 30.0f : 50.0f );
		m_flIdealTorsoYaw = torsoYaw;
	}
	float headYaw = max( -45.0f, min( torsoYaw - yaw, 45.0f ) );
	if ( !m_fAttacking || m_pBeam )
		m_flIdealHeadYaw = headYaw;
	if ( !m_fLockHeight )
	{
		float height = enemy->pev->origin.z;
		if ( !m_fTopLevelLocked && height > m_flTargetLevels[3] )
			m_iLevel = 3;
		else if ( height > m_flTargetLevels[2] )
			m_iLevel = 2;
		else if ( height > m_flTargetLevels[1] )
			m_iLevel = 1;
		else
			m_iLevel = 0;
		m_posDesired.z = m_flLevels[m_iLevel];
	}
}

void COFPitWormUp::DyingThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents( 0.1f );
	StudioFrameAdvance();
	if ( pev->deadflag == DEAD_NO )
	{
		pev->deadflag = DEAD_DYING;
		m_posDesired.z = m_flLevels[0];
		int direction = 1;
		pev->sequence = FindTransition( pev->sequence, 13, &direction );
		pev->frame = direction <= 0 ? 255.0f : 0.0f;
		ResetSequenceInfo();
		m_flLevelSpeed = 5;
		if ( m_fShockEffect )
		{
			pev->rendermode = m_iOldRenderMode;
			pev->renderfx = m_iOldRenderFX;
			pev->rendercolor = m_OldRenderColor;
			pev->renderamt = m_flOldRenderAmt;
			m_flShockDuration = 0;
			m_fShockEffect = FALSE;
		}
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "pitworm/pit_worm_death.wav", 1, 0.1f, 0, 100 );
		m_flDeathStartTime = gpGlobals->time;
		if ( m_pBeam )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
		if ( m_pSprite )
		{
			UTIL_Remove( m_pSprite );
			m_pSprite = NULL;
		}
		SetTouch( NULL );
		SetUse( NULL );
	}
	else if ( pev->deadflag == DEAD_DYING )
	{
		if ( gpGlobals->time - m_flDeathStartTime > 3 )
			ChangeLevel();
		if ( fabs( (double)( pev->origin.z - m_flLevels[0] ) ) < 16 )
		{
			pev->velocity = g_vecZero;
			pev->deadflag = DEAD_DEAD;
			SetThink( &CBaseEntity::SUB_Remove );
			pev->nextthink = gpGlobals->time + 0.1f;
		}
	}
}

void COFPitWormUp::HuntThink()
{
	pev->nextthink = gpGlobals->time + 0.1f;
	DispatchAnimEvents( 0.1f );
	StudioFrameAdvance();
	UpdateShockEffect();
	if ( m_pBeam )
	{
		if ( (CBaseEntity*)m_hEnemy != NULL && m_flBeamExpireTime > gpGlobals->time )
			StrafeBeam();
		else
		{
			UTIL_Remove( m_pBeam );
			UTIL_Remove( m_pSprite );
			m_pBeam = NULL;
			m_pSprite = NULL;
		}
	}
	if ( pev->health <= 0 )
	{
		SetThink( &COFPitWormUp::DyingThink );
		m_fSequenceFinished = TRUE;
		return;
	}
	if ( gpGlobals->time - m_flLastBlinkTime >= 6 && !m_pBeam )
	{
		float elapsed = gpGlobals->time - m_flLastBlinkTime;
		if ( elapsed >= RANDOM_FLOAT( 6, 9 ) )
		{
			pev->skin = 1;
			m_flLastBlinkInterval = m_flLastBlinkTime = gpGlobals->time;
		}
	}
	if ( pev->skin > 0 && gpGlobals->time - m_flLastBlinkInterval >= 0 )
	{
		pev->skin = pev->skin == 5 ? 0 : pev->skin + 1;
		m_flLastBlinkInterval = gpGlobals->time;
	}
	if ( m_iWasHit == 1 )
	{
		int direction = 1;
		int sequence = 12 - ( RANDOM_LONG( 0, 1 ) != 0 );
		pev->sequence = FindTransition( pev->sequence, sequence, &direction );
		pev->frame = direction <= 0 ? 255.0f : 0.0f;
		ResetSequenceInfo();
		m_iWasHit = 0;
		PainSound();
	}
	else if ( m_fSequenceFinished )
	{
		int sequence = pev->sequence;
		if ( m_fAttacking )
		{
			m_fLockHeight = m_fLockYaw = m_fAttacking = FALSE;
			m_flNextMeleeTime = gpGlobals->time + 0.25f;
		}
		NextActivity();
		if ( !m_fSequenceLoops || pev->sequence != sequence )
		{
			pev->frame = 0;
			ResetSequenceInfo();
		}
	}
	if ( m_hEnemy != NULL && FVisible( m_hEnemy ) )
	{
		if ( gpGlobals->time - 5 > m_flLastSeen )
			m_flPrevSeen = gpGlobals->time;
		m_flLastSeen = gpGlobals->time;
		m_posTarget = m_hEnemy->pev->origin;
		m_posTarget.z += 24;
		Vector eye, angles;
		GetAttachment( 0, eye, angles );
		m_vecTarget = ( m_posTarget - eye ).Normalize();
		m_vecDesired = m_vecTarget;
	}
	if ( m_posDesired.z > m_flLevels[3] )
		m_posDesired.z = m_flLevels[3];
	if ( m_posDesired.z < m_flLevels[0] )
		m_posDesired.z = m_flLevels[0];
	ChangeLevel();
	if ( (CBaseEntity*)m_hEnemy != NULL && !m_pBeam )
		TrackEnemy();
}
