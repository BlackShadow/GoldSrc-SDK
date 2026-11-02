#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"
#include "hostage.h"

void HideState::OnEnter( CCSBot* bot )
{
	m_atSpot = false;
	if ( m_duration < 0.0f )
		m_duration = RANDOM_FLOAT( 30.0f, 60.0f );
	if ( RANDOM_FLOAT( 0.0f, 100.0f ) < 50.0f )
		m_holdPosition = true;
	m_holdTime = m_holdPosition ? RANDOM_FLOAT( 3.0f, 10.0f ) : 0.0f;
	m_heardEnemy = false;
	m_holdTimestamp = 0.0f;
	m_spotRetries = 0;
	if ( bot->m_isFollowing )
		m_leaderPosition = bot->m_leader->pev->origin;
}

void HideState::OnExit( CCSBot* bot )
{
	m_holdPosition = false;
	bot->StandUp();
	bot->ResetStuckMonitor();
	bot->m_lookAtState = 0;
	bot->m_lookAtDescription = NULL;
	bot->m_approachPointCount = 0;
	if ( bot->HasShield() && bot->HasShield() && bot->m_bShieldDrawn )
		bot->SecondaryAttack();
}

void HideState::OnUpdate( CCSBot* bot )
{
	CCSBotManager* manager = TheCSBots();
	CBasePlayerWeapon* weapon = (CBasePlayerWeapon*)bot->m_pActiveItem;
	bool reloading = weapon && ( weapon->m_fInReload || weapon->m_fInSpecialReload );
	if ( !reloading )
	{
		if ( gpGlobals->time - bot->m_stateTimestamp > m_duration )
		{
			if ( bot->m_task == CCSBot::TASK_GUARD_LOOSE_BOMB )
			{
				bot->Hide( manager->GetLooseBombArea(), -1.0f, 750.0f, false );
				return;
			}
			if ( bot->m_task == CCSBot::TASK_FETCH_BOMB )
			{
				const CCSBotManager::Zone* zone = manager->GetClosestZone( &bot->pev->origin );
				if ( zone )
				{
					CNavArea* area = manager->GetRandomAreaInZone( zone );
					if ( area )
					{
						bot->Hide( area, -1.0f, 750.0f, false );
						return;
					}
				}
			}
			else if ( bot->m_task == CCSBot::TASK_GUARD_HOSTAGE_RESCUE_ZONE && bot->GuardRandomZone( 500.0f ) )
			{
				bot->m_task = CCSBot::TASK_GUARD_HOSTAGE_RESCUE_ZONE;
				bot->m_taskEntity = NULL;
				bot->PrintIfWatched( "Continuing to guard hostage rescue zones\n" );
				bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
				bot->GetChatter()->GuardingHostageEscapeZone( true );
				return;
			}
			bot->Idle();
			return;
		}
		if ( bot->m_isFollowing )
		{
			CBasePlayer* leader = (CBasePlayer*)(CBaseEntity*)bot->m_leader;
			if ( leader->pev->velocity.LengthSquared() > 40000.0f
				|| ( m_leaderPosition - leader->pev->origin ).LengthSquared() > 62500.0f )
			{
				bot->Follow( leader );
				return;
			}
		}
		if ( manager->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB )
		{
			if ( bot->m_iTeam == CT )
			{
				if ( bot->m_task == CCSBot::TASK_HOLD_POSITION )
				{
					if ( manager->IsBombPlanted() && manager->m_bombPlantTimestamp > bot->m_stateTimestamp )
					{
						bot->Idle();
						return;
					}
				}
				else if ( bot->m_task == CCSBot::TASK_GUARD_BOMB_DEFUSER )
				{
					if ( !manager->GetBombDefuser() )
					{
						bot->Idle();
						return;
					}
				}
				else
				{
					if ( bot->m_task == CCSBot::TASK_GUARD_LOOSE_BOMB && !manager->GetLooseBomb() )
					{
						bot->GetChatter()->TheyPickedUpTheBomb();
						bot->Idle();
						return;
					}
					if ( bot->m_task == CCSBot::TASK_FETCH_BOMB && bot->m_gameState.IsLooseBombLocationKnown() )
					{
						bot->Idle();
						return;
					}
				}
				if ( bot->IsDoingScenario() && bot->m_task == CCSBot::TASK_FETCH_BOMB && manager->IsBombPlanted() )
				{
					bot->Idle();
					return;
				}
			}
			else
			{
				CBasePlayer* defuser = manager->GetBombDefuser();
				if ( defuser && ( defuser->pev->origin - bot->pev->origin ).LengthSquared() < 4000000.0f )
				{
					if ( bot->CanSeePlantedBomb() )
						bot->Attack( defuser );
					else
					{
						bot->MoveTo( &defuser->pev->origin, FASTEST_ROUTE );
						bot->InhibitLookAround( 10.0f );
					}
					return;
				}
			}
		}
		else if ( manager->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES )
		{
			if ( bot->m_task == CCSBot::TASK_GUARD_HOSTAGES )
			{
				if ( bot->m_gameState.AreAllHostagesBeingRescued() || bot->m_gameState.AreAllHostagesGone() )
				{
					bot->Idle();
					return;
				}
			}
			else if ( bot->m_task == CCSBot::TASK_GUARD_HOSTAGE_RESCUE_ZONE )
			{
				CHostage* hostage = bot->m_gameState.GetNearestVisibleFreeHostage();
				if ( hostage )
				{
					CNavArea* area = TheNavAreaGrid.GetNearestNavArea( &hostage->pev->origin, false );
					if ( area )
					{
						bot->m_task = CCSBot::TASK_GUARD_HOSTAGES;
						bot->m_taskEntity = NULL;
						bot->Hide( area, -1.0f, 750.0f, false );
						bot->PrintIfWatched( "I'm guarding hostages I found\n" );
						return;
					}
				}
			}
		}
		weapon = (CBasePlayerWeapon*)bot->m_pActiveItem;
		if ( !( bot->IsSniper() && m_atSpot ) && !( weapon && ( weapon->m_fInReload || weapon->m_fInSpecialReload ) )
			&& bot->GetDisposition() == CCSBot::ENGAGE_AND_INVESTIGATE )
		{
			if ( m_holdPosition && m_heardEnemy && gpGlobals->time - m_holdTimestamp > m_holdTime )
			{
				bot->InvestigateNoise();
				return;
			}
			if ( bot->ShouldInvestigateNoise( NULL ) )
			{
				if ( !m_atSpot || !m_holdPosition )
				{
					bot->InvestigateNoise();
					return;
				}
				if ( !m_heardEnemy )
				{
					m_heardEnemy = true;
					m_holdTimestamp = gpGlobals->time;
					bot->PrintIfWatched( "Heard enemy, holding position for %f2.1 seconds...\n", m_holdTime );
				}
			}
		}
	}
	bot->UpdateLookAround( false );
	if ( m_atSpot )
	{
		bot->Crouch();
		if ( bot->HasShield() && ( !bot->HasShield() || !bot->m_bShieldDrawn ) )
			bot->SecondaryAttack();
		if ( !bot->m_enemyVisible && gpGlobals->time - bot->m_attackerTimestamp < 1.0f )
		{
			bot->Idle();
			return;
		}
		if ( !bot->IsDoingScenario() && bot->m_iTeam == CT )
		{
			if ( bot->m_task == CCSBot::TASK_FETCH_BOMB && bot->IsAtHidingSpot() && manager->IsBombPlanted() )
			{
				int enemies = bot->GetEnemiesRemaining();
				if ( bot->m_nearbyEnemyCount <= enemies )
					enemies = bot->m_nearbyEnemyCount;
				if ( !enemies )
				{
					if ( manager->GetBombTimeLeft() > 30.0f )
						bot->GetChatter()->Encourage( "BombsiteSecure", RANDOM_FLOAT( 10.0f, 15.0f ), 3.0f );
					else if ( manager->GetBombTimeLeft() > 11.0f )
						bot->GetChatter()->Encourage( "WaitingForHumanToDefuseBomb", RANDOM_FLOAT( 5.0f, 8.0f ), 3.0f );
					else
						bot->GetChatter()->Encourage( "WaitingForHumanToDefuseBombPanic", RANDOM_FLOAT( 3.0f, 4.0f ), 3.0f );
				}
			}
			if ( bot->m_task == CCSBot::TASK_GUARD_HOSTAGES && bot->IsAtHidingSpot() )
			{
				int enemies = bot->GetEnemiesRemaining();
				if ( bot->m_nearbyEnemyCount <= enemies )
					enemies = bot->m_nearbyEnemyCount;
				if ( !enemies && bot->m_gameState.GetNearestVisibleFreeHostage() )
					bot->GetChatter()->Encourage( "WaitingForHumanToRescueHostages", RANDOM_FLOAT( 10.0f, 15.0f ), 3.0f );
			}
		}
		return;
	}
	float distance;
	CBasePlayer* closest = UTIL_GetClosestPlayer( &m_hidingSpot, &distance );
	if ( closest && bot != closest && distance < 75.0f && bot->IsVisible( closest, true, NULL ) )
	{
		bot->PrintIfWatched( "Someone's in my hiding spot - picking another...\n" );
		if ( m_spotRetries++ <= 2 )
			bot->Hide( TheNavAreaGrid.GetNavArea( &m_hidingSpot, 120.0f ), -1.0f, 750.0f, false );
		else
		{
			bot->PrintIfWatched( "Can't find a free hiding spot, giving up.\n" );
			bot->Idle();
		}
		return;
	}
	Vector feet = bot->pev->origin;
	feet.z -= bot->IsCrouching() ? 18.0f : 36.0f;
	float range = ( m_hidingSpot - feet ).Length();
	if ( range < 200.0f )
		bot->Crouch();
	if ( range < 20.0f )
	{
		m_atSpot = true;
		bot->ComputeApproachPoints();
		bot->EquipBestWeapon( true );
		bot->SetDisposition( CCSBot::OPPORTUNITY_FIRE );
		if ( bot->m_task == CCSBot::TASK_MOVE_TO_SNIPER_SPOT )
		{
			bot->m_task = CCSBot::TASK_SNIPING;
			bot->m_taskEntity = NULL;
		}
		float longest = 0.0f;
		float bestYaw = 0.0f;
		for ( float yaw = 0.0f; yaw < 360.0f; yaw += 45.0f )
		{
			float sine = BotSIN( yaw );
			float cosine = BotCOS( yaw );
			bot->m_eyePosition = bot->pev->origin + bot->pev->view_ofs;
			Vector end = bot->m_eyePosition + Vector( cosine * 1000.0f, sine * 1000.0f, 0.0f );
			bot->m_eyePosition = bot->pev->origin + bot->pev->view_ofs;
			TraceResult trace;
			UTIL_TraceLine( bot->m_eyePosition, end, ignore_monsters, ignore_glass, bot->edict(), &trace );
			if ( trace.flFraction > longest )
			{
				longest = trace.flFraction;
				bestYaw = yaw;
			}
		}
		bot->m_lookAheadYaw = bestYaw;
	}
	if ( bot->UpdatePathMovement( true ) != 0 && !m_atSpot )
	{
		bot->PrintIfWatched( "Can't get to my hiding spot - finding another...\n" );
		const Vector* spot = FindNearbyHidingSpot( bot, &m_hidingSpot, m_searchArea, m_maxRange, bot->IsSniper(), false );
		if ( spot )
			m_hidingSpot = *spot;
		else
		{
			bot->PrintIfWatched( "No available hiding spots - hiding where I'm at.\n" );
			m_hidingSpot.x = bot->pev->origin.x;
			m_hidingSpot.x = bot->pev->origin.y;
			m_hidingSpot.z = bot->pev->origin.z - ( bot->IsCrouching() ? 18.0f : 36.0f );
		}
		CNavArea* area = TheNavAreaGrid.GetNavArea( &m_hidingSpot, 120.0f );
		if ( !bot->ComputePath( area, &m_hidingSpot, FASTEST_ROUTE ) )
		{
			bot->PrintIfWatched( "Can't pathfind to hiding spot\n" );
			bot->Idle();
		}
	}
}

const char* HideState::GetName() const
{
	return "Hide";
}
