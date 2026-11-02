#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "schedule.h"

#define ZOMBIE_AE_ATTACK_RIGHT	1
#define ZOMBIE_AE_ATTACK_LEFT		2
#define ZOMBIE_AE_ATTACK_BOTH		3
#define ZOMBIE_FLINCH_DELAY		2

class CZombieBarney : public CBaseMonster
{
public:
	void Spawn();
	void Precache();
	void SetYawSpeed();
	int Classify();
	void HandleAnimEvent( MonsterEvent_t* pEvent );
	int IgnoreConditions();

	float m_flNextFlinch;

	void PainSound();
	void AlertSound();
	void IdleSound();
	void AttackSound();

	static const char* pAttackSounds[];
	static const char* pIdleSounds[];
	static const char* pAlertSounds[];
	static const char* pPainSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackMissSounds[];

	BOOL CheckRangeAttack1( float, float ) { return FALSE; }
	BOOL CheckRangeAttack2( float, float ) { return FALSE; }
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType );
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( monster_zombie_barney, CZombieBarney );

const char* CZombieBarney::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav",
	"zombie/claw_strike2.wav",
	"zombie/claw_strike3.wav"
};

const char* CZombieBarney::pAttackMissSounds[] =
{
	"zombie/claw_miss1.wav",
	"zombie/claw_miss2.wav"
};

const char* CZombieBarney::pAttackSounds[] =
{
	"zombie/zo_attack1.wav",
	"zombie/zo_attack2.wav"
};

const char* CZombieBarney::pIdleSounds[] =
{
	"zombie/zo_idle1.wav",
	"zombie/zo_idle2.wav",
	"zombie/zo_idle3.wav",
	"zombie/zo_idle4.wav"
};

const char* CZombieBarney::pAlertSounds[] =
{
	"zombie/zo_alert10.wav",
	"zombie/zo_alert20.wav",
	"zombie/zo_alert30.wav"
};

const char* CZombieBarney::pPainSounds[] =
{
	"zombie/zo_pain1.wav",
	"zombie/zo_pain2.wav"
};

int CZombieBarney::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void CZombieBarney::SetYawSpeed()
{
	pev->yaw_speed = 120;
}

int CZombieBarney::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	if ( bitsDamageType == DMG_BULLET )
	{
		Vector vecDir = pev->origin - ( pevInflictor->absmin + pevInflictor->absmax ) * 0.5;
		vecDir = vecDir.Normalize();
		float flForce = DamageForce( flDamage );
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
	}

	if ( IsAlive() )
		PainSound();
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CZombieBarney::PainSound()
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );
	if ( RANDOM_LONG( 0, 5 ) < 2 )
	{
		const char* sample = pPainSounds[RANDOM_LONG( 0, ARRAYSIZE( pPainSounds ) - 1 )];
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, sample, 1.0, ATTN_NORM, 0, pitch );
	}
}

void CZombieBarney::AlertSound()
{
	int pitch = 95 + RANDOM_LONG( 0, 9 );
	const char* sample = pAlertSounds[RANDOM_LONG( 0, ARRAYSIZE( pAlertSounds ) - 1 )];
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, sample, 1.0, ATTN_NORM, 0, pitch );
}

void CZombieBarney::IdleSound()
{
	RANDOM_LONG( 0, 9 );
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	const char* sample = pIdleSounds[RANDOM_LONG( 0, ARRAYSIZE( pIdleSounds ) - 1 )];
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, sample, 1.0, ATTN_NORM, 0, pitch );
}

void CZombieBarney::AttackSound()
{
	int pitch = 100 + RANDOM_LONG( -5, 5 );
	const char* sample = pAttackSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackSounds ) - 1 )];
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, sample, 1.0, ATTN_NORM, 0, pitch );
}

void CZombieBarney::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	CBaseEntity* pHurt;
	switch ( pEvent->event )
	{
	case ZOMBIE_AE_ATTACK_RIGHT:
		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.zombieBarneyDmgOneSlash, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.z = -18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
		}
		break;

	case ZOMBIE_AE_ATTACK_LEFT:
		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.zombieBarneyDmgOneSlash, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.z = 18;
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
		}
		break;

	case ZOMBIE_AE_ATTACK_BOTH:
		pHurt = CheckTraceHullAttack( 70, (int)gSkillData.zombieBarneyDmgBothSlash, DMG_SLASH );
		if ( pHurt && ( pHurt->pev->flags & ( FL_MONSTER | FL_CLIENT ) ) )
		{
			pHurt->pev->punchangle.x = 5;
			pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
		}
		break;

	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		return;
	}

	int pitch = 100 + RANDOM_LONG( -5, 5 );
	const char* sample;
	if ( pHurt )
		sample = pAttackHitSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackHitSounds ) - 1 )];
	else
		sample = pAttackMissSounds[RANDOM_LONG( 0, ARRAYSIZE( pAttackMissSounds ) - 1 )];
	EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, sample, 1.0, ATTN_NORM, 0, pitch );

	if ( RANDOM_LONG( 0, 1 ) )
		AttackSound();
}

void CZombieBarney::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/zombie_barney.mdl" );
	UTIL_SetSize( pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_GREEN;
	pev->health = gSkillData.zombieBarneyHealth;
	pev->view_ofs = VEC_VIEW;
	m_flFieldOfView = 0.5;
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;
	MonsterInit();
}

void CZombieBarney::Precache()
{
	PRECACHE_MODEL( "models/zombie_barney.mdl" );
	for ( int i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackHitSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackMissSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackMissSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pIdleSounds ); i++ )
		PRECACHE_SOUND( (char*)pIdleSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char*)pAlertSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char*)pPainSounds[i] );
}

int CZombieBarney::IgnoreConditions()
{
	int iIgnore = CBaseMonster::IgnoreConditions();
	if ( m_Activity == ACT_MELEE_ATTACK1 )
	{
		if ( m_flNextFlinch >= gpGlobals->time )
			iIgnore |= bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE;
	}
	else if ( m_Activity == ACT_SMALL_FLINCH || m_Activity == ACT_BIG_FLINCH )
	{
		if ( m_flNextFlinch < gpGlobals->time )
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
	}
	return iIgnore;
}
