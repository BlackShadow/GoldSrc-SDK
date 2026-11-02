#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "game.h"
#include "player.h"

#define SR_AE_JUMPATTACK	2

Task_t tlSRRangeAttack1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_WAIT_RANDOM, 0.5 },
};

Schedule_t slSRRangeAttack1[] =
{
	{
		tlSRRangeAttack1,
		ARRAYSIZE( tlSRRangeAttack1 ),
		bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		0,
		"HCRangeAttack1"
	},
};

Task_t tlSRRangeAttack1Fast[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_RANGE_ATTACK1, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
};

Schedule_t slSRRangeAttack1Fast[] =
{
	{
		tlSRRangeAttack1Fast,
		ARRAYSIZE( tlSRRangeAttack1Fast ),
		bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED,
		0,
		"HCRAFast"
	},
};

class COFShockRoach : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	int Classify();
	Vector Center();
	Vector BodyTarget( const Vector& posSrc );
	void SetYawSpeed();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void MonsterThink();
	void PrescheduleThink();
	void StartTask( Task_t* pTask );
	void RunTask( Task_t* pTask );
	void IdleSound();
	void AlertSound();
	void PainSound();
	void DeathSound();
	void EXPORT LeapTouch( CBaseEntity* pOther );
	Schedule_t* GetScheduleOfType( int Type );
	virtual float GetDamageAmount();
	virtual int GetVoicePitch();
	virtual float GetSoundVolue();

	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackSounds[];
	static const char* pDeathSounds[];
	static const char* pBiteSounds[];

	float	m_flBirthTime;
	BOOL	m_fRoachSolid;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_shockroach, COFShockRoach );

TYPEDESCRIPTION COFShockRoach::m_SaveData[] =
{
	DEFINE_FIELD( COFShockRoach, m_flBirthTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( COFShockRoach, CBaseMonster );

DEFINE_CUSTOM_SCHEDULES( COFShockRoach )
{
	slSRRangeAttack1,
	slSRRangeAttack1Fast,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFShockRoach, CBaseMonster );

const char* COFShockRoach::pIdleSounds[] =
{
	"shockroach/shock_idle1.wav",
	"shockroach/shock_idle2.wav",
	"shockroach/shock_idle3.wav",
};

const char* COFShockRoach::pAlertSounds[] = { "shockroach/shock_angry.wav" };
const char* COFShockRoach::pPainSounds[] = { "shockroach/shock_flinch.wav" };
const char* COFShockRoach::pAttackSounds[] =
{
	"shockroach/shock_jump1.wav",
	"shockroach/shock_jump2.wav",
};
const char* COFShockRoach::pDeathSounds[] = { "shockroach/shock_die.wav" };
const char* COFShockRoach::pBiteSounds[] = { "shockroach/shock_bite.wav" };

int COFShockRoach::Classify()
{
	return CLASS_ALIEN_PREY;
}

Vector COFShockRoach::Center()
{
	return Vector( pev->origin.x, pev->origin.y, pev->origin.z + 6 );
}

Vector COFShockRoach::BodyTarget( const Vector& posSrc )
{
	return Center();
}

void COFShockRoach::SetYawSpeed()
{
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_WALK:
	case ACT_RUN:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		pev->yaw_speed = 90;
		break;
	default:
		pev->yaw_speed = 30;
		break;
	}
}

void COFShockRoach::Precache()
{
	PRECACHE_SOUND_ARRAY( pIdleSounds );
	PRECACHE_SOUND_ARRAY( pAlertSounds );
	PRECACHE_SOUND_ARRAY( pPainSounds );
	PRECACHE_SOUND_ARRAY( pAttackSounds );
	PRECACHE_SOUND_ARRAY( pDeathSounds );
	PRECACHE_SOUND_ARRAY( pBiteSounds );
	PRECACHE_SOUND( "shockroach/shock_walk.wav" );
	PRECACHE_MODEL( "models/w_shock_rifle.mdl" );
}

BOOL COFShockRoach::CheckRangeAttack1( float flDot, float flDist )
{
	return ( pev->flags & FL_ONGROUND ) && flDist <= 256 && flDot >= 0.65;
}

BOOL COFShockRoach::CheckRangeAttack2( float flDot, float flDist )
{
	return FALSE;
}

void COFShockRoach::IdleSound()
{
}

void COFShockRoach::AlertSound()
{
}

void COFShockRoach::PainSound()
{
}

void COFShockRoach::DeathSound()
{
}

int COFShockRoach::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_ACID )
		flDamage = 0;
	if ( gpGlobals->time - m_flBirthTime < 2 )
		flDamage = 0;

	bitsDamageType = ( bitsDamageType & ~( DMG_NEVERGIB | DMG_ALWAYSGIB ) ) | DMG_NEVERGIB;
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void COFShockRoach::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/w_shock_rifle.mdl" );
	UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 4 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->effects = 0;
	pev->health = gSkillData.shockroachHealth;
	pev->view_ofs = Vector( 0, 0, 20 );
	pev->yaw_speed = 5;
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;
	m_fRoachSolid = FALSE;
	m_flBirthTime = gpGlobals->time;
	MonsterInit();
}

void COFShockRoach::MonsterThink()
{
	if ( gpGlobals->time - m_flBirthTime >= 0.2 )
		pev->movetype = MOVETYPE_STEP;

	if ( !m_fRoachSolid && gpGlobals->time - m_flBirthTime >= 2 )
	{
		UTIL_SetSize( pev, Vector( -12, -12, 0 ), Vector( 12, 12, 4 ) );
		m_fRoachSolid = TRUE;
	}

	if ( gpGlobals->time - m_flBirthTime >= gSkillData.shockroachLifespan )
		TakeDamage( pev, pev, pev->health, DMG_NEVERGIB );

	CBaseMonster::MonsterThink();
}

void COFShockRoach::PrescheduleThink()
{
	if ( m_MonsterState == MONSTERSTATE_COMBAT && RANDOM_FLOAT( 0, 5 ) < 0.1 )
		IdleSound();
}

void COFShockRoach::StartTask( Task_t* pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	if ( pTask->iTask == TASK_RANGE_ATTACK1 )
	{
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch( &COFShockRoach::LeapTouch );
	}
	else
		CBaseMonster::StartTask( pTask );
}

void COFShockRoach::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_RANGE_ATTACK1 || pTask->iTask == TASK_RANGE_ATTACK2 )
	{
		if ( m_fSequenceFinished )
		{
			TaskComplete();
			SetTouch( NULL );
			m_IdealActivity = ACT_IDLE;
		}
	}
	else
		CBaseMonster::RunTask( pTask );
}

Schedule_t* COFShockRoach::GetScheduleOfType( int Type )
{
	if ( Type == SCHED_RANGE_ATTACK1 )
		return slSRRangeAttack1;

	return CBaseMonster::GetScheduleOfType( Type );
}

void COFShockRoach::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	if ( pEvent->event != SR_AE_JUMPATTACK )
	{
		CBaseMonster::HandleAnimEvent( pEvent );
		return;
	}

	ClearBits( pev->flags, FL_ONGROUND );
	UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );
	UTIL_MakeVectors( pev->angles );
	Vector vecJumpDir;
	if ( m_hEnemy != NULL )
	{
		float gravity = 1;
		if ( gravity < g_psv_gravity->value )
			gravity = g_psv_gravity->value;

		float height = m_hEnemy->pev->origin.z + m_hEnemy->pev->view_ofs.z - pev->origin.z;
		if ( !( height >= 16 ) )
			height = 16;

		float speed = sqrt( 2 * gravity * height );
		float time = speed / gravity;
		vecJumpDir = ( m_hEnemy->pev->origin + m_hEnemy->pev->view_ofs - pev->origin ) * ( 1.0 / time );
		vecJumpDir.z = speed;
		float distance = vecJumpDir.Length();
		if ( distance > 650 )
			vecJumpDir = vecJumpDir * ( 650.0 / distance );
	}
	else
		vecJumpDir = Vector( gpGlobals->v_forward.x, gpGlobals->v_forward.y, gpGlobals->v_up.z ) * 350;

	pev->velocity = vecJumpDir;
	m_flNextAttack = gpGlobals->time + 2;
}

void COFShockRoach::LeapTouch( CBaseEntity* pOther )
{
	if ( !pOther->pev->takedamage || pOther->Classify() == Classify() )
		return;

	if ( pOther->IsPlayer() )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pOther;
		if ( !pPlayer->HasNamedPlayerItem( "weapon_shockrifle" ) )
		{
			pPlayer->GiveNamedItem( "weapon_shockrifle" );
			SetTouch( NULL );
			UTIL_Remove( this );
			return;
		}
	}
	else if ( pev->flags & FL_ONGROUND )
	{
		SetTouch( NULL );
		return;
	}

	int pitch = GetVoicePitch();
	float volume = GetSoundVolue();
	int sound = RANDOM_LONG( 0, ARRAYSIZE( pBiteSounds ) - 1 );
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, pBiteSounds[sound], volume, ATTN_IDLE, 0, pitch );
	pOther->TakeDamage( pev, pev, GetDamageAmount(), DMG_SLASH );
	SetTouch( NULL );
}

float COFShockRoach::GetDamageAmount()
{
	return gSkillData.shockroachDmgBite;
}

int COFShockRoach::GetVoicePitch()
{
	return PITCH_NORM;
}

float COFShockRoach::GetSoundVolue()
{
	return 1;
}
