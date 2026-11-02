#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "cs_bot.h"
#include "bot_profile.h"
#include <algorithm>

void CCSBot::UpdateLookAt( void )
{
	Vector direction = m_lookAtPosition - (pev->origin + pev->view_ofs);
	Vector angles = UTIL_VecToAngles( direction );
	m_desiredYaw = angles.y;
	m_desiredPitch = 360.0f - angles.x;
}

void CCSBot::SetLookAt( const char *description, const Vector *position, PriorityType priority,
	float duration, bool clearIfClose, float tolerance )
{
	if ( position == NULL )
		return;
	if ( m_lookAtState != 0 && m_lookAtPriority > priority )
		return;
	if ( m_lookAtState != 0 &&
		abs( static_cast<int>(position->x - m_lookAtPosition.x) ) < 10 &&
		abs( static_cast<int>(position->y - m_lookAtPosition.y) ) < 10 &&
		abs( static_cast<int>(position->z - m_lookAtPosition.z) ) < 10 )
	{
		m_lookAtDuration = duration;
		if ( m_lookAtPriority < priority )
			m_lookAtPriority = priority;
	}
	else
	{
		m_lookAtPosition = *position;
		m_lookAtDuration = duration;
		m_lookAtState = 1;
		m_lookAtPriority = priority;
	}
	m_lookAtClearIfClose = clearIfClose;
	m_lookAtTolerance = tolerance;
	m_lookAtDescription = description;
}

void CCSBot::InhibitLookAround( float duration )
{
	m_inhibitLookAroundTimestamp = gpGlobals->time + duration;
}

void CCSBot::UpdatePeripheralVision( void )
{
	if ( gpGlobals->time - m_peripheralVisionTimestamp < 0.29f )
		return;
	m_peripheralVisionTimestamp = gpGlobals->time;
	if ( m_spotEncounter )
	{
		for ( SpotOrderList::const_iterator it = m_spotEncounter->spotList.begin(); it != m_spotEncounter->spotList.end(); ++it )
		{
			Vector position = *it->spot->GetPosition();
			position.z += 36.0f;
			if ( IsVisible( &position, CHECK_FOV ) )
				SetHidingSpotCheckTimestamp( it->spot );
		}
	}
}

bool CCSBot::IsVisible( const Vector *position, bool testFOV ) const
{
	if ( m_blindUntilTime > gpGlobals->time ||
		(testFOV && !const_cast<CCSBot *>(this)->FInViewCone( const_cast<Vector *>(position) )) )
		return false;
	m_eyePosition = pev->origin + pev->view_ofs;
	if ( TheBots->IsLineBlockedBySmoke( &m_eyePosition, position ) )
		return false;
	TraceResult trace;
	UTIL_TraceLine( m_eyePosition, *position, ignore_monsters, ignore_glass, ENT( pev ), &trace );
	return trace.flFraction == 1.0f;
}

bool CCSBot::IsVisible( CBasePlayer *player, bool testFOV, unsigned char *visibleParts ) const
{
	unsigned char parts = IsVisible( &player->pev->origin, testFOV ) ? CHEST : NONE;
	Vector position = player->pev->origin;
	position.z += 25.0f;
	if ( IsVisible( &position, testFOV ) )
		parts |= HEAD;
	position.z = player->pev->origin.z + ((player->pev->flags & FL_DUCKING) ? -14.0f : -34.0f);
	if ( IsVisible( &position, testFOV ) )
		parts |= FEET;
	Vector2D direction = (player->pev->origin - pev->origin).Make2D();
	direction.NormalizeInPlace();
	Vector2D side( -direction.y * 13.0f, direction.x * 13.0f );
	position = player->pev->origin + Vector( side.x, side.y, 0.0f );
	if ( IsVisible( &position, testFOV ) )
		parts |= LEFT_SIDE;
	position.x = player->pev->origin.x - side.x;
	position.y = player->pev->origin.y - side.y;
	if ( IsVisible( &position, testFOV ) )
		parts |= RIGHT_SIDE;
	if ( visibleParts )
		*visibleParts = parts;
	return parts != NONE;
}

void CCSBot::UpdateReactionQueue( void )
{
	if ( cv_bot_zombie.value > 0.0f )
		return;
	CBasePlayer *enemy = FindMostDangerousThreat();
	int index = m_reactionWriteIndex;
	Reaction &reaction = m_reactionQueue[index];
	reaction.enemy = enemy;
	if ( enemy )
	{
		CBasePlayerWeapon *weapon = static_cast<CBasePlayerWeapon *>(enemy->m_pActiveItem);
		reaction.reloading = weapon && weapon->m_fInReload != 0;
		reaction.shield = enemy->HasShield() && enemy->m_bShieldDrawn;
	}
	else
	{
		reaction.reloading = false;
		reaction.shield = false;
	}
	if ( ++m_reactionWriteIndex > 19 )
		m_reactionWriteIndex = 0;
	if ( m_reactionCount <= 19 )
		++m_reactionCount;
	float reactionTime = std::min( GetProfile()->GetReactionTime(), 20.0f * g_flBotFullThinkInterval - 0.01f );
	int recognized = index - static_cast<int>(reactionTime / g_flBotFullThinkInterval + 0.5f);
	m_recognizedIndex = static_cast<unsigned char>(recognized + (recognized < 0 ? 20 : 0));
}

CBasePlayer *CCSBot::GetRecognizedEnemy( void )
{
	if ( m_recognizedIndex >= m_reactionCount )
		return NULL;
	return static_cast<CBasePlayer *>(static_cast<CBaseEntity *>(m_reactionQueue[m_recognizedIndex].enemy));
}

bool CCSBot::IsRecognizedEnemyReloading( void )
{
	return m_recognizedIndex < m_reactionCount && m_reactionQueue[m_recognizedIndex].reloading;
}

bool CCSBot::IsRecognizedEnemyProtectedByShield( void )
{
	return m_recognizedIndex < m_reactionCount && m_reactionQueue[m_recognizedIndex].shield;
}

float CCSBot::GetRangeToNearestRecognizedEnemy( void )
{
	CBasePlayer *enemy = GetRecognizedEnemy();
	return enemy ? (pev->origin - enemy->pev->origin).Length() : 100000000.0f;
}

void CCSBot::Blind( float duration, float holdTime, float fadeTime, int alpha )
{
	CBasePlayer::Blind( duration, holdTime, fadeTime, alpha );
	PrintIfWatched( "I'm blind!\n" );
	if ( RANDOM_FLOAT( 0.0f, 100.0f ) < 33.3f )
	{
		BotStatement *statement = new BotStatement( &m_chatter, BOT_STATEMENT_SAY, 1.0f );
		statement->AppendPhrase( TheBotPhrases->GetPhrase( "Blinded" ) );
		m_chatter.AddStatement( statement );
	}
	m_blindMoveDirection = RANDOM_LONG( 1, 5 );
	m_blindFire = RANDOM_FLOAT( 0.0f, 100.0f ) < 10.0f;
	AdjustSafeTime();
}

bool CCSBot::BendLineOfSight( const Vector *eye, const Vector *target, Vector *visible ) const
{
	Vector raisedTarget = *target + Vector( 0.0f, 0.0f, HalfHumanHeight );
	TraceResult trace;
	UTIL_TraceLine( *eye, raisedTarget, ignore_monsters, ENT( pev ), &trace );
	if ( trace.flFraction == 1.0f && !trace.fStartSolid )
	{
		*visible = *target;
		return true;
	}
	Vector delta = *target - *eye;
	float yaw = UTIL_VecToYaw( delta );
	float range = delta.Length2D();
	for ( float angle = 10.0f; angle <= 130.0f; angle += 10.0f )
	{
		for ( int side = 0; side < 2; ++side )
		{
			float testYaw = side ? yaw + angle : yaw - angle;
			Vector point( eye->x + range * BotCOS( testYaw ), eye->y + range * BotSIN( testYaw ), target->z );
			Vector raisedPoint = point + Vector( 0.0f, 0.0f, HalfHumanHeight );
			UTIL_TraceLine( *eye, raisedPoint, ignore_monsters, ENT( pev ), &trace );
			if ( trace.fStartSolid )
				continue;
			Vector direction = point - *eye;
			float length = direction.NormalizeInPlace();
			float limit = length * trace.flFraction;
			for ( float distance = 50.0f; distance <= limit; distance += 50.0f )
			{
				Vector position = *eye + direction * distance;
				UTIL_TraceLine( position, raisedTarget, ignore_monsters, ENT( pev ), &trace );
				if ( trace.flFraction == 1.0f && !trace.fStartSolid )
				{
					if ( !GetGroundHeight( &position, &position.z, NULL ) )
						position.z = target->z;
					*visible = position;
					return true;
				}
			}
		}
	}
	*visible = *target;
	return false;
}

void CCSBot::UpdateLookAngles( void )
{
	float interval = g_flBotCommandInterval;
	bool attacking = IsAttacking();
	float damping = attacking ? 30.0f : 25.0f;
	float stiffness = attacking ? 300.0f : 200.0f;
	float yaw = m_desiredYaw;
	float pitch = m_desiredPitch;
	if ( m_ladder )
	{
		Vector direction = m_ladder->m_top - pev->origin;
		float ladderYaw = UTIL_VecToYaw( direction );
		NavDirType facing = m_isAscendingLadder ? OppositeDirection( m_ladder->m_dir ) : m_ladder->m_dir;
		switch ( m_ladderState )
		{
		case 0:
		case 1:
			yaw = ladderYaw;
			if ( (m_goalPosition - pev->origin).LengthSquared() < 10000.0f )
				pitch = m_ladderState == 0 ? -60.0f : 60.0f;
			break;
		case 2:
			yaw = ladderYaw;
			pitch = -60.0f;
			break;
		case 3:
			yaw = ladderYaw;
			pitch = 60.0f;
			break;
		case 4:
		case 5:
		case 6:
		case 7:
		{
			static const float facingYaw[NUM_DIRECTIONS] = { 270.0f, 0.0f, 90.0f, 180.0f };
			float baseYaw = static_cast<unsigned int>(facing) < NUM_DIRECTIONS ? facingYaw[facing] : 0.0f;
			float currentYaw = pev->v_angle.y;
			while ( currentYaw < 0.0f )
				currentYaw += 360.0f;
			while ( currentYaw > 360.0f )
				currentYaw -= 360.0f;
			float error;
			if ( currentYaw < 45.0f || currentYaw > 315.0f )
				error = m_ladder->m_top.y - pev->origin.y;
			else if ( currentYaw < 135.0f )
				error = -(m_ladder->m_top.x - pev->origin.x);
			else if ( currentYaw < 225.0f )
				error = -(m_ladder->m_top.y - pev->origin.y);
			else
				error = m_ladder->m_top.x - pev->origin.x;
			yaw = baseYaw + error;
			pitch = m_ladderState == 4 || m_ladderState == 6 ? -60.0f : 60.0f;
			break;
		}
		case 8:
		case 9:
		{
			static const float facingYaw[NUM_DIRECTIONS] = { 270.0f, 0.0f, 90.0f, 180.0f };
			yaw = static_cast<unsigned int>(facing) < NUM_DIRECTIONS ? facingYaw[facing] : 0.0f;
			break;
		}
		}
	}

	float error = yaw - pev->v_angle.y;
	while ( error < -180.0f )
		error += 360.0f;
	while ( error > 180.0f )
		error -= 360.0f;
	if ( error < 1.0f && error > -1.0f )
	{
		m_yawAngularVelocity = 0.0f;
		pev->v_angle.y = yaw;
	}
	else
	{
		float acceleration = error * stiffness - damping * m_yawAngularVelocity;
		acceleration = std::max( -3000.0f, std::min( 3000.0f, acceleration ) );
		m_yawAngularVelocity += acceleration * interval;
		pev->v_angle.y += m_yawAngularVelocity * interval;
	}
	error = pitch - pev->v_angle.x;
	while ( error < -180.0f )
		error += 360.0f;
	while ( error > 180.0f )
		error -= 360.0f;
	float acceleration = error * (stiffness + stiffness) - damping * m_pitchAngularVelocity;
	acceleration = std::max( -3000.0f, std::min( 3000.0f, acceleration ) );
	m_pitchAngularVelocity += acceleration * interval;
	pev->v_angle.x += interval * m_pitchAngularVelocity;
	if ( pev->v_angle.x < -89.0f )
		pev->v_angle.x = -89.0f;
	else if ( pev->v_angle.x > 89.0f )
		pev->v_angle.x = 89.0f;
	pev->v_angle.z = 0.0f;
}

void CCSBot::UpdateLookAround( bool )
{
	if ( m_inhibitLookAroundTimestamp > gpGlobals->time )
		return;
	if ( !m_enemyVisible && CanHearNearbyEnemyGunfire( 1000.0f ) )
	{
		Vector position = m_noisePosition + Vector( 0.0f, 0.0f, HalfHumanHeight );
		SetLookAt( "Check dangerous noise", &position, PRIORITY_HIGH, 0.25f, false, 5.0f );
		float duration = RANDOM_FLOAT( 2.0f, 4.0f );
		m_inhibitLookAroundTimestamp = duration + gpGlobals->time;
		return;
	}
	if ( !(m_lookAtState != 0 && m_lookAtPriority > PRIORITY_LOW) && gpGlobals->time - m_lastSawEnemyTime < 0.25f )
	{
		m_lookAtState = 0;
		m_lookAtDescription = NULL;
		Vector position = m_lastEnemyPosition;
		if ( GetSimpleGroundHeight( &m_lastEnemyPosition, &position.z, NULL ) )
		{
			position.z += HalfHumanHeight;
			SetLookAt( "Last Enemy Position", &position, PRIORITY_MEDIUM, RANDOM_FLOAT( 2.0f, 3.0f ), true, 5.0f );
			return;
		}
	}
	if ( UpdateLookAtNoise() )
		return;
	if ( pev->velocity.LengthSquared() < 100.0f )
	{
		if ( IsUsingSniperRifle() && GetProfile()->GetSkill() > 0.4f )
		{
			if ( m_yawAngularVelocity >= 1.0f || m_yawAngularVelocity <= -1.0f ||
				m_pitchAngularVelocity >= 1.0f || m_pitchAngularVelocity <= -1.0f )
			{
				if ( m_iFOV <= 60 )
					SecondaryAttack();
			}
			else
				AdjustZoom( ComputeWeaponSightRange() );
		}
		if ( m_lastKnownArea && m_lastLookAroundTimestamp <= gpGlobals->time )
		{
			float time = gpGlobals->time;
			m_lastLookAroundTimestamp = time + (IsUsingSniperRifle() ? RANDOM_FLOAT( 5.0f, 10.0f ) : RANDOM_FLOAT( 1.0f, 2.0f ));
			if ( m_approachPointCount )
			{
				Vector position = m_approachPoint[RANDOM_LONG( 0, m_approachPointCount - 1 )];
				position.z += HalfHumanHeight;
				SetLookAt( "Approach Point (Hiding)", &position, PRIORITY_LOW, -1.0f, false, 5.0f );
			}
			else
			{
				m_lookAtState = 0;
				m_lookAtDescription = NULL;
			}
		}
		return;
	}
	if ( m_spotEncounter == NULL || IsSafe() || (m_lookAtState != 0 && m_lookAtPriority >= PRIORITY_LOW) ||
		m_spotEncounterTimestamp > gpGlobals->time )
		return;
	float time = gpGlobals->time;
	float scale = 1.0f - GetProfile()->GetSkill();
	scale *= scale;
	scale *= scale;
	m_spotEncounterTimestamp = RANDOM_FLOAT( 10.0f, 30.0f ) * scale + time;
	Vector delta = m_spotEncounter->path.to - m_spotEncounter->path.from;
	float length = delta.Length();
	float t;
	if ( abs( static_cast<int>(delta.x) ) > abs( static_cast<int>(delta.y) ) )
		t = (pev->origin.x - m_spotEncounter->path.from.x) / delta.x;
	else
		t = (pev->origin.y - m_spotEncounter->path.from.y) / delta.y;
	t = std::max( 0.0f, std::min( 1.0f, t + 50.0f / length ) );
	HidingSpot *candidate[8];
	int count = 0;
	int index = 0;
	for ( SpotOrderList::const_iterator it = m_spotEncounter->spotList.begin(); it != m_spotEncounter->spotList.end(); ++it )
	{
		float now = gpGlobals->time;
		if ( now - GetHidingSpotCheckTimestamp( it->spot ) > 10.0f )
		{
			if ( it->t > t )
				break;
			candidate[index] = it->spot;
			if ( ++index > 7 )
				index = 0;
			if ( count <= 7 )
				++count;
		}
	}
	if ( count )
	{
		HidingSpot *spot = candidate[RANDOM_LONG( 0, count - 1 )];
		Vector position = *spot->GetPosition() + Vector( 0.0f, 0.0f, HalfHumanHeight );
		SetLookAt( "Encounter Spot", &position, PRIORITY_LOW, 0.0f, true, 10.0f );
		SetHidingSpotCheckTimestamp( spot );
	}
}

CBasePlayer *CCSBot::FindMostDangerousThreat( void )
{
	struct Threat
	{
		CBasePlayer *player;
		float rangeSquared;
	};
	Threat threat[16];
	int threatCount = 0;
	float closestFriendRange = 100000000000.0f;
	float closestHumanRange = 100000000000.0f;
	m_visibleBomber = NULL;
	m_closestFriend = NULL;
	m_closestHumanFriend = NULL;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = static_cast<CBasePlayer *>(UTIL_PlayerByIndex( i ));
		if ( player == NULL || FNullEnt( player->pev ) || STRING( player->pev->netname )[0] == '\0' ||
			!player->IsPlayer() || player->entindex() == entindex() || !player->IsAlive() )
			continue;
		if ( player->m_iTeam == m_iTeam )
		{
			m_eyePosition = pev->origin + pev->view_ofs;
			TraceResult trace;
			UTIL_TraceLine( m_eyePosition, player->pev->origin, ignore_monsters, ignore_glass, ENT( pev ), &trace );
			if ( trace.flFraction == 1.0f )
			{
				PlayerVisibility &visibility = m_playerVisibility[player->entindex() - 1];
				visibility.timestamp = gpGlobals->time;
				visibility.isEnemy = false;
				float rangeSquared = (pev->origin - player->pev->origin).LengthSquared();
				if ( closestFriendRange > rangeSquared )
				{
					m_closestFriend = player;
					closestFriendRange = rangeSquared;
				}
				if ( !player->IsBot() && closestHumanRange > rangeSquared )
				{
					m_closestHumanFriend = player;
					closestHumanRange = rangeSquared;
				}
			}
			continue;
		}
		if ( !IsVisible( player, CHECK_FOV, NULL ) )
			continue;
		PlayerVisibility &visibility = m_playerVisibility[player->entindex() - 1];
		visibility.timestamp = gpGlobals->time;
		visibility.isEnemy = true;
		if ( player->IsBombGuy() )
			m_visibleBomber = player;
		float rangeSquared = (pev->origin - player->pev->origin).LengthSquared();
		int index = 0;
		while ( index < threatCount && threat[index].rangeSquared <= rangeSquared )
			++index;
		for ( int j = threatCount; j > index; --j )
			threat[j] = threat[j - 1];
		threat[index].player = player;
		threat[index].rangeSquared = rangeSquared;
		if ( threatCount <= 15 )
			++threatCount;
	}

	int oldEnemyCount = m_nearbyEnemyCount;
	int oldFriendCount = m_nearbyFriendCount;
	m_nearbyEnemyCount = 0;
	m_nearbyFriendCount = 0;
	for ( int i = 0; i < 32; ++i )
	{
		const PlayerVisibility &visibility = m_playerVisibility[i];
		if ( visibility.timestamp > 0.0f && gpGlobals->time - visibility.timestamp < 3.0f )
		{
			if ( visibility.isEnemy )
				++m_nearbyEnemyCount;
			else
				++m_nearbyFriendCount;
		}
	}
	if ( oldEnemyCount == 0 && m_nearbyEnemyCount != 0 )
		m_firstNearbyEnemyTime = gpGlobals->time;
	if ( oldEnemyCount != m_nearbyEnemyCount || oldFriendCount != m_nearbyFriendCount )
		PrintIfWatched( "Nearby friends = %d, enemies = %d\n", m_nearbyFriendCount, m_nearbyEnemyCount );
	if ( threatCount == 0 )
	{
		m_enemyPlace = UNDEFINED_PLACE;
		return NULL;
	}

	struct PlaceRank
	{
		Place place;
		int count;
	};
	static PlaceRank placeRank[64];
	int placeCount = 0;
	int bestCount = 0;
	Place bestPlace = UNDEFINED_PLACE;
	for ( int i = 0; i < threatCount; ++i )
	{
		CBasePlayer *player = threat[i].player;
		CNavArea *area = player->IsBot() ? static_cast<CCSBot *>(player)->GetLastKnownArea() :
			TheNavAreaGrid.GetNearestNavArea( &player->pev->origin );
		if ( area == NULL || area->GetPlace() == UNDEFINED_PLACE )
			continue;
		Place place = area->GetPlace();
		int index = 0;
		while ( index < placeCount && placeRank[index].place != place )
			++index;
		if ( index < placeCount )
		{
			int count = ++placeRank[index].count;
			if ( bestCount < count )
			{
				bestCount = count;
				bestPlace = placeRank[index].place;
			}
		}
		else if ( placeCount <= 63 )
		{
			placeRank[placeCount].place = place;
			placeRank[placeCount].count = 1;
			if ( bestCount == 0 )
			{
				bestPlace = place;
				bestCount = 1;
			}
			++placeCount;
		}
	}
	m_enemyPlace = bestPlace;
	for ( int i = 0; i < threatCount; ++i )
	{
		CBasePlayer *player = threat[i].player;
		if ( !player->HasShield() || !player->m_bShieldDrawn )
			return player;
	}
	return threat[0].player;
}
