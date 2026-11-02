#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "soundent.h"
#include "nodes.h"

int iSpikeTrail;
int iPitdroneSpitSprite;

enum
{
	SCHED_PITDRONE_HURTHOP = LAST_COMMON_SCHEDULE + 1,
	SCHED_PITDRONE_SMELLFOOD,
	SCHED_PITDRONE_EAT,
	SCHED_PITDRONE_SNIFF_AND_EAT,
	SCHED_PITDRONE_WALLOW,
	SCHED_PITDRONE_HIDE_RELOAD,
	SCHED_PITDRONE_WAIT_IN_COVER,
	SCHED_PITDRONE_FAIL,
};

enum
{
	TASK_PITDRONE_HOPTURN = LAST_COMMON_TASK + 1,
};

#define PITDRONE_AE_SPIT		1
#define PITDRONE_AE_BITE		2
#define PITDRONE_AE_WHIP		4
#define PITDRONE_AE_HOP		5
#define PITDRONE_AE_THROW		6
#define PITDRONE_AE_RELOAD	7

class CPitdroneSpike : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	int Classify();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	void EXPORT StartTrail();
	void EXPORT SpikeTouch( CBaseEntity* pOther );
	static void Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles );
	static TYPEDESCRIPTION m_SaveData[];

	int m_maxFrame;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( pitdronespike, CPitdroneSpike );

TYPEDESCRIPTION CPitdroneSpike::m_SaveData[] =
{
	DEFINE_FIELD( CPitdroneSpike, m_maxFrame, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CPitdroneSpike, CBaseEntity );

class CPitdrone : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	int Classify();
	int ISoundMask();
	void IdleSound();
	void AlertSound();
	void PainSound();
	void GibMonster();
	void SetYawSpeed();
	void CheckAmmo();
	void RunAI();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckMeleeAttack2( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL FValidateHintType( short sHint );
	int IRelationship( CBaseEntity* pTarget );
	int IgnoreConditions();
	void StartTask( Task_t* pTask );
	void RunTask( Task_t* pTask );
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType( int Type );

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flLastHurtTime;
	float	m_flNextSpikeTime;
	int		m_iInitialAmmo;
	float	m_flNextEatTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_pitdrone, CPitdrone );

TYPEDESCRIPTION CPitdrone::m_SaveData[] =
{
	DEFINE_FIELD( CPitdrone, m_flLastHurtTime, FIELD_TIME ),
	DEFINE_FIELD( CPitdrone, m_flNextSpikeTime, FIELD_TIME ),
	DEFINE_FIELD( CPitdrone, m_flNextEatTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CPitdrone, CBaseMonster );

Task_t tlPitdroneTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PITDRONE_FAIL },
	{ TASK_WAIT, 0.2 },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, (float)SCHED_PITDRONE_WAIT_IN_COVER },
};

Schedule_t slPitdroneTakeCover[] =
{
	{ tlPitdroneTakeCover1, ARRAYSIZE( tlPitdroneTakeCover1 ), 0, 0, "PitdroneTakeCover" },
};

Task_t tlPitdroneWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 1 },
};

Schedule_t slPitdroneWaitInCover[] =
{
	{
		tlPitdroneWaitInCover, ARRAYSIZE( tlPitdroneWaitInCover ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK2 | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "PitdroneWaitInCover"
	},
};

Task_t tlPitdroneHideReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_RELOAD, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};

Schedule_t slPitdroneHideReload[] =
{
	{
		tlPitdroneHideReload, ARRAYSIZE( tlPitdroneHideReload ),
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "PitdroneHideReload"
	},
};

Task_t tlPitdroneWallow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FAIL },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_EAT, 10 },
	{ TASK_GET_PATH_TO_BESTSCENT, 0 },
	{ TASK_STORE_LASTPOSITION, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_INSPECT_FLOOR },
	{ TASK_EAT, 50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 },
};

Schedule_t slPitdroneWallow[] =
{
	{
		tlPitdroneWallow, ARRAYSIZE( tlPitdroneWallow ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_GARBAGE, "SquidWallow"
	},
};

Task_t tlPitdroneSniffAndEat[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FAIL },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_EAT, 10 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_DETECT_SCENT },
	{ TASK_GET_PATH_TO_BESTSCENT, 0 },
	{ TASK_STORE_LASTPOSITION, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, 50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 },
};

Schedule_t slPitdroneSniffAndEat[] =
{
	{
		tlPitdroneSniffAndEat, ARRAYSIZE( tlPitdroneSniffAndEat ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_MEAT | bits_SOUND_CARCASS, "SquidSniffAndEat"
	},
};

Task_t tlPitdroneEat[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_FAIL },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_EAT, 10 },
	{ TASK_GET_PATH_TO_BESTSCENT, 0 },
	{ TASK_STORE_LASTPOSITION, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_PLAY_SEQUENCE, (float)ACT_EAT },
	{ TASK_EAT, 50 },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 },
};

Schedule_t slPitdroneEat[] =
{
	{
		tlPitdroneEat, ARRAYSIZE( tlPitdroneEat ),
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_MEAT | bits_SOUND_CARCASS, "SquidEat"
	},
};

Task_t tlPitdroneHurtHop[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SOUND_WAKE, 0 },
	{ TASK_PITDRONE_HOPTURN, 0 },
	{ TASK_FACE_ENEMY, 0 },
};

Schedule_t slPitdroneHurtHop[] =
{
	{ tlPitdroneHurtHop, ARRAYSIZE( tlPitdroneHurtHop ), 0, 0, "SquidHurtHop" },
};

Task_t tlPitdroneChaseEnemy1[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RANGE_ATTACK1 },
	{ TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
};

Schedule_t slPitdroneChaseEnemy[] =
{
	{
		tlPitdroneChaseEnemy1, ARRAYSIZE( tlPitdroneChaseEnemy1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2 | bits_COND_TASK_FAILED | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_MEAT, "Squid Chase Enemy"
	},
};

Task_t tlPitdroneRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slPitdroneRangeAttack1[] =
{
	{
		tlPitdroneRangeAttack1, ARRAYSIZE( tlPitdroneRangeAttack1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE |
		bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		0, "Squid Range Attack1"
	},
};

DEFINE_CUSTOM_SCHEDULES( CPitdrone )
{
	slPitdroneRangeAttack1,
	slPitdroneChaseEnemy,
	slPitdroneHurtHop,
	slPitdroneEat,
	slPitdroneSniffAndEat,
	slPitdroneWallow,
	slPitdroneHideReload,
	slPitdroneWaitInCover,
	slPitdroneTakeCover,
};

IMPLEMENT_CUSTOM_SCHEDULES( CPitdrone, CBaseMonster );

int CPitdroneSpike::Classify()
{
	return CLASS_NONE;
}

void CPitdroneSpike::Precache()
{
	PRECACHE_MODEL( "models/pit_drone_spike.mdl" );
	PRECACHE_SOUND( "weapons/xbow_hitbod1.wav" );
	PRECACHE_SOUND( "weapons/xbow_hit1.wav" );
	iSpikeTrail = PRECACHE_MODEL( "sprites/spike_trail.spr" );
}

void CPitdroneSpike::Spawn()
{
	pev->movetype = MOVETYPE_FLY;
	pev->classname = MAKE_STRING( "pitdronespike" );
	pev->solid = SOLID_BBOX;
	pev->takedamage = DAMAGE_YES;
	pev->flags |= FL_MONSTER;
	pev->health = 1;
	SET_MODEL( ENT( pev ), "models/pit_drone_spike.mdl" );
	pev->frame = 0;
	pev->scale = 0.5;
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	m_maxFrame = (int)( (float)MODEL_FRAMES( pev->modelindex ) - 1.0f );
}

void CPitdroneSpike::StartTrail()
{
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
		WRITE_BYTE( TE_BEAMFOLLOW );
		WRITE_SHORT( entindex() );
		WRITE_SHORT( iSpikeTrail );
		WRITE_BYTE( 2 );
		WRITE_BYTE( 1 );
		WRITE_BYTE( 197 );
		WRITE_BYTE( 194 );
		WRITE_BYTE( 11 );
		WRITE_BYTE( 192 );
	MESSAGE_END();
	SetTouch( &CPitdroneSpike::SpikeTouch );
	SetThink( NULL );
}

void CPitdroneSpike::Shoot( entvars_t* pevOwner, Vector vecStart, Vector vecVelocity, Vector vecAngles )
{
	CPitdroneSpike* pSpike = GetClassPtr( (CPitdroneSpike*)NULL );
	pSpike->pev->angles = vecAngles;
	UTIL_SetOrigin( pSpike->pev, vecStart );
	pSpike->Spawn();
	pSpike->pev->velocity = vecVelocity;
	pSpike->pev->owner = ENT( pevOwner );
	pSpike->SetThink( &CPitdroneSpike::StartTrail );
	pSpike->pev->nextthink = gpGlobals->time + 0.1;
}

void CPitdroneSpike::SpikeTouch( CBaseEntity* pOther )
{
	float pitch = RANDOM_FLOAT( 120, 140 );
	const char* sample;
	if ( pOther->pev->takedamage )
	{
		pOther->TakeDamage( pev, pev, gSkillData.pitdroneDmgSpit, DMG_GENERIC );
		sample = "weapons/xbow_hitbod1.wav";
	}
	else
		sample = "weapons/xbow_hit1.wav";

	EMIT_SOUND_DYN( edict(), CHAN_VOICE, sample, 1, ATTN_NORM, 0, (int)pitch );
	SetTouch( NULL );
	if ( !FClassnameIs( pOther->pev, "worldspawn" ) )
	{
		SetThink( &CPitdroneSpike::SUB_Remove );
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	Vector vecDir = pev->velocity.Normalize();
	UTIL_SetOrigin( pev, pev->origin - vecDir * 6 );
	pev->angles = UTIL_VecToAngles( vecDir );
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_FLY;
	pev->angles.z = (float)RANDOM_LONG( 0, 360 );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	SetThink( &CPitdroneSpike::SUB_FadeOut );
	pev->nextthink = gpGlobals->time + 90;
}

int CPitdrone::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

int CPitdrone::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_CARCASS | bits_SOUND_MEAT |
		bits_SOUND_GARBAGE | bits_SOUND_PLAYER;
}

BOOL CPitdrone::FValidateHintType( short sHint )
{
	if ( sHint == HINT_WORLD_HUMAN_BLOOD )
		return TRUE;

	ALERT( at_aiconsole, "Couldn't validate hint type" );
	return FALSE;
}

void CPitdrone::SetYawSpeed()
{
	pev->yaw_speed = 90;
}

void CPitdrone::IdleSound()
{
}

void CPitdrone::AlertSound()
{
	int pitch = RANDOM_LONG( 140, 160 );
	switch ( RANDOM_LONG( 0, 1 ) )
	{
	case 0:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_alert1.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 1:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_alert2.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 2:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_alert3.wav", 1, ATTN_NORM, 0, pitch );
		break;
	}
}

void CPitdrone::PainSound()
{
	int pitch = RANDOM_LONG( 85, 120 );
	switch ( RANDOM_LONG( 0, 3 ) )
	{
	case 0:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_pain1.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 1:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_pain2.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 2:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_pain3.wav", 1, ATTN_NORM, 0, pitch );
		break;
	case 3:
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "pitdrone/pit_drone_pain4.wav", 1, ATTN_NORM, 0, pitch );
		break;
	}
}

void CPitdrone::CheckAmmo()
{
	if ( m_iInitialAmmo != -1 && m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

void CPitdrone::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/pit_drone.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.pitdroneHealth;
	m_flFieldOfView = -0.7;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextSpikeTime = gpGlobals->time;
	if ( m_iInitialAmmo == -1 )
		pev->body = 1;
	else if ( m_iInitialAmmo <= 0 )
	{
		m_iInitialAmmo = -1;
		pev->body = 0;
	}
	else
		pev->body = 7 - m_iInitialAmmo;

	m_cAmmoLoaded = m_iInitialAmmo == -1 ? 0 : m_iInitialAmmo;
	m_flNextEatTime = gpGlobals->time;
	MonsterInit();
}

void CPitdrone::Precache()
{
	PRECACHE_MODEL( "models/pit_drone.mdl" );
	PRECACHE_MODEL( "models/pit_drone_gibs.mdl" );
	UTIL_PrecacheOther( "pitdronespike" );
	iPitdroneSpitSprite = PRECACHE_MODEL( "sprites/tinyspit.spr" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_alert1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_alert2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_alert3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_attack_spike1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_attack_spike2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_communicate1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_communicate2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_communicate3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_communicate4.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_die1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_die2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_die3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_hunt1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_hunt2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_hunt3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_idle1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_idle2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_idle3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_melee_attack1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_melee_attack2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_pain1.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_pain2.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_pain3.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_pain4.wav" );
	PRECACHE_SOUND( "pitdrone/pit_drone_run_on_grate.wav" );
	PRECACHE_SOUND( "bullchicken/bc_bite2.wav" );
	PRECACHE_SOUND( "bullchicken/bc_bite3.wav" );
}

void CPitdrone::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "initammo" ) )
	{
		m_iInitialAmmo = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

BOOL CPitdrone::CheckMeleeAttack1( float flDot, float flDist )
{
	return flDist <= 64 && flDot >= 0.7 && RANDOM_LONG( 0, 3 ) == 0;
}

BOOL CPitdrone::CheckMeleeAttack2( float flDot, float flDist )
{
	if ( flDist > 64 || flDot < 0.7 )
		return FALSE;

	return !HasConditions( bits_COND_CAN_MELEE_ATTACK1 );
}

BOOL CPitdrone::CheckRangeAttack1( float flDot, float flDist )
{
	if ( m_iInitialAmmo == -1 || pev->body == 0 || ( IsMoving() && flDist >= 512 ) )
		return FALSE;

	if ( flDist > 128 && flDist <= 784 && flDot >= 0.5 && gpGlobals->time >= m_flNextSpikeTime )
	{
		if ( m_hEnemy != NULL && !( fabs( pev->origin.z - m_hEnemy->pev->origin.z ) <= 256 ) )
			return FALSE;

		m_flNextSpikeTime = gpGlobals->time + ( IsMoving() ? 5 : 0.5 );
		return TRUE;
	}

	return FALSE;
}

int CPitdrone::IRelationship( CBaseEntity* pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_pitdrone" ) )
		return R_AL;

	return CBaseMonster::IRelationship( pTarget );
}

int CPitdrone::IgnoreConditions()
{
	int conditions = CBaseMonster::IgnoreConditions();
	if ( gpGlobals->time - m_flLastHurtTime <= 20 )
		return bits_COND_SMELL | bits_COND_SMELL_FOOD;

	return conditions;
}

void CPitdrone::StartTask( Task_t* pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( pTask->iTask )
	{
	case TASK_PITDRONE_HOPTURN:
		SetActivity( ACT_HOP );
		MakeIdealYaw( m_vecEnemyLKP );
		break;
	case TASK_GET_PATH_TO_ENEMY:
		if ( BuildRoute( m_hEnemy->pev->origin, bits_MF_TO_ENEMY, m_hEnemy ) )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
		else
		{
			ALERT( at_aiconsole, "GetPathToEnemy failed!!\n" );
			TaskFail();
		}
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CPitdrone::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_PITDRONE_HOPTURN )
	{
		MakeIdealYaw( m_vecEnemyLKP );
		ChangeYaw( (int)pev->yaw_speed );
		if ( m_fSequenceFinished )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	else
		CBaseMonster::RunTask( pTask );
}

void CPitdrone::RunAI()
{
	CBaseMonster::RunAI();
	if ( m_hEnemy != NULL && m_Activity == ACT_RUN &&
		( pev->origin - m_hEnemy->pev->origin ).Length2D() < 256 )
		pev->framerate = 1.25;
}

Schedule_t* CPitdrone::GetSchedule()
{
	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			return CBaseMonster::GetSchedule();
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return GetScheduleOfType( SCHED_WAKE_ANGRY );
		if ( HasConditions( bits_COND_NO_AMMO_LOADED ) && m_iInitialAmmo != -1 )
			return GetScheduleOfType( SCHED_PITDRONE_HIDE_RELOAD );

		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if ( m_iInitialAmmo != -1 && pev->body != 0 )
				return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		}
		else
		{
			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
			if ( HasConditions( bits_COND_CAN_MELEE_ATTACK2 ) )
				return GetScheduleOfType( SCHED_MELEE_ATTACK2 );
		}

		return GetScheduleOfType( SCHED_CHASE_ENEMY );
	}

	if ( m_MonsterState == MONSTERSTATE_ALERT )
	{
		if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_PITDRONE_HURTHOP );
		if ( gpGlobals->time < m_flNextEatTime )
			return CBaseMonster::GetSchedule();

		if ( HasConditions( bits_COND_SMELL_FOOD ) )
		{
			CSound* pScent = PBestScent();
			if ( !pScent || ( FInViewCone( &pScent->m_vecOrigin ) && FVisible( pScent->m_vecOrigin ) ) )
			{
				m_flNextEatTime = gpGlobals->time + 90;
				return GetScheduleOfType( SCHED_PITDRONE_EAT );
			}

			m_flNextEatTime = gpGlobals->time + 90;
			return GetScheduleOfType( SCHED_PITDRONE_SNIFF_AND_EAT );
		}

		if ( HasConditions( bits_COND_SMELL ) && PBestScent() )
		{
			m_flNextEatTime = gpGlobals->time + 30;
			return GetScheduleOfType( SCHED_PITDRONE_WALLOW );
		}
	}

	return CBaseMonster::GetSchedule();
}

Schedule_t* CPitdrone::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_CHASE_ENEMY: return slPitdroneChaseEnemy;
	case SCHED_TAKE_COVER_FROM_ENEMY: return slPitdroneTakeCover;
	case SCHED_RANGE_ATTACK1: return slPitdroneRangeAttack1;
	case SCHED_PITDRONE_HURTHOP: return slPitdroneHurtHop;
	case SCHED_PITDRONE_EAT: return slPitdroneEat;
	case SCHED_PITDRONE_SNIFF_AND_EAT: return slPitdroneSniffAndEat;
	case SCHED_PITDRONE_WALLOW: return slPitdroneWallow;
	case SCHED_PITDRONE_HIDE_RELOAD: return slPitdroneHideReload;
	case SCHED_PITDRONE_WAIT_IN_COVER: return slPitdroneWaitInCover;
	case SCHED_PITDRONE_FAIL:
		return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ? SCHED_RANGE_ATTACK1 : SCHED_FAIL );
	default:
		return CBaseMonster::GetScheduleOfType( Type );
	}
}

void CPitdrone::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case PITDRONE_AE_SPIT:
		if ( m_iInitialAmmo != -1 && pev->body != 0 )
		{
			UTIL_MakeVectors( pev->angles );
			Vector vecSpitOffset = gpGlobals->v_right * 0 + gpGlobals->v_forward * 15 +
				gpGlobals->v_up * 36 + pev->origin;
			Vector vecSpitDir = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - vecSpitOffset ).Normalize();
			vecSpitDir.x += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.y += RANDOM_FLOAT( -0.05, 0.05 );
			vecSpitDir.z += RANDOM_FLOAT( -0.05, 0 );
			MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, vecSpitOffset );
				WRITE_BYTE( TE_SPRITE_SPRAY );
				WRITE_COORD( vecSpitOffset.x );
				WRITE_COORD( vecSpitOffset.y );
				WRITE_COORD( vecSpitOffset.z );
				WRITE_COORD( vecSpitDir.x );
				WRITE_COORD( vecSpitDir.y );
				WRITE_COORD( vecSpitDir.z );
				WRITE_SHORT( iPitdroneSpitSprite );
				WRITE_BYTE( 10 );
				WRITE_BYTE( 110 );
				WRITE_BYTE( 25 );
			MESSAGE_END();
			CPitdroneSpike::Shoot( pev, vecSpitOffset, vecSpitDir * 900, UTIL_VecToAngles( vecSpitDir ) );
			pev->body = pev->body == 6 ? 0 : pev->body + 1;
			m_cAmmoLoaded--;
		}
		break;
	case PITDRONE_AE_BITE:
		{
			CBaseEntity* pHurt = CheckTraceHullAttack( 70, (int)gSkillData.pitdroneDmgBite, DMG_SLASH );
			if ( pHurt )
			{
				pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_forward * 100;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;
	case PITDRONE_AE_WHIP:
		{
			CBaseEntity* pHurt = CheckTraceHullAttack( 70, (int)gSkillData.pitdroneDmgWhip, DMG_CLUB | DMG_ALWAYSGIB );
			if ( pHurt )
			{
				pHurt->pev->punchangle.z = -20;
				pHurt->pev->punchangle.x = 20;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 200;
				pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_up * 100;
			}
		}
		break;
	case PITDRONE_AE_HOP:
		{
			float gravity = CVAR_GET_FLOAT( "sv_gravity" );
			ClearBits( pev->flags, FL_ONGROUND );
			pev->velocity.z += gravity * 0.625 * 0.5;
		}
		break;
	case PITDRONE_AE_THROW:
		{
			CBaseEntity* pHurt = CheckTraceHullAttack( 70, 0, 0 );
			if ( pHurt )
			{
				int pitch = (int)RANDOM_FLOAT( 90, 110 );
				switch ( RANDOM_LONG( 0, 1 ) )
				{
				case 0:
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "bullchicken/bc_bite2.wav", 1, ATTN_NORM, 0, pitch );
					break;
				case 1:
					EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "bullchicken/bc_bite3.wav", 1, ATTN_NORM, 0, pitch );
					break;
				}

				UTIL_ScreenShake( pHurt->pev->origin, 25, 1.5, 0.7, 2 );
				if ( pHurt->IsPlayer() )
				{
					UTIL_MakeVectors( pev->angles );
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 200 + gpGlobals->v_up * 200;
				}
			}
		}
		break;
	case PITDRONE_AE_RELOAD:
		if ( m_iInitialAmmo == -1 )
			m_cAmmoLoaded = 0;
		else
		{
			pev->body = 1;
			m_cAmmoLoaded = 6;
		}
		ClearConditions( bits_COND_NO_AMMO_LOADED );
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CPitdrone::GibMonster()
{
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	float violence = CVAR_GET_FLOAT( "violence_agibs" );
	if ( violence != violence )
	{
		for ( int i = 0; i < 6; i++ )
		{
			CGib* pGib = GetClassPtr( (CGib*)NULL );
			pGib->Spawn( "models/pit_drone_gibs.mdl" );
			pGib->pev->body = RANDOM_LONG( 0, 6 );
			if ( pev )
			{
				pGib->pev->origin.x = pev->absmin.x + pev->size.x * RANDOM_FLOAT( 0, 1 );
				pGib->pev->origin.y = pev->absmin.y + pev->size.y * RANDOM_FLOAT( 0, 1 );
				pGib->pev->origin.z = pev->absmin.z + pev->size.z * RANDOM_FLOAT( 0, 1 ) + 1;
				pGib->pev->velocity = g_vecAttackDir * -1;
				pGib->pev->velocity.x += RANDOM_FLOAT( -0.25, 0.25 );
				pGib->pev->velocity.y += RANDOM_FLOAT( -0.25, 0.25 );
				pGib->pev->velocity.z += RANDOM_FLOAT( -0.25, 0.25 );
				pGib->pev->velocity = pGib->pev->velocity * RANDOM_FLOAT( 300, 400 );
				pGib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
				pGib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );
				pGib->m_bloodColor = CBaseEntity::Instance( pev )->BloodColor();
				if ( pev->health > -50 )
					pGib->pev->velocity = pGib->pev->velocity * 0.7;
				else if ( pev->health > -200 )
					pGib->pev->velocity = pGib->pev->velocity * 2;
				else
					pGib->pev->velocity = pGib->pev->velocity * 4;

				pGib->pev->solid = SOLID_BBOX;
				UTIL_SetSize( pGib->pev, g_vecZero, g_vecZero );
			}

			pGib->LimitVelocity();
		}
	}

	SetThink( &CPitdrone::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}
