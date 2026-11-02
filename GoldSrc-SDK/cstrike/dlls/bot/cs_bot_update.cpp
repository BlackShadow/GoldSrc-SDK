#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"
#include "gamerules.h"

void CCSBot::Update()
{
	CCSBotManager* manager = TheCSBots();
	if ( CCSBotManager::m_isAnalysisRequested && m_process == 0 )
	{
		CCSBotManager::m_isAnalysisRequested = false;
		StartAnalyzeAlphaProcess();
	}

	switch ( m_process )
	{
	case 1:
		UpdateLearnProcess();
		return;
	case 2:
		UpdateAnalyzeAlphaProcess();
		return;
	case 3:
		UpdateAnalyzeBetaProcess();
		return;
	case 4:
		UpdateSaveProcess();
		return;
	}

	m_chatter.Update();
	if ( m_voiceEndTime != 0.0f && ( gpGlobals->time >= m_voiceEndTime || m_voiceStartTime > gpGlobals->time ) )
		EndVoiceFeedback( false );

	if ( !IsAlive() )
	{
		m_diedLastRound = true;
		BotDeathThink();
		return;
	}

	if ( ( cv_bot_traceview.value == 100.0f && IsLocalPlayerWatchingMe() ) || cv_bot_traceview.value == 101.0f )
	{
		Vector angles = pev->v_angle + pev->punchangle;
		UTIL_MakeVectors( angles );
		Vector forward = gpGlobals->v_forward;
		Vector eye = EyePosition();
		if ( !IsFriendInLineOfFire() )
		{
			Vector end = eye + forward * 2000.0f;
			if ( m_iTeam == TERRORIST )
				UTIL_DrawBeamPoints( eye, end, 1, 255, 0, 0 );
			else
				UTIL_DrawBeamPoints( eye, end, 1, 0, 50, 255 );
		}
	}

	if ( ( cv_bot_traceview.value == 2.0f && IsLocalPlayerWatchingMe() ) || cv_bot_traceview.value == 3.0f )
		DrawApproachPoints();

	if ( ( cv_bot_traceview.value == 4.0f && IsLocalPlayerWatchingMe() ) || cv_bot_traceview.value == 5.0f )
	{
		if ( m_spotEncounter )
		{
			UTIL_DrawBeamPoints( m_spotEncounter->path.from, m_spotEncounter->path.to, 3, 0, 0, 255 );
			Vector direction = m_spotEncounter->path.to - m_spotEncounter->path.from;
			float length = direction.NormalizeInPlace();
			for ( SpotOrderList::const_iterator spot = m_spotEncounter->spotList.begin();
				spot != m_spotEncounter->spotList.end(); ++spot )
			{
				Vector position = m_spotEncounter->path.from + direction * ( length * spot->t );
				UTIL_DrawBeamPoints( position, *spot->spot->GetPosition(), 3, 0, 255, 255 );
			}
		}
	}

	if ( cv_bot_traceview.value == 1.0f && IsLocalPlayerWatchingMe() )
	{
		m_eyePosition = pev->origin + pev->view_ofs;
		double angle = M_PI * m_lookAheadYaw / 180.0;
		float forwardX = (float)( 50.0 * cos( angle ) );
		float forwardY = (float)( 50.0 * sin( angle ) );
		Vector end( m_eyePosition.x + forwardX, m_eyePosition.y + forwardY, m_eyePosition.z );
		UTIL_DrawBeamPoints( m_eyePosition, end, 1, 0, 255, 255 );
	}

	if ( cv_bot_stop.value != 0.0f )
		return;

	StuckCheck();
	if ( m_noiseTimestamp > 0.0f && gpGlobals->time - m_noiseTimestamp > 20.0f )
		m_noiseTimestamp = 0.0f;

	if ( !m_area || !m_area->Contains( &pev->origin ) )
		m_area = TheNavAreaGrid.GetNavArea( &pev->origin, 120.0f );
	if ( m_area && m_area != m_lastKnownArea )
	{
		m_lastKnownArea = m_area;
		m_area->SetClearedTimestamp( m_iTeam - 1 );
	}

	if ( ( m_lastOrigin - pev->origin ).LengthSquared() > 2500.0f )
	{
		ComputeApproachPoints();
		m_lastOrigin = pev->origin;
	}
	if ( cv_bot_show_nav.value > 0.0f && m_lastKnownArea )
		m_lastKnownArea->DrawConnectedAreas();

	if ( m_blindUntilTime > gpGlobals->time )
	{
		if ( !IsAtHidingSpot() )
		{
			switch ( m_blindMoveDirection )
			{
			case 0:
				MoveForward();
				break;
			case 1:
				StrafeRight();
				break;
			case 2:
				MoveBackward();
				break;
			case 3:
				StrafeLeft();
				break;
			default:
				Crouch();
				break;
			}
		}
		if ( m_blindFire )
			PrimaryAttack();
		return;
	}

	UpdateReactionQueue();
	CBasePlayer* threat = GetRecognizedEnemy();
	if ( threat )
	{
		AdjustSafeTime();
		if ( IsUsingGrenade() )
		{
			ThrowGrenade( &threat->pev->origin );
		}
		else if ( GetDisposition() != IGNORE_ENEMIES &&
			( GetDisposition() == ENGAGE_AND_INVESTIGATE || GetDisposition() == OPPORTUNITY_FIRE ||
			IsPlayerLookingAtMe( threat ) || ( pev->origin - threat->pev->origin ).LengthSquared() < 562500.0f ) )
		{
			if ( !m_enemy || threat != (CBasePlayer*)(CBaseEntity*)m_enemy || !IsAttacking() )
			{
				if ( !IsUsingKnife() || !IsHiding() ||
					( pev->origin - threat->pev->origin ).LengthSquared() < 62500.0f )
				{
					Attack( threat );
				}
			}
		}
		else
		{
			SetEnemy( threat );
			m_enemyVisible = true;
		}

		if ( GetDisposition() != IGNORE_ENEMIES && !IsAttacking() && gpGlobals->time - m_attackerTimestamp < 1.0f )
		{
			Attack( threat );
			PrintIfWatched( "Ouch! Retaliating!\n" );
		}
		manager->m_lastSeenEnemyTimestamp = gpGlobals->time;
	}

	if ( m_enemy )
	{
		if ( m_enemyDeathTimestamp == 0.0f ||
			( m_enemy && ( m_enemy->IsAlive() || gpGlobals->time - m_enemyDeathTimestamp <= 1.0f - m_profile->GetSkill() ) ) )
		{
			if ( IsVisible( (CBasePlayer*)(CBaseEntity*)m_enemy, false, &m_visibleEnemyParts ) )
			{
				m_enemyVisible = true;
				m_lastSawEnemyTime = gpGlobals->time;
				m_lastEnemyPosition = m_enemy->pev->origin;
			}
			else
			{
				m_enemyVisible = false;
			}
			if ( m_enemy->IsAlive() )
			{
				m_enemyDeathTimestamp = 0.0f;
				m_enemyDead = false;
			}
			else if ( m_enemyDeathTimestamp == 0.0f )
			{
				m_enemyDead = true;
				m_enemyDeathTimestamp = gpGlobals->time;
			}
		}
		else
		{
			m_enemy = NULL;
			m_enemyVisible = false;
		}
	}
	else
	{
		m_enemyVisible = false;
	}
	m_isAimingAtEnemy = m_enemy && gpGlobals->time - m_lastSawEnemyTime < 3.0f;
	if ( GetDisposition() == IGNORE_ENEMIES )
		FireWeaponAtEnemy();

	Vector grenadeTarget;
	if ( IsEndOfSafeTime() && IsUsingGrenade() && ( IsWellPastSafe() || !IsUsingHEGrenade() ) &&
		!m_isWaitingToThrowGrenade && FindGrenadeTossPathTarget( &grenadeTarget ) )
	{
		ThrowGrenade( &grenadeTarget );
	}
	if ( IsUsingGrenade() )
	{
		if ( m_isWaitingToThrowGrenade && ( m_grenadeTossTimer.IsElapsed() || m_lookAtState == 2 ) )
		{
			ClearPrimaryAttack();
			m_isWaitingToThrowGrenade = false;
		}
		else
		{
			PrimaryAttack();
		}
	}
	else
	{
		m_isWaitingToThrowGrenade = false;
	}
	if ( IsHunting() && IsWellPastSafe() && IsUsingGrenade() )
		EquipBestWeapon( true );
	if ( !IsSafe() && !IsUsingGrenade() && IsActiveWeaponOutOfAmmo() )
		EquipBestWeapon( false );
	if ( !IsSafe() && !IsUsingGrenade() && IsUsingKnife() && !IsEscapingFromBomb() )
		EquipBestWeapon( false );

	CBasePlayerWeapon* weapon = GetActiveWeapon();
	if ( ( !weapon || ( !weapon->m_fInReload && !weapon->m_fInSpecialReload ) ) && IsUsingPistol() &&
		!IsPrimaryWeaponEmpty() && gpGlobals->time - m_lastSawEnemyTime > 5.0f )
	{
		EquipBestWeapon( false );
	}
	ReloadCheck();
	SilencerCheck();
	RespondToRadioCommands();

	if ( gpGlobals->time - m_nearbyPlayerTimestamp < 0.33f && m_nearbyPlayer )
		StrafeAwayFromPosition( &m_nearbyPlayer->pev->origin );
	else
		m_nearbyPlayer = NULL;

	if ( m_isJumpCrouching )
	{
		float elapsed = gpGlobals->time - m_jumpCrouchTimestamp;
		if ( elapsed > 0.05f && elapsed < 0.6f )
			Crouch();
		if ( elapsed >= 0.6f )
			StandUp();
		if ( elapsed > 0.75f )
			m_isJumpCrouching = false;
	}

	if ( !IsAtHidingSpot() && !IsAttacking() && IsUsingSniperRifle() && IsUsingScope() )
		SecondaryAttack();
	UpdatePeripheralVision();
	if ( m_visibleBomber )
		m_chatter.SpottedBomber( (CBasePlayer*)(CBaseEntity*)m_visibleBomber );
	if ( CanSeeLooseBomb() )
		m_chatter.SpottedLooseBomb( manager->m_looseBomb );

	if ( manager->m_scenario == CCSBotManager::SCENARIO_DEFUSE_BOMB )
	{
		float escapeTime = 8.0f - ( m_profile->GetAggression() + m_profile->GetAggression() );
		if ( m_bHasDefuser )
			escapeTime *= 0.66f;
		if ( !IsEscapingFromBomb() && manager->m_isBombPlanted && m_gameState.IsPlantedBombLocationKnown() &&
			escapeTime > manager->GetBombTimeLeft() && !IsDefusingBomb() && !IsAttacking() )
		{
			EscapeFromBomb();
		}
	}
	else if ( manager->m_scenario == CCSBotManager::SCENARIO_RESCUE_HOSTAGES )
	{
		if ( m_iTeam == CT )
		{
			UpdateHostageEscortCount();
		}
		else
		{
			unsigned int changes = m_gameState.ValidateHostagePositions();
			if ( changes & 4 )
			{
				m_chatter.HostagesTaken();
				Idle();
			}
			else if ( changes & 2 )
			{
				m_gameState.SetHostageTaken( true );
				Idle();
			}
		}
	}

	if ( gpGlobals->time - manager->m_roundStartTimestamp > 5.0f && m_profile->GetTeamwork() > 0.4f &&
		gpGlobals->time > m_allowAutoFollowTime && !IsBusy() )
	{
		if ( !m_isFollowing && !m_gameState.IsAtPlantedBombsite() )
		{
			float teamwork = m_profile->GetTeamwork();
			if ( teamwork > RANDOM_FLOAT( 0.0f, 1.0f ) )
			{
				CBasePlayer* leader = (CBasePlayer*)(CBaseEntity*)m_closestHumanFriend;
				if ( leader && gpGlobals->time > leader->m_allowAutoFollowTime && GetBotFollowCount( leader ) < 2 &&
					( leader->pev->origin - pev->origin ).LengthSquared() < 90000.0f )
				{
					CNavArea* leaderArea = TheNavAreaGrid.GetNavArea( &leader->pev->origin, 120.0f );
					if ( leaderArea )
					{
						PathCost cost( this, FASTEST_ROUTE );
						if ( NavAreaTravelDistance( m_lastKnownArea, leaderArea, cost ) < 300.0f )
						{
							Follow( leader );
							PrintIfWatched( "Auto-Following %s\n", STRING( leader->pev->netname ) );
							if ( ( (CHalfLifeMultiplay*)g_pGameRules )->IsCareer() )
								m_chatter.Say( "FollowingCommander", 10.0f, 0.0f );
							else
								m_chatter.Say( "FollowingSir", 10.0f, 0.0f );
						}
					}
				}
			}
			else
			{
				m_allowAutoFollowTime = ( 1.0f - m_profile->GetTeamwork() ) * 30.0f + ( gpGlobals->time + 15.0f );
			}
		}
	}

	if ( m_isFollowing )
	{
		CBaseEntity* leader = m_leader;
		if ( !leader || !leader->IsAlive() )
			StopFollowing();
		float teamwork = m_profile->GetTeamwork();
		if ( teamwork < 0.85f && gpGlobals->time - m_followStartTime > teamwork * 40.0f + 15.0f )
		{
			StopFollowing();
			PrintIfWatched( "Stopping following - bored\n" );
		}
	}
	if ( !m_isFollowing && m_morale < 0 && IsSafe() && GetSafeTimeRemaining() < 2.0f && IsHunting() )
	{
		float chance = m_morale * -40.0f;
		if ( chance > RANDOM_FLOAT( 0.0f, 100.0f ) && ( manager->IsOnOffense( this ) || !manager->m_isDefenseRushing ) )
		{
			SetDisposition( OPPORTUNITY_FIRE );
			float duration = RANDOM_FLOAT( 3.0f, 15.0f );
			Hide( m_lastKnownArea, duration, 750.0f, false );
			m_chatter.Say( "WaitingHere", 3.0f, 0.0f );
		}
	}

	if ( m_isAttacking )
		m_attackState.OnUpdate( this );
	else
		m_state->OnUpdate( this );

	if ( m_isWaitingToThrowGrenade )
	{
		ResetStuckMonitor();
		ClearMovement();
	}
	weapon = GetActiveWeapon();
	if ( weapon && weapon->m_fInReload && !m_enemyVisible )
	{
		ResetStuckMonitor();
		ClearMovement();
	}
	if ( !IsAttacking() && m_hostageWaitTimer.IsElapsed() && m_task == TASK_RESCUE_HOSTAGES &&
		GetRangeToFarthestEscortedHostage() > 500.0f )
	{
		if ( m_isWaitingForHostage )
		{
			if ( !m_hostageWaitTimeout.IsElapsed() )
			{
				ResetStuckMonitor();
				ClearMovement();
			}
			else
			{
				m_isWaitingForHostage = false;
				m_hostageWaitTimer.Start( 3.0f );
			}
		}
		else
		{
			m_isWaitingForHostage = true;
			m_hostageWaitTimeout.Start( 10.0f );
		}
	}
	m_wasSafe = IsSafe();
}
