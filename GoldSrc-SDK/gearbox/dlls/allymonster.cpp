#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "allymonster.h"
#include "defaultai.h"
#include "scripted.h"
#include "soundent.h"
#include "animation.h"

float COFAllyMonster::g_talkWaitTime = 0;

TYPEDESCRIPTION COFAllyMonster::m_SaveData[] =
{
	DEFINE_FIELD( COFAllyMonster, m_bitsSaid, FIELD_INTEGER ),
	DEFINE_FIELD( COFAllyMonster, m_nSpeak, FIELD_INTEGER ),
	DEFINE_FIELD( COFAllyMonster, m_useTime, FIELD_TIME ),
	DEFINE_FIELD( COFAllyMonster, m_iszUse, FIELD_STRING ),
	DEFINE_FIELD( COFAllyMonster, m_iszUnUse, FIELD_STRING ),
	DEFINE_FIELD( COFAllyMonster, m_flLastSaidSmelled, FIELD_TIME ),
	DEFINE_FIELD( COFAllyMonster, m_flStopTalkTime, FIELD_TIME ),
	DEFINE_FIELD( COFAllyMonster, m_hTalkTarget, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( COFAllyMonster, CBaseMonster );

char* COFAllyMonster::m_szFriends[TLK_CFRIENDS] =
{
	"monster_human_grunt_ally",
	"monster_human_medic_ally",
	"monster_human_torch_ally",
};

Task_t tlOFAllyIdleResponse[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 0.5 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_TLK_RESPOND, 0 },
	{ TASK_TLK_IDEALYAW, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
};

Schedule_t slOFAllyIdleResponse[] =
{
	{ tlOFAllyIdleResponse, ARRAYSIZE( tlOFAllyIdleResponse ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "Idle Response" },
};

Task_t tlOFAllyIdleSpeak[] =
{
	{ TASK_TLK_SPEAK, 0 },
	{ TASK_TLK_IDEALYAW, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT_RANDOM, 0.5 },
};

Schedule_t slOFAllyIdleSpeak[] =
{
	{ tlOFAllyIdleSpeak, ARRAYSIZE( tlOFAllyIdleSpeak ),
		bits_COND_NEW_ENEMY | bits_COND_CLIENT_PUSH | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		0, "Idle Speak" },
};

Task_t tlOFAllyIdleSpeakWait[] =
{
	{ TASK_TLK_SPEAK, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT, 2 },
};

Schedule_t slOFAllyIdleSpeakWait[] =
{
	{ tlOFAllyIdleSpeakWait, ARRAYSIZE( tlOFAllyIdleSpeakWait ),
		bits_COND_NEW_ENEMY | bits_COND_CLIENT_PUSH | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		0, "Idle Speak Wait" },
};

Task_t tlOFAllyIdleHello[] =
{
	{ TASK_TLK_HELLO, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT, 0.5 },
	{ TASK_TLK_HELLO, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT, 0.5 },
	{ TASK_TLK_HELLO, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT, 0.5 },
	{ TASK_TLK_HELLO, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
	{ TASK_WAIT, 0.5 },
};

Schedule_t slOFAllyIdleHello[] =
{
	{ tlOFAllyIdleHello, ARRAYSIZE( tlOFAllyIdleHello ),
		bits_COND_NEW_ENEMY | bits_COND_CLIENT_PUSH | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_PROVOKED, bits_SOUND_COMBAT, "Idle Hello" },
};

Task_t tlOFAllyIdleStopShooting[] =
{
	{ TASK_TLK_STOPSHOOTING, 0 },
};

Schedule_t slOFAllyIdleStopShooting[] =
{
	{ tlOFAllyIdleStopShooting, ARRAYSIZE( tlOFAllyIdleStopShooting ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		0, "Idle Stop Shooting" },
};

Task_t tlOFAllyMoveAway[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MOVE_AWAY_FAIL },
	{ TASK_STORE_LASTPOSITION, 0 },
	{ TASK_MOVE_AWAY_PATH, 100 },
	{ TASK_WALK_PATH_FOR_UNITS, 100 },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_PLAYER, 0.5 },
};

Schedule_t slOFAllyMoveAway[] =
{
	{ tlOFAllyMoveAway, ARRAYSIZE( tlOFAllyMoveAway ), 0, 0, "MoveAway" },
};

Task_t tlOFAllyMoveAwayFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_PLAYER, 0.5 },
};

Schedule_t slOFAllyMoveAwayFail[] =
{
	{ tlOFAllyMoveAwayFail, ARRAYSIZE( tlOFAllyMoveAwayFail ), 0, 0, "MoveAwayFail" },
};

Task_t tlOFAllyMoveAwayFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TARGET_FACE },
	{ TASK_STORE_LASTPOSITION, 0 },
	{ TASK_MOVE_AWAY_PATH, 100 },
	{ TASK_WALK_PATH_FOR_UNITS, 100 },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE },
};

Schedule_t slOFAllyMoveAwayFollow[] =
{
	{ tlOFAllyMoveAwayFollow, ARRAYSIZE( tlOFAllyMoveAwayFollow ), 0, 0, "MoveAwayFollow" },
};

Task_t tlOFAllyTlkIdleWatchClient[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_LOOK_AT_CLIENT, 6 },
};

Task_t tlOFAllyTlkIdleWatchClientStare[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_TLK_CLIENT_STARE, 6 },
	{ TASK_TLK_STARE, 0 },
	{ TASK_TLK_IDEALYAW, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
};

Schedule_t slOFAllyTlkIdleWatchClient[] =
{
	{ tlOFAllyTlkIdleWatchClient, ARRAYSIZE( tlOFAllyTlkIdleWatchClient ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_CLIENT_PUSH | bits_COND_CLIENT_UNSEEN | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"TlkIdleWatchClient" },
	{ tlOFAllyTlkIdleWatchClientStare, ARRAYSIZE( tlOFAllyTlkIdleWatchClientStare ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_CLIENT_PUSH | bits_COND_CLIENT_UNSEEN | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"TlkIdleWatchClientStare" },
};

Task_t tlOFAllyTlkIdleEyecontact[] =
{
	{ TASK_TLK_IDEALYAW, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_TLK_EYECONTACT, 0 },
};

Schedule_t slOFAllyTlkIdleEyecontact[] =
{
	{ tlOFAllyTlkIdleEyecontact, ARRAYSIZE( tlOFAllyTlkIdleEyecontact ),
		bits_COND_NEW_ENEMY | bits_COND_CLIENT_PUSH | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE,
		0, "TlkIdleEyecontact" },
};

DEFINE_CUSTOM_SCHEDULES( COFAllyMonster )
{
	slOFAllyIdleResponse,
	slOFAllyIdleSpeak,
	slOFAllyIdleHello,
	slOFAllyIdleSpeakWait,
	slOFAllyIdleStopShooting,
	slOFAllyMoveAway,
	slOFAllyMoveAwayFollow,
	slOFAllyMoveAwayFail,
	slOFAllyTlkIdleWatchClient,
	&slOFAllyTlkIdleWatchClient[1],
	slOFAllyTlkIdleEyecontact,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFAllyMonster, CBaseMonster );

void COFAllyMonster::StartTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TLK_SPEAK:
		FIdleSpeak();
		TaskComplete();
		break;
	case TASK_TLK_RESPOND:
		IdleRespond();
		TaskComplete();
		break;
	case TASK_TLK_HELLO:
		FIdleHello();
		TaskComplete();
		break;
	case TASK_TLK_STARE:
		FIdleStare();
		TaskComplete();
		break;
	case TASK_FACE_PLAYER:
	case TASK_TLK_LOOK_AT_CLIENT:
	case TASK_TLK_CLIENT_STARE:
		m_flWaitFinished = gpGlobals->time + pTask->flData;
		break;
	case TASK_TLK_EYECONTACT:
		break;
	case TASK_TLK_IDEALYAW:
		if ( m_hTalkTarget != NULL )
		{
			pev->yaw_speed = 60;
			float yaw = VecToYaw( m_hTalkTarget->pev->origin - pev->origin ) - pev->angles.y;
			if ( yaw > 180 )
				yaw -= 360;
			if ( yaw < -180 )
				yaw += 360;
			if ( yaw < 0 )
				pev->ideal_yaw = min( yaw + 45.0f, 0.0f ) + pev->angles.y;
			else
				pev->ideal_yaw = max( yaw - 45.0f, 0.0f ) + pev->angles.y;
		}
		TaskComplete();
		break;
	case TASK_TLK_HEADRESET:
		m_hTalkTarget = NULL;
		TaskComplete();
		break;
	case TASK_TLK_STOPSHOOTING:
		PlaySentence( m_szGrp[TLK_NOSHOOT], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
		TaskComplete();
		break;
	case TASK_CANT_FOLLOW:
		StopFollowing( FALSE );
		PlaySentence( m_szGrp[TLK_STOP], RANDOM_FLOAT( 2, 2.5 ), VOL_NORM, ATTN_NORM );
		TaskComplete();
		break;
	case TASK_WALK_PATH_FOR_UNITS:
		m_movementActivity = ACT_WALK;
		break;
	case TASK_MOVE_AWAY_PATH:
		{
			Vector angles = pev->angles;
			angles.y = pev->ideal_yaw + 180;
			Vector move;
			UTIL_MakeVectorsPrivate( angles, move, NULL, NULL );
			Vector destination = pev->origin + move * pTask->flData;
			if ( MoveToLocation( ACT_WALK, 2, destination ) )
			{
				TaskComplete();
			}
			else if ( FindCover( pev->origin, pev->view_ofs, 0, CoverRadius() ) )
			{
				m_flMoveWaitFinished = gpGlobals->time + 2;
				TaskComplete();
			}
			else
			{
				TaskFail();
			}
		}
		break;
	case TASK_PLAY_SCRIPT:
		m_hTalkTarget = NULL;
		CBaseMonster::StartTask( pTask );
		break;
	default:
		CBaseMonster::StartTask( pTask );
	}
}

void COFAllyMonster::RunTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TLK_CLIENT_STARE:
	case TASK_TLK_LOOK_AT_CLIENT:
		{
			if ( m_MonsterState != MONSTERSTATE_IDLE || IsMoving() || IsTalking() )
			{
				TaskFail();
				return;
			}
			edict_t* pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
			if ( !pPlayer )
			{
				TaskFail();
				return;
			}
			IdleHeadTurn( pPlayer->v.origin );
			if ( pTask->iTask == TASK_TLK_CLIENT_STARE )
			{
				if ( ( pPlayer->v.origin - pev->origin ).Length2D() > TLK_STARE_DIST )
					TaskFail();
				UTIL_MakeVectors( pPlayer->v.angles );
				if ( UTIL_DotPoints( pPlayer->v.origin, pev->origin, gpGlobals->v_forward ) < m_flFieldOfView )
					TaskFail();
			}
			if ( gpGlobals->time > m_flWaitFinished )
				TaskComplete();
		}
		break;
	case TASK_FACE_PLAYER:
		{
			edict_t* pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
			if ( pPlayer )
			{
				MakeIdealYaw( pPlayer->v.origin );
				ChangeYaw( pev->yaw_speed );
				IdleHeadTurn( pPlayer->v.origin );
				if ( gpGlobals->time > m_flWaitFinished && FlYawDiff() < 10 )
					TaskComplete();
			}
			else
			{
				TaskFail();
			}
		}
		break;
	case TASK_TLK_EYECONTACT:
		if ( !IsMoving() && IsTalking() && m_hTalkTarget != NULL )
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		else
			TaskComplete();
		break;
	case TASK_WALK_PATH_FOR_UNITS:
		if ( ( m_vecLastPosition - pev->origin ).Length2D() > pTask->flData || MovementIsComplete() )
		{
			TaskComplete();
			RouteClear();
		}
		break;
	case TASK_WAIT_FOR_MOVEMENT:
		if ( IsTalking() && m_hTalkTarget != NULL )
		{
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		}
		else
		{
			IdleHeadTurn( pev->origin );
			FIdleHello();
			if ( RANDOM_LONG( 0, m_nSpeak * 20 ) == 0 )
				FIdleSpeak();
		}
		CBaseMonster::RunTask( pTask );
		if ( TaskIsComplete() )
			IdleHeadTurn( pev->origin );
		break;
	default:
		if ( IsTalking() && m_hTalkTarget != NULL )
			IdleHeadTurn( m_hTalkTarget->pev->origin );
		else
			SetBoneController( 0, 0 );
		CBaseMonster::RunTask( pTask );
	}
}

CBaseEntity* COFAllyMonster::EnumFriends( CBaseEntity* pPrevious, int listNumber, BOOL bTrace )
{
	const char* name = m_szFriends[FriendNumber( listNumber )];
	while ( ( pPrevious = UTIL_FindEntityByClassname( pPrevious, name ) ) != NULL )
	{
		if ( pPrevious == this || !pPrevious->IsAlive() )
			continue;
		if ( bTrace )
		{
			Vector vecCheck = pPrevious->pev->origin;
			vecCheck.z = pPrevious->pev->absmax.z;
			TraceResult tr;
			UTIL_TraceLine( pev->origin, vecCheck, ignore_monsters, edict(), &tr );
			if ( tr.flFraction != 1.0 )
				continue;
		}
		break;
	}
	return pPrevious;
}

void COFAllyMonster::AlertFriends()
{
	for ( int i = 0; i < TLK_CFRIENDS; i++ )
	{
		CBaseEntity* pFriend = NULL;
		while ( ( pFriend = EnumFriends( pFriend, i, TRUE ) ) != NULL )
		{
			CBaseMonster* pMonster = pFriend->MyMonsterPointer();
			if ( pMonster->IsAlive() )
				pMonster->Remember( bits_MEMORY_PROVOKED );
		}
	}
}

void COFAllyMonster::ShutUpFriends()
{
	for ( int i = 0; i < TLK_CFRIENDS; i++ )
	{
		CBaseEntity* pFriend = NULL;
		while ( ( pFriend = EnumFriends( pFriend, i, TRUE ) ) != NULL )
		{
			CBaseMonster* pMonster = pFriend->MyMonsterPointer();
			if ( pMonster )
				pMonster->SentenceStop();
		}
	}
}

void COFAllyMonster::LimitFollowers( CBaseEntity* pPlayer, int maxFollowers )
{
	int count = 0;
	for ( int i = 0; i < TLK_CFRIENDS; i++ )
	{
		CBaseEntity* pFriend = NULL;
		while ( ( pFriend = EnumFriends( pFriend, i, FALSE ) ) != NULL )
		{
			CBaseMonster* pMonster = pFriend->MyMonsterPointer();
			if ( pMonster && pMonster->m_hTargetEnt == pPlayer && ++count > maxFollowers )
				pMonster->StopFollowing( TRUE );
		}
	}
}

void COFAllyMonster::Killed( entvars_t* pevAttacker, int iGib )
{
	if ( m_MonsterState != MONSTERSTATE_DEAD && m_MonsterState != MONSTERSTATE_PRONE &&
		( pevAttacker->flags & FL_CLIENT ) )
	{
		AlertFriends();
		LimitFollowers( CBaseEntity::Instance( pevAttacker ), 0 );
	}
	m_hTargetEnt = NULL;
	SentenceStop();
	SetUse( NULL );
	CBaseMonster::Killed( pevAttacker, iGib );
}

float COFAllyMonster::TargetDistance()
{
	if ( m_hTargetEnt != NULL && m_hTargetEnt->IsAlive() )
		return ( m_hTargetEnt->pev->origin - pev->origin ).Length();
	return 1000000;
}

void COFAllyMonster::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	if ( pEvent->event == SCRIPT_EVENT_SENTENCE_RND1 )
	{
		if ( RANDOM_LONG( 0, 99 ) > 74 )
		{
			ShutUpFriends();
			PlaySentence( pEvent->options, RANDOM_FLOAT( 2.8, 3.4 ), VOL_NORM, ATTN_NORM );
		}
	}
	else if ( pEvent->event == SCRIPT_EVENT_SENTENCE )
	{
		ShutUpFriends();
		PlaySentence( pEvent->options, RANDOM_FLOAT( 2.8, 3.4 ), VOL_NORM, ATTN_NORM );
	}
	else
	{
		CBaseMonster::HandleAnimEvent( pEvent );
	}
}

void COFAllyMonster::TalkInit()
{
	g_talkWaitTime = 0;
	m_voicePitch = 100;
}

int COFAllyMonster::GetVoicePitch()
{
	return m_voicePitch + RANDOM_LONG( 0, 3 );
}

void COFAllyMonster::Touch( CBaseEntity* pOther )
{
	if ( !pOther->IsPlayer() || ( m_afMemory & bits_MEMORY_PROVOKED ) || IsTalking() )
		return;
	float speed = fabs( pOther->pev->velocity.x ) + fabs( pOther->pev->velocity.y );
	if ( speed > 50 )
	{
		SetConditions( bits_COND_CLIENT_PUSH );
		MakeIdealYaw( pOther->pev->origin );
	}
}

void COFAllyMonster::IdleRespond()
{
	GetVoicePitch();
	PlaySentence( m_szGrp[TLK_ANSWER], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
}

int COFAllyMonster::CanPlaySentence( BOOL fDisregardState )
{
	if ( fDisregardState )
		return IsAlive();
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ||
		pev->deadflag != DEAD_NO || gpGlobals->time <= g_talkWaitTime || ( pev->spawnflags & SF_MONSTER_GAG ) )
		return FALSE;
	return FOkToSpeak();
}

void COFAllyMonster::PlayScriptedSentence( const char* pszSentence, float duration, float volume,
	float attenuation, BOOL bConcurrent, CBaseEntity* pListener )
{
	if ( !bConcurrent )
		ShutUpFriends();
	ClearConditions( bits_COND_CLIENT_PUSH );
	m_useTime = gpGlobals->time + duration;
	PlaySentence( pszSentence, duration, volume, attenuation );
	m_hTalkTarget = pListener;
}

void COFAllyMonster::PlaySentence( const char* pszSentence, float duration, float volume, float attenuation )
{
	if ( !pszSentence )
		return;
	Talk( duration );
	g_talkWaitTime = gpGlobals->time + duration + 2;
	if ( pszSentence[0] == '!' )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, pszSentence, volume, attenuation, 0, GetVoicePitch() );
	else
		SENTENCEG_PlayRndSz( edict(), pszSentence, volume, attenuation, 0, GetVoicePitch() );
	m_bitsSaid |= bit_saidHelloPlayer;
}

void COFAllyMonster::Talk( float flDuration )
{
	m_flStopTalkTime = gpGlobals->time + ( flDuration <= 0 ? 3 : flDuration );
}

void COFAllyMonster::SetAnswerQuestion( COFAllyMonster* pSpeaker )
{
	if ( !m_pCine )
		ChangeSchedule( slOFAllyIdleResponse );
	m_hTalkTarget = pSpeaker;
}

int COFAllyMonster::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	if ( m_MonsterState != MONSTERSTATE_SCRIPT && pev->deadflag == DEAD_NO && pevAttacker &&
		m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		CBaseEntity* pFriend = FindNearestFriend( FALSE );
		if ( pFriend && pFriend->IsAlive() )
			( (COFAllyMonster*)pFriend )->ChangeSchedule( slOFAllyIdleStopShooting );
	}
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

BOOL COFAllyMonster::IsTalking()
{
	return m_flStopTalkTime > gpGlobals->time;
}

void COFAllyMonster::PrescheduleThink()
{
	if ( !HasConditions( bits_COND_SEE_CLIENT ) )
		SetConditions( bits_COND_CLIENT_UNSEEN );
}

int COFAllyMonster::IRelationship( CBaseEntity* pTarget )
{
	if ( pTarget->IsPlayer() && ( m_afMemory & bits_MEMORY_PROVOKED ) )
		return R_HT;
	return CBaseMonster::IRelationship( pTarget );
}

void COFAllyMonster::StopFollowing( BOOL clearSchedule )
{
	if ( !IsFollowing() )
		return;
	if ( !( m_afMemory & bits_MEMORY_PROVOKED ) )
	{
		PlaySentence( m_szGrp[TLK_UNUSE], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
		m_hTalkTarget = m_hTargetEnt;
	}
	if ( m_movementGoal == MOVEGOAL_TARGETENT )
		RouteClear();
	m_hTargetEnt = NULL;
	if ( clearSchedule )
		ClearSchedule();
	if ( m_hEnemy != NULL )
		m_IdealMonsterState = MONSTERSTATE_COMBAT;
}

void COFAllyMonster::StartFollowing( CBaseEntity* pLeader )
{
	if ( m_pCine )
		m_pCine->CancelScript();
	if ( m_hEnemy != NULL )
		m_IdealMonsterState = MONSTERSTATE_ALERT;
	m_hTargetEnt = pLeader;
	PlaySentence( m_szGrp[TLK_USE], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
	m_hTalkTarget = m_hTargetEnt;
	ClearConditions( bits_COND_CLIENT_PUSH );
	ClearSchedule();
}

BOOL COFAllyMonster::CanFollow()
{
	if ( m_MonsterState == MONSTERSTATE_SCRIPT && !m_pCine->CanInterrupt() )
		return FALSE;
	if ( !IsAlive() )
		return FALSE;
	return !IsFollowing();
}

void COFAllyMonster::FollowerUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( gpGlobals->time < m_useTime || !pCaller || !pCaller->IsPlayer() )
		return;
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
	{
		DeclineFollowing();
		return;
	}
	if ( !CanFollow() )
	{
		StopFollowing( TRUE );
		return;
	}
	LimitFollowers( pCaller, 1 );
	if ( m_afMemory & bits_MEMORY_PROVOKED )
	{
		ALERT( at_console, "I'm not following you, you evil person!\n" );
		return;
	}
	StartFollowing( pCaller );
	m_bitsSaid |= bit_saidHelloPlayer;
}

void COFAllyMonster::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "UseSentence" ) )
	{
		m_iszUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "UnUseSentence" ) )
	{
		m_iszUnUse = ALLOC_STRING( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

void COFAllyMonster::Precache()
{
	if ( m_iszUse )
		m_szGrp[TLK_USE] = STRING( m_iszUse );
	if ( m_iszUnUse )
		m_szGrp[TLK_UNUSE] = STRING( m_iszUnUse );
}

CBaseEntity* COFAllyMonster::FindNearestFriend( BOOL fPlayer )
{
	CBaseEntity* pNearest = NULL;
	float range = 10000000;
	Vector vecStart = pev->origin;
	vecStart.z = pev->absmax.z;
	int count = fPlayer ? 1 : TLK_CFRIENDS;
	for ( int i = count - 1; i >= 0; i-- )
	{
		const char* name = fPlayer ? "player" : m_szFriends[FriendNumber( i )];
		if ( !name )
			continue;
		CBaseEntity* pFriend = NULL;
		while ( ( pFriend = UTIL_FindEntityByClassname( pFriend, name ) ) != NULL )
		{
			if ( pFriend == this || !pFriend->IsAlive() )
				continue;
			CBaseMonster* pMonster = pFriend->MyMonsterPointer();
			if ( !pMonster || pMonster->m_MonsterState == MONSTERSTATE_SCRIPT ||
				pMonster->m_MonsterState == MONSTERSTATE_PRONE )
				continue;
			Vector vecCheck = pFriend->pev->origin;
			vecCheck.z = pFriend->pev->absmax.z;
			if ( range > ( vecStart - vecCheck ).Length() )
			{
				TraceResult tr;
				UTIL_TraceLine( vecStart, vecCheck, ignore_monsters, edict(), &tr );
				if ( tr.flFraction == 1.0 && ( vecStart - vecCheck ).Length() < TALKRANGE_MIN )
				{
					pNearest = pFriend;
					range = ( vecStart - vecCheck ).Length();
				}
			}
		}
	}
	return pNearest;
}

int COFAllyMonster::FOkToSpeak()
{
	if ( m_MonsterState == MONSTERSTATE_PRONE || m_IdealMonsterState == MONSTERSTATE_PRONE ||
		pev->deadflag != DEAD_NO || gpGlobals->time <= g_talkWaitTime || ( pev->spawnflags & SF_MONSTER_GAG ) )
		return FALSE;
	if ( !IsAlive() || FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) )
		return FALSE;
	if ( m_hEnemy != NULL && FVisible( m_hEnemy ) )
		return FALSE;
	return TRUE;
}

int COFAllyMonster::FIdleStare()
{
	if ( !FOkToSpeak() )
		return FALSE;
	PlaySentence( m_szGrp[TLK_STARE], RANDOM_FLOAT( 5, 7.5 ), VOL_NORM, ATTN_IDLE );
	m_hTalkTarget = FindNearestFriend( TRUE );
	return TRUE;
}

int COFAllyMonster::FIdleHello()
{
	if ( !FOkToSpeak() || ( m_bitsSaid & bit_saidHelloPlayer ) )
		return FALSE;
	CBaseEntity* pPlayer = FindNearestFriend( TRUE );
	if ( !pPlayer || !FInViewCone( pPlayer ) || !FVisible( pPlayer ) )
		return FALSE;
	m_hTalkTarget = pPlayer;
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
		PlaySentence( m_szGrp[TLK_PHELLO], RANDOM_FLOAT( 3, 3.5 ), VOL_NORM, ATTN_IDLE );
	else
		PlaySentence( m_szGrp[TLK_HELLO], RANDOM_FLOAT( 3, 3.5 ), VOL_NORM, ATTN_IDLE );
	m_bitsSaid |= bit_saidHelloPlayer;
	return TRUE;
}

void COFAllyMonster::IdleHeadTurn( Vector& vecFriend )
{
	if ( !( m_afCapability & bits_CAP_TURN_HEAD ) )
		return;
	float yaw = VecToYaw( vecFriend - pev->origin ) - pev->angles.y;
	if ( yaw > 180 )
		yaw -= 360;
	if ( yaw < -180 )
		yaw += 360;
	SetBoneController( 0, yaw );
}

int COFAllyMonster::FIdleSpeak()
{
	if ( !FOkToSpeak() )
		return FALSE;
	const char* idleGroup;
	const char* questionGroup;
	float duration;
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
	{
		idleGroup = m_szGrp[TLK_PIDLE];
		questionGroup = m_szGrp[TLK_PQUESTION];
		duration = RANDOM_FLOAT( 4.8, 5.2 );
	}
	else
	{
		idleGroup = m_szGrp[TLK_IDLE];
		questionGroup = m_szGrp[TLK_QUESTION];
		duration = RANDOM_FLOAT( 2.8, 3.2 );
	}
	GetVoicePitch();
	CBaseEntity* pTarget = m_hTargetEnt;
	if ( pTarget && pTarget->IsPlayer() && pTarget->IsAlive() )
	{
		m_hTalkTarget = m_hTargetEnt;
		if ( !( m_bitsSaid & bit_saidDamageHeavy ) && pTarget->pev->health <= pTarget->pev->max_health / 8 )
		{
			PlaySentence( m_szGrp[TLK_PLHURT3], duration, VOL_NORM, ATTN_IDLE );
			m_bitsSaid |= bit_saidDamageHeavy;
			return TRUE;
		}
		if ( !( m_bitsSaid & bit_saidDamageMedium ) && pTarget->pev->health <= pTarget->pev->max_health / 4 )
		{
			PlaySentence( m_szGrp[TLK_PLHURT2], duration, VOL_NORM, ATTN_IDLE );
			m_bitsSaid |= bit_saidDamageMedium;
			return TRUE;
		}
		if ( !( m_bitsSaid & bit_saidDamageLight ) && pTarget->pev->health <= pTarget->pev->max_health / 2 )
		{
			PlaySentence( m_szGrp[TLK_PLHURT1], duration, VOL_NORM, ATTN_IDLE );
			m_bitsSaid |= bit_saidDamageLight;
			return TRUE;
		}
	}
	CBaseEntity* pFriend = FindNearestFriend( FALSE );
	if ( pFriend && !pFriend->IsMoving() && RANDOM_LONG( 0, 99 ) < 75 )
	{
		PlaySentence( questionGroup, duration, VOL_NORM, ATTN_IDLE );
		COFAllyMonster* pTalkMonster = (COFAllyMonster*)pFriend;
		m_hTalkTarget = pFriend;
		pTalkMonster->SetAnswerQuestion( this );
		pTalkMonster->m_flStopTalkTime = m_flStopTalkTime;
		m_nSpeak++;
		return TRUE;
	}
	if ( RANDOM_LONG( 0, 1 ) )
	{
		pFriend = FindNearestFriend( TRUE );
		if ( pFriend )
		{
			m_hTalkTarget = pFriend;
			PlaySentence( idleGroup, duration, VOL_NORM, ATTN_IDLE );
			m_nSpeak++;
			return TRUE;
		}
	}
	Talk( 0 );
	g_talkWaitTime = 0;
	return FALSE;
}

void COFAllyMonster::TrySmellTalk()
{
	if ( !FOkToSpeak() )
		return;
	if ( gpGlobals->time > m_flLastSaidSmelled )
		m_bitsSaid &= ~bit_saidSmelled;
	if ( !( m_bitsSaid & bit_saidSmelled ) && HasConditions( bits_COND_SMELL ) )
	{
		PlaySentence( m_szGrp[TLK_SMELL], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
		m_bitsSaid |= bit_saidSmelled;
		m_flLastSaidSmelled = gpGlobals->time + 60;
	}
}

Schedule_t* COFAllyMonster::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_MOVE_AWAY:
		return slOFAllyMoveAway;
	case SCHED_MOVE_AWAY_FOLLOW:
		return slOFAllyMoveAwayFollow;
	case SCHED_MOVE_AWAY_FAIL:
		return slOFAllyMoveAwayFail;
	case SCHED_TARGET_FACE:
		return RANDOM_LONG( 0, 99 ) < 2 ? slOFAllyIdleSpeakWait : slIdleStand;
	case SCHED_IDLE_STAND:
		if ( !( m_bitsSaid & bit_saidHelloPlayer ) )
			return slOFAllyIdleHello;
		if ( !( m_bitsSaid & bit_saidWoundLight ) && pev->health <= pev->max_health * 0.75 )
		{
			PlaySentence( m_szGrp[TLK_WOUND], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
			m_bitsSaid |= bit_saidWoundLight;
			return slIdleStand;
		}
		if ( !( m_bitsSaid & bit_saidWoundHeavy ) && pev->health <= pev->max_health * 0.5 )
		{
			PlaySentence( m_szGrp[TLK_MORTAL], RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_NORM );
			m_bitsSaid |= bit_saidWoundHeavy;
			return slIdleStand;
		}
		if ( FOkToSpeak() && RANDOM_LONG( 0, m_nSpeak * 2 ) == 0 )
			return slOFAllyIdleSpeak;
		if ( !IsTalking() && HasConditions( bits_COND_SEE_CLIENT ) && RANDOM_LONG( 0, 6 ) == 0 )
		{
			edict_t* pPlayer = g_engfuncs.pfnPEntityOfEntIndex( 1 );
			if ( pPlayer )
			{
				UTIL_MakeVectors( pPlayer->v.angles );
				if ( ( pPlayer->v.origin - pev->origin ).Length2D() < TLK_STARE_DIST &&
					UTIL_DotPoints( pPlayer->v.origin, pev->origin, gpGlobals->v_forward ) >= m_flFieldOfView )
					return &slOFAllyTlkIdleWatchClient[1];
				return slOFAllyTlkIdleWatchClient;
			}
		}
		else
		{
			return IsTalking() ? slOFAllyTlkIdleEyecontact : slIdleStand;
		}
		break;
	}
	return CBaseMonster::GetScheduleOfType( Type );
}
