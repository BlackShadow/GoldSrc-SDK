#pragma warning( push )
#pragma warning( disable: 4100 )
#include "cs_bot.h"
#include "gamerules.h"
#pragma warning( pop )
#include "cs_bot_manager.h"
#include "client.h"
#include "debug.h"

void CCSBot::Panic( CBasePlayer *enemy )
{
	if ( gpGlobals->time - m_lastPanicTime < 5 )
		return;
	float sine = BotSIN( pev->v_angle.y );
	float cosine = BotCOS( pev->v_angle.y );
	Vector position;
	if ( GetProfile()->GetSkill() >= 0.5f )
	{
		Vector direction = enemy->pev->origin - pev->origin;
		float length = direction.Length2D();
		if ( length <= 0 )
		{
			direction.x = 1;
			direction.y = 0;
		}
		else
		{
			direction.x /= length;
			direction.y /= length;
		}
		float ahead = cosine * direction.x + sine * direction.y;
		float offset = RANDOM_FLOAT( -75, 75 );
		if ( ahead > 0.7071f )
		{
			position.x = pev->origin.x + cosine * 100 - sine * offset;
			position.y = pev->origin.y + sine * 100 + cosine * offset;
		}
		else if ( ahead < -0.7071f )
		{
			position.x = pev->origin.x - cosine * 100 - sine * offset;
			position.y = pev->origin.y - sine * 100 + cosine * offset;
		}
		else if ( -sine * direction.x + cosine * direction.y <= 0 )
		{
			position.x = pev->origin.x + sine * 100 + cosine * offset;
			position.y = pev->origin.y - cosine * 100 + sine * offset;
		}
		else
		{
			position.x = pev->origin.x - sine * 100 + cosine * offset;
			position.y = pev->origin.y + cosine * 100 + sine * offset;
		}
	}
	else
	{
		float offset = RANDOM_FLOAT( -200, 200 ) * 2;
		position.x = pev->origin.x - cosine * 200 - sine * offset;
		position.y = pev->origin.y - sine * 200 + cosine * offset;
	}
	float height = pev->origin.z;
	position.z = RANDOM_FLOAT( -50, 50 ) + height;
	m_surpriseDelay = RANDOM_FLOAT( 0.1f, 0.2f );
	m_lastPanicTime = gpGlobals->time;
	SetLookAt( "Panic", &position, PRIORITY_HIGH, 0, false, 5 );
	PrintIfWatched( "Aaaah!\n" );
}

int CCSBot::TakeDamage( entvars_t *inflictor, entvars_t *attacker, float damage, int damageType )
{
	CBaseEntity *entity = GetClassPtr( (CBaseEntity *)inflictor );
	if ( entity->IsPlayer() && ((CBasePlayer *)entity)->m_iTeam == m_iTeam && !((CBasePlayer *)entity)->IsBot() )
		m_chatter.FriendlyFire();
	if ( entity->IsPlayer() && IsEnemy( entity ) )
	{
		CBasePlayer *oldAttacker = m_attacker;
		float oldTimestamp = m_attackerTimestamp;
		m_attacker = (CBasePlayer *)entity;
		m_attackerTimestamp = gpGlobals->time;
		float elapsed = TheCSBots()->GetElapsedRoundTime();
		if ( m_safeTime > elapsed )
			m_safeTime = elapsed - 2;
		if ( gpGlobals->time - m_lastPanicTime >= 5 && (gpGlobals->time != oldTimestamp || oldAttacker != entity) &&
			!IsVisible( (CBasePlayer *)entity, true, NULL ) )
		{
			float surpriseChance = 1 - GetProfile()->GetSkill();
			surpriseChance *= 50 * surpriseChance;
			if ( !IsAttacking() || !m_enemyVisible || surpriseChance > RANDOM_FLOAT( 0, 100 ) )
				Panic( m_attacker );
		}
	}
	return CBasePlayer::TakeDamage( inflictor, attacker, damage, damageType );
}

void CCSBot::Killed( entvars_t *attacker, int gib )
{
	PrintIfWatched( "Killed( attacker = %s )\n", STRING( attacker->netname ) );
	m_chatter.OnDeath();
	IncreaseDangerNearby( m_iTeam - 1, 1, m_lastKnownArea, &pev->origin, 500 );
	EndVoiceFeedback( true );
	CBasePlayer::Killed( attacker, gib );
}

void CCSBot::OnTouchingWeapon( CWeaponBox *box )
{
	CBasePlayerItem *weapon = box->m_rgpPlayerItems[1];
	CBasePlayerItem *current = m_rgpPlayerItems[1];
	if ( !weapon || !current || weapon->m_iId == current->m_iId || !GetProfile()->HasPrimaryPreference() ||
		gpGlobals->time - m_lastSawEnemyTime < 2.5f )
		return;
	for ( int i = 0; GetProfile()->GetWeaponPreferenceCount() != 0; ++i )
	{
		int preference = GetProfile()->GetWeaponPreference( i );
		if ( IsPrimaryWeapon( preference ) )
		{
			if ( current->m_iId == preference )
				return;
			if ( weapon->m_iId == preference )
			{
				DropPrimary( this );
				return;
			}
		}
	}
}

void CCSBot::BotTouch( CBaseEntity *entity )
{
	if ( entity->IsPlayer() )
	{
		if ( !IsDefusingBomb() )
		{
			unsigned int priority = TheCSBots()->GetPlayerPriority( (CBasePlayer *)entity );
			if ( priority <= TheCSBots()->GetPlayerPriority( this ) &&
				(!m_nearbyPlayer || priority <= TheCSBots()->GetPlayerPriority( (CBasePlayer *)(CBaseEntity *)m_nearbyPlayer )) )
			{
				m_nearbyPlayer = entity;
				m_nearbyPlayerTimestamp = gpGlobals->time;
			}
		}
		return;
	}
	if ( entity->pev->takedamage != DAMAGE_YES || IsAttacking() || !FClassnameIs( entity->pev, "func_breakable" ) )
		return;
	Vector center = (entity->pev->absmin + entity->pev->absmax) * 0.5f;
	if ( m_pathLength )
	{
		const Vector &lo = entity->pev->absmin;
		const Vector &hi = entity->pev->absmax;
		unsigned int startSide = 0;
		unsigned int endSide = 0;
		if ( pev->origin.x > hi.x ) startSide |= 1;
		if ( pev->origin.x < lo.x ) startSide |= 2;
		if ( pev->origin.y > hi.y ) startSide |= 4;
		if ( pev->origin.y < lo.y ) startSide |= 8;
		if ( pev->origin.z > hi.z ) startSide |= 16;
		if ( pev->origin.z < lo.z ) startSide |= 32;
		if ( m_goalPosition.x > hi.x ) endSide |= 1;
		if ( m_goalPosition.x < lo.x ) endSide |= 2;
		if ( m_goalPosition.y > hi.y ) endSide |= 4;
		if ( m_goalPosition.y < lo.y ) endSide |= 8;
		if ( m_goalPosition.z + 36 > hi.z ) endSide |= 16;
		if ( m_goalPosition.z + 36 < lo.z ) endSide |= 32;
		if ( startSide & endSide )
			return;
	}
	SetLookAt( "Breakable", &center, PRIORITY_HIGH, 0.2f, false, 5 );
	if ( IsUsingGrenade() )
		EquipBestWeapon( false );
	else
		PrimaryAttack();
}

void CCSBot::OnEvent( GameEventType event, CBaseEntity *entity, CBaseEntity *other )
{
	m_gameState.OnEvent( event, entity, other );
	m_chatter.OnEvent( event, entity, other );
	if ( event == EVENT_TERRORISTS_WIN )
	{
		if ( m_iTeam == CT )
			DecreaseMorale();
		else
			IncreaseMorale();
	}
	else if ( event == EVENT_CTS_WIN )
	{
		if ( m_iTeam == CT )
			IncreaseMorale();
		else
			DecreaseMorale();
	}
	if ( !IsAlive() )
		return;
	CCSBotManager *manager = TheCSBots();
	CBasePlayer *player = entity && entity->IsPlayer() ? (CBasePlayer *)entity : NULL;
	if ( event == EVENT_PLAYER_DIED )
	{
		if ( !player )
			return;
		if ( !IsAttacking() && (GetDisposition() == ENGAGE_AND_INVESTIGATE || GetDisposition() == OPPORTUNITY_FIRE) &&
			player->m_iTeam == m_iTeam && other && other->IsPlayer() && ((CBasePlayer *)other)->m_iTeam != m_iTeam &&
			IsVisible( &player->pev->origin, false ) )
		{
			m_inhibitFollowingTimer.Start( RANDOM_FLOAT( 10, 15 ) );
			if ( !IsHiding() || !IsUsingKnife() || RANDOM_FLOAT( 0, 100 ) < 50 )
			{
				PrintIfWatched( "Attacking our friend's killer!\n" );
				Attack( (CBasePlayer *)other );
				return;
			}
		}
		CBasePlayer *killer = other && other->IsPlayer() ? (CBasePlayer *)other : NULL;
		if ( ((CHalfLifeMultiplay *)g_pGameRules)->IsCareer() && !player->IsBot() && player->m_iTeam == m_iTeam )
		{
			BotStatement *statement = new BotStatement( &m_chatter, BOT_STATEMENT_SAY, 20 );
			statement->AppendPhrase( TheBotPhrases->GetPhrase( "CommanderDown" ) );
			m_chatter.AddStatement( statement, false );
		}
		if ( killer == this )
			m_lastVictimIndex = player->entindex();
		if ( player->m_iTeam == m_iTeam )
		{
			if ( killer && !killer->IsBot() && killer->m_iTeam == m_iTeam && killer != this )
				m_chatter.KilledFriend();
			if ( IsHunting() )
			{
				PrintIfWatched( "Rethinking hunt due to teammate death\n" );
				Idle();
			}
			else if ( IsAttacking() && gpGlobals->time - m_lastSawEnemyTime > 0.4f )
			{
				PrintIfWatched( "Rethinking my attack due to teammate death\n" );
				IgnoreEnemies( 1 );
				m_task = TASK_CHASE_ENEMY;
				m_taskEntity = (CBaseEntity *)m_enemy;
				MoveTo( &m_lastEnemyPosition, SAFEST_ROUTE );
			}
		}
		else if ( killer && killer->m_iTeam == m_iTeam )
		{
			int enemies = GetEnemiesRemaining();
			if ( m_nearbyEnemyCount < enemies )
				enemies = m_nearbyEnemyCount;
			if ( enemies <= 1 )
			{
				m_chatter.EnemiesRemaining();
				bool visible = IsVisible( &player->pev->origin, true );
				if ( killer != this && visible )
				{
					float delay = RANDOM_FLOAT( 2, 3 );
					if ( killer->IsBot() )
					{
						if ( RANDOM_FLOAT( 0, 100 ) < 40 )
							m_chatter.Say( "NiceShot", 3, delay );
					}
					else if ( ((CHalfLifeMultiplay *)g_pGameRules)->IsCareer() )
						m_chatter.Say( "NiceShotCommander", 3, delay );
					else
						m_chatter.Say( "NiceShotSir", 3, delay );
				}
			}
		}
		return;
	}
	switch ( event )
	{
	case EVENT_WEAPON_FIRED:
	case EVENT_WEAPON_FIRED_ON_EMPTY:
	case EVENT_WEAPON_RELOADED:
		if ( player == (CBaseEntity *)m_enemy && IsUsingKnife() )
		{
			Run();
			m_hurryTimer.Start( 5 );
		}
		break;
	case EVENT_BOMB_PLANTED:
		if ( m_iTeam == CT )
			Idle();
		if ( m_isFollowing )
		{
			StopFollowing();
			Idle();
		}
		OnEvent( EVENT_BOMB_BEEP, other, NULL );
		return;
	case EVENT_BOMB_PICKED_UP:
		if ( !entity )
			return;
		if ( m_iTeam == CT && (pev->origin - entity->pev->origin).LengthSquared() < 1000 * 1000 )
			m_chatter.TheyPickedUpTheBomb();
		return;
	case EVENT_BOMB_BEEP:
		if ( !entity )
			return;
		if ( !m_gameState.IsPlantedBombLocationKnown() && (pev->origin - entity->pev->origin).LengthSquared() < 1000 * 1000 )
		{
			if ( m_iTeam == CT && m_gameState.GetPlantedBombsite() == -1 )
			{
				const CCSBotManager::Zone *zone = manager->GetZone( &entity->pev->origin );
				if ( zone )
					m_chatter.FoundPlantedBomb( zone->m_index );
			}
			m_gameState.UpdatePlantedBomb( &entity->pev->origin );
		}
		return;
	case EVENT_BOMB_DEFUSE_ABORTED:
		PrintIfWatched( "BOMB DEFUSE ABORTED\n" );
		return;
	case EVENT_BOMB_DEFUSED:
		if ( m_iTeam == CT && manager->GetBombTimeLeft() < 2 )
		{
			BotStatement *statement = new BotStatement( &m_chatter, BOT_STATEMENT_SAY, 3 );
			statement->AppendPhrase( TheBotPhrases->GetPhrase( "BarelyDefused" ) );
			m_chatter.AddStatement( statement, false );
		}
		return;
	case EVENT_TERRORISTS_WIN:
		if ( m_iTeam == TERRORIST )
			m_chatter.CelebrateWin();
		return;
	case EVENT_CTS_WIN:
		if ( m_iTeam == CT )
			m_chatter.CelebrateWin();
		return;
	default:
		break;
	}
	if ( player && player->m_iTeam == m_iTeam &&
		(unsigned int)(event - EVENT_RADIO_COVER_ME) <= EVENT_RADIO_ENEMY_DOWN - EVENT_RADIO_COVER_ME )
	{
		if ( event != EVENT_RADIO_REPORTING_IN && event != EVENT_RADIO_NEGATIVE && event != EVENT_RADIO_AFFIRMATIVE )
		{
			m_lastRadioCommand = event;
			m_radioCommandTimestamp = gpGlobals->time;
			m_radioSender = player;
			m_radioPosition = player->pev->origin;
		}
		return;
	}
	if ( !entity )
		return;
	if ( !IsRogue() && event == EVENT_HOSTAGE_CALLED_FOR_HELP && m_iTeam == CT && IsHunting() )
	{
		if ( (entity->pev->origin - pev->origin).LengthSquared() > 1000 * 1000 )
			return;
		Vector center = entity->Center();
		if ( IsVisible( &center, false ) )
		{
			m_task = TASK_COLLECT_HOSTAGES;
			m_taskEntity = NULL;
			Run();
			m_avoidEntity = entity;
			MoveTo( &entity->pev->origin, m_hostageEscortCount ? FASTEST_ROUTE : SAFEST_ROUTE );
			PrintIfWatched( "I'm fetching a hostage that called out to me\n" );
			return;
		}
	}
	float range;
	PriorityType priority;
	bool hostile;
	if ( !player || !IsEnemy( player ) || !IsGameEventAudible( event, entity, other, &range, &priority, &hostile ) )
		return;
	if ( event == EVENT_HOSTAGE_USED )
	{
		if ( m_iTeam != TERRORIST || (player->pev->origin - pev->origin).LengthSquared() > range * range )
			return;
		m_chatter.HostagesBeingTaken();
		if ( !m_gameState.GetNearestVisibleFreeHostage() && m_task != TASK_GUARD_HOSTAGE_RESCUE_ZONE && GuardRandomZone( 500 ) )
		{
			m_task = TASK_GUARD_HOSTAGE_RESCUE_ZONE;
			m_taskEntity = NULL;
			SetDisposition( OPPORTUNITY_FIRE );
			PrintIfWatched( "Trying to beat them to an escape zone!\n" );
		}
	}
	float distance = (pev->origin - player->pev->origin).Length();
	if ( range <= distance )
		return;
	bool watched = IsLocalPlayerWatchingMe();
	if ( (watched && cv_bot_debug.value == 3) || cv_bot_debug.value == 4 )
	{
		const char *priorityName = priority == PRIORITY_HIGH ? "HIGH" : (priority == PRIORITY_MEDIUM ? "MEDIUM" : "LOW");
		PrintIfWatched( "Heard noise (%sfrom %s, pri %s, time %3.1f)\n", event == EVENT_WEAPON_FIRED ? "Weapon fire " : "",
			STRING( player->pev->netname ), priorityName, gpGlobals->time );
	}
	if ( event == EVENT_PLAYER_FOOTSTEP && IsUsingSniperRifle() && distance < 300 )
		EquipPistol();
	if ( m_noiseTimestamp <= 0 || gpGlobals->time - m_noiseTimestamp >= 3 ||
		(m_noisePriority <= priority && distance < (pev->origin - m_noisePosition).Length()) )
	{
		CNavArea *area = TheNavAreaGrid.GetNavArea( &player->pev->origin );
		if ( !area )
			area = TheNavAreaGrid.GetNearestNavArea( &player->pev->origin );
		if ( area )
		{
			m_noiseArea = area;
			m_noisePriority = priority;
			float error = distance * 400 / 2000;
			m_noisePosition.x = player->pev->origin.x + RANDOM_FLOAT( -error, error );
			m_noisePosition.y = player->pev->origin.y + RANDOM_FLOAT( -error, error );
			m_noiseArea->GetClosestPointOnArea( &m_noisePosition, &m_noisePosition );
			m_noiseTravelDistanceChecked = false;
			m_noiseTimestamp = gpGlobals->time;
		}
		else
			PrintIfWatched( "  *** Noise occurred off the nav mesh - ignoring!\n" );
	}
}
