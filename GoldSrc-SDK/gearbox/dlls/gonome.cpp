#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "weapons.h"
#include "player.h"

#define GONOME_AE_ATTACK_RIGHT	1
#define GONOME_AE_ATTACK_LEFT		2
#define GONOME_AE_GUTS_GRAB		3
#define GONOME_AE_GUTS_THROW		4
#define GONOME_AE_BITE1			19
#define GONOME_AE_BITE2			20
#define GONOME_AE_BITE3			21
#define GONOME_AE_BITE4			22

#define GONOME_FLINCH_DELAY		2

enum
{
	TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1,
};

class COFGonomeGuts : public CBaseEntity
{
public:
	void Spawn();
	void Touch( CBaseEntity* pOther );
	void EXPORT Animate();
	int Save( CSave& save );
	int Restore( CRestore& restore );

	static void Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );
	void Launch( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity );
	void SetAttachment( edict_t* pEntity, int iAttachment )
	{
		if ( pEntity )
		{
			pev->skin = ENTINDEX( pEntity );
			pev->body = iAttachment;
			pev->aiment = pEntity;
			pev->movetype = MOVETYPE_FOLLOW;
		}
	}
	void ClearAttachment()
	{
		pev->skin = 0;
		pev->body = 0;
		pev->aiment = NULL;
		pev->movetype = MOVETYPE_FLY;
	}
	static COFGonomeGuts* GonomeGutsCreate( const Vector& origin );
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFGonome : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	int IgnoreConditions();
	void PainSound();
	void AlertSound();
	void IdleSound();
	void SetActivity( Activity NewActivity );
	void StartTask( Task_t* pTask );
	Schedule_t* GetScheduleOfType( int Type );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float, float ) { return FALSE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t* pevAttacker, int iGib );
	int Save( CSave& save );
	int Restore( CRestore& restore );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAlertSounds[];
	static const char* pIdleSounds[];
	static const char* pPainSounds[];

	float			m_flNextFlinch;
	float			m_flNextThrowTime;
	COFGonomeGuts*	m_pGonomeGuts;
	BOOL			m_fPlayerLocked;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CDeadGonome : public CBaseMonster
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	int Classify() { return CLASS_ALIEN_PASSIVE; }

	static const char* m_szPoses[];
	int m_iPose;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( gonomeguts, COFGonomeGuts );
LINK_ENTITY_TO_CLASS( monster_gonome, COFGonome );
LINK_ENTITY_TO_CLASS( monster_gonome_dead, CDeadGonome );

TYPEDESCRIPTION COFGonomeGuts::m_SaveData[] =
{
	DEFINE_FIELD( COFGonomeGuts, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COFGonomeGuts, CBaseEntity );

TYPEDESCRIPTION COFGonome::m_SaveData[] =
{
	DEFINE_FIELD( COFGonome, m_flNextFlinch, FIELD_TIME ),
	DEFINE_FIELD( COFGonome, m_flNextThrowTime, FIELD_TIME ),
	DEFINE_FIELD( COFGonome, m_fPlayerLocked, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( COFGonome, CBaseMonster );

const char* COFGonome::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

const char* COFGonome::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* COFGonome::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav",
};

const char* COFGonome::pIdleSounds[] =
{
	"gonome/gonome_idle1.wav",
	"gonome/gonome_idle2.wav",
	"gonome/gonome_idle3.wav",
};

const char* COFGonome::pPainSounds[] =
{
	"gonome/gonome_pain1.wav",
	"gonome/gonome_pain2.wav",
	"gonome/gonome_pain3.wav",
	"gonome/gonome_pain4.wav",
};

const char* CDeadGonome::m_szPoses[] =
{
	"dead_on_stomach1",
	"dead_on_back",
	"dead_on_side",
};

void COFGonomeGuts::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "gonomeguts" );
	pev->solid = SOLID_BBOX;
	pev->rendermode = kRenderTransAlpha;
	pev->renderamt = 255;

	if ( g_Language == LANGUAGE_GERMAN )
	{
		SET_MODEL( ENT( pev ), "sprites/bigspit.spr" );
		pev->rendercolor.x = 0;
		pev->rendercolor.x = 255;
		pev->rendercolor.x = 0;
	}
	else
	{
		SET_MODEL( ENT( pev ), "sprites/bigspit.spr" );
		pev->rendercolor.x = 128;
		pev->rendercolor.x = 32;
		pev->rendercolor.x = 128;
	}

	pev->frame = 0;
	pev->scale = 0.5;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_maxFrame = (int)( (float)MODEL_FRAMES( pev->modelindex ) - 1 );
}

void COFGonomeGuts::Animate()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pev->frame++ && pev->frame > m_maxFrame )
		pev->frame = 0;
}

void COFGonomeGuts::Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	COFGonomeGuts* pGuts = GetClassPtr( (COFGonomeGuts*)NULL );
	pGuts->Spawn();
	UTIL_SetOrigin( pGuts->pev, vecStart );
	pGuts->pev->velocity = vecVelocity;
	pGuts->pev->owner = ENT( pevOwner );

	if ( pGuts->m_maxFrame > 0 )
	{
		pGuts->SetThink( &COFGonomeGuts::Animate );
		pGuts->pev->nextthink = gpGlobals->time + 0.1;
	}
}

void COFGonomeGuts::Launch( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity )
{
	UTIL_SetOrigin( pev, vecStart );
	pev->velocity = vecVelocity;
	pev->owner = ENT( pevOwner );
	SetThink( &COFGonomeGuts::Animate );
	pev->nextthink = gpGlobals->time + 0.1;
}

COFGonomeGuts* COFGonomeGuts::GonomeGutsCreate( const Vector& origin )
{
	COFGonomeGuts* pGuts = GetClassPtr( (COFGonomeGuts*)NULL );
	pGuts->Spawn();
	pGuts->pev->origin = origin;
	return pGuts;
}

void COFGonomeGuts::Touch( CBaseEntity* pOther )
{
	int pitch = (int)RANDOM_FLOAT( 90, 110 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "bullchicken/bc_acid1.wav", 1, ATTN_NORM, 0, pitch );

	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit1.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "bullchicken/bc_spithit2.wav", 1, ATTN_NORM, 0, pitch );
		break;
	}

	if ( !pOther->pev->takedamage )
	{
		TraceResult tr;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, ENT( pev ), &tr );
		UTIL_BloodDecalTrace( &tr, BLOOD_COLOR_RED );
		UTIL_BloodDrips( tr.vecEndPos, tr.vecPlaneNormal, BLOOD_COLOR_RED, 35 );
	}
	else
	{
		pOther->TakeDamage( pev, pev, gSkillData.gonomeDmgGuts, DMG_GENERIC );
	}

	SetThink( &COFGonomeGuts::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

int COFGonome::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void COFGonome::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

void COFGonome::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/gonome.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.gonomeHealth;
	pev->view_ofs = VEC_VIEW;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	m_flFieldOfView = 0.5;
	m_flNextThrowTime = gpGlobals->time;
	m_pGonomeGuts = NULL;
	m_fPlayerLocked = FALSE;
	MonsterInit();
}

void COFGonome::Precache()
{
	PRECACHE_MODEL( "models/gonome.mdl" );
	PRECACHE_MODEL( "sprites/bigspit.spr" );

	for ( int i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackHitSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackMissSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char*)pAlertSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char*)pIdleSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char*)pPainSounds[i] );

	PRECACHE_SOUND( "gonome/gonome_death2.wav" );
	PRECACHE_SOUND( "gonome/gonome_death3.wav" );
	PRECACHE_SOUND( "gonome/gonome_death4.wav" );
	PRECACHE_SOUND( "gonome/gonome_jumpattack.wav" );
	PRECACHE_SOUND( "gonome/gonome_melee1.wav" );
	PRECACHE_SOUND( "gonome/gonome_melee2.wav" );
	PRECACHE_SOUND( "gonome/gonome_run.wav" );
	PRECACHE_SOUND( "gonome/gonome_eat.wav" );
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}

void COFGonome::IdleSound()
{
	RANDOM_LONG( 0, 9 );
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	int sound = RANDOM_LONG( 0, ARRAYSIZE( pIdleSounds ) - 1 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pIdleSounds[sound], 1, ATTN_NORM, 0, pitch );
}

void COFGonome::AlertSound()
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );
	int sound = RANDOM_LONG( 0, ARRAYSIZE( pAlertSounds ) - 1 );
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAlertSounds[sound], 1, ATTN_NORM, 0, pitch );
}

void COFGonome::PainSound()
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );
	if ( RANDOM_LONG( 0, 5 ) < 2 )
	{
		int sound = RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 );
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[sound], 1, ATTN_NORM, 0, pitch );
	}
}

int COFGonome::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.15;
	}

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void COFGonome::Killed( entvars_t* pevAttacker, int iGib )
{
	if ( m_pGonomeGuts )
	{
		UTIL_Remove( m_pGonomeGuts );
		m_pGonomeGuts = NULL;
	}

	if ( m_fPlayerLocked )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer && pPlayer->IsAlive() )
			pPlayer->EnableControl( TRUE );
		m_fPlayerLocked = FALSE;
	}

	CBaseMonster::Killed( pevAttacker, iGib );
}

BOOL COFGonome::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( flDist > 64 || flDot < 0.7 )
		return FALSE;
	return m_hEnemy && ( m_hEnemy->pev->flags & FL_ONGROUND );
}

BOOL COFGonome::CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist < 256 )
		return FALSE;
	if ( IsMoving() && flDist >= 512 )
		return FALSE;

	if ( flDist > 64 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextThrowTime )
	{
		if ( m_hEnemy && !( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) <= 256 ) )
			return FALSE;

		if ( IsMoving() )
			m_flNextThrowTime = gpGlobals->time + 5;
		else
			m_flNextThrowTime = gpGlobals->time + 0.5;
		return TRUE;
	}

	return FALSE;
}

int COFGonome::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	if ( m_Activity == ACT_RANGE_ATTACK1 )
		return iIgnore | bits_COND_ENEMY_OCCLUDED | bits_COND_ENEMY_TOOFAR |
			bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE;

	if ( m_Activity == ACT_MELEE_ATTACK1 )
	{
		if ( m_flNextFlinch >= gpGlobals->time )
			return iIgnore | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE;
	}
	else if ( m_Activity == ACT_SMALL_FLINCH || m_Activity == ACT_BIG_FLINCH )
	{
		if ( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + GONOME_FLINCH_DELAY;
	}

	return iIgnore;
}

void COFGonome::SetActivity( Activity NewActivity )
{
	GET_MODEL_PTR( ENT( pev ) );
	if ( NewActivity != ACT_RANGE_ATTACK1 && m_pGonomeGuts )
	{
		UTIL_Remove( m_pGonomeGuts );
		m_pGonomeGuts = NULL;
	}

	if ( m_fPlayerLocked && NewActivity != ACT_MELEE_ATTACK1 )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( NULL, "player" );
		if ( pPlayer && pPlayer->IsAlive() )
			pPlayer->EnableControl( TRUE );
		m_fPlayerLocked = FALSE;
	}

	int iSequence;
	if ( NewActivity == ACT_RUN )
	{
		if ( m_hEnemy )
		{
			if ( ( pev->origin - m_hEnemy->pev->origin ).Length2D() <= 512 )
				iSequence = LookupSequence( "runshort" );
			else
				iSequence = LookupSequence( "runlong" );
		}
		else
		{
			iSequence = LookupActivity( ACT_RUN );
		}
	}
	else if ( NewActivity == ACT_MELEE_ATTACK1 )
	{
		if ( m_hEnemy )
		{
			if ( ( pev->origin - m_hEnemy->pev->origin ).Length2D() >= 48 )
				iSequence = LookupSequence( "attack1" );
			else
				iSequence = LookupSequence( "attack2" );
		}
		else
		{
			iSequence = LookupActivity( ACT_MELEE_ATTACK1 );
		}
	}
	else
	{
		CBaseMonster::SetActivity( NewActivity );
		return;
	}

	if ( iSequence < 0 )
	{
		ALERT( at_aiconsole, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;
	}
	else
	{
		if ( ( pev->sequence != iSequence || !m_fSequenceLoops ) &&
			!( ( m_Activity == ACT_WALK || m_Activity == ACT_RUN ) &&
			( NewActivity == ACT_WALK || NewActivity == ACT_RUN ) ) )
		{
			pev->frame = 0;
		}

		pev->sequence = iSequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}

	m_Activity = NewActivity;
	m_IdealActivity = NewActivity;
}

void COFGonome::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	CBaseEntity* pHurt;
	switch ( pEvent->event )
	{
	case GONOME_AE_ATTACK_RIGHT:
		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.gonomeDmgOneSlash, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.z = -9;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 25;
		}
		break;

	case GONOME_AE_ATTACK_LEFT:
		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.gonomeDmgOneSlash, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.z = 9;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 25;
		}
		break;

	case GONOME_AE_GUTS_GRAB:
		if ( m_hEnemy )
		{
			Vector vecGutsPos, vecGutsDir;
			GetAttachment( 0, vecGutsPos, vecGutsDir );
			if ( !m_pGonomeGuts )
				m_pGonomeGuts = COFGonomeGuts::GonomeGutsCreate( vecGutsPos );

			m_pGonomeGuts->SetAttachment( edict(), 1 );

			vecGutsDir = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecGutsPos ).Normalize();
			vecGutsDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecGutsDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecGutsDir.z += RANDOM_FLOAT( -0.05, 0 );
			UTIL_BloodDrips( vecGutsPos, vecGutsDir, BLOOD_COLOR_RED, 35 );
		}
		return;

	case GONOME_AE_GUTS_THROW:
		if ( m_hEnemy )
		{
			Vector vecGutsPos, vecGutsDir;
			GetAttachment( 0, vecGutsPos, vecGutsDir );
			UTIL_MakeVectors( pev->angles );
			if ( !m_pGonomeGuts )
				m_pGonomeGuts = COFGonomeGuts::GonomeGutsCreate( vecGutsPos );

			vecGutsDir = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecGutsPos ).Normalize();
			vecGutsDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecGutsDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecGutsDir.z += RANDOM_FLOAT( -0.05, 0 );
			UTIL_BloodDrips( vecGutsPos, vecGutsDir, BLOOD_COLOR_RED, 35 );
			m_pGonomeGuts->ClearAttachment();
			m_pGonomeGuts->Launch( pev, vecGutsPos, vecGutsDir * 900 );
		}
		else if ( m_pGonomeGuts )
		{
			UTIL_Remove( m_pGonomeGuts );
		}
		m_pGonomeGuts = NULL;
		return;

	case GONOME_AE_BITE1:
	case GONOME_AE_BITE2:
	case GONOME_AE_BITE3:
		if ( ( pev->origin - m_hEnemy->pev->origin ).Length2D() < 48 )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( NULL, "player" );
			if ( pPlayer && pPlayer->IsAlive() )
				pPlayer->EnableControl( FALSE );
			m_fPlayerLocked = TRUE;
		}

		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.gonomeDmgOneBite, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.x = 9;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 25;
		}
		break;

	case GONOME_AE_BITE4:
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( NULL, "player" );
			if ( pPlayer && pPlayer->IsAlive() )
				pPlayer->EnableControl( TRUE );
			m_fPlayerLocked = FALSE;
		}

		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.gonomeDmgOneBite, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 75;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		return;
	}

	int pitch = 100 + RANDOM_LONG( -5, 5 );
	const char* sound;
	if ( pHurt )
		sound = pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )];
	else
		sound = pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )];
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, sound, 1, ATTN_NORM, 0, pitch );
}

Task_t tlGonomeVictoryDance[] =
{
	{ TASK_STOP_MOVING,					0 },
	{ TASK_SET_FAIL_SCHEDULE,				(float)SCHED_FAIL },
	{ TASK_WAIT,							0.2f },
	{ TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE,	0 },
	{ TASK_WALK_PATH,						0 },
	{ TASK_WAIT_FOR_MOVEMENT,				0 },
	{ TASK_FACE_ENEMY,						0 },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE,					(float)ACT_VICTORY_DANCE },
};

Schedule_t slGonomeVictoryDance[] =
{
	{
		tlGonomeVictoryDance,
		ARRAYSIZE( tlGonomeVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		0,
		"VoltigoreVictoryDance"
	},
};

DEFINE_CUSTOM_SCHEDULES( COFGonome )
{
	slGonomeVictoryDance,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFGonome, CBaseMonster );

Schedule_t* COFGonome::GetScheduleOfType( int Type )
{
	if ( Type == SCHED_VICTORY_DANCE )
		return slGonomeVictoryDance;
	return CBaseMonster::GetScheduleOfType( Type );
}

void COFGonome::StartTask( Task_t* pTask )
{
	if ( pTask->iTask != TASK_GONOME_GET_PATH_TO_ENEMY_CORPSE )
	{
		CBaseMonster::StartTask( pTask );
		return;
	}

	if ( m_pGonomeGuts )
	{
		UTIL_Remove( m_pGonomeGuts );
		m_pGonomeGuts = NULL;
	}
	UTIL_MakeVectors( pev->angles );
	if ( BuildRoute( m_vecEnemyLKP - gpGlobals->v_forward * 64, bits_MF_TO_LOCATION, NULL ) )
		TaskComplete();
	else
	{
		ALERT( at_aiconsole, "GonomeGetPathToEnemyCorpse failed!!\n" );
		TaskFail();
	}
}

void CDeadGonome::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

void CDeadGonome::Spawn()
{
	PRECACHE_MODEL( "models/gonome.mdl" );
	SET_MODEL( ENT( pev ), "models/gonome.mdl" );
	pev->effects = 0;
	pev->sequence = 0;
	pev->health = 8;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead gonome with bad pose\n" );
	MonsterInitDead();
}
