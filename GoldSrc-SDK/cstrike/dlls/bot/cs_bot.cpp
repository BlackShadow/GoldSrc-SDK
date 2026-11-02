#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"
#include "hostage.h"

extern int gmsgBotProgress;

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CCSBot::CCSBot() : m_gameState( this ), m_chatter( this )
{
}

bool CCSBot::Initialize( const BotProfile* profile )
{
	CBot::Initialize( profile );
	m_diedLastRound = false;
	m_morale = 1;
	m_navigationState = 0;
	m_currentNode = NULL;
	m_combatRange = RANDOM_FLOAT( 325.0f, 425.0f );
	m_name[0] = '\0';
	m_safeTime = 5.0f * m_profile->GetAggression() + 15.0f;
	ResetValues();
	StartNormalProcess();
	return true;
}

void CCSBot::ResetValues()
{
	m_chatter.Reset();
	m_gameState.Reset();
	m_nearbyPlayer = NULL;
	m_nearbyPlayerTimestamp = 0.0f;
	m_inhibitFollowingTimer.Invalidate();
	m_stuck = false;
	m_stuckTimestamp = 0.0f;
	m_nextWiggleTime = 0.0f;
	m_nextJumpTime = 0.0f;
	m_pathLength = 0;
	m_pathIndex = 0;
	m_areaEnteredTimestamp = 0.0f;
	m_area = NULL;
	m_lastKnownArea = NULL;
	m_friendCheckTimer.Invalidate();
	m_isFriendInTheWay = false;
	m_friendAvoidanceFlag = false;
	m_disposition = ENGAGE_AND_INVESTIGATE;
	m_enemy = NULL;
	m_isWaitingToThrowGrenade = false;
	m_wasSafe = true;
	m_nearbyEnemyCount = 0;
	m_enemyPlace = UNDEFINED_PLACE;
	m_nearbyFriendCount = 0;
	m_closestFriend = NULL;
	m_closestHumanFriend = NULL;
	for ( int i = 0; i < BOT_MAX_CLIENTS; ++i )
	{
		m_playerVisibility[i].timestamp = 0.0f;
		m_playerVisibility[i].isEnemy = false;
	}
	m_enemyVisible = false;
	m_visibleEnemyParts = 0;
	m_lastSawEnemyTime = 0.0f;
	m_firstNearbyEnemyTime = 0.0f;
	m_enemyChangeTimestamp = 0.0f;
	m_enemyDead = true;
	m_attacker = NULL;
	m_attackerTimestamp = 0.0f;
	m_enemyDeathTimestamp = 0.0f;
	m_lastVictimIndex = 0;
	m_isAimingAtEnemy = false;
	m_nextFireTime = 0.0f;
	m_lastEquipTime.Invalidate();
	m_isFollowing = false;
	m_leader = NULL;
	m_followStartTime = 0.0f;
	m_allowAutoFollowTime = 0.0f;
	m_reactionWriteIndex = 0;
	m_reactionCount = 0;
	m_recognizedIndex = 0;
	m_visibleBomber = NULL;
	m_lastLookAroundTimestamp = 0.0f;
	m_inhibitLookAroundTimestamp = 0.0f;
	m_desiredPitch = 0.0f;
	m_pitchAngularVelocity = 0.0f;
	m_desiredYaw = 0.0f;
	m_yawAngularVelocity = 0.0f;
	m_nextAimOffsetTime = 0.0f;
	m_aimAccuracyTimestamp = 0.0f;
	m_lookAtState = 0;
	m_spotEncounter = NULL;
	m_spotEncounterTimestamp = 0.0f;
	m_peripheralVisionTimestamp = 0.0f;
	m_velocitySampleIndex = 0;
	m_velocitySampleCount = 0;
	m_lastPosition = pev ? pev->origin : Vector( 0.0f, 0.0f, 0.0f );
	m_lastRadioCommand = EVENT_INVALID;
	m_radioCommandTimestamp = 0.0f;
	m_lastSentRadioTimestamp = 0.0f;
	m_radioSender = NULL;
	m_voiceEndTime = 0.0f;
	m_hostageEscortCount = 0;
	m_hostageEscortTimestamp = 0.0f;
	m_noisePosition = Vector( 0.0f, 0.0f, 0.0f );
	m_noiseTimestamp = 0.0f;
	m_noiseCheckTimestamp = 0.0f;
	m_noiseTravelDistanceChecked = false;
	m_stateTimestamp = 0.0f;
	m_task = TASK_SEEK_AND_DESTROY;
	m_taskEntity = NULL;
	m_approachPointCount = 0;
	m_lastOrigin = Vector( 0.0f, 0.0f, 0.0f );
	m_hidingSpotCheckCount = 0;
	m_isJumpCrouching = false;
	StandUp();
	Run();
	m_hurryTimer.Invalidate();
	m_ladder = NULL;
	m_repathTimer.Invalidate();
	m_huntState.m_huntArea = NULL;
	if ( m_diedLastRound )
		DecreaseMorale();
	m_diedLastRound = false;
	m_isRogue = false;
	m_surpriseDelay = 0.0f;
	m_lastPanicTime = 0.0f;
	m_avoidEntity = NULL;
	m_nearbyPlayer = NULL;
	m_enemy = NULL;
	StopAttacking();
	Idle();
}

void BotState::OnEnter( CCSBot* )
{
}

void BotState::OnUpdate( CCSBot* )
{
}

void BotState::OnExit( CCSBot* )
{
}

void CCSBot::IncreaseMorale()
{
	if ( m_morale <= 2 )
		++m_morale;
}

void CCSBot::DecreaseMorale()
{
	if ( m_morale >= -2 )
		--m_morale;
}

void CCSBot::SetDisposition( DispositionType disposition )
{
	m_disposition = disposition;
	if ( disposition != IGNORE_ENEMIES )
		m_ignoreEnemiesTimer.Invalidate();
}

CCSBot::DispositionType CCSBot::GetDisposition() const
{
	return m_ignoreEnemiesTimer.IsElapsed() ? m_disposition : IGNORE_ENEMIES;
}

void CCSBot::IgnoreEnemies( float duration )
{
	m_ignoreEnemiesTimer.Start( duration );
}

bool CCSBot::IsRogue() const
{
	if ( cv_bot_allow_rogues.value <= 0.0f )
		return false;
	if ( m_rogueTimer.IsElapsed() )
	{
		m_rogueTimer.Start( RANDOM_FLOAT( 10.0f, 30.0f ) );
		float chance = (1.0f - m_profile->GetTeamwork()) * 100.0f;
		m_isRogue = chance > RANDOM_FLOAT( 0.0f, 100.0f );
	}
	return m_isRogue;
}

bool CCSBot::IsSafe() const
{
	return m_safeTime > TheCSBots()->GetElapsedRoundTime();
}

bool CCSBot::IsWellPastSafe() const
{
	return TheCSBots()->GetElapsedRoundTime() > 1.25f * m_safeTime;
}

bool CCSBot::IsEndOfSafeTime() const
{
	return m_wasSafe && m_safeTime <= TheCSBots()->GetElapsedRoundTime();
}

float CCSBot::GetSafeTimeRemaining() const
{
	return m_safeTime - TheCSBots()->GetElapsedRoundTime();
}

void CCSBot::AdjustSafeTime()
{
	float elapsed = TheCSBots()->GetElapsedRoundTime();
	if ( m_safeTime > elapsed )
		m_safeTime = elapsed - 2.0f;
}

bool CCSBot::HasNotSeenEnemyForLongTime() const
{
	return gpGlobals->time - m_lastSawEnemyTime > 30.0f;
}

Place CCSBot::GetPlace() const
{
	return m_lastKnownArea ? m_lastKnownArea->GetPlace() : UNDEFINED_PLACE;
}

CBasePlayer* CCSBot::GetAttacker() const
{
	return m_attacker && m_attacker->IsAlive() ? m_attacker : NULL;
}

bool CCSBot::IsOutnumbered() const
{
	int friends = GetFriendsRemaining();
	if ( m_nearbyFriendCount <= friends )
		friends = m_nearbyFriendCount;
	int enemies = GetEnemiesRemaining();
	if ( m_nearbyEnemyCount <= enemies )
		enemies = m_nearbyEnemyCount;
	return enemies - 1 > friends;
}

int CCSBot::OutnumberedCount() const
{
	if ( !IsOutnumbered() )
		return 0;
	int enemies = GetEnemiesRemaining();
	if ( m_nearbyEnemyCount <= enemies )
		enemies = m_nearbyEnemyCount;
	int friends = GetFriendsRemaining();
	if ( m_nearbyFriendCount <= friends )
		friends = m_nearbyFriendCount;
	return enemies - 1 - friends;
}

bool CCSBot::IsDoingScenario() const
{
	return cv_bot_defer_to_human.value <= 0.0f || UTIL_HumansOnTeam( m_iTeam, true ) == 0;
}

bool CCSBot::NoticeLooseBomb() const
{
	return TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB && TheCSBots()->GetLooseBomb() != NULL;
}

bool CCSBot::CanSeeLooseBomb() const
{
	if ( TheCSBots()->GetScenario() != CCSBotManager::SCENARIO_DEFUSE_BOMB )
		return false;
	CBaseEntity* bomb = TheCSBots()->GetLooseBomb();
	return bomb && IsVisible( &bomb->pev->origin, true );
}

bool CCSBot::CanSeePlantedBomb() const
{
	if ( TheCSBots()->GetScenario() != CCSBotManager::SCENARIO_DEFUSE_BOMB || m_gameState.GetBombState() != CSGameState::BOMB_PLANTED )
		return false;
	const Vector* position = m_gameState.GetBombPosition();
	return position && IsVisible( position, true );
}

float CCSBot::GetHidingSpotCheckTimestamp( HidingSpot* spot ) const
{
	for ( int i = 0; i < m_hidingSpotCheckCount; ++i )
	{
		if ( m_hidingSpotCheck[i].spot->GetID() == spot->GetID() )
			return m_hidingSpotCheck[i].timestamp;
	}
	return -999999.9f;
}

void CCSBot::SetHidingSpotCheckTimestamp( HidingSpot* spot )
{
	int oldest = 0;
	float oldestTime = gpGlobals->time + 1.0f;
	for ( int i = 0; i < m_hidingSpotCheckCount; ++i )
	{
		if ( m_hidingSpotCheck[i].spot->GetID() == spot->GetID() )
		{
			m_hidingSpotCheck[i].timestamp = gpGlobals->time;
			return;
		}
		if ( oldestTime > m_hidingSpotCheck[i].timestamp )
		{
			oldestTime = m_hidingSpotCheck[i].timestamp;
			oldest = i;
		}
	}
	int index = oldest;
	if ( m_hidingSpotCheckCount < 64 )
		index = m_hidingSpotCheckCount++;
	m_hidingSpotCheck[index].spot = spot;
	m_hidingSpotCheck[index].timestamp = gpGlobals->time;
}

void CCSBot::SetEnemy( CBasePlayer* enemy )
{
	if ( static_cast<CBaseEntity*>( m_enemy ) != enemy )
	{
		m_enemy = enemy;
		m_enemyChangeTimestamp = gpGlobals->time;
	}
}

bool CCSBot::IsHurrying() const
{
	if ( !m_inhibitFollowingTimer.IsElapsed() )
		return true;
	if ( TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB )
		return TheCSBots()->IsBombPlanted();
	return TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_RESCUE_HOSTAGES && m_iTeam == TERRORIST && m_gameState.AreAllHostagesBeingRescued();
}

void CCSBot::Walk()
{
	if ( !m_hurryTimer.IsElapsed() )
		Run();
	else
		CBot::Walk();
}

bool CCSBot::Jump( bool mustJump )
{
	if ( m_lastKnownArea && (m_lastKnownArea->GetAttributes() & NAV_CROUCH) && !(m_lastKnownArea->GetAttributes() & NAV_JUMP) )
		return false;
	return CBot::Jump( mustJump );
}

void CCSBot::GetOffLadder()
{
	if ( m_ladder )
	{
		Jump( true );
		m_pathLength = 0;
		m_ladder = NULL;
	}
}

void CCSBot::Disconnect()
{
	EndVoiceFeedback( true );
	if ( m_process != 0 )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgBotProgress );
		WRITE_BYTE( 2 );
		MESSAGE_END();
	}
}

void CCSBot::RoundRespawn()
{
	CBasePlayer::RoundRespawn();
	EndVoiceFeedback( true );
}

void CCSBot::BotDeathThink()
{
}

void CCSBot::SpawnBot()
{
	TheCSBots()->ValidateMapData();
	ResetValues();
	strcpy( m_name, STRING( pev->netname ) );
	SetState( &m_buyState );
	SetTouch( &CCSBot::BotTouch );
	if ( TheNavAreaList.empty() && !CCSBotManager::IsLearningMap() )
	{
		CCSBotManager::m_isLearningMap = true;
		StartLearnProcess();
	}
}

void CCSBot::Upkeep()
{
	if ( CCSBotManager::IsLearningMap() || !IsAlive() )
		return;
	if ( m_isRapidFire )
		TogglePrimaryAttack();
	if ( !m_isAimingAtEnemy )
	{
		if ( m_lookAtClearIfClose && (m_lookAtPosition - pev->origin).LengthSquared() < 10000.0f )
		{
			m_lookAtState = 0;
			m_desiredPitch = 0.0f;
			m_desiredYaw = m_lookAheadYaw;
		}
		else if ( m_lookAtState == 1 )
		{
			UpdateLookAt();
			if ( IsLookingAtPosition( &m_lookAtPosition, m_lookAtTolerance ) )
			{
				m_lookAtState = 2;
				m_lookAtAcquiredTimestamp = gpGlobals->time;
			}
		}
		else if ( m_lookAtState == 2 )
		{
			UpdateLookAt();
			if ( m_lookAtDuration >= 0.0f && gpGlobals->time - m_lookAtAcquiredTimestamp > m_lookAtDuration )
				m_lookAtState = 0;
		}
		else if ( m_lookAtState == 0 )
		{
			m_desiredPitch = 0.0f;
			m_desiredYaw = m_lookAheadYaw;
		}
		float amplitude = IsUsingSniperRifle() && IsUsingScope() ? 0.5f : 2.0f;
		m_desiredYaw += BotCOS( 33.0f * gpGlobals->time ) * amplitude;
		m_desiredPitch += BotSIN( 13.0f * gpGlobals->time ) * amplitude;
		UpdateLookAngles();
		return;
	}
	UpdateAimOffset();
	if ( static_cast<int>( m_enemy ) == 0 )
	{
		UpdateLookAngles();
		return;
	}
	float halfHeight = IsCrouching() ? 18.0f : 36.0f;
	if ( !m_enemyVisible )
		m_aimSpot = m_lastEnemyPosition;
	else
	{
		if ( m_profile->GetSkill() > 0.5f )
		{
			Vector lead = (m_enemy->pev->velocity - pev->velocity) * (3.0f * g_flBotCommandInterval);
			m_aimSpot = m_enemy->pev->origin + lead;
		}
		else
			m_aimSpot = m_enemy->pev->origin;
		CBasePlayerWeapon* weapon = GetActiveWeapon();
		bool preferChest = IsUsingAWP() || IsUsingShotgun() || IsUsingMachinegun() || m_profile->GetSkill() < 0.8f ||
			(weapon && weapon->m_flAccuracy > 0.4f && !IsUsingPistol() && !IsUsingSniperRifle());
		if ( preferChest && IsEnemyPartVisible( CHEST ) )
			m_aimSpot.z -= halfHeight * 0.25f;
		else if ( !preferChest && IsEnemyPartVisible( HEAD ) )
		{
		}
		else if ( IsEnemyPartVisible( CHEST ) )
			m_aimSpot.z -= halfHeight * 0.5f;
		else if ( IsEnemyPartVisible( HEAD ) )
		{
		}
		else if ( IsEnemyPartVisible( LEFT_SIDE ) )
		{
			Vector2D direction = (m_enemy->pev->origin - pev->origin).Make2D();
			direction.NormalizeInPlace();
			m_aimSpot.x -= direction.y * 16.0f;
			m_aimSpot.y += direction.x * 16.0f;
			m_aimSpot.z -= halfHeight * 0.5f;
		}
		else if ( IsEnemyPartVisible( RIGHT_SIDE ) )
		{
			Vector2D direction = (m_enemy->pev->origin - pev->origin).Make2D();
			direction.NormalizeInPlace();
			m_aimSpot.x += direction.y * 16.0f;
			m_aimSpot.y -= direction.x * 16.0f;
			m_aimSpot.z -= halfHeight * 0.5f;
		}
		else
			m_aimSpot.z -= halfHeight + halfHeight;
	}
	m_aimSpot = m_aimSpot + m_aimOffset;
	Vector angles = UTIL_VecToAngles( m_aimSpot - pev->origin );
	m_desiredPitch = 360.0f - angles.x;
	m_desiredYaw = angles.y;
	UpdateLookAngles();
}

void CCSBot::ResetStuckMonitor()
{
	if ( m_stuck && IsLocalPlayerWatchingMe() && cv_bot_debug.value > 0.0f )
		EMIT_SOUND_DYN( pev->pContainingEntity, CHAN_VOICE, "buttons/bell1.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM );
	m_stuck = false;
	m_stuckTimestamp = 0.0f;
	m_nextJumpTime = 0.0f;
	m_velocitySampleIndex = 0;
	m_velocitySampleCount = 0;
	m_areaEnteredTimestamp = gpGlobals->time;
}

void CCSBot::StuckCheck()
{
	if ( m_stuck )
	{
		if ( (pev->origin - m_stuckPosition).LengthSquared() > 75.0f * 75.0f )
		{
			ResetStuckMonitor();
			PrintIfWatched( "UN-STUCK\n" );
		}
	}
	else
	{
		Vector displacement = pev->origin - m_lastPosition;
		if ( IsJumping() )
			displacement.z = 0.0f;
		m_velocitySamples[m_velocitySampleIndex++] = displacement.Length() / g_flBotFullThinkInterval;
		if ( m_velocitySampleIndex == 5 )
			m_velocitySampleIndex = 0;
		if ( m_velocitySampleCount <= 4 )
			++m_velocitySampleCount;
		else
		{
			float speed = 0.0f;
			for ( int i = 0; i < m_velocitySampleCount; ++i )
				speed += m_velocitySamples[i];
			speed /= 5.0f;
			float minimumSpeed = m_ladder ? 10.0f : 20.0f;
			if ( speed < minimumSpeed )
			{
				m_stuckTimestamp = gpGlobals->time;
				m_stuckPosition = pev->origin;
				float now = gpGlobals->time;
				m_nextJumpTime = RANDOM_FLOAT( 0.0f, 0.5f ) + now;
				PrintIfWatched( "STUCK\n" );
				if ( IsLocalPlayerWatchingMe() && cv_bot_debug.value > 0.0f )
					EMIT_SOUND_DYN( pev->pContainingEntity, CHAN_VOICE, "buttons/button11.wav", 1.0f, ATTN_NORM, 0, PITCH_NORM );
				m_stuck = true;
			}
		}
	}
	m_lastPosition = pev->origin;
}

void CCSBot::Wiggle()
{
	if ( IsCrouching() )
	{
		ResetStuckMonitor();
		return;
	}
	if ( gpGlobals->time >= m_nextWiggleTime )
	{
		m_wiggleDirection = RANDOM_LONG( 0, 3 );
		float now = gpGlobals->time;
		m_nextWiggleTime = RANDOM_FLOAT( 0.5f, 1.5f ) + now;
	}
	switch ( m_wiggleDirection )
	{
	case 0: MoveForward(); break;
	case 1: StrafeRight(); break;
	case 2: MoveBackward(); break;
	case 3: StrafeLeft(); break;
	default: break;
	}
	if ( gpGlobals->time >= m_nextJumpTime && Jump( false ) )
	{
		float now = gpGlobals->time;
		m_nextJumpTime = RANDOM_FLOAT( 1.0f, 2.0f ) + now;
	}
}

bool CCSBot::IsEnemyPartVisible( VisiblePartType part ) const
{
	return m_enemyVisible && (m_visibleEnemyParts & part) != 0;
}

CBasePlayer* CCSBot::FindNearbyPlayer()
{
	CBaseEntity* entity = NULL;
	Vector center = pev->origin;
	while ( (entity = UTIL_FindEntityInSphere( entity, center, 800.0f )) != NULL )
	{
		if ( entity->IsPlayer() && !(entity->pev->flags & FL_FAKECLIENT) )
			return static_cast<CBasePlayer*>( entity );
	}
	return NULL;
}

CBasePlayer* CCSBot::GetImportantEnemy( bool checkVisibility ) const
{
	CBasePlayer* closest = NULL;
	float closestDistance = 1000000000.0f;
	CCSBotManager* manager = TheCSBots();
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBaseEntity* entity = UTIL_PlayerByIndex( i );
		if ( entity == NULL || entity->pev == NULL || FNullEnt( entity->pev ) || STRING( entity->pev->netname )[0] == '\0' )
			continue;
		if ( !entity->IsPlayer() || !entity->IsAlive() )
			continue;
		CBasePlayer* player = static_cast<CBasePlayer*>( entity );
		if ( player->m_iTeam == m_iTeam || !manager->IsImportantPlayer( player ) )
			continue;
		float distance = (pev->origin - player->pev->origin).LengthSquared();
		if ( closestDistance > distance && (!checkVisibility || IsVisible( player, true, NULL )) )
		{
			closestDistance = distance;
			closest = player;
		}
	}
	return closest;
}

bool CCSBot::GuardRandomZone( float range )
{
	CCSBotManager* manager = TheCSBots();
	if ( manager->GetZoneCount() == 0 )
		return false;
	const CCSBotManager::Zone* zone = manager->GetZone( RANDOM_LONG( 0, manager->GetZoneCount() - 1 ) );
	if ( zone == NULL )
		return false;
	CNavArea* area = manager->GetRandomAreaInZone( zone );
	if ( area == NULL )
		return false;
	Hide( area, -1.0f, range, false );
	return true;
}

bool CCSBot::TryToRetreat()
{
	const Vector* spot = FindNearbyRetreatSpot( this, 1000.0f );
	if ( spot == NULL )
		return false;
	IgnoreEnemies( 10.0f );
	float duration = RANDOM_FLOAT( 3.0f, 15.0f );
	StandUp();
	Run();
	Hide( spot, duration, false );
	PrintIfWatched( "Retreating to a safe spot!\n" );
	return true;
}

void CCSBot::UpdateHostageEscortCount()
{
	if ( m_hostageEscortCount == 0 || gpGlobals->time - m_hostageEscortTimestamp < 1.0f )
		return;
	m_hostageEscortTimestamp = gpGlobals->time;
	m_hostageEscortCount = 0;
	CBaseEntity* entity = NULL;
	while ( (entity = UTIL_FindEntityByClassname( entity, "hostage_entity" )) != NULL )
	{
		if ( FNullEnt( entity->pev ) )
			return;
		CHostage* hostage = static_cast<CHostage*>( entity );
		if ( hostage->pev->takedamage == DAMAGE_YES && hostage->IsFollowing( this ) )
			++m_hostageEscortCount;
	}
}

float CCSBot::GetRangeToFarthestEscortedHostage() const
{
	float farthest = -1.0f;
	for ( int i = 0; i < g_pHostages->m_hostageCount; ++i )
	{
		CHostage* hostage = g_pHostages->m_hostage[i];
		if ( hostage->pev->deadflag != DEAD_DEAD && hostage->pev->takedamage == DAMAGE_YES && hostage->IsFollowing( this ) )
		{
			float distance = (hostage->Center() - pev->origin).Length();
			if ( distance > farthest )
				farthest = distance;
		}
	}
	return farthest;
}

class CollectRetreatSpotsFunctor
{
public:
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	CollectRetreatSpotsFunctor( CCSBot* bot, float range ) : m_bot( bot ), m_range( range ), m_count( 0 )
	{
	}

	bool operator()( CNavArea* area )
	{
		const HidingSpotList* spots = area->GetHidingSpotList();
		for ( HidingSpotList::const_iterator spot = spots->begin(); spot != spots->end() && m_count < 256; ++spot )
		{
			const Vector* position = (*spot)->GetPosition();
			if ( m_range > 0.0f && (*position - m_bot->pev->origin).LengthSquared() > m_range * m_range )
				continue;
			if ( IsSpotOccupied( m_bot, position ) )
				continue;
			Vector eye = *position + Vector( 0.0f, 0.0f, 36.0f );
			int enemyTeam = m_bot->m_iTeam == TERRORIST ? CT : TERRORIST;
			if ( UTIL_IsVisibleToTeam( eye, enemyTeam, -1.0f ) )
				continue;
			CBasePlayer* closest = UTIL_GetClosestPlayer( position, NULL );
			if ( closest && closest->m_iTeam != m_bot->m_iTeam )
				continue;
			m_spot[m_count++] = position;
		}
		return m_count != 256;
	}

	const Vector* GetRandomSpot() const
	{
		return m_count ? m_spot[RANDOM_LONG( 0, m_count - 1 )] : NULL;
	}

private:
	CCSBot* m_bot;
	float m_range;
	const Vector* m_spot[256];
	int m_count;
};

const Vector* FindNearbyRetreatSpot( CCSBot* bot, float maxRange )
{
	CNavArea* area = bot->GetLastKnownArea();
	if ( area == NULL )
		return NULL;
	CollectRetreatSpotsFunctor collect( bot, maxRange );
	SearchSurroundingAreas( area, &bot->pev->origin, collect, maxRange );
	return collect.GetRandomSpot();
}

int GetBotFollowCount( CBasePlayer* leader )
{
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBaseEntity* entity = UTIL_PlayerByIndex( i );
		if ( entity == NULL || entity->pev == NULL || FNullEnt( entity->pev ) || STRING( entity->pev->netname )[0] == '\0' )
			continue;
		CBasePlayer* player = static_cast<CBasePlayer*>( entity );
		if ( !player->IsBot() || !player->IsAlive() )
			continue;
		CCSBot* bot = static_cast<CCSBot*>( player );
		if ( static_cast<CBaseEntity*>( bot->m_leader ) == leader )
			++count;
	}
	return count;
}
