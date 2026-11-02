#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"

class FollowTargetCollector
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	FollowTargetCollector( CBaseEntity* leader )
	{
		m_leader = leader;
		m_direction = leader->pev->velocity.Make2D();
		float speed = m_direction.Length();
		if ( speed <= 0.0f )
			m_direction = Vector2D( 1.0f, 0.0f );
		else
			m_direction = m_direction / speed;
		if ( speed < 100.0f )
		{
			m_direction = Vector2D( 0.0f, 0.0f );
			m_target = leader->pev->origin.Make2D();
		}
		else
		{
			float distance = speed > 200.0f ? 300.0f : speed * 1.5f;
			m_target = leader->pev->origin.Make2D() + m_direction * distance;
		}
		m_count = 0;
	}

	bool operator()( CNavArea* area )
	{
		if ( m_count > 127 )
			return false;
		if ( !area->GetParent() || area->IsConnected( area->GetParent(), NUM_DIRECTIONS ) )
		{
			bool collect = m_direction.x > -0.01f && m_direction.x < 0.01f
				&& m_direction.y > -0.01f && m_direction.y < 0.01f;
			if ( !collect )
			{
				Vector2D direction = area->GetCenter()->Make2D() - m_target;
				float length = direction.Length();
				if ( length <= 0.0f )
					direction = Vector2D( 1.0f, 0.0f );
				else
					direction = direction / length;
				collect = DotProduct( direction, m_direction ) > 0.7071f;
			}
			if ( collect )
				m_area[m_count++] = area;
		}
		return m_count <= 127;
	}

	CBaseEntity*	m_leader;
	Vector2D		m_direction;
	Vector2D		m_target;
	CNavArea*		m_area[128];
	int				m_count;
};

void FollowState::OnEnter( CCSBot* bot )
{
	bot->StandUp();
	bot->Run();
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
	m_initialFollowFlag = 0;
	m_initialFollowValue = 0;
	m_leaderPosition = Vector( -100000000.0f, -100000000.0f, -100000000.0f );
	m_lastSawLeader = 0.0f;
	m_repathTimer.Invalidate();
	m_sneaking = false;
	m_walkHysteresisTimer.Invalidate();
	m_hysteresisStarted = false;
	m_leaderMotion = 0;
	m_boredTimer.Start( RANDOM_FLOAT( 2.0f, 5.0f ) );
}

void FollowState::OnExit( CCSBot* )
{
}

void FollowState::OnUpdate( CCSBot* bot )
{
	if ( !m_leader || !m_leader->IsAlive() )
	{
		bot->Idle();
		return;
	}
	if ( bot->m_bHasC4 && ( bot->m_signals.GetState() & SIGNAL_BOMB ) )
	{
		bot->m_task = CCSBot::TASK_PLANT_BOMB;
		bot->m_taskEntity = NULL;
		bot->PlantBomb();
		bot->GetChatter()->PlantingTheBomb( bot->GetPlace() );
		return;
	}
	bot->UpdateLookAround( false );
	if ( bot->pev->velocity.LengthSquared() >= 100.0f )
		m_boredTimer.Start( RANDOM_FLOAT( 2.0f, 5.0f ) );
	float speed = m_leader->pev->velocity.Length2D();
	ComputeLeaderMotionState( speed );
	bool visible = bot->IsVisible( &m_leader->pev->origin, false );
	if ( visible )
		m_lastSawLeader = gpGlobals->time;
	if ( ( m_leader->pev->origin - bot->pev->origin ).LengthSquared() > 562500.0f )
		m_sneaking = false;
	else if ( visible )
	{
		if ( m_leaderMotion == 2 )
			m_sneaking = true;
		else if ( m_sneaking && m_leaderMotion == 3 )
			m_sneaking = false;
	}
	if ( gpGlobals->time - m_lastSawLeader > 20.0f )
		m_sneaking = false;
	if ( m_sneaking )
		bot->Walk();
	else
		bot->Run();
	if ( !bot->m_pathLength && m_leaderMotion == 1
		&& m_motionChangedTimer.GetElapsedTime() > m_nextStopHideDelay )
	{
		m_nextStopHideDelay += RANDOM_FLOAT( 1.0f, 3.0f );
		if ( ( m_leader->pev->origin - bot->pev->origin ).LengthSquared() < 62500.0f
			&& bot->TryToHide( NULL, -1.0f, 250.0f, false, true ) )
		{
			bot->ResetStuckMonitor();
			return;
		}
	}
	bool bored = m_boredTimer.IsElapsed();
	if ( bored )
		m_sneaking = true;
	if ( speed <= 100.0f || m_leaderMotion == 1 )
	{
		if ( bot->UpdatePathMovement( false ) != 0 )
		{
			bot->m_pathLength = 0;
			bot->m_ladder = NULL;
		}
		if ( !bored )
			return;
	}
	else if ( bot->UpdatePathMovement( false ) != 0 )
	{
		bored = true;
		bot->m_pathLength = 0;
		bot->m_ladder = NULL;
	}
	if ( m_repathTimer.IsElapsed() )
	{
		m_leaderPosition = m_leader->pev->origin;
		bot->ResetStuckMonitor();
		float range = speed > 200.0f ? 600.0f : 400.0f;
		FollowTargetCollector collector( m_leader );
		CNavArea* nearest = TheNavAreaGrid.GetNearestNavArea( &m_leaderPosition, false );
		SearchSurroundingAreas( nearest, &m_leaderPosition, collector, range );
		if ( cv_bot_debug.value > 0.0f )
			for ( int i = 0; i < collector.m_count; ++i )
				collector.m_area[i]->Draw( 255, 0, 0, 2 );
		if ( collector.m_count )
		{
			CNavArea* area;
			if ( m_boredTimer.IsElapsed() )
			{
				area = collector.m_area[RANDOM_LONG( 0, collector.m_count - 1 )];
				bot->PrintIfWatched( "%4.1f: Bored. Repathing to a new nearby area\n", gpGlobals->time );
			}
			else
			{
				bot->PrintIfWatched( "%4.1f: Repathing to stay with leader.\n", gpGlobals->time );
				area = NULL;
				float shortest = 10000000000.0f;
				for ( int i = 0; i < collector.m_count; ++i )
				{
					Vector close;
					collector.m_area[i]->GetClosestPointOnArea( &bot->pev->origin, &close );
					float distance = ( bot->pev->origin - close ).LengthSquared();
					if ( distance < shortest )
					{
						shortest = distance;
						area = collector.m_area[i];
					}
				}
			}
			if ( !bot->ComputePath( area, NULL, FASTEST_ROUTE ) )
				bot->PrintIfWatched( "Pathfind to leader failed.\n" );
			m_repathTimer.Start( 0.5f );
			m_boredTimer.Reset();
		}
	}
}

void FollowState::ComputeLeaderMotionState( float speed )
{
	int previous = m_leaderMotion;
	if ( speed > 140.0f )
	{
		m_leaderMotion = 3;
		m_hysteresisStarted = false;
	}
	else if ( !( speed > 10.0f ) )
	{
		m_leaderMotion = 1;
		m_hysteresisStarted = false;
	}
	else
	{
		if ( !m_hysteresisStarted )
		{
			m_hysteresisStarted = true;
			m_walkHysteresisTimer.Start();
		}
		if ( !m_walkHysteresisTimer.HasStarted() || m_walkHysteresisTimer.GetElapsedTime() > 0.25f )
			m_leaderMotion = 2;
	}

	if ( previous != m_leaderMotion )
	{
		m_motionChangedTimer.Start();
		m_nextStopHideDelay = RANDOM_FLOAT( 1.0f, 3.0f );
	}
}

const char* FollowState::GetName() const
{
	return "Follow";
}
