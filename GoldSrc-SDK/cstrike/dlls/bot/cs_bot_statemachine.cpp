#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"

void CCSBot::Attack( CBasePlayer* enemy )
{
	if ( enemy == NULL || cv_bot_zombie.value > 0.0f )
		return;
	CBasePlayerWeapon* weapon = GetActiveWeapon();
	if ( weapon && (weapon->m_fInReload || weapon->m_fInSpecialReload) )
		return;
	SetEnemy( enemy );
	if ( m_isAttacking )
		return;
	m_attackState.m_crouchAndHold = IsHiding() && m_hideState.m_atSpot && RANDOM_FLOAT( 0.0f, 100.0f ) < 60.0f;
	PrintIfWatched( "ATTACK BEGIN (reaction time = %g (+ update time), surprise time = %g, attack delay = %g)\n",
		m_profile->GetReactionTime(), m_surpriseDelay, m_profile->GetAttackDelay() );
	m_isAttacking = true;
	m_attackState.OnEnter( this );
	m_lastEnemyPosition = enemy->pev->origin;
	m_lastSawEnemyTime = gpGlobals->time;
	m_aimAccuracyTimestamp = gpGlobals->time;
	Vector angles = UTIL_VecToAngles( enemy->pev->origin - pev->origin );
	float difference = static_cast<float>( abs( static_cast<int>( m_desiredYaw - angles.y ) ) );
	while ( difference > 180.0f )
		difference -= 360.0f;
	if ( difference < 0.0f )
		difference = -difference;
	float fraction = difference / 180.0f;
	SetAimOffset( m_profile->GetSkill() / (fraction + 1.0f) );
	float now = gpGlobals->time;
	m_nextAimOffsetTime = RANDOM_FLOAT( fraction + 0.25f, 1.5f ) + now;
}

void CCSBot::SetState( BotState* state )
{
	PrintIfWatched( "SetState: %s -> %s\n", m_state ? m_state->GetName() : "NULL", state->GetName() );
	if ( m_isAttacking )
		StopAttacking();
	if ( m_state )
		m_state->OnExit( this );
	state->OnEnter( this );
	m_state = state;
	m_stateTimestamp = gpGlobals->time;
}

void CCSBot::StopAttacking()
{
	PrintIfWatched( "ATTACK END\n" );
	m_attackState.OnExit( this );
	m_isAttacking = false;
	if ( m_isFollowing )
		Idle();
}

void CCSBot::Idle()
{
	m_task = TASK_SEEK_AND_DESTROY;
	m_taskEntity = NULL;
	SetState( &m_idleState );
}

void CCSBot::Hunt()
{
	SetState( &m_huntState );
}

void CCSBot::Follow( CBasePlayer* leader )
{
	if ( leader == NULL )
		return;
	if ( !m_isFollowing || leader != static_cast<CBaseEntity*>( m_leader ) )
		m_followStartTime = gpGlobals->time;
	m_isFollowing = true;
	m_leader = leader;
	m_task = TASK_FOLLOW;
	m_taskEntity = NULL;
	m_followState.m_leader = leader;
	SetState( &m_followState );
}

void CCSBot::ContinueFollowing()
{
	m_task = TASK_FOLLOW;
	m_taskEntity = NULL;
	m_followState.m_leader = static_cast<CBaseEntity*>( m_leader );
	SetState( &m_followState );
}

void CCSBot::StopFollowing()
{
	m_isFollowing = false;
	m_leader = NULL;
	m_allowAutoFollowTime = gpGlobals->time + 10.0f;
}

void CCSBot::MoveTo( const Vector* position, RouteType route )
{
	m_moveToState.m_goal = *position;
	m_moveToState.m_route = route;
	SetState( &m_moveToState );
}

void CCSBot::UseEntity( CBaseEntity* entity )
{
	m_useEntityState.SetEntity( entity );
	SetState( &m_useEntityState );
}

void CCSBot::FetchBomb()
{
	SetState( &m_fetchBombState );
}

void CCSBot::PlantBomb()
{
	SetState( &m_plantBombState );
}

void CCSBot::DefuseBomb()
{
	SetState( &m_defuseBombState );
}

void CCSBot::EscapeFromBomb()
{
	m_task = TASK_ESCAPE_FROM_BOMB;
	m_taskEntity = NULL;
	SetState( &m_escapeFromBombState );
}

void CCSBot::InvestigateNoise()
{
	SetState( &m_investigateNoiseState );
}

void CCSBot::RescueHostages()
{
	m_task = TASK_RESCUE_HOSTAGES;
	m_taskEntity = NULL;
}

void CCSBot::Hide( CNavArea* area, float duration, float range, bool holdPosition )
{
	m_pathLength = 0;
	m_ladder = NULL;
	Vector position;
	if ( area )
		position = *area->GetCenter();
	else
	{
		position = pev->origin;
		area = m_lastKnownArea;
		if ( area == NULL )
		{
			PrintIfWatched( "Hide from area is NULL.\n" );
			Idle();
			return;
		}
	}
	m_hideState.m_maxRange = range;
	m_hideState.m_searchArea = area;
	m_hideState.m_holdPosition = holdPosition;
	m_hideState.m_duration = duration;
	const Vector* spot = FindNearbyHidingSpot( this, &position, area, range, IsSniper(), false );
	if ( spot == NULL )
	{
		PrintIfWatched( "No available hiding spots.\n" );
		spot = &pev->origin;
	}
	m_hideState.m_hidingSpot = *spot;
	if ( !ComputePath( TheNavAreaGrid.GetNavArea( spot ), spot, FASTEST_ROUTE ) )
	{
		PrintIfWatched( "Can't pathfind to hiding spot\n" );
		Idle();
		return;
	}
	SetState( &m_hideState );
}

void CCSBot::Hide( const Vector* position, float duration, bool holdPosition )
{
	CNavArea* area = TheNavAreaGrid.GetNearestNavArea( position );
	if ( area )
	{
		m_pathLength = 0;
		m_ladder = NULL;
		m_hideState.m_searchArea = area;
		m_hideState.m_maxRange = 750.0f;
		m_hideState.m_duration = duration;
		m_hideState.m_holdPosition = holdPosition;
		m_hideState.m_hidingSpot = *position;
		if ( ComputePath( area, position, FASTEST_ROUTE ) )
		{
			SetState( &m_hideState );
			return;
		}
		PrintIfWatched( "Can't pathfind to hiding spot\n" );
	}
	else
		PrintIfWatched( "Hiding spot off nav mesh\n" );
	Idle();
}

bool CCSBot::TryToHide( CNavArea* area, float duration, float range, bool holdPosition, bool useNearest )
{
	Vector position;
	if ( area )
		position = *area->GetCenter();
	else
	{
		position = pev->origin;
		area = m_lastKnownArea;
		if ( area == NULL )
		{
			PrintIfWatched( "Hide from area is NULL.\n" );
			return false;
		}
	}
	m_hideState.m_maxRange = range;
	m_hideState.m_searchArea = area;
	m_hideState.m_holdPosition = holdPosition;
	m_hideState.m_duration = duration;
	const Vector* spot = FindNearbyHidingSpot( this, &position, area, range, IsSniper(), useNearest );
	if ( spot == NULL )
	{
		PrintIfWatched( "No available hiding spots.\n" );
		return false;
	}
	m_hideState.m_hidingSpot = *spot;
	if ( !ComputePath( TheNavAreaGrid.GetNavArea( spot ), spot, FASTEST_ROUTE ) )
	{
		PrintIfWatched( "Can't pathfind to hiding spot\n" );
		return false;
	}
	SetState( &m_hideState );
	return true;
}

bool CCSBot::IsAttacking() const
{
	return m_isAttacking;
}

bool CCSBot::IsBuying() const
{
	return m_state == &m_buyState;
}

bool CCSBot::IsHiding() const
{
	return m_state == &m_hideState;
}

bool CCSBot::IsAtHidingSpot() const
{
	return IsHiding() && m_hideState.m_atSpot;
}

bool CCSBot::IsHunting() const
{
	return m_state == &m_huntState;
}

bool CCSBot::IsMovingTo() const
{
	return m_state == &m_moveToState;
}

bool CCSBot::IsDefusingBomb() const
{
	return m_state == &m_defuseBombState;
}

bool CCSBot::IsEscapingFromBomb() const
{
	return m_state == &m_escapeFromBombState;
}

bool CCSBot::IsBusy() const
{
	return IsAttacking() || IsBuying() || IsDefusingBomb() || m_task == TASK_PLANT_BOMB || m_task == TASK_RESCUE_HOSTAGES || IsSniping();
}
