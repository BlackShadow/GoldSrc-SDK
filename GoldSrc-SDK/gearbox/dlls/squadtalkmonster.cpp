#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "squadtalkmonster.h"
#include "defaultai.h"
#include "plane.h"
#include "saverestore.h"

extern Schedule_t slChaseEnemyFailed[];

TYPEDESCRIPTION COFSquadTalkMonster::m_SaveData[] =
{
	DEFINE_FIELD( COFSquadTalkMonster, m_hSquadLeader, FIELD_EHANDLE ),
	DEFINE_ARRAY( COFSquadTalkMonster, m_hSquadMember, FIELD_EHANDLE, MAX_SQUAD_MEMBERS - 1 ),
	DEFINE_FIELD( COFSquadTalkMonster, m_fEnemyEluded, FIELD_BOOLEAN ),
	DEFINE_FIELD( COFSquadTalkMonster, m_hWaitMedic, FIELD_EHANDLE ),
	DEFINE_FIELD( COFSquadTalkMonster, m_flMedicWaitTime, FIELD_FLOAT ),
	DEFINE_FIELD( COFSquadTalkMonster, m_iMySlot, FIELD_INTEGER ),
};

IMPLEMENT_SAVERESTORE( COFSquadTalkMonster, COFAllyMonster );

char* COFSquadTalkMonster::m_szSquadFriends[] =
{
	"monster_human_grunt_ally",
};

BOOL COFSquadTalkMonster::OccupySlot( int iDesiredSlots )
{
	if ( !InSquad() )
		return TRUE;
	if ( SquadEnemySplit() )
	{
		m_iMySlot = bits_SLOT_SQUAD_SPLIT;
		return TRUE;
	}
	COFSquadTalkMonster* pLeader = MySquadLeader();
	if ( !( iDesiredSlots ^ pLeader->m_afSquadSlots ) )
		return FALSE;
	for ( int i = 0; i < NUM_SLOTS; i++ )
	{
		int mask = 1 << i;
		if ( ( iDesiredSlots & mask ) && !( pLeader->m_afSquadSlots & mask ) )
		{
			pLeader->m_afSquadSlots |= mask;
			m_iMySlot = mask;
			return TRUE;
		}
	}
	return FALSE;
}

void COFSquadTalkMonster::VacateSlot()
{
	if ( m_iMySlot != bits_NO_SLOT && InSquad() )
	{
		MySquadLeader()->m_afSquadSlots &= ~m_iMySlot;
		m_iMySlot = bits_NO_SLOT;
	}
}

void COFSquadTalkMonster::ScheduleChange()
{
	VacateSlot();
}

void COFSquadTalkMonster::Killed( entvars_t* pevAttacker, int iGib )
{
	VacateSlot();
	if ( InSquad() )
		MySquadLeader()->SquadRemove( this );
	COFAllyMonster::Killed( pevAttacker, iGib );
}

void COFSquadTalkMonster::SquadRemove( COFSquadTalkMonster* pRemove )
{
	ASSERT( pRemove != NULL );
	ASSERT( IsLeader() );
	ASSERT( pRemove->m_hSquadLeader == this );
	if ( pRemove == MySquadLeader() )
	{
		for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
		{
			COFSquadTalkMonster* pMember = MySquadMember( i );
			if ( pMember )
			{
				pMember->m_hSquadLeader = NULL;
				m_hSquadMember[i] = NULL;
			}
		}
	}
	else
	{
		COFSquadTalkMonster* pLeader = MySquadLeader();
		for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
		{
			if ( pLeader->m_hSquadMember[i] == this )
			{
				pLeader->m_hSquadMember[i] = NULL;
				break;
			}
		}
	}
	pRemove->m_hSquadLeader = NULL;
}

BOOL COFSquadTalkMonster::SquadAdd( COFSquadTalkMonster* pAdd )
{
	ASSERT( pAdd != NULL );
	ASSERT( !pAdd->InSquad() );
	ASSERT( IsLeader() );
	for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
	{
		if ( !m_hSquadMember[i] )
		{
			m_hSquadMember[i] = pAdd;
			pAdd->m_hSquadLeader = this;
			return TRUE;
		}
	}
	return FALSE;
}

int COFSquadTalkMonster::SquadRecruit( int searchRadius, int maxMembers )
{
	int myClass = Classify();
	if ( InSquad() || maxMembers < 2 )
		return 0;
	m_hSquadLeader = this;
	int count = 1;
	CBaseEntity* pEntity = NULL;
	if ( pev->netname )
	{
		while ( ( pEntity = UTIL_FindEntityByString( pEntity, "netname", STRING( pev->netname ) ) ) != NULL )
		{
			COFSquadTalkMonster* pRecruit = (COFSquadTalkMonster*)pEntity;
			if ( pRecruit->InSquad() || pRecruit->Classify() != myClass || pRecruit == this )
				continue;
			if ( !SquadAdd( pRecruit ) )
				break;
			count++;
		}
	}
	else
	{
		while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, (float)searchRadius ) ) != NULL )
		{
			COFSquadTalkMonster* pRecruit = pEntity->MySquadTalkMonsterPointer();
			if ( !pRecruit || pRecruit == this || !pRecruit->IsAlive() || pRecruit->m_pCine ||
				pRecruit->InSquad() || pRecruit->Classify() != myClass )
				continue;
			if ( myClass == CLASS_ALIEN_MONSTER && !FClassnameIs( pRecruit->pev, STRING( pev->classname ) ) )
				continue;
			if ( pRecruit->pev->netname )
				continue;
			TraceResult tr;
			UTIL_TraceLine( pev->origin + pev->view_ofs, pRecruit->pev->origin + pev->view_ofs,
				ignore_monsters, pRecruit->edict(), &tr );
			if ( tr.flFraction != 1.0 )
				continue;
			if ( !SquadAdd( pRecruit ) )
				break;
			count++;
		}
	}
	if ( count == 1 )
		m_hSquadLeader = NULL;
	return count;
}

void COFSquadTalkMonster::StartMonster()
{
	CBaseMonster::StartMonster();
	if ( ( m_afCapability & bits_CAP_SQUAD ) && !InSquad() )
	{
		if ( pev->netname && !( pev->spawnflags & SF_SQUADMONSTER_LEADER ) )
			return;
		int count = SquadRecruit( 1024, MAX_SQUAD_MEMBERS );
		ALERT( at_aiconsole, "Squad of %d %s formed\n", count, STRING( pev->classname ) );
	}
	m_flLastHitByPlayer = gpGlobals->time;
	m_iPlayerHits = 0;
}

int COFSquadTalkMonster::SquadCount()
{
	if ( !InSquad() )
		return 0;
	int count = 0;
	COFSquadTalkMonster* pLeader = MySquadLeader();
	for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		if ( pLeader->MySquadMember( i ) )
			count++;
	}
	return count;
}

void COFSquadTalkMonster::SquadPasteEnemyInfo()
{
	m_vecEnemyLKP = MySquadLeader()->m_vecEnemyLKP;
}

void COFSquadTalkMonster::SquadCopyEnemyInfo()
{
	MySquadLeader()->m_vecEnemyLKP = m_vecEnemyLKP;
}

int COFSquadTalkMonster::CheckEnemy( CBaseEntity* pEnemy )
{
	int updated = CBaseMonster::CheckEnemy( m_hEnemy );
	if ( InSquad() && (CBaseEntity*)m_hEnemy == (CBaseEntity*)MySquadLeader()->m_hEnemy )
	{
		if ( updated )
			SquadCopyEnemyInfo();
		else
			SquadPasteEnemyInfo();
	}
	return updated;
}

BOOL COFSquadTalkMonster::SquadEnemySplit()
{
	if ( !InSquad() )
		return FALSE;
	COFSquadTalkMonster* pLeader = MySquadLeader();
	for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		COFSquadTalkMonster* pMember = pLeader->MySquadMember( i );
		if ( pMember && pMember->m_hEnemy != NULL && (CBaseEntity*)pMember->m_hEnemy != (CBaseEntity*)pLeader->m_hEnemy )
			return TRUE;
	}
	return FALSE;
}

BOOL COFSquadTalkMonster::SquadMemberInRange( const Vector& vecLocation, float flDist )
{
	if ( !InSquad() )
		return FALSE;
	COFSquadTalkMonster* pLeader = MySquadLeader();
	for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		COFSquadTalkMonster* pMember = pLeader->MySquadMember( i );
		if ( pMember && !( flDist < ( vecLocation - pMember->pev->origin ).Length2D() ) )
			return TRUE;
	}
	return FALSE;
}

BOOL COFSquadTalkMonster::FValidateCover( const Vector& vecCoverLocation )
{
	return !SquadMemberInRange( vecCoverLocation, 128 );
}

BOOL COFSquadTalkMonster::NoFriendlyFire()
{
	if ( !InSquad() )
		return TRUE;
	if ( m_hEnemy == NULL )
		return FALSE;
	UTIL_MakeVectors( UTIL_VecToAngles( m_hEnemy->Center() - pev->origin ) );
	Vector vecLeftSide = pev->origin - gpGlobals->v_right * ( pev->size.x * 1.5 );
	Vector vecRightSide = pev->origin + gpGlobals->v_right * ( pev->size.x * 1.5 );
	CPlane leftPlane;
	CPlane rightPlane;
	CPlane backPlane;
	leftPlane.InitializePlane( gpGlobals->v_right, vecLeftSide );
	rightPlane.InitializePlane( -gpGlobals->v_right, vecRightSide );
	backPlane.InitializePlane( gpGlobals->v_forward, pev->origin );
	COFSquadTalkMonster* pLeader = MySquadLeader();
	for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		COFSquadTalkMonster* pMember = pLeader->MySquadMember( i );
		if ( pMember && pMember != this && backPlane.PointInFront( pMember->pev->origin ) &&
			leftPlane.PointInFront( pMember->pev->origin ) && rightPlane.PointInFront( pMember->pev->origin ) )
			return FALSE;
	}
	return TRUE;
}

void COFSquadTalkMonster::SquadMakeEnemy( CBaseEntity* pEnemy )
{
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
		return;
	if ( !InSquad() )
	{
		if ( m_hEnemy != NULL )
			PushEnemy( m_hEnemy, m_vecEnemyLKP );
		ALERT( at_aiconsole, "Non-Squad friendly grunt adopted enemy of type %s\n", STRING( pEnemy->pev->classname ) );
		m_hEnemy = pEnemy;
		m_vecEnemyLKP = pEnemy->pev->origin;
		SetConditions( bits_COND_NEW_ENEMY );
		return;
	}
	if ( !pEnemy )
	{
		ALERT( at_console, "ERROR: SquadMakeEnemy() - pEnemy is NULL!\n" );
		return;
	}
	COFSquadTalkMonster* pLeader = MySquadLeader();
	if ( !pLeader )
	{
		ALERT( at_aiconsole, "No squad leader defined in MakeEnemy\n", STRING( pEnemy->pev->classname ) );
		return;
	}
	BOOL leaderFollowing = pLeader->IsFollowing();
	if ( !IsLeader() && leaderFollowing != IsFollowing() )
	{
		ALERT( at_aiconsole, "Squad Member is not leader, and following state doesn't match in MakeEnemy\n" );
		return;
	}
	for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
	{
		COFSquadTalkMonster* pMember = pLeader->MySquadMember( i );
		if ( pMember && pMember->IsFollowing() == leaderFollowing && pMember->m_hEnemy != pEnemy &&
			!pMember->HasConditions( bits_COND_SEE_ENEMY ) )
		{
			if ( pMember->m_hEnemy != NULL )
				pMember->PushEnemy( pMember->m_hEnemy, pMember->m_vecEnemyLKP );
			ALERT( at_aiconsole, "Non-Squad friendly grunt adopted enemy of type %s\n", STRING( pEnemy->pev->classname ) );
			pMember->m_hEnemy = pEnemy;
			pMember->m_vecEnemyLKP = pEnemy->pev->origin;
			pMember->SetConditions( bits_COND_NEW_ENEMY );
		}
	}
	if ( pLeader->m_hEnemy != pEnemy && !pLeader->HasConditions( bits_COND_SEE_ENEMY ) )
	{
		if ( pLeader->m_hEnemy != NULL )
			pLeader->PushEnemy( pLeader->m_hEnemy, pLeader->m_vecEnemyLKP );
		ALERT( at_aiconsole, "Squad Leader friendly grunt adopted enemy of type %s\n", STRING( pEnemy->pev->classname ) );
		pLeader->m_hEnemy = pEnemy;
		pLeader->m_vecEnemyLKP = pEnemy->pev->origin;
		pLeader->SetConditions( bits_COND_NEW_ENEMY );
	}
}

MONSTERSTATE COFSquadTalkMonster::GetIdealState()
{
	IScheduleFlags();
	if ( ( m_MonsterState == MONSTERSTATE_IDLE || m_MonsterState == MONSTERSTATE_ALERT ) &&
		HasConditions( bits_COND_NEW_ENEMY ) && InSquad() )
		SquadMakeEnemy( m_hEnemy );
	return CBaseMonster::GetIdealState();
}

Schedule_t* COFSquadTalkMonster::GetScheduleOfType( int iType )
{
	if ( iType == SCHED_CHASE_ENEMY_FAILED )
		return slChaseEnemyFailed;
	return COFAllyMonster::GetScheduleOfType( iType );
}

void COFSquadTalkMonster::SquadStartFollowing( CBaseEntity* pLeader )
{
	StartFollowing( pLeader );
	for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
	{
		COFSquadTalkMonster* pMember = MySquadMember( i );
		if ( pMember )
			pMember->StartFollowing( pLeader );
	}
}

void COFSquadTalkMonster::SquadStopFollowing( BOOL clearSchedule )
{
	StopFollowing( clearSchedule );
	for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
	{
		COFSquadTalkMonster* pMember = MySquadMember( i );
		if ( pMember )
			pMember->StopFollowing( clearSchedule );
	}
}

void COFSquadTalkMonster::LimitSquadFollowers( CBaseEntity* pPlayer, int maxFollowers )
{
	CBaseEntity* pEntity = NULL;
	int count = 0;
	while ( ( pEntity = EnumFriends( pEntity, 0, FALSE ) ) != NULL )
	{
		CBaseMonster* pMonster = pEntity->MyMonsterPointer();
		if ( pMonster && pMonster->m_hTargetEnt == pPlayer && ++count > maxFollowers )
			pMonster->StopFollowing( TRUE );
	}
}

void COFSquadTalkMonster::FollowerUse( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( gpGlobals->time < m_useTime || !pCaller || !pCaller->IsPlayer() )
		return;
	if ( pev->spawnflags & SF_MONSTER_PREDISASTER )
	{
		DeclineFollowing();
		return;
	}
	if ( !CanFollow() )
	{
		StopFollowing( TRUE );
		return;
	}
	LimitSquadFollowers( pCaller, 6 );
	if ( m_afMemory & bits_MEMORY_PROVOKED )
	{
		ALERT( at_console, "I'm not following you, you evil person!\n" );
		return;
	}
	StartFollowing( pCaller );
	m_bitsSaid |= bit_saidHelloPlayer;
}

COFSquadTalkMonster* COFSquadTalkMonster::MySquadMedic()
{
	for ( int i = 0; i < MAX_SQUAD_MEMBERS - 1; i++ )
	{
		COFSquadTalkMonster* pMember = MySquadMember( i );
		if ( pMember && FClassnameIs( pMember->pev, "monster_human_medic_ally" ) )
			return pMember;
	}
	return NULL;
}

COFSquadTalkMonster* COFSquadTalkMonster::FindSquadMedic( int searchRadius )
{
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, (float)searchRadius ) ) != NULL )
	{
		COFSquadTalkMonster* pMedic = pEntity->MySquadTalkMonsterPointer();
		if ( pMedic && pMedic != this && pMedic->IsAlive() && !pMedic->m_pCine &&
			FClassnameIs( pMedic->pev, "monster_human_medic_ally" ) )
			return pMedic;
	}
	return NULL;
}

int COFSquadTalkMonster::TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int bitsDamageType )
{
	if ( m_MonsterState == MONSTERSTATE_SCRIPT )
		return COFAllyMonster::TakeDamage( pevInflictor, pevAttacker, 0, bitsDamageType );
	if ( pev->deadflag == DEAD_NO && flDamage >= pev->max_health )
	{
		COFSquadTalkMonster* pLeader = MySquadLeader();
		for ( int i = 0; i < MAX_SQUAD_MEMBERS; i++ )
		{
			COFSquadTalkMonster* pMember = pLeader->MySquadMember( i );
			if ( !pMember || !pMember->IsAlive() || pMember->m_hEnemy != NULL )
				continue;
			if ( m_MonsterState != MONSTERSTATE_PRONE && ( pevAttacker->flags & FL_CLIENT ) )
			{
				pMember->Remember( bits_MEMORY_PROVOKED );
			}
			else
			{
				Vector vecDir = ( pevAttacker->origin + pevAttacker->view_ofs -
					( pMember->pev->origin + pMember->pev->view_ofs ) ).Normalize();
				g_vecAttackDir = vecDir;
				TraceResult tr;
				UTIL_TraceLine( pMember->pev->origin + pMember->pev->view_ofs,
					pevAttacker->origin + pevAttacker->view_ofs + vecDir * m_flDistLook,
					dont_ignore_monsters, pMember->edict(), &tr );
				if ( tr.flFraction == 1.0 )
				{
					m_IdealMonsterState = MONSTERSTATE_HUNT;
				}
				else
				{
					pMember->m_hEnemy = CBaseEntity::Instance( tr.pHit );
					pMember->m_vecEnemyLKP = pevAttacker->origin;
					pMember->SetConditions( bits_COND_NEW_ENEMY );
				}
			}
		}
	}
	m_flWaitFinished = gpGlobals->time;
	return COFAllyMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}
