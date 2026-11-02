#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadmonster.h"
#include "schedule.h"
#include "effects.h"
#include "weapons.h"
#include "soundent.h"
#include "charge_bolt.h"

enum
{
	SCHED_VOLTIGORE_THREAT_DISPLAY = LAST_COMMON_SCHEDULE + 1
};

enum
{
	TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE = LAST_COMMON_TASK + 1
};

class COFVoltigore : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	void SetObjectCollisionBox() override;
	int Classify() override;
	int ISoundMask() override;
	void SetYawSpeed() override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
		int bitsDamageType ) override;
	int IRelationship( CBaseEntity* pTarget ) override;
	void AlertSound() override;
	void PainSound() override;
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	void GibMonster() override;
	void StartTask( Task_t* pTask ) override;
	void RunTask( Task_t* pTask ) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType( int Type ) override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	virtual void DeathGibThink();
	void EXPORT CallDeathGibThink();
	void StopTalking();
	BOOL ShouldSpeak();
	void ClearBeams();

	static TYPEDESCRIPTION m_SaveData[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];
	CUSTOM_SCHEDULES;

	CBeam*				m_pBeam[8];
	int					m_iBeams;
	float				m_flNextBeamAttackCheck;
	float				m_flNextPainTime;
	float				m_flNextSpeakTime;
	float				m_flNextWordTime;
	int					m_iLastWord;
	COFChargedBolt*		m_pChargedBolt;
	int					m_iVoltigoreGibs;
	BOOL				m_fDeathCharge;
	float				m_flDeathStartTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_alien_voltigore, COFVoltigore );

typedef struct giblimit_s
{
	int nMaxGibs;
	int nCurGibs;
} giblimit_t;

giblimit_t g_giblimitlist[] =
{
	{ 1, 0 }, { 1, 0 }, { 1, 0 }, { 1, 0 }, { 2, 0 },
	{ 1, 0 }, { 2, 0 }, { 1, 0 }, { 2, 0 }
};

int iVoltigoreMuzzleFlash;

const char* COFVoltigore::pAlertSounds[] =
{
	"voltigore/voltigore_alert1.wav",
	"voltigore/voltigore_alert2.wav",
	"voltigore/voltigore_alert3.wav"
};

const char* COFVoltigore::pPainSounds[] =
{
	"voltigore/voltigore_pain1.wav",
	"voltigore/voltigore_pain2.wav",
	"voltigore/voltigore_pain3.wav",
	"voltigore/voltigore_pain4.wav"
};

const char* COFVoltigore::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav", "zombie/claw_strike2.wav", "zombie/claw_strike3.wav"
};

const char* COFVoltigore::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav", "zombie/claw_miss2.wav"
};

TYPEDESCRIPTION COFVoltigore::m_SaveData[] =
{
	DEFINE_FIELD( COFVoltigore, m_flNextBeamAttackCheck, FIELD_TIME ),
	DEFINE_FIELD( COFVoltigore, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( COFVoltigore, m_flNextSpeakTime, FIELD_TIME ),
	DEFINE_FIELD( COFVoltigore, m_flNextWordTime, FIELD_TIME ),
	DEFINE_FIELD( COFVoltigore, m_iLastWord, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( COFVoltigore, CSquadMonster );

Task_t tlVoltigoreFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slVoltigoreFail[] =
{
	{ tlVoltigoreFail, ARRAYSIZE( tlVoltigoreFail ), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1,
		0, "Voltigore Fail" }
};

Task_t tlVoltigoreCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
	{ TASK_WAIT_PVS, 0 }
};

Schedule_t slVoltigoreCombatFail[] =
{
	{ tlVoltigoreCombatFail, ARRAYSIZE( tlVoltigoreCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK1, 0, "Voltigore Combat Fail" }
};

Task_t tlVoltigoreStandoff[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 }
};

Schedule_t slVoltigoreStandoff[] =
{
	{ tlVoltigoreStandoff, ARRAYSIZE( tlVoltigoreStandoff ), bits_COND_SEE_ENEMY | bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "Voltigore Standoff" }
};

Task_t tlVoltigoreRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_RANGE_ATTACK1_NOTURN, 0 },
	{ TASK_WAIT, 0.5f }
};

Schedule_t slVoltigoreRangeAttack1[] =
{
	{ tlVoltigoreRangeAttack1, ARRAYSIZE( tlVoltigoreRangeAttack1 ), bits_COND_ENEMY_OCCLUDED |
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_ENEMY_DEAD, 0, "Voltigore Range Attack1" }
};

Task_t tlVoltigoreTakeCoverFromEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_WAIT, 0.2f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 }
};

Schedule_t slVoltigoreTakeCoverFromEnemy[] =
{
	{ tlVoltigoreTakeCoverFromEnemy, ARRAYSIZE( tlVoltigoreTakeCoverFromEnemy ),
		bits_COND_NEW_ENEMY, 0, "VoltigoreTakeCoverFromEnemy" }
};

Task_t tlVoltigoreVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_VOLTIGORE_THREAT_DISPLAY },
	{ TASK_WAIT, 0.2f },
	{ TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE, 0 },
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
	{ TASK_PLAY_SEQUENCE, ACT_STAND }
};

Schedule_t slVoltigoreVictoryDance[] =
{
	{ tlVoltigoreVictoryDance, ARRAYSIZE( tlVoltigoreVictoryDance ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY, 0, "VoltigoreVictoryDance" }
};

Task_t tlVoltigoreThreatDisplay[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_THREAT_DISPLAY }
};

Schedule_t slVoltigoreThreatDisplay[] =
{
	{ tlVoltigoreThreatDisplay, ARRAYSIZE( tlVoltigoreThreatDisplay ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY,
		bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "VoltigoreThreatDisplay" }
};

DEFINE_CUSTOM_SCHEDULES( COFVoltigore )
{
	slVoltigoreFail,
	slVoltigoreCombatFail,
	slVoltigoreStandoff,
	slVoltigoreRangeAttack1,
	slVoltigoreTakeCoverFromEnemy,
	slVoltigoreVictoryDance,
	slVoltigoreThreatDisplay
};

IMPLEMENT_CUSTOM_SCHEDULES( COFVoltigore, CBaseMonster );

int COFVoltigore::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

int COFVoltigore::Classify()
{
	return CLASS_ALIEN_MILITARY;
}

void COFVoltigore::SetYawSpeed()
{
	pev->yaw_speed = m_Activity == ACT_TURN_LEFT || m_Activity == ACT_TURN_RIGHT ? 80 : 70;
}

BOOL COFVoltigore::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

void COFVoltigore::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/voltigore.mdl" );
	UTIL_SetSize( pev, Vector( -80, -80, 0 ), Vector( 80, 80, 90 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = gSkillData.voltigoreHealth;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.2f;
	m_HackedGunPos = Vector( 24, 64, 48 );
	float flSpeakTime = gpGlobals->time + 10;
	m_flNextWordTime = m_flNextSpeakTime = flSpeakTime + RANDOM_LONG( 0, 10 );
	m_flNextBeamAttackCheck = gpGlobals->time;
	m_fDeathCharge = FALSE;
	m_flDeathStartTime = gpGlobals->time;
	MonsterInit();
}

void COFVoltigore::Precache()
{
	PRECACHE_MODEL( "models/voltigore.mdl" );
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
	UTIL_PrecacheOther( "charged_bolt" );
	m_iVoltigoreGibs = PRECACHE_MODEL( "models/vgibs.mdl" );
}

void COFVoltigore::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -80, -80, 0 );
	pev->absmax = pev->origin + Vector( 80, 80, 90 );
}

void COFVoltigore::StopTalking()
{
	float flSpeakTime = gpGlobals->time + 10;
	m_flNextSpeakTime = m_flNextWordTime = flSpeakTime + RANDOM_LONG( 0, 10 );
}

BOOL COFVoltigore::ShouldSpeak()
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

void COFVoltigore::AlertSound()
{
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pAlertSounds[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM, 0, 100 );
}

void COFVoltigore::PainSound()
{
	if ( m_flNextPainTime > gpGlobals->time )
		return;
	m_flNextPainTime = gpGlobals->time + 0.6;
	StopTalking();
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, 3 )], 1, ATTN_NORM, 0, 100 );
}

BOOL COFVoltigore::CheckMeleeAttack1( float flDot, float flDist )
{
	return HasConditions( bits_COND_SEE_ENEMY ) && flDist <= 128 && flDot >= 0.6f && m_hEnemy != NULL;
}

BOOL COFVoltigore::CheckRangeAttack1( float flDot, float flDist )
{
	if ( IsMoving() && flDist >= 512 )
		return FALSE;
	if ( flDist < 128 || flDist > 1024 || flDot < 0.5f || gpGlobals->time < m_flNextBeamAttackCheck )
		return FALSE;
	UTIL_MakeVectors( pev->angles );
	Vector vecSrc, vecAngles;
	GetAttachment( 4, vecSrc, vecAngles );
	TraceResult tr;
	UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), dont_ignore_monsters, edict(), &tr );
	if ( tr.flFraction == 1 || tr.pHit == m_hEnemy->edict() )
	{
		m_flNextBeamAttackCheck = gpGlobals->time + RANDOM_FLOAT( 20, 30 );
		return TRUE;
	}
	m_flNextBeamAttackCheck = gpGlobals->time + 0.2;
	return FALSE;
}

void COFVoltigore::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	if ( bitsDamageType & DMG_SHOCK )
		return;
	SpawnBlood( ptr->vecEndPos, BloodColor(), flDamage );
	TraceBleed( flDamage, vecDir, ptr, bitsDamageType );
	AddMultiDamage( pevAttacker, this, flDamage, bitsDamageType );
}

int COFVoltigore::IRelationship( CBaseEntity* pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_human_grunt" ) )
		return R_NM;
	return CBaseMonster::IRelationship( pTarget );
}

void COFVoltigore::ClearBeams()
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

void COFVoltigore::Killed( entvars_t* pevAttacker, int iGib )
{
	ClearBeams();
	if ( m_pChargedBolt )
	{
		UTIL_Remove( m_pChargedBolt );
		m_pChargedBolt = NULL;
	}
	CSquadMonster::Killed( pevAttacker, iGib );
}

void COFVoltigore::StartTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_DIE )
	{
		SetThink( &COFVoltigore::CallDeathGibThink );
		CBaseMonster::StartTask( pTask );
		return;
	}
	if ( pTask->iTask == TASK_RANGE_ATTACK1_NOTURN )
	{
		ClearBeams();
		if ( m_pChargedBolt )
		{
			UTIL_Remove( m_pChargedBolt );
			m_pChargedBolt = NULL;
		}
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
		m_pChargedBolt = COFChargedBolt::ChargedBoltCreate();
		UTIL_SetOrigin( m_pChargedBolt->pev, vecConverge );
		UTIL_EmitAmbientSound( edict(), pev->origin, "debris/beamstart2.wav", 0.5f, ATTN_NORM, 0,
			RANDOM_LONG( 140, 160 ) );
		CBaseMonster::StartTask( pTask );
		return;
	}
	if ( m_pChargedBolt )
	{
		ClearBeams();
		UTIL_Remove( m_pChargedBolt );
		m_pChargedBolt = NULL;
	}
	if ( pTask->iTask == TASK_VOLTIGORE_GET_PATH_TO_ENEMY_CORPSE )
	{
		UTIL_MakeVectors( pev->angles );
		Vector vecGoal = m_vecEnemyLKP - gpGlobals->v_forward * 64;
		if ( BuildRoute( vecGoal, bits_MF_TO_LOCATION, NULL ) )
			TaskComplete();
		else
		{
			ALERT( at_aiconsole, "VoltigoreGetPathToEnemyCorpse failed!!\n" );
			TaskFail();
		}
		return;
	}
	CBaseMonster::StartTask( pTask );
}

void COFVoltigore::RunTask( Task_t* pTask )
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

Schedule_t* COFVoltigore::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_CHASE_ENEMY_FAILED:
		return slVoltigoreRangeAttack1;
	case SCHED_VICTORY_DANCE:
		return slVoltigoreVictoryDance;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		return slVoltigoreTakeCoverFromEnemy;
	case SCHED_RANGE_ATTACK1:
		return HasConditions( bits_COND_SEE_ENEMY ) ? slVoltigoreRangeAttack1 : slVoltigoreCombatFail;
	case SCHED_STANDOFF:
		return slVoltigoreStandoff;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slVoltigoreCombatFail : slVoltigoreFail;
	case SCHED_VOLTIGORE_THREAT_DISPLAY:
		return slVoltigoreThreatDisplay;
	default:
		return CSquadMonster::GetScheduleOfType( Type );
	}
}

Schedule_t* COFVoltigore::GetSchedule()
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
		return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
	if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
		return GetScheduleOfType( SCHED_SMALL_FLINCH );
	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
	if ( OccupySlot( bits_SLOT_AGRUNT_CHASE ) )
		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	return GetScheduleOfType( SCHED_STANDOFF );
}

void COFVoltigore::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	if ( pEvent->event == 12 || pEvent->event == 13 )
	{
		CBaseEntity* pHurt = CheckTraceHullAttack( 128, (int)gSkillData.voltigoreDmgPunch, DMG_CLUB );
		if ( !pHurt )
		{
			int pitch = RANDOM_LONG( -5, 5 ) + 100;
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG( 0, 1 )],
				1, ATTN_NORM, 0, pitch );
			return;
		}
		pHurt->pev->punchangle.y = pEvent->event == 12 ? -25.0f : 25.0f;
		pHurt->pev->punchangle.x = 8;
		if ( pHurt->IsPlayer() )
			pHurt->pev->velocity = gpGlobals->v_right * ( pEvent->event == 12 ? 250.0f : -250.0f ) +
				pHurt->pev->velocity;
		int pitch = RANDOM_LONG( -5, 5 ) + 100;
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, 2 )],
			1, ATTN_NORM, 0, pitch );
		Vector vecPos, vecAngles;
		GetAttachment( 0, vecPos, vecAngles );
		SpawnBlood( vecPos, pHurt->BloodColor(), 25 );
		return;
	}
	if ( pEvent->event >= 1 && pEvent->event <= 5 )
	{
		if ( m_pChargedBolt )
		{
			ClearBeams();
			UTIL_MakeVectors( pev->angles );
			Vector vecSrc = gpGlobals->v_up * 32 + gpGlobals->v_forward * 50 + pev->origin;
			Vector vecAim = ShootAtEnemy( vecSrc );
			TraceResult tr;
			UTIL_TraceLine( vecSrc, vecAim * 1024 + vecSrc, dont_ignore_monsters, edict(), &tr );
			m_pChargedBolt->LaunchChargedBolt( vecAim, edict(), 1000, 10 );
			m_pChargedBolt = NULL;
		}
		return;
	}
	CBaseMonster::HandleAnimEvent( pEvent );
}

void COFVoltigore::CallDeathGibThink()
{
	DeathGibThink();
}

void COFVoltigore::GibMonster()
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM, 0, 100 );
	pev->renderfx = kRenderFxExplode;
	pev->rendercolor = Vector( 255, 0, 0 );
	pev->framerate = 0;
	pev->nextthink = gpGlobals->time + 0.15;
	SetThink( &CBaseEntity::SUB_Remove );
	for ( int i = 0; i < ARRAYSIZE( g_giblimitlist ); i++ )
		g_giblimitlist[i].nCurGibs = 0;
	int iBody = 0;
	for ( int i = 0; i < 12; i++ )
	{
		CGib* pGib = GetClassPtr( (CGib*)NULL );
		if ( g_Language == LANGUAGE_GERMAN )
		{
			pGib->Spawn( "models/germangibs.mdl" );
			pGib->pev->body = RANDOM_LONG( 0, 3 );
		}
		else
		{
			pGib->Spawn( "models/vgibs.mdl" );
			if ( g_giblimitlist[iBody].nCurGibs >= g_giblimitlist[iBody].nMaxGibs )
				iBody++;
			g_giblimitlist[iBody].nCurGibs++;
			pGib->pev->body = iBody;
		}
		if ( pev )
		{
			pGib->pev->origin.x = RANDOM_FLOAT( 0, 1 ) * pev->size.x + pev->absmin.x;
			pGib->pev->origin.y = RANDOM_FLOAT( 0, 1 ) * pev->size.y + pev->absmin.y;
			pGib->pev->origin.z = RANDOM_FLOAT( 0, 1 ) * pev->size.z + pev->absmin.z + 1;
			pGib->pev->velocity = -g_vecAttackDir;
			pGib->pev->velocity.x += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.y += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity.z += RANDOM_FLOAT( -0.25f, 0.25f );
			pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300, 400 );
			pGib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
			pGib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );
			pGib->pev->avelocity.z = RANDOM_FLOAT( 100, 200 );
			pGib->m_bloodColor = BloodColor();
			pGib->pev->velocity = pGib->pev->velocity * 2;
			pGib->pev->solid = SOLID_BBOX;
			UTIL_SetSize( pGib->pev, g_vecZero, g_vecZero );
		}
		pGib->LimitVelocity();
	}
	for ( int i = 0; i < 8; i++ )
	{
		int side = i & 1 ? 1 : -1;
		UTIL_MakeAimVectors( pev->angles );
		Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
		TraceResult tr, trClosest;
		float flClosest = 1;
		for ( int j = 0; j < 3; j++ )
		{
			Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
			vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + vecAim;
			UTIL_TraceLine( vecSrc, vecAim * 512 + vecSrc, dont_ignore_monsters, edict(), &tr );
			if ( tr.flFraction < flClosest )
			{
				trClosest = tr;
				flClosest = tr.flFraction;
			}
		}
		if ( flClosest == 1 )
			return;
		CBeam* pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
		if ( !pBeam )
			return;
		CBaseEntity* pEntity = CBaseEntity::Instance( trClosest.pHit );
		Vector vecCenter = pev->origin + Vector( 0, 0, 32 );
		if ( pEntity && pEntity->pev->takedamage != DAMAGE_NO )
		{
			pBeam->PointEntInit( vecCenter, pEntity->entindex() );
			pBeam->SetColor( 180, 16, 255 );
			pBeam->SetBrightness( 255 );
			pBeam->SetNoise( 128 );
		}
		else
		{
			pBeam->PointsInit( trClosest.vecEndPos, vecCenter );
			pBeam->SetColor( 180, 16, 255 );
			pBeam->SetBrightness( 255 );
			pBeam->SetNoise( 192 );
		}
		pBeam->LiveForTime( 0.6f );
	}
	ClearMultiDamage();
	::RadiusDamage( pev->origin, pev, pev, gSkillData.voltigoreDmgBeam, 160, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
}

void COFVoltigore::DeathGibThink()
{
	pev->nextthink = gpGlobals->time + 0.1;
	DispatchAnimEvents( 0.1f );
	StudioFrameAdvance();
	if ( m_fSequenceFinished )
	{
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM, 0, 100 );
		pev->renderfx = kRenderFxExplode;
		pev->rendercolor = Vector( 255, 0, 0 );
		pev->framerate = 0;
		pev->nextthink = gpGlobals->time + 0.15;
		SetThink( &CBaseEntity::SUB_Remove );
		for ( int i = 0; i < ARRAYSIZE( g_giblimitlist ); i++ )
			g_giblimitlist[i].nCurGibs = 0;
		int iBody = 0;
		for ( int i = 0; i < 12; i++ )
		{
			CGib* pGib = GetClassPtr( (CGib*)NULL );
			if ( g_Language == LANGUAGE_GERMAN )
			{
				pGib->Spawn( "models/germangibs.mdl" );
				pGib->pev->body = RANDOM_LONG( 0, 3 );
			}
			else
			{
				pGib->Spawn( "models/vgibs.mdl" );
				if ( g_giblimitlist[iBody].nCurGibs >= g_giblimitlist[iBody].nMaxGibs )
					iBody++;
				g_giblimitlist[iBody].nCurGibs++;
				pGib->pev->body = iBody;
			}
			if ( pev )
			{
				pGib->pev->origin.x = RANDOM_FLOAT( 0, 1 ) * pev->size.x + pev->absmin.x;
				pGib->pev->origin.y = RANDOM_FLOAT( 0, 1 ) * pev->size.y + pev->absmin.y;
				pGib->pev->origin.z = RANDOM_FLOAT( 0, 1 ) * pev->size.z + pev->absmin.z + 1;
				pGib->pev->velocity = -g_vecAttackDir;
				pGib->pev->velocity.x += RANDOM_FLOAT( -0.25f, 0.25f );
				pGib->pev->velocity.y += RANDOM_FLOAT( -0.25f, 0.25f );
				pGib->pev->velocity.z += RANDOM_FLOAT( -0.25f, 0.25f );
				pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300, 400 );
				pGib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
				pGib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );
				pGib->pev->avelocity.z = RANDOM_FLOAT( 100, 200 );
				pGib->m_bloodColor = BloodColor();
				pGib->pev->velocity = pGib->pev->velocity * 2;
				pGib->pev->solid = SOLID_BBOX;
				UTIL_SetSize( pGib->pev, g_vecZero, g_vecZero );
			}
			pGib->LimitVelocity();
		}
		return;
	}
	for ( int i = 0; i < 2; i++ )
	{
		int side = i ? 1 : -1;
		UTIL_MakeAimVectors( pev->angles );
		Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
		TraceResult tr, trClosest;
		float flClosest = 1;
		for ( int j = 0; j < 3; j++ )
		{
			Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
			vecAim = gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 ) + vecAim;
			UTIL_TraceLine( vecSrc, vecAim * 512 + vecSrc, dont_ignore_monsters, edict(), &tr );
			if ( tr.flFraction < flClosest )
			{
				trClosest = tr;
				flClosest = tr.flFraction;
			}
		}
		if ( flClosest == 1 )
			return;
		CBeam* pBeam = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
		if ( !pBeam )
			return;
		CBaseEntity* pEntity = CBaseEntity::Instance( trClosest.pHit );
		Vector vecCenter = pev->origin + Vector( 0, 0, 32 );
		if ( pEntity && pEntity->pev->takedamage != DAMAGE_NO )
		{
			pBeam->PointEntInit( vecCenter, pEntity->entindex() );
			pBeam->SetColor( 180, 16, 255 );
			pBeam->SetBrightness( 255 );
			pBeam->SetNoise( 128 );
		}
		else
		{
			pBeam->PointsInit( trClosest.vecEndPos, vecCenter );
			pBeam->SetColor( 180, 16, 255 );
			pBeam->SetBrightness( 255 );
			pBeam->SetNoise( 192 );
		}
		pBeam->LiveForTime( 0.6f );
	}
	ClearMultiDamage();
	::RadiusDamage( pev->origin, pev, pev, gSkillData.voltigoreDmgBeam, 160, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
}
