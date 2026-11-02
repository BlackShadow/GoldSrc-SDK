#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"
#include "hostage.h"

void IdleState::OnEnter( CCSBot *bot )
{
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
	bot->SetEnemy( NULL );
	if ( bot->IsUsingKnife() && bot->IsWellPastSafe() && !bot->IsHurrying() )
		bot->Walk();
	bot->SetTask( CCSBot::TASK_SEEK_AND_DESTROY );
	bot->SetTaskEntity( NULL );
	bot->SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
}

void IdleState::OnUpdate( CCSBot *bot )
{
	if ( !bot->m_lastKnownArea && !bot->StayOnNavMesh() )
		return;
	if ( cv_bot_stop.value > 0 )
	{
		bot->ResetStuckMonitor();
		return;
	}
	CCSBotManager *manager = TheCSBots();
	CSGameState *game = bot->GetGameState();
	if ( bot->IsSafe() && !bot->EquipGrenade( false ) && bot->GetProfile()->GetSkill() > 0.33f )
		bot->EquipKnife();
	if ( game->IsRoundOver() )
	{
		if ( bot->m_hostageEscortCount )
		{
			const CCSBotManager::Zone *closest = NULL;
			PathCost cost( bot, FASTEST_ROUTE );
			double closestDistance = 100000000;
			if ( bot->m_lastKnownArea )
			{
				for ( int i = 0; i < manager->GetZoneCount(); ++i )
				{
					const CCSBotManager::Zone *zone = manager->GetZone( i );
					if ( zone->m_areaCount )
					{
						float distance = NavAreaTravelDistance( bot->m_lastKnownArea, zone->m_area[0], cost );
						if ( distance < closestDistance )
						{
							closestDistance = distance;
							closest = zone;
						}
					}
				}
			}
			bot->SetTask( CCSBot::TASK_RESCUE_HOSTAGES );
			bot->SetTaskEntity( NULL );
			bot->Run();
			bot->SetDisposition( CCSBot::SELF_DEFENSE );
			bot->MoveTo( manager->GetRandomPositionInZone( closest ), FASTEST_ROUTE );
			bot->PrintIfWatched( "Trying to rescue hostages at the end of the round\n" );
			return;
		}
		bot->Hunt();
		return;
	}
	if ( bot->m_isFollowing )
	{
		bot->ContinueFollowing();
		return;
	}
	if ( manager->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES )
	{
		if ( bot->m_iTeam == TERRORIST )
		{
			bool guardHostages = true;
			if ( !bot->IsSafe() )
			{
				if ( game->m_isHostageTaken || game->AreAllHostagesBeingRescued() )
					guardHostages = false;
				else
				{
					float chance = (manager->GetElapsedRoundTime() - bot->m_safeTime) * 100 / 120;
					guardHostages = RANDOM_FLOAT( 0, 100 ) > chance;
				}
			}
			if ( bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) <= 75 )
			{
				const Vector *position = game->GetRandomFreeHostagePosition();
				if ( position && guardHostages )
				{
					bot->SetTask( CCSBot::TASK_MOVE_TO_SNIPER_SPOT );
					bot->SetTaskEntity( NULL );
					bot->PrintIfWatched( "Sniping near hostages\n" );
					CNavArea *area = TheNavAreaGrid.GetNearestNavArea( position );
					bot->Hide( area, -1, 2000, false );
					bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
					return;
				}
				if ( bot->GuardRandomZone( 2000 ) )
				{
					bot->SetTask( CCSBot::TASK_MOVE_TO_SNIPER_SPOT );
					bot->SetTaskEntity( NULL );
					bot->PrintIfWatched( "Sniping near a rescue zone\n" );
					bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
					return;
				}
			}
			if ( !bot->IsSafe() && !bot->IsRogue() )
			{
				CHostage *hostage = game->GetNearestVisibleFreeHostage();
				if ( hostage )
				{
					CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &hostage->pev->origin );
					if ( area )
					{
						bot->SetTask( CCSBot::TASK_GUARD_HOSTAGES );
						bot->SetTaskEntity( NULL );
						bot->Hide( area, -1, 750, false );
						bot->PrintIfWatched( "I'm guarding hostages I found\n" );
						return;
					}
				}
			}
			int morale = bot->m_morale;
			if ( !bot->GetFriendsRemaining() || (!bot->IsRogue() && !manager->IsDefenseRushing() &&
				morale * 25 + 70 <= RANDOM_FLOAT( 0, 100 )) )
			{
				const Vector *position = game->GetRandomFreeHostagePosition();
				if ( position && guardHostages )
				{
					CNavArea *area = TheNavAreaGrid.GetNearestNavArea( position );
					if ( area )
					{
						bot->SetTask( CCSBot::TASK_GUARD_HOSTAGES );
						bot->SetTaskEntity( NULL );
						bot->PrintIfWatched( "I'm guarding hostages\n" );
						bot->Hide( area, -1, float( (bot->m_morale + 3) * 250 + 750 ), false );
						bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
						if ( RANDOM_FLOAT( 0, 100 ) < 50 )
							bot->GetChatter()->GuardingHostages( area->GetPlace(), true );
						return;
					}
				}
				if ( bot->GuardRandomZone( 500 ) )
				{
					bot->SetTask( CCSBot::TASK_GUARD_HOSTAGE_RESCUE_ZONE );
					bot->SetTaskEntity( NULL );
					bot->PrintIfWatched( "I'm guarding a rescue zone\n" );
					bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
					bot->GetChatter()->GuardingHostageEscapeZone( true );
					return;
				}
			}
			bot->Hunt();
			return;
		}
		if ( !bot->m_hostageEscortCount )
		{
			if ( bot->GetFriendsRemaining() && bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) < 10 )
				goto snipe;
			if ( bot->GetFriendsRemaining() && !bot->m_hostageEscortCount &&
				(bot->IsRogue() || RANDOM_FLOAT( 0, 100 ) < 33.3f) )
			{
				bot->Hunt();
				return;
			}
		}
		CHostage *hostage = game->GetNearestFreeHostage( NULL );
		if ( !bot->IsDoingScenario() )
		{
			if ( hostage )
			{
				CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &hostage->pev->origin );
				if ( area )
				{
					bot->SetTask( CCSBot::TASK_GUARD_HOSTAGES );
					bot->SetTaskEntity( NULL );
					bot->Hide( area, -1, 750, false );
					bot->PrintIfWatched( "I'm securing the hostages for a human to rescue\n" );
					return;
				}
			}
			bot->Hunt();
			return;
		}
		bot->m_avoidEntity = NULL;
		const CCSBotManager::Zone *closest = NULL;
		bool rescue = false;
		if ( bot->m_hostageEscortCount && bot->m_lastKnownArea && manager->GetZoneCount() > 0 )
		{
			PathCost cost( bot, FASTEST_ROUTE );
			double closestDistance = 100000000;
			for ( int i = 0; i < manager->GetZoneCount(); ++i )
			{
				const CCSBotManager::Zone *zone = manager->GetZone( i );
				if ( zone->m_areaCount )
				{
					float distance = NavAreaTravelDistance( bot->m_lastKnownArea, zone->m_area[0], cost );
					if ( distance < closestDistance )
					{
						closestDistance = distance;
						closest = zone;
					}
				}
			}
			if ( closest )
			{
				if ( !hostage )
					rescue = true;
				else
				{
					float zoneDistance = NavAreaTravelDistance( bot->m_lastKnownArea, closest->m_area[0], cost );
					CNavArea *hostageArea = TheNavAreaGrid.GetNearestNavArea( &hostage->pev->origin );
					float hostageDistance = NavAreaTravelDistance( bot->m_lastKnownArea, hostageArea, cost );
					rescue = hostageDistance < 0 || hostageDistance > zoneDistance;
				}
			}
		}
		if ( rescue )
		{
			bot->SetTask( CCSBot::TASK_RESCUE_HOSTAGES );
			bot->SetTaskEntity( NULL );
			bot->Run();
			bot->SetDisposition( CCSBot::SELF_DEFENSE );
			bot->MoveTo( manager->GetRandomPositionInZone( closest ), FASTEST_ROUTE );
			bot->PrintIfWatched( "I'm rescuing hostages\n" );
			bot->GetChatter()->EscortingHostages();
			return;
		}
		if ( !hostage )
		{
			bot->Hunt();
			return;
		}
		bot->SetTask( CCSBot::TASK_COLLECT_HOSTAGES );
		bot->SetTaskEntity( NULL );
		bot->Run();
		bot->m_avoidEntity = hostage;
		bool escorting = bot->m_hostageEscortCount != 0;
		bot->m_isWaitingForHostage = false;
		bot->m_hostageWaitTimer.Invalidate();
		bot->MoveTo( &hostage->pev->origin, escorting ? FASTEST_ROUTE : SAFEST_ROUTE );
		bot->PrintIfWatched( "I'm collecting hostages\n" );
		return;
	}
	if ( manager->GetScenario() == CCSBotManager::SCENARIO_ESCORT_VIP )
	{
		if ( bot->m_iTeam == TERRORIST )
		{
			if ( bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) <= 75 )
			{
				const CCSBotManager::Zone *zone = manager->GetRandomZone();
				if ( zone )
				{
					CNavArea *area = manager->GetRandomAreaInZone( zone );
					if ( area )
					{
						bot->SetTask( CCSBot::TASK_MOVE_TO_SNIPER_SPOT );
						bot->SetTaskEntity( NULL );
						bot->Hide( area, -1, 2000, false );
						bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
						bot->PrintIfWatched( "Sniping near escape zone\n" );
						return;
					}
				}
			}
			if ( !bot->IsRogue() && !manager->IsDefenseRushing() &&
				bot->m_morale * -34.0f > RANDOM_FLOAT( 0, 100 ) )
			{
				const CCSBotManager::Zone *zone = manager->GetRandomZone();
				if ( zone )
				{
					CNavArea *area = manager->GetRandomAreaInZone( zone );
					if ( area )
					{
						bot->SetTask( CCSBot::TASK_GUARD_VIP_ESCAPE_ZONE );
						bot->SetTaskEntity( NULL );
						bot->PrintIfWatched( "I'm guarding an escape zone\n" );
						bot->Hide( area, -1, float( (bot->m_morale + 3) * 250 + 750 ), false );
						bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
						return;
					}
				}
			}
			bot->Hunt();
			return;
		}
		if ( !bot->m_bIsVIP )
		{
			if ( bot->GetFriendsRemaining() && bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) < 10 )
				goto snipe;
			bot->Hunt();
			return;
		}
		const CCSBotManager::Zone *zone = NULL;
		if ( manager->GetElapsedRoundTime() >= 20 )
		{
			PathCost cost( bot, SAFEST_ROUTE );
			double closestDistance = 100000000;
			if ( bot->m_lastKnownArea )
			{
				for ( int i = 0; i < manager->GetZoneCount(); ++i )
				{
					const CCSBotManager::Zone *candidate = manager->GetZone( i );
					if ( candidate->m_areaCount )
					{
						float distance = NavAreaTravelDistance( bot->m_lastKnownArea, candidate->m_area[0], cost );
						if ( distance < closestDistance )
						{
							closestDistance = distance;
							zone = candidate;
						}
					}
				}
			}
		}
		else
			zone = manager->GetRandomZone();
		if ( zone )
		{
			const Vector *position = manager->GetRandomPositionInZone( zone );
			if ( position )
			{
				bot->SetTask( CCSBot::TASK_VIP_ESCAPE );
				bot->SetTaskEntity( NULL );
				bot->Run();
				bot->MoveTo( position, SAFEST_ROUTE );
				if ( bot->GetFriendsRemaining() && manager->GetRadioMessageInterval( EVENT_RADIO_FOLLOW_ME, bot->m_iTeam ) > 30 )
					bot->SendRadioMessage( EVENT_RADIO_FOLLOW_ME );
				return;
			}
		}
		bot->Hunt();
		return;
	}
	if ( manager->GetScenario() != CCSBotManager::SCENARIO_DEFUSE_BOMB )
	{
		if ( bot->GetFriendsRemaining() && bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) < 10 )
			goto snipe;
		bot->Hunt();
		return;
	}
	if ( bot->m_iTeam == TERRORIST )
	{
		if ( game->GetBombState() == CSGameState::BOMB_PLANTED )
		{
			if ( game->GetPlantedBombsite() != -1 )
			{
				const CCSBotManager::Zone *zone = manager->GetZone( game->GetPlantedBombsite() );
				bot->SetTask( CCSBot::TASK_GUARD_TICKING_BOMB );
				bot->SetTaskEntity( NULL );
				Place place = TheNavAreaGrid.GetPlace( &zone->m_center );
				if ( place )
				{
					const Vector *spot = FindRandomHidingSpot( bot, place, bot->IsSniper() );
					if ( spot )
					{
						bot->Hide( spot, -1, false );
						return;
					}
				}
				bot->Hide( TheNavAreaGrid.GetNearestNavArea( &zone->m_center ), -1, 750, false );
				return;
			}
			bot->GetChatter()->RequestBombLocation();
			const CCSBotManager::Zone *zone = manager->GetZone( game->GetNextBombsiteToSearch() );
			const Vector *position = manager->GetRandomPositionInZone( zone );
			if ( position )
			{
				bot->SetTask( CCSBot::TASK_FIND_TICKING_BOMB );
				bot->SetTaskEntity( NULL );
				bot->MoveTo( position, SAFEST_ROUTE );
				return;
			}
			bot->Hunt();
			return;
		}
		if ( bot->m_bHasC4 )
		{
			if ( bot->m_signals.GetState() & SIGNAL_BOMB )
			{
				bot->SetTask( CCSBot::TASK_PLANT_BOMB );
				bot->SetTaskEntity( NULL );
				bot->PlantBomb();
				bot->GetChatter()->PlantingTheBomb( bot->GetPlace() );
				return;
			}
			const CCSBotManager::Zone *closest = NULL;
			if ( gpGlobals->time >= manager->m_earliestBombPlantTimestamp && bot->m_lastKnownArea )
			{
				PathCost cost( bot, SAFEST_ROUTE );
				double closestDistance = 100000000;
				for ( int i = 0; i < manager->GetZoneCount(); ++i )
				{
					const CCSBotManager::Zone *zone = manager->GetZone( i );
					if ( zone->m_areaCount )
					{
						float distance = NavAreaTravelDistance( bot->m_lastKnownArea, zone->m_area[0], cost );
						if ( distance < closestDistance )
						{
							closestDistance = distance;
							closest = zone;
						}
					}
				}
			}
			if ( closest )
			{
				const Vector *position = manager->GetRandomPositionInZone( closest );
				if ( position )
				{
					bot->SetTask( CCSBot::TASK_PLANT_BOMB );
					bot->SetTaskEntity( NULL );
					bot->Run();
					bot->MoveTo( position, SAFEST_ROUTE );
					return;
				}
			}
			bot->Hunt();
			return;
		}
		if ( bot->GetFriendsRemaining() && bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) < 10 )
			goto snipe;
		if ( bot->NoticeLooseBomb() )
		{
			bot->FetchBomb();
			return;
		}
		if ( !bot->IsRogue() && game->GetBombState() == CSGameState::BOMB_PLANTED && game->GetBombPosition() )
		{
			bot->SetTask( CCSBot::TASK_GUARD_TICKING_BOMB );
			bot->SetTaskEntity( NULL );
			bot->Hide( TheNavAreaGrid.GetNavArea( game->GetBombPosition() ), -1, 750, false );
			return;
		}
		bot->Hunt();
		return;
	}
	if ( game->GetBombState() == CSGameState::BOMB_PLANTED )
	{
		const Vector *position = game->GetBombPosition();
		if ( position )
		{
			if ( manager->GetBombDefuser() )
			{
				if ( !bot->IsRogue() )
				{
					bot->SetTask( CCSBot::TASK_GUARD_BOMB_DEFUSER );
					bot->SetTaskEntity( NULL );
					bot->Hide( TheNavAreaGrid.GetNavArea( position ), -1, 750, false );
					return;
				}
			}
			else
			{
				if ( bot->IsDoingScenario() )
				{
					bot->SetTask( CCSBot::TASK_DEFUSE_BOMB );
					bot->SetTaskEntity( NULL );
					bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
					bot->MoveTo( position, SAFEST_ROUTE );
				}
				else
				{
					bot->SetTask( CCSBot::TASK_FETCH_BOMB );
					bot->SetTaskEntity( NULL );
					bot->Hide( TheNavAreaGrid.GetNavArea( position ), -1, 750, false );
					bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
				}
				return;
			}
		}
		else if ( game->GetPlantedBombsite() != -1 )
		{
			const CCSBotManager::Zone *zone = manager->GetZone( game->GetPlantedBombsite() );
			if ( !bot->IsDoingScenario() )
			{
				bot->SetTask( CCSBot::TASK_FETCH_BOMB );
				bot->SetTaskEntity( NULL );
				bot->Hide( TheNavAreaGrid.GetNavArea( &zone->m_center ), -1, 750, false );
			}
			else
			{
				bot->SetTask( CCSBot::TASK_DEFUSE_BOMB );
				bot->SetTaskEntity( NULL );
				bot->MoveTo( &zone->m_center, SAFEST_ROUTE );
			}
			bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
			return;
		}
		else
		{
			const CCSBotManager::Zone *closest = NULL;
			float closestDistance = 10000000;
			ShortestPathCost cost;
			for ( int i = 0; i < manager->GetZoneCount(); ++i )
			{
				const CCSBotManager::Zone *zone = manager->GetZone( i );
				if ( zone->m_areaCount && !game->IsBombsiteClear( i ) )
				{
					CNavArea *area = TheNavAreaGrid.GetNearestNavArea( &zone->m_center );
					float distance = NavAreaTravelDistance( bot->m_lastKnownArea, area, cost );
					if ( distance < closestDistance )
					{
						closestDistance = distance;
						closest = zone;
					}
				}
			}
			if ( !closest || closestDistance > 2000 )
				closest = manager->GetZone( game->GetNextBombsiteToSearch() );
			const Vector *target = manager->GetRandomPositionInZone( closest );
			if ( target )
			{
				bot->SetTask( CCSBot::TASK_FIND_TICKING_BOMB );
				bot->SetTaskEntity( NULL );
				bot->MoveTo( target, SAFEST_ROUTE );
				return;
			}
		}
	}
	if ( bot->IsSniper() && RANDOM_FLOAT( 0, 100 ) <= 75 )
	{
		CNavArea *area = NULL;
		if ( game->IsLooseBombLocationKnown() )
		{
			area = TheNavAreaGrid.GetNearestNavArea( game->GetBombPosition() );
			bot->PrintIfWatched( "Sniping near loose bomb\n" );
		}
		else
		{
			const CCSBotManager::Zone *zone = manager->GetRandomZone();
			if ( zone )
			{
				area = manager->GetRandomAreaInZone( zone );
				bot->PrintIfWatched( "Sniping near bombsite\n" );
			}
		}
		if ( area )
		{
			bot->SetTask( CCSBot::TASK_MOVE_TO_SNIPER_SPOT );
			bot->SetTaskEntity( NULL );
			bot->Hide( area, -1, 2000, false );
			bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
			return;
		}
	}
	if ( !bot->IsRogue() && !manager->IsDefenseRushing() && !game->IsLooseBombLocationKnown() &&
		(bot->IsSafe() || bot->HasNotSeenEnemyForLongTime()) && bot->m_morale * -34.0f > RANDOM_FLOAT( 0, 100 ) )
	{
		int morale = bot->m_morale;
		const CCSBotManager::Zone *zone = manager->GetRandomZone();
		if ( zone )
		{
			CNavArea *area = manager->GetRandomAreaInZone( zone );
			if ( area )
			{
				bot->PrintIfWatched( "I'm guarding a bombsite\n" );
				bot->GetChatter()->AnnouncePlan( "GoingToDefendBombsite", area->GetPlace() );
				bot->SetTask( CCSBot::TASK_FETCH_BOMB );
				bot->SetTaskEntity( NULL );
				bot->Hide( area, -1, float( (morale + 3) * 100 + 500 ), false );
				bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
				return;
			}
		}
	}
	bot->Hunt();
	return;

snipe:
	bot->SetTask( CCSBot::TASK_MOVE_TO_SNIPER_SPOT );
	bot->SetTaskEntity( NULL );
	bot->Hide( bot->m_lastKnownArea, RANDOM_FLOAT( 10, 30 ), 2000, false );
	bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
	bot->PrintIfWatched( "Sniping!\n" );
}

const char *IdleState::GetName() const
{
	return "Idle";
}
