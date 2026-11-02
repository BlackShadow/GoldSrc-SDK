#include "hostage/hostage_improv.h"
#include "cs_bot_manager.h"

void HostageEscapeState::OnEnter( CHostageImprov *improv )
{
	const CCSBotManager::Zone *zone = TheCSBots()->GetRandomZone();
	if ( zone )
	{
		m_toCoverState.SetRescueGoal( zone->m_center );
		m_behavior.Reset( improv );
		m_behavior.SetState( &m_toCoverState );
	}
	m_canEscape = true;
}

void HostageEscapeState::OnExit( CHostageImprov *improv )
{
	improv->Run();
}

void HostageEscapeState::OnMoveToFailure( const Vector &goal, MoveToFailureType reason )
{
	m_behavior.OnMoveToFailure( goal, reason );
}

void HostageEscapeState::OnUpdate( CHostageImprov *improv )
{
	if ( !m_canEscape || (improv->IsScared() && improv->GetScareIntensity() == CHostageImprov::TERRIFIED) )
	{
		improv->Stop();
		improv->Idle();
		return;
	}
	if ( m_runTimer.IsElapsed() )
		improv->Walk();
	else
		improv->Run();
	CBasePlayer *player = improv->GetClosestVisiblePlayer( 0 );
	if ( !player )
	{
		m_behavior.Update();
		return;
	}
	if ( player->m_iTeam != TERRORIST )
	{
		improv->Stop();
		improv->Idle();
		return;
	}
	Vector delta = player->pev->origin - improv->GetCentroid();
	if ( DotProduct( delta, delta ) <= 562500 )
	{
		improv->Frighten( CHostageImprov::SCARED );
		improv->Stop();
		improv->Idle();
		return;
	}
	improv->Frighten( CHostageImprov::NERVOUS );
	m_runTimer.Start( RANDOM_FLOAT( 3, 6 ) );
	m_behavior.SetState( &m_toCoverState );
}

void HostageEscapeToCoverState::OnEnter( CHostageImprov *improv )
{
	improv->GetPath()->Invalidate();
	m_canEscape = false;
	CNavPath path;
	HostagePathCost cost;
	if ( !path.Compute( &improv->GetFeet(), &m_rescueGoal, cost ) )
		return;
	int index = path.GetSegmentIndexAlongPath( 500 );
	if ( index < 0 )
		return;
	if ( index < path.GetSegmentCount() - 1 )
		++index;
	Vector position = path[index]->pos;
	CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &position );
	const Vector *spot = FindNearbyHidingSpot( improv->GetEntity(), &position, area, 450, false, false );
	if ( !spot )
		spot = &position;
	m_spot = *spot;
	improv->Run();
	improv->MoveTo( m_spot );
	m_canEscape = true;
}

void HostageEscapeToCoverState::OnExit( CHostageImprov * ) {}

void HostageEscapeToCoverState::OnMoveToFailure( const Vector &, MoveToFailureType )
{
	((HostageEscapeState *)GetParent())->LookAround();
}

void HostageEscapeToCoverState::OnUpdate( CHostageImprov *improv )
{
	if ( !m_canEscape )
	{
		improv->Idle();
		return;
	}
	if ( IsSpotOccupied( improv->GetEntity(), &m_spot ) )
	{
		CNavArea *area = improv->GetLastKnownArea();
		const Vector &feet = improv->GetFeet();
		const Vector *spot = FindNearbyHidingSpot( improv->GetEntity(), &feet, area, 300, false, false );
		if ( !spot )
		{
			((HostageEscapeState *)GetParent())->LookAround();
			return;
		}
		m_spot = *spot;
		improv->MoveTo( m_spot );
	}
	if ( improv->IsAtMoveGoal( 20 ) )
		((HostageEscapeState *)GetParent())->LookAround();
}

void HostageEscapeLookAroundState::OnEnter( CHostageImprov *improv )
{
	m_timer.Start( RANDOM_FLOAT( 5, 10 ) );
	improv->Stop();
	improv->FaceOutwards();
}

void HostageEscapeLookAroundState::OnExit( CHostageImprov *improv )
{
	improv->ClearFaceTo();
}

void HostageEscapeLookAroundState::OnUpdate( CHostageImprov *improv )
{
	improv->UpdateIdleActivity( ACT_IDLE_SNEAKY, ACT_IDLE_SNEAKY_FIDGET );
	if ( m_timer.IsElapsed() )
		((HostageEscapeState *)GetParent())->ToCover();
}
