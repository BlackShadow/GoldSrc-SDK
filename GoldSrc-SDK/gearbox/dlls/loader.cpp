#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"
#include "animation.h"

class COFLoader : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	int Classify();
	int ISoundMask();
	void SetYawSpeed();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	void StartTask( Task_t* pTask );
	void SetTurnActivity();
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType );
};

LINK_ENTITY_TO_CLASS( monster_op4loader, COFLoader );

int COFLoader::Classify()
{
	return CLASS_PLAYER_ALLY;
}

int COFLoader::ISoundMask()
{
	return 0;
}

void COFLoader::SetYawSpeed()
{
	pev->yaw_speed = 90;
}

void COFLoader::Precache()
{
	PRECACHE_MODEL( "models/loader.mdl" );
	PRECACHE_SOUND( "ambience/loader_step1.wav" );
	PRECACHE_SOUND( "ambience/loader_hydra1.wav" );
}

void COFLoader::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/loader.mdl" );
	if ( FStrEq( STRING( pev->model ), "models/player.mdl" ) ||
		FStrEq( STRING( pev->model ), "models/holo.mdl" ) )
		UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
	else
		UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = DONT_BLEED;
	pev->health = 8;
	m_MonsterState = MONSTERSTATE_NONE;
	m_flFieldOfView = 0.5;
	pev->takedamage = DAMAGE_NO;
	MonsterInit();
}

int COFLoader::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	return 1;
}

void COFLoader::TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir, TraceResult* ptr, int bitsDamageType )
{
	UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
}

void COFLoader::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	CBaseMonster::HandleAnimEvent( pEvent );
}

void COFLoader::StartTask( Task_t* pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_TURN_LEFT:
		pev->ideal_yaw = UTIL_AngleMod( UTIL_AngleMod( pev->angles.y ) + pTask->flData );
		SetTurnActivity();
		break;
	case TASK_TURN_RIGHT:
		pev->ideal_yaw = UTIL_AngleMod( UTIL_AngleMod( pev->angles.y ) - pTask->flData );
		SetTurnActivity();
		break;
	default:
		CBaseMonster::StartTask( pTask );
		break;
	}
}

void COFLoader::SetTurnActivity()
{
	float yaw = FlYawDiff();
	if ( !( yaw > -45 ) && LookupActivity( ACT_TURN_RIGHT ) != ACTIVITY_NOT_AVAILABLE )
		m_IdealActivity = ACT_TURN_RIGHT;
	else if ( yaw > 45 && LookupActivity( ACT_TURN_LEFT ) != ACTIVITY_NOT_AVAILABLE )
		m_IdealActivity = ACT_TURN_LEFT;
}
