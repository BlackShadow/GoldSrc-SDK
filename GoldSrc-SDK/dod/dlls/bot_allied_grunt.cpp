#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"
#include "squadmonster.h"
#include "weapons.h"
#include "talkmonster.h"
#include "soundent.h"
#include "effects.h"
#include "scripted.h"

#define ALLIEDGRUNT_COLT			( 1 << 0 )
#define ALLIEDGRUNT_GARAND		( 1 << 1 )
#define ALLIEDGRUNT_THOMPSON		( 1 << 2 )
#define ALLIEDGRUNT_BAR			( 1 << 3 )
#define ALLIEDGRUNT_HANDGRENADE	( 1 << 4 )
#define bits_COND_GRUNT_NOFIRE	bits_COND_SPECIAL1

enum
{
	SCHED_GRUNT_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,
	SCHED_GRUNT_ELOF_FAIL
};

enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE
};

enum
{
	ALLIEDGRUNT_SENT_NONE = -1,
	ALLIEDGRUNT_SENT_GREN,
	ALLIEDGRUNT_SENT_ALERT,
	ALLIEDGRUNT_SENT_MONSTER,
	ALLIEDGRUNT_SENT_COVER,
	ALLIEDGRUNT_SENT_THROW,
	ALLIEDGRUNT_SENT_CHARGE,
	ALLIEDGRUNT_SENT_TAUNT
} ALLIEDGRUNT_SENTENCE_TYPES;

int g_fAlliedGruntQuestion;
extern DLL_GLOBAL int g_iSkillLevel;

class CAlliedGrunt : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void SetYawSpeed() override;
	int Classify() override;
	int ISoundMask() override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack2( float flDot, float flDist ) override;
	void CheckAmmo() override;
	void SetActivity( Activity NewActivity ) override;
	void StartTask( Task_t* pTask ) override;
	void RunTask( Task_t* pTask ) override;
	void DeathSound() override;
	void PainSound() override;
	void IdleSound() override;
	Vector GetGunPosition() override;
	void Shoot();
	void PrescheduleThink() override;
	void GibMonster() override;
	void SpeakSentence();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	CBaseEntity* Kick();
	Schedule_t* GetSchedule() override;
	Schedule_t* GetScheduleOfType( int Type ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
		int bitsDamageType ) override;
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;
	int IRelationship( CBaseEntity* pTarget ) override;
	BOOL FOkToSpeak();
	void JustSpoke();
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	float	m_flNextGrenadeCheck;
	float	m_flNextPainTime;
	float	m_flLastEnemySightTime;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;
	int		m_cClipSize;
	int		m_voicePitch;
	int		m_iBrassShell;
	int		m_iSentence;
	static const char* pAlliedGruntSentences[];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_allied_grunt, CAlliedGrunt );

TYPEDESCRIPTION CAlliedGrunt::m_SaveData[] =
{
	DEFINE_FIELD( CAlliedGrunt, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CAlliedGrunt, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CAlliedGrunt, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CAlliedGrunt, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CAlliedGrunt, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CAlliedGrunt, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CAlliedGrunt, m_iSentence, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CAlliedGrunt, CSquadMonster );

const char* CAlliedGrunt::pAlliedGruntSentences[] =
{
	"ALLIEDG_GREN", "ALLIEDG_ALERT", "ALLIEDG_MONSTER", "ALLIEDG_COVER",
	"ALLIEDG_THROW", "ALLIEDG_CHARGE", "ALLIEDG_TAUNT"
};

int CAlliedGrunt::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

BOOL CAlliedGrunt::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

int CAlliedGrunt::Classify()
{
	if ( m_iClass )
		return m_iClass;
	return ( pev->spawnflags & 0x800 ) ? CLASS_PLAYER_ALLY : CLASS_HUMAN_MILITARY;
}

int CAlliedGrunt::IRelationship( CBaseEntity* pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_axis_grunt" ) )
		return R_NM;
	return CBaseMonster::IRelationship( pTarget );
}

void CAlliedGrunt::Precache()
{
	PRECACHE_MODEL( "models/allied_grunt.mdl" );
	PRECACHE_SOUND( "weapons/garand_shoot.wav" );
	PRECACHE_SOUND( "weapons/colt_shoot.wav" );
	PRECACHE_SOUND( "weapons/bar_shoot.wav" );
	PRECACHE_SOUND( "weapons/thompson_shoot.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain4.wav" );
	PRECACHE_SOUND( "hgrunt/gr_pain5.wav" );
	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	m_voicePitch = RANDOM_LONG( 0, 1 ) ? 90 + RANDOM_LONG( 0, 7 ) : 100;
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
}

void CAlliedGrunt::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/allied_grunt.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = 100;
	m_flFieldOfView = 0.2f;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = ALLIEDGRUNT_SENT_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	m_flDistTooFar = 2048;
	m_flDistLook = 2048;
	if ( !pev->weapons )
		pev->weapons = ALLIEDGRUNT_GARAND;
	if ( pev->weapons & ALLIEDGRUNT_GARAND )
	{
		SetBodygroup( 0, 0 );
		m_cClipSize = 8;
		ALERT( at_console, "Spawn new light grunt with GARAND" );
	}
	else if ( pev->weapons & ALLIEDGRUNT_THOMPSON )
	{
		SetBodygroup( 0, 1 );
		ALERT( at_console, "Spawn new medium grunt with THOMPSON" );
	}
	else if ( pev->weapons & ALLIEDGRUNT_BAR )
	{
		SetBodygroup( 0, 2 );
		m_cClipSize = 20;
		ALERT( at_console, "Spawn new heavy grunt with BAR" );
	}
	m_cAmmoLoaded = m_cClipSize;
	CTalkMonster::g_talkWaitTime = 0;
	MonsterInit();
}

void CAlliedGrunt::SetYawSpeed()
{
	int speed;
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN: speed = 150; break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT: speed = 180; break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2: speed = 120; break;
	case ACT_GLIDE:
	case ACT_FLY: speed = 30; break;
	default: speed = 90; break;
	}
	pev->yaw_speed = (float)speed;
}

void CAlliedGrunt::CheckAmmo()
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

Vector CAlliedGrunt::GetGunPosition()
{
	return pev->origin + Vector( 0, 0, m_fStanding ? 60 : 48 );
}

int CAlliedGrunt::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CAlliedGrunt::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr,
	int bitsDamageType )
{
	if ( ptr->iHitgroup == 11 )
	{
		if ( GetBodygroup( 0 ) != 1 && ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB ) ) )
		{
			flDamage -= 20;
			if ( flDamage <= 0 )
			{
				UTIL_Ricochet( ptr->vecEndPos, 1 );
				flDamage = 0.01f;
			}
		}
		ptr->iHitgroup = HITGROUP_HEAD;
	}
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType | DMG_NEVERGIB );
}

BOOL CAlliedGrunt::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( m_hEnemy == NULL )
		return FALSE;
	CBaseMonster* pEnemy = m_hEnemy->MyMonsterPointer();
	if ( !pEnemy )
		return FALSE;
	if ( flDist > 64 || flDot < 0.7 )
		return FALSE;
	return pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON && pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON;
}

BOOL CAlliedGrunt::CheckRangeAttack1( float flDot, float flDist )
{
	if ( !HasConditions( bits_COND_ENEMY_OCCLUDED ) && flDist <= 2048 && flDot >= 0.5f && NoFriendlyFire() )
	{
		if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
			return FALSE;
		TraceResult tr;
		Vector vecSrc = GetGunPosition();
		UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ignore_glass, edict(), &tr );
		return tr.flFraction == 1;
	}
	return FALSE;
}

BOOL CAlliedGrunt::CheckRangeAttack2( float flDot, float flDist )
{
	(void)flDot;
	(void)flDist;
	if ( !( pev->weapons & ALLIEDGRUNT_HANDGRENADE ) )
		return FALSE;
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;
	if ( !( m_hEnemy->pev->flags & FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0
		&& m_vecEnemyLKP.z > pev->absmax.z )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector vecTarget;
	if ( RANDOM_LONG( 0, 1 ) )
		vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
	else
		vecTarget = m_vecEnemyLKP;
	if ( InSquad() && SquadMemberInRange( vecTarget, 256 ) )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
	}
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5f );
	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = gpGlobals->time;
	}
	else
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	return m_fThrowGrenade;
}

void CAlliedGrunt::Shoot()
{
	if ( m_hEnemy == NULL )
		return;
	Vector vecSrc = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecSrc );
	UTIL_MakeVectors( pev->angles );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 )
		+ gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	EjectBrass( vecSrc - vecShootDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	if ( pev->weapons & ALLIEDGRUNT_GARAND )
	{
		FireBullets( 1, vecSrc, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_PLAYER_GARAND, 2, 0, pev );
		ALERT( at_console, "Shoot GARAND\n" );
	}
	else if ( pev->weapons & ALLIEDGRUNT_BAR )
	{
		FireBullets( 1, vecSrc, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BAR, 2, 0, pev );
		ALERT( at_console, "Shoot BAR\n" );
	}
	else if ( pev->weapons & ALLIEDGRUNT_THOMPSON )
	{
		FireBullets( 1, vecSrc, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_THOMPSON, 2, 0, pev );
		ALERT( at_console, "Shoot THOMPSON\n" );
	}
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	SetBlending( 0, UTIL_VecToAngles( vecShootDir ).x );
}

CBaseEntity* CAlliedGrunt::Kick()
{
	TraceResult tr;
	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5f;
	UTIL_TraceHull( vecStart, vecStart + gpGlobals->v_forward * 70, dont_ignore_monsters, head_hull, edict(), &tr );
	return tr.pHit ? CBaseEntity::Instance( tr.pHit ) : NULL;
}

void CAlliedGrunt::GibMonster()
{
	if ( GetBodygroup( 2 ) != 2 && !( pev->spawnflags & 0x400 ) )
	{
		Vector vecGunPos, vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		CBaseEntity* pGun = DropItem( ( pev->weapons & ALLIEDGRUNT_GARAND ) ? "weapon_garand"
			: ( pev->weapons & ALLIEDGRUNT_BAR ) ? "weapon_bar" : "weapon_colt", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}
	CBaseMonster::GibMonster();
}

void CAlliedGrunt::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 2:
		EMIT_SOUND( edict(), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
		m_cAmmoLoaded = m_cClipSize;
		ClearConditions( bits_COND_NO_AMMO_LOADED );
		break;
	case 3:
	{
		CBaseEntity* pHurt = Kick();
		if ( pHurt )
		{
			UTIL_MakeVectors( pev->angles );
			pHurt->pev->punchangle.x = 15;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			pHurt->TakeDamage( pev, pev, gSkillData.hgruntDmgKick, DMG_CLUB );
		}
		break;
	}
	case 4:
		if ( !( pev->weapons & ALLIEDGRUNT_GARAND ) && ( pev->weapons & ALLIEDGRUNT_BAR ) )
			EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/bar_shoot.wav", 1, ATTN_NORM );
		Shoot();
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );
		break;
	case 5:
	case 6:
		Shoot();
		break;
	case 7:
	{
		UTIL_MakeVectors( pev->angles );
		Vector vecToss = m_vecTossVelocity;
		if ( m_pCine )
		{
			vecToss = g_vecZero;
			if ( m_hTargetEnt != NULL && m_pCine->m_fTurnType == 1
				&& ( m_pCine->m_fMoveTo == 5 || ( m_pCine->m_iszAttack && m_pCine->m_fMoveTo != 0 ) ) )
				vecToss = VecCheckToss( pev, GetGunPosition(), m_hTargetEnt->pev->origin, 0.5f );
			if ( vecToss == g_vecZero )
				vecToss = ( gpGlobals->v_forward * 0.5f + gpGlobals->v_up * 0.5f ).Normalize() * gSkillData.hgruntGrenadeSpeed;
		}
		CGrenade::ShootTimed( pev, GetGunPosition(), vecToss, 3.5f );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	}
	case 9:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27
			+ gpGlobals->v_up * 6, g_vecZero, 3 );
		break;
	case 10:
		if ( FOkToSpeak() )
		{
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_ALERT", 0.35f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	case 11:
		if ( !( pev->spawnflags & 0x400 ) )
		{
			Vector vecGunPos, vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			DropItem( ( pev->weapons & ALLIEDGRUNT_GARAND ) ? "weapon_garand"
				: ( pev->weapons & ALLIEDGRUNT_BAR ) ? "weapon_bar" : "weapon_colt", vecGunPos, vecGunAngles );
		}
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

BOOL CAlliedGrunt::FOkToSpeak()
{
	if ( !( gpGlobals->time > CTalkMonster::g_talkWaitTime ) )
		return FALSE;
	return !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT;
}

void CAlliedGrunt::JustSpoke()
{
	float time = gpGlobals->time;
	CTalkMonster::g_talkWaitTime = time + RANDOM_FLOAT( 1.5f, 2 );
	m_iSentence = ALLIEDGRUNT_SENT_NONE;
}

void CAlliedGrunt::SpeakSentence()
{
	if ( m_iSentence != ALLIEDGRUNT_SENT_NONE && FOkToSpeak() )
	{
		SENTENCEG_PlayRndSz( edict(), pAlliedGruntSentences[m_iSentence], 0.35f, ATTN_NORM, 0, m_voicePitch );
		JustSpoke();
	}
}

void CAlliedGrunt::IdleSound()
{
	if ( !FOkToSpeak() || !( g_fAlliedGruntQuestion || RANDOM_LONG( 0, 1 ) ) )
		return;
	if ( !g_fAlliedGruntQuestion )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_CHECK", 0.35f, ATTN_NORM, 0, m_voicePitch );
			g_fAlliedGruntQuestion = 1;
			break;
		case 1:
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_QUEST", 0.35f, ATTN_NORM, 0, m_voicePitch );
			g_fAlliedGruntQuestion = 2;
			break;
		case 2:
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_IDLE", 0.35f, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	else
	{
		if ( g_fAlliedGruntQuestion == 1 )
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_CLEAR", 0.35f, ATTN_NORM, 0, m_voicePitch );
		else if ( g_fAlliedGruntQuestion == 2 )
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_ANSWER", 0.35f, ATTN_NORM, 0, m_voicePitch );
		g_fAlliedGruntQuestion = 0;
	}
	JustSpoke();
}

void CAlliedGrunt::PainSound()
{
	if ( gpGlobals->time > m_flNextPainTime )
	{
		switch ( RANDOM_LONG( 0, 6 ) )
		{
		case 0: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_pain3.wav", 1, ATTN_NORM ); break;
		case 1: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_pain4.wav", 1, ATTN_NORM ); break;
		case 2: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_pain5.wav", 1, ATTN_NORM ); break;
		case 3: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_pain1.wav", 1, ATTN_NORM ); break;
		case 4: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_pain2.wav", 1, ATTN_NORM ); break;
		}
		m_flNextPainTime = gpGlobals->time + 1;
	}
}

void CAlliedGrunt::DeathSound()
{
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_die1.wav", 1, ATTN_IDLE ); break;
	case 1: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_die2.wav", 1, ATTN_IDLE ); break;
	case 2: EMIT_SOUND( edict(), CHAN_VOICE, "hgrunt/gr_die3.wav", 1, ATTN_IDLE ); break;
	}
}

void CAlliedGrunt::PrescheduleThink()
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

void CAlliedGrunt::SetActivity( Activity NewActivity )
{
	GET_MODEL_PTR( edict() );
	int sequence = ACTIVITY_NOT_AVAILABLE;
	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		if ( pev->weapons & ALLIEDGRUNT_COLT )
			sequence = LookupSequence( m_fStanding ? "standing_colt" : "crouching_colt" );
		else if ( pev->weapons & ALLIEDGRUNT_GARAND )
			sequence = LookupSequence( m_fStanding ? "standing_garand" : "crouching_garand" );
		break;
	case ACT_RANGE_ATTACK2:
		if ( pev->weapons & ALLIEDGRUNT_HANDGRENADE )
			sequence = LookupSequence( "throwgrenade" );
		else
			ALERT( at_console, "No grenades available. " );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health > 20 ? ACT_RUN : ACT_RUN_HURT );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health > 20 ? ACT_WALK : ACT_WALK_HURT );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			NewActivity = ACT_IDLE_ANGRY;
		sequence = LookupActivity( NewActivity );
		break;
	default:
		sequence = LookupActivity( NewActivity );
		break;
	}
	m_Activity = NewActivity;
	if ( sequence >= 0 )
	{
		if ( pev->sequence != sequence || !m_fSequenceLoops )
			pev->frame = 0;
		pev->sequence = sequence;
		ResetSequenceInfo();
		SetYawSpeed();
	}
	else
	{
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), NewActivity );
		pev->sequence = 0;
	}
}

void CAlliedGrunt::StartTask( Task_t* pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( pTask->iTask )
	{
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		Forget( bits_MEMORY_INCOVER );
		CBaseMonster::StartTask( pTask );
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CBaseMonster::StartTask( pTask );
		if ( pev->movetype == MOVETYPE_FLY )
			m_IdealActivity = ACT_GLIDE;
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_GRUNT_FACE_TOSS_DIR:
		break;
	case TASK_GRUNT_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_GRUNT_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_GRUNT_NOFIRE );
		TaskComplete();
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CAlliedGrunt::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_GRUNT_FACE_TOSS_DIR )
	{
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( (int)pev->yaw_speed );
		if ( FacingIdeal() )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	else
		CBaseMonster::RunTask( pTask );
}

Task_t tlAlliedGruntFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slAlliedGruntFail[] =
{
	{ tlAlliedGruntFail, ARRAYSIZE( tlAlliedGruntFail ), bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2
		| bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2, 0, "Grunt Fail" }
};

Task_t tlAlliedGruntCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slAlliedGruntCombatFail[] =
{
	{ tlAlliedGruntCombatFail, ARRAYSIZE( tlAlliedGruntCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Grunt Combat Fail" }
};

Task_t tlAlliedGruntVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};
Schedule_t slAlliedGruntVictoryDance[] =
{
	{ tlAlliedGruntVictoryDance, ARRAYSIZE( tlAlliedGruntVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "GruntVictoryDance" }
};

Task_t tlAlliedGruntEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_GRUNT_ELOF_FAIL },
	{ TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }
};
Schedule_t slAlliedGruntEstablishLineOfFire[] =
{
	{ tlAlliedGruntEstablishLineOfFire, ARRAYSIZE( tlAlliedGruntEstablishLineOfFire ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2
		| bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntEstablishLineOfFire" }
};

Task_t tlAlliedGruntFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};
Schedule_t slAlliedGruntFoundEnemy[] =
{
	{ tlAlliedGruntFoundEnemy, ARRAYSIZE( tlAlliedGruntFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntFoundEnemy" }
};

Task_t tlAlliedGruntCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5f }, { TASK_SET_SCHEDULE, SCHED_GRUNT_SWEEP }
};
Schedule_t slAlliedGruntCombatFace[] =
{
	{ tlAlliedGruntCombatFace1, ARRAYSIZE( tlAlliedGruntCombatFace1 ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD
		| bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Combat Face" }
};

Task_t tlAlliedGruntSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slAlliedGruntSignalSuppress[] =
{
	{ tlAlliedGruntSignalSuppress, ARRAYSIZE( tlAlliedGruntSignalSuppress ), bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE
		| bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "SignalSuppress" }
};

Task_t tlAlliedGruntSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slAlliedGruntSuppress[] =
{
	{ tlAlliedGruntSuppress, ARRAYSIZE( tlAlliedGruntSuppress ), bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE
		| bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Suppress" }
};

Task_t tlAlliedGruntWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 1 }
};
Schedule_t slAlliedGruntWaitInCover[] =
{
	{ tlAlliedGruntWaitInCover, ARRAYSIZE( tlAlliedGruntWaitInCover ), bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND
		| bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2,
		bits_SOUND_DANGER, "GruntWaitInCover" }
};

Task_t tlAlliedGruntTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_GRUNT_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};
Schedule_t slAlliedGruntTakeCover[] =
{
	{ tlAlliedGruntTakeCover1, ARRAYSIZE( tlAlliedGruntTakeCover1 ), 0, 0, "TakeCover" }
};

Task_t tlAlliedGruntGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};
Schedule_t slAlliedGruntGrenadeCover[] =
{
	{ tlAlliedGruntGrenadeCover1, ARRAYSIZE( tlAlliedGruntGrenadeCover1 ), 0, 0, "GrenadeCover" }
};

Task_t tlAlliedGruntTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 }, { TASK_RANGE_ATTACK2, 0 }, { TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};
Schedule_t slAlliedGruntTossGrenadeCover[] =
{
	{ tlAlliedGruntTossGrenadeCover1, ARRAYSIZE( tlAlliedGruntTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" }
};

Task_t tlAlliedGruntTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, 179 }
};
Schedule_t slAlliedGruntTakeCoverFromBestSound[] =
{
	{ tlAlliedGruntTakeCoverFromBestSound, ARRAYSIZE( tlAlliedGruntTakeCoverFromBestSound ), 0, 0, "GruntTakeCoverFromBestSound" }
};

Task_t tlAlliedGruntHideReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RELOAD }
};
Schedule_t slAlliedGruntHideReload[] =
{
	{ tlAlliedGruntHideReload, ARRAYSIZE( tlAlliedGruntHideReload ), bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntHideReload" }
};

Task_t tlAlliedGruntSweep[] =
{
	{ TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }, { TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }
};
Schedule_t slAlliedGruntSweep[] =
{
	{ tlAlliedGruntSweep, ARRAYSIZE( tlAlliedGruntSweep ), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE
		| bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "Grunt Sweep" }
};

Task_t tlAlliedGruntRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slAlliedGruntRangeAttack1A[] =
{
	{ tlAlliedGruntRangeAttack1A, ARRAYSIZE( tlAlliedGruntRangeAttack1A ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD
		| bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1A" }
};

Task_t tlAlliedGruntRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_IDLE_ANGRY },
	{ TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slAlliedGruntRangeAttack1B[] =
{
	{ tlAlliedGruntRangeAttack1B, ARRAYSIZE( tlAlliedGruntRangeAttack1B ), bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD
		| bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_NO_AMMO_LOADED | bits_COND_GRUNT_NOFIRE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "Range Attack1B" }
};

Task_t tlAlliedGruntRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_GRUNT_FACE_TOSS_DIR, 0 }, { TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, SCHED_GRUNT_WAIT_FACE_ENEMY }
};
Schedule_t slAlliedGruntRangeAttack2[] =
{
	{ tlAlliedGruntRangeAttack2, ARRAYSIZE( tlAlliedGruntRangeAttack2 ), 0, 0, "RangeAttack2" }
};

DEFINE_CUSTOM_SCHEDULES( CAlliedGrunt )
{
	slAlliedGruntFail,
	slAlliedGruntCombatFail,
	slAlliedGruntVictoryDance,
	slAlliedGruntEstablishLineOfFire,
	slAlliedGruntFoundEnemy,
	slAlliedGruntCombatFace,
	slAlliedGruntSignalSuppress,
	slAlliedGruntSuppress,
	slAlliedGruntWaitInCover,
	slAlliedGruntTakeCover,
	slAlliedGruntGrenadeCover,
	slAlliedGruntTossGrenadeCover,
	slAlliedGruntTakeCoverFromBestSound,
	slAlliedGruntHideReload,
	slAlliedGruntSweep,
	slAlliedGruntRangeAttack1A,
	slAlliedGruntRangeAttack1B,
	slAlliedGruntRangeAttack2
};

IMPLEMENT_CUSTOM_SCHEDULES( CAlliedGrunt, CSquadMonster );

Schedule_t* CAlliedGrunt::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 )
				&& OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				if ( FOkToSpeak() )
				{
					SENTENCEG_PlayRndSz( edict(), "ALLIEDG_THROW", 0.35f, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return slAlliedGruntTossGrenadeCover;
			}
			return slAlliedGruntTakeCover;
		}
		return RANDOM_LONG( 0, 1 ) ? slAlliedGruntTakeCover : slAlliedGruntGrenadeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slAlliedGruntTakeCoverFromBestSound;
	case SCHED_GRUNT_TAKECOVER_FAILED:
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		return GetScheduleOfType( SCHED_FAIL );
	case SCHED_GRUNT_ELOF_FAIL:
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		return slAlliedGruntEstablishLineOfFire;
	case SCHED_RANGE_ATTACK1:
		if ( RANDOM_LONG( 0, 4 ) == 0 )
			m_fStanding = RANDOM_LONG( 0, 1 );
		return m_fStanding ? slAlliedGruntRangeAttack1B : slAlliedGruntRangeAttack1A;
	case SCHED_RANGE_ATTACK2:
		return slAlliedGruntRangeAttack2;
	case SCHED_COMBAT_FACE:
		return slAlliedGruntCombatFace;
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		return slAlliedGruntWaitInCover;
	case SCHED_GRUNT_SWEEP:
		return slAlliedGruntSweep;
	case SCHED_GRUNT_COVER_AND_RELOAD:
		return slAlliedGruntHideReload;
	case SCHED_GRUNT_FOUND_ENEMY:
		return slAlliedGruntFoundEnemy;
	case SCHED_VICTORY_DANCE:
		return InSquad() && !IsLeader() ? slAlliedGruntFail : slAlliedGruntVictoryDance;
	case SCHED_GRUNT_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slAlliedGruntSignalSuppress;
		}
		return slAlliedGruntSuppress;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slAlliedGruntCombatFail : slAlliedGruntFail;
	default:
		return CSquadMonster::GetScheduleOfType( Type );
	}
}

Schedule_t* CAlliedGrunt::GetSchedule()
{
	m_iSentence = ALLIEDGRUNT_SENT_NONE;
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if ( pev->flags & FL_ONGROUND )
		{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_GRUNT_REPEL_LAND );
		}
		return GetScheduleOfType( m_MonsterState == MONSTERSTATE_COMBAT ? SCHED_GRUNT_REPEL_ATTACK : SCHED_GRUNT_REPEL );
	}
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( edict(), "ALLIEDG_GREN", 0.35f, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}
	if ( m_MonsterState != MONSTERSTATE_COMBAT || HasConditions( bits_COND_ENEMY_DEAD ) )
		return CBaseMonster::GetSchedule();
	if ( HasConditions( bits_COND_NEW_ENEMY ) )
	{
		if ( InSquad() )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			if ( !IsLeader() )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			ALERT( at_aiconsole, "leader spotted player!\n" );
			if ( FOkToSpeak() )
			{
				if ( m_hEnemy != NULL && m_hEnemy->IsPlayer() )
					SENTENCEG_PlayRndSz( edict(), "ALLIEDG_ALERT", 0.35f, ATTN_NORM, 0, m_voicePitch );
				else if ( m_hEnemy != NULL && m_hEnemy->Classify() != CLASS_PLAYER_ALLY
					&& m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE && m_hEnemy->Classify() != CLASS_MACHINE )
					SENTENCEG_PlayRndSz( edict(), "ALLIEDG_MONST", 0.35f, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 )
				? SCHED_GRUNT_SUPPRESS : SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
		}
		if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
		return CBaseMonster::GetSchedule();
	}
	if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
		return GetScheduleOfType( SCHED_GRUNT_COVER_AND_RELOAD );
	if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
	{
		if ( RANDOM_LONG( 0, 99 ) > 90 || m_hEnemy == NULL )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if ( FOkToSpeak() )
			m_iSentence = ALLIEDGRUNT_SENT_COVER;
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	}
	if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			return GetScheduleOfType( SCHED_GRUNT_FOUND_ENEMY );
		}
		if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	}
	if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) )
	{
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( edict(), "ALLIEDG_THROW", 0.35f, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		}
		if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
		{
			if ( FOkToSpeak() )
				m_iSentence = ALLIEDGRUNT_SENT_CHARGE;
			return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
		}
		if ( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
		{
			SENTENCEG_PlayRndSz( edict(), "ALLIEDG_TAUNT", 0.35f, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
		return GetScheduleOfType( SCHED_STANDOFF );
	}
	if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
	return CBaseMonster::GetSchedule();
}

class CDeadAlliedGrunt : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override;
	void KeyValue( KeyValueData* pkvd ) override;
	int m_iPose;
	static char* m_szPoses[3];
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_alliedgrunt_dead, CDeadAlliedGrunt );

char* CDeadAlliedGrunt::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

int CDeadAlliedGrunt::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

void CDeadAlliedGrunt::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CDeadAlliedGrunt::Spawn()
{
	PRECACHE_MODEL( "models/allied_grunt.mdl" );
	SET_MODEL( edict(), "models/allied_grunt.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead hgrunt with bad pose\n" );
	pev->health = 8;
	switch ( pev->body )
	{
	case 0:
		pev->skin = 0;
		SetBodygroup( 0, 0 );
		break;
	case 1:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( 0, 1 );
		break;
	case 2:
		pev->body = 0;
		pev->skin = 0;
		SetBodygroup( 0, 2 );
		break;
	}
	MonsterInitDead();
}
