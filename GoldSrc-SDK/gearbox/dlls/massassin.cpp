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

#define MASSASSIN_9MMAR			1
#define MASSASSIN_HANDGRENADE	2
#define MASSASSIN_GRENADELAUNCHER	4
#define MASSASSIN_SNIPERRIFLE	8
#define bits_COND_MASSASSIN_NOFIRE	bits_COND_SPECIAL1

enum
{
	SCHED_MASSASSIN_SUPPRESS = LAST_COMMON_SCHEDULE + 1,
	SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE,
	SCHED_MASSASSIN_HIDE_RELOAD,
	SCHED_MASSASSIN_RELOAD,
	SCHED_MASSASSIN_SWEEP,
	SCHED_MASSASSIN_FOUND_ENEMY,
	SCHED_MASSASSIN_REPEL,
	SCHED_MASSASSIN_REPEL_ATTACK,
	SCHED_MASSASSIN_REPEL_LAND,
	SCHED_MASSASSIN_WAIT_FACE_ENEMY,
	SCHED_MASSASSIN_TAKECOVER_FAILED,
	SCHED_MASSASSIN_ELOF_FAIL
};

enum
{
	TASK_MASSASSIN_FACE_TOSS_DIR = LAST_COMMON_TASK + 1,
	TASK_MASSASSIN_SPEAK_SENTENCE,
	TASK_MASSASSIN_CHECK_FIRE
};

enum
{
	MASSASSIN_SENT_NONE = -1,
	MASSASSIN_SENT_GREN,
	MASSASSIN_SENT_ALERT,
	MASSASSIN_SENT_MONSTER,
	MASSASSIN_SENT_COVER,
	MASSASSIN_SENT_THROW,
	MASSASSIN_SENT_CHARGE,
	MASSASSIN_SENT_TAUNT
} MASSASSIN_SENTENCE_TYPES;

int g_fAssassinQuestion;
extern DLL_GLOBAL int g_iSkillLevel;

class CMOFAssassin : public CSquadMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void SetYawSpeed() override;
	int Classify() override;
	int ISoundMask() override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	BOOL FCanCheckAttacks() override;
	BOOL CheckMeleeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack2( float flDot, float flDist ) override;
	void CheckAmmo() override;
	void SetActivity( Activity activity ) override;
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
	static const char* pAssassinSentences[];

	float	m_flNextGrenadeCheck;
	float	m_flNextPainTime;
	float	m_flLastEnemySightTime;
	Vector	m_vecTossVelocity;
	BOOL	m_fThrowGrenade;
	BOOL	m_fStanding;
	BOOL	m_fFirstEncounter;
	int		m_cClipSize;
	float	m_flLastShot;
	BOOL	m_fStandingGround;
	float	m_flStandGroundRange;
	int		m_voicePitch;
	int		m_iBrassShell;
	int		m_iSentence;
	int		m_iAssassinHead;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_male_assassin, CMOFAssassin );

TYPEDESCRIPTION CMOFAssassin::m_SaveData[] =
{
	DEFINE_FIELD( CMOFAssassin, m_flNextGrenadeCheck, FIELD_TIME ),
	DEFINE_FIELD( CMOFAssassin, m_flNextPainTime, FIELD_TIME ),
	DEFINE_FIELD( CMOFAssassin, m_vecTossVelocity, FIELD_VECTOR ),
	DEFINE_FIELD( CMOFAssassin, m_fThrowGrenade, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMOFAssassin, m_fStanding, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMOFAssassin, m_fFirstEncounter, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMOFAssassin, m_cClipSize, FIELD_INTEGER ),
	DEFINE_FIELD( CMOFAssassin, m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( CMOFAssassin, m_iSentence, FIELD_INTEGER ),
	DEFINE_FIELD( CMOFAssassin, m_iAssassinHead, FIELD_INTEGER ),
	DEFINE_FIELD( CMOFAssassin, m_flLastShot, FIELD_TIME ),
	DEFINE_FIELD( CMOFAssassin, m_fStandingGround, FIELD_BOOLEAN ),
	DEFINE_FIELD( CMOFAssassin, m_flStandGroundRange, FIELD_FLOAT )
};

IMPLEMENT_SAVERESTORE( CMOFAssassin, CSquadMonster );

const char* CMOFAssassin::pAssassinSentences[] =
{
	"HG_GREN", "HG_ALERT", "HG_MONSTER", "HG_COVER", "HG_THROW", "HG_CHARGE", "HG_TAUNT"
};

Task_t tlOFMAssassinFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slOFMAssassinFail[] =
{
	{ tlOFMAssassinFail, ARRAYSIZE( tlOFMAssassinFail ), bits_COND_CAN_ATTACK, 0, "Assassin Fail" }
};
Task_t tlOFMAssassinCombatFail[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 2 }, { TASK_WAIT_PVS, 0 }
};
Schedule_t slOFMAssassinCombatFail[] =
{
	{ tlOFMAssassinCombatFail, ARRAYSIZE( tlOFMAssassinCombatFail ),
		bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Assassin Combat Fail" }
};
Task_t tlOFMAssassinVictoryDance[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_WAIT, 1.5f },
	{ TASK_GET_PATH_TO_ENEMY_CORPSE, 0 }, { TASK_WALK_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_VICTORY_DANCE }
};
Schedule_t slOFMAssassinVictoryDance[] =
{
	{ tlOFMAssassinVictoryDance, ARRAYSIZE( tlOFMAssassinVictoryDance ),
		bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE, 0, "AssassinVictoryDance" }
};
Task_t tlOFMAssassinEstablishLineOfFire[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_MASSASSIN_ELOF_FAIL }, { TASK_GET_PATH_TO_ENEMY, 0 },
	{ TASK_MASSASSIN_SPEAK_SENTENCE, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }
};
Schedule_t slOFMAssassinEstablishLineOfFire[] =
{
	{ tlOFMAssassinEstablishLineOfFire, ARRAYSIZE( tlOFMAssassinEstablishLineOfFire ),
		bits_COND_NEW_ENEMY | bits_COND_ENEMY_DEAD | bits_COND_CAN_ATTACK | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "AssassinEstablishLineOfFire" }
};
Task_t tlOFMAssassinFoundEnemy[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL1 }
};
Schedule_t slOFMAssassinFoundEnemy[] =
{
	{ tlOFMAssassinFoundEnemy, ARRAYSIZE( tlOFMAssassinFoundEnemy ), bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "AssassinFoundEnemy" }
};
Task_t tlOFMAssassinCombatFace1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_FACE_ENEMY, 0 },
	{ TASK_WAIT, 1.5f }, { TASK_SET_SCHEDULE, SCHED_MASSASSIN_SWEEP }
};
Schedule_t slOFMAssassinCombatFace[] =
{
	{ tlOFMAssassinCombatFace1, ARRAYSIZE( tlOFMAssassinCombatFace1 ), bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2, 0, "Combat Face" }
};
Task_t tlOFMAssassinSignalSuppress[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_SIGNAL2 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slOFMAssassinSignalSuppress[] =
{
	{ tlOFMAssassinSignalSuppress, ARRAYSIZE( tlOFMAssassinSignalSuppress ), bits_COND_ENEMY_DEAD |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_NO_AMMO_LOADED |
		bits_COND_MASSASSIN_NOFIRE, bits_SOUND_DANGER, "SignalSuppress" }
};
Task_t tlOFMAssassinSuppress[] =
{
	{ TASK_STOP_MOVING, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 },
	{ TASK_FACE_ENEMY, 0 }, { TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slOFMAssassinSuppress[] =
{
	{ tlOFMAssassinSuppress, ARRAYSIZE( tlOFMAssassinSuppress ), bits_COND_ENEMY_DEAD | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND | bits_COND_NO_AMMO_LOADED | bits_COND_MASSASSIN_NOFIRE,
		bits_SOUND_DANGER, "Suppress" }
};
Task_t tlOFMAssassinWaitInCover[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_ACTIVITY, ACT_IDLE }, { TASK_WAIT_FACE_ENEMY, 1 }
};
Schedule_t slOFMAssassinWaitInCover[] =
{
	{ tlOFMAssassinWaitInCover, ARRAYSIZE( tlOFMAssassinWaitInCover ), bits_COND_NEW_ENEMY |
		bits_COND_CAN_ATTACK | bits_COND_HEAR_SOUND, bits_SOUND_DANGER, "AssassinWaitInCover" }
};
Task_t tlOFMAssassinTakeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_MASSASSIN_TAKECOVER_FAILED },
	{ TASK_WAIT, 0.2f }, { TASK_FIND_COVER_FROM_ENEMY, 0 }, { TASK_MASSASSIN_SPEAK_SENTENCE, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_SET_SCHEDULE, SCHED_MASSASSIN_WAIT_FACE_ENEMY }
};
Schedule_t slOFMAssassinTakeCover[] =
{
	{ tlOFMAssassinTakeCover1, ARRAYSIZE( tlOFMAssassinTakeCover1 ), 0, 0, "TakeCover" }
};
Task_t tlOFMAssassinGrenadeCover1[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_ENEMY, 99 },
	{ TASK_FIND_FAR_NODE_COVER_FROM_ENEMY, 384 }, { TASK_PLAY_SEQUENCE, ACT_SPECIAL_ATTACK1 },
	{ TASK_CLEAR_MOVE_WAIT, 0 }, { TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_SET_SCHEDULE, SCHED_MASSASSIN_WAIT_FACE_ENEMY }
};
Schedule_t slOFMAssassinGrenadeCover[] =
{
	{ tlOFMAssassinGrenadeCover1, ARRAYSIZE( tlOFMAssassinGrenadeCover1 ), 0, 0, "GrenadeCover" }
};
Task_t tlOFMAssassinTossGrenadeCover1[] =
{
	{ TASK_FACE_ENEMY, 0 }, { TASK_RANGE_ATTACK2, 0 }, { TASK_SET_SCHEDULE, SCHED_TAKE_COVER_FROM_ENEMY }
};
Schedule_t slOFMAssassinTossGrenadeCover[] =
{
	{ tlOFMAssassinTossGrenadeCover1, ARRAYSIZE( tlOFMAssassinTossGrenadeCover1 ), 0, 0, "TossGrenadeCover" }
};
Task_t tlOFMAssassinTakeCoverFromBestSound[] =
{
	{ TASK_SET_FAIL_SCHEDULE, SCHED_COWER }, { TASK_STOP_MOVING, 0 }, { TASK_FIND_COVER_FROM_BEST_SOUND, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_TURN_LEFT, 179 }
};
Schedule_t slOFMAssassinTakeCoverFromBestSound[] =
{
	{ tlOFMAssassinTakeCoverFromBestSound, ARRAYSIZE( tlOFMAssassinTakeCoverFromBestSound ),
		0, 0, "AssassinTakeCoverFromBestSound" }
};
Task_t tlOFMAssassinHideReload[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD }, { TASK_FIND_COVER_FROM_ENEMY, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_REMEMBER, bits_MEMORY_INCOVER },
	{ TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_RELOAD }
};
Schedule_t slOFMAssassinHideReload[] =
{
	{ tlOFMAssassinHideReload, ARRAYSIZE( tlOFMAssassinHideReload ), bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "AssassinHideReload" }
};
Task_t tlOFMAssassinReload[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_SET_FAIL_SCHEDULE, SCHED_RELOAD }, { TASK_WAIT_FOR_MOVEMENT, 0 },
	{ TASK_REMEMBER, bits_MEMORY_INCOVER }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_RELOAD }
};
Schedule_t slOFMAssassinReload[] =
{
	{ tlOFMAssassinReload, ARRAYSIZE( tlOFMAssassinReload ), bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER, "AssassinReload" }
};
Task_t tlOFMAssassinSweep[] =
{
	{ TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }, { TASK_TURN_LEFT, 179 }, { TASK_WAIT, 1 }
};
Schedule_t slOFMAssassinSweep[] =
{
	{ tlOFMAssassinSweep, ARRAYSIZE( tlOFMAssassinSweep ), bits_COND_NEW_ENEMY | bits_COND_LIGHT_DAMAGE |
		bits_COND_HEAVY_DAMAGE | bits_COND_CAN_RANGE_ATTACK1 | bits_COND_CAN_RANGE_ATTACK2 | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_PLAYER | bits_SOUND_WORLD, "Assassin Sweep" }
};
Task_t tlOFMAssassinRangeAttack1A[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_CROUCH },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slOFMAssassinRangeAttack1A[] =
{
	{ tlOFMAssassinRangeAttack1A, ARRAYSIZE( tlOFMAssassinRangeAttack1A ), bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAR_SOUND |
		bits_COND_NO_AMMO_LOADED | bits_COND_MASSASSIN_NOFIRE, bits_SOUND_DANGER, "Range Attack1A" }
};
Task_t tlOFMAssassinRangeAttack1B[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE_FACE_ENEMY, ACT_IDLE_ANGRY },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }, { TASK_FACE_ENEMY, 0 },
	{ TASK_MASSASSIN_CHECK_FIRE, 0 }, { TASK_RANGE_ATTACK1, 0 }
};
Schedule_t slOFMAssassinRangeAttack1B[] =
{
	{ tlOFMAssassinRangeAttack1B, ARRAYSIZE( tlOFMAssassinRangeAttack1B ), bits_COND_NEW_ENEMY |
		bits_COND_ENEMY_DEAD | bits_COND_HEAVY_DAMAGE | bits_COND_ENEMY_OCCLUDED | bits_COND_HEAR_SOUND |
		bits_COND_NO_AMMO_LOADED | bits_COND_MASSASSIN_NOFIRE, bits_SOUND_DANGER, "Range Attack1B" }
};
Task_t tlOFMAssassinRangeAttack2[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_MASSASSIN_FACE_TOSS_DIR, 0 }, { TASK_PLAY_SEQUENCE, ACT_RANGE_ATTACK2 },
	{ TASK_SET_SCHEDULE, SCHED_MASSASSIN_WAIT_FACE_ENEMY }
};
Schedule_t slOFMAssassinRangeAttack2[] =
{
	{ tlOFMAssassinRangeAttack2, ARRAYSIZE( tlOFMAssassinRangeAttack2 ), 0, 0, "RangeAttack2" }
};
Task_t tlOFMAssassinRepel[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_IDEAL, 0 }, { TASK_PLAY_SEQUENCE, ACT_GLIDE }
};
Schedule_t slOFMAssassinRepel[] =
{
	{ tlOFMAssassinRepel, ARRAYSIZE( tlOFMAssassinRepel ), bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_PLAYER | bits_SOUND_COMBAT, "Repel" }
};
Task_t tlOFMAssassinRepelAttack[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_FACE_ENEMY, 0 }, { TASK_PLAY_SEQUENCE, ACT_FLY }
};
Schedule_t slOFMAssassinRepelAttack[] =
{
	{ tlOFMAssassinRepelAttack, ARRAYSIZE( tlOFMAssassinRepelAttack ), bits_COND_ENEMY_OCCLUDED, 0, "Repel Attack" }
};
Task_t tlOFMAssassinRepelLand[] =
{
	{ TASK_STOP_MOVING, 0 }, { TASK_PLAY_SEQUENCE, ACT_LAND }, { TASK_GET_PATH_TO_LASTPOSITION, 0 },
	{ TASK_RUN_PATH, 0 }, { TASK_WAIT_FOR_MOVEMENT, 0 }, { TASK_CLEAR_LASTPOSITION, 0 }
};
Schedule_t slOFMAssassinRepelLand[] =
{
	{ tlOFMAssassinRepelLand, ARRAYSIZE( tlOFMAssassinRepelLand ), bits_COND_SEE_ENEMY | bits_COND_NEW_ENEMY |
		bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE | bits_COND_HEAR_SOUND,
		bits_SOUND_DANGER | bits_SOUND_PLAYER | bits_SOUND_COMBAT, "Repel Land" }
};

DEFINE_CUSTOM_SCHEDULES( CMOFAssassin )
{
	slOFMAssassinFail, slOFMAssassinCombatFail, slOFMAssassinVictoryDance, slOFMAssassinEstablishLineOfFire,
	slOFMAssassinFoundEnemy, slOFMAssassinCombatFace, slOFMAssassinSignalSuppress, slOFMAssassinSuppress,
	slOFMAssassinWaitInCover, slOFMAssassinTakeCover, slOFMAssassinGrenadeCover, slOFMAssassinTossGrenadeCover,
	slOFMAssassinTakeCoverFromBestSound, slOFMAssassinHideReload, slOFMAssassinReload, slOFMAssassinSweep,
	slOFMAssassinRangeAttack1A, slOFMAssassinRangeAttack1B, slOFMAssassinRangeAttack2,
	slOFMAssassinRepel, slOFMAssassinRepelAttack, slOFMAssassinRepelLand
};

IMPLEMENT_CUSTOM_SCHEDULES( CMOFAssassin, CSquadMonster );

int CMOFAssassin::ISoundMask()
{
	return bits_SOUND_WORLD | bits_SOUND_COMBAT | bits_SOUND_PLAYER | bits_SOUND_DANGER;
}

int CMOFAssassin::Classify()
{
	return CLASS_HUMAN_MILITARY;
}

BOOL CMOFAssassin::FCanCheckAttacks()
{
	return !HasConditions( bits_COND_ENEMY_TOOFAR );
}

void CMOFAssassin::SetYawSpeed()
{
	switch ( m_Activity )
	{
	case ACT_IDLE:
	case ACT_RUN: pev->yaw_speed = 150; break;
	case ACT_WALK:
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT: pev->yaw_speed = 180; break;
	case ACT_RANGE_ATTACK1:
	case ACT_RANGE_ATTACK2:
	case ACT_MELEE_ATTACK1:
	case ACT_MELEE_ATTACK2: pev->yaw_speed = 120; break;
	case ACT_GLIDE:
	case ACT_FLY: pev->yaw_speed = 30; break;
	default: pev->yaw_speed = 90; break;
	}
}

void CMOFAssassin::IdleSound()
{
}

void CMOFAssassin::PainSound()
{
}

void CMOFAssassin::DeathSound()
{
	static const char* sounds[] = { "hgrunt/gr_die1.wav", "hgrunt/gr_die2.wav", "hgrunt/gr_die3.wav" };
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, sounds[RANDOM_LONG( 0, 2 )], 1, ATTN_IDLE, 0, 100 );
}

void CMOFAssassin::Precache()
{
	PRECACHE_MODEL( "models/massn.mdl" );
	PRECACHE_SOUND( "hgrunt/gr_mgun1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_mgun2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die1.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die2.wav" );
	PRECACHE_SOUND( "hgrunt/gr_die3.wav" );
	PRECACHE_SOUND( "hgrunt/gr_reload1.wav" );
	PRECACHE_SOUND( "weapons/glauncher.wav" );
	PRECACHE_SOUND( "weapons/sbarrel1.wav" );
	PRECACHE_SOUND( "zombie/claw_miss2.wav" );
	m_voicePitch = RANDOM_LONG( 0, 1 ) ? RANDOM_LONG( 0, 7 ) + 109 : 100;
	m_iBrassShell = PRECACHE_MODEL( "models/shell.mdl" );
}

void CMOFAssassin::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/massn.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 72 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_RED;
	pev->effects = 0;
	pev->health = gSkillData.massassinHealth;
	m_flFieldOfView = 0.2f;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	m_flNextPainTime = gpGlobals->time;
	m_iSentence = MASSASSIN_SENT_NONE;
	m_HackedGunPos = Vector( 0, 0, 55 );
	m_afCapability = bits_CAP_SQUAD | bits_CAP_TURN_HEAD | bits_CAP_DOORS_GROUP;
	m_fEnemyEluded = FALSE;
	m_fFirstEncounter = TRUE;
	if ( !pev->weapons )
		pev->weapons = MASSASSIN_9MMAR | MASSASSIN_HANDGRENADE;
	if ( m_iAssassinHead == -1 )
		m_iAssassinHead = RANDOM_LONG( 0, 2 );
	if ( pev->weapons & MASSASSIN_9MMAR )
	{
		pev->body = m_iAssassinHead;
		m_cClipSize = 36;
	}
	else if ( pev->weapons & MASSASSIN_SNIPERRIFLE )
	{
		pev->body = m_iAssassinHead + 3;
		m_cClipSize = 1;
	}
	else
	{
		pev->body = m_iAssassinHead + 6;
		m_cClipSize = 0;
	}
	m_cAmmoLoaded = m_cClipSize;
	m_flLastShot = gpGlobals->time;
	pev->skin = 0;
	CTalkMonster::g_talkWaitTime = 0;
	m_fStandingGround = m_flStandGroundRange != 0;
	MonsterInit();
}

void CMOFAssassin::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "head" ) )
	{
		m_iAssassinHead = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else if ( FStrEq( pkvd->szKeyName, "standgroundrange" ) )
	{
		m_flStandGroundRange = atof( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

Vector CMOFAssassin::GetGunPosition()
{
	return pev->origin + Vector( 0, 0, m_fStanding ? 60 : 48 );
}

void CMOFAssassin::CheckAmmo()
{
	if ( pev->weapons && m_cAmmoLoaded <= 0 )
		SetConditions( bits_COND_NO_AMMO_LOADED );
}

void CMOFAssassin::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType )
{
	if ( ptr->iHitgroup == 11 )
		ptr->iHitgroup = HITGROUP_HEAD;
	CBaseMonster::TraceAttack( pevAttacker, flDamage, vecDir, ptr, bitsDamageType );
}

int CMOFAssassin::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	Forget( bits_MEMORY_INCOVER );
	return CSquadMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

int CMOFAssassin::IRelationship( CBaseEntity* pTarget )
{
	if ( FClassnameIs( pTarget->pev, "monster_alien_grunt" ) || FClassnameIs( pTarget->pev, "monster_gargantua" ) )
		return R_NM;
	return CBaseMonster::IRelationship( pTarget );
}

BOOL CMOFAssassin::FOkToSpeak()
{
	return CTalkMonster::g_talkWaitTime < gpGlobals->time &&
		( !( pev->spawnflags & SF_MONSTER_GAG ) || m_MonsterState == MONSTERSTATE_COMBAT );
}

void CMOFAssassin::JustSpoke()
{
	CTalkMonster::g_talkWaitTime = gpGlobals->time + RANDOM_FLOAT( 1.5, 2 );
	m_iSentence = MASSASSIN_SENT_NONE;
}

void CMOFAssassin::SpeakSentence()
{
	if ( m_iSentence == MASSASSIN_SENT_NONE || !FOkToSpeak() )
		return;
	SENTENCEG_PlayRndSz( edict(), pAssassinSentences[m_iSentence], 0.35f, ATTN_NORM, 0, m_voicePitch );
	JustSpoke();
}

void CMOFAssassin::PrescheduleThink()
{
	if ( !InSquad() || m_hEnemy == NULL )
		return;
	if ( HasConditions( bits_COND_SEE_ENEMY ) )
		MySquadLeader()->m_flLastEnemySightTime = gpGlobals->time;
	else if ( gpGlobals->time - MySquadLeader()->m_flLastEnemySightTime > 5 )
		MySquadLeader()->m_fEnemyEluded = TRUE;
}

BOOL CMOFAssassin::CheckMeleeAttack1( float flDot, float flDist )
{
	if ( m_hEnemy == NULL )
		return FALSE;
	CBaseMonster* pEnemy = m_hEnemy->MyMonsterPointer();
	if ( !pEnemy )
		return FALSE;
	return flDist <= 64 && flDot >= 0.7 && pEnemy->Classify() != CLASS_ALIEN_BIOWEAPON &&
		pEnemy->Classify() != CLASS_PLAYER_BIOWEAPON;
}

BOOL CMOFAssassin::CheckRangeAttack1( float flDot, float flDist )
{
	if ( !pev->weapons )
		return FALSE;
	if ( m_fStandingGround && m_flStandGroundRange >= flDist )
		m_fStandingGround = FALSE;
	if ( HasConditions( bits_COND_ENEMY_OCCLUDED ) || flDist > 2048 || flDot < 0.5f || !NoFriendlyFire() )
		return FALSE;
	if ( !m_hEnemy->IsPlayer() && flDist <= 64 )
		return FALSE;
	TraceResult tr;
	Vector vecSrc = GetGunPosition();
	UTIL_TraceLine( vecSrc, m_hEnemy->BodyTarget( vecSrc ), ignore_monsters, ignore_glass, edict(), &tr );
	return tr.flFraction == 1;
}

BOOL CMOFAssassin::CheckRangeAttack2( float flDot, float flDist )
{
	(void)flDot;
	(void)flDist;
	if ( !( pev->weapons & ( MASSASSIN_HANDGRENADE | MASSASSIN_GRENADELAUNCHER ) ) )
		return FALSE;
	if ( m_flGroundSpeed != 0 )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	if ( m_flNextGrenadeCheck > gpGlobals->time )
		return m_fThrowGrenade;
	if ( !( m_hEnemy->pev->flags & FL_ONGROUND ) && m_hEnemy->pev->waterlevel == 0 &&
		m_vecEnemyLKP.z > pev->absmax.z )
	{
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector vecTarget;
	if ( pev->weapons & MASSASSIN_HANDGRENADE )
	{
		if ( RANDOM_LONG( 0, 1 ) )
			vecTarget = Vector( m_hEnemy->pev->origin.x, m_hEnemy->pev->origin.y, m_hEnemy->pev->absmin.z );
		else
			vecTarget = m_vecEnemyLKP;
	}
	else
	{
		vecTarget = m_hEnemy->BodyTarget( pev->origin ) - m_hEnemy->pev->origin + m_vecEnemyLKP;
		if ( HasConditions( bits_COND_SEE_ENEMY ) )
			vecTarget = vecTarget + m_hEnemy->pev->velocity *
				( ( vecTarget - pev->origin ).Length() / gSkillData.massassinGrenadeSpeed );
	}
	if ( InSquad() && SquadMemberInRange( vecTarget, 256 ) )
	{
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 1;
	}
	if ( ( vecTarget - pev->origin ).Length2D() <= 256 )
	{
		m_flNextGrenadeCheck = gpGlobals->time + 1;
		m_fThrowGrenade = FALSE;
		return FALSE;
	}
	Vector vecToss;
	if ( pev->weapons & MASSASSIN_HANDGRENADE )
		vecToss = VecCheckToss( pev, GetGunPosition(), vecTarget, 0.5f );
	else
		vecToss = VecCheckThrow( pev, GetGunPosition(), vecTarget, gSkillData.massassinGrenadeSpeed, 0.5f );
	if ( vecToss != g_vecZero )
	{
		m_vecTossVelocity = vecToss;
		m_fThrowGrenade = TRUE;
		m_flNextGrenadeCheck = pev->weapons & MASSASSIN_HANDGRENADE ? gpGlobals->time : gpGlobals->time + 0.3;
		return TRUE;
	}
	m_fThrowGrenade = FALSE;
	m_flNextGrenadeCheck = gpGlobals->time + 1;
	return FALSE;
}

void CMOFAssassin::StartTask( Task_t* pTask )
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
	case TASK_MASSASSIN_FACE_TOSS_DIR:
		break;
	case TASK_MASSASSIN_SPEAK_SENTENCE:
		TaskComplete();
		break;
	case TASK_MASSASSIN_CHECK_FIRE:
		if ( !NoFriendlyFire() )
			SetConditions( bits_COND_MASSASSIN_NOFIRE );
		TaskComplete();
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void CMOFAssassin::RunTask( Task_t* pTask )
{
	if ( pTask->iTask == TASK_MASSASSIN_FACE_TOSS_DIR )
	{
		MakeIdealYaw( pev->origin + m_vecTossVelocity * 64 );
		ChangeYaw( pev->yaw_speed );
		if ( FacingIdeal() )
			TaskComplete();
	}
	else
		CBaseMonster::RunTask( pTask );
}

CBaseEntity* CMOFAssassin::Kick()
{
	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5f;
	TraceResult tr;
	UTIL_TraceHull( vecStart, vecStart + gpGlobals->v_forward * 70,
		dont_ignore_monsters, head_hull, edict(), &tr );
	return tr.pHit ? (CBaseEntity*)GET_PRIVATE( tr.pHit ) : NULL;
}

void CMOFAssassin::Shoot()
{
	if ( m_hEnemy == NULL || ( ( pev->weapons & MASSASSIN_SNIPERRIFLE ) && gpGlobals->time - m_flLastShot <= 0.11 ) )
		return;
	Vector vecSrc = GetGunPosition();
	Vector vecDir = ShootAtEnemy( vecSrc );
	UTIL_MakeVectors( pev->angles );
	Vector vecForward = gpGlobals->v_forward * RANDOM_FLOAT( -40, 40 );
	Vector vecUp = gpGlobals->v_up * RANDOM_FLOAT( 75, 200 );
	Vector vecShellVelocity = gpGlobals->v_right * RANDOM_FLOAT( 40, 90 ) + vecUp + vecForward;
	if ( pev->weapons & MASSASSIN_9MMAR )
	{
		EjectBrass( vecSrc - vecDir * 24, vecShellVelocity, pev->angles.y, m_iBrassShell, TE_BOUNCE_SHELL );
		FireBullets( 1, vecSrc, vecDir, VECTOR_CONE_5DEGREES, 2048, BULLET_MONSTER_MP5, 4 );
	}
	else
		FireBullets( 1, vecSrc, vecDir, VECTOR_CONE_1DEGREES, 2048, BULLET_PLAYER_556, 4 );
	pev->effects |= EF_MUZZLEFLASH;
	m_cAmmoLoaded--;
	Vector angDir = UTIL_VecToAngles( vecDir );
	SetBlending( 0, angDir.x );
	m_flLastShot = gpGlobals->time;
}

void CMOFAssassin::SetActivity( Activity activity )
{
	GET_MODEL_PTR( edict() );
	int sequence;
	Activity requested = activity;
	switch ( activity )
	{
	case ACT_RANGE_ATTACK1:
		sequence = LookupSequence( m_fStanding ? "standing_mp5" : "crouching_mp5" );
		break;
	case ACT_RANGE_ATTACK2:
		sequence = LookupSequence( pev->weapons & MASSASSIN_HANDGRENADE ? "throwgrenade" : "launchgrenade" );
		break;
	case ACT_WALK:
		sequence = LookupActivity( pev->health > 20 ? ACT_WALK : ACT_WALK_HURT );
		break;
	case ACT_RUN:
		sequence = LookupActivity( pev->health > 20 ? ACT_RUN : ACT_RUN_HURT );
		break;
	case ACT_IDLE:
		if ( m_MonsterState == MONSTERSTATE_COMBAT )
			activity = ACT_IDLE_ANGRY;
		requested = activity;
		sequence = LookupActivity( activity );
		break;
	default:
		CBaseMonster::SetActivity( activity );
		return;
	}
	m_Activity = activity;
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
		ALERT( at_console, "%s has no sequence for act:%d\n", STRING( pev->classname ), requested );
		pev->sequence = 0;
	}
}

void CMOFAssassin::GibMonster()
{
	if ( (unsigned int)( pev->body - 6 ) > 2 )
	{
		Vector vecPos, vecAngles;
		GetAttachment( 0, vecPos, vecAngles );
		CBaseEntity* pGun = DropItem( pev->weapons & MASSASSIN_9MMAR ? "weapon_9mmAR" : "weapon_sniperrifle",
			vecPos, vecAngles );
		if ( pGun )
		{
			float z = RANDOM_FLOAT( 200, 300 );
			float y = RANDOM_FLOAT( -100, 100 );
			pGun->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), y, z );
			pGun->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
		}
		if ( pev->weapons & MASSASSIN_GRENADELAUNCHER )
		{
			CBaseEntity* pAmmo = DropItem( "ammo_ARgrenades", vecPos, vecAngles );
			if ( pAmmo )
			{
				float z = RANDOM_FLOAT( 200, 300 );
				float y = RANDOM_FLOAT( -100, 100 );
				pAmmo->pev->velocity = Vector( RANDOM_FLOAT( -100, 100 ), y, z );
				pAmmo->pev->avelocity = Vector( 0, RANDOM_FLOAT( 200, 400 ), 0 );
			}
		}
	}
	CBaseMonster::GibMonster();
}

void CMOFAssassin::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 2:
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "hgrunt/gr_reload1.wav", 1, ATTN_NORM, 0, 100 );
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
				pHurt->pev->velocity = gpGlobals->v_up * 50 + gpGlobals->v_forward * 100 + pHurt->pev->velocity;
				pHurt->TakeDamage( pev, pev, gSkillData.massassinDmgKick, DMG_CLUB );
			}
		}
		break;
	case 4:
		Shoot();
		if ( pev->weapons & MASSASSIN_9MMAR )
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_LONG( 0, 1 ) ? "hgrunt/gr_mgun1.wav" : "hgrunt/gr_mgun2.wav",
				1, ATTN_NORM, 0, 100 );
		else
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/sniper_fire.wav", 1, ATTN_NORM, 0, 100 );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 384, 0.3f );
		break;
	case 5:
	case 6:
		if ( !( pev->weapons & MASSASSIN_SNIPERRIFLE ) )
			Shoot();
		break;
	case 7:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, GetGunPosition(), m_vecTossVelocity, 3.5f );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = gpGlobals->time + 6;
		break;
	case 8:
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/glauncher.wav", 0.8f, ATTN_NORM, 0, 100 );
		CGrenade::ShootContact( pev, GetGunPosition(), m_vecTossVelocity );
		m_fThrowGrenade = FALSE;
		m_flNextGrenadeCheck = g_iSkillLevel == SKILL_HARD ? gpGlobals->time + RANDOM_FLOAT( 2, 5 ) : gpGlobals->time + 6;
		break;
	case 9:
		UTIL_MakeVectors( pev->angles );
		CGrenade::ShootTimed( pev, gpGlobals->v_forward * 17 + pev->origin - gpGlobals->v_right * 27 + gpGlobals->v_up * 6,
			g_vecZero, 3 );
		break;
	case 11:
		{
			Vector vecPos, vecAngles;
			GetAttachment( 0, vecPos, vecAngles );
			pev->body = pev->body % 3 + 6;
			DropItem( pev->weapons & MASSASSIN_9MMAR ? "weapon_9mmAR" : "weapon_sniperrifle", vecPos, vecAngles );
			if ( pev->weapons & MASSASSIN_GRENADELAUNCHER )
				DropItem( "ammo_ARgrenades", BodyTarget( pev->origin ), vecAngles );
		}
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

Schedule_t* CMOFAssassin::GetSchedule()
{
	m_iSentence = MASSASSIN_SENT_NONE;
	if ( pev->movetype == MOVETYPE_FLY && m_MonsterState != MONSTERSTATE_PRONE )
	{
		if ( pev->flags & FL_ONGROUND )
		{
			pev->movetype = MOVETYPE_STEP;
			return GetScheduleOfType( SCHED_MASSASSIN_REPEL_LAND );
		}
		return GetScheduleOfType( m_MonsterState == MONSTERSTATE_COMBAT ?
			SCHED_MASSASSIN_REPEL_ATTACK : SCHED_MASSASSIN_REPEL );
	}
	if ( HasConditions( bits_COND_HEAR_SOUND ) )
	{
		CSound* pSound = PBestSound();
		if ( pSound && ( pSound->m_iType & bits_SOUND_DANGER ) )
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_BEST_SOUND );
	}
	if ( m_MonsterState != MONSTERSTATE_COMBAT || HasConditions( bits_COND_ENEMY_DEAD ) )
		return CBaseMonster::GetSchedule();
	if ( m_fStandingGround )
	{
		if ( HasConditions( bits_COND_NO_AMMO_LOADED ) )
			return GetScheduleOfType( SCHED_MASSASSIN_RELOAD );
		return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
			SCHED_RANGE_ATTACK1 : SCHED_MASSASSIN_WAIT_FACE_ENEMY );
	}
	if ( HasConditions( bits_COND_NEW_ENEMY ) )
	{
		if ( InSquad() )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			if ( IsLeader() )
				return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) ?
					SCHED_MASSASSIN_SUPPRESS : SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
			return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
		}
		if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
			return GetScheduleOfType( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
		return CBaseMonster::GetSchedule();
	}
	if ( pev->weapons && HasConditions( bits_COND_NO_AMMO_LOADED ) )
		return GetScheduleOfType( SCHED_MASSASSIN_HIDE_RELOAD );
	if ( HasConditions( bits_COND_LIGHT_DAMAGE ) )
	{
		if ( RANDOM_LONG( 0, 99 ) > 90 || m_hEnemy == NULL )
			return GetScheduleOfType( SCHED_SMALL_FLINCH );
		if ( FOkToSpeak() )
			m_iSentence = MASSASSIN_SENT_COVER;
		return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	}
	if ( HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_MELEE_ATTACK1 );
	if ( ( pev->weapons & MASSASSIN_GRENADELAUNCHER ) && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
		OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
		return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
	if ( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
	{
		if ( InSquad() && MySquadLeader()->m_fEnemyEluded && !HasConditions( bits_COND_ENEMY_FACING_ME ) )
		{
			MySquadLeader()->m_fEnemyEluded = FALSE;
			return GetScheduleOfType( SCHED_MASSASSIN_FOUND_ENEMY );
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
			return GetScheduleOfType( SCHED_RANGE_ATTACK2 );
		if ( !OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) )
			return GetScheduleOfType( SCHED_STANDOFF );
		if ( FOkToSpeak() )
			m_iSentence = MASSASSIN_SENT_CHARGE;
		return GetScheduleOfType( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
	}
	if ( HasConditions( bits_COND_SEE_ENEMY ) && !HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		return GetScheduleOfType( SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE );
	return CBaseMonster::GetSchedule();
}

Schedule_t* CMOFAssassin::GetScheduleOfType( int Type )
{
	switch ( Type )
	{
	case SCHED_COMBAT_FACE: return slOFMAssassinCombatFace;
	case SCHED_VICTORY_DANCE: return InSquad() && !IsLeader() ? slOFMAssassinFail : slOFMAssassinVictoryDance;
	case SCHED_TAKE_COVER_FROM_ENEMY:
		if ( InSquad() )
		{
			if ( g_iSkillLevel == SKILL_HARD && HasConditions( bits_COND_CAN_RANGE_ATTACK2 ) &&
				OccupySlot( bits_SLOTS_HGRUNT_GRENADE ) )
				return slOFMAssassinTossGrenadeCover;
		}
		else if ( RANDOM_LONG( 0, 1 ) == 0 )
			return slOFMAssassinGrenadeCover;
		return slOFMAssassinTakeCover;
	case SCHED_TAKE_COVER_FROM_BEST_SOUND: return slOFMAssassinTakeCoverFromBestSound;
	case SCHED_RANGE_ATTACK1:
		if ( RANDOM_LONG( 0, 9 ) == 0 )
			m_fStanding = RANDOM_LONG( 0, 1 );
		return m_fStanding ? slOFMAssassinRangeAttack1B : slOFMAssassinRangeAttack1A;
	case SCHED_RANGE_ATTACK2: return slOFMAssassinRangeAttack2;
	case SCHED_FAIL: return m_hEnemy != NULL ? slOFMAssassinCombatFail : slOFMAssassinFail;
	case SCHED_MASSASSIN_SUPPRESS:
		if ( m_hEnemy->IsPlayer() && m_fFirstEncounter )
		{
			m_fFirstEncounter = FALSE;
			return slOFMAssassinSignalSuppress;
		}
		return slOFMAssassinSuppress;
	case SCHED_MASSASSIN_ESTABLISH_LINE_OF_FIRE: return slOFMAssassinEstablishLineOfFire;
	case SCHED_MASSASSIN_HIDE_RELOAD: return slOFMAssassinHideReload;
	case SCHED_MASSASSIN_RELOAD: return slOFMAssassinReload;
	case SCHED_MASSASSIN_SWEEP: return slOFMAssassinSweep;
	case SCHED_MASSASSIN_FOUND_ENEMY: return slOFMAssassinFoundEnemy;
	case SCHED_MASSASSIN_REPEL:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slOFMAssassinRepel;
	case SCHED_MASSASSIN_REPEL_ATTACK:
		if ( pev->velocity.z > -128 )
			pev->velocity.z -= 32;
		return slOFMAssassinRepelAttack;
	case SCHED_MASSASSIN_REPEL_LAND: return slOFMAssassinRepelLand;
	case SCHED_MASSASSIN_WAIT_FACE_ENEMY: return slOFMAssassinWaitInCover;
	case SCHED_MASSASSIN_TAKECOVER_FAILED:
		return GetScheduleOfType( HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) &&
			OccupySlot( bits_SLOTS_HGRUNT_ENGAGE ) ? SCHED_RANGE_ATTACK1 : SCHED_FAIL );
	case SCHED_MASSASSIN_ELOF_FAIL: return GetScheduleOfType( SCHED_TAKE_COVER_FROM_ENEMY );
	default: return CSquadMonster::GetScheduleOfType( Type );
	}
}

class CMOFAssassinRepel : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void EXPORT RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	int m_iSpriteTexture;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_assassin_repel, CMOFAssassinRepel );

void CMOFAssassinRepel::Spawn()
{
	Precache();
	pev->solid = SOLID_NOT;
	SetUse( &CMOFAssassinRepel::RepelUse );
}

void CMOFAssassinRepel::Precache()
{
	UTIL_PrecacheOther( "monster_male_assassin" );
	m_iSpriteTexture = PRECACHE_MODEL( "sprites/rope.spr" );
}

void CMOFAssassinRepel::RepelUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	(void)pActivator;
	(void)pCaller;
	(void)useType;
	(void)value;
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, -4096 ), dont_ignore_monsters, edict(), &tr );
	CBaseMonster* pAssassin = Create( "monster_male_assassin", pev->origin, pev->angles )->MyMonsterPointer();
	pAssassin->pev->movetype = MOVETYPE_FLY;
	pAssassin->pev->velocity = Vector( 0, 0, RANDOM_FLOAT( -196, -128 ) );
	pAssassin->SetActivity( ACT_GLIDE );
	pAssassin->m_vecLastPosition = tr.vecEndPos;
	CBeam* pBeam = CBeam::BeamCreate( "sprites/rope.spr", 10 );
	pBeam->PointEntInit( pev->origin + Vector( 0, 0, 112 ), pAssassin->entindex() );
	pBeam->SetFlags( BEAM_FSOLID );
	pBeam->SetColor( 255, 255, 255 );
	pBeam->SetThink( &CBaseEntity::SUB_Remove );
	pBeam->pev->nextthink = gpGlobals->time + -4096 * tr.flFraction / pAssassin->pev->velocity.z + 0.5;
	UTIL_Remove( this );
}

class CDeadMOFAssassin : public CBaseMonster
{
public:
	void Spawn() override;
	int Classify() override { return CLASS_HUMAN_MILITARY; }
	void KeyValue( KeyValueData* pkvd ) override;
	int m_iPose;
	static const char* m_szPoses[];
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_massassin_dead, CDeadMOFAssassin );

const char* CDeadMOFAssassin::m_szPoses[] = { "deadstomach", "deadside", "deadsitting" };

void CDeadMOFAssassin::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "pose" ) )
	{
		m_iPose = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		CBaseMonster::KeyValue( pkvd );
}

void CDeadMOFAssassin::Spawn()
{
	PRECACHE_MODEL( "models/massn.mdl" );
	SET_MODEL( edict(), "models/massn.mdl" );
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
	case 0: pev->skin = 0; pev->body = 0; break;
	case 1: pev->body = 0; pev->skin = 0; pev->body = 2; break;
	case 2: pev->body = 0; pev->skin = 0; pev->body = 7; break;
	case 3: pev->body = 0; pev->skin = 0; pev->body = 8; break;
	}
	MonsterInitDead();
}
