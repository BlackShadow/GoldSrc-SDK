#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "soundent.h"
#include "animation.h"

#define NUM_SCIENTIST_HEADS		4
#define SCIENTIST_AE_HEAL			1
#define SCIENTIST_AE_NEEDLEON		2
#define SCIENTIST_AE_NEEDLEOFF	3

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

class CCleansuitScientist : public CTalkMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	void RunTask( Task_t* pTask );
	void StartTask( Task_t* pTask );
	int ObjectCaps() { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	int FriendNumber( int arrayNumber );
	void SetActivity( Activity newActivity );
	Activity GetStoppedActivity();
	int ISoundMask();
	void DeclineFollowing();
	float CoverRadius() { return 1200; }
	BOOL DisregardEnemy( CBaseEntity* pEnemy )
	{
		return !pEnemy->IsAlive() || gpGlobals->time - m_fearTime > 15;
	}
	BOOL CanHeal();
	void Heal();
	void Scream();
	Schedule_t* GetScheduleOfType( int Type );
	Schedule_t* GetSchedule();
	MONSTERSTATE GetIdealState();
	void DeathSound();
	void PainSound();
	void TalkInit();
	void Killed( entvars_t* pevAttacker, int iGib );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	CUSTOM_SCHEDULES;

private:
	float	m_painTime;
	float	m_healTime;
	float	m_fearTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_cleansuit_scientist, CCleansuitScientist );

TYPEDESCRIPTION CCleansuitScientist::m_SaveData[] =
{
	DEFINE_FIELD( CCleansuitScientist, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CCleansuitScientist, m_healTime, FIELD_TIME ),
	DEFINE_FIELD( CCleansuitScientist, m_fearTime, FIELD_TIME )
};

IMPLEMENT_SAVERESTORE( CCleansuitScientist, CTalkMonster );

Task_t tlcssFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_CANT_FOLLOW },
	{ TASK_MOVE_TO_TARGET_RANGE, 128 }
};

Schedule_t slcssFollow[] =
{
	{ tlcssFollow, ARRAYSIZE( tlcssFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "Follow" }
};

Task_t tlcssFollowScared[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },
	{ TASK_MOVE_TO_TARGET_RANGE_SCARED, 128 }
};

Schedule_t slcssFollowScared[] =
{
	{ tlcssFollowScared, ARRAYSIZE( tlcssFollowScared ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		bits_SOUND_DANGER, "FollowScared" }
};

Task_t tlcssFaceTargetScared[] =
{
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE_SCARED }
};

Schedule_t slcssFaceTargetScared[] =
{
	{ tlcssFaceTargetScared, ARRAYSIZE( tlcssFaceTargetScared ),
		bits_COND_HEAR_SOUND | bits_COND_NEW_ENEMY, bits_SOUND_DANGER, "FaceTargetScared" }
};

Task_t tlcssStopFollowing[] =
{
	{ TASK_CANT_FOLLOW, 0 }
};

Schedule_t slcssStopFollowing[] =
{
	{ tlcssStopFollowing, ARRAYSIZE( tlcssStopFollowing ), 0, 0, "StopFollowing" }
};

Task_t tlcssHeal[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 50 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_CHASE },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_SAY_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_ARM },
	{ TASK_HEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_TARGET, (float)ACT_DISARM }
};

Schedule_t slcssHeal[] =
{
	{ tlcssHeal, ARRAYSIZE( tlcssHeal ), 0, 0, "Heal" }
};

Task_t tlcssFaceTarget[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE }
};

Schedule_t slcssFaceTarget[] =
{
	{ tlcssFaceTarget, ARRAYSIZE( tlcssFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND,
		bits_SOUND_COMBAT | bits_SOUND_DANGER, "FaceTarget" }
};

Task_t tlcssSciPanic[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_SCREAM, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_EXCITED },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE }
};

Schedule_t slcssSciPanic[] =
{
	{ tlcssSciPanic, ARRAYSIZE( tlcssSciPanic ), 0, 0, "SciPanic" }
};

Task_t tlcssIdleSciStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 }
};

Schedule_t slcssIdleSciStand[] =
{
	{ tlcssIdleSciStand, ARRAYSIZE( tlcssIdleSciStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_CLIENT_PUSH | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleSciStand" }
};

Task_t tlcssScientistCover[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH_SCARED, 0 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_SET_SCHEDULE, (float)SCHED_HIDE }
};

Schedule_t slcssScientistCover[] =
{
	{ tlcssScientistCover, ARRAYSIZE( tlcssScientistCover ), bits_COND_NEW_ENEMY, 0, "ScientistCover" }
};

Task_t tlcssScientistHide[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_CROUCH },
	{ TASK_SET_ACTIVITY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 10 }
};

Schedule_t slcssScientistHide[] =
{
	{ tlcssScientistHide, ARRAYSIZE( tlcssScientistHide ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_SEE_ENEMY | bits_COND_SEE_HATE |
		bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE, bits_SOUND_DANGER, "ScientistHide" }
};

Task_t tlcssScientistStartle[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_PANIC },
	{ TASK_RANDOM_SCREAM, 0.3 },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_RANDOM_SCREAM, 0.1 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCHIDLE },
	{ TASK_WAIT_RANDOM, 1 }
};

Schedule_t slcssScientistStartle[] =
{
	{ tlcssScientistStartle, ARRAYSIZE( tlcssScientistStartle ),
		bits_COND_NEW_ENEMY | bits_COND_SEE_ENEMY | bits_COND_SEE_HATE | bits_COND_SEE_FEAR | bits_COND_SEE_DISLIKE,
		0, "ScientistStartle" }
};

Task_t tlcssFear[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_SAY_FEAR, 0 }
};

Schedule_t slcssFear[] =
{
	{ tlcssFear, ARRAYSIZE( tlcssFear ), bits_COND_NEW_ENEMY, 0, "Fear" }
};

DEFINE_CUSTOM_SCHEDULES( CCleansuitScientist )
{
	slcssFollow,
	slcssFaceTarget,
	slcssIdleSciStand,
	slcssFear,
	slcssScientistCover,
	slcssScientistHide,
	slcssScientistStartle,
	slcssHeal,
	slcssStopFollowing,
	slcssSciPanic,
	slcssFollowScared,
	slcssFaceTargetScared
};

IMPLEMENT_CUSTOM_SCHEDULES( CCleansuitScientist, CTalkMonster );

void CCleansuitScientist::DeclineFollowing()
{
	Talk( 10 );
	m_hTalkTarget = m_hEnemy;
	PlaySentence( "SC_POK", 2, VOL_NORM, ATTN_NORM );
}

void CCleansuitScientist::Scream()
{
	if ( FOkToSpeak() )
	{
		Talk( 10 );
		m_hTalkTarget = m_hEnemy;
		PlaySentence( "SC_SCREAM", RANDOM_FLOAT( 3, 6 ), VOL_NORM, ATTN_NORM );
	}
}

Activity CCleansuitScientist::GetStoppedActivity()
{
	return m_hEnemy != NULL ? ACT_EXCITED : ACT_IDLE;
}

void CCleansuitScientist::StartTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_SAY_HEAL:
		Talk( 2 );
		m_hTalkTarget = m_hTargetEnt;
		PlaySentence( "SC_HEAL", 2, VOL_NORM, ATTN_IDLE );
		TaskComplete();
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
	case TASK_SAY_FEAR:
		if ( FOkToSpeak() )
		{
			Talk( 2 );
			m_hTalkTarget = m_hEnemy;
			if ( m_hEnemy->IsPlayer() )
				PlaySentence( "SC_PLFEAR", 5, VOL_NORM, ATTN_NORM );
			else
				PlaySentence( "SC_FEAR", 5, VOL_NORM, ATTN_NORM );
		}
		TaskComplete();
		break;
	case TASK_HEAL:
		m_IdealActivity = ACT_MELEE_ATTACK1;
		break;
	case TASK_RUN_PATH_SCARED:
		m_movementActivity = ACT_RUN_SCARED;
		break;
	case TASK_MOVE_TO_TARGET_RANGE_SCARED:
		if ( ( m_hTargetEnt->pev->origin - pev->origin ).Length() < 1 )
			TaskComplete();
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

void CCleansuitScientist::RunTask( Task_t* pTask )
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
			TaskFail();
		else
		{
			float distance = ( m_vecMoveGoal - pev->origin ).Length2D();
			if ( distance < pTask->flData ||
				( m_vecMoveGoal - m_hTargetEnt->pev->origin ).Length() > pTask->flData * 0.5 )
			{
				m_vecMoveGoal = m_hTargetEnt->pev->origin;
				distance = ( m_vecMoveGoal - pev->origin ).Length2D();
				FRefreshRoute();
			}
			if ( distance < pTask->flData )
			{
				TaskComplete();
				RouteClear();
			}
			else if ( distance < 190 && m_movementActivity != ACT_WALK_SCARED )
				m_movementActivity = ACT_WALK_SCARED;
			else if ( distance >= 270 && m_movementActivity != ACT_RUN_SCARED )
				m_movementActivity = ACT_RUN_SCARED;
		}
		break;
	case TASK_HEAL:
		if ( m_fSequenceFinished )
			TaskComplete();
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

int CCleansuitScientist::Classify()
{
	return CLASS_HUMAN_PASSIVE;
}

void CCleansuitScientist::SetYawSpeed()
{
	int ys = 90;
	switch ( m_Activity )
	{
	case ACT_IDLE:
		ys = 120;
		break;
	case ACT_WALK:
		ys = 180;
		break;
	case ACT_RUN:
		ys = 150;
		break;
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		ys = 120;
		break;
	}
	pev->yaw_speed = ys;
}

void CCleansuitScientist::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case SCIENTIST_AE_HEAL:
		Heal();
		break;
	case SCIENTIST_AE_NEEDLEON:
		pev->body = pev->body % NUM_SCIENTIST_HEADS + NUM_SCIENTIST_HEADS;
		break;
	case SCIENTIST_AE_NEEDLEOFF:
		pev->body %= NUM_SCIENTIST_HEADS;
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CCleansuitScientist::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/cleansuit_scientist.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.cleansuitScientistHealth;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	pev->skin = 0;
	if ( pev->body == -1 )
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );
	if ( pev->body == 2 )
		pev->skin = 1;
	MonsterInit();
	SetUse( &CCleansuitScientist::FollowerUse );
}

void CCleansuitScientist::Precache()
{
	PRECACHE_MODEL( "models/cleansuit_scientist.mdl" );
	PRECACHE_SOUND( "scientist/sci_pain1.wav" );
	PRECACHE_SOUND( "scientist/sci_pain2.wav" );
	PRECACHE_SOUND( "scientist/sci_pain3.wav" );
	PRECACHE_SOUND( "scientist/sci_pain4.wav" );
	PRECACHE_SOUND( "scientist/sci_pain5.wav" );
	TalkInit();
	CTalkMonster::Precache();
}

void CCleansuitScientist::TalkInit()
{
	CTalkMonster::TalkInit();
	m_szFriends[0] = "monster_scientist";
	m_szFriends[1] = "monster_sitting_scientist";
	m_szFriends[2] = "monster_barney";
	m_szGrp[TLK_ANSWER] = "SC_ANSWER";
	m_szGrp[TLK_QUESTION] = "SC_QUESTION";
	m_szGrp[TLK_IDLE] = "SC_IDLE";
	m_szGrp[TLK_STARE] = "SC_STARE";
	m_szGrp[TLK_USE] = "SC_OK";
	m_szGrp[TLK_UNUSE] = "SC_WAIT";
	m_szGrp[TLK_STOP] = "SC_STOP";
	m_szGrp[TLK_NOSHOOT] = "SC_SCARED";
	m_szGrp[TLK_HELLO] = "SC_HELLO";
	m_szGrp[TLK_PLHURT1] = "!SC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!SC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!SC_CUREC";
	m_szGrp[TLK_PHELLO] = "SC_PHELLO";
	m_szGrp[TLK_PIDLE] = "SC_PIDLE";
	m_szGrp[TLK_PQUESTION] = "SC_PQUEST";
	m_szGrp[TLK_SMELL] = "SC_SMELL";
	m_szGrp[TLK_WOUND] = "SC_WOUND";
	m_szGrp[TLK_MORTAL] = "SC_MORTAL";
	switch ( pev->body % 3 )
	{
	case 1:
		m_voicePitch = 100;
		break;
	case 2:
		m_voicePitch = 95;
		break;
	default:
		m_voicePitch = 105;
		break;
	}
}

int CCleansuitScientist::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	if ( pevInflictor && ( pevInflictor->flags & FL_CLIENT ) )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}
	return CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

int CCleansuitScientist::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_PLAYER;
}

void CCleansuitScientist::PainSound()
{
	if ( gpGlobals->time < m_painTime )
		return;
	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );
	switch ( RANDOM_LONG( 0, 4 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 3:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain4.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 4:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "scientist/sci_pain5.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CCleansuitScientist::DeathSound()
{
	PainSound();
}

void CCleansuitScientist::Killed( entvars_t* pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CCleansuitScientist::SetActivity( Activity newActivity )
{
	if ( LookupActivity( newActivity ) == ACTIVITY_NOT_AVAILABLE )
		newActivity = ACT_IDLE;
	CTalkMonster::SetActivity( newActivity );
}

Schedule_t* CCleansuitScientist::GetScheduleOfType( int Type )
{
	Schedule_t* psched;
	switch ( Type )
	{
	case SCHED_IDLE_STAND:
		psched = CTalkMonster::GetScheduleOfType( Type );
		return psched == slIdleStand ? slcssIdleSciStand : psched;
	case SCHED_TARGET_FACE:
		psched = CTalkMonster::GetScheduleOfType( Type );
		return psched == slIdleStand ? slcssFaceTarget : psched;
	case SCHED_TARGET_CHASE:
		return slcssFollow;
	case SCHED_CANT_FOLLOW:
		return slcssStopFollowing;
	case SCHED_HIDE:
		return slcssScientistHide;
	case SCHED_FEAR:
		return slcssFear;
	case SCHED_PANIC:
		return slcssSciPanic;
	case SCHED_STARTLE:
		return slcssScientistStartle;
	case SCHED_TARGET_CHASE_SCARED:
		return slcssFollowScared;
	case SCHED_TARGET_FACE_SCARED:
		return slcssFaceTargetScared;
	}
	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t* CCleansuitScientist::GetSchedule()
{
	CBaseEntity* pEnemy = m_hEnemy;
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( pEnemy )
		{
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
				m_fearTime = gpGlobals->time;
			else if ( DisregardEnemy( pEnemy ) )
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
			if ( pSound && ( pSound->m_iType & ( bits_SOUND_COMBAT | bits_SOUND_DANGER ) ) &&
				gpGlobals->time - m_fearTime > 3 )
			{
				m_fearTime = gpGlobals->time;
				return GetScheduleOfType( SCHED_STARTLE );
			}
		}
		if ( IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				StopFollowing( FALSE );
				break;
			}
			int relationship = pEnemy ? IRelationship( pEnemy ) : R_NO;
			if ( relationship == R_DL || relationship == R_HT )
			{
				if ( HasConditions( bits_COND_NEW_ENEMY ) )
					return GetScheduleOfType( SCHED_FEAR );
				return GetScheduleOfType( SCHED_TARGET_FACE_SCARED );
			}
			if ( TargetDistance() <= 128 && HasConditions( bits_COND_CLIENT_PUSH ) )
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			return GetScheduleOfType( SCHED_TARGET_FACE );
		}
		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		TrySmellTalk();
		break;
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
			return slcssFear;
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			return slcssScientistCover;
		if ( HasConditions( bits_COND_HEAR_SOUND ) )
			return slTakeCoverFromBestSound;
		return slcssScientistCover;
	}
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CCleansuitScientist::GetIdealState()
{
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_NEW_ENEMY ) )
		{
			if ( IsFollowing() )
			{
				if ( IRelationship( m_hEnemy ) == R_FR &&
					HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
					StopFollowing( TRUE );
				else
				{
					m_IdealMonsterState = MONSTERSTATE_ALERT;
					return m_IdealMonsterState;
				}
			}
		}
		else if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) && IsFollowing() )
			StopFollowing( TRUE );
		break;
	case MONSTERSTATE_COMBAT:
	{
		CBaseEntity* pEnemy = m_hEnemy;
		if ( pEnemy )
		{
			if ( DisregardEnemy( pEnemy ) )
			{
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				m_hEnemy = NULL;
				return m_IdealMonsterState;
			}
			if ( m_hTargetEnt != NULL )
			{
				m_IdealMonsterState = MONSTERSTATE_ALERT;
				return m_IdealMonsterState;
			}
			if ( HasConditions( bits_COND_SEE_ENEMY ) )
			{
				m_fearTime = gpGlobals->time;
				m_IdealMonsterState = MONSTERSTATE_COMBAT;
				return m_IdealMonsterState;
			}
		}
		break;
	}
	}
	return CTalkMonster::GetIdealState();
}

BOOL CCleansuitScientist::CanHeal()
{
	return FALSE;
}

void CCleansuitScientist::Heal()
{
}

int CCleansuitScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 1, 2, 0 };
	if ( arrayNumber <= 2 )
		return array[arrayNumber];
	return arrayNumber;
}

class CDeadCleansuitScientist : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_HUMAN_PASSIVE; }
	void KeyValue( KeyValueData* pkvd );

	int m_iPose;
	static char* m_szPoses[9];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_cleansuit_scientist_dead, CDeadCleansuitScientist );

char* CDeadCleansuitScientist::m_szPoses[9] =
{
	"lying_on_back",
	"lying_on_stomach",
	"dead_sitting",
	"dead_hang",
	"dead_table1",
	"dead_table2",
	"dead_table3",
	"scientist_deadpose1",
	"dead_against_wall"
};

void CDeadCleansuitScientist::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CDeadCleansuitScientist::Spawn()
{
	PRECACHE_MODEL( "models/cleansuit_scientist.mdl" );
	SET_MODEL( ENT( pev ), "models/cleansuit_scientist.mdl" );
	pev->effects = 0;
	pev->sequence = 0;
	pev->health = 8;
	m_bloodColor = BLOOD_COLOR_RED;
	if ( pev->body == -1 )
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );
	pev->skin = pev->body == 2;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead cleansuit scientist with bad pose\n" );
	MonsterInitDead();
}

class CSittingCleansuitScientist : public CCleansuitScientist
{
public:
	void Spawn();
	void Precache();
	void EXPORT SittingThink();
	int Classify();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];
	void SetAnswerQuestion( CTalkMonster* pSpeaker );
	int FriendNumber( int arrayNumber );
	int FIdleSpeak();

	int		m_baseSequence;
	int		m_headTurn;
	float	m_flResponseDelay;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_sitting_cleansuit_scientist, CSittingCleansuitScientist );

TYPEDESCRIPTION CSittingCleansuitScientist::m_SaveData[] =
{
	DEFINE_FIELD( CSittingCleansuitScientist, m_headTurn, FIELD_INTEGER ),
	DEFINE_FIELD( CSittingCleansuitScientist, m_flResponseDelay, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CSittingCleansuitScientist, CCleansuitScientist );

enum
{
	SITTING_ANIM_sitlookleft,
	SITTING_ANIM_sitlookright,
	SITTING_ANIM_sitscared,
	SITTING_ANIM_sitting2,
	SITTING_ANIM_sitting3
};

void CSittingCleansuitScientist::Spawn()
{
	PRECACHE_MODEL( "models/cleansuit_scientist.mdl" );
	SET_MODEL( ENT( pev ), "models/cleansuit_scientist.mdl" );
	Precache();
	InitBoneControllers();
	UTIL_SetSize( pev, Vector( -14, -14, 0 ), Vector( 14, 14, 36 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = 50;
	m_bloodColor = BLOOD_COLOR_RED;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD;
	pev->spawnflags |= SF_MONSTER_PREDISASTER;
	if ( pev->body == -1 )
		pev->body = RANDOM_LONG( 0, NUM_SCIENTIST_HEADS - 1 );
	if ( pev->body == 2 )
		pev->skin = 1;
	m_baseSequence = LookupSequence( "sitlookleft" );
	pev->sequence = m_baseSequence + RANDOM_LONG( 0, 4 );
	ResetSequenceInfo();
	SetThink( &CSittingCleansuitScientist::SittingThink );
	pev->nextthink = gpGlobals->time + 0.1;
	DROP_TO_FLOOR( ENT( pev ) );
}

void CSittingCleansuitScientist::Precache()
{
	m_baseSequence = LookupSequence( "sitlookleft" );
	TalkInit();
}

int CSittingCleansuitScientist::Classify()
{
	return CLASS_HUMAN_PASSIVE;
}

int CSittingCleansuitScientist::FriendNumber( int arrayNumber )
{
	static int array[3] = { 2, 1, 0 };
	if ( arrayNumber <= 2 )
		return array[arrayNumber];
	return arrayNumber;
}

void CSittingCleansuitScientist::SittingThink()
{
	CBaseEntity* pent;
	StudioFrameAdvance();
	if ( FIdleHello() )
	{
		pent = FindNearestFriend( TRUE );
		if ( pent )
		{
			float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;
			if ( yaw > 180 )
				yaw -= 360;
			if ( yaw < -180 )
				yaw += 360;
			if ( yaw > 0 )
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
			else
				pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;
			ResetSequenceInfo();
			pev->frame = 0;
			SetBoneController( 0, 0 );
		}
	}
	else if ( m_fSequenceFinished )
	{
		int i = RANDOM_LONG( 0, 99 );
		m_headTurn = 0;
		if ( m_flResponseDelay && gpGlobals->time > m_flResponseDelay )
		{
			IdleRespond();
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
			m_flResponseDelay = 0;
		}
		else if ( i < 30 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			if ( !( m_bitsSaid & bit_saidHelloPlayer ) )
				pent = FindNearestFriend( TRUE );
			else
				pent = FindNearestFriend( FALSE );
			if ( !FIdleSpeak() || !pent )
			{
				m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
				pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			}
			else
			{
				float yaw = VecToYaw( pent->pev->origin - pev->origin ) - pev->angles.y;
				if ( yaw > 180 )
					yaw -= 360;
				if ( yaw < -180 )
					yaw += 360;
				if ( yaw > 0 )
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookleft;
				else
					pev->sequence = m_baseSequence + SITTING_ANIM_sitlookright;
			}
		}
		else if ( i < 60 )
		{
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting3;
			m_headTurn = RANDOM_LONG( 0, 8 ) * 10 - 40;
			if ( RANDOM_LONG( 0, 99 ) < 5 )
				FIdleSpeak();
		}
		else if ( i < 80 )
			pev->sequence = m_baseSequence + SITTING_ANIM_sitting2;
		else if ( i < 100 )
			pev->sequence = m_baseSequence + SITTING_ANIM_sitscared;
		ResetSequenceInfo();
		pev->frame = 0;
		SetBoneController( 0, (float)m_headTurn );
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CSittingCleansuitScientist::SetAnswerQuestion( CTalkMonster* pSpeaker )
{
	m_flResponseDelay = gpGlobals->time + RANDOM_FLOAT( 3, 4 );
	m_hTalkTarget = pSpeaker;
}

int CSittingCleansuitScientist::FIdleSpeak()
{
	if ( !FOkToSpeak() )
		return FALSE;
	g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 4.8, 5.2 );
	int pitch = GetVoicePitch();
	CBaseEntity* pentFriend = FindNearestFriend( FALSE );
	if ( pentFriend && RANDOM_LONG( 0, 1 ) )
	{
		CTalkMonster* pTalkMonster = GetClassPtr( (CTalkMonster*)pentFriend->pev );
		pTalkMonster->SetAnswerQuestion( this );
		IdleHeadTurn( pentFriend->pev->origin );
		SENTENCEG_PlayRndSz( ENT( pev ), m_szGrp[TLK_PQUESTION], 1.0, ATTN_IDLE, 0, pitch );
		g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 4.8, 5.2 );
		return TRUE;
	}
	if ( RANDOM_LONG( 0, 1 ) )
	{
		SENTENCEG_PlayRndSz( ENT( pev ), m_szGrp[TLK_PIDLE], 1.0, ATTN_IDLE, 0, pitch );
		g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 4.8, 5.2 );
		return TRUE;
	}
	g_talkWaitTime = 0;
	return FALSE;
}
