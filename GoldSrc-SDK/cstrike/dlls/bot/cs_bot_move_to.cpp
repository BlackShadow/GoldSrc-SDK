#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#pragma warning( pop )
#include "cs_bot_manager.h"
#include "hostage.h"

void MoveToState::OnEnter( CCSBot *bot )
{
	if ( bot->IsUsingKnife() && bot->IsWellPastSafe() && !bot->IsHurrying() )
		bot->Walk();
	else
		bot->Run();
	RouteType route = SAFEST_ROUTE;
	if ( bot->m_task == CCSBot::TASK_FIND_TICKING_BOMB || bot->m_task == CCSBot::TASK_DEFUSE_BOMB ||
		bot->m_task == CCSBot::TASK_CHASE_ENEMY )
		route = FASTEST_ROUTE;
	bot->ComputePath( TheNavAreaGrid.GetNavArea( &m_goal ), &m_goal, route );
	m_announcedGoingToPlant = false;
	m_announcedPlanting = false;
}

void MoveToState::OnExit( CCSBot *bot )
{
	bot->Run();
	bot->SetDisposition( CCSBot::ENGAGE_AND_INVESTIGATE );
}

void MoveToState::OnUpdate( CCSBot *bot )
{
	if ( bot->m_task == CCSBot::TASK_CHASE_ENEMY )
	{
		CBasePlayer *enemy = (CBasePlayer *)(CBaseEntity *)bot->m_taskEntity;
		if ( !enemy || !enemy->IsAlive() )
		{
			bot->PrintIfWatched( "The enemy I was chasing was killed - giving up.\n" );
			bot->Idle();
			return;
		}
	}
	bot->UpdateLookAround( false );
	CCSBotManager *manager = TheCSBots();
	CSGameState *game = bot->GetGameState();
	if ( manager->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB )
	{
		if ( bot->m_task == CCSBot::TASK_FIND_TICKING_BOMB )
		{
			if ( game->GetBombState() != CSGameState::BOMB_PLANTED || game->GetPlantedBombsite() != -1 )
			{
				bot->Idle();
				return;
			}
			for ( int i = 0; i < manager->GetZoneCount(); ++i )
			{
				const Extent &extent = manager->GetZone( i )->m_extent;
				const Vector &origin = bot->pev->origin;
				if ( !game->IsBombsiteClear( i ) && origin.x >= extent.lo.x && origin.x <= extent.hi.x &&
					origin.y >= extent.lo.y && origin.y <= extent.hi.y && origin.z >= extent.lo.z && origin.z <= extent.hi.z )
				{
					game->ClearBombsite( i );
					if ( bot->m_iTeam == CT )
						bot->GetChatter()->BombsiteClear( i );
					bot->Idle();
					return;
				}
			}
		}
		else if ( bot->m_iTeam == CT )
		{
			if ( game->GetBombState() == CSGameState::BOMB_PLANTED )
			{
				if ( bot->m_task != CCSBot::TASK_DEFUSE_BOMB )
				{
					bot->Idle();
					return;
				}
				if ( bot->CanSeePlantedBomb() && manager->GetBombDefuser() )
				{
					BotStatement *statement = new BotStatement( bot->GetChatter(), BOT_STATEMENT_SAY, 3 );
					statement->AppendPhrase( TheBotPhrases->GetPhrase( "CoveringFriend" ) );
					bot->GetChatter()->AddStatement( statement, false );
					bot->Idle();
					return;
				}
			}
		}
		else if ( bot->m_task == CCSBot::TASK_PLANT_BOMB && bot->GetFriendsRemaining() && !m_announcedPlanting )
		{
			if ( (bot->m_signals.GetState() & SIGNAL_BOMB) && bot->GetPathDistanceRemaining() < 50 )
			{
				bot->GetChatter()->PlantingTheBomb( bot->GetPlace() );
				m_announcedPlanting = true;
			}
			if ( !m_announcedGoingToPlant && gpGlobals->time - bot->m_stateTimestamp > 2 )
			{
				bot->GetChatter()->GoingToPlantTheBomb( TheNavAreaGrid.GetPlace( &m_goal ) );
				m_announcedGoingToPlant = true;
			}
		}
	}
	else if ( manager->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES )
	{
		if ( bot->m_task == CCSBot::TASK_COLLECT_HOSTAGES )
		{
			CHostage *hostage = (CHostage *)(CBaseEntity *)bot->m_avoidEntity;
			if ( !hostage || !hostage->IsValid() || hostage->IsFollowingSomeone() )
			{
				bot->Idle();
				return;
			}
			if ( (hostage->pev->origin - m_goal).LengthSquared() > 75 * 75 )
			{
				m_goal = hostage->pev->origin;
				bot->ComputePath( TheNavAreaGrid.GetNavArea( &m_goal ), &m_goal, SAFEST_ROUTE );
			}
			if ( !bot->m_ladder )
			{
				Vector head = hostage->pev->origin + Vector( 0, 0, 54 );
				float rangeSquared = (head - bot->pev->origin).LengthSquared();
				if ( rangeSquared < 100 * 100 )
				{
					bot->SetLookAt( "Hostage", &head, PRIORITY_LOW, 0.5f, false, 5 );
					switch ( RANDOM_LONG( 0, 3 ) )
					{
					case 0: bot->MoveForward(); break;
					case 1: bot->StrafeRight(); break;
					case 2: bot->MoveBackward(); break;
					case 3: bot->StrafeLeft(); break;
					}
					if ( rangeSquared < 50 * 50 )
					{
						bot->UseEntity( bot->m_avoidEntity );
						return;
					}
				}
			}
		}
		else if ( bot->m_task == CCSBot::TASK_RESCUE_HOSTAGES && !bot->m_hostageEscortCount )
		{
			bot->Idle();
			return;
		}
	}
	if ( bot->UpdatePathMovement( true ) == 0 )
		return;
	if ( bot->m_task == CCSBot::TASK_DEFUSE_BOMB )
	{
		CBasePlayerWeapon *weapon = (CBasePlayerWeapon *)bot->m_pActiveItem;
		if ( weapon && (weapon->m_fInReload || weapon->m_fInSpecialReload) )
			return;
		const Vector *position = game->GetBombPosition();
		if ( position )
		{
			Vector feet = bot->pev->origin;
			feet.z -= bot->IsCrouching() ? 18.0f : 36.0f;
			if ( (*position - feet).LengthSquared() < 100 * 100 )
			{
				bot->DefuseBomb();
				return;
			}
		}
	}
	else if ( bot->m_task == CCSBot::TASK_CHASE_ENEMY )
	{
		CBasePlayer *enemy = (CBasePlayer *)(CBaseEntity *)bot->m_taskEntity;
		if ( enemy && enemy->IsAlive() )
		{
			BotStatement *statement = new BotStatement( bot->GetChatter(), BOT_STATEMENT_SAY, 3 );
			statement->AppendPhrase( TheBotPhrases->GetPhrase( "LostEnemy" ) );
			bot->GetChatter()->AddStatement( statement, false );
		}
	}
	else if ( bot->m_task == CCSBot::TASK_PLANT_BOMB && (bot->m_signals.GetState() & SIGNAL_BOMB) && bot->m_bHasC4 )
	{
		bot->PlantBomb();
		return;
	}
	bot->Idle();
}

const char *MoveToState::GetName() const
{
	return "MoveTo";
}
