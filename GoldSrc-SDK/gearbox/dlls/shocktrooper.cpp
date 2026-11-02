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
#include "customentity.h"
#include "op4weap.h"

#define SHOCKTROOPER_HANDGRENADE	2
#define SHOCKTROOPER_SENTENCE_VOLUME	0.35f
#define bits_COND_SHOCKTROOPER_NOFIRE	bits_COND_SPECIAL1

enum
{
	SCHED_SHOCKTROOPER_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE,
	SCHED_SHOCKTROOPER_SWEEP,
	SCHED_SHOCKTROOPER_FOUND_ENEMY,
	SCHED_SHOCKTROOPER_REPEL,
	SCHED_SHOCKTROOPER_REPEL_ATTACK,
	SCHED_SHOCKTROOPER_REPEL_LAND,
	SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY,
	SCHED_SHOCKTROOPER_TAKECOVER_FAILED,
	SCHED_SHOCKTROOPER_ELOF_FAIL,
};

enum
{
	TASK_SHOCKTROOPER_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_SHOCKTROOPER_SPEAK_SENTENCE,
	TASK_SHOCKTROOPER_CHECK_FIRE,
};

enum
{
	SHOCKTROOPER_SENT_NONE = -1,
	SHOCKTROOPER_SENT_GREN,
	SHOCKTROOPER_SENT_ALERT,
	SHOCKTROOPER_SENT_MONSTER,
	SHOCKTROOPER_SENT_COVER,
	SHOCKTROOPER_SENT_THROW,
	SHOCKTROOPER_SENT_CHARGE,
	SHOCKTROOPER_SENT_TAUNT,
} ShockTrooper_SENTENCE_TYPES;

int g_fShockTrooperQuestion;
int iShockTrooperMuzzleFlash;
extern DLL_GLOBAL int g_iSkillLevel;

class CShockTrooper : public CSquadMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	int ISoundMask();
	void HandleAnimEvent( MonsterEvent_t* event );
	BOOL FCanCheckAttacks();
	BOOL CheckMeleeAttack1( float dot, float distance );
	BOOL CheckRangeAttack1( float dot, float distance );
	BOOL CheckRangeAttack2( float dot, float distance );
	void CheckAmmo();
	void SetActivity( Activity activity );
	void StartTask( Task_t* task );
	void RunTask( Task_t* task );
	void PainSound();
	void IdleSound();
	Vector GetGunPosition();
	void Shoot();
	void PrescheduleThink();
	void GibMonster();
	void SpeakSentence();
	void MonsterThink();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	CBaseEntity* Kick();
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType( int type );
	void TraceAttack( entvars_t* attacker, float damage, Vector direction, TraceResult* trace, int damageType );
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType );
	int IRelationship( CBaseEntity* target );
	BOOL FOkToSpeak();
	void JustSpoke();
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pGruntSentences[];

	float	m_flNextGrenadeCheck;
	float	m_flNextPainTime;
	float	m_flLastEnemySightTime;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;
	int		m_cClipSize;
	float	m_flLastShot;
	int		m_voicePitch;
	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iSentence;
	float	m_flLastChargeTime;
	float	m_flLastBlinkTime;
	float	m_flLastBlinkInterval;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_shocktrooper, CShockTrooper );

TYPEDESCRIPTION CShockTrooper::m_SaveData[] =
{
	DEFINE_FIELD( CShockTrooper, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CShockTrooper, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CShockTrooper, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CShockTrooper, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShockTrooper, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShockTrooper, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CShockTrooper, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CShockTrooper, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CShockTrooper, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CShockTrooper, m_flLastChargeTime, FIELD_FLOAT ),
	DEFINE_FIELD( CShockTrooper, m_flLastShot, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CShockTrooper, CSquadMonster );

const char* CShockTrooper::pGruntSentences[] =
{
	"ST_GREN", "ST_ALERT", "ST_MONSTER", "ST_COVER", "ST_THROW", "ST_CHARGE", "ST_TAUNT"
};

Task_t tlShockTrooperFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slShockTrooperFail[] =
{
	{ tlShockTrooperFail, ARRAYSIZE( tlShockTrooperFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2,
		0, "Grunt Fail" }
};

Task_t tlShockTrooperCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slShockTrooperCombatFail[] =
{
	{ tlShockTrooperCombatFail, ARRAYSIZE( tlShockTrooperCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Grunt Combat Fail" }
};

Task_t tlShockTrooperVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 1.5 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 }, { TASK_WALK_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};
Schedule_t slShockTrooperVictoryDance[] =
{
	{ tlShockTrooperVictoryDance, ARRAYSIZE( tlShockTrooperVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "GruntVictoryDance" }
};

Task_t tlShockTrooperEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_SHOCKTROOPER_ELOF_FAIL }, { TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_SPEAK_SENTENCE, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }
};
Schedule_t slShockTrooperEstablishLineOfFire[] =
{
	{ tlShockTrooperEstablishLineOfFire, ARRAYSIZE( tlShockTrooperEstablishLineOfFire ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntEstablishLineOfFire" }
};

Task_t tlShockTrooperFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};
Schedule_t slShockTrooperFoundEnemy[] =
{
	{ tlShockTrooperFoundEnemy, ARRAYSIZE( tlShockTrooperFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntFoundEnemy" }
};

Task_t tlShockTrooperCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 }, { TASK_SET_SCHEDULE, SCHED_SHOCKTROOPER_SWEEP }
};
Schedule_t slShockTrooperCombatFace[] =
{
	{ tlShockTrooperCombatFace1, ARRAYSIZE( tlShockTrooperCombatFace1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,
		0, "Combat Face" }
};

Task_t tlShockTrooperSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slShockTrooperSignalSuppress[] =
{
	{ tlShockTrooperSignalSuppress, ARRAYSIZE( tlShockTrooperSignalSuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_NO_AMMO_LOADED | bits_COND_SHOCKTROOPER_NOFIRE,
		bits_SOUND_DANGER, "SignalSuppress" }
};

Task_t tlShockTrooperSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slShockTrooperSuppress[] =
{
	{ tlShockTrooperSuppress, ARRAYSIZE( tlShockTrooperSuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_NO_AMMO_LOADED | bits_COND_SHOCKTROOPER_NOFIRE,
		bits_SOUND_DANGER, "Suppress" }
};

Task_t tlShockTrooperWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 1 }
};
Schedule_t slShockTrooperWaitInCover[] =
{
	{ tlShockTrooperWaitInCover, ARRAYSIZE( tlShockTrooperWaitInCover ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 |
		bits_COND_CAN_MELEE_ATTACK1 | bits_COND_CAN_MELEE_ATTACK2, bits_SOUND_DANGER, "GruntWaitInCover" }
};

Task_t tlShockTrooperTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_SHOCKTROOPER_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f }, { TASK_FIND_COVER_FROM_ENEMY, 0 }, { TASK_SHOCKTROOPER_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY }
};
Schedule_t slShockTrooperTakeCover[] =
{
	{ tlShockTrooperTakeCover1, ARRAYSIZE( tlShockTrooperTakeCover1 ), 0, 0, "TakeCover" }
};

Task_t tlShockTrooperGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_ENEMY, 99 }, { TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 }, { TASK_CLEAR_MOVE_WAIT, 0 }, { TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_SET_SCHEDULE, SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY }
};
Schedule_t slShockTrooperGrenadeCover[] =
{
	{ tlShockTrooperGrenadeCover1, ARRAYSIZE( tlShockTrooperGrenadeCover1 ), 0, 0, "GrenadeCover" }
};

Task_t tlShockTrooperTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 }, { TASK_RANGE_ATTACK2, 0 }, { TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};
Schedule_t slShockTrooperTossGrenadeCover[] =
{
	{ tlShockTrooperTossGrenadeCover1, ARRAYSIZE( tlShockTrooperTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" }
};

Task_t tlShockTrooperTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER }, { TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_TURN_LEFT, 179 }
};
Schedule_t slShockTrooperTakeCoverFromBestSound[] =
{
	{ tlShockTrooperTakeCoverFromBestSound, ARRAYSIZE( tlShockTrooperTakeCoverFromBestSound ),
		0, 0, "GruntTakeCoverFromBestSound" }
};

Task_t tlShockTrooperSweep[] =
{
	{ TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }, { TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }
};
Schedule_t slShockTrooperSweep[] =
{
	{ tlShockTrooperSweep, ARRAYSIZE( tlShockTrooperSweep ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "Grunt Sweep" }
};

Task_t tlShockTrooperRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slShockTrooperRangeAttack1A[] =
{
	{ tlShockTrooperRangeAttack1A, ARRAYSIZE( tlShockTrooperRangeAttack1A ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_NO_AMMO_LOADED | bits_COND_SHOCKTROOPER_NOFIRE,
		bits_SOUND_DANGER, "Range Attack1A" }
};

Task_t tlShockTrooperRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_IDLE_ANGRY },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_SHOCKTROOPER_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slShockTrooperRangeAttack1B[] =
{
	{ tlShockTrooperRangeAttack1B, ARRAYSIZE( tlShockTrooperRangeAttack1B ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_NO_AMMO_LOADED | bits_COND_SHOCKTROOPER_NOFIRE,
		bits_SOUND_DANGER, "Range Attack1B" }
};

Task_t tlShockTrooperRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SHOCKTROOPER_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 }, { TASK_SET_SCHEDULE, SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY }
};
Schedule_t slShockTrooperRangeAttack2[] =
{
	{ tlShockTrooperRangeAttack2, ARRAYSIZE( tlShockTrooperRangeAttack2 ), 0, 0, "RangeAttack2" }
};

Task_t tlShockTrooperRepel[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE, ACT_GLIDE }
};
Schedule_t slShockTrooperRepel[] =
{
	{ tlShockTrooperRepel, ARRAYSIZE( tlShockTrooperRepel ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel" }
};

Task_t tlShockTrooperRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_FLY }
};
Schedule_t slShockTrooperRepelAttack[] =
{
	{ tlShockTrooperRepelAttack, ARRAYSIZE( tlShockTrooperRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" }
};

Task_t tlShockTrooperRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE, ACT_LAND }, { TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_CLEAR_LASTPOSITION, 0 }
};
Schedule_t slShockTrooperRepelLand[] =
{
	{ tlShockTrooperRepelLand, ARRAYSIZE( tlShockTrooperRepelLand ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel Land" }
};

DEFINE_CUSTOM_SCHEDULES( CShockTrooper )
{
	slShockTrooperFail, slShockTrooperCombatFail, slShockTrooperVictoryDance, slShockTrooperEstablishLineOfFire,
	slShockTrooperFoundEnemy, slShockTrooperCombatFace, slShockTrooperSignalSuppress, slShockTrooperSuppress,
	slShockTrooperWaitInCover, slShockTrooperTakeCover, slShockTrooperGrenadeCover, slShockTrooperTossGrenadeCover,
	slShockTrooperTakeCoverFromBestSound, slShockTrooperSweep, slShockTrooperRangeAttack1A, slShockTrooperRangeAttack1B,
	slShockTrooperRangeAttack2, slShockTrooperRepel, slShockTrooperRepelAttack, slShockTrooperRepelLand
};

IMPLEMENT_CUSTOM_SCHEDULES( CShockTrooper, CBaseMonster );

void CShockTrooper::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/strooper.mdl" );
	UTIL_SetSize( pev, Vector( -24, -24, 0 ), Vector( 24, 24, 72 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->effects = 0;
	pev->health = 2.5f * gSkillData.shocktrooperHealth;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.2f;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fFirstEncounter = TRUE;
	m_fEnemyEluded = FALSE;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = SHOCKTROOPER_SENT_NONE;
	pev->weapons = 3;
	m_cClipSize = (int)gSkillData.shocktrooperMaxCharge;
	m_cAmmoLoaded = m_cClipSize;
	m_flLastChargeTime = gpGlobals->time;
	m_flLastBlinkTime = gpGlobals->time;
	m_flLastBlinkInterval = gpGlobals->time;
	m_flLastShot = gpGlobals->time;
	CTalkMonster::g_talkWaitTime = 0;
	pev->skin = 0;
	MonsterInit();
}

void CShockTrooper::Precache()
{
	PRECACHE_MODEL( "models/strooper.mdl" );
	PRECACHE_MODEL( "models/strooper_gibs.mdl" );
	PRECACHE_SOUND( "weapons/shock_fire.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_attack.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die1.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die2.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die3.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_die4.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain1.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain2.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain3.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain4.wav" );
	PRECACHE_SOUND( "shocktrooper/shock_trooper_pain5.wav" );
	m_voicePitch = RANDOM_LONG( 0, 1 ) ? 109 + RANDOM_LONG( 0, 7 ) : 100;
	UTIL_PrecacheOther( "monster_shockroach" );
	iShockTrooperMuzzleFlash = PRECACHE_MODEL( "sprites/muzzle_shock.spr" );
}

int CShockTrooper::Classify()
{
	return 15;
}

int CShockTrooper::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

void CShockTrooper::SetYawSpeed()
{
	int speed;
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN:
		speed = 150;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		speed = 180;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		speed = 120;
		break;
	case ACT_FLY:
	case ACT_GLIDE:
		speed = 30;
		break;
	default:
		speed = 90;
		break;
	}
	pev->yaw_speed = (float)speed;
}

BOOL CShockTrooper::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

void CShockTrooper::CheckAmmo()
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

void CShockTrooper::MonsterThink()
{
	if ( gpGlobals->time - m_flLastChargeTime >= gSkillData.shocktrooperRechargeSpeed && m_cAmmoLoaded < m_cClipSize )
	{
		m_cAmmoLoaded++;
		m_flLastChargeTime = gpGlobals->time;
		ALERT( at_aiconsole, "Shocktrooper Reload: %d\n", m_cAmmoLoaded );
	}
	if ( gpGlobals->time - m_flLastBlinkTime >= 3 )
	{
		float elapsed = gpGlobals->time - m_flLastBlinkTime;
		if ( elapsed >= RANDOM_FLOAT( 3, 7 ) )
		{
			pev->skin = 1;
			m_flLastBlinkInterval = gpGlobals->time;
			m_flLastBlinkTime = gpGlobals->time;
		}
	}
	if ( pev->skin > 0 && gpGlobals->time - m_flLastBlinkInterval >= 0.1 )
	{
		pev->skin = pev->skin == 3 ? 0 : pev->skin + 1;
		m_flLastBlinkInterval = gpGlobals->time;
	}
	CBaseMonster::MonsterThink();
}

int CShockTrooper::IRelationship( CBaseEntity* target )
{
	if ( FClassnameIs( target->pev, "monster_alien_grunt" ) || FClassnameIs( target->pev, "monster_gargantua" ) )
		return R_NM;
	return CBaseMonster::IRelationship( target );
}

void CShockTrooper::TraceAttack( entvars_t* attacker, float damage, Vector direction,
		TraceResult* trace, int damageType )
{
	CBaseMonster::TraceAttack( attacker, damage, direction, trace, damageType );
}

int CShockTrooper::TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType )
{
	Forget( bits_MEMORY_INCOVER );
	return CSquadMonster::TakeDamage( inflictor, attacker, damage, damageType );
}

BOOL CShockTrooper::CheckMeleeAttack1( float dot, float distance )
{
	CBaseMonster* enemy = NULL;
	if ( m_hEnemy != NULL )
	{
		enemy = m_hEnemy->MyMonsterPointer();
		if ( !enemy )
			return FALSE;
	}
	return distance <= 64 && dot >= 0.7 && enemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		enemy->Classify() != CLASS_PLAYER_BIOWEAPON;
}

BOOL CShockTrooper::CheckRangeAttack1( float dot, float distance )
{
	if ( gpGlobals->time - m_flLastShot > 0.175 && !HasConditions( bits_COND_ENEMY_OCCLUDED ) &&
		distance <= 2048 && dot >= 0.5 && NoFriendlyFire() && !HasConditions( bits_COND_NO_AMMO_LOADED ) )
	{
		if ( !m_hEnemy->IsPlayer() && distance <= 64 )
			return FALSE;
		TraceResult trace;
		Vector start = GetGunPosition();
		UTIL_TraceLine( start, m_hEnemy->BodyTarget( start ), ignore_monsters, ignore_glass, ENT( pev ), &trace );
		return trace.flFraction == 1;
	}
	return FALSE;
}

BOOL CShockTrooper::CheckRangeAttack2( float dot, float distance )
{
	(void)dot;
	(void)distance;
	if ( !( pev->weapons & SHOCKTROOPER_HANDGRENADE ) )
		return FALSE;
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;
	if ( !( m_hEnemy->pev->flags & FL_ONGROUND ) && !m_hEnemy->pev->waterlevel && m_vecEnemyLKP.z > pev->absmax.z )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector target;
	if ( pev->weapons & SHOCKTROOPER_HANDGRENADE )
	{
		if ( RANDOM_LONG( 0, 1 ) )
			target = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			target = m_vecEnemyLKP;
	}
	else
	{
		target = m_vecEnemyLKP + ( m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin );
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			target = target + ( ( target - pev->origin ).Length() / gSkillData.shocktrooperGrenadeSpeed ) *
				m_hEnemy->pev->velocity;
	}
	if ( InSquad() && SquadMemberInRange( target, 256 ) )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( ( target - pev->origin ).Length2D() <= 256 )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector toss;
	if ( pev->weapons & SHOCKTROOPER_HANDGRENADE )
		toss = VecCheckToss( pev, GetGunPosition(), target, 0.5 );
	else
		toss = VecCheckThrow( pev, GetGunPosition(), target, gSkillData.shocktrooperGrenadeSpeed, 0.5 );
	if ( toss != g_vecZero )
	{
		m_vecTossVelocity = toss;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = gpGlobals->time;
		if ( !( pev->weapons & SHOCKTROOPER_HANDGRENADE ) )
			m_flNextGrenadeCheck = gpGlobals->time + 0.3f;
	}
	else
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	return m_fThrowGrenade;
}

Vector CShockTrooper::GetGunPosition()
{
	Vector origin, angles;
	UTIL_MakeVectors( pev->angles );
	GetAttachment( 0, origin, angles );
	return origin + gpGlobals->v_forward * 32;
}

void CShockTrooper::Shoot()
{
	if ( m_hEnemy == NULL || !( gpGlobals->time - m_flLastShot > 0.11 ) )
		return;
	Vector origin = GetGunPosition();
	Vector direction = ShootAtEnemy( origin );
	Vector angles = UTIL_VecToAngles( direction );
	angles.x = -angles.x;
	UTIL_MakeVectors( pev->angles );
	RANDOM_FLOAT( -40, 40 );
	RANDOM_FLOAT( 75, 200 );
	RANDOM_FLOAT( 40, 90 );
	CShockBeam* beam = CShockBeam::CreateShockBeam( origin, angles, this );
	beam->pev->velocity = direction * 2000;
	beam->pev->speed = 2000;
	m_cAmmoLoaded--;
	SetBlending( 0, angles.x );
	m_flLastShot = gpGlobals->time;
}

CBaseEntity* CShockTrooper::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector start = pev->origin;
	start.z += pev->size.z * 0.5f;
	TraceResult trace;
	UTIL_TraceHull( start, start + gpGlobals->v_forward * 70, dont_ignore_monsters, head_hull, ENT( pev ), &trace );
	return trace.pHit ? CBaseEntity::Instance( trace.pHit ) : NULL;
}

void CShockTrooper::HandleAnimEvent( MonsterEvent_t* event )
{
	switch ( event->event )
	{
	case 3:
	{
		CBaseEntity* enemy = Kick();
		if ( enemy )
		{
			UTIL_MakeVectors( pev->angles );
			enemy->pev->punchangle.x = 15;
			enemy->pev->velocity = enemy->pev->velocity + gpGlobals->v_forward * 100 + gpGlobals->v_up * 50;
			enemy->TakeDamage( pev, pev, gSkillData.shocktrooperDmgKick, DMG_CLUB );
		}
		break;
	}
	case 4:
	{
		Vector origin, angles;
		GetAttachment( 0, origin, angles );
		MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, origin );
			WRITE_BYTE( TE_SPRITE );
			WRITE_COORD( origin.x );
			WRITE_COORD( origin.y );
			WRITE_COORD( origin.z );
			WRITE_SHORT( iShockTrooperMuzzleFlash );
			WRITE_BYTE( 4 );
			WRITE_BYTE( 128 );
		MESSAGE_END();
		Shoot();
		EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "weapons/shock_fire.wav", 1, ATTN_NORM );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );
		break;
	}
	case 7:
		UTIL_MakeVectors( pev->angles );
		CSpore::CreateSpore( pev->origin + Vector( 0, 0, 98 ), m_vecTossVelocity, this, 2, TRUE, FALSE );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case 10:
		if ( FOkToSpeak() )
		{
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_ALERT", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
		CBaseMonster::HandleAnimEvent( event );
		break;
	case 11:
		if ( pev->body != 1 )
		{
			Vector origin, angles;
			GetAttachment( 0, origin, angles );
			pev->body = 1;
			angles = Vector( 0, angles.y, 0 );
			CBaseEntity* roach = DropItem( "monster_shockroach", pev->origin + Vector( 0, 0, 48 ), angles );
			if ( roach )
			{
				float z = RANDOM_FLOAT( 20, 30 );
				float y = RANDOM_FLOAT( -20, 20 );
				roach->pev->velocity = Vector( RANDOM_FLOAT( -20, 20 ), y, z );
				roach->pev->avelocity = Vector( 0, RANDOM_FLOAT( 20, 40 ), 0 );
			}
		}
		break;
	default:
		CBaseMonster::HandleAnimEvent( event );
		break;
	}
}

BOOL CShockTrooper::FOkToSpeak()
{
	return CTalkMonster::g_talkWaitTime < gpGlobals->time &&
		( !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT );
}

void CShockTrooper::JustSpoke()
{
	float time = gpGlobals->time;
	CTalkMonster::g_talkWaitTime = RANDOM_FLOAT( 1.5, 2 ) + time;
	m_iSentence = SHOCKTROOPER_SENT_NONE;
}

void CShockTrooper::SpeakSentence()
{
	if ( m_iSentence == SHOCKTROOPER_SENT_NONE || !FOkToSpeak() )
		return;
	SENTENCEG_PlayRndSz( ENT( pev ), pGruntSentences[m_iSentence], SHOCKTROOPER_SENTENCE_VOLUME,
		ATTN_NORM, 0, m_voicePitch );
	JustSpoke();
}

void CShockTrooper::IdleSound()
{
	if ( !FOkToSpeak() || ( !g_fShockTrooperQuestion && !RANDOM_LONG( 0, 1 ) ) )
		return;
	if ( g_fShockTrooperQuestion )
	{
		if ( g_fShockTrooperQuestion == 1 )
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_CLEAR", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		else if ( g_fShockTrooperQuestion == 2 )
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_ANSWER", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		g_fShockTrooperQuestion = 0;
	}
	else
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_CHECK", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fShockTrooperQuestion = 1;
			break;
		case 1:
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_QUEST", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fShockTrooperQuestion = 2;
			break;
		case 2:
			SENTENCEG_PlayRndSz( ENT( pev ), "ST_IDLE", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	JustSpoke();
}

void CShockTrooper::PainSound()
{
	if ( !( gpGlobals->time > m_flNextPainTime ) )
		return;
	switch ( RANDOM_LONG( 0, 6 ) )
	{
	case 0:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "ShockTrooper/shock_trooper_pain3.wav", 1, ATTN_NORM );
		break;
	case 1:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "ShockTrooper/shock_trooper_pain4.wav", 1, ATTN_NORM );
		break;
	case 2:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "ShockTrooper/shock_trooper_pain5.wav", 1, ATTN_NORM );
		break;
	case 3:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "ShockTrooper/shock_trooper_pain1.wav", 1, ATTN_NORM );
		break;
	case 4:
		EMIT_SOUND( ENT( pev ), CHAN_VOICE, "ShockTrooper/shock_trooper_pain2.wav", 1, ATTN_NORM );
		break;
	}
	m_flNextPainTime = gpGlobals->time + 1;
}

void CShockTrooper::PrescheduleThink()
{
	if ( !InSquad() || m_hEnemy == NULL )
		return;
	if ( HasConditions( bits_COND_SEE_ENEMY ) )
		MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
	else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
		MySquadLeader()->m_fEnemyEluded = TRUE;
}

void CShockTrooper::StartTask( Task_t* task )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( task->iTask )
	{
	case TASK_SHOCKTROOPER_FACE_TOSS_DIR:
		break;
	case TASK_SHOCKTROOPER_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_SHOCKTROOPER_NOFIRE );
		TaskComplete();
		break;
	case TASK_SHOCKTROOPER_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_RUN_PATH:
	case TASK_WALK_PATH:
		Forget( bits_MEMORY_INCOVER );
		CBaseMonster::StartTask( task );
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		CBaseMonster::StartTask( task );
		if ( pev->movetype == MOVETYPE_FLY )
			m_IdealActivity = ACT_GLIDE;
		break;
	default:
		CBaseMonster::StartTask( task );
		break;
	}
}

void CShockTrooper::RunTask( Task_t* task )
{
	if ( task->iTask == TASK_SHOCKTROOPER_FACE_TOSS_DIR )
	{
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( (int)pev->yaw_speed );
		if ( FacingIdeal() )
			m_iTaskStatus = TASKSTATUS_COMPLETE;
	}
	else
		CBaseMonster::RunTask( task );
}

void CShockTrooper::SetActivity( Activity activity )
{
	GET_MODEL_PTR( ENT( pev ) );
	int sequence;
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		sequence = LookupSequence( m_fStanding ? "standing_mp5" : "crouching_mp5" );
		break;
	case ACT_RANGE_ATTACK2:
		sequence = LookupSequence( "throwgrenade" );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health > 20 ? ACT_RUN : ACT_RUN_HURT );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health > 20 ? ACT_WALK : ACT_WALK_HURT );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			activity = ACT_IDLE_ANGRY;
		sequence = LookupActivity( activity );
		break;
	default:
		sequence = LookupActivity( activity );
		break;
	}
	m_Activity = activity;
	if ( sequence < 0 )
	{
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), activity );
		pev->sequence = 0;
		return;
	}
	if ( pev->sequence != sequence || !m_fSequenceLoops )
		pev->frame = 0;
	pev->sequence = sequence;
	ResetSequenceInfo();
	SetYawSpeed();
}

void CShockTrooper::GibMonster()
{
	if ( !pev )
		return;
	if ( pev->body != 1 )
	{
		Vector origin, angles;
		GetAttachment( 0, origin, angles );
		angles = Vector( 0, angles.y, 0 );
		CBaseEntity* roach = DropItem( "monster_shockroach", pev->origin + Vector( 0, 0, 32 ), angles );
		if ( roach )
		{
			float z = RANDOM_FLOAT( 200, 300 );
			float y = RANDOM_FLOAT( -100, 100 );
			roach->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), y, z );
			roach->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}
	EMIT_SOUND( ENT( pev ), CHAN_WEAPON, "common/bodysplat.wav", 1, ATTN_NORM );
	if ( CVAR_GET_FLOAT( "violence_agibs" ) )
	{
		for ( int i = 0; i < 6; i++ )
		{
			CGib* gib = GetClassPtr( (CGib*)NULL );
			if ( g_Language == LANGUAGE_GERMAN )
			{
				gib->Spawn( "models/germangibs.mdl" );
				gib->pev->body = RANDOM_LONG( 0, 3 );
			}
			else
			{
				gib->Spawn( "models/strooper_gibs.mdl" );
				gib->pev->body = RANDOM_LONG( 0, 7 );
			}
			if ( pev )
			{
				gib->pev->origin.x = pev->absmin.x + pev->size.x * RANDOM_FLOAT( 0, 1 );
				gib->pev->origin.y = pev->absmin.y + pev->size.y * RANDOM_FLOAT( 0, 1 );
				gib->pev->origin.z = pev->absmin.z + pev->size.z * RANDOM_FLOAT( 0, 1 ) + 1;
				gib->pev->velocity = g_vecAttackDir * -1;
				gib->pev->velocity.x += RANDOM_FLOAT( -0.25, 0.25 );
				gib->pev->velocity.y += RANDOM_FLOAT( -0.25, 0.25 );
				gib->pev->velocity.z += RANDOM_FLOAT( -0.25, 0.25 );
				gib->pev->velocity = gib->pev->velocity * RANDOM_FLOAT( 300, 400 );
				gib->pev->avelocity.x = RANDOM_FLOAT( 100, 200 );
				gib->pev->avelocity.y = RANDOM_FLOAT( 100, 300 );
				gib->m_bloodColor = BloodColor();
				if ( pev->health > -50 )
					gib->pev->velocity = gib->pev->velocity * 0.7f;
				else if ( pev->health > -200 )
					gib->pev->velocity = gib->pev->velocity * 2;
				else
					gib->pev->velocity = gib->pev->velocity * 4;
				gib->pev->solid = SOLID_BBOX;
				UTIL_SetSize( gib->pev, g_vecZero, g_vecZero );
			}
			gib->LimitVelocity();
		}
	}
	SetThink( &CBaseEntity::SUB_Remove );
	if ( pev )
		pev->nextthink = gpGlobals->time;
}

Schedule_t* CShockTrooper::GetSchedule()
{
	m_iSentence = SHOCKTROOPER_SENT_NONE;
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if ( pev->flags & FL_ONGROUND )
		{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_SHOCKTROOPER_REPEL_LAND );
		}
		return GetScheduleOfType( m_MonsterState == MONSTERSTATE_COMBAT ?
			SCHED_SHOCKTROOPER_REPEL_ATTACK : SCHED_SHOCKTROOPER_REPEL );
	}
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* sound = PBestSound();
		if ( sound && ( sound->m_iType & bits_SOUND_DANGER ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( ENT( pev ), "ST_GREN", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}
	if ( m_MonsterState != MONSTERSTATE_COMBAT || HasConditions( bits_COND_ENEMY_DEAD ) )
		return CBaseMonster::GetSchedule();
	if ( HasConditions( bits_COND_NEW_ENEMY ) && InSquad() )
	{
		MySquadLeader()->m_fEnemyEluded = FALSE;
		if ( !IsLeader() )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		if ( FOkToSpeak() )
		{
			if ( m_hEnemy != NULL && m_hEnemy->IsPlayer() )
				SENTENCEG_PlayRndSz( ENT( pev ), "ST_ALERT", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			else if ( m_hEnemy != NULL && m_hEnemy->Classify() != CLASS_PLAYER_ALLY &&
				m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE && m_hEnemy->Classify() != CLASS_MACHINE )
				SENTENCEG_PlayRndSz( ENT( pev ), "ST_MONST", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
		return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
			SCHED_SHOCKTROOPER_SUPPRESS : SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
	}
	if ( !HasConditions( bits_COND_NEW_ENEMY ) )
	{
		if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
		{
			if ( m_cAmmoLoaded >= m_cClipSize )
				ClearConditions( bits_COND_NO_AMMO_LOADED );
			if ( HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) && OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		if ( HasConditions( bits_COND_SHOCKTROOPER_NOFIRE ) )
			return GetScheduleOfType( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
		if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
		{
			if ( RANDOM_LONG( 0, 99 ) > 90 || m_hEnemy == NULL )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			if ( FOkToSpeak() )
				m_iSentence = SHOCKTROOPER_SENT_COVER;
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return GetScheduleOfType( SCHED_SHOCKTROOPER_FOUND_ENEMY );
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
					SENTENCEG_PlayRndSz( ENT( pev ), "ST_THROW", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				if ( FOkToSpeak() )
					m_iSentence = SHOCKTROOPER_SENT_CHARGE;
				return GetScheduleOfType( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
			}
			if ( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
			{
				SENTENCEG_PlayRndSz( ENT( pev ), "ST_TAUNT", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_STANDOFF );
		}
	}
	if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE );
	return CBaseMonster::GetSchedule();
}

Schedule_t* CShockTrooper::GetScheduleOfType( int type )
{
	switch ( type )
	{
	case SCHED_COMBAT_FACE:
		return slShockTrooperCombatFace;
	case SCHED_VICTORY_DANCE:
		return InSquad() && !IsLeader() ? slShockTrooperFail : slShockTrooperVictoryDance;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
				OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			{
				if ( FOkToSpeak() )
				{
					SENTENCEG_PlayRndSz( ENT( pev ), "ST_THROW", SHOCKTROOPER_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return slShockTrooperTossGrenadeCover;
			}
		}
		else if ( !RANDOM_LONG( 0, 1 ) )
			return slShockTrooperGrenadeCover;
		return slShockTrooperTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slShockTrooperTakeCoverFromBestSound;
	case SCHED_RANGE_ATTACK1:
		if ( !RANDOM_LONG( 0, 9 ) )
			m_fStanding = RANDOM_LONG( 0, 1 );
		return m_fStanding ? slShockTrooperRangeAttack1B : slShockTrooperRangeAttack1A;
	case SCHED_RANGE_ATTACK2:
		return slShockTrooperRangeAttack2;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slShockTrooperCombatFail : slShockTrooperFail;
	case SCHED_SHOCKTROOPER_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slShockTrooperSignalSuppress;
		}
		return slShockTrooperSuppress;
	case SCHED_SHOCKTROOPER_ESTABLISH_LINE_OF_FIRE:
		return slShockTrooperEstablishLineOfFire;
	case SCHED_SHOCKTROOPER_SWEEP:
		return slShockTrooperSweep;
	case SCHED_SHOCKTROOPER_FOUND_ENEMY:
		return slShockTrooperFoundEnemy;
	case SCHED_SHOCKTROOPER_REPEL:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slShockTrooperRepel;
	case SCHED_SHOCKTROOPER_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slShockTrooperRepelAttack;
	case SCHED_SHOCKTROOPER_REPEL_LAND:
		return slShockTrooperRepelLand;
	case SCHED_SHOCKTROOPER_WAIT_FACE_ENEMY:
		return slShockTrooperWaitInCover;
	case SCHED_SHOCKTROOPER_TAKECOVER_FAILED:
		return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) ?
			SCHED_RANGE_ATTACK1 : SCHED_FAIL );
	case SCHED_SHOCKTROOPER_ELOF_FAIL:
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	default:
		return CSquadMonster::GetScheduleOfType( type );
	}
}

class CShockTrooperRepel : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void EXPORT RepelUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value );
	int m_iSpriteTexture;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_shocktrooper_repel, CShockTrooperRepel );

void CShockTrooperRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &CShockTrooperRepel::RepelUse );
}

void CShockTrooperRepel::Precache()
{
	UTIL_PrecacheOther( "monster_human_grunt" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CShockTrooperRepel::RepelUse( CBaseEntity* activator, CBaseEntity* caller, USE_TYPE useType, float value )
{
	(void)activator;
	(void)caller;
	(void)useType;
	(void)value;
	TraceResult trace;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, ENT( pev ), &trace );
	CBaseMonster* trooper = CBaseEntity::Create( "monster_human_grunt", pev->origin, pev->angles )->MyMonsterPointer();
	trooper->pev->movetype = MOVETYPE_FLY;
	trooper->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	trooper->SetActivity( ACT_GLIDE );
	trooper->m_vecLastPosition = trace.vecEndPos;
	CBeam* beam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	beam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), trooper->entindex() );
	beam->SetFlags( BEAM_FSOLID );
	beam->SetColor( 255, 255, 255 );
	beam->SetThink( &CBaseEntity::SUB_Remove );
	beam->pev->nextthink = gpGlobals->time + -4096 * trace.flFraction / trooper->pev->velocity.z + 0.5f;
	UTIL_Remove( this );
}

class CDeadShockTrooper : public CBaseMonster
{
public:
	void Spawn();
	int Classify() { return CLASS_HUMAN_MILITARY; }
	void KeyValue( KeyValueData* data );
	int m_iPose;
	static const char* m_szPoses[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_ShockTrooper_dead, CDeadShockTrooper );

const char* CDeadShockTrooper::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadShockTrooper::KeyValue( KeyValueData* data )
{
	if ( FStrEq( data->szKeyName, "pose" ) )
	{
		m_iPose = atoi( data->szValue );
		data->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( data );
}

void CDeadShockTrooper::Spawn()
{
	PRECACHE_MODEL( "models/strooper.mdl" );
	SET_MODEL( ENT( pev ), "models/strooper.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead ShockTrooper with bad pose\n" );
	pev->health = 8;
	if ( (unsigned int)pev->body < 2 )
		pev->skin = 0;
	MonsterInitDead();
}
