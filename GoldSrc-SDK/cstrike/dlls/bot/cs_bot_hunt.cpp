#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"
#include "hostage.h"

void HuntState::OnEnter( CCSBot *bot )
{
	if ( bot->IsUsingKnife() && bot->IsWellPastSafe() && !bot->IsHurrying() )
		bot->Walk();
	else
		bot->Run();
	bot->StandUp();
	bot->SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
	bot->SetTask( CCSBot::TASK_SEEK_AND_DESTROY );
	bot->SetTaskEntity( NULL );
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
}

void HuntState::OnExit( CCSBot * )
{
}

void HuntState::OnUpdate( CCSBot *bot )
{
	if ( gpGlobals->time - bot->m_stateTimestamp > 30 )
	{
		bot->PrintIfWatched( "Giving up hunting, and being a rogue\n" );
		bot->m_isRogue = false;
		bot->Idle();
		return;
	}
	CCSBotManager *manager = TheCSBots();
	if ( manager->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB )
	{
		if ( bot->m_iTeam == TERRORIST )
		{
			if ( bot->m_bHasC4 && (gpGlobals->time >= manager->m_earliestBombPlantTimestamp ||
				((bot->m_signals.GetState() & SIGNAL_BOMB) && gpGlobals->time - bot->m_lastSawEnemyTime > 3)) )
			{
				bot->Idle();
				return;
			}
			if ( bot->NoticeLooseBomb() )
			{
				bot->FetchBomb();
				return;
			}
			if ( !bot->IsRogue() && bot->GetGameState()->GetBombState() == CSGameState::BOMB_PLANTED &&
				bot->GetGameState()->GetBombPosition() )
			{
				bot->SetTask( CCSBot::TASK_GUARD_TICKING_BOMB );
				bot->SetTaskEntity( NULL );
				const Vector *position = bot->GetGameState()->GetBombPosition();
				CNavArea *area = TheNavAreaGrid.GetNavArea( position );
				bot->Hide( area, -1, 750, false );
				return;
			}
		}
		else
		{
			if ( !bot->IsRogue() && bot->CanSeeLooseBomb() )
			{
				bot->SetTask( CCSBot::TASK_GUARD_LOOSE_BOMB );
				bot->SetTaskEntity( NULL );
				CNavArea *area = manager->GetLooseBombArea();
				bot->Hide( area, -1, 750, false );
				bot->GetChatter()->AnnouncePlan( "GoingToGuardLooseBomb", area->GetPlace() );
				return;
			}
			if ( manager->IsBombPlanted() && (!bot->IsRogue() || !manager->GetBombDefuser()) )
			{
				bot->Idle();
				return;
			}
		}
	}
	else if ( manager->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES && bot->m_iTeam == TERRORIST )
	{
		if ( bot->GetGameState()->AreAllHostagesBeingRescued() && bot->GuardRandomZone( 500 ) )
		{
			bot->SetTask( CCSBot::TASK_GUARD_HOSTAGE_RESCUE_ZONE );
			bot->SetTaskEntity( NULL );
			bot->PrintIfWatched( "Trying to beat them to an escape zone!\n" );
			bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
			bot->GetChatter()->GuardingHostageEscapeZone( true );
			return;
		}
		if ( !bot->IsRogue() && !bot->IsSafe() )
		{
			CHostage *hostage = bot->GetGameState()->GetNearestVisibleFreeHostage();
			if ( hostage )
			{
				CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &hostage->pev->origin );
				if ( area )
				{
					bot->SetTask( CCSBot::TASK_GUARD_HOSTAGES );
					bot->SetTaskEntity( NULL );
					bot->Hide( area, -1, 750, false );
					bot->PrintIfWatched( "I'm guarding hostages\n" );
					bot->GetChatter()->GuardingHostages( area->GetPlace(), true );
					return;
				}
			}
		}
	}
	if ( bot->ShouldInvestigateNoise( NULL ) )
	{
		bot->InvestigateNoise();
		return;
	}
	bot->UpdateLookAround( false );
	if ( m_huntArea == bot->m_lastKnownArea || bot->UpdatePathMovement( true ) != 0 )
	{
		m_huntArea = NULL;
		double oldest = 0;
		int lastIndex = -1;
		for ( NavAreaList::iterator it = TheNavAreaList.begin(); it != TheNavAreaList.end(); ++it )
		{
			CNavArea *area = *it;
			++lastIndex;
			if ( area->GetSizeX() >= 150 && area->GetSizeY() >= 150 )
			{
				double age = gpGlobals->time - area->GetClearedTimestamp( bot->m_iTeam - 1 );
				if ( age > oldest )
				{
					oldest = age;
					m_huntArea = area;
				}
			}
		}
		int index = RANDOM_LONG( 0, lastIndex );
		for ( NavAreaList::iterator it = TheNavAreaList.begin(); it != TheNavAreaList.end(); ++it )
		{
			m_huntArea = *it;
			if ( index-- == 0 )
				break;
		}
		bot->ComputePath( m_huntArea, NULL, SAFEST_ROUTE );
	}
}

const char *HuntState::GetName() const
{
	return "Hunt";
}
