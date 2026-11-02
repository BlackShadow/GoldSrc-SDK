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

#define HGRUNT_9MMAR			1
#define HGRUNT_HANDGRENADE	2
#define HGRUNT_GRENADELAUNCHER	4
#define HGRUNT_SHOTGUN		8
#define HGRUNT_SAW			16
#define HGRUNT_LIMP_HEALTH	20
#define HGRUNT_SENTENCE_VOLUME	0.35f
#define GRUNT_ATTN			ATTN_NORM
#define bits_COND_GRUNT_NOFIRE	bits_COND_SPECIAL1

enum
{
	SCHED_GRUNT_SUPPRESS = LAST_TALKMONSTER_SCHEDULE + 1,
	SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE,
	SCHED_GRUNT_COVER_AND_RELOAD,
	SCHED_GRUNT_SWEEP,
	SCHED_GRUNT_FOUND_ENEMY,
	SCHED_GRUNT_REPEL,
	SCHED_GRUNT_REPEL_ATTACK,
	SCHED_GRUNT_REPEL_LAND,
	SCHED_GRUNT_WAIT_FACE_ENEMY,
	SCHED_GRUNT_TAKECOVER_FAILED,
	SCHED_GRUNT_ELOF_FAIL,
};

enum
{
	TASK_GRUNT_FACE_TOSS_DIR = LAST_TALKMONSTER_TASK + 1,
	TASK_GRUNT_SPEAK_SENTENCE,
	TASK_GRUNT_CHECK_FIRE,
};

enum
{
	HGRUNT_SENT_NONE = -1,
	HGRUNT_SENT_GREN,
	HGRUNT_SENT_ALERT,
	HGRUNT_SENT_MONSTER,
	HGRUNT_SENT_COVER,
	HGRUNT_SENT_THROW,
	HGRUNT_SENT_CHARGE,
	HGRUNT_SENT_TAUNT,
} HGRUNT_ALLY_SENTENCE_TYPES;

int g_fGruntAllyQuestion;

class CHGruntAlly : public COFSquadTalkMonster
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
	void Shotgun();
	void ShootSaw();
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
	void KeyValue( KeyValueData* pkvd );
	void DeclineFollowing();
	void TalkInit();
	BOOL FOkToSpeak();
	void JustSpoke();
	CUSTOM_SCHEDULES;
	static TYPEDESCRIPTION m_SaveData[];

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
	int		m_iBrassShell;
	int		m_iShotgunShell;
	int		m_iSawShell;
	int		m_iSawLink;
	int		m_iSentence;
	int		m_iWeaponIdx;
	int		m_iGruntHead;
	int		m_iGruntTorso;
	static const char* pGruntSentences[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally, CHGruntAlly );

TYPEDESCRIPTION CHGruntAlly::m_SaveData[] =
{
	DEFINE_FIELD( CHGruntAlly, m_lastAttackCheck, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_flPlayerDamage, FIELD_FLOAT ),
	DEFINE_FIELD( CHGruntAlly, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CHGruntAlly, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CHGruntAlly, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CHGruntAlly, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CHGruntAlly, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_iWeaponIdx, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_iGruntHead, FIELD_INTEGER ),
	DEFINE_FIELD( CHGruntAlly, m_iGruntTorso, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( CHGruntAlly, COFSquadTalkMonster );

const char* CHGruntAlly::pGruntSentences[] =
{
	"FG_GREN", "FG_ALERT", "FG_MONSTER", "FG_COVER", "FG_THROW", "FG_CHARGE", "FG_TAUNT",
};

Task_t tlGruntAllyFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_WAIT_PVS, 0 },
};
Schedule_t slGruntAllyFail[] =
{
	{ tlGruntAllyFail, ARRAYSIZE( tlGruntAllyFail ), bits_COND_CAN_ATTACK, 0, "Grunt Fail" },
};
Task_t tlGruntAllyCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 2 },
	{ TASK_WAIT_PVS, 0 },
};
Schedule_t slGruntAllyCombatFail[] =
{
	{ tlGruntAllyCombatFail, ARRAYSIZE( tlGruntAllyCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Grunt Combat Fail" },
};
Task_t tlGruntAllyVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 },
	{ TASK_WALK_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_VICTORY_DANCE },
};
Schedule_t slGruntAllyVictoryDance[] =
{
	{ tlGruntAllyVictoryDance, ARRAYSIZE( tlGruntAllyVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "GruntVictoryDance" },
};
Task_t tlGruntAllyEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_GRUNT_ELOF_FAIL },
	{ TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
};
Schedule_t slGruntAllyEstablishLineOfFire[] =
{
	{ tlGruntAllyEstablishLineOfFire, ARRAYSIZE( tlGruntAllyEstablishLineOfFire ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_ATTACK | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntEstablishLineOfFire" },
};
Task_t tlGruntAllyFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL1 },
};
Schedule_t slGruntAllyFoundEnemy[] =
{
	{ tlGruntAllyFoundEnemy, ARRAYSIZE( tlGruntAllyFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "GruntFoundEnemy" },
};
Task_t tlGruntAllyCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_SWEEP },
};
Schedule_t slGruntAllyCombatFace[] =
{
	{ tlGruntAllyCombatFace1, ARRAYSIZE( tlGruntAllyCombatFace1 ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2,
		0, "Combat Face" },
};

#define GRUNT_ALLY_FIRE_TASKS \
	{ TASK_FACE_ENEMY, 0 }, { TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }

Task_t tlGruntAllySignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_SIGNAL2 },
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
};
Schedule_t slGruntAllySignalSuppress[] =
{
	{ tlGruntAllySignalSuppress, ARRAYSIZE( tlGruntAllySignalSuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "SignalSuppress" },
};
Task_t tlGruntAllySuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
};
Schedule_t slGruntAllySuppress[] =
{
	{ tlGruntAllySuppress, ARRAYSIZE( tlGruntAllySuppress ),
		bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED, bits_SOUND_DANGER, "Suppress" },
};
Task_t tlGruntAllyWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT_FACE_ENEMY, 1 },
};
Schedule_t slGruntAllyWaitInCover[] =
{
	{ tlGruntAllyWaitInCover, ARRAYSIZE( tlGruntAllyWaitInCover ),
		bits_COND_NEW_ENEMY | bits_COND_HEAR_SOUND | bits_COND_CAN_ATTACK, bits_SOUND_DANGER, "GruntWaitInCover" },
};
Task_t tlGruntAllyTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_GRUNT_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_GRUNT_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY },
};
Schedule_t slGruntAllyTakeCover[] =
{
	{ tlGruntAllyTakeCover1, ARRAYSIZE( tlGruntAllyTakeCover1 ), 0, 0, "TakeCover" },
};
Task_t tlGruntAllyGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY },
};
Schedule_t slGruntAllyGrenadeCover[] =
{
	{ tlGruntAllyGrenadeCover1, ARRAYSIZE( tlGruntAllyGrenadeCover1 ), 0, 0, "GrenadeCover" },
};
Task_t tlGruntAllyTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_RANGE_ATTACK2, 0 },
	{ TASK_SET_SCHEDULE, (float)SCHED_TAKE_COVER_FROM_ENEMY },
};
Schedule_t slGruntAllyTossGrenadeCover[] =
{
	{ tlGruntAllyTossGrenadeCover1, ARRAYSIZE( tlGruntAllyTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" },
};
Task_t tlGruntAllyTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_COWER },
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, 179 },
};
Schedule_t slGruntAllyTakeCoverFromBestSound[] =
{
	{ tlGruntAllyTakeCoverFromBestSound, ARRAYSIZE( tlGruntAllyTakeCoverFromBestSound ),
		0, 0, "GruntTakeCoverFromBestSound" },
};
Task_t tlGruntAllyHideReload[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_RELOAD },
	{ TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, (float)bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RELOAD },
};
Schedule_t slGruntAllyHideReload[] =
{
	{ tlGruntAllyHideReload, ARRAYSIZE( tlGruntAllyHideReload ),
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "GruntHideReload" },
};
Task_t tlGruntAllySweep[] =
{
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 },
	{ TASK_TURN_LEFT, 179 },
	{ TASK_WAIT, 1 },
};
Schedule_t slGruntAllySweep[] =
{
	{ tlGruntAllySweep, ARRAYSIZE( tlGruntAllySweep ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 |
		bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_WORLD | bits_SOUND_DANGER | bits_SOUND_PLAYER, "Grunt Sweep" },
};
Task_t tlGruntAllyRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_CROUCH },
	{ TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
};
Schedule_t slGruntAllyRangeAttack1A[] =
{
	{ tlGruntAllyRangeAttack1A, ARRAYSIZE( tlGruntAllyRangeAttack1A ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1A" },
};
Task_t tlGruntAllyRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE_FACE_ENEMY, (float)ACT_IDLE_ANGRY },
	{ TASK_GRUNT_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
	GRUNT_ALLY_FIRE_TASKS,
};
Schedule_t slGruntAllyRangeAttack1B[] =
{
	{ tlGruntAllyRangeAttack1B, ARRAYSIZE( tlGruntAllyRangeAttack1B ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED |
		bits_COND_HEAR_SOUND | bits_COND_GRUNT_NOFIRE | bits_COND_NO_AMMO_LOADED,
		bits_SOUND_DANGER, "Range Attack1B" },
};
Task_t tlGruntAllyRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_GRUNT_FACE_TOSS_DIR, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, (float)SCHED_GRUNT_WAIT_FACE_ENEMY },
};
Schedule_t slGruntAllyRangeAttack2[] =
{
	{ tlGruntAllyRangeAttack2, ARRAYSIZE( tlGruntAllyRangeAttack2 ), 0, 0, "RangeAttack2" },
};
Task_t tlGruntAllyRepel[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_IDEAL, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_GLIDE },
};
Schedule_t slGruntAllyRepel[] =
{
	{ tlGruntAllyRepel, ARRAYSIZE( tlGruntAllyRepel ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel" },
};
Task_t tlGruntAllyRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_FLY },
};
Schedule_t slGruntAllyRepelAttack[] =
{
	{ tlGruntAllyRepelAttack, ARRAYSIZE( tlGruntAllyRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" },
};
Task_t tlGruntAllyRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_PLAY_SEQUENCE, (float)ACT_LAND },
	{ TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 },
	{ TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_CLEAR_LASTPOSITION, 0 },
};
Schedule_t slGruntAllyRepelLand[] =
{
	{ tlGruntAllyRepelLand, ARRAYSIZE( tlGruntAllyRepelLand ),
		bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND, bits_SOUND_DANGER | bits_SOUND_COMBAT | bits_SOUND_PLAYER, "Repel Land" },
};
Task_t tlGruntAllyIdleStand[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_WAIT, 2 },
	{ TASK_TLK_HEADRESET, 0 },
};
Schedule_t slGruntAllyIdleStand[] =
{
	{ tlGruntAllyIdleStand, ARRAYSIZE( tlGruntAllyIdleStand ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_SMELL | bits_COND_PROVOKED,
		bits_SOUND_COMBAT | bits_SOUND_DANGER | bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE,
		"IdleStand" },
};
Task_t tlGruntAllyFaceTarget[] =
{
	{ TASK_STOP_MOVING, 1 },
	{ TASK_FACE_TARGET, 0 },
	{ TASK_SET_ACTIVITY, (float)ACT_IDLE },
	{ TASK_SET_SCHEDULE, (float)SCHED_TARGET_CHASE },
};
Schedule_t slGruntAllyFaceTarget[] =
{
	{ tlGruntAllyFaceTarget, ARRAYSIZE( tlGruntAllyFaceTarget ),
		bits_COND_CLIENT_PUSH | bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE |
		bits_COND_HEAR_SOUND | bits_COND_PROVOKED, bits_SOUND_DANGER, "FaceTarget" },
};
Task_t tlGruntAllyFollow[] =
{
	{ TASK_SET_FAIL_SCHEDULE, (float)SCHED_MOVE_AWAY_FOLLOW },
	{ TASK_MOVE_TO_TARGET_RANGE, 256 },
};
Schedule_t slGruntAllyFollow[] =
{
	{ tlGruntAllyFollow, ARRAYSIZE( tlGruntAllyFollow ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND |
		bits_COND_PROVOKED, bits_SOUND_DANGER, "Follow" },
};

DEFINE_CUSTOM_SCHEDULES( CHGruntAlly )
{
	slGruntAllyFollow, slGruntAllyFaceTarget, slGruntAllyIdleStand,
	slGruntAllyFail, slGruntAllyCombatFail, slGruntAllyVictoryDance, slGruntAllyEstablishLineOfFire,
	slGruntAllyFoundEnemy, slGruntAllyCombatFace, slGruntAllySignalSuppress, slGruntAllySuppress,
	slGruntAllyWaitInCover, slGruntAllyTakeCover, slGruntAllyGrenadeCover, slGruntAllyTossGrenadeCover,
	slGruntAllyTakeCoverFromBestSound, slGruntAllyHideReload, slGruntAllySweep, slGruntAllyRangeAttack1A,
	slGruntAllyRangeAttack1B, slGruntAllyRangeAttack2, slGruntAllyRepel, slGruntAllyRepelAttack,
	slGruntAllyRepelLand,
};

IMPLEMENT_CUSTOM_SCHEDULES( CHGruntAlly, COFSquadTalkMonster );

int CHGruntAlly::Classify()
{
	return CLASS_HUMAN_FRIENDLY;
}

int CHGruntAlly::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER |
		bits_SOUND_MEAT | bits_SOUND_CARCASS | bits_SOUND_GARBAGE;
}

void CHGruntAlly::SetYawSpeed()
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
	case ACT_GLIDE:
	case ACT_FLY:
		speed = 30;
		break;
	default:
		speed = 90;
	}
	pev->yaw_speed = speed;
}

void CHGruntAlly::TalkInit()
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

void CHGruntAlly::Precache()
{
	PRECACHE_MODEL( "models/hgrunt_opfor.mdl" );
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
	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "weapons/saw_fire1.wav" );
	PRECACHE_SOUND( "weapons/saw_fire2.wav" );
	PRECACHE_SOUND( "weapons/saw_fire3.wav" );
	PRECACHE_SOUND( "weapons/saw_reload.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "fgrunt/medic.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	m_voicePitch = 100;
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
	m_iShotgunShell = PRECACHE_MODEL( "models/shotgunshell.mdl" );
	COFAllyMonster::Precache();
}

void CHGruntAlly::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/hgrunt_opfor.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.hgruntAllyHealth;
	pev->view_ofs = Vector( 0, 0, 50 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = -0.7;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_iSentence = HGRUNT_SENT_NONE;
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_HEAR | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	pev->body = 0;
	m_iGruntTorso = 0;
	if ( pev->weapons & HGRUNT_9MMAR )
	{
		m_iWeaponIdx = 0;
		m_cClipSize = 36;
	}
	else if ( pev->weapons & HGRUNT_SHOTGUN )
	{
		m_iWeaponIdx = 1;
		m_iGruntTorso = 3;
		m_cClipSize = 8;
	}
	else if ( pev->weapons & HGRUNT_SAW )
	{
		m_iWeaponIdx = 2;
		m_iGruntTorso = 1;
		m_cClipSize = 36;
	}
	else
	{
		m_iWeaponIdx = 3;
		m_cClipSize = 0;
	}
	m_cAmmoLoaded = m_cClipSize;
	if ( m_iGruntHead == -1 )
	{
		if ( pev->spawnflags & SF_SQUADMONSTER_LEADER )
			m_iGruntHead = 1;
		else if ( m_iWeaponIdx == 1 )
			m_iGruntHead = 2;
		else if ( m_iWeaponIdx == 2 )
			m_iGruntHead = RANDOM_LONG( 0, 1 ) + 3;
		else if ( m_iWeaponIdx == 0 )
			m_iGruntHead = 0;
		else
			m_iGruntHead = 5;
	}
	pev->body = m_iGruntHead + 8 * ( m_iGruntTorso + 4 * m_iWeaponIdx );
	if ( m_iGruntHead == 2 || m_iGruntHead == 4 )
		m_voicePitch = 90;
	pev->skin = 0;
	m_iSawShell = PRECACHE_MODEL( "models/saw_shell.mdl" );
	m_iSawLink = PRECACHE_MODEL( "models/saw_link.mdl" );
	COFAllyMonster::g_talkWaitTime = 0;
	m_flMedicWaitTime = gpGlobals->time;
	MonsterInit();
	SetUse( &COFSquadTalkMonster::FollowerUse );
}

void CHGruntAlly::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iGruntHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		COFAllyMonster::KeyValue( pkvd );
	}
}

void CHGruntAlly::DeclineFollowing()
{
	PlaySentence( "FG_POK", 2, VOL_NORM, ATTN_NORM );
}

MONSTERSTATE CHGruntAlly::GetIdealState()
{
	return COFSquadTalkMonster::GetIdealState();
}

BOOL CHGruntAlly::FOkToSpeak()
{
	return gpGlobals->time > COFAllyMonster::g_talkWaitTime &&
		( !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT );
}

void CHGruntAlly::JustSpoke()
{
	COFAllyMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.5, 2 );
	m_iSentence = HGRUNT_SENT_NONE;
}

void CHGruntAlly::SpeakSentence()
{
	if ( m_iSentence != HGRUNT_SENT_NONE && FOkToSpeak() )
	{
		SENTENCEG_PlayRndSz( edict(), pGruntSentences[m_iSentence], HGRUNT_SENTENCE_VOLUME,
			GRUNT_ATTN, 0, m_voicePitch );
		JustSpoke();
	}
}

void CHGruntAlly::AlertSound()
{
	if ( m_hEnemy != NULL && FOkToSpeak() )
		PlaySentence( "FG_ATTACK", RANDOM_FLOAT( 2.8, 3.2 ), VOL_NORM, ATTN_IDLE );
}

void CHGruntAlly::PainSound()
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

void CHGruntAlly::DeathSound()
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

BOOL CHGruntAlly::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

void CHGruntAlly::CheckAmmo()
{
	if ( pev->weapons && m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

Vector CHGruntAlly::GetGunPosition()
{
	return pev->origin + Vector( 0, 0, m_fStanding ? 60 : 48 );
}

#pragma warning( push )
#pragma warning( disable : 4701 4703 )
BOOL CHGruntAlly::CheckMeleeAttack1( float flDot, float flDist )
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

BOOL CHGruntAlly::CheckRangeAttack1( float flDot, float flDist )
{
	if ( !pev->weapons || flDist > ( ( pev->weapons & HGRUNT_SHOTGUN ) ? 640 : 1024 ) || flDot < 0.5 )
		return FALSE;
	Vector vecStart = pev->origin + Vector( 0, 0, 55 );
	CBaseEntity* pEnemy = m_hEnemy;
	Vector vecEnd = pEnemy->BodyTarget( vecStart ) - pEnemy->pev->origin + m_vecEnemyLKP;
	TraceResult tr;
	UTIL_TraceLine( vecStart, vecEnd, dont_ignore_monsters, edict(), &tr );
	m_lastAttackCheck = tr.flFraction == 1.0 || ( tr.pHit && CBaseEntity::Instance( tr.pHit ) == pEnemy );
	return m_lastAttackCheck;
}

BOOL CHGruntAlly::CheckRangeAttack2( float flDot, float flDist )
{
	if ( !( pev->weapons & ( HGRUNT_HANDGRENADE | HGRUNT_GRENADELAUNCHER ) ) )
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
	if ( pev->weapons & HGRUNT_HANDGRENADE )
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
			vecTarget = vecTarget + ( ( vecTarget - pev->origin ).Length() / gSkillData.hgruntAllyGrenadeSpeed ) *
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
	if ( pev->weapons & HGRUNT_HANDGRENADE )
		toss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5 );
	else
		toss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.hgruntAllyGrenadeSpeed, 0.5 );
	if ( toss != g_vecZero )
	{
		m_vecTossVelocity = toss;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = ( pev->weapons & HGRUNT_HANDGRENADE ) ? gpGlobals->time : gpGlobals->time + 0.3;
	}
	else
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	return m_fThrowGrenade;
}

void CHGruntAlly::IdleSound()
{
	if ( !FOkToSpeak() || !( g_fGruntAllyQuestion || RANDOM_LONG( 0, 1 ) ) )
		return;
	if ( !g_fGruntAllyQuestion )
	{
		switch ( RANDOM_LONG( 0, 2 ) )
		{
		case 0:
			SENTENCEG_PlayRndSz( edict(), "FG_CHECK", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fGruntAllyQuestion = 1;
			break;
		case 1:
			SENTENCEG_PlayRndSz( edict(), "FG_QUEST", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			g_fGruntAllyQuestion = 2;
			break;
		case 2:
			SENTENCEG_PlayRndSz( edict(), "FG_IDLE", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
			break;
		}
	}
	else
	{
		if ( g_fGruntAllyQuestion == 1 )
			SENTENCEG_PlayRndSz( edict(), "FG_CLEAR", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		else if ( g_fGruntAllyQuestion == 2 )
			SENTENCEG_PlayRndSz( edict(), "FG_ANSWER", HGRUNT_SENTENCE_VOLUME, ATTN_NORM, 0, m_voicePitch );
		g_fGruntAllyQuestion = 0;
	}
	JustSpoke();
}

void CHGruntAlly::PrescheduleThink()
{
	if ( InSquad() && m_hEnemy != NULL )
	{
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
		else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
			MySquadLeader()->m_fEnemyEluded = TRUE;
	}
}

void CHGruntAlly::Shoot()
{
	if ( m_hEnemy == NULL )
		return;
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	UTIL_MakeVectors( pev->angles );
	Vector vecShellVelocity = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	vecShellVelocity = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + vecShellVelocity;
	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + vecShellVelocity;
	EjectBrass( vecShootOrigin + vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_MP5, 4 );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

void CHGruntAlly::Shotgun()
{
	if ( m_hEnemy == NULL )
		return;
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	UTIL_MakeVectors( pev->angles );
	Vector vecShellVelocity = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	vecShellVelocity = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 ) + vecShellVelocity;
	vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + vecShellVelocity;
	EjectBrass( vecShootOrigin + vecShootDir * 6, vecShellVelocity, pev->angles.y, m_iShotgunShell, TE_BOUNCE_SHOTSHELL );
	FireBullets( (unsigned long)gSkillData.hgruntAllyShotgunPellets, vecShootOrigin, vecShootDir,
		VECTOR_CONE_10DEGREES, 2048, BULLET_PLAYER_BUCKSHOT, 0 );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

void CHGruntAlly::ShootSaw()
{
	if ( m_hEnemy == NULL )
		return;
	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy( vecShootOrigin );
	UTIL_MakeVectors( pev->angles );
	Vector vecShellVelocity = gpGlobals->v_forward * 25;
	int shell;
	if ( RANDOM_LONG( 0, 1 ) )
	{
		vecShellVelocity = gpGlobals->v_up * RANDOM_FLOAT( 100, 150 ) + vecShellVelocity;
		vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 100, 250 ) + vecShellVelocity;
		shell = m_iSawShell;
	}
	else
	{
		vecShellVelocity = gpGlobals->v_up * RANDOM_FLOAT( 150, 200 ) + vecShellVelocity;
		vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 75, 200 ) + vecShellVelocity;
		shell = m_iSawLink;
	}
	EjectBrass( vecShootOrigin + vecShootDir * 6, vecShellVelocity, pev->angles.y, shell, TE_BOUNCE_SHELL );
	FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_5DEGREES, 8192, BULLET_PLAYER_556, 2 );
	switch ( RANDOM_LONG( 0, 2 ) )
	{
	case 0:
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/saw_fire1.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
		break;
	case 1:
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/saw_fire2.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
		break;
	case 2:
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/saw_fire3.wav", 1, ATTN_NORM, 0, 94 + RANDOM_LONG( 0, 15 ) );
		break;
	}
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecShootDir );
	SetBlending( 0, angDir.x );
}

CBaseEntity* CHGruntAlly::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + gpGlobals->v_forward * 70;
	TraceResult tr;
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, head_hull, edict(), &tr );
	return tr.pHit ? CBaseEntity::Instance( tr.pHit ) : NULL;
}

void CHGruntAlly::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 2:
		EMIT_SOUND( edict(), CHAN_WEAPON,
			( pev->weapons & HGRUNT_SAW ) ? "weapons/saw_reload.wav" : "hgrunt/gr_reload1.wav", 1, ATTN_NORM );
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
				pHurt->TakeDamage( pev, pev, gSkillData.hgruntAllyDmgKick, DMG_CLUB );
			}
		}
		break;
	case 4:
		if ( pev->weapons & HGRUNT_9MMAR )
		{
			Shoot();
			EMIT_SOUND( edict(), CHAN_WEAPON,
				RANDOM_LONG( 0, 1 ) ? "hgrunt/gr_mgun1.wav" : "hgrunt/gr_mgun2.wav", 1, ATTN_NORM );
		}
		else if ( pev->weapons & HGRUNT_SAW )
		{
			ShootSaw();
		}
		else
		{
			Shotgun();
			EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/sbarrel1.wav", 1, ATTN_NORM );
		}
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3 );
		break;
	case 5:
	case 6:
		if ( pev->weapons & HGRUNT_9MMAR )
			Shoot();
		else if ( pev->weapons & HGRUNT_SAW )
			ShootSaw();
		break;
	case 7:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5 );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case 8:
		EMIT_SOUND( edict(), CHAN_WEAPON, "weapons/glauncher.wav", 0.8, ATTN_NORM );
		CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + ( g_iSkillLevel == SKILL_HARD ? RANDOM_FLOAT( 2, 5 ) : 6 );
		break;
	case 9:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, pev->origin + gpGlobals->v_forward * 17 - gpGlobals->v_right * 27 +
			gpGlobals->v_up * 6, g_vecZero, 3 );
		break;
	case 10:
		if ( FOkToSpeak() )
		{
			SENTENCEG_PlayRndSz( edict(), "FG_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
			JustSpoke();
		}
		COFAllyMonster::HandleAnimEvent( pEvent );
		break;
	case 11:
		{
			Vector vecGunPos;
			Vector vecGunAngles;
			GetAttachment( 0, vecGunPos, vecGunAngles );
			pev->body = m_iGruntHead + 8 * m_iGruntTorso + 96;
			if ( pev->weapons & HGRUNT_SHOTGUN )
				DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
			else if ( pev->weapons & HGRUNT_SAW )
				DropItem( "weapon_m249", vecGunPos, vecGunAngles );
			else
				DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
			if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecGunAngles );
			m_iWeaponIdx = 3;
		}
		break;
	default:
		COFAllyMonster::HandleAnimEvent( pEvent );
	}
}

void CHGruntAlly::StartTask( Task_t* pTask )
{
	m_iTaskStatus = TASKSTATUS_RUNNING;
	switch ( pTask->iTask )
	{
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
	case TASK_WALK_PATH:
	case TASK_RUN_PATH:
		Forget( bits_MEMORY_INCOVER );
		COFAllyMonster::StartTask( pTask );
		break;
	case TASK_RELOAD:
		m_IdealActivity = ACT_RELOAD;
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

void CHGruntAlly::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_GRUNT_FACE_TOSS_DIR )
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

void CHGruntAlly::SetActivity( Activity NewActivity )
{
	GET_MODEL_PTR( edict() );
	int sequence;
	switch ( NewActivity )
	{
	case ACT_RANGE_ATTACK1:
		if ( pev->weapons & HGRUNT_SAW )
			sequence = LookupSequence( m_fStanding ? "standing_saw" : "crouching_saw" );
		else if ( pev->weapons & HGRUNT_9MMAR )
			sequence = LookupSequence( m_fStanding ? "standing_mp5" : "crouching_mp5" );
		else
			sequence = LookupSequence( m_fStanding ? "standing_shotgun" : "crouching_shotgun" );
		break;
	case ACT_RANGE_ATTACK2:
		sequence = LookupSequence( ( pev->weapons & HGRUNT_HANDGRENADE ) ? "throwgrenade" : "launchgrenade" );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health <= HGRUNT_LIMP_HEALTH ? ACT_RUN_HURT : ACT_RUN );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health <= HGRUNT_LIMP_HEALTH ? ACT_WALK_HURT : ACT_WALK );
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

static BOOL IsFacing( entvars_t* pevTest, const Vector& reference )
{
	Vector vecDir = reference - pevTest->origin;
	vecDir.z = 0;
	vecDir = vecDir.Normalize();
	Vector angle = pevTest->v_angle;
	angle.x = 0;
	Vector forward;
	UTIL_MakeVectorsPrivate( angle, forward, NULL, NULL );
	return DotProduct( forward, vecDir ) > 0.96;
}

int CHGruntAlly::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	int result = COFSquadTalkMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
	if ( pev->deadflag != DEAD_NO )
		return result;
	Forget( bits_MEMORY_INCOVER );
	if ( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
	{
		m_flPlayerDamage += flDamage;
		if ( m_hEnemy == NULL )
		{
			if ( gpGlobals->time - m_flLastHitByPlayer < 4 && m_iPlayerHits > 2 &&
				( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) ) )
			{
				PlaySentence( "FG_MAD", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_PROVOKED );
				StopFollowing( TRUE );
				ALERT( at_console, "HGrunt Ally is now MAD!\n" );
			}
			else
			{
				PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
				Remember( bits_MEMORY_SUSPICIOUS );
				if ( gpGlobals->time - m_flLastHitByPlayer < 4 )
					m_iPlayerHits++;
				else
					m_iPlayerHits = 0;
				m_flLastHitByPlayer = gpGlobals->time;
				ALERT( at_console, "HGrunt Ally is now SUSPICIOUS!\n" );
			}
		}
		else if ( !m_hEnemy->IsPlayer() && pev->deadflag == DEAD_NO )
		{
			PlaySentence( "FG_SHOT", 4, VOL_NORM, ATTN_NORM );
		}
	}
	return result;
}

void CHGruntAlly::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	switch ( ptr->iHitgroup )
	{
	case HITGROUP_CHEST:
	case HITGROUP_STOMACH:
		if ( bitsDamageType & ( DMG_BULLET | DMG_SLASH | DMG_BLAST ) )
			flDamage *= 0.5;
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

void CHGruntAlly::Killed( entvars_t* pevAttacker, int iGib )
{
	if ( m_MonsterState != MONSTERSTATE_DEAD &&
		( ( m_afMemory & bits_MEMORY_SUSPICIOUS ) || IsFacing( pevAttacker, pev->origin ) ) )
	{
		Remember( bits_MEMORY_PROVOKED );
		StopFollowing( TRUE );
	}
	if ( m_hWaitMedic != NULL )
	{
		if ( m_hWaitMedic->pev->deadflag != DEAD_NO )
			m_hWaitMedic = NULL;
		else
			( (COFSquadTalkMonster*)(CBaseEntity*)m_hWaitMedic )->HealMe( NULL );
	}
	SetUse( NULL );
	COFSquadTalkMonster::Killed( pevAttacker, iGib );
}

void CHGruntAlly::GibMonster()
{
	if ( m_hWaitMedic != NULL )
	{
		if ( m_hWaitMedic->pev->deadflag != DEAD_NO )
			m_hWaitMedic = NULL;
		else
			( (COFSquadTalkMonster*)(CBaseEntity*)m_hWaitMedic )->HealMe( NULL );
	}
	if ( m_iWeaponIdx != 3 )
	{
		Vector vecGunPos;
		Vector vecGunAngles;
		GetAttachment( 0, vecGunPos, vecGunAngles );
		CBaseEntity* pGun;
		if ( pev->weapons & HGRUNT_SHOTGUN )
			pGun = DropItem( "weapon_shotgun", vecGunPos, vecGunAngles );
		else if ( pev->weapons & HGRUNT_SAW )
			pGun = DropItem( "weapon_m249", vecGunPos, vecGunAngles );
		else
			pGun = DropItem( "weapon_9mmAR", vecGunPos, vecGunAngles );
		if ( pGun )
		{
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if ( pev->weapons & HGRUNT_GRENADELAUNCHER )
		{
			pGun = DropItem( "ammo_ARgrenades", vecGunPos, vecGunAngles );
			if ( pGun )
			{
				pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( -100, 100 ), RANDOM_FLOAT( 200, 300 ) );
				pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
		m_iWeaponIdx = 3;
	}
	CBaseMonster::GibMonster();
}

Schedule_t* CHGruntAlly::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_IDLE_STAND:
		{
			Schedule_t* schedule = COFSquadTalkMonster::GetScheduleOfType( Type );
			return schedule == slIdleStand ? slGruntAllyIdleStand : schedule;
		}
	case SCHED_TARGET_FACE:
		{
			Schedule_t* schedule = COFSquadTalkMonster::GetScheduleOfType( Type );
			return schedule == slIdleStand ? slGruntAllyFaceTarget : schedule;
		}
	case SCHED_TARGET_CHASE:
		return slGruntAllyFollow;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() && g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
			OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		{
			if ( FOkToSpeak() )
			{
				SENTENCEG_PlayRndSz( edict(), "FG_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
				JustSpoke();
			}
			return slGruntAllyTossGrenadeCover;
		}
		return slGruntAllyTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND:
		return slGruntAllyTakeCoverFromBestSound;
	case SCHED_GRUNT_TAKECOVER_FAILED:
		if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) && OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK1 );
		return GetScheduleOfType( SCHED_FAIL );
	case SCHED_GRUNT_ELOF_FAIL:
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	case SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE:
		return slGruntAllyEstablishLineOfFire;
	case SCHED_RANGE_ATTACK1:
		if ( pev->weapons & HGRUNT_SAW )
		{
			m_fStanding = TRUE;
		}
		else if ( RANDOM_LONG( 0, 9 ) == 0 )
		{
			m_fStanding = RANDOM_LONG( 0, 1 );
		}
		return m_fStanding ? slGruntAllyRangeAttack1B : slGruntAllyRangeAttack1A;
	case SCHED_RANGE_ATTACK2:
		return slGruntAllyRangeAttack2;
	case SCHED_COMBAT_FACE:
		return slGruntAllyCombatFace;
	case SCHED_GRUNT_WAIT_FACE_ENEMY:
		return slGruntAllyWaitInCover;
	case SCHED_GRUNT_SWEEP:
		return slGruntAllySweep;
	case SCHED_GRUNT_COVER_AND_RELOAD:
		return slGruntAllyHideReload;
	case SCHED_GRUNT_FOUND_ENEMY:
		return slGruntAllyFoundEnemy;
	case SCHED_VICTORY_DANCE:
		return InSquad() && !IsLeader() ? slGruntAllyFail : slGruntAllyVictoryDance;
	case SCHED_GRUNT_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slGruntAllySignalSuppress;
		}
		return slGruntAllySuppress;
	case SCHED_FAIL:
		return m_hEnemy != NULL ? slGruntAllyCombatFail : slGruntAllyFail;
	case SCHED_CANT_FOLLOW:
		return slGruntAllyFail;
	case SCHED_GRUNT_REPEL:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slGruntAllyRepel;
	case SCHED_GRUNT_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slGruntAllyRepelAttack;
	case SCHED_GRUNT_REPEL_LAND:
		return slGruntAllyRepelLand;
	}
	return COFSquadTalkMonster::GetScheduleOfType( Type );
}

Schedule_t* CHGruntAlly::GetSchedule()
{
	m_iSentence = HGRUNT_SENT_NONE;
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
				SENTENCEG_PlayRndSz( edict(), "FG_GREN", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
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
		if ( m_hWaitMedic == NULL && gpGlobals->time > m_flMedicWaitTime && pev->health <= HGRUNT_LIMP_HEALTH )
		{
			COFSquadTalkMonster* pMedic = MySquadMedic();
			if ( !pMedic )
				pMedic = FindSquadMedic( 1024 );
			if ( pMedic && pMedic->pev->deadflag == DEAD_NO )
			{
				ALERT( at_aiconsole, "Injured Grunt found Medic\n" );
				if ( pMedic->HealMe( this ) )
				{
					ALERT( at_aiconsole, "Injured Grunt called for Medic\n" );
					EMIT_SOUND( edict(), CHAN_VOICE, "fgrunt/medic.wav", 1, ATTN_NORM );
					JustSpoke();
					m_flMedicWaitTime = gpGlobals->time + 5;
				}
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
					SENTENCEG_PlayRndSz( edict(), "FG_ALERT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
				else if ( m_hEnemy != NULL && m_hEnemy->Classify() != CLASS_PLAYER_ALLY &&
					m_hEnemy->Classify() != CLASS_HUMAN_PASSIVE && m_hEnemy->Classify() != CLASS_MACHINE )
					SENTENCEG_PlayRndSz( edict(), "FG_MONST", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
				JustSpoke();
			}
			return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
				SCHED_GRUNT_SUPPRESS : SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
		}
		if ( HasConditions( bits_COND_HEAVY_DAMAGE ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		if ( pev->weapons && HasConditions( bits_COND_NO_AMMO_LOADED ) )
			return GetScheduleOfType( SCHED_GRUNT_COVER_AND_RELOAD );
		if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
		{
			if ( RANDOM_LONG( 0, 99 ) <= 90 && m_hEnemy != NULL )
			{
				if ( FOkToSpeak() )
					m_iSentence = HGRUNT_SENT_COVER;
				return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
			}
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		}
		if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
		if ( ( pev->weapons & HGRUNT_GRENADELAUNCHER ) && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
			OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
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
					SENTENCEG_PlayRndSz( edict(), "FG_THROW", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
			}
			if ( OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			{
				if ( FOkToSpeak() )
					m_iSentence = HGRUNT_SENT_CHARGE;
				return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
			}
			if ( !IsFollowing() )
			{
				if ( FOkToSpeak() && RANDOM_LONG( 0, 1 ) )
				{
					SENTENCEG_PlayRndSz( edict(), "FG_TAUNT", HGRUNT_SENTENCE_VOLUME, GRUNT_ATTN, 0, m_voicePitch );
					JustSpoke();
				}
				return GetScheduleOfType( SCHED_STANDOFF );
			}
		}
		if ( !IsFollowing() && HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_GRUNT_ESTABLISH_LINE_OF_FIRE );
		break;
	default:
		break;
	}
	return CBaseMonster::GetSchedule();
}

class CHGruntAllyRepel : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void EXPORT RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int	m_iSpriteTexture;
	int	m_iGruntHead;
	int	m_iszUse;
	int	m_iszUnUse;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_grunt_ally_repel, CHGruntAllyRepel );

void CHGruntAllyRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &CHGruntAllyRepel::RepelUse );
}

void CHGruntAllyRepel::Precache()
{
	UTIL_PrecacheOther( "monster_human_grunt_ally" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CHGruntAllyRepel::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iGruntHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "UseSentence" ) )
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

void CHGruntAllyRepel::RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, edict(), &tr );
	CBaseEntity* pEntity = Create( "monster_human_grunt_ally", pev->origin, pev->angles );
	CHGruntAlly* pGrunt = (CHGruntAlly*)pEntity->MySquadTalkMonsterPointer();
	if ( !pGrunt )
		return;
	pGrunt->pev->weapons = pev->weapons;
	pGrunt->pev->netname = pev->netname;
	pGrunt->pev->spawnflags |= pev->spawnflags & ( SF_SQUADMONSTER_LEADER | SF_MONSTER_PREDISASTER |
		SF_MONSTER_GAG | SF_MONSTER_HITMONSTERCLIP | SF_MONSTER_WAIT_TILL_SEEN | SF_MONSTER_WAIT_FOR_SCRIPT );
	pGrunt->m_iGruntHead = m_iGruntHead;
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

class CDeadHGruntAlly : public CBaseMonster
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	int Classify() { return CLASS_HUMAN_FRIENDLY; }
	int m_iPose;
	int m_iGruntHead;
	static char* m_szPoses[7];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_human_grunt_ally_dead, CDeadHGruntAlly );

char* CDeadHGruntAlly::m_szPoses[] =
{
	"deadstomach", "deadside", "deadsitting", "dead_on_back", "hgrunt_dead_stomach", "dead_headcrabed", "dead_canyon",
};

void CDeadHGruntAlly::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iGruntHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

void CDeadHGruntAlly::Spawn()
{
	PRECACHE_MODEL( "models/hgrunt_opfor.mdl" );
	SET_MODEL( edict(), "models/hgrunt_opfor.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead hgrunt with bad pose\n" );
	pev->health = 8;
	if ( pev->weapons & HGRUNT_9MMAR )
		pev->body = m_iGruntHead;
	else if ( pev->weapons & HGRUNT_SHOTGUN )
		pev->body = m_iGruntHead + 56;
	else if ( pev->weapons & HGRUNT_SAW )
		pev->body = m_iGruntHead + 72;
	else
		pev->body = m_iGruntHead + 96;
	pev->skin = 0;
	MonsterInitDead();
}

class COFSkeleton : public CBaseMonster
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	int Classify() { return CLASS_HUMAN_MILITARY; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	int m_iPose;
	static char* m_szPoses[4];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_skeleton_dead, COFSkeleton );

char* COFSkeleton::m_szPoses[] =
{
	"s_onback", "s_sitting", "dead_against_wall", "dead_stomach",
};

void COFSkeleton::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
	{
		CBaseMonster::KeyValue( pkvd );
	}
}

int COFSkeleton::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, 0, bitsDamageType );
}

void COFSkeleton::Spawn()
{
	PRECACHE_MODEL( "models/skeleton.mdl" );
	SET_MODEL( edict(), "models/skeleton.mdl" );
	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = DONT_BLEED;
	pev->sequence = LookupSequence( m_szPoses[m_iPose] );
	if ( pev->sequence == -1 )
		ALERT( at_console, "Dead skeleton with bad pose\n" );
	pev->health = 8;
	pev->skin = 0;
	MonsterInitDead();
}
