#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "animation.h"
#include "schedule.h"
#include "defaultai.h"
#include "soundent.h"

#define NUM_SCIENTIST_HEADS	4

enum
{
	SCHED_HIDE = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_FEAR,
	SCHED_PANIC,
	SCHED_STARTLE,
	SCHED_TARGET_CHASE_SCARED,
	SCHED_TARGET_FACE_SCARED
};

enum
{
	TASK_SAY_HEAL = LAST_TALKMONSTER_TASK + 1,
	TASK_HEAL,
	TASK_SAY_FEAR,
	TASK_RUN_PATH_SCARED,
	TASK_SCREAM,
	TASK_RANDOM_SCREAM,
	TASK_MOVE_TO_TARGET_RANGE_SCARED
};

#define ROSENBERG_AE_HEAL		1
#define ROSENBERG_AE_NEEDLEON	2
#define ROSENBERG_AE_NEEDLEOFF	3

class CRosenberg : public CTalkMonster
{
public:
	virtual void Spawn();
	virtual void Precache();
	virtual void SetYawSpeed();
	virtual int Classify();
	virtual void HandleAnimEvent( MonsterEvent_t* pEvent );
	virtual void RunTask( Task_t* pTask );
	virtual void StartTask( Task_t* pTask );
	virtual int ObjectCaps();
	virtual int FriendNumber( int arrayNumber );
	virtual void SetActivity( Activity newActivity );
	virtual Activity GetStoppedActivity();
	virtual int ISoundMask();
	virtual void DeclineFollowing();
	virtual float CoverRadius();

	BOOL CanHeal();
	void Heal();
	void Scream();

	virtual Schedule_t* GetScheduleOfType( int Type );
	virtual Schedule_t* GetSchedule();
	virtual MONSTERSTATE GetIdealState();
	virtual void DeathSound();
	virtual void PainSound();
	void TalkInit();
	virtual void Killed( entvars_t* pevAttacker, int iGib );
	virtual int Save( CSave& save );
	virtual int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:
	float	m_painTime;
	float	m_healTime;
	float	m_fearTime;

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_rosenberg, CRosenberg );

TYPEDESCRIPTION CRosenberg::m_SaveData[] =
{
	DEFINE_FIELD( CRosenberg, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CRosenberg, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CRosenberg, m_fearTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CRosenberg, CTalkMonster );

Task_t tlRoFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CANT_FOLLOW },
	{ TASK_MOVE_TO_TARGET_RANGE, 128 }
};

Schedule_t slRoFollow[] =
{
	{
		tlRoFollow, ARRAYSIZE( tlRoFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "Follow"
	}
};

Task_t tlRoFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, 128 }
};

Schedule_t slRoFollowScared[] =
{
	{
		tlRoFollowScared, ARRAYSIZE( tlRoFollowScared ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "FollowScared"
	}
};

Task_t tlRoFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED }
};

Schedule_t slRoFaceTargetScared[] =
{
	{
		tlRoFaceTargetScared, ARRAYSIZE( tlRoFaceTargetScared ),
		bits_COND_HEAR_SOUND | bits_COND_NEW_ENEMY, bits_SOUND_DANGER, "FaceTargetScared"
	}
};

Task_t tlRoStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, 0 }
};

Schedule_t slRoStopFollowing[] =
{
	{ tlRoStopFollowing, ARRAYSIZE( tlRoStopFollowing ), 0, 0, "StopFollowing" }
};

Task_t tlRoHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 50 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_SAY_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_ARM },
	{ TASK_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_DISARM }
};

Schedule_t slRoHeal[] =
{
	{ tlRoHeal, ARRAYSIZE( tlRoHeal ), 0, 0, "Heal" }
};

Task_t tlRoFaceTarget[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE }
};

Schedule_t slRoFaceTarget[] =
{
	{
		tlRoFaceTarget, ARRAYSIZE( tlRoFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "FaceTarget"
	}
};

Task_t tlRoSciPanic[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_SCREAM, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE }
};

Schedule_t slRoSciPanic[] =
{
	{ tlRoSciPanic, ARRAYSIZE( tlRoSciPanic ), 0, 0, "SciPanic" }
};

Task_t tlRoIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 }
};

Schedule_t slRoIdleSciStand[] =
{
	{
		tlRoIdleSciStand, ARRAYSIZE( tlRoIdleSciStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND
			| bits_COND_SMELL | bits_COND_CLIENT_PUSH | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleSciStand"
	}
};

Task_t tlRoScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH_SCARED, 0 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE }
};

Schedule_t slRoScientistCover[] =
{
	{ tlRoScientistCover, ARRAYSIZE( tlRoScientistCover ), bits_COND_NEW_ENEMY, 0, "ScientistCover" }
};

Task_t tlRoScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 10 }
};

Schedule_t slRoScientistHide[] =
{
	{
		tlRoScientistHide, ARRAYSIZE( tlRoScientistHide ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_SEE_ENEMY | bits_COND_SEE_HATE
			| bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE,
		bits_SOUND_DANGER, "ScientistHide"
	}
};

Task_t tlRoScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_RANDOM_SCREAM, 0.3f },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, 0.1f },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 1 }
};

Schedule_t slRoScientistStartle[] =
{
	{
		tlRoScientistStartle, ARRAYSIZE( tlRoScientistStartle ),
		bits_COND_NEW_ENEMY | bits_COND_SEE_ENEMY | bits_COND_SEE_HATE | bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE,
		0, "ScientistStartle"
	}
};

Task_t tlRoFear[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_SAY_FEAR, 0 }
};

Schedule_t slRoFear[] =
{
	{ tlRoFear, ARRAYSIZE( tlRoFear ), bits_COND_NEW_ENEMY, 0, "Fear" }
};

DEFINE_CUSTOM_SCHEDULES( CRosenberg )
{
	slRoFollow,
	slRoFaceTarget,
	slRoIdleSciStand,
	slRoFear,
	slRoScientistCover,
	slRoScientistHide,
	slRoScientistStartle,
	slRoHeal,
	slRoStopFollowing,
	slRoSciPanic,
	slRoFollowScared,
	slRoFaceTargetScared
};

IMPLEMENT_CUSTOM_SCHEDULES( CRosenberg, CTalkMonster );

int CRosenberg::Classify()
{
	return CLASS_HUMAN_PASSIVE;
}

void CRosenberg::SetYawSpeed()
{
	int speed = 90;

	switch ( m_Activity )
	{
	case ACT_IDLE:
		speed = 120;
		break;
	case ACT_WALK:
		speed = 180;
		break;
	case ACT_RUN:
		speed = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		speed = 120;
		break;
	default:
		break;
	}

	pev->yaw_speed = (float)speed;
}

int CRosenberg::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_PLAYER;
}

void CRosenberg::DeathSound()
{
	PainSound();
}

int CRosenberg::FriendNumber( int arrayNumber )
{
	static int array[] = { 1, 2, 0 };

	if ( arrayNumber < 3 )
		return array[arrayNumber];
	return arrayNumber;
}

Activity CRosenberg::GetStoppedActivity()
{
	return m_hEnemy != NULL ? ACT_EXCITED : ACT_IDLE;
}

void CRosenberg::SetActivity( Activity newActivity )
{
	if ( LookupActivity( newActivity ) == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}

void CRosenberg::Killed( entvars_t* pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CRosenberg::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/scientist.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.scientistHealth;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_OPEN_DOORS | bits_CAP_AUTO_DOORS | bits_CAP_USE;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	pev->skin = 0;
	if ( pev->body == -1 )
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );
	if ( pev->body == 2 )
		pev->skin = 1;
	MonsterInit();
	if ( !( pev->spawnflags & SF_MONSTER_PREDISASTER ) )
		SetUse( &CRosenberg::FollowerUse );
}

void CRosenberg::DeclineFollowing()
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( "RO_POK", 2, VOL_NORM, ATTN_NORM );
}

void CRosenberg::PainSound()
{
	if ( !( m_painTime <= gpGlobals->time ) )
		return;

	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );
	switch ( RANDOM_LONG( 0, 4 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rosenberg/ro_pain1.wav", 1, ATTN_NORM, 0, 100 );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rosenberg/ro_pain2.wav", 1, ATTN_NORM, 0, 100 );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rosenberg/ro_pain3.wav", 1, ATTN_NORM, 0, 100 );
		break;
	case 3:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rosenberg/ro_pain4.wav", 1, ATTN_NORM, 0, 100 );
		break;
	case 4:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "rosenberg/ro_pain5.wav", 1, ATTN_NORM, 0, 100 );
		break;
	}
}

void CRosenberg::Precache()
{
	PRECACHE_MODEL( "models/scientist.mdl" );
	PRECACHE_SOUND( "rosenberg/ro_pain1.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain2.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain3.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain4.wav" );
	PRECACHE_SOUND( "rosenberg/ro_pain5.wav" );
	TalkInit();
	CTalkMonster::Precache();
}

void CRosenberg::TalkInit()
{
	CTalkMonster::TalkInit();
	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";
	m_szGrp[TLK_ANSWER] = "RO_ANSWER";
	m_szGrp[TLK_QUESTION] = "RO_QUESTION";
	m_szGrp[TLK_IDLE] = "RO_IDLE";
	m_szGrp[TLK_STARE] = "RO_STARE";
	m_szGrp[TLK_USE] = "RO_OK";
	m_szGrp[TLK_UNUSE] = "RO_WAIT";
	m_szGrp[TLK_STOP] = "RO_STOP";
	m_szGrp[TLK_NOSHOOT] = "RO_SCARED";
	m_szGrp[TLK_HELLO] = "RO_HELLO";
	m_szGrp[TLK_PLHURT1] = "!RO_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RO_CUREB";
	m_szGrp[TLK_PLHURT3] = "!RO_CUREC";
	m_szGrp[TLK_PHELLO] = "RO_PHELLO";
	m_szGrp[TLK_PIDLE] = "RO_PIDLE";
	m_szGrp[TLK_PQUESTION] = "RO_PQUEST";
	m_szGrp[TLK_SMELL] = "RO_SMELL";
	m_szGrp[TLK_WOUND] = "RO_WOUND";
	m_szGrp[TLK_MORTAL] = "RO_MORTAL";
	m_voicePitch = 100;
}

void CRosenberg::Scream()
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "RO_SCREAM", RANDOM_FLOAT( 3, 6 ), VOL_NORM, ATTN_NORM );
	}
}

BOOL CRosenberg::CanHeal()
{
	return m_healTime <= gpGlobals->time && m_hTargetEnt != NULL
		&& m_hTargetEnt->pev->health <= 0.5 * m_hTargetEnt->pev->max_health;
}

void CRosenberg::Heal()
{
	if ( CanHeal() && ( m_hTargetEnt->pev->origin - pev->origin ).Length() <= 100 )
	{
		m_hTargetEnt->TakeHealth( gSkillData.scientistHeal, DMG_GENERIC );
		m_healTime = gpGlobals->time + 60;
	}
}

void CRosenberg::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case ROSENBERG_AE_NEEDLEON:
		pev->body = pev->body % NUM_SCIENTIST_HEADS + NUM_SCIENTIST_HEADS;
		break;
	case ROSENBERG_AE_NEEDLEOFF:
		pev->body %= NUM_SCIENTIST_HEADS;
		break;
	case ROSENBERG_AE_HEAL:
		Heal();
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

int CRosenberg::ObjectCaps()
{
	return FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE;
}

float CRosenberg::CoverRadius()
{
	return 1200;
}

void CRosenberg::StartTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "RO_HEAL", 2, VOL_NORM, ATTN_IDLE );
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_SAY_FEAR:
		if ( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			if ( m_hEnemy->IsPlayer() )
				PlaySentence( "RO_PLFEAR", 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( "RO_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_SCREAM:
		Scream();
		TaskComplete();
		break;
	case TASK_RANDOM_SCREAM:
		if ( RANDOM_FLOAT( 0, 1 ) < pTask->flData )
			Scream();
		TaskComplete();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		if ( ( m_hTargetEnt->pev->origin - pev->origin ).Length() < 1 )
		{
			TaskComplete();
		}
		else
		{
			m_vecMoveGoal = m_hTargetEnt->pev->origin;
			if ( !MoveToTarget( ACT_WALK_SCARED, 0.5 ) )
				TaskFail();
		}
		break;
	default:
		CTalkMonster::StartTask( pTask );
		break;
	}
}

void CRosenberg::RunTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH_SCARED:
		if ( MovementIsComplete() )
			TaskComplete();
		if ( RANDOM_LONG( 0, 31 ) < 8 )
			Scream();
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		if ( RANDOM_LONG( 0, 63 ) < 8 )
			Scream();
		if ( m_hEnemy == NULL )
		{
			TaskFail();
		}
		else
		{
			float distance = ( m_vecMoveGoal - pev->origin ).Length2D();
			if ( distance < pTask->flData
				|| !( ( m_vecMoveGoal - m_hTargetEnt->pev->origin ).Length() <= pTask->flData * 0.5 ) )
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				FRefreshRoute();
			}
			if ( !( pTask->flData <= distance ) )
			{
				TaskComplete();
				RouteClear();
			}
			else if ( !( distance >= 190 ) && m_movementActivity != ACT_WALK_SCARED )
			{
				m_movementActivity = ACT_WALK_SCARED;
			}
			else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
			{
				m_movementActivity = ACT_RUN_SCARED;
			}
		}
		break;
	case TASK_HEAL:
		if ( m_fSequenceFinished )
		{
			TaskComplete();
		}
		else
		{
			if ( TargetDistance() > 90 )
				TaskComplete();
			pev->ideal_yaw = UTIL_VecToYaw( m_hTargetEnt->pev->origin - pev->origin );
			ChangeYaw( (int)pev->yaw_speed );
		}
		break;
	default:
		CTalkMonster::RunTask( pTask );
		break;
	}
}

Schedule_t* CRosenberg::GetScheduleOfType( int Type )
{
	Schedule_t* pSchedule;

	switch ( Type )
	{
	case SCHED_IDLE_STAND:
		pSchedule = CTalkMonster::GetScheduleOfType( Type );
		return pSchedule == slIdleStand ? slRoIdleSciStand : pSchedule;
	case SCHED_TARGET_FACE:
		pSchedule = CTalkMonster::GetScheduleOfType( Type );
		return pSchedule == slIdleStand ? slRoFaceTarget : pSchedule;
	case SCHED_TARGET_CHASE:
		return slRoFollow;
	case SCHED_CANT_FOLLOW:
		return slRoStopFollowing;
	case SCHED_HIDE:
		return slRoScientistHide;
	case SCHED_FEAR:
		return slRoFear;
	case SCHED_PANIC:
		return slRoSciPanic;
	case SCHED_STARTLE:
		return slRoScientistStartle;
	case SCHED_TARGET_CHASE_SCARED:
		return slRoFollowScared;
	case SCHED_TARGET_FACE_SCARED:
		return slRoFaceTargetScared;
	default:
		return CTalkMonster::GetScheduleOfType( Type );
	}
}

MONSTERSTATE CRosenberg::GetIdealState()
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( IsFollowing() )
			{
				if ( IRelationship( m_hEnemy ) == R_FR
					&& HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
				{
					StopFollowing( TRUE );
				}
				else
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return MONSTERSTATE_ALERT;
				}
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && IsFollowing() )
		{
			StopFollowing( TRUE );
		}
		break;
	case MONSTERSTATE_COMBAT:
		{
			CBaseEntity* pEnemy = m_hEnemy;
			if ( pEnemy != NULL )
			{
				if ( !pEnemy->IsAlive() || gpGlobals->time - m_fearTime > 15 )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					m_hEnemy = NULL;
					return m_IdealMonsterState;
				}
				if ( m_hTargetEnt != NULL )
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return MONSTERSTATE_ALERT;
				}
				if ( HasConditions( bits_COND_SEE_ENEMY ) )
				{
					m_fearTime = gpGlobals->time;
					m_IdealMonsterState = MONSTERSTATE_COMBAT;
					return MONSTERSTATE_COMBAT;
				}
			}
		}
		break;
	default:
		break;
	}

	return CBaseMonster::GetIdealState();
}

Schedule_t* CRosenberg::GetSchedule()
{
	CBaseEntity* pEnemy = m_hEnemy;

	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}

	if ( m_MonsterState == MONSTERSTATE_COMBAT )
	{
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return slRoFear;
		if ( !HasConditions( bits_COND_SEE_ENEMY ) && HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;
		return slRoScientistCover;
	}
	if ( m_MonsterState != MONSTERSTATE_IDLE && m_MonsterState != MONSTERSTATE_ALERT )
		return CBaseMonster::GetSchedule();

	if ( pEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
		{
			m_fearTime = gpGlobals->time;
		}
		else if ( !pEnemy->IsAlive() || gpGlobals->time - m_fearTime > 15 )
		{
			m_hEnemy = NULL;
			pEnemy = NULL;
		}
	}
	if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
		return GetScheduleOfType( SCHED_SMALL_FLINCH );
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & ( bits_SOUND_DANGER | bits_SOUND_COMBAT ) )
			&& gpGlobals->time - m_fearTime > 3 )
		{
			m_fearTime = gpGlobals->time;
			return GetScheduleOfType( SCHED_STARTLE );
		}
	}

	if ( !IsFollowing() )
	{
		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		TrySmellTalk();
		return CBaseMonster::GetSchedule();
	}
	if ( !m_hTargetEnt->IsAlive() )
	{
		StopFollowing( FALSE );
		return CBaseMonster::GetSchedule();
	}
	if ( pEnemy != NULL )
	{
		int relationship = IRelationship( pEnemy );
		if ( relationship == R_DL || relationship == R_HT )
		{
			if ( HasConditions( bits_COND_NEW_ENEMY ) )
				return GetScheduleOfType( SCHED_FEAR );
			return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );
		}
	}
	if ( TargetDistance() <= 128 )
	{
		if ( CanHeal() )
			return slRoHeal;
		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
			return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
	}
	return GetScheduleOfType( SCHED_TARGET_FACE );
}
