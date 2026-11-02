#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "bot_profile.h"

void AttackState::StopAttacking( CCSBot* bot )
{
	if ( bot->m_task == CCSBot::TASK_SNIPING )
		bot->Hide( bot->m_lastKnownArea, -1.0f, 50.0f, false );
	else
		bot->StopAttacking();
}

void AttackState::OnEnter( CCSBot* bot )
{
	CBasePlayer* enemy = (CBasePlayer*)(CBaseEntity*)bot->m_enemy;
	bot->PushPostureContext();
	bot->m_pathLength = 0;
	bot->m_ladder = NULL;
	if ( enemy && bot->IsUsingKnife() && !bot->IsPlayerFacingMe( enemy ) )
		bot->Walk();
	else
		bot->Run();
	bot->GetOffLadder();
	bot->ResetStuckMonitor();
	m_repathTimer.Invalidate();
	m_nextDodgeTime = 0.0f;
	m_firstDodge = true;
	m_lostEnemy = false;
	m_reacquireFireTime = 0.0f;
	m_haveSeenEnemy = bot->m_enemyVisible;
	float time = gpGlobals->time;
	m_nextRetreatTime = time + RANDOM_FLOAT( 7.0f, 10.0f );
	time = gpGlobals->time;
	m_shieldToggleTime = time + RANDOM_FLOAT( 2.0f, 10.0f );
	m_shieldAttackMode = false;
	if ( bot->IsEscapingFromBomb() )
		bot->EquipBestWeapon( false );
	if ( bot->IsUsingKnife() )
	{
		m_crouchAndHold = false;
		bot->StandUp();
	}
	else
	{
		if ( enemy && !m_crouchAndHold )
		{
			float chance = 50.0f;
			if ( !bot->IsUsingSniperRifle() && ( bot->pev->origin - enemy->pev->origin ).LengthSquared() <= 562500.0f )
				chance = ( 1.0f - bot->GetProfile()->GetAggression() ) * 20.0f;
			if ( chance > RANDOM_FLOAT( 0.0f, 100.0f ) )
			{
				Vector start = bot->pev->origin;
				if ( !bot->IsCrouching() )
					start.z -= 20.0f;
				TraceResult trace;
				UTIL_TraceLine( start, enemy->EyePosition(), ignore_monsters, ignore_glass, bot->edict(), &trace );
				if ( trace.flFraction == 1.0f )
					m_crouchAndHold = true;
			}
		}
		if ( m_crouchAndHold )
		{
			bot->Crouch();
			bot->PrintIfWatched( "Crouch and hold attack!\n" );
		}
	}
	m_scopeTimestamp = 0.0f;
	m_checkedRetreatWhenLost = false;
	float skill = bot->GetProfile()->GetSkill();
	float chance = skill * 80.0f;
	if ( bot->IsUsingKnife() )
		chance += chance;
	if ( skill > 0.5f && bot->IsOutnumbered() )
		chance = 100.0f;
	m_shouldDodge = chance > RANDOM_FLOAT( 0.0f, 100.0f );
	m_retreatWhenOutnumbered = RANDOM_FLOAT( 0.0f, 100.0f ) > 100.0f * bot->GetProfile()->GetAggression();
}

void AttackState::OnExit( CCSBot* bot )
{
	bot->PrintIfWatched( "AttackState:OnExit()\n" );
	m_crouchAndHold = false;
	bot->m_noiseTimestamp = 0.0f;
	bot->ResetStuckMonitor();
	bot->PopPostureContext();
	if ( bot->HasShield() && bot->m_bShieldDrawn )
		bot->SecondaryAttack();
	bot->m_isRapidFire = false;
	bot->m_surpriseDelay = 0.0f;
	bot->m_lastPanicTime = 0.0f;
}

void AttackState::OnUpdate( CCSBot* bot )
{
	bot->ResetStuckMonitor();
	bot->m_isRapidFire = false;
	if ( bot->m_pActiveItem )
	{
		int id = bot->m_pActiveItem->m_iId;
		if ( id == WEAPON_HEGRENADE || id == WEAPON_C4 || id == WEAPON_SMOKEGRENADE || id == WEAPON_FLASHBANG )
			bot->EquipBestWeapon( false );
	}
	CBasePlayer* enemy = (CBasePlayer*)(CBaseEntity*)bot->m_enemy;
	if ( !enemy )
	{
		StopAttacking( bot );
		return;
	}
	if ( !m_haveSeenEnemy )
		m_haveSeenEnemy = bot->m_enemyVisible;
	const BotProfile* profile = bot->GetProfile();
	if ( m_retreatTimer.IsElapsed() && ( gpGlobals->time > m_nextRetreatTime
		|| ( bot->IsOutnumbered() && m_retreatWhenOutnumbered )
		|| ( bot->OutnumberedCount() > 1 && profile->GetAggression() < 1.0f ) ) )
	{
		if ( gpGlobals->time > m_nextRetreatTime )
			bot->GetChatter()->PinnedDown();
		else
			bot->GetChatter()->Scared();
		m_retreatTimer.Start( RANDOM_FLOAT( 3.0f, 15.0f ) );
		if ( bot->TryToRetreat() )
		{
			if ( bot->IsUsingSniperRifle() )
				bot->EquipPistol();
		}
		else
			bot->PrintIfWatched( "I want to retreat, but no safe spots nearby!\n" );
	}
	if ( bot->IsUsingKnife() )
	{
		m_crouchAndHold = false;
		bot->StandUp();
		if ( !bot->IsPlayerFacingMe( enemy ) )
			bot->Walk();
		else
		{
			bot->Run();
			bot->m_hurryTimer.Start( 5.0f );
			bot->m_inhibitFollowingTimer.Start( 10.0f );
		}
		bot->FireWeaponAtEnemy();
		if ( !bot->m_pathLength
			|| ( bot->m_path[bot->m_pathLength - 1].position - enemy->pev->origin ).LengthSquared() > 10000.0f )
		{
			if ( m_repathTimer.IsElapsed() )
			{
				CNavArea* area = TheNavAreaGrid.GetNearestNavArea( &enemy->pev->origin, false );
				bot->ComputePath( area, &enemy->pev->origin, FASTEST_ROUTE );
				m_repathTimer.Start( 0.5f );
			}
		}
		if ( bot->UpdatePathMovement( false ) != 0 )
		{
			bot->m_pathLength = 0;
			bot->m_ladder = NULL;
		}
		return;
	}
	if ( bot->HasShield() )
	{
		bool defend = bot->m_enemyVisible && !m_shieldAttackMode && !bot->IsRecognizedEnemyReloading()
			&& ( !bot->m_pActiveItem || !((CBasePlayerWeapon*)bot->m_pActiveItem)->m_fInReload )
			&& bot->IsPlayerLookingAtMe( enemy );
		if ( defend )
		{
			if ( !bot->HasShield() || !bot->m_bShieldDrawn )
				bot->SecondaryAttack();
		}
		else if ( bot->HasShield() && bot->m_bShieldDrawn )
			bot->SecondaryAttack();
		if ( gpGlobals->time > m_shieldToggleTime )
		{
			float time = gpGlobals->time;
			float duration = RANDOM_FLOAT( 0.5f, 2.0f );
			m_shieldAttackMode = !m_shieldAttackMode;
			m_shieldToggleTime = time + duration;
		}
	}
	float rangeSquared = ( enemy->pev->origin - bot->pev->origin ).LengthSquared();
	if ( bot->IsUsingSniperRifle() )
	{
		if ( rangeSquared < 96100.0f )
			bot->EquipPistol();
	}
	else if ( bot->IsUsingShotgun() && rangeSquared > 1000000.0f )
		bot->EquipPistol();
	if ( bot->IsUsingSniperRifle() )
	{
		if ( bot->m_bResumeZoom )
		{
			m_scopeTimestamp = gpGlobals->time;
			return;
		}
		float range = ( bot->m_aimSpot - bot->pev->origin ).Length();
		if ( bot->m_iFOV > 60 && bot->AdjustZoom( range ) )
			m_scopeTimestamp = gpGlobals->time;
		if ( profile->GetReactionTime() + 0.2f > gpGlobals->time - m_scopeTimestamp )
			return;
	}
	if ( bot->m_enemyDeathTimestamp != 0.0f && ( !bot->m_enemy
		|| ( !bot->m_enemy->IsAlive() && gpGlobals->time - bot->m_enemyDeathTimestamp > 1.0f - profile->GetSkill() ) ) )
	{
		if ( bot->m_lastVictimIndex == enemy->entindex() )
		{
			int enemies = bot->GetEnemiesRemaining();
			if ( bot->m_nearbyEnemyCount <= enemies )
				enemies = bot->m_nearbyEnemyCount;
			if ( enemies <= 1 )
				bot->GetChatter()->KilledMyEnemy( enemy->entindex() );
		}
		StopAttacking( bot );
		return;
	}
	float unseen = gpGlobals->time - bot->m_lastSawEnemyTime;
	if ( bot->m_enemyVisible )
	{
		m_checkedRetreatWhenLost = false;
		if ( m_lostEnemy )
		{
			m_lostEnemy = false;
			m_reacquireFireTime = gpGlobals->time + profile->GetReactionTime();
		}
	}
	else
	{
		if ( unseen > 0.5f && bot->CanHearNearbyEnemyGunfire( -1.0f ) )
		{
			StopAttacking( bot );
			bot->SetLookAt( "Nearby enemy gunfire", bot->m_noiseTimestamp > 0.0f ? &bot->m_noisePosition : NULL,
				PRIORITY_HIGH, 0.0f, false, 5.0f );
			bot->PrintIfWatched( "Checking nearby threatening enemy gunfire!\n" );
			return;
		}
		if ( unseen > 0.25f )
			m_lostEnemy = true;
		if ( unseen > 0.1f )
		{
			if ( bot->GetDisposition() != CCSBot::ENGAGE_AND_INVESTIGATE )
			{
				StopAttacking( bot );
				return;
			}
			if ( m_haveSeenEnemy && !m_checkedRetreatWhenLost )
			{
				if ( !( RANDOM_FLOAT( 0.0f, 100.0f ) >= 33.3f ) )
				{
					float duration = RANDOM_FLOAT( 3.0f, 15.0f );
					const Vector* retreat = FindNearbyRetreatSpot( bot, 200.0f );
					if ( retreat )
					{
						bot->IgnoreEnemies( 1.0f );
						bot->Run();
						bot->StandUp();
						bot->Hide( retreat, duration, true );
						return;
					}
				}
				m_checkedRetreatWhenLost = true;
			}
		}
	}
	float chaseDelay = 1.0f - profile->GetAggression() + 1.0f - profile->GetAggression() + 2.0f;
	if ( bot->IsUsingSniperRifle() )
		chaseDelay += 3.0f;
	else if ( bot->IsCrouching() )
		chaseDelay += 1.0f;
	if ( !bot->m_enemyVisible && ( unseen > chaseDelay || !m_haveSeenEnemy ) )
	{
		if ( bot->m_task == CCSBot::TASK_SNIPING )
			StopAttacking( bot );
		else
		{
			bot->m_task = CCSBot::TASK_CHASE_ENEMY;
			bot->m_taskEntity = enemy;
			bot->MoveTo( &bot->m_lastEnemyPosition, SAFEST_ROUTE );
		}
		return;
	}
	if ( !bot->m_enemyVisible && gpGlobals->time - bot->m_attackerTimestamp < 3.0f
		&& bot->GetAttacker() && bot->GetAttacker() != (CBaseEntity*)bot->m_enemy )
	{
		if ( bot->IsVisible( bot->GetAttacker(), true, NULL ) )
		{
			bot->Attack( bot->GetAttacker() );
			bot->PrintIfWatched( "Switching targets to retaliate against new attacker!\n" );
		}
		return;
	}
	if ( gpGlobals->time > m_reacquireFireTime )
		bot->FireWeaponAtEnemy();
	if ( !m_shouldDodge || bot->IsUsingSniperRifle() || m_crouchAndHold )
		return;
	float distance = ( enemy->pev->origin - bot->pev->origin ).Length2D();
	if ( profile->GetSkill() < 0.66f || !bot->m_enemyVisible )
	{
		if ( distance <= bot->m_combatRange + 125.0f )
		{
			if ( bot->m_combatRange - 125.0f > distance )
				bot->MoveBackward();
		}
		else
			bot->MoveForward();
	}
	if ( distance > 2000.0f || !bot->IsPlayerFacingMe( enemy ) )
	{
		m_dodgeState = 0;
		m_nextDodgeTime = 0.0f;
		return;
	}
	if ( gpGlobals->time >= m_nextDodgeTime )
	{
		int state;
		if ( m_firstDodge && profile->GetSkill() > 0.5f && RANDOM_FLOAT( 0.0f, 100.0f ) < 33.3f
			&& !( bot->pev->velocity.LengthSquared() < 100.0f ) )
			state = RANDOM_LONG( 0, 3 );
		else
			state = RANDOM_LONG( 0, 2 );
		while ( !m_firstDodge && state == m_dodgeState )
			state = RANDOM_LONG( 0, 2 );
		m_dodgeState = state;
		float time = gpGlobals->time;
		m_nextDodgeTime = time + RANDOM_FLOAT( 0.3f, 1.0f );
		m_firstDodge = false;
	}
	switch ( m_dodgeState )
	{
	case 1: bot->StrafeLeft(); break;
	case 2: bot->StrafeRight(); break;
	case 3:
		if ( bot->m_enemyVisible )
			bot->Jump( false );
		break;
	}
}

const char* AttackState::GetName() const
{
	return "Attack";
}
