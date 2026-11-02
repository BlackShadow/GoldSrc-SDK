#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "schedule.h"
#include "effects.h"
#include "soundent.h"
#include "weapons.h"

#define BABYVOLTIGORE_AE_FOOTSTEP_LEFT	10
#define BABYVOLTIGORE_AE_FOOTSTEP_RIGHT	11
#define BABYVOLTIGORE_AE_PUNCH_LEFT		12
#define BABYVOLTIGORE_AE_PUNCH_RIGHT		13
#define BABYVOLTIGORE_AE_RUN_GRUNT		14

enum
{
	SCHED_BABYVOLTIGORE_THREAT_DISPLAY = LAST_COMMON_SCHEDULE + 1
};

enum
{
	TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1
};

class COFBabyVoltigore : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void SetObjectCollisionBox() override;
	int Classify() override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
		int bitsDamageType ) override;
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	int IRelationship( CBaseEntity* pTarget ) override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	BOOL CheckMeleeAttack1( float flDot, float flDist ) override;
	void StartTask( Task_t* pTask ) override;
	void RunTask( Task_t* pTask ) override;
	Schedule_t* GetScheduleOfType( int Type ) override;
	Schedule_t* GetSchedule() override;
	void SetYawSpeed() override;
	BOOL FCanCheckAttacks() override;
	int ISoundMask() override;
	void DeathSound() override;
	void AlertSound() override;
	void PainSound() override;

	void AttackSound();
	void StopTalking();
	BOOL ShouldSpeak();
	CBaseEntity* CheckTraceHullAttack( float flDist, int iDamage, int iDmgType );
	void ClearBeams();

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pDieSounds[];
	static const char* pAttackSounds[];
	static const char* pAttackMissSounds[];
	static const char* pAttackHitSounds[];

	CBeam*		m_pBeam[8];
	int			m_iBeams;
	float		m_flNextBeamAttackCheck;
	float		m_flNextPainTime;
	float		m_flNextSpeakTime;
	float		m_flNextWordTime;
	int			m_iLastWord;
	int			m_iBabyVoltigoreGibs;
	BOOL		m_fDeathCharge;
	float		m_flDeathStartTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_alien_babyvoltigore, COFBabyVoltigore );

TYPEDESCRIPTION COFBabyVoltigore::m_SaveData[] =
{
	DEFINE_FIELD( COFBabyVoltigore, m_flNextBeamAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextSpeakTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_flNextWordTime, FIELD_TIME ),
	DEFINE_FIELD( COFBabyVoltigore, m_iLastWord, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COFBabyVoltigore, CSquadMonster );

const char* COFBabyVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav",
};

const char* COFBabyVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav",
};

const char* COFBabyVoltigore::pDieSounds[] =
{
	"voltigore/voltigore_die1.wav",
	"voltigore/voltigore_die2.wav",
	"voltigore/voltigore_die3.wav",
};

const char* COFBabyVoltigore::pAttackSounds[] =
{
	"voltigore/voltigore_attack_melee1.wav",
	"voltigore/voltigore_attack_melee2.wav",
};

const char* COFBabyVoltigore::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav",
};

const char* COFBabyVoltigore::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav",
};

int iBabyVoltigoreMuzzleFlash;

Task_t tlBabyVoltigoreFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_WAIT_PVS, 0 },
};

Schedule_t slBabyVoltigoreFail[] =
{
	{
		tlBabyVoltigoreFail, ARRAYSIZE( tlBabyVoltigoreFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1,
		0, "BabyVoltigore Fail"
	},
};

Task_t tlBabyVoltigoreCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
	{ TASK_WAIT_PVS, 0 },
};

Schedule_t slBabyVoltigoreCombatFail[] =
{
	{
		tlBabyVoltigoreCombatFail, ARRAYSIZE( tlBabyVoltigoreCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1,
		0, "BabyVoltigore Combat Fail"
	},
};

Task_t tlBabyVoltigoreStandoff[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
};

Schedule_t slBabyVoltigoreStandoff[] =
{
	{
		tlBabyVoltigoreStandoff, ARRAYSIZE( tlBabyVoltigoreStandoff ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1 | bits_COND_SEE_ENEMY |
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "BabyVoltigore Standoff"
	},
};

Task_t tlBabyVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_RANGE_ATTACK1_NOTURN, 0 },
	{ TASK_WAIT, 0.5 },
};

Schedule_t slBabyVoltigoreRangeAttack1[] =
{
	{
		tlBabyVoltigoreRangeAttack1, ARRAYSIZE( tlBabyVoltigoreRangeAttack1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_ENEMY_OCCLUDED,
		0, "BabyVoltigore Range Attack1"
	},
};

Task_t tlBabyVoltigoreTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_WAIT, 0.2 },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 },
};

Schedule_t slBabyVoltigoreTakeCoverFromEnemy[] =
{
	{
		tlBabyVoltigoreTakeCoverFromEnemy, ARRAYSIZE( tlBabyVoltigoreTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY,
		0, "BabyVoltigoreTakeCoverFromEnemy"
	},
};

Task_t tlBabyVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_BABYVOLTIGORE_THREAT_DISPLAY },
	{ TASK_WAIT, 0.2 },
	{ TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_CROUCH },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_STAND },
	{ TASK_PLAY_SEQUENCE, ACT_THREAT_DISPLAY },
	{ TASK_PLAY_SEQUENCE, ACT_CROUCH },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE },
	{ TASK_PLAY_SEQUENCE, ACT_STAND },
};

Schedule_t slBabyVoltigoreVictoryDance[] =
{
	{
		tlBabyVoltigoreVictoryDance, ARRAYSIZE( tlBabyVoltigoreVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		0, "BabyVoltigoreVictoryDance"
	},
};

Task_t tlBabyVoltigoreThreatDisplay[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_THREAT_DISPLAY },
};

Schedule_t slBabyVoltigoreThreatDisplay[] =
{
	{
		tlBabyVoltigoreThreatDisplay, ARRAYSIZE( tlBabyVoltigoreThreatDisplay ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "BabyVoltigoreThreatDisplay"
	},
};

DEFINE_CUSTOM_SCHEDULES( COFBabyVoltigore )
{
	slBabyVoltigoreFail,
	slBabyVoltigoreCombatFail,
	slBabyVoltigoreStandoff,
	slBabyVoltigoreRangeAttack1,
	slBabyVoltigoreTakeCoverFromEnemy,
	slBabyVoltigoreVictoryDance,
	slBabyVoltigoreThreatDisplay,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFBabyVoltigore, CSquadMonster );

int COFBabyVoltigore::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

int COFBabyVoltigore::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void COFBabyVoltigore::SetYawSpeed()
{
	if ( m_Activity == ACT_TURN_LEFT || m_Activity == ACT_TURN_RIGHT )
		pev->yaw_speed = 80;
	else
		pev->yaw_speed = 70;
}

BOOL COFBabyVoltigore::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

BOOL COFBabyVoltigore::CheckMeleeAttack1( float flDot, float flDist )
{
	return HasConditions( bits_COND_SEE_ENEMY ) && flDist <= 64 && flDot >= 0.6 && m_hEnemy != NULL;
}

void COFBabyVoltigore::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/baby_voltigore.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 32 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = gSkillData.babyvoltigoreHealth;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.2;
	m_HackedGunPos = Vector( 24, 64, 48 );
	StopTalking();
	m_flNextBeamAttackCheck = gpGlobals->time;
	m_fDeathCharge = FALSE;
	m_flDeathStartTime = gpGlobals->time;
	MonsterInit();
}

void COFBabyVoltigore::Precache()
{
	PRECACHE_MODEL( "models/baby_voltigore.mdl" );
	for ( int i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackHitSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackMissSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char*)pPainSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char*)pAlertSounds[i] );
	PRECACHE_SOUND( "voltigore/voltigore_attack_melee1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_attack_melee2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_attack_shock.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_communicate1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_communicate2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_communicate3.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_die1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_die2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_die3.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep3.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt2.wav" );
	PRECACHE_SOUND( "hassault/hw_shoot1.wav" );
	PRECACHE_SOUND( "debris/beamstart2.wav" );
	iBabyVoltigoreMuzzleFlash = PRECACHE_MODEL( "sprites/muz4.spr" );
	UTIL_PrecacheOther( "charged_bolt" );
	m_iBabyVoltigoreGibs = PRECACHE_MODEL( "models/vgibs.mdl" );
}

void COFBabyVoltigore::AlertSound()
{
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pAlertSounds ) - 1 )],
		1, ATTN_NORM, 0, 180 );
}

void COFBabyVoltigore::DeathSound()
{
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pDieSounds[RANDOM_LONG( 0, ARRAYSIZE( pDieSounds ) - 1 )],
		1, ATTN_NORM, 0, 180 );
}

void COFBabyVoltigore::AttackSound()
{
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackSounds ) - 1 )],
		1, ATTN_NORM, 0, 180 );
}

void COFBabyVoltigore::PainSound()
{
	if ( m_flNextPainTime > gpGlobals->time )
		return;

	m_flNextPainTime = gpGlobals->time + 0.6;
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )],
		1, ATTN_NORM, 0, 180 );
}

void COFBabyVoltigore::StopTalking()
{
	m_flNextWordTime = m_flNextSpeakTime = gpGlobals->time + 10 + RANDOM_LONG( 0, 10 );
}

BOOL COFBabyVoltigore::ShouldSpeak()
{
	if ( m_flNextSpeakTime > gpGlobals->time )
		return FALSE;

	if ( ( pev->spawnflags & SF_MONSTER_GAG ) && m_MonsterState != MONSTERSTATE_COMBAT )
	{
		m_flNextSpeakTime = gpGlobals->time + 3;
		return FALSE;
	}
	return TRUE;
}

void COFBabyVoltigore::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 32 );
}

void COFBabyVoltigore::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
	int bitsDamageType )
{
	if ( bitsDamageType & DMG_SHOCK )
		return;

	if ( ptr->iHitgroup == 10 && ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) ) )
	{
		if ( pev->dmgtime != gpGlobals->time || RANDOM_LONG( 0, 10 ) < 1 )
		{
			UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
			pev->dmgtime = gpGlobals->time;
		}
		if ( RANDOM_LONG( 0, 1 ) == 0 )
		{
			Vector vecTracerDir = vecDir;
			vecTracerDir.x += RANDOM_FLOAT( -0.3, 0.3 );
			vecTracerDir.y += RANDOM_FLOAT( -0.3, 0.3 );
			vecTracerDir.z += RANDOM_FLOAT( -0.3, 0.3 );
			vecTracerDir = vecTracerDir * -512;
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, ptr->vecEndPos );
			WRITE_BYTE( TE_TRACER );
			WRITE_COORD( ptr->vecEndPos.x );
			WRITE_COORD( ptr->vecEndPos.y );
			WRITE_COORD( ptr->vecEndPos.z );
			WRITE_COORD( vecTracerDir.x );
			WRITE_COORD( vecTracerDir.y );
			WRITE_COORD( vecTracerDir.z );
			MESSAGE_END();
		}
		flDamage -= 20;
		if ( flDamage <= 0 )
			flDamage = 0.1;
	}
	else
	{
		SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );
		TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
	}
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

int COFBabyVoltigore::IRelationship( CBaseEntity* pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
		return R_NM;
	return CBaseMonster::IRelationship( pTarget );
}

BOOL COFBabyVoltigore::CheckRangeAttack1( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
		return FALSE;
	if ( flDist < 64 || flDist > 1024 || flDot < 0.5 || gpGlobals->time < m_flNextBeamAttackCheck )
		return FALSE;

	UTIL_MakeVectors( pev->angles );
	Vector vecArmPos, vecArmAng;
	GetAttachment( 4, vecArmPos, vecArmAng );
	TraceResult tr;
	UTIL_TraceLine( vecArmPos, m_hEnemy->BodyTarget( vecArmPos ), dont_ignore_monsters, ENT( pev ), &tr );
	if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
	{
		m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT( 20, 30 );
		return TRUE;
	}
	m_flNextBeamAttackCheck = gpGlobals->time + 0.2;
	return FALSE;
}

CBaseEntity* COFBabyVoltigore::CheckTraceHullAttack( float flDist, int iDamage, int iDmgType )
{
	if ( IsPlayer() )
		UTIL_MakeVectors( pev->angles );
	else
		UTIL_MakeAimVectors( pev->angles );

	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z;
	Vector vecEnd = vecStart + gpGlobals->v_forward * flDist;
	TraceResult tr;
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, ENT( pev ), &tr );
	if ( !tr.pHit )
		return NULL;

	CBaseEntity* pHurt = CBaseEntity::Instance( tr.pHit );
	if ( iDamage > 0 )
		pHurt->TakeDamage( pev, pev, (float)iDamage, iDmgType );
	return pHurt;
}

void COFBabyVoltigore::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	PRECACHE_SOUND( "voltigore/voltigore_footstep1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep2.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_footstep3.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt1.wav" );
	PRECACHE_SOUND( "voltigore/voltigore_run_grunt2.wav" );

	switch ( pEvent->event )
	{
	case BABYVOLTIGORE_AE_FOOTSTEP_LEFT:
	case BABYVOLTIGORE_AE_FOOTSTEP_RIGHT:
		{
			int sound = RANDOM_LONG( 0, 2 );
			if ( sound == 0 )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep1.wav", 1, ATTN_IDLE, 0, 130 );
			else if ( sound == 1 )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep2.wav", 1, ATTN_IDLE, 0, 130 );
			else if ( sound == 2 )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_BODY, "voltigore/voltigore_footstep3.wav", 1, ATTN_IDLE, 0, 130 );
		}
		break;
	case BABYVOLTIGORE_AE_PUNCH_LEFT:
	case BABYVOLTIGORE_AE_PUNCH_RIGHT:
		{
			CBaseEntity* pHurt = CheckTraceHullAttack( 64, (int)gSkillData.babyvoltigoreDmgPunch, DMG_CLUB );
			if ( pHurt )
			{
				pHurt->pev->punchangle.y = pEvent->event == BABYVOLTIGORE_AE_PUNCH_LEFT ? -25.0f : 25.0f;
				pHurt->pev->punchangle.x = 8;
				if ( pHurt->IsPlayer() )
				{
					float flPush = pEvent->event == BABYVOLTIGORE_AE_PUNCH_LEFT ? 50.0f : -50.0f;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * flPush;
				}
				int pitch = 130 + RANDOM_LONG( -5, 5 );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON,
					pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )], 1, ATTN_NORM, 0, pitch );
				Vector vecArmPos, vecArmAng;
				GetAttachment( 0, vecArmPos, vecArmAng );
				SpawnBlood( vecArmPos, pHurt->BloodColor(), 25 );
			}
			else
			{
				int pitch = 130 + RANDOM_LONG( -5, 5 );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON,
					pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )], 1, ATTN_NORM, 0, pitch );
			}
		}
		break;
	case BABYVOLTIGORE_AE_RUN_GRUNT:
		{
			int sound = RANDOM_LONG( 0, 1 );
			if ( sound == 0 )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "voltigore/voltigore_run_grunt1.wav", 1, ATTN_NORM, 0, 180 );
			else if ( sound == 1 )
				EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "voltigore/voltigore_run_grunt2.wav", 1, ATTN_NORM, 0, 180 );
		}
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void COFBabyVoltigore::ClearBeams()
{
	for ( int i = 0; i < ARRAYSIZE( m_pBeam ); i++ )
	{
		if ( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;
}

void COFBabyVoltigore::Killed( entvars_t* pevAttacker, int iGib )
{
	ClearBeams();
	CSquadMonster::Killed( pevAttacker, iGib );
}

void COFBabyVoltigore::StartTask( Task_t* pTask )
{
	ClearBeams();
	switch ( pTask->iTask )
	{
	case TASK_RANGE_ATTACK1_NOTURN:
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecConverge = gpGlobals->v_up * 32 + gpGlobals->v_forward * 50 + pev->origin;
			for ( int i = 1; i <= 3; i++ )
			{
				m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 50 );
				if ( !m_pBeam[m_iBeams] )
					return;
				m_pBeam[m_iBeams]->PointEntInit( vecConverge, entindex() );
				m_pBeam[m_iBeams]->SetEndAttachment( i );
				m_pBeam[m_iBeams]->SetColor( 180, 16, 255 );
				m_pBeam[m_iBeams]->SetBrightness( 255 );
				m_pBeam[m_iBeams]->SetNoise( 20 );
				m_iBeams++;
			}
			UTIL_EmitAmbientSound( ENT( pev ), pev->origin, "debris/beamstart2.wav", 0.5, ATTN_NORM, 0,
				RANDOM_LONG( 140, 160 ) );
			CBaseMonster::StartTask( pTask );
		}
		break;
	case TASK_BABYVOLTIGORE_GET_PATH_TO_ENEMY_CORPSE:
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecGoal = m_vecEnemyLKP - gpGlobals->v_forward * 64;
			if ( BuildRoute( vecGoal, bits_MF_TO_LOCATION, NULL ) )
				TaskComplete();
			else
			{
				ALERT( at_aiconsole, "BabyVoltigoreGetPathToEnemyCorpse failed!!\n" );
				TaskFail();
			}
		}
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void COFBabyVoltigore::RunTask( Task_t* pTask )
{
	if ( pTask->iTask != TASK_DIE )
	{
		CBaseMonster::RunTask( pTask );
		return;
	}
	if ( !m_fSequenceFinished || pev->frame < 255 )
		return;

	pev->deadflag = DEAD_DEAD;
	pev->framerate = 0;
	if ( BBoxFlat() )
		UTIL_SetSize( pev, pev->mins, Vector( pev->maxs.x, pev->maxs.y, pev->mins.z + 1 ) );
	else
		UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 1 ) );
	if ( ShouldFadeOnDeath() )
		SUB_StartFadeOut();
	else
		CSoundEnt::InsertSound( bits_SOUND_CARCASS, pev->origin, 384, 30 );
}

Schedule_t* COFBabyVoltigore::GetSchedule()
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( m_MonsterState != MONSTERSTATE_COMBAT || HasConditions( bits_COND_ENEMY_DEAD ) )
		return CBaseMonster::GetSchedule();
	if ( HasConditions( bits_COND_NEW_ENEMY ) )
		return GetScheduleOfType( SCHED_WAKE_ANGRY );
	if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
	{
		AttackSound();
		return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
	}
	if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		return GetScheduleOfType( SCHED_SMALL_FLINCH );
	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
	if ( OccupySlot( bits_SLOT_AGRUNT_CHASE ) )
		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	return GetScheduleOfType( SCHED_STANDOFF );
}

Schedule_t* COFBabyVoltigore::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_CHASE_ENEMY_FAILED:
		return slBabyVoltigoreRangeAttack1;
	case SCHED_VICTORY_DANCE:
		return slBabyVoltigoreVictoryDance;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slBabyVoltigoreTakeCoverFromEnemy;
	case SCHED_RANGE_ATTACK1:
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slBabyVoltigoreRangeAttack1;
		return slBabyVoltigoreCombatFail;
	case SCHED_STANDOFF:
		return slBabyVoltigoreStandoff;
	case SCHED_FAIL:
		if ( m_hEnemy != NULL )
			return slBabyVoltigoreCombatFail;
		return slBabyVoltigoreFail;
	case SCHED_BABYVOLTIGORE_THREAT_DISPLAY:
		return slBabyVoltigoreThreatDisplay;
	default:
		return CSquadMonster::GetScheduleOfType( Type );
	}
}
