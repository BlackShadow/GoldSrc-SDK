#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "talkmonster.h"
#include "schedule.h"
#include "defaultai.h"
#include "weapons.h"
#include "soundent.h"

#define BARNEY_AE_DRAW		2
#define BARNEY_AE_SHOOT		3
#define BARNEY_AE_HOLSTER		4

class CRecruit : public CTalkMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int ISoundMask();
	void BarneyFirePistol();
	void AlertSound();
	int Classify();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	void RunTask( Task_t* pTask );
	void StartTask( Task_t* pTask );
	int ObjectCaps() { return CTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	void DeclineFollowing();
	Schedule_t* GetScheduleOfType( int Type );
	Schedule_t* GetSchedule();
	MONSTERSTATE GetIdealState();
	void DeathSound();
	void PainSound();
	void TalkInit();
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType );
	void Killed( entvars_t* pevAttacker, int iGib );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	static TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	float	m_flPlayerDamage;

	CUSTOM_SCHEDULES;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_recruit, CRecruit );

TYPEDESCRIPTION CRecruit::m_SaveData[] =
{
	DEFINE_FIELD( CRecruit, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRecruit, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( CRecruit, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( CRecruit, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CRecruit, m_flPlayerDamage, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CRecruit, CTalkMonster );

Task_t tlRcFollow[] =
{
	{ TASK_MOVE_TO_TARGET_RANGE, 128 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_FACE }
};

Schedule_t slRcFollow[] =
{
	{ tlRcFollow, ARRAYSIZE( tlRcFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED, bits_SOUND_DANGER, "Follow" }
};

Task_t tlRcrneyEnemyDraw[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_ARM }
};

Schedule_t slRcrneyEnemyDraw[] =
{
	{ tlRcrneyEnemyDraw, ARRAYSIZE( tlRcrneyEnemyDraw ), 0, 0, "Barney Enemy Draw" }
};

Task_t tlRcFaceTarget[] =
{
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE }
};

Schedule_t slRcFaceTarget[] =
{
	{ tlRcFaceTarget, ARRAYSIZE( tlRcFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_PROVOKED, bits_SOUND_DANGER, "FaceTarget" }
};

Task_t tlIdleRcStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 }
};

Schedule_t slIdleRcStand[] =
{
	{ tlIdleRcStand, ARRAYSIZE( tlIdleRcStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleStand" }
};

DEFINE_CUSTOM_SCHEDULES( CRecruit )
{
	slRcFollow,
	slRcrneyEnemyDraw,
	slRcFaceTarget,
	slIdleRcStand
};

IMPLEMENT_CUSTOM_SCHEDULES( CRecruit, CTalkMonster );

void CRecruit::StartTask( Task_t* pTask )
{
	CTalkMonster::StartTask( pTask );
}

void CRecruit::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_RANGE_ATTACK1 && m_hEnemy != NULL && m_hEnemy->IsPlayer() )
		pev->framerate = 1.5;
	CTalkMonster::RunTask( pTask );
}

int CRecruit::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_CARCASS | bits_SOUND_MEAT |
		bits_SOUND_GARBAGE | bits_SOUND_DANGER | bits_SOUND_PLAYER;
}

int CRecruit::Classify()
{
	return CLASS_HUMAN_FRIENDLY;
}

void CRecruit::AlertSound()
{
	if ( m_hEnemy != NULL && FOkToSpeak() )
		PlaySentence( "RC_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
}

void CRecruit::SetYawSpeed()
{
	pev->yaw_speed = m_Activity == ACT_RUN ? 90 : 70;
}

BOOL CRecruit::CheckRangeAttack1( float flDot, float flDist )
{
	if ( flDist > 1024 || flDot < 0.5 )
		return FALSE;
	if ( gpGlobals->time > m_checkAttackTime )
	{
		TraceResult tr;
		Vector shootOrigin = pev->origin + Vector( 0, 0, 55 );
		CBaseEntity* pEnemy = m_hEnemy;
		Vector shootTarget = ( pEnemy->BodyTarget( shootOrigin ) - pEnemy->pev->origin ) + m_vecEnemyLKP;
		UTIL_TraceLine( shootOrigin, shootTarget, dont_ignore_monsters, ENT( pev ), &tr );
		m_checkAttackTime = gpGlobals->time + 1;
		m_lastAttackCheck = tr.flFraction == 1.0 ||
			( tr.pHit != NULL && CBaseEntity::Instance( tr.pHit ) == pEnemy );
		m_checkAttackTime = gpGlobals->time + 1.5;
	}
	return m_lastAttackCheck;
}

void CRecruit::BarneyFirePistol()
{
	UTIL_MakeVectors( pev->angles );
	Vector vecShootOrigin = pev->origin + Vector( 0, 0, 55 );
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	pev->effects = EF_MUZZLEFLASH;
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_MONSTER_9MM );
	int pitchShift = RANDOM_LONG( 0, 20 );
	if ( pitchShift > 10 )
		pitchShift = 0;
	else
		pitchShift -= 5;
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "barney/ba_attack2.wav", 1, ATTN_NORM, 0, 100 + pitchShift );
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
	m_cAmmoLoaded--;
}

void CRecruit::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case BARNEY_AE_SHOOT:
		BarneyFirePistol();
		break;
	case BARNEY_AE_DRAW:
		pev->body = 1;
		m_fGunDrawn = TRUE;
		break;
	case BARNEY_AE_HOLSTER:
		pev->body = 0;
		m_fGunDrawn = FALSE;
		break;
	default:
		CTalkMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void CRecruit::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/recruit.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->health = gSkillData.barneyHealth;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_flFieldOfView = VIEW_FIELD_WIDE;
	m_MonsterState = MONSTERSTATE_NONE;
	pev->body = 0;
	m_fGunDrawn = FALSE;
	m_afCapability = bits_CAP_HEAR | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	MonsterInit();
	SetUse( &CRecruit::FollowerUse );
}

void CRecruit::Precache()
{
	PRECACHE_MODEL( "models/recruit.mdl" );
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

void CRecruit::TalkInit()
{
	CTalkMonster::TalkInit();
	m_szGrp[TLK_ANSWER] = "RC_ANSWER";
	m_szGrp[TLK_QUESTION] = "RC_QUESTION";
	m_szGrp[TLK_IDLE] = "RC_IDLE";
	m_szGrp[TLK_STARE] = "RC_STARE";
	m_szGrp[TLK_USE] = "RC_OK";
	m_szGrp[TLK_UNUSE] = "RC_WAIT";
	m_szGrp[TLK_STOP] = "RC_STOP";
	m_szGrp[TLK_NOSHOOT] = "RC_SCARED";
	m_szGrp[TLK_HELLO] = "RC_HELLO";
	m_szGrp[TLK_PLHURT1] = "!RC_CUREA";
	m_szGrp[TLK_PLHURT2] = "!RC_CUREB";
	m_szGrp[TLK_PLHURT3] = "!RC_CUREC";
	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;
	m_szGrp[TLK_PQUESTION] = "RC_PQUEST";
	m_szGrp[TLK_SMELL] = "RC_SMELL";
	m_szGrp[TLK_WOUND] = "RC_WOUND";
	m_szGrp[TLK_MORTAL] = "RC_MORTAL";
	m_voicePitch = 100;
}

static BOOL IsFacing( entvars_t* pevTest, const Vector& reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector forward;
	Vector angle = pevTest->v_angle;
	angle.x = 0;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	return DotProduct( forward, vecDir ) > 0.96;
}

int CRecruit::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	int ret = CTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return ret;
	if ( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;
		if ( m_hEnemy == NULL )
		{
			if ( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) )
			{
				PlaySentence( "RC_MAD", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				PlaySentence( "RC_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
			}
		}
		else if ( !m_hEnemy->IsPlayer() && pev->deadflag == DEAD_NO )
			PlaySentence( "RC_SHOT", 4, VOL_NORM, ATTN_NORM );
	}
	return ret;
}

void CRecruit::PainSound()
{
	if ( gpGlobals->time < m_painTime )
		return;
	m_painTime = gpGlobals->time + RANDOM_FLOAT( 0.5, 0.75 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_pain3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CRecruit::DeathSound()
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die1.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 1:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die2.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	case 2:
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "barney/ba_die3.wav", 1, ATTN_NORM, 0, GetVoicePitch() );
		break;
	}
}

void CRecruit::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
			flDamage /= 2;
		break;
	case 10:
		if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
		{
			flDamage -= 20;
			if ( flDamage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1.0 );
				flDamage = 0.01;
			}
		}
		ptr->iHitgroup = HITGROUP_HEAD;
		break;
	}
	CTalkMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

void CRecruit::Killed( entvars_t* pevAttacker, int iGib )
{
	SetUse( NULL );
	CTalkMonster::Killed( pevAttacker, iGib );
}

Schedule_t* CRecruit::GetScheduleOfType( int Type )
{
	Schedule_t* psched;
	switch ( Type )
	{
	case SCHED_ARM_WEAPON:
		if ( m_hEnemy != NULL )
			return slRcrneyEnemyDraw;
		break;
	case SCHED_TARGET_FACE:
		psched = CTalkMonster::GetScheduleOfType( Type );
		return psched == slIdleStand ? slRcFaceTarget : psched;
	case SCHED_TARGET_CHASE:
		return slRcFollow;
	case SCHED_IDLE_STAND:
		psched = CTalkMonster::GetScheduleOfType( Type );
		return psched == slIdleStand ? slIdleRcStand : psched;
	}
	return CTalkMonster::GetScheduleOfType( Type );
}

Schedule_t* CRecruit::GetSchedule()
{
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
		PlaySentence( "RC_KILL", 4, VOL_NORM, ATTN_NORM );
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
			if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				return GetScheduleOfType( SCHED_MOVE_AWAY_FOLLOW );
			return GetScheduleOfType( SCHED_TARGET_FACE );
		}
		if ( HasConditions( bits_COND_CLIENT_PUSH ) )
			return GetScheduleOfType( SCHED_MOVE_AWAY );
		TrySmellTalk();
		break;
	}
	return CTalkMonster::GetSchedule();
}

MONSTERSTATE CRecruit::GetIdealState()
{
	return CTalkMonster::GetIdealState();
}

void CRecruit::DeclineFollowing()
{
	PlaySentence( "RC_POK", 2, VOL_NORM, ATTN_NORM );
}
