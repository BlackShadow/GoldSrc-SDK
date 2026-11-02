#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "effects.h"
#include "weapons.h"

#define bits_MEMORY_ADVANCE_NODE		bits_MEMORY_CUSTOM2
#define bits_MEMORY_COMPLETED_NODE	bits_MEMORY_CUSTOM3
#define bits_COND_PITWORM_SLOWED		bits_COND_SPECIAL2

int gSpikeSprite;
int gSpikeDebrisSprite;

class COFInfoPW : public CPointEntity
{
public:
	void Spawn() override;
	void KeyValue( KeyValueData* pkvd ) override;
	void EXPORT StartNode();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	int m_preSequence;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class COFPitWorm : public CBaseMonster
{
public:
	void Spawn() override;
	void Precache() override;
	void Activate() override;
	void KeyValue( KeyValueData* pkvd ) override;
	int Classify() override;
	void SetYawSpeed() override;
	void StartMonster() override;
	void RunAI() override;
	void Move( float flInterval ) override;
	BOOL CheckMeleeAttack1( float flDot, float flDist ) override;
	BOOL CheckRangeAttack1( float flDot, float flDist ) override;
	void HandleAnimEvent( MonsterEvent_t* pEvent ) override;
	int TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType ) override;
	void TraceAttack( entvars_t* pevAttacker, float flDamage, Vector vecDir,
		TraceResult* ptr, int bitsDamageType ) override;
	void SetObjectCollisionBox() override;
	void StartupUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void ShootBeam();
	void StrafeBeam();
	void TrackEnemy();
	void UpdateEye();
	int Save( CSave& save ) override;
	int Restore( CRestore& restore ) override;
	static TYPEDESCRIPTION m_SaveData[];
	static const char* pFootSounds[];
	static const char* pPainSounds[];
	static const char* pAlertSounds[];
	static const char* pBirthSounds[];
	static const char* pAttackHitSounds[];
	static const char* pAttackSounds[];
	static const char* pDeathSounds[];
	static const char* pSackSounds[];
	static const char* pChildDieSounds[];

	float		m_nodeTime;
	float		m_spikeTime;
	float		m_painSoundTime;
	Activity	m_slowMode;
	float		m_slowTime;
	float		m_flHeadYaw;
	float		m_flHeadPitch;
	float		m_flIdealHeadYaw;
	float		m_flIdealHeadPitch;
	Vector		m_posTarget;
	Vector		m_vecTarget;
	CBeam*		m_pBeam;
	Vector		m_posBeam;
	Vector		m_vecBeam;
	Vector		m_angleBeam;
	float		m_offsetBeam;
	float		m_flBeamExpireTime;
	float		m_flBeamDir;
	Activity	m_NextActivity;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( info_pitworm, COFInfoPW );
LINK_ENTITY_TO_CLASS( monster_pitworm, COFPitWorm );

TYPEDESCRIPTION COFInfoPW::m_SaveData[] =
{
	DEFINE_FIELD( COFInfoPW, m_preSequence, FIELD_STRING )
};

IMPLEMENT_SAVERESTORE( COFInfoPW, CBaseEntity );

TYPEDESCRIPTION COFPitWorm::m_SaveData[] =
{
	DEFINE_FIELD( COFPitWorm, m_nodeTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWorm, m_spikeTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWorm, m_painSoundTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWorm, m_slowMode, FIELD_INTEGER ),
	DEFINE_FIELD( COFPitWorm, m_slowTime, FIELD_TIME ),
	DEFINE_FIELD( COFPitWorm, m_flHeadYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_flHeadPitch, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_flIdealHeadYaw, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_flIdealHeadPitch, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_posTarget, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWorm, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWorm, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFPitWorm, m_posBeam, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWorm, m_vecBeam, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWorm, m_angleBeam, FIELD_VECTOR ),
	DEFINE_FIELD( COFPitWorm, m_offsetBeam, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_flBeamExpireTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_flBeamDir, FIELD_FLOAT ),
	DEFINE_FIELD( COFPitWorm, m_NextActivity, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( COFPitWorm, CBaseMonster );

const char* COFPitWorm::pFootSounds[] =
{
	"gonarch/gon_step1.wav", "gonarch/gon_step2.wav", "gonarch/gon_step3.wav"
};
const char* COFPitWorm::pPainSounds[] =
{
	"gonarch/gon_pain2.wav", "gonarch/gon_pain4.wav", "gonarch/gon_pain5.wav"
};
const char* COFPitWorm::pAlertSounds[] =
{
	"gonarch/gon_alert1.wav", "gonarch/gon_alert2.wav", "gonarch/gon_alert3.wav"
};
const char* COFPitWorm::pBirthSounds[] =
{
	"gonarch/gon_birth1.wav", "gonarch/gon_birth2.wav", "gonarch/gon_birth3.wav"
};
const char* COFPitWorm::pAttackHitSounds[] =
{
	"zombie/claw_strike1.wav", "zombie/claw_strike2.wav", "zombie/claw_strike3.wav"
};
const char* COFPitWorm::pAttackSounds[] =
{
	"gonarch/gon_attack1.wav", "gonarch/gon_attack2.wav", "gonarch/gon_attack3.wav"
};
const char* COFPitWorm::pDeathSounds[] = { "gonarch/gon_die1.wav" };
const char* COFPitWorm::pSackSounds[] =
{
	"gonarch/gon_sack1.wav", "gonarch/gon_sack2.wav", "gonarch/gon_sack3.wav"
};
const char* COFPitWorm::pChildDieSounds[] =
{
	"gonarch/gon_childdie1.wav", "gonarch/gon_childdie2.wav", "gonarch/gon_childdie3.wav"
};

void COFInfoPW::Spawn()
{
	SetThink( &COFInfoPW::StartNode );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
}

void COFInfoPW::StartNode()
{
	pev->origin.z += 1;
	DROP_TO_FLOOR( ENT( pev ) );
}

void COFInfoPW::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "radius" ) )
		pev->scale = (float)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "reachdelay" ) )
		pev->speed = (float)atof( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "reachtarget" ) )
		pev->message = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "reachsequence" ) )
		pev->netname = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "presequence" ) )
		m_preSequence = ALLOC_STRING( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

int COFPitWorm::Classify()
{
	return CLASS_ALIEN_MONSTER;
}

void COFPitWorm::SetYawSpeed()
{
	pev->yaw_speed = m_Activity == ACT_IDLE ? 100 : 90;
}

int COFPitWorm::TakeDamage( entvars_t*, entvars_t*, float flDamage, int bitsDamageType )
{
	if ( bitsDamageType & DMG_ACID )
		flDamage = 0;
	if ( HasMemory( bits_MEMORY_PATH_FINISHED ) || flDamage < pev->health )
		return 0;
	pev->health = flDamage + 1;
	Remember( bits_MEMORY_ADVANCE_NODE | bits_MEMORY_COMPLETED_NODE );
	ALERT( at_aiconsole, "BM: Finished node health!!!\n" );
	return 0;
}

void COFPitWorm::Precache()
{
	PRECACHE_MODEL( "models/pit_worm.mdl" );
	for ( int i = 0; i < ARRAYSIZE( pChildDieSounds ); i++ )
		PRECACHE_SOUND( (char*)pChildDieSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pSackSounds ); i++ )
		PRECACHE_SOUND( (char*)pSackSounds[i] );
	PRECACHE_SOUND( (char*)pDeathSounds[0] );
	for ( int i = 0; i < ARRAYSIZE( pAttackSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAttackHitSounds ); i++ )
		PRECACHE_SOUND( (char*)pAttackHitSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pBirthSounds ); i++ )
		PRECACHE_SOUND( (char*)pBirthSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pAlertSounds ); i++ )
		PRECACHE_SOUND( (char*)pAlertSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pPainSounds ); i++ )
		PRECACHE_SOUND( (char*)pPainSounds[i] );
	for ( int i = 0; i < ARRAYSIZE( pFootSounds ); i++ )
		PRECACHE_SOUND( (char*)pFootSounds[i] );
	PRECACHE_MODEL( "sprites/xspark1.spr" );
	gSpikeSprite = PRECACHE_MODEL( "sprites/mommaspout.spr" );
	gSpikeDebrisSprite = PRECACHE_MODEL( "sprites/mommablob.spr" );
	PRECACHE_SOUND( "bullchicken/bc_acid1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit1.wav" );
	PRECACHE_SOUND( "bullchicken/bc_spithit2.wav" );
}

BOOL COFPitWorm::CheckMeleeAttack1( float flDot, float flDist )
{
	return flDot >= 0.7 && flDist <= 192;
}

void COFPitWorm::Activate()
{
	if ( m_hTargetEnt == NULL )
		Remember( bits_MEMORY_ADVANCE_NODE );
}

void COFPitWorm::StartMonster()
{
	if ( LookupActivity( ACT_RANGE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_RANGE_ATTACK1;
	if ( LookupActivity( ACT_RANGE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_RANGE_ATTACK2;
	if ( LookupActivity( ACT_MELEE_ATTACK1 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_MELEE_ATTACK1;
	if ( LookupActivity( ACT_MELEE_ATTACK2 ) != ACTIVITY_NOT_AVAILABLE )
		m_afCapability |= bits_CAP_MELEE_ATTACK2;
	if ( pev->movetype == MOVETYPE_FLY || ( pev->spawnflags & SF_MONSTER_FALL_TO_GROUND ) )
	{
		pev->flags &= ~FL_ONGROUND;
	}
	else
	{
		pev->origin.z += 1;
		DROP_TO_FLOOR( ENT( pev ) );
		if ( !WALK_MOVE( ENT( pev ), 0, 0, WALKMOVE_NORMAL ) )
		{
			ALERT( at_error, "Monster %s stuck in wall--level design error", STRING( pev->classname ) );
			pev->effects = EF_BRIGHTFIELD;
		}
	}
	if ( !FStringNull( pev->target ) )
	{
		m_pGoalEnt = CBaseEntity::Instance( FIND_ENTITY_BY_TARGETNAME( NULL, STRING( pev->target ) ) );
		if ( m_pGoalEnt == NULL )
		{
			ALERT( at_error, "ReadyMonster()--%s couldn't find target %s", STRING( pev->classname ), STRING( pev->target ) );
		}
		else
		{
			MakeIdealYaw( m_pGoalEnt->pev->origin );
			m_movementGoal = MOVEGOAL_PATHCORNER;
			m_movementActivity = m_slowMode;
			if ( !FRefreshRoute() )
				ALERT( at_aiconsole, "Can't Create Route!\n" );
			SetState( MONSTERSTATE_IDLE );
		}
	}
	Vector eyePos, eyeAngles;
	GetAttachment( 0, eyePos, eyeAngles );
	pev->view_ofs = eyePos - pev->origin;
	SetThink( &COFPitWorm::CallMonsterThink );
	pev->nextthink = gpGlobals->time + RANDOM_FLOAT( 0.1, 0.4 );
}

void COFPitWorm::Spawn()
{
	Precache();
	SET_MODEL( ENT( pev ), "models/pit_worm.mdl" );
	UTIL_SetSize( pev, Vector( -32, -32, 0 ), Vector( 32, 32, 64 ) );
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	m_bloodColor = BLOOD_COLOR_YELLOW;
	pev->health = 150 * gSkillData.bigmommaHealthFactor;
	pev->view_ofs = Vector( 0, 0, 128 );
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_TURN_HEAD | bits_CAP_RANGE_ATTACK1 | bits_CAP_MELEE_ATTACK1;
	m_slowMode = ACT_RUN;
	m_flFieldOfView = 0.3f;
	MonsterInit();
	m_flDistLook = 1024;
	m_slowTime = 0;
	m_pBeam = NULL;
}

void COFPitWorm::KeyValue( KeyValueData* pkvd )
{
	CBaseMonster::KeyValue( pkvd );
}

BOOL COFPitWorm::CheckRangeAttack1( float, float flDist )
{
	if ( flDist > 1024 || gpGlobals->time <= m_spikeTime )
		return FALSE;
	CBaseEntity* pEnemy = m_hEnemy;
	if ( pEnemy == NULL || !FVisible( pEnemy ) )
		return FALSE;
	Vector origin, angles;
	TraceResult trace;
	GetAttachment( 0, origin, angles );
	UTIL_TraceLine( origin, pEnemy->BodyTarget( origin ), dont_ignore_monsters, ENT( pev ), &trace );
	return TRUE;
}

void COFPitWorm::TraceAttack( entvars_t*, float flDamage, Vector vecDir, TraceResult* ptr, int )
{
	if ( ptr->iHitgroup == 1 )
	{
		if ( gpGlobals->time > m_painSoundTime )
		{
			m_painSoundTime = gpGlobals->time + RANDOM_LONG( 1, 3 );
			int pitch = RANDOM_LONG( 95, 105 );
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, pPainSounds[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM, 0, pitch );
			if ( gpGlobals->time > m_slowTime )
			{
				m_slowMode = flDamage >= 50 ? ACT_WALK_HURT : ACT_WALK;
				m_IdealActivity = m_slowMode;
				m_movementActivity = m_slowMode;
				m_slowTime = gpGlobals->time + 3;
				SetConditions( bits_COND_PITWORM_SLOWED );
			}
			UTIL_BloodStream( ptr->vecEndPos, vecDir, m_bloodColor, 128 );
			UTIL_BloodDecalTrace( ptr, m_bloodColor );
		}
	}
	else if ( pev->dmgtime != gpGlobals->time || RANDOM_LONG( 0, 10 ) <= 0 )
	{
		UTIL_Ricochet( ptr->vecEndPos, RANDOM_FLOAT( 1, 2 ) );
		pev->dmgtime = gpGlobals->time;
	}
}

void COFPitWorm::Move( float flInterval )
{
	if ( FRouteClear() && ( m_movementGoal == MOVEGOAL_NONE || !FRefreshRoute() ) )
	{
		ALERT( at_aiconsole, "Tried to move with no route!\n" );
		TaskFail();
		return;
	}
	if ( !( m_flMoveWaitFinished <= gpGlobals->time ) )
		return;
	Vector direction = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Normalize();
	float waypointDist = ( m_Route[m_iRouteIndex].vecLocation - pev->origin ).Length2D();
	if ( waypointDist > 32 )
	{
		MakeIdealYaw( m_Route[m_iRouteIndex].vecLocation );
		ChangeYaw( (int)pev->yaw_speed );
	}
	float checkDist = waypointDist < 200 ? waypointDist : 200;
	CBaseEntity* pTarget = NULL;
	int routeType = m_Route[m_iRouteIndex].iType & ~bits_MF_NOT_TO_MASK;
	if ( routeType == bits_MF_TO_ENEMY )
		pTarget = m_hEnemy;
	else if ( routeType == bits_MF_TO_TARGETENT )
		pTarget = m_hTargetEnt;
	float distance = 0;
	if ( CheckLocalMove( pev->origin, pev->origin + direction * checkDist, pTarget, &distance ) != LOCALMOVE_VALID )
	{
		CBaseEntity* pBlocker = CBaseEntity::Instance( gpGlobals->trace_ent );
		if ( pBlocker )
		{
			DispatchBlocked( edict(), pBlocker->edict() );
			m_IdealActivity = ACT_MELEE_ATTACK1;
			m_movementActivity = ACT_MELEE_ATTACK1;
			if ( m_moveWaitTime > 0 && pBlocker->IsMoving() && !pBlocker->IsPlayer()
				&& gpGlobals->time - m_flMoveWaitFinished > 3 )
			{
				if ( m_flGroundSpeed > distance )
				{
					m_flMoveWaitFinished = gpGlobals->time + m_moveWaitTime;
					return;
				}
				m_IdealActivity = m_slowMode;
				m_movementActivity = m_slowMode;
			}
		}
		else
		{
			m_IdealActivity = m_Activity == ACT_MELEE_ATTACK1 ? ACT_MELEE_ATTACK1 : ACT_IDLE;
			m_movementActivity = m_IdealActivity;
		}
	}
	if ( ShouldAdvanceRoute( waypointDist ) )
		AdvanceRoute( waypointDist );
	if ( m_flMoveWaitFinished > gpGlobals->time )
	{
		Stop();
		return;
	}
	if ( flInterval * m_flGroundSpeed > checkDist )
		flInterval = checkDist / m_flGroundSpeed;
	MoveExecute( pTarget, direction, flInterval );
	if ( m_movementGoal == MOVEGOAL_NONE )
	{
		Stop();
		RouteClear();
	}
}

void COFPitWorm::ShootBeam()
{
	if ( m_hEnemy == NULL )
		return;
	m_flBeamDir = RANDOM_LONG( 0, 1 ) ? -1.0f : 1.0f;
	m_offsetBeam = -m_flBeamDir * 40;
	Vector eyePos, eyeAngles;
	GetAttachment( 0, eyePos, eyeAngles );
	m_vecBeam = ( m_posTarget - eyePos ).Normalize();
	m_angleBeam = UTIL_VecToAngles( m_vecBeam );
	UTIL_MakeVectors( m_angleBeam );
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;
	Vector end = eyePos + m_vecBeam * 2048 + gpGlobals->v_right * m_offsetBeam;
	TraceResult trace;
	UTIL_TraceLine( eyePos, end, dont_ignore_monsters, ENT( pev ), &trace );
	m_pBeam = CBeam::BeamCreate( "sprites/laserbeam.spr", 80 );
	if ( m_pBeam == NULL )
		return;
	m_pBeam->PointEntInit( trace.vecEndPos, entindex() );
	m_pBeam->SetEndAttachment( 1 );
	m_pBeam->SetColor( 0, 255, 32 );
	m_pBeam->SetBrightness( 192 );
	m_pBeam->SetWidth( 32 );
	m_pBeam->pev->spawnflags |= SF_BEAM_SPARKEND;
	CBaseEntity* pHit = CBaseEntity::Instance( trace.pHit );
	if ( pHit && pHit->pev->takedamage )
	{
		ClearMultiDamage();
		pHit->TraceAttack( pev, 10, m_vecBeam, &trace, DMG_ENERGYBEAM );
		pHit->TakeDamage( pev, pev, 10, DMG_ENERGYBEAM );
	}
	else if ( trace.flFraction != 1 )
	{
		UTIL_DecalTrace( &trace, RANDOM_LONG( 0, 4 ) );
		m_pBeam->DoSparks( trace.vecEndPos, trace.vecEndPos );
	}
	m_flBeamExpireTime = gpGlobals->time + 0.5f;
	m_flIdealHeadYaw += 50 * m_flBeamDir;
}

void COFPitWorm::StrafeBeam()
{
	m_offsetBeam += 20 * m_flBeamDir;
	Vector eyePos, eyeAngles;
	GetAttachment( 0, eyePos, eyeAngles );
	m_vecBeam = ( m_posTarget - eyePos ).Normalize();
	m_angleBeam = UTIL_VecToAngles( m_vecBeam );
	UTIL_MakeVectors( m_angleBeam );
	m_vecBeam = gpGlobals->v_forward;
	m_vecBeam.z = -m_vecBeam.z;
	Vector end = eyePos + m_vecBeam * 2048 + gpGlobals->v_right * m_offsetBeam;
	TraceResult trace;
	UTIL_TraceLine( eyePos, end, dont_ignore_monsters, ENT( pev ), &trace );
	m_pBeam->pev->origin = trace.vecEndPos;
	CBaseEntity* pHit = CBaseEntity::Instance( trace.pHit );
	if ( pHit && pHit->pev->takedamage )
	{
		ClearMultiDamage();
		pHit->TraceAttack( pev, 10, m_vecBeam, &trace, DMG_ENERGYBEAM );
		pHit->TakeDamage( pev, pev, 10, DMG_ENERGYBEAM );
	}
	else if ( trace.flFraction != 1 )
	{
		UTIL_DecalTrace( &trace, RANDOM_LONG( 0, 4 ) );
		m_pBeam->DoSparks( trace.vecEndPos, trace.vecEndPos );
	}
}

void COFPitWorm::HandleAnimEvent( MonsterEvent_t* pEvent )
{
	switch ( pEvent->event )
	{
	case 1:
		{
			UTIL_ScreenShake( pev->origin, 4, 3, 1, 750 );
			Vector forward, right;
			g_engfuncs.pfnAngleVectors( pev->angles, forward, right, NULL );
			Vector center = pev->origin + forward * 128;
			CBaseEntity* entities[8];
			int count = UTIL_EntitiesInBox( entities, ARRAYSIZE( entities ), center - Vector( 96, 96, 0 ),
				center + Vector( 96, 96, 128 ), FL_CLIENT | FL_MONSTER );
			for ( int i = 0; i < count; i++ )
			{
				CBaseEntity* pHit = entities[i];
				if ( pHit == this || pHit->pev->owner == edict() )
					continue;
				pHit->TakeDamage( pev, pev, gSkillData.bigmommaDmgSlash, DMG_CRUSH | DMG_SLASH );
				pHit->pev->punchangle.x = 15;
				pHit->pev->velocity = pHit->pev->velocity + forward * 220 + Vector( 0, 0, 200 );
				pHit->pev->flags &= ~FL_ONGROUND;
				int pitch = 100 + RANDOM_LONG( -5, 5 );
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM, 0, pitch );
				break;
			}
		}
		break;
	case 2:
		{
			m_posTarget = m_hEnemy->pev->origin;
			m_posTarget.z += 24;
			Vector eyePos, eyeAngles;
			GetAttachment( 0, eyePos, eyeAngles );
			m_vecTarget = ( m_posTarget - eyePos ).Normalize();
			m_posBeam = m_posTarget;
			m_vecBeam = m_vecTarget;
			m_angleBeam = UTIL_VecToAngles( m_vecBeam );
			ShootBeam();
		}
		break;
	case 3:
		pev->flags &= ~FL_ONGROUND;
		UTIL_SetOrigin( pev, pev->origin + Vector( 0, 0, 1 ) );
		UTIL_MakeVectors( pev->angles );
		pev->velocity = gpGlobals->v_up * 500 + gpGlobals->v_forward * 200;
		break;
	default:
		CBaseMonster::HandleAnimEvent( pEvent );
		break;
	}
}

void COFPitWorm::StartupUse( CBaseEntity*, CBaseEntity*, USE_TYPE, float )
{
	SetThink( &COFPitWorm::CallMonsterThink );
	pev->nextthink = (float)( gpGlobals->time + 0.1 );
	SetUse( NULL );
}

void COFPitWorm::UpdateEye()
{
	if ( m_flHeadYaw != m_flIdealHeadYaw )
	{
		if ( m_flIdealHeadYaw > m_flHeadYaw )
			m_flHeadYaw += min( m_flIdealHeadYaw - m_flHeadYaw, 5.0f );
		else
			m_flHeadYaw -= min( m_flHeadYaw - m_flIdealHeadYaw, 5.0f );
		SetBoneController( 1, m_flHeadYaw );
	}
	if ( m_flHeadPitch != m_flIdealHeadPitch )
	{
		if ( m_flIdealHeadPitch > m_flHeadPitch )
			m_flHeadPitch += min( m_flIdealHeadPitch - m_flHeadPitch, 5.0f );
		else
			m_flHeadPitch -= min( m_flHeadPitch - m_flIdealHeadPitch, 5.0f );
		SetBoneController( 0, m_flHeadPitch );
	}
}

void COFPitWorm::TrackEnemy()
{
	CBaseEntity* pEnemy = m_hEnemy;
	Vector eyePos, eyeAngles;
	GetAttachment( 0, eyePos, eyeAngles );
	eyePos.x = pev->origin.x;
	eyePos.y = pev->origin.y;
	Vector target = pEnemy->pev->origin + pEnemy->pev->view_ofs - eyePos;
	Vector angles = UTIL_VecToAngles( target );
	float pitch = angles.x - pev->angles.x;
	if ( pitch < 0 )
		pitch += 360;
	pitch = -pitch;
	if ( pitch > 180 )
		pitch -= 360;
	if ( pitch < -180 )
		pitch += 360;
	m_flHeadPitch = max( -45.0f, min( pitch, 45.0f ) );
	float yaw = -VecToYaw( pEnemy->pev->origin + pEnemy->pev->view_ofs - eyePos ) - pev->angles.y;
	if ( yaw < 0 )
		yaw += 360;
	if ( yaw > 180 )
		yaw -= 360;
	if ( yaw < -180 )
		yaw += 360;
	m_flHeadYaw = max( -45.0f, min( yaw, 45.0f ) );
}

void COFPitWorm::RunAI()
{
	if ( m_hEnemy == NULL )
	{
		Look( 1024 );
		if ( HasConditions( bits_COND_SEE_HATE | bits_COND_SEE_DISLIKE | bits_COND_SEE_NEMESIS ) )
		{
			CBaseEntity* pEnemy = BestVisibleEnemy();
			if ( (CBaseEntity*)m_hEnemy != pEnemy && pEnemy != NULL )
			{
				PushEnemy( m_hEnemy, m_vecEnemyLKP );
				SetConditions( bits_COND_NEW_ENEMY );
				m_hEnemy = pEnemy;
				m_vecEnemyLKP = m_hEnemy->pev->origin;
			}
		}
		if ( m_hEnemy == NULL && PopEnemy() )
			SetConditions( bits_COND_NEW_ENEMY );
	}
	if ( m_pGoalEnt == NULL && !FStringNull( pev->target ) )
	{
		m_pGoalEnt = UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
		m_movementGoal = MOVEGOAL_PATHCORNER;
		m_movementActivity = m_slowMode;
		if ( !FRefreshRoute() )
			ALERT( at_aiconsole, "Can't Create Route!\n" );
	}
	if ( m_hEnemy != NULL )
	{
		SetConditions( bits_COND_SEE_ENEMY );
		CheckEnemy( m_hEnemy );
		if ( !m_hEnemy->IsAlive() )
			m_hEnemy = NULL;
		else if ( m_hEnemy != NULL && m_pBeam == NULL )
			TrackEnemy();
	}
	UpdateEye();
	if ( m_pBeam != NULL )
	{
		if ( (CBaseEntity*)m_hEnemy == NULL || m_flBeamExpireTime <= gpGlobals->time )
		{
			UTIL_Remove( m_pBeam );
			m_pBeam = NULL;
		}
		else
		{
			StrafeBeam();
		}
	}
	if ( m_fSequenceFinished )
	{
		if ( m_hEnemy != NULL && HasConditions( bits_COND_CAN_RANGE_ATTACK1 ) )
		{
			m_IdealActivity = ACT_RANGE_ATTACK1;
			m_spikeTime = gpGlobals->time + RANDOM_FLOAT( 7, 10 );
		}
		else if ( m_hEnemy != NULL && HasConditions( bits_COND_CAN_MELEE_ATTACK1 ) )
		{
			m_IdealActivity = ACT_MELEE_ATTACK1;
		}
		else if ( m_Activity == ACT_MELEE_ATTACK1 || m_Activity == ACT_RANGE_ATTACK1 )
		{
			m_IdealActivity = m_movementGoal == MOVEGOAL_NONE ? ACT_IDLE : m_slowMode;
		}
		else if ( m_movementGoal == MOVEGOAL_NONE )
		{
			m_IdealActivity = ACT_IDLE;
			if ( m_Activity == ACT_IDLE )
				return;
		}
		else if ( HasConditions( bits_COND_PITWORM_SLOWED ) && gpGlobals->time > m_slowTime )
		{
			ClearConditions( bits_COND_PITWORM_SLOWED );
			m_slowMode = ACT_RUN;
			m_IdealActivity = ACT_RUN;
		}
		else
		{
			m_IdealActivity = m_slowMode;
		}
		if ( m_Activity == m_IdealActivity && ( m_Activity == ACT_MELEE_ATTACK1 || m_Activity == ACT_RANGE_ATTACK1 ) )
			m_IdealActivity = ACT_IDLE;
	}
	if ( m_Activity != m_IdealActivity )
	{
		m_movementActivity = m_IdealActivity;
		SetActivity( m_IdealActivity );
	}
}

void COFPitWorm::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -64, -64, 0 );
	pev->absmax = pev->origin + Vector( 64, 96, 128 );
}
