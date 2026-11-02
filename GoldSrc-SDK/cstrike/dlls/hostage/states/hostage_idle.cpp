#include "hostage/hostage_improv.h"
#include "cs_bot_manager.h"

void HostageIdleState::OnEnter( CHostageImprov * )
{
	m_moveState = MoveDone;
	m_fleeTimer.Invalidate();
	m_mustFlee = false;
}

void HostageIdleState::OnExit( CHostageImprov *improv )
{
	improv->StandUp();
	improv->ClearFaceTo();
}

void HostageIdleState::OnMoveToSuccess( const Vector & )
{
	m_moveState = MoveDone;
}

void HostageIdleState::OnMoveToFailure( const Vector &, MoveToFailureType )
{
	m_moveState = MoveFailed;
}

void HostageIdleState::OnInjury( float )
{
	m_fleeTimer.Invalidate();
	m_mustFlee = true;
}

void HostageIdleState::UpdateStationaryAnimation( CHostageImprov *improv )
{
	if ( improv->IsScared() )
	{
		if ( improv->GetScareIntensity() == CHostageImprov::TERRIFIED )
			improv->Afraid();
		else
			improv->UpdateIdleActivity( ACT_IDLE_SCARED, ACT_RESET );
	}
	else if ( improv->IsAtHome() )
		improv->UpdateIdleActivity( ACT_IDLE, ACT_IDLE_FIDGET );
	else
		improv->UpdateIdleActivity( ACT_IDLE_SNEAKY, ACT_IDLE_SNEAKY_FIDGET );
}

void HostageIdleState::OnUpdate( CHostageImprov *improv )
{
	if ( !UTIL_ActivePlayersInGame() )
		return;
	bool tryFlee = m_mustFlee;
	if ( !tryFlee && improv->IsScared() && !improv->IsTerroristNearby() && m_moveState != Moving )
	{
		m_mustFlee |= improv->GetScareIntensity() == CHostageImprov::TERRIFIED;
		tryFlee = m_mustFlee || (m_fleeTimer.IsElapsed() && improv->GetScareIntensity() > CHostageImprov::NERVOUS);
	}
	if ( tryFlee )
	{
		m_fleeTimer.Start( RANDOM_FLOAT( 10, 20 ) );
		if ( m_mustFlee || (improv->GetTimeSinceLastSawPlayer( TERRORIST ) > 5 &&
			RANDOM_FLOAT( 0, 100 ) < 33.3f) || m_mustFlee )
		{
			m_mustFlee = false;
			CNavArea *area = improv->GetLastKnownArea();
			const Vector &feet = improv->GetFeet();
			const Vector *spot = FindNearbyRetreatSpot( NULL, &feet, area, 500, TERRORIST, false );
			if ( spot )
			{
				improv->MoveTo( *spot );
				improv->Run();
				m_moveState = Moving;
				if ( improv->GetScareIntensity() == CHostageImprov::TERRIFIED )
					improv->Frighten( CHostageImprov::SCARED );
				return;
			}
		}
	}
	if ( m_moveState != NotMoving )
	{
		if ( improv->IsAtMoveGoal( 20 ) )
		{
			m_moveState = NotMoving;
			improv->Stop();
			improv->FaceOutwards();
			if ( improv->IsScared() && !improv->IsAtHome() && RANDOM_FLOAT( 0, 100 ) <= 33.3f )
				improv->Crouch();
			return;
		}
		if ( m_moveState == Moving )
		{
			improv->Run();
			return;
		}
	}
	if ( !improv->IsAtMoveGoal( 75 ) )
	{
		improv->Walk();
		m_moveState = Moving;
		return;
	}
	CBasePlayer *rescuer = improv->GetClosestVisiblePlayer( CT );
	CBasePlayer *terrorist = improv->GetClosestVisiblePlayer( TERRORIST );
	if ( rescuer )
	{
		improv->LookAt( rescuer->EyePosition() );
		improv->Stop();
		if ( terrorist )
		{
			if ( (improv->GetCentroid() - terrorist->pev->origin).Length() < 700 )
			{
				if ( improv->IsAnyPlayerLookingAtMe( TERRORIST, 0.95f ) )
					improv->LookAt( terrorist->EyePosition() );
				else
				{
					TraceResult trace;
					UTIL_TraceLine( rescuer->pev->origin, terrorist->pev->origin, ignore_monsters,
						ignore_glass, terrorist->edict(), &trace );
					if ( trace.flFraction != 1 && m_disagreeTimer.IsElapsed() )
					{
						improv->Disagree();
						m_disagreeTimer.Start( RANDOM_FLOAT( 2, 4 ) );
					}
				}
				return;
			}
		}
		else if ( !TheCSBots()->IsRoundOver() && m_askTimer.IsElapsed() )
		{
			Vector delta = rescuer->pev->origin - improv->GetCentroid();
			if ( DotProduct( delta, delta ) < 40000 && improv->IsPlayerLookingAtMe( rescuer, 0.99f ) )
			{
				improv->Chatter( improv->IsTerroristNearby() ? HOSTAGE_CHATTER_WARN_NEARBY :
					HOSTAGE_CHATTER_PLEASE_RESCUE_ME, false );
				m_askTimer.Start( RANDOM_FLOAT( 3, 10 ) );
			}
		}
		if ( m_waveTimer.IsElapsed() )
		{
			Vector delta = rescuer->pev->origin - improv->GetEntity()->pev->origin;
			if ( DotProduct( delta, delta ) > 62500 )
			{
				improv->Stop();
				improv->Wave();
				improv->LookAt( rescuer->EyePosition() );
				improv->Chatter( HOSTAGE_CHATTER_CALL_TO_RESCUER, false );
				m_moveState = NotMoving;
				m_waveTimer.Start( RANDOM_FLOAT( 10, 20 ) );
			}
		}
		return;
	}
	if ( terrorist )
	{
		improv->LookAt( terrorist->EyePosition() );
		improv->Stop();
		Vector delta = terrorist->pev->origin - improv->GetCentroid();
		if ( DotProduct( delta, delta ) >= 40000 || !improv->IsPlayerLookingAtMe( terrorist, 0.99f ) )
		{
			m_intimidatedTimer.Invalidate();
			return;
		}
		if ( !m_intimidatedTimer.HasStarted() )
			m_intimidatedTimer.Start();
		if ( !improv->IsScared() )
			improv->Frighten( CHostageImprov::NERVOUS );
		if ( m_intimidatedTimer.GetElapsedTime() > 1 && m_pleadTimer.IsElapsed() )
		{
			improv->Chatter( HOSTAGE_CHATTER_INTIMIDATED, true );
			m_pleadTimer.Start( RANDOM_FLOAT( 10, 20 ) );
		}
		if ( !improv->IsAtHome() )
		{
			improv->Chatter( HOSTAGE_CHATTER_RETREAT, true );
			improv->Retreat();
		}
		return;
	}
	improv->ClearLookAt();
	if ( TheCSBots()->GetElapsedRoundTime() > 60.0f - 5.0f * improv->GetAggression() && m_escapeTimer.IsElapsed() )
	{
		m_escapeTimer.Start( 5 );
		float lastSawT = improv->GetTimeSinceLastSawPlayer( TERRORIST );
		float lastSawCT = improv->GetTimeSinceLastSawPlayer( CT );
		float wait = 15.0f - 3.0f * improv->GetAggression();
		if ( lastSawT > wait && lastSawCT > wait )
		{
			if ( improv->IsTerroristNearby() )
			{
				if ( cv_hostage_debug.value > 0 )
					CONSOLE_ECHO( "Hostage: I want to escape, but a T is nearby\n" );
				m_escapeTimer.Start( wait );
			}
			else
			{
				improv->Escape();
				if ( cv_hostage_debug.value > 0 )
					CONSOLE_ECHO( "Hostage: I'm escaping!\n" );
			}
		}
	}
}
