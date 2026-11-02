#include "hostage/hostage_improv.h"
#include "cs_bot_manager.h"
#include <stdlib.h>

void HostageFollowState::OnEnter( CHostageImprov *improv )
{
	improv->Chatter( HOSTAGE_CHATTER_START_FOLLOW, true );
	improv->Agree();
	m_isWaiting = false;
	m_isWaitingForFriend = false;
	improv->MoveTo( improv->GetFeet() );
	m_lastLeaderPos = Vector( 999999.9f, 999999.9f, 999999.9f );
	m_makeWayTimer.Invalidate();
	m_stopRange = RANDOM_FLOAT( 125, 175 );
	if ( improv->IsTerroristNearby() )
		improv->DelayedChatter( 3, HOSTAGE_CHATTER_WARN_NEARBY, false );
}

void HostageFollowState::OnExit( CHostageImprov *improv )
{
	improv->Stop();
}

void HostageFollowState::UpdateStationaryAnimation( CHostageImprov *improv )
{
	if ( improv->IsScared() )
		improv->UpdateIdleActivity( ACT_FOLLOW_IDLE_SCARED, ACT_RESET );
	else
		improv->UpdateIdleActivity( ACT_FOLLOW_IDLE, ACT_FOLLOW_IDLE_FIDGET );
}

void HostageFollowState::OnUpdate( CHostageImprov *improv )
{
	if ( !m_leader )
	{
		improv->Idle();
		return;
	}
	if ( m_leader->pev->deadflag != DEAD_NO )
	{
		improv->Frighten( CHostageImprov::TERRIFIED );
		improv->Idle();
		return;
	}
	float range = (m_leader->pev->origin - improv->GetCentroid()).Length();
	if ( range > 1000 || (improv->GetPath()->IsValid() && improv->GetPath()->GetLength() > 3000) )
	{
		improv->Idle();
		return;
	}
	float stopRange = m_stopRange;
	float leaderSpeed = m_leader->pev->velocity.Length2D();
	if ( leaderSpeed > 140 )
		m_isWaiting = false;
	else if ( m_isWaiting )
	{
		if ( range > stopRange + 20 )
			m_isWaiting = false;
	}
	else if ( range < m_stopRange )
	{
		m_isWaiting = true;
		m_impatientTimer.Start( RANDOM_FLOAT( 5, 20 ) );
	}
	if ( m_isWaiting )
	{
		if ( m_impatientTimer.IsElapsed() && !TheCSBots()->IsRoundOver() )
		{
			m_impatientTimer.Start( RANDOM_FLOAT( 20, 30 ) );
			improv->Chatter( improv->CanSeeRescueZone() ? HOSTAGE_CHATTER_SEE_RESCUE_ZONE :
				HOSTAGE_CHATTER_IMPATIENT_FOR_RESCUE, false );
		}
		Vector delta = m_leader->pev->origin - improv->GetCentroid();
		if ( DotProduct( delta, delta ) < 40000 )
		{
			if ( improv->IsPlayerLookingAtMe( (CBasePlayer *)(CBaseEntity *)m_leader, 0.99f ) )
			{
				if ( !m_makeWayTimer.HasStarted() )
					m_makeWayTimer.Start( RANDOM_FLOAT( 0.4f, 0.75f ) );
				else if ( m_makeWayTimer.IsElapsed() )
				{
					m_impatientTimer.Invalidate();
					Vector to = m_leader->pev->origin - improv->GetCentroid();
					to.NormalizeInPlace();
					Vector side( to.y, -to.x, 0.0f );
					if ( DotProduct( side, gpGlobals->v_forward ) > 0 )
						side = -side;
					Vector position = improv->GetFeet() + side * 15;
					float ground;
					if ( GetGroundHeight( &position, &ground, NULL ) &&
						fabs( ground - improv->GetFeet().z ) < 18 )
					{
						improv->ApplyForce( side * 20 );
						improv->MoveTo( position );
						return;
					}
				}
			}
			else
				m_makeWayTimer.Invalidate();
		}
		improv->Stop();
		CBasePlayer *terrorist = improv->GetClosestVisiblePlayer( TERRORIST );
		improv->LookAt( terrorist ? terrorist->EyePosition() : m_leader->EyePosition() );
		return;
	}
	if ( improv->IsFriendInTheWay() )
	{
		if ( !m_isWaitingForFriend )
		{
			m_isWaitingForFriend = true;
			m_waitForFriendTimer.Start( 15 );
			improv->Stop();
			return;
		}
		if ( !m_waitForFriendTimer.IsElapsed() )
		{
			improv->Stop();
			return;
		}
	}
	if ( !improv->GetPath()->IsValid() && m_repathTimer.IsElapsed() )
	{
		improv->MoveTo( m_leader->pev->origin );
		m_lastLeaderPos = m_leader->pev->origin;
		m_repathTimer.Start( 1 );
	}
	if ( leaderSpeed <= 140 && range <= stopRange + 50 )
		improv->Walk();
	else
		improv->Run();
	Vector delta = m_lastLeaderPos - m_leader->pev->origin;
	if ( DotProduct( delta, delta ) > 1225 )
	{
		improv->MoveTo( m_leader->pev->origin );
		m_lastLeaderPos = m_leader->pev->origin;
	}
}
