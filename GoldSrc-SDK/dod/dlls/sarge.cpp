#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "weapons.h"
#include "soundent.h"

class CSarge : public CTalkMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int ISoundMask() override;
	int Classify() override;
	int ObjectCaps() override;
	void AlertSound() override;
	void PainSound() override;
	void DeathSound() override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	void StartTask( Task_t* pTask ) override;
	void RunTask( Task_t* pTask ) override;
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType( int Type ) override;
	MONSTERSTATE GetIdealState() override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType ) override;
	void Killed( entvars_t* pevAttacker, int iGib ) override;
	void DeclineFollowing() override;
	int IRelationship( CBaseEntity* pTarget ) override;
	CBaseEntity* BestVisibleEnemy() override;
	void Touch( CBaseEntity* pOther ) override;
	void RunAI() override;
	BOOL GetEnemy();
	void BarneyFirePistol();
	void TalkInit();
	void GiveAdvice( CBaseEntity* pTarget );
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	CUSTOM_SCHEDULES;

	BOOL m_fGunDrawn;
	float m_painTime;
	float m_checkAttackTime;
	BOOL m_lastAttackCheck;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_sarge_allies, CSarge );

TYPEDESCRIPTION CSarge::m_SaveData[] =
{
	DEFINE_FIELD( CSarge, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CSarge, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CSarge, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CSarge, m_lastAttackCheck, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CSarge, CTalkMonster );

Task_t tlBaFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 128 },
	{ TASK_SET_SCHEDULE, SCHED_TARGET_FACE },
};

Schedule_t slBaFollow[] =
{
	{
		tlBaFollow,
		ARRAYSIZE( tlBaFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"Follow"
	},
};

Task_t tlBarneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_ARM },
};

Schedule_t slBarneyEnemyDraw[] =
{
	{
		tlBarneyEnemyDraw,
		ARRAYSIZE( tlBarneyEnemyDraw ),
		0,
		0,
		"Barney Enemy Draw"
	},
};

Task_t tlBaFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_SET_SCHEDULE, SCHED_TARGET_CHASE },
};

Schedule_t slBaFaceTarget[] =
{
	{
		tlBaFaceTarget,
		ARRAYSIZE( tlBaFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_PROVOKED,
		bits_SOUND_DANGER,
		"FaceTarget"
	},
};

Task_t tlIdleBaStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 },
};

Schedule_t slIdleBaStand[] =
{
	{
		tlIdleBaStand,
		ARRAYSIZE( tlIdleBaStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_SMELL | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT |
		bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleStand"
	},
};

DEFINE_CUSTOM_SCHEDULES( CSarge )
{
	slBaFollow,
	slBarneyEnemyDraw,
	slBaFaceTarget,
	slIdleBaStand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CSarge, CTalkMonster );

int CSarge::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_CARCASS | bits_SOUND_MEAT | bits_SOUND_GARBAGE | bits_SOUND_DANGER | bits_SOUND_PLAYER;
}

int CSarge::Classify()
{
	return CLASS_PLAYER_ALLY;
}

int CSarge::ObjectCaps()
{
	return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE;
}

void CSarge::SetYawSpeed()
{
	pev->yaw_speed = m_Activity == ACT_RUN ? 90 : 70;
}

void CSarge::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/barney.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = 1000;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = VIEW_FIELD_WIDE;
	pev->team = 1;
	pev->body = 0;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fGunDrawn = FALSE;
	MonsterInit();
	SetUse( NULL );
}

void CSarge::Precache()
{
	PRECACHE_MODEL( "models/barney.mdl" );
	PRECACHE_SOUND( "barney/ba_attack1.wav" );
	PRECACHE_SOUND( "barney/ba_attack2.wav" );
	PRECACHE_SOUND( "barney/ba_pain1.wav" );
	PRECACHE_SOUND( "barney/ba_pain2.wav" );
	PRECACHE_SOUND( "barney/ba_pain3.wav" );
	PRECACHE_SOUND( "barney/ba_die1.wav" );
	PRECACHE_SOUND( "barney/ba_die2.wav" );
	PRECACHE_SOUND( "barney/ba_die3.wav" );
	TalkInit();
	CTalkMonster::Precache();
}

void CSarge::TalkInit()
{
	CTalkMonster::TalkInit();
	m_szGrp[TLK_ANSWER] = "BA_ANSWER";
	m_szGrp[TLK_QUESTION] = "BA_QUESTION";
	m_szGrp[TLK_IDLE] = "BA_IDLE";
	m_szGrp[TLK_STARE] = "BA_STARE";
	m_szGrp[TLK_USE] = "BA_OK";
	m_szGrp[TLK_UNUSE] = "BA_WAIT";
	m_szGrp[TLK_STOP] = "BA_STOP";
	m_szGrp[TLK_NOSHOOT] = "BA_SCARED";
	m_szGrp[TLK_HELLO] = "BA_HELLO";
	m_szGrp[TLK_PLHURT1] = "!BA_CUREA";
	m_szGrp[TLK_PLHURT2] = "!BA_CUREB";
	m_szGrp[TLK_PLHURT3] = "!BA_CUREC";
	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;
	m_szGrp[TLK_PQUESTION] = "BA_PQUEST";
	m_szGrp[TLK_SMELL] = "BA_SMELL";
	m_szGrp[TLK_WOUND] = "BA_WOUND";
	m_szGrp[TLK_MORTAL] = "BA_MORTAL";
	m_voicePitch = 99;
}

void CSarge::StartTask( Task_t* pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CSarge::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_RANGE_ATTACK1 && m_hEnemy != NULL && m_hEnemy->IsPlayer() )
		pev->framerate = 1.5;
	CTalkMonster::RunTask( pTask );
}

BOOL CSarge::CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist > 1024 || flDot < 0.5 )
		return FALSE;
	if ( gpGlobals->time > m_checkAttackTime )
	{
		TraceResult tr;
		Vector vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
		CBaseEntity* pEnemy = m_hEnemy;
		Vector vecShootTarget = pEnemy->BodyTarget( vecShootOrigin ) - pEnemy->pev->origin + m_vecEnemyLKP;
		UTIL_TraceLine( vecShootOrigin, vecShootTarget, dont_ignore_monsters, edict(), &tr );
		m_checkAttackTime = gpGlobals->time + 1;
		m_lastAttackCheck = tr.flFraction == 1 || (tr.pHit && CBaseEntity::Instance( tr.pHit ) == pEnemy);
		m_checkAttackTime = gpGlobals->time + 1.5;
	}
	return m_lastAttackCheck;
}

void CSarge::BarneyFirePistol()
{
	UTIL_MakeVectors( pev->angles );
	Vector vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_1DEGREES, 1024, BULLET_MONSTER_9MM );
	int pitchShift = RANDOM_LONG( 0, 20 );
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	--m_cAmmoLoaded;
}

void CSarge::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 2:
		pev->body = 1;
		m_fGunDrawn = TRUE;
		break;
	case 3:
		BarneyFirePistol();
		break;
	case 4:
		pev->body = 0;
		m_fGunDrawn = FALSE;
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

int CSarge::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	int result = CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( IsAlive() && pev->deadflag != DEAD_DYING )
		Remember( bits_MEMORY_PROVOKED );
	return result;
}

void CSarge::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType )
{
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_BLAST) )
			flDamage /= 2;
		break;
	case 10:
		if ( bitsDamageType & (DMG_BULLET | DMG_SLASH | DMG_CLUB) )
		{
			flDamage -= 20;
			if ( flDamage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1 );
				flDamage = 0.01;
			}
		}
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CSarge::Killed( entvars_t* pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

void CSarge::PainSound()
{
	if ( gpGlobals->time < m_painTime )
		return;
	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 1: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 2: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	}
}

void CSarge::DeathSound()
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 1: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	case 2: EMIT_SOUND_DYN( edict(), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() ); break;
	}
}

void CSarge::AlertSound()
{
	if ( m_hEnemy != NULL && FOkToSpeak() )
		PlaySentence( "BA_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
}

void CSarge::DeclineFollowing()
{
	PlaySentence( "BA_POK", 2, VOL_NORM, ATTN_NORM );
}

MONSTERSTATE CSarge::GetIdealState()
{
	return CBaseMonster::GetIdealState();
}

Schedule_t* CSarge::GetScheduleOfType( int Type )
{
	Schedule_t* pSchedule;
	switch ( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
			return slBarneyEnemyDraw;
		break;
	case SCHED_TARGET_FACE:
		pSchedule = CTalkMonster::GetScheduleOfType( Type );
		return pSchedule == slIdleStand ? slBaFaceTarget : pSchedule;
	case SCHED_TARGET_CHASE:
		return slBaFollow;
	case SCHED_IDLE_STAND:
		pSchedule = CTalkMonster::GetScheduleOfType( Type );
		return pSchedule == slIdleStand ? slIdleBaStand : pSchedule;
	}
	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t* CSarge::GetSchedule()
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && (pSound->m_iType & bits_SOUND_DANGER) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
		PlaySentence( "BA_KILL", 4, VOL_NORM, ATTN_NORM );
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			return CBaseMonster::GetSchedule();
		if ( HasConditions( bits_COND_NEW_ENEMY ) && HasConditions( bits_COND_LIGHT_DAMAGE ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if ( !m_fGunDrawn )
			return GetScheduleOfType( SCHED_ARM_WEAPON );
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		break;
	case MONSTERSTATE_ALERT:
	case MONSTERSTATE_IDLE:
		if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( !m_hTargetEnt->IsAlive() )
			{
				StopFollowing( FALSE );
				break;
			}
			return GetScheduleOfType( HasConditions( bits_COND_CLIENT_PUSH ) ? SCHED_MOVE_AWAY_FOLLOW : SCHED_TARGET_FACE );
		}
		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		TrySmellTalk();
		break;
	}
	return CBaseMonster::GetSchedule();
}

int CSarge::IRelationship( CBaseEntity* pTarget )
{
	if ( pTarget->pev->iuser1 || pTarget->pev->deadflag == DEAD_DEAD )
		return R_NO;
	return pTarget->pev->team == pev->team ? R_AL : R_NM;
}

CBaseEntity* CSarge::BestVisibleEnemy()
{
	CBaseEntity* pReturn = NULL;
	int bestRelationship = R_NO;
	int nearest = 8192;
	for ( CBaseEntity* pEntity = m_pLink; pEntity; pEntity = pEntity->m_pLink )
	{
		if ( !pEntity->IsAlive() || pEntity->pev->team == pev->team )
			continue;
		if ( IRelationship( pEntity ) > bestRelationship )
		{
			bestRelationship = IRelationship( pEntity );
			nearest = (int)(pEntity->pev->origin - pev->origin).Length();
			pReturn = pEntity;
		}
		else if ( bestRelationship == IRelationship( pEntity ) )
		{
			int distance = (int)(pEntity->pev->origin - pev->origin).Length();
			if ( distance <= nearest )
			{
				pReturn = pEntity;
				nearest = distance;
				bestRelationship = IRelationship( pEntity );
			}
		}
	}
	return pReturn;
}

BOOL CSarge::GetEnemy()
{
	if ( HasConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS ) )
	{
		CBaseEntity* pNewEnemy = BestVisibleEnemy();
		if ( pNewEnemy != m_hEnemy && pNewEnemy && m_pSchedule )
		{
			if ( m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY )
			{
				PushEnemy( m_hEnemy, m_vecEnemyLKP );
				SetConditions( bits_COND_NEW_ENEMY );
				m_hEnemy = pNewEnemy;
				m_vecEnemyLKP = m_hEnemy->pev->origin;
			}
			if ( pNewEnemy->pev->owner )
			{
				CBaseEntity* pOwner = GetMonsterPointer( pNewEnemy->pev->owner );
				if ( pOwner && (pOwner->pev->flags & FL_MONSTER) && IRelationship( pOwner ) != R_NO )
					PushEnemy( pOwner, m_vecEnemyLKP );
			}
		}
	}
	if ( m_hEnemy == NULL && PopEnemy() && m_pSchedule && (m_pSchedule->iInterruptMask & bits_COND_NEW_ENEMY) )
		SetConditions( bits_COND_NEW_ENEMY );
	return m_hEnemy != NULL;
}

void CSarge::RunAI()
{
	UTIL_ParticleEffect( pev->origin + pev->view_ofs, g_vecZero, 255, 10 );
	if ( m_MonsterState == MONSTERSTATE_ALERT || m_MonsterState == MONSTERSTATE_IDLE )
	{
		if ( RANDOM_LONG( 0, 99 ) == 0 && !(pev->flags & FL_DUCKING) )
			IdleSound();
	}
	if ( m_MonsterState != MONSTERSTATE_PRONE && m_MonsterState != MONSTERSTATE_NONE && m_MonsterState != MONSTERSTATE_DEAD )
	{
		if ( !FNullEnt( FIND_CLIENT_IN_PVS( edict() ) ) || m_MonsterState == MONSTERSTATE_COMBAT )
		{
			Look( (int)m_flDistLook );
			Listen();
			ClearConditions( IgnoreConditions() );
			GetEnemy();
		}
		if ( m_hEnemy != NULL )
			CheckEnemy( m_hEnemy );
		CheckAmmo();
	}
	FCheckAITrigger();
	PrescheduleThink();
	MaintainSchedule();
}

void CSarge::Touch( CBaseEntity* pOther )
{
	if ( pOther->IsPlayer() && IRelationship( pOther ) <= R_NO )
		IsTalking();
}

void CSarge::GiveAdvice( CBaseEntity* pTarget )
{
}
