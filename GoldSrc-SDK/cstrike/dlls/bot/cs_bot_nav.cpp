#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "cs_bot.h"
#include "bot_profile.h"
#include <algorithm>
#include <cstdlib>

namespace
{
	enum LadderState
	{
		APPROACH_ASCENDING_LADDER = 0,
		APPROACH_DESCENDING_LADDER = 1,
		FACE_ASCENDING_LADDER = 2,
		FACE_DESCENDING_LADDER = 3,
		MOUNT_ASCENDING_LADDER = 4,
		MOUNT_DESCENDING_LADDER = 5,
		ASCEND_LADDER = 6,
		DESCEND_LADDER = 7,
		DISMOUNT_ASCENDING_LADDER = 8,
		MOVE_TO_DESTINATION = 10
	};
}

bool CCSBot::StayOnNavMesh()
{
	if ( m_area )
		return true;
	CNavArea* area = m_lastKnownArea;
	if ( area )
		PrintIfWatched( "Getting out of NULL area...\n" );
	else
	{
		area = TheNavAreaGrid.GetNearestNavArea( &pev->origin, false );
		PrintIfWatched( "Started off the nav mesh - moving to closest nav area...\n" );
	}
	if ( area )
	{
		Vector close;
		area->GetClosestPointOnArea( &pev->origin, &close );
		Vector direction = close - pev->origin;
		direction.NormalizeInPlace();
		close = close + direction * 5.0f;
		MoveTowardsPosition( &close );
	}
	if ( m_stuck )
		Wiggle();
	return false;
}

bool CCSBot::GetSimpleGroundHeightWithFloor( const Vector* position, float* height, Vector* normal )
{
	bool result = GetSimpleGroundHeight( position, height, normal );
	if ( result && m_lastKnownArea && m_lastKnownArea->IsOverlapping( position ) )
		*height = std::max( m_lastKnownArea->GetZ( position ), *height );
	return result;
}

float CCSBot::GetApproximateFallDamage( float height ) const
{
	float damage = height * 0.2178f - 26.0f;
	return damage >= 0.0f ? damage : 0.0f;
}

bool CCSBot::IsStraightLinePathWalkable( const Vector* ) const
{
	return true;
}

bool CCSBot::DiscontinuityJump( float groundHeight, bool onlyJumpDown, bool mustJump )
{
	if ( m_isJumpCrouching )
		return false;
	float feetHeight = pev->origin.z - (IsCrouching() ? 18.0f : 36.0f);
	float heightChange = groundHeight - feetHeight;
	bool jumped;
	if ( !onlyJumpDown && heightChange > 18.0f )
		jumped = Jump( true );
	else
	{
		if ( m_ladder || heightChange >= -41.8f )
			return false;
		jumped = Jump( mustJump );
	}
	if ( !jumped )
		return false;
	m_isJumpCrouching = true;
	m_hasJumpCrouched = false;
	StandUp();
	m_jumpCrouchTimestamp = gpGlobals->time;
	return jumped;
}

void CCSBot::MoveTowardsPosition( const Vector* position )
{
	if ( (m_lastKnownArea == NULL || !(m_lastKnownArea->GetAttributes() & NAV_NO_JUMP)) && !IsOnLadder() && !m_isJumpCrouching )
	{
		Vector direction = *position - pev->origin;
		direction.z = 0.0f;
		direction.NormalizeInPlace();
		Vector probe;
		Vector normal;
		float ground;
		bool jumped = false;
		if ( IsRunning() )
		{
			probe = pev->origin + direction * 80.0f + Vector( 0, 0, 36 );
			if ( GetSimpleGroundHeightWithFloor( &probe, &ground, &normal ) && normal.z > 0.9f && !m_isJumpCrouching )
				jumped = DiscontinuityJump( ground, true, false );
		}
		if ( !jumped )
		{
			probe = pev->origin + direction * 30.0f + Vector( 0, 0, 36 );
			if ( GetSimpleGroundHeightWithFloor( &probe, &ground, NULL ) && !m_isJumpCrouching )
				jumped = DiscontinuityJump( ground, false, false );
			if ( !jumped )
			{
				probe = pev->origin + direction * 10.0f + Vector( 0, 0, 36 );
				if ( GetSimpleGroundHeightWithFloor( &probe, &ground, NULL ) && !m_isJumpCrouching )
					DiscontinuityJump( ground, true, true );
			}
		}
	}
	float sine = BotSIN( pev->v_angle.y );
	float cosine = BotCOS( pev->v_angle.y );
	Vector2D direction = (*position - pev->origin).Make2D();
	direction.NormalizeInPlace();
	float forward = cosine * direction.x + sine * direction.y;
	if ( forward > 0.25f )
		MoveForward();
	else if ( forward < -0.25f )
		MoveBackward();
	if ( !m_nearbyPlayer )
	{
		float side = cosine * direction.y - sine * direction.x;
		if ( side >= 0.25f )
			StrafeLeft();
		else if ( side <= -0.25f )
			StrafeRight();
	}
}

void CCSBot::MoveAwayFromPosition( const Vector* position )
{
	float sine = BotSIN( pev->v_angle.y );
	float cosine = BotCOS( pev->v_angle.y );
	Vector2D direction = (*position - pev->origin).Make2D();
	direction.NormalizeInPlace();
	float forward = cosine * direction.x + sine * direction.y;
	float side = cosine * direction.y - sine * direction.x;
	if ( forward > 0.5f )
		MoveBackward();
	else if ( forward < -0.5f )
		MoveForward();
	if ( side >= 0.5f )
		StrafeRight();
	else if ( side <= -0.5f )
		StrafeLeft();
}

void CCSBot::StrafeAwayFromPosition( const Vector* position )
{
	float sine = BotSIN( pev->v_angle.y );
	float cosine = BotCOS( pev->v_angle.y );
	Vector2D direction = (*position - pev->origin).Make2D();
	direction.NormalizeInPlace();
	if ( cosine * direction.y - sine * direction.x < 0.0f )
		StrafeLeft();
	else
		StrafeRight();
}

void CCSBot::ComputeApproachPoints()
{
	m_approachPointCount = 0;
	if ( m_lastKnownArea == NULL )
		return;
	Vector eye = pev->origin;
	for ( int i = 0; i < m_lastKnownArea->GetApproachInfoCount() && m_approachPointCount <= 15; ++i )
	{
		const CNavArea::ApproachInfo* approach = m_lastKnownArea->GetApproachInfo( i );
		if ( approach->here.area == NULL || approach->prev.area == NULL )
			continue;
		Vector center;
		if ( approach->prevToHereHow <= GO_WEST )
		{
			float halfWidth;
			approach->prev.area->ComputePortal( approach->here.area, static_cast<NavDirType>( approach->prevToHereHow ), &center, &halfWidth );
			center.z = approach->here.area->GetZ( &center );
		}
		else
			center = *approach->here.area->GetCenter();
		Vector visible;
		if ( BendLineOfSight( &eye, &center, &visible ) )
			m_approachPoint[m_approachPointCount++] = visible;
	}
}

void CCSBot::DrawApproachPoints()
{
	for ( int i = 0; i < m_approachPointCount; ++i )
		UTIL_DrawBeamPoints( m_approachPoint[i], m_approachPoint[i] + Vector( 0, 0, 50 ), 3, 0, 255, 255 );
}

bool CCSBot::FindApproachPointNearestPath( Vector* point )
{
	if ( m_pathLength == 0 )
		return false;
	ComputeApproachPoints();
	float bestDistance = 0.0f;
	bool found = false;
	Vector best;
	for ( int i = 0; i < m_approachPointCount; ++i )
	{
		Vector close;
		if ( FindClosestPointOnPath( &m_approachPoint[i], m_pathIndex, m_pathLength, &close ) )
		{
			float distance = (m_approachPoint[i] - close).LengthSquared();
			if ( distance <= 10000.0f && distance > bestDistance )
			{
				best = close;
				found = true;
				bestDistance = distance;
			}
		}
	}
	if ( !found )
		return false;
	*point = best + Vector( 0, 0, 36 );
	return true;
}

bool CCSBot::IsNearJump() const
{
	if ( m_pathIndex == 0 || m_pathIndex >= m_pathLength )
		return false;
	return (m_path[m_pathIndex - 1].area->GetAttributes() & NAV_JUMP) && m_path[m_pathIndex].position.z - m_path[m_pathIndex - 1].position.z > 0.0f;
}

bool CCSBot::IsFriendInTheWay( const Vector* goal ) const
{
	if ( !m_friendCheckTimer.IsElapsed() )
		return m_isFriendInTheWay;
	m_friendCheckTimer.Start( 0.5f );
	Vector direction = *goal - pev->origin;
	float length = direction.NormalizeInPlace();
	m_isFriendInTheWay = false;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* player = static_cast<CBasePlayer*>( UTIL_PlayerByIndex( i ) );
		if ( player == NULL || FNullEnt( player->pev ) || !player->IsAlive() || player->m_iTeam != m_iTeam || player == this )
			continue;
		Vector delta = player->pev->origin - pev->origin;
		if ( delta.LengthSquared() > 10000.0f )
			continue;
		float along = DotProduct( delta, direction );
		if ( along <= 0.0f )
			continue;
		Vector close = along >= length ? *goal : pev->origin + direction * along;
		if ( (close - player->pev->origin).LengthSquared() < 900.0f )
		{
			m_isFriendInTheWay = true;
			return true;
		}
	}
	return m_isFriendInTheWay;
}

void CCSBot::FeelerReflexAdjustment( Vector* goal )
{
	if ( m_lastKnownArea && (m_lastKnownArea->GetAttributes() & NAV_PRECISE) )
		return;
	float sine = BotSIN( m_lookAheadMovementYaw );
	float cosine = BotCOS( m_lookAheadMovementYaw );
	float spacing = IsCrouching() ? 15.0f : 20.0f;
	float length = IsCrouching() ? 20.0f : (IsRunning() ? 50.0f : 30.0f);
	m_eyePosition = pev->origin + pev->view_ofs;
	float ground;
	Vector normal;
	if ( !GetSimpleGroundHeightWithFloor( &m_eyePosition, &ground, &normal ) )
		return;
	Vector forward = CrossProduct( Vector( -sine, cosine, 0 ), normal );
	Vector side = CrossProduct( forward, normal );
	Vector feet = pev->origin;
	feet.z -= IsCrouching() ? 18.0f : 36.0f;
	Vector center = feet + Vector( 0, 0, 18.1f );
	Vector leftStart = center + side * spacing;
	Vector leftEnd = leftStart + forward * length;
	bool leftClear = IsWalkableTraceLineClear( leftStart, leftEnd, WALK_THRU_DOORS | WALK_THRU_BREAKABLES );
	if ( (cv_bot_traceview.value == 1.0f && IsLocalPlayerWatchingMe()) || cv_bot_traceview.value == 10.0f )
		UTIL_DrawBeamPoints( leftStart, leftEnd, 1, leftClear ? 0 : 255, leftClear ? 255 : 0, 0 );
	Vector rightStart = center - side * spacing;
	Vector rightEnd = rightStart + forward * length;
	bool rightClear = IsWalkableTraceLineClear( rightStart, rightEnd, WALK_THRU_DOORS | WALK_THRU_BREAKABLES );
	if ( (cv_bot_traceview.value == 1.0f && IsLocalPlayerWatchingMe()) || cv_bot_traceview.value == 10.0f )
		UTIL_DrawBeamPoints( rightStart, rightEnd, 1, rightClear ? 0 : 255, rightClear ? 255 : 0, 0 );
	float avoidance = IsCrouching() ? 150.0f : 300.0f;
	if ( rightClear )
	{
		if ( !leftClear )
			*goal = *goal - side * avoidance;
	}
	else if ( leftClear )
		*goal = *goal + side * avoidance;
}

void CCSBot::SetupLadderMovement()
{
	if ( m_pathIndex <= 0 || m_pathLength == 0 || m_path[m_pathIndex].ladder == NULL )
		return;
	m_spotEncounter = NULL;
	m_ladder = m_path[m_pathIndex].ladder;
	m_areaEnteredTimestamp = gpGlobals->time;
	m_ladderStartTimestamp = gpGlobals->time;
	if ( m_path[m_pathIndex].how == GO_LADDER_UP )
	{
		m_ladderState = APPROACH_ASCENDING_LADDER;
		m_isAscendingLadder = true;
		PrintIfWatched( "APPROACH_ASCENDING_LADDER\n" );
		m_goalPosition = m_ladder->m_bottom;
		AddDirectionVector( &m_goalPosition, m_ladder->m_dir, 32.0f );
		m_lookAheadYaw = DirectionToAngle( OppositeDirection( m_ladder->m_dir ) );
		return;
	}
	m_goalPosition = m_ladder->m_top;
	AddDirectionVector( &m_goalPosition, OppositeDirection( m_ladder->m_dir ), 32.0f );
	TraceResult trace;
	UTIL_TraceLine( m_ladder->m_top, m_goalPosition, ignore_monsters, m_ladder->m_entity->edict(), &trace );
	m_ladderState = APPROACH_DESCENDING_LADDER;
	if ( trace.flFraction == 1.0f )
	{
		PrintIfWatched( "APPROACH_DESCENDING_LADDER (face out)\n" );
		m_isAscendingLadder = false;
		m_lookAheadYaw = DirectionToAngle( m_ladder->m_dir );
	}
	else
	{
		PrintIfWatched( "APPROACH_DESCENDING_LADDER (face in)\n" );
		m_isAscendingLadder = true;
		m_lookAheadYaw = DirectionToAngle( OppositeDirection( m_ladder->m_dir ) );
		m_goalPosition = m_ladder->m_top;
		AddDirectionVector( &m_goalPosition, m_ladder->m_dir, 16.0f );
	}
}

void CCSBot::ComputeLadderEndpoint( bool ascending )
{
	Vector from = ascending ? m_ladder->m_bottom : m_ladder->m_top;
	const Vector& to = ascending ? m_ladder->m_top : m_ladder->m_bottom;
	from.z = pev->origin.z;
	TraceResult trace;
	UTIL_TraceLine( from, to, ignore_monsters, m_ladder->m_entity->edict(), &trace );
	m_ladderEndHeight = trace.flFraction == 1.0f ? to.z : from.z + trace.flFraction * (to.z - from.z);
}

bool CCSBot::UpdateLadderMovement()
{
	if ( m_ladder == NULL )
		return false;
	if ( gpGlobals->time - m_ladderStartTimestamp > 10.0f )
	{
		PrintIfWatched( "Ladder timeout!\n" );
		Jump( true );
		Wiggle();
		ResetStuckMonitor();
		m_pathLength = 0;
		m_ladder = NULL;
		Run();
		return false;
	}
	if ( (m_ladderState == APPROACH_ASCENDING_LADDER || m_ladderState == APPROACH_DESCENDING_LADDER || m_ladderState == ASCEND_LADDER || m_ladderState == DESCEND_LADDER || m_ladderState == DISMOUNT_ASCENDING_LADDER || m_ladderState == MOVE_TO_DESTINATION) && m_stuck )
	{
		PrintIfWatched( "Giving up ladder - stuck\n" );
		Jump( true );
		Wiggle();
		ResetStuckMonitor();
		m_pathLength = 0;
		m_ladder = NULL;
		Run();
		return false;
	}
	ResetStuckMonitor();
	if ( m_ladderState >= MOUNT_ASCENDING_LADDER && m_ladderState <= DESCEND_LADDER && (m_ladder->m_top - pev->origin).Make2D().IsLengthGreaterThan( 200.0f ) )
	{
		PrintIfWatched( "Missed ladder\n" );
		Jump( true );
		m_pathLength = 0;
		m_ladder = NULL;
		Run();
		return false;
	}
	m_areaEnteredTimestamp = gpGlobals->time;
	float feetHeight = pev->origin.z - (IsCrouching() ? 18.0f : 36.0f);
	switch ( m_ladderState )
	{
	case APPROACH_ASCENDING_LADDER:
	{
		Vector2D delta = (pev->origin - m_goalPosition).Make2D();
		bool aligned = DotProduct( delta, m_ladder->m_dirVector ) < 0.0f && std::abs( static_cast<int>( m_ladder->m_dirVector.x * delta.y - m_ladder->m_dirVector.y * delta.x ) ) < 10 && delta.IsLengthLessThan( 25.0f );
		if ( delta.IsLengthLessThan( 50.0f ) )
		{
			Walk();
			StandUp();
		}
		if ( IsOnLadder() )
		{
			m_ladderState = ASCEND_LADDER;
			PrintIfWatched( "ASCEND_LADDER\n" );
			ComputeLadderEndpoint( true );
		}
		else if ( aligned )
		{
			m_ladderState = FACE_ASCENDING_LADDER;
			PrintIfWatched( "FACE_ASCENDING_LADDER\n" );
		}
		else
			MoveTowardsPosition( &m_goalPosition );
		return true;
	}
	case APPROACH_DESCENDING_LADDER:
	{
		if ( m_ladder->m_bottom.z + 36.0f >= feetHeight )
			break;
		Vector2D delta = (pev->origin - m_goalPosition).Make2D();
		bool aligned = DotProduct( delta, m_ladder->m_dirVector ) > 0.0f && std::abs( static_cast<int>( m_ladder->m_dirVector.x * delta.y - m_ladder->m_dirVector.y * delta.x ) ) < 10 && delta.IsLengthLessThan( 25.0f );
		if ( m_ladder->m_topBehindArea != m_lastKnownArea && !IsCrouching() && delta.IsLengthLessThan( 150.0f ) )
			Walk();
		if ( IsOnLadder() )
		{
			m_ladderState = DESCEND_LADDER;
			Run();
			PrintIfWatched( "DESCEND_LADDER\n" );
			ComputeLadderEndpoint( false );
		}
		else if ( aligned )
		{
			m_ladderState = FACE_DESCENDING_LADDER;
			PrintIfWatched( "FACE_DESCENDING_LADDER\n" );
		}
		else
			MoveTowardsPosition( &m_goalPosition );
		return true;
	}
	case FACE_ASCENDING_LADDER:
	case FACE_DESCENDING_LADDER:
	{
		Vector target = m_ladderState == FACE_ASCENDING_LADDER ? m_ladder->m_bottom : m_ladder->m_top;
		Vector angles = UTIL_VecToAngles( target - pev->origin );
		float difference = pev->v_angle.y - angles.y;
		while ( difference > 180.0f )
			difference -= 360.0f;
		while ( difference < -180.0f )
			difference += 360.0f;
		if ( std::abs( static_cast<int>( difference ) ) < 5 )
		{
			if ( m_ladderState == FACE_ASCENDING_LADDER )
			{
				Run();
				ResetStuckMonitor();
				m_ladderState = MOUNT_ASCENDING_LADDER;
				PrintIfWatched( "MOUNT_ASCENDING_LADDER\n" );
			}
			else
			{
				m_ladderState = MOUNT_DESCENDING_LADDER;
				ResetStuckMonitor();
				PrintIfWatched( "MOUNT_DESCENDING_LADDER\n" );
			}
		}
		return true;
	}
	case MOUNT_ASCENDING_LADDER:
		if ( IsOnLadder() )
		{
			m_ladderState = ASCEND_LADDER;
			PrintIfWatched( "ASCEND_LADDER\n" );
			ComputeLadderEndpoint( true );
		}
		MoveForward();
		return true;
	case MOUNT_DESCENDING_LADDER:
		if ( m_ladder->m_bottom.z + 36.0f >= feetHeight )
			break;
		if ( IsOnLadder() )
		{
			m_ladderState = DESCEND_LADDER;
			PrintIfWatched( "DESCEND_LADDER\n" );
			ComputeLadderEndpoint( false );
		}
		MoveForward();
		return true;
	case ASCEND_LADDER:
		Run();
		if ( m_path[m_pathIndex].area->GetAttributes() & NAV_CROUCH )
			Crouch();
		feetHeight = pev->origin.z - (IsCrouching() ? 18.0f : 36.0f);
		if ( feetHeight < m_ladderEndHeight )
		{
			if ( !IsOnLadder() )
			{
				m_pathLength = 0;
				m_ladder = NULL;
				return false;
			}
		}
		else
		{
			m_ladderState = DISMOUNT_ASCENDING_LADDER;
			PrintIfWatched( "DISMOUNT_ASCENDING_LADDER\n" );
			if ( m_path[m_pathIndex].area == m_ladder->m_topForwardArea )
				m_ladderDismountDirection = FORWARD;
			else if ( m_path[m_pathIndex].area == m_ladder->m_topLeftArea )
				m_ladderDismountDirection = LEFT;
			else if ( m_path[m_pathIndex].area == m_ladder->m_topRightArea )
				m_ladderDismountDirection = RIGHT;
			m_ladderDismountTimestamp = gpGlobals->time;
		}
		MoveForward();
		return true;
	case DESCEND_LADDER:
		Run();
		if ( !IsOnLadder() || m_ladderEndHeight + 36.0f >= feetHeight )
		{
			m_ladderState = MOVE_TO_DESTINATION;
			m_path[m_pathIndex].area->GetClosestPointOnArea( &m_ladder->m_bottom, &m_goalPosition );
			AddDirectionVector( &m_goalPosition, m_ladder->m_dir, 16.0f );
			PrintIfWatched( "MOVE_TO_DESTINATION\n" );
		}
		MoveForward();
		return true;
	case DISMOUNT_ASCENDING_LADDER:
		if ( gpGlobals->time - m_ladderDismountTimestamp >= 0.4f )
		{
			m_ladderState = MOVE_TO_DESTINATION;
			m_path[m_pathIndex].area->GetClosestPointOnArea( &pev->origin, &m_goalPosition );
			PrintIfWatched( "MOVE_TO_DESTINATION\n" );
		}
		if ( m_isAscendingLadder )
		{
			if ( m_ladderDismountDirection == RIGHT )
				StrafeRight();
			else if ( m_ladderDismountDirection == LEFT )
				StrafeLeft();
			else if ( m_ladderDismountDirection == FORWARD )
				MoveForward();
			return m_isAscendingLadder;
		}
		if ( m_ladderDismountDirection == FORWARD )
			MoveBackward();
		else if ( m_ladderDismountDirection == RIGHT )
			StrafeLeft();
		else if ( m_ladderDismountDirection == LEFT )
			StrafeRight();
		return true;
	case MOVE_TO_DESTINATION:
		if ( m_path[m_pathIndex].area->Contains( &pev->origin ) )
		{
			PrintIfWatched( "Ladder traversed.\n" );
			m_ladder = NULL;
			SetPathIndex( m_pathIndex + 1 );
			return false;
		}
		MoveTowardsPosition( &m_goalPosition );
		return true;
	default:
		return true;
	}
	PrintIfWatched( "Fell from ladder.\n" );
	m_ladderState = MOVE_TO_DESTINATION;
	m_path[m_pathIndex].area->GetClosestPointOnArea( &m_ladder->m_bottom, &m_goalPosition );
	AddDirectionVector( &m_goalPosition, m_ladder->m_dir, 16.0f );
	PrintIfWatched( "MOVE_TO_DESTINATION\n" );
	return true;
}

