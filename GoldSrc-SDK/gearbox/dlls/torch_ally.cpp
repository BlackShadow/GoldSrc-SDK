#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "squadtalkmonster.h"
#include "animation.h"
#include "defaultai.h"
#include "weapons.h"
#include "soundent.h"
#include "effects.h"
#include "customentity.h"
#include "explode.h"
#include "decals.h"

#define TORCH_EAGLE			1
#define TORCH_TORCH			2
#define TORCH_HANDGRENADE	4
#define TORCH_LIMP_HEALTH	20
#define TORCH_SENTENCE_VOLUME	0.35f
#define bits_COND_TORCH_NOFIRE	bits_COND_SPECIAL1

enum
{
	SCHED_TORCH_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_TORCH_ESTABLISH_LINE_OF_FIRE,
	SCHED_TORCH_COVER_AND_RELOAD,
	SCHED_TORCH_SWEEP,
	SCHED_TORCH_FOUND_ENEMY,
	SCHED_TORCH_REPEL,
	SCHED_TORCH_REPEL_ATTACK,
	SCHED_TORCH_REPEL_LAND,
	SCHED_TORCH_WAIT_FACE_ENEMY,
	SCHED_TORCH_TAKECOVER_FAILED,
	SCHED_TORCH_ELOF_FAIL,
};

enum
{
	TASK_TORCH_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_TORCH_SPEAK_SENTENCE,
	TASK_TORCH_CHECK_FIRE,
};

enum
{
	TORCH_SENT_NONE = -1,
	TORCH_SENT_GREN,
	TORCH_SENT_ALERT,
	TORCH_SENT_MONSTER,
	TORCH_SENT_COVER,
	TORCH_SENT_THROW,
	TORCH_SENT_CHARGE,
	TORCH_SENT_TAUNT,
} TORCH_SENTENCE_TYPES;

int g_fTorchAllyQuestion;

class COFTorchAlly : public COFSquadTalkMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	int ISoundMask();
	int ObjectCaps() { return COFSquadTalkMonster::ObjectCaps() | FCAP_IMPULSE_USE; }
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	BOOL FCanCheckAttacks();
	BOOL CheckMeleeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack1( float flDot, float flDist );
	BOOL CheckRangeAttack2( float flDot, float flDist );
	void CheckAmmo();
	void SetActivity( Activity NewActivity );
	void StartTask( Task_t* pTask );
	void RunTask( Task_t* pTask );
	void DeathSound();
	void PainSound();
	void IdleSound();
	void AlertSound();
	Vector GetGunPosition();
	void Shoot();
	void MonsterThink();
	void PrescheduleThink();
	void GibMonster();
	void SpeakSentence();
	int Save( CSave& save );
	int Restore( CRestore& restore );
	CBaseEntity* Kick();
	Schedule_t* GetSchedule();
	Schedule_t* GetScheduleOfType( int Type );
	MONSTERSTATE GetIdealState();
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType );
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void Killed( entvars_t* pevAttacker, int iGib );
	void DeclineFollowing();
	void TalkInit();
	BOOL FOkToSpeak();
	void JustSpoke();
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

	BOOL	m_fGunDrawn;
	float	m_painTime;
	float	m_checkAttackTime;
	BOOL	m_lastAttackCheck;
	float	m_flPlayerDamage;
	float	m_flNextGrenadeCheck;
	float	m_flNextPainTime;
	float	m_flLastEnemySightTime;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;
	int		m_cClipSize;
	BOOL	m_fUseTorch;
	EHANDLE	m_hNewTargetEnt;
	int		m_iBlackOrWhite;
	BOOL	m_fGunHolstered;
	BOOL	m_fTorchHolstered;
	BOOL	m_fTorchActive;
	CBeam*	m_pTorchBeam;
	float	m_flLastShot;
	int		m_iSentence;
	static const char* pTorchSentences[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_human_torch_ally, COFTorchAlly );

TYPEDESCRIPTION COFTorchAlly::m_SaveData[] =
{
	DEFINE_FIELD( COFTorchAlly, m_fGunDrawn, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_painTime, FIELD_TIME ),
	DEFINE_FIELD( COFTorchAlly, m_checkAttackTime, FIELD_TIME ),
	DEFINE_FIELD( COFTorchAlly, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( COFTorchAlly, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( COFTorchAlly, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( COFTorchAlly, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( COFTorchAlly, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( COFTorchAlly, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( COFTorchAlly, m_iBlackOrWhite, FIELD_INTEGER ),
	DEFINE_FIELD( COFTorchAlly, m_fUseTorch, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_fGunHolstered, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_fTorchHolstered, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_fTorchActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFTorchAlly, m_flLastShot, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( COFTorchAlly, COFSquadTalkMonster );

const char* COFTorchAlly::pTorchSentences[] =
{
	"FG_GREN", "FG_ALERT", "FG_MONSTER", "FG_COVER", "FG_THROW", "FG_CHARGE", "FG_TAUNT",
};

Task_t tlTorchAllyFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_WAIT_PVS, 0 },
};
Schedule_t slTorchAllyFail[] =
{
	{ tlTorchAllyFail, ARRAYSIZE( tlTorchAllyFail ), bits_COND_CAN_ATTACK, 0, "Grunt Fail" },
};
Task_t tlTorchAllyCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 }, { TASK_WAIT_PVS, 0 },
};
Schedule_t slTorchAllyCombatFail[] =
{
	{ tlTorchAllyCombatFail, ARRAYSIZE( tlTorchAllyCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Grunt Combat Fail" },
};
Task_t tlTorchAllyVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 1.5 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 }, { TASK_WALK_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
};
Schedule_t slTorchAllyVictoryDance[] =
{
	{ tlTorchAllyVictoryDance, ARRAYSIZE( tlTorchAllyVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "GruntVictoryDance" },
};
Task_t tlTorchAllyEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_TORCH_ELOF_FAIL }, { TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_TORCH_SPEAK_SENTENCE, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
};
Schedule_t slTorchAllyEstablishLineOfFire[] =
{
	{ tlTorchAllyEstablishLineOfFire, ARRAYSIZE( tlTorchAllyEstablishLineOfFire ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_ATTACK | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntEstablishLineOfFire" },
};
Task_t tlTorchAllyFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1 },
};
Schedule_t slTorchAllyFoundEnemy[] =
{
	{ tlTorchAllyFoundEnemy, ARRAYSIZE( tlTorchAllyFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntFoundEnemy" },
};
Task_t tlTorchAllyCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE }, { TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 }, { TASK_SET_SCHEDULE, (float)SCHED_TORCH_SWEEP },
};
Schedule_t slTorchAllyCombatFace[] =
{
	{ tlTorchAllyCombatFace1, ARRAYSIZE( tlTorchAllyCombatFace1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,
		0, "Combat Face" },
};

#define TORCH_FIRE_TASKS \
	{ TASK_FACE_ENEMY, 0 }, { TASK_TORCH_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }

Task_t tlTorchAllySignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2 },
	TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS,
};
Schedule_t slTorchAllySignalSuppress[] =
{
	{ tlTorchAllySignalSuppress, ARRAYSIZE( tlTorchAllySignalSuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_TORCH_NOFIRE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "SignalSuppress" },
};
Task_t tlTorchAllySuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS,
};
Schedule_t slTorchAllySuppress[] =
{
	{ tlTorchAllySuppress, ARRAYSIZE( tlTorchAllySuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_TORCH_NOFIRE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "Suppress" },
};
Task_t tlTorchAllyWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 1 },
};
Schedule_t slTorchAllyWaitInCover[] =
{
	{ tlTorchAllyWaitInCover, ARRAYSIZE( tlTorchAllyWaitInCover ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_CAN_ATTACK, bits_SOUND_DANGER, "GruntWaitInCover" },
};
Task_t tlTorchAllyTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, (float)SCHED_TORCH_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f }, { TASK_FIND_COVER_FROM_ENEMY, 0 }, { TASK_TORCH_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, (float)SCHED_TORCH_WAIT_FACE_ENEMY },
};
Schedule_t slTorchAllyTakeCover[] =
{
	{ tlTorchAllyTakeCover1, ARRAYSIZE( tlTorchAllyTakeCover1 ), 0, 0, "TakeCover" },
};
Task_t tlTorchAllyGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 }, { TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TORCH_WAIT_FACE_ENEMY },
};
Schedule_t slTorchAllyGrenadeCover[] =
{
	{ tlTorchAllyGrenadeCover1, ARRAYSIZE( tlTorchAllyGrenadeCover1 ), 0, 0, "GrenadeCover" },
};
Task_t tlTorchAllyTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 }, { TASK_RANGE_ATTACK2, 0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY },
};
Schedule_t slTorchAllyTossGrenadeCover[] =
{
	{ tlTorchAllyTossGrenadeCover1, ARRAYSIZE( tlTorchAllyTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" },
};
Task_t tlTorchAllyTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER }, { TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER }, { TASK_TURN_LEFT, 179 },
};
Schedule_t slTorchAllyTakeCoverFromBestSound[] =
{
	{ tlTorchAllyTakeCoverFromBestSound, ARRAYSIZE( tlTorchAllyTakeCoverFromBestSound ),
		0, 0, "GruntTakeCoverFromBestSound" },
};
Task_t tlTorchAllyHideReload[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER }, { TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};
Schedule_t slTorchAllyHideReload[] =
{
	{ tlTorchAllyHideReload, ARRAYSIZE( tlTorchAllyHideReload ),
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "GruntHideReload" },
};
Task_t tlTorchAllySweep[] =
{
	{ TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }, { TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 },
};
Schedule_t slTorchAllySweep[] =
{
	{ tlTorchAllySweep, ARRAYSIZE( tlTorchAllySweep ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "Grunt Sweep" },
};
Task_t tlTorchAllyRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_TORCH_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS,
};
Schedule_t slTorchAllyRangeAttack1A[] =
{
	{ tlTorchAllyRangeAttack1A, ARRAYSIZE( tlTorchAllyRangeAttack1A ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_TORCH_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1A" },
};
Task_t tlTorchAllyRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY },
	{ TASK_TORCH_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	TORCH_FIRE_TASKS, TORCH_FIRE_TASKS, TORCH_FIRE_TASKS,
};
Schedule_t slTorchAllyRangeAttack1B[] =
{
	{ tlTorchAllyRangeAttack1B, ARRAYSIZE( tlTorchAllyRangeAttack1B ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_TORCH_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1B" },
};
Task_t tlTorchAllyRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_TORCH_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TORCH_WAIT_FACE_ENEMY },
};
Schedule_t slTorchAllyRangeAttack2[] =
{
	{ tlTorchAllyRangeAttack2, ARRAYSIZE( tlTorchAllyRangeAttack2 ), 0, 0, "RangeAttack2" },
};
Task_t tlTorchAllyRepel[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE, (float)ACT_GLIDE },
};
Schedule_t slTorchAllyRepel[] =
{
	{ tlTorchAllyRepel, ARRAYSIZE( tlTorchAllyRepel ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel" },
};
Task_t tlTorchAllyRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, (float)ACT_FLY },
};
Schedule_t slTorchAllyRepelAttack[] =
{
	{ tlTorchAllyRepelAttack, ARRAYSIZE( tlTorchAllyRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" },
};
Task_t tlTorchAllyRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE, (float)ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 },
};
Schedule_t slTorchAllyRepelLand[] =
{
	{ tlTorchAllyRepelLand, ARRAYSIZE( tlTorchAllyRepelLand ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel Land" },
};
Task_t tlTorchAllyIdleStand[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_TLK_HEADRESET, 0 },
};
Schedule_t slTorchAllyIdleStand[] =
{
	{ tlTorchAllyIdleStand, ARRAYSIZE( tlTorchAllyIdleStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleStand" },
};
Task_t tlTorchAllyFaceTarget[] =
{
	{ TASK_STOP_MOVING, 1 }, { TASK_FACE_TARGET, 0 }, { TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};
Schedule_t slTorchAllyFaceTarget[] =
{
	{ tlTorchAllyFaceTarget, ARRAYSIZE( tlTorchAllyFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_PROVOKED, bits_SOUND_DANGER, "FaceTarget" },
};
Task_t tlTorchAllyFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MOVE_AWAY_FOLLOW }, { TASK_MOVE_TO_TARGET_RANGE, 256 },
};
Schedule_t slTorchAllyFollow[] =
{
	{ tlTorchAllyFollow, ARRAYSIZE( tlTorchAllyFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED, bits_SOUND_DANGER, "Follow" },
};

DEFINE_CUSTOM_SCHEDULES( COFTorchAlly )
{
	slTorchAllyFollow, slTorchAllyFaceTarget, slTorchAllyIdleStand,
	slTorchAllyFail, slTorchAllyCombatFail, slTorchAllyVictoryDance, slTorchAllyEstablishLineOfFire,
	slTorchAllyFoundEnemy, slTorchAllyCombatFace, slTorchAllySignalSuppress, slTorchAllySuppress,
	slTorchAllyWaitInCover, slTorchAllyTakeCover, slTorchAllyGrenadeCover, slTorchAllyTossGrenadeCover,
	slTorchAllyTakeCoverFromBestSound, slTorchAllyHideReload, slTorchAllySweep, slTorchAllyRangeAttack1A,
	slTorchAllyRangeAttack1B, slTorchAllyRangeAttack2, slTorchAllyRepel, slTorchAllyRepelAttack,
	slTorchAllyRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( COFTorchAlly, COFSquadTalkMonster );

int COFTorchAlly::Classify()
{
	return CLASS_HUMAN_FRIENDLY;
}

int COFTorchAlly::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER |
		bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE;
}

void COFTorchAlly::SetYawSpeed()
{
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN:
		pev->yaw_speed = 150;
		break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		pev->yaw_speed = 180;
		break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2:
		pev->yaw_speed = 120;
		break;
	case ACT_GLIDE:
	case ACT_FLY:
		pev->yaw_speed = 30;
		break;
	default:
		pev->yaw_speed = 90;
	}
}

void COFTorchAlly::TalkInit()
{
	COFAllyMonster::TalkInit();
	m_szGrp[TLK_ANSWER] = "FG_ANSWER";
	m_szGrp[TLK_QUESTION] = "FG_QUESTION";
	m_szGrp[TLK_IDLE] = "FG_IDLE";
	m_szGrp[TLK_STARE] = "FG_STARE";
	m_szGrp[TLK_USE] = "FG_OK";
	m_szGrp[TLK_UNUSE] = "FG_WAIT";
	m_szGrp[TLK_STOP] = "FG_STOP";
	m_szGrp[TLK_NOSHOOT] = "FG_SCARED";
	m_szGrp[TLK_HELLO] = "FG_HELLO";
	m_szGrp[TLK_PHELLO] = NULL;
	m_szGrp[TLK_PIDLE] = NULL;
	m_szGrp[TLK_PQUESTION] = "FG_PQUEST";
	m_szGrp[TLK_PLHURT1] = "!FG_CUREA";
	m_szGrp[TLK_PLHURT2] = "!FG_CUREB";
	m_szGrp[TLK_PLHURT3] = "!FG_CUREC";
	m_szGrp[TLK_SMELL] = "FG_SMELL";
	m_szGrp[TLK_WOUND] = "FG_WOUND";
	m_szGrp[TLK_MORTAL] = "FG_MORTAL";
	m_voicePitch = 100;
}

void COFTorchAlly::Precache()
{
	PRECACHE_MODEL( "models/hgrunt_torch.mdl" );
	PRECACHE_MODEL( "sprites/xbeam3.spr" );
	TalkInit();
	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );
	PRECACHE_SOUND( "fgrunt/death1.wav" );
	PRECACHE_SOUND( "fgrunt/death2.wav" );
	PRECACHE_SOUND( "fgrunt/death3.wav" );
	PRECACHE_SOUND( "fgrunt/death4.wav" );
	PRECACHE_SOUND( "fgrunt/death5.wav" );
	PRECACHE_SOUND( "fgrunt/death6.wav" );
	PRECACHE_SOUND( "fgrunt/pain1.wav" );
	PRECACHE_SOUND( "fgrunt/pain2.wav" );
	PRECACHE_SOUND( "fgrunt/pain3.wav" );
	PRECACHE_SOUND( "fgrunt/pain4.wav" );
	PRECACHE_SOUND( "fgrunt/pain5.wav" );
	PRECACHE_SOUND( "fgrunt/pain6.wav" );
	PRECACHE_SOUND( "weapons/desert_eagle_fire.wav" );
	PRECACHE_SOUND( "weapons/desert_eagle_reload.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	PRECACHE_SOUND( "fgrunt/torch_light.wav" );
	PRECACHE_SOUND( "fgrunt/torch_cut_loop.wav" );
	PRECACHE_SOUND( "fgrunt/medic.wav" );
	m_voicePitch = 95;
	COFAllyMonster::Precache();
}

void COFTorchAlly::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/hgrunt_torch.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.torchAllyHealth;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = -0.7;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = TORCH_SENT_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_HEAR | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	pev->body = 0;
	m_fGunDrawn = FALSE;
	m_fGunHolstered = FALSE;
	m_fTorchHolstered = TRUE;
	m_fTorchActive = FALSE;
	if ( !pev->weapons )
		pev->weapons = TORCH_EAGLE;
	if ( pev->weapons & TORCH_EAGLE )
	{
		pev->body = 0;
		m_cClipSize = 8;
	}
	else
	{
		pev->body = 1;
		m_cClipSize = 0;
		m_fGunHolstered = TRUE;
		m_fTorchHolstered = FALSE;
	}
	m_cAmmoLoaded = m_cClipSize;
	COFAllyMonster::g_talkWaitTime = 0;
	m_flLastShot = gpGlobals->time;
	pev->skin = 0;
	MonsterInit();
	SetUse( &COFSquadTalkMonster::FollowerUse );
}

void COFTorchAlly::DeclineFollowing()
{
	PlaySentence( "FG_POK", 2, VOL_NORM, ATTN_NORM );
}

MONSTERSTATE COFTorchAlly::GetIdealState()
{
	return COFSquadTalkMonster::GetIdealState();
}

BOOL COFTorchAlly::FOkToSpeak()
{
	return gpGlobals->time > COFAllyMonster::g_talkWaitTime &&
		( !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT );
}

void COFTorchAlly::JustSpoke()
{
	COFAllyMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.5, 2 );
	m_iSentence = TORCH_SENT_NONE;
}

void COFTorchAlly::SpeakSentence()
{
	if ( m_iSentence != TORCH_SENT_NONE && FOkToSpeak() )
	{
		SENTENCEG_PlayRndSz( edict(), pTorchSentences[m_iSentence], TORCH_SENTENCE_VOLUME,
			ATTN_NORM, 0, m_voicePitch );
		JustSpoke();
	}
}

void COFTorchAlly::AlertSound()
{
	if ( m_hEnemy != NULL && FOkToSpeak() )
		PlaySentence( "FG_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
}

void COFTorchAlly::PainSound()
{
	if ( gpGlobals->time <= m_flNextPainTime )
		return;
	switch ( RANDOM_LONG( 0, 7 ) )
	{
	case 0: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain3.wav", 1, ATTN_NORM ); break;
	case 1: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain4.wav", 1, ATTN_NORM ); break;
	case 2: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain5.wav", 1, ATTN_NORM ); break;
	case 3: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain1.wav", 1, ATTN_NORM ); break;
	case 4: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain2.wav", 1, ATTN_NORM ); break;
	case 5: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/pain6.wav", 1, ATTN_NORM ); break;
	}
	m_flNextPainTime = gpGlobals->time + 1;
}

void COFTorchAlly::DeathSound()
{
	switch ( RANDOM_LONG( 0, 5 ) )
	{
	case 0: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death1.wav", 1, ATTN_IDLE ); break;
	case 1: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death2.wav", 1, ATTN_IDLE ); break;
	case 2: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death3.wav", 1, ATTN_IDLE ); break;
	case 3: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death4.wav", 1, ATTN_IDLE ); break;
	case 4: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death5.wav", 1, ATTN_IDLE ); break;
	case 5: EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/gr_death6.wav", 1, ATTN_IDLE ); break;
	}
}

void COFTorchAlly::IdleSound()
{
	if ( !FOkToSpeak() || !( g_fTorchAllyQuestion || RANDOM_LONG( 0, 1 ) ) )
		return;
	if ( !g_fTorchAllyQuestion )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SENTENCEG_PlayRndSz( edict(), "FG_CHECK", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fTorchAllyQuestion = 1;
			break;
		case 1:
			SENTENCEG_PlayRndSz( edict(), "FG_QUEST", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fTorchAllyQuestion = 2;
			break;
		case 2:
			SENTENCEG_PlayRndSz( edict(), "FG_IDLE", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	else
	{
		if ( g_fTorchAllyQuestion == 1 )
			SENTENCEG_PlayRndSz( edict(), "FG_CLEAR", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		else if ( g_fTorchAllyQuestion == 2 )
			SENTENCEG_PlayRndSz( edict(), "FG_ANSWER", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		g_fTorchAllyQuestion = 0;
	}
	JustSpoke();
}

void COFTorchAlly::PrescheduleThink()
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

BOOL COFTorchAlly::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

void COFTorchAlly::CheckAmmo()
{
	if ( m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

Vector COFTorchAlly::GetGunPosition()
{
	return pev->origin + Vector( 0, 0, m_fStanding ? 60 : 48 );
}

#pragma warning( push )
#pragma warning( disable : 4701 4703 )
BOOL COFTorchAlly::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( m_hEnemy == NULL )
		return FALSE;
	CBaseMonster* pEnemy = m_hEnemy->MyMonsterPointer();
	if ( !pEnemy )
		return FALSE;
	return flDist <= 64 && flDot >= 0.7 && pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON;
}
#pragma warning( pop )

BOOL COFTorchAlly::CheckRangeAttack1( float flDot, float flDist )
{
	if ( m_fGunHolstered || flDist > 1024 || flDot < 0.5 )
		return FALSE;
	Vector vecStart = pev->origin + Vector( 0, 0, 55 );
	CBaseEntity* pEnemy = m_hEnemy;
	Vector vecEnd = pEnemy->BodyTarget( vecStart ) - pEnemy->pev->origin + m_vecEnemyLKP;
	TraceResult tr;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
	m_lastAttackCheck = tr.flFraction == 1.0 || ( tr.pHit && CBaseEntity::Instance( tr.pHit ) == pEnemy );
	return m_lastAttackCheck;
}

BOOL COFTorchAlly::CheckRangeAttack2( float flDot, float flDist )
{
	if ( m_fGunHolstered || !( pev->weapons & TORCH_HANDGRENADE ) )
		return FALSE;
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( gpGlobals->time < m_flNextGrenadeCheck )
		return m_fThrowGrenade;
	if ( !( m_hEnemy->pev->flags & FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 &&
		m_vecEnemyLKP.z > pev->absmax.z )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector vecTarget;
	if ( pev->weapons & TORCH_HANDGRENADE )
	{
		if ( RANDOM_LONG( 0, 1 ) )
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			vecTarget = m_vecEnemyLKP;
	}
	else
	{
		vecTarget = m_vecEnemyLKP + ( m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin );
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			vecTarget = vecTarget + ( ( vecTarget - pev->origin ).Length() / gSkillData.torchAllyGrenadeSpeed ) *
				m_hEnemy->pev->velocity;
	}
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
	Vector toss;
	if ( pev->weapons & TORCH_HANDGRENADE )
		toss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );
	else
		toss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.torchAllyGrenadeSpeed, 0.5 );
	if ( toss != g_vecZero )
	{
		m_vecTossVelocity = toss;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = ( pev->weapons & TORCH_HANDGRENADE ) ? gpGlobals->time : gpGlobals->time + 0.3;
	}
	else
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	return m_fThrowGrenade;
}

void COFTorchAlly::Shoot()
{
	if ( m_hEnemy == NULL || gpGlobals->time - m_flLastShot <= 0.11 )
		return;
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	UTIL_MakeVectors( pev->angles );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_2DEGREES, 1024, BULLET_PLAYER_357, 0 );
	int pitch = RANDOM_LONG( 0, 20 );
	if ( pitch <= 10 )
		pitch += 95;
	else
		pitch = PITCH_NORM;
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/desert_eagle_fire.wav", 1, ATTN_NORM, 0, pitch );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
	m_flLastShot = gpGlobals->time;
}

CBaseEntity* COFTorchAlly::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + gpGlobals->v_forward * 70;
	TraceResult tr;
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr );
	return tr.pHit ? CBaseEntity::Instance( tr.pHit ) : NULL;
}

void COFTorchAlly::MonsterThink()
{
	if ( m_fTorchActive && m_pTorchBeam )
	{
		Vector vecTorchPos;
		Vector vecTorchAng;
		Vector vecEndPos;
		Vector vecEndAng;
		GetAttachment( 2, vecTorchPos, vecTorchAng );
		GetAttachment( 3, vecEndPos, vecEndAng );
		Vector vecDir = ( vecEndPos - vecTorchPos ).Normalize();
		TraceResult tr;
		UTIL_TraceLine( vecTorchPos, vecTorchPos + vecDir * 4, ignore_monsters, edict(), &tr );
		if ( tr.flFraction != 1.0 )
		{
			m_pTorchBeam->pev->spawnflags &= ~SF_BEAM_SPARKSTART;
			m_pTorchBeam->pev->spawnflags |= ~SF_BEAM_SPARKEND;
			UTIL_DecalTrace( &tr, DECAL_GUNSHOT1 + RANDOM_LONG( 0, 4 ) );
			m_pTorchBeam->DoSparks( tr.vecEndPos, tr.vecEndPos );
		}
		m_pTorchBeam->SetBrightness( RANDOM_LONG( 192, 255 ) );
	}
	CBaseMonster::MonsterThink();
}

void COFTorchAlly::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 2:
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/desert_eagle_reload.wav", 1, ATTN_NORM );
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
				pHurt->TakeDamage( pev, pev, gSkillData.torchAllyDmgKick, DMG_CLUB );
			}
			}
		break;
	case 4:
		Shoot();
		break;
	case 7:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case 9:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 +
			gpGlobals->v_up * 6, g_vecZero, 3 );
		break;
	case 10:
		if ( FOkToSpeak() )
		{
			SENTENCEG_PlayRndSz( edict(), "FG_ALERT", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			JustSpoke();
		}
		COFAllyMonster::HandleAnimEvent( pEvent );
		break;
	case 11:
		if ( pev->body == 0 )
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			pev->body = 2;
			DropItem( "weapon_eagle", vecGunPos, vecGunAngles );
		}
		break;
	case 17:
		pev->body = 0;
		m_fGunHolstered = FALSE;
		m_fTorchHolstered = TRUE;
		break;
	case 18:
		pev->body = 1;
		m_fTorchHolstered = FALSE;
		m_fGunHolstered = TRUE;
		break;
	case 19:
		pev->body = 2;
		m_fTorchHolstered = TRUE;
		m_fGunHolstered = TRUE;
		break;
	case 20:
		m_fTorchActive = TRUE;
		m_pTorchBeam = CBeam::BeamCreate( "sprites/xbeam3.spr", 5 );
		if ( m_pTorchBeam )
		{
			Vector vecTorchPos;
			Vector vecTorchAng;
			GetAttachment( 2, vecTorchPos, vecTorchAng );
			m_pTorchBeam->EntsInit( entindex(), entindex() );
			m_pTorchBeam->SetStartAttachment( 4 );
			m_pTorchBeam->SetEndAttachment( 3 );
			m_pTorchBeam->SetColor( 0, 0, 255 );
			m_pTorchBeam->SetBrightness( 255 );
			m_pTorchBeam->SetWidth( 5 );
			m_pTorchBeam->SetFlags( BEAM_FSHADEIN );
			m_pTorchBeam->SetScrollRate( 20 );
			m_pTorchBeam->pev->spawnflags |= SF_BEAM_SPARKEND;
			m_pTorchBeam->DoSparks( vecTorchPos, vecTorchPos );
		}
		break;
	case 21:
		if ( m_pTorchBeam )
		{
			m_fTorchActive = FALSE;
			UTIL_Remove( m_pTorchBeam );
			m_pTorchBeam = NULL;
		}
		break;
	default:
		COFAllyMonster::HandleAnimEvent( pEvent );
	}
}

void COFTorchAlly::StartTask( Task_t* pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( pTask->iTask )
	{
	case TASK_TORCH_FACE_TOSS_DIR:
		break;
	case TASK_TORCH_SPEAK_SENTENCE:
		SpeakSentence();
		TaskComplete();
		break;
	case TASK_TORCH_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_TORCH_NOFIRE );
		TaskComplete();
		break;
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		Forget( bits_MEMORY_INCOVER );
		COFAllyMonster::StartTask( pTask );
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
		break;
	case TASK_MELEE_ATTACK2:
		m_IdealActivity = ACT_MELEE_ATTACK2;
		break;
	case TASK_FACE_IDEAL:
	case TASK_FACE_ENEMY:
		COFAllyMonster::StartTask( pTask );
		if ( pev->movetype == MOVETYPE_FLY )
			m_IdealActivity = ACT_GLIDE;
		break;
	default:
		COFAllyMonster::StartTask( pTask );
	}
}

void COFTorchAlly::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_TORCH_FACE_TOSS_DIR )
	{
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( pev->yaw_speed );
		if ( FacingIdeal() )
			TaskComplete();
	}
	else
	{
		COFAllyMonster::RunTask( pTask );
	}
}

void COFTorchAlly::SetActivity( Activity NewActivity )
{
	GET_MODEL_PTR( edict() );
	int sequence;
	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		sequence = LookupSequence( m_fStanding ? "standing_mp5" : "crouching_mp5" );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health <= TORCH_LIMP_HEALTH ? ACT_RUN_HURT : ACT_RUN );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health <= TORCH_LIMP_HEALTH ? ACT_WALK_HURT : ACT_WALK );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			NewActivity = ACT_IDLE_ANGRY;
		sequence = LookupActivity( NewActivity );
		break;
	default:
		CBaseMonster::SetActivity( NewActivity );
		return;
	}
	m_Activity = NewActivity;
	if ( sequence > ACTIVITY_NOT_AVAILABLE )
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

void COFTorchAlly::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
			flDamage *= 0.5;
		break;
	case 8:
		flDamage = pev->health;
		ExplosionCreate( ptr->vecEndPos, pev->angles, edict(), 100, TRUE );
		bitsDamageType = DMG_BLAST | DMG_ALWAYSGIB;
		break;
	case 11:
		if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_CLUB ) )
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

int COFTorchAlly::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	int result = COFSquadTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( !IsAlive() || pev->deadflag == DEAD_DYING )
		return result;
	Forget( bits_MEMORY_INCOVER );
	if ( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;
		if ( m_hEnemy == NULL )
		{
			Vector vecDir = pev->origin - pevAttacker->origin;
			vecDir.z = 0;
			vecDir = vecDir.Normalize();
			Vector angle = pevAttacker->v_angle;
			angle.x = 0;
			Vector forward;
			BOOL deliberate = FALSE;
			if ( gpGlobals->time - m_flLastHitByPlayer < 4 && m_iPlayerHits > 2 )
			{
				if ( m_afMemory & bits_MEMORY_SUSPICIOUS )
				{
					deliberate = TRUE;
				}
				else
				{
					UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
					deliberate = DotProduct( forward, vecDir ) > 0.96;
				}
			}
			if ( deliberate )
			{
				PlaySentence( "FG_MAD", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
			}
			else
			{
				PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
				if ( gpGlobals->time - m_flLastHitByPlayer < 4 )
					m_iPlayerHits++;
				else
					m_iPlayerHits = 0;
			}
		}
		else if ( !m_hEnemy->IsPlayer() && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}
	return result;
}

void COFTorchAlly::Killed( entvars_t* pevAttacker, int iGib )
{
	if ( m_hTargetEnt != NULL )
		( (COFSquadTalkMonster*)(CBaseEntity*)m_hTargetEnt )->m_hWaitMedic = NULL;
	if ( m_hWaitMedic != NULL )
	{
		if ( m_hWaitMedic->pev->deadflag != DEAD_NO )
			m_hWaitMedic = NULL;
		else
			( (COFSquadTalkMonster*)(CBaseEntity*)m_hWaitMedic )->HealMe( NULL );
	}
	SetUse( NULL );
	if ( m_fTorchActive )
	{
		m_fTorchActive = FALSE;
		UTIL_Remove( m_pTorchBeam );
		m_pTorchBeam = NULL;
	}
	COFSquadTalkMonster::Killed( pevAttacker, iGib );
}

void COFTorchAlly::GibMonster()
{
	if ( m_hWaitMedic != NULL )
	{
		if ( m_hWaitMedic->pev->deadflag != DEAD_NO )
			m_hWaitMedic = NULL;
		else
			( (COFSquadTalkMonster*)(CBaseEntity*)m_hWaitMedic )->HealMe( NULL );
	}
	if ( pev->body == 0 )
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		CBaseEntity* pGun = DropItem( "weapon_eagle", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
	}
	if ( m_fTorchActive )
	{
		m_fTorchActive = FALSE;
		UTIL_Remove( m_pTorchBeam );
		m_pTorchBeam = NULL;
	}
	CBaseMonster::GibMonster();
}

Schedule_t* COFTorchAlly::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_IDLE_STAND:
		{
			Schedule_t* schedule = COFSquadTalkMonster::GetScheduleOfType( Type );
			return schedule == slIdleStand ? slTorchAllyIdleStand : schedule;
		}
	case SCHED_TARGET_FACE:
		{
			Schedule_t* schedule = COFSquadTalkMonster::GetScheduleOfType( Type );
			return schedule == slIdleStand ? slTorchAllyFaceTarget : schedule;
		}
	case SCHED_TARGET_CHASE:
		return slTorchAllyFollow;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() && g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
			OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( edict(), "FG_THROW", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return slTorchAllyTossGrenadeCover;
		}
		return slTorchAllyTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slTorchAllyTakeCoverFromBestSound;
	case SCHED_TORCH_TAKECOVER_FAILED:
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		return GetScheduleOfType( SCHED_FAIL );
	case SCHED_TORCH_ELOF_FAIL:
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	case SCHED_TORCH_ESTABLISH_LINE_OF_FIRE:
		return slTorchAllyEstablishLineOfFire;
	case SCHED_RANGE_ATTACK1:
		if ( RANDOM_LONG( 0, 9 ) == 0 )
			m_fStanding = RANDOM_LONG( 0, 1 );
		return m_fStanding ? slTorchAllyRangeAttack1B : slTorchAllyRangeAttack1A;
	case SCHED_RANGE_ATTACK2:
		return slTorchAllyRangeAttack2;
	case SCHED_COMBAT_FACE:
		return slTorchAllyCombatFace;
	case SCHED_TORCH_WAIT_FACE_ENEMY:
		return slTorchAllyWaitInCover;
	case SCHED_TORCH_SWEEP:
		return slTorchAllySweep;
	case SCHED_TORCH_COVER_AND_RELOAD:
		return slTorchAllyHideReload;
	case SCHED_TORCH_FOUND_ENEMY:
		return slTorchAllyFoundEnemy;
	case SCHED_VICTORY_DANCE:
		return InSquad() && !IsLeader() ? slTorchAllyFail : slTorchAllyVictoryDance;
	case SCHED_TORCH_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slTorchAllySignalSuppress;
		}
		return slTorchAllySuppress;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slTorchAllyCombatFail : slTorchAllyFail;
	case SCHED_CANT_FOLLOW:
		return slTorchAllyFail;
	case SCHED_TORCH_REPEL:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slTorchAllyRepel;
	case SCHED_TORCH_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slTorchAllyRepelAttack;
	case SCHED_TORCH_REPEL_LAND:
		return slTorchAllyRepelLand;
	}
	return COFSquadTalkMonster::GetScheduleOfType( Type );
}

Schedule_t* COFTorchAlly::GetSchedule()
{
	m_iSentence = TORCH_SENT_NONE;
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if ( pev->flags & FL_ONGROUND )
		{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_TORCH_REPEL_LAND );
		}
		return GetScheduleOfType( m_MonsterState == MONSTERSTATE_COMBAT ? SCHED_TORCH_REPEL_ATTACK : SCHED_TORCH_REPEL );
	}
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( edict(), "FG_GREN", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
		}
	}
	if ( HasConditions( bits_COND_ENEMY_DEAD ) && FOkToSpeak() )
		PlaySentence( "FG_KILL", 4, VOL_NORM, ATTN_NORM );
	switch ( m_MonsterState )
	{
	case MONSTERSTATE_IDLE:
	case MONSTERSTATE_ALERT:
		if ( HasConditions( bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if ( m_hWaitMedic == NULL && gpGlobals->time > m_flMedicWaitTime && pev->health <= TORCH_LIMP_HEALTH )
		{
			COFSquadTalkMonster* pMedic = MySquadMedic();
			if ( !pMedic )
				pMedic = FindSquadMedic( 1024 );
			if ( pMedic && pMedic->pev->deadflag == DEAD_NO && pMedic->HealMe( this ) )
			{
				EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/medic.wav", 1, ATTN_NORM );
				JustSpoke();
				m_flMedicWaitTime = gpGlobals->time + 5;
			}
		}
		if ( m_hEnemy == NULL && IsFollowing() )
		{
			if ( m_hTargetEnt->IsAlive() )
				return GetScheduleOfType( HasConditions( bits_COND_CLIENT_PUSH ) ? SCHED_MOVE_AWAY_FOLLOW : SCHED_TARGET_FACE );
			StopFollowing( FALSE );
		}
		else
		{
			if ( HasConditions( bits_COND_CLIENT_PUSH ) )
				return GetScheduleOfType( SCHED_MOVE_AWAY );
			TrySmellTalk();
		}
		break;
	case MONSTERSTATE_COMBAT:
		if ( HasConditions( bits_COND_ENEMY_DEAD ) )
			break;
		if ( m_hWaitMedic != NULL )
		{
			if ( m_hWaitMedic->pev->deadflag != DEAD_NO )
				m_hWaitMedic = NULL;
			else
				( (COFSquadTalkMonster*)(CBaseEntity*)m_hWaitMedic )->HealMe( NULL );
			m_flMedicWaitTime = gpGlobals->time + 5;
		}
		if ( !m_fTorchHolstered )
			break;
		if ( HasAllConditions( bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE ) )
		{
			if ( !InSquad() )
				return GetScheduleOfType( SCHED_SMALL_FLINCH );
			MySquadLeader()->m_fEnemyEluded = FALSE;
			if ( !IsLeader() )
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			if ( FOkToSpeak() )
			{
				if ( m_hEnemy != NULL && m_hEnemy->IsPlayer() )
					SENTENCEG_PlayRndSz( edict(), "FG_ALERT", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				else if ( m_hEnemy != NULL && m_hEnemy->Classify() != CLASS_PLAYER_ALLY &&
					m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE && m_hEnemy->Classify() != CLASS_MACHINE )
					SENTENCEG_PlayRndSz( edict(), "FG_MONST", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
				SCHED_TORCH_SUPPRESS : SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
		}
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			return GetScheduleOfType( SCHED_TORCH_COVER_AND_RELOAD );
		if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
		{
			if ( RANDOM_LONG( 0, 99 ) <= 90 && m_hEnemy != NULL )
			{
				if ( FOkToSpeak() )
					m_iSentence = TORCH_SENT_COVER;
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
			{
				MySquadLeader()->m_fEnemyEluded = FALSE;
				return GetScheduleOfType( SCHED_TORCH_FOUND_ENEMY );
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
					SENTENCEG_PlayRndSz( edict(), "FG_THROW", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				if ( FOkToSpeak() )
					m_iSentence = TORCH_SENT_CHARGE;
				return GetScheduleOfType( SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
			}
			if ( !IsFollowing() )
			{
				if ( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
				{
					SENTENCEG_PlayRndSz( edict(), "FG_TAUNT", TORCH_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_STANDOFF );
			}
		}
		if ( !IsFollowing() && HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_TORCH_ESTABLISH_LINE_OF_FIRE );
		break;
	default:
		break;
	}
	return CBaseMonster::GetSchedule();
}

class COFTorchAllyRepel : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void EXPORT RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int	m_iSpriteTexture;
	int	m_iszUse;
	int	m_iszUnUse;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_torch_ally_repel, COFTorchAllyRepel );

void COFTorchAllyRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &COFTorchAllyRepel::RepelUse );
}

void COFTorchAllyRepel::Precache()
{
	UTIL_PrecacheOther( "monster_human_grunt_ally" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void COFTorchAllyRepel::KeyValue( KeyValueData* pkvd )
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

void COFTorchAllyRepel::RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, edict(), &tr );
	CBaseEntity* pEntity = Create( "monster_human_grunt_ally", pev->origin, pev->angles );
	COFSquadTalkMonster* pGrunt = pEntity->MySquadTalkMonsterPointer();
	pGrunt->pev->weapons = pev->weapons;
	pGrunt->pev->netname = pev->netname;
	pGrunt->pev->spawnflags |= pev->spawnflags & ( SF_SQUADMONSTER_LEADER | SF_MONSTER_PREDISASTER |
		SF_MONSTER_GAG | SF_MONSTER_HITMONSTERCLIP | SF_MONSTER_WAIT_TILL_SEEN | SF_MONSTER_WAIT_FOR_SCRIPT );
	pGrunt->m_iszUse = m_iszUse;
	pGrunt->m_iszUnUse = m_iszUnUse;
	pGrunt->Spawn();
	pGrunt->pev->movetype = MOVETYPE_FLY;
	pGrunt->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pGrunt->SetActivity( ACT_GLIDE );
	pGrunt->m_vecLastPosition = tr.vecEndPos;
	CBeam* pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), pGrunt->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096.0 * tr.flFraction / pGrunt->pev->velocity.z + 0.5;
	UTIL_Remove( this );
}
