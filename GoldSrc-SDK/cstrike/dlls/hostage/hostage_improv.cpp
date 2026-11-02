#include "hostage_improv.h"
#include "cs_bot_manager.h"
#include "gamerules.h"
#include "weapons.h"
#include <math.h>
#include <stdio.h>

CHostageImprov::CHostageImprov( CBaseEntity *entity )
{
	m_moveFlags = 0;
	m_vel = Vector2D( 0, 0 );
	m_actualVel = Vector( 0, 0, 0 );
	m_moveType = Stopped;
	m_hostage = (CHostage *)entity;
	m_lastKnownArea = NULL;
	m_hasKnownGoodPos = false;
	m_hasPriorKnownGoodPos = false;
	m_isTerroristNearby = false;
	m_moveLimit = Running;
	m_isCrouching = false;
	m_moveAngle = entity->pev->angles.y;
	m_moveGoal = m_hostage->GetHomePosition();
	m_blinkTimer.Invalidate();
	m_isLookingAt = false;
	m_isFacingTo = false;
	m_blinkCounter = 0;
	m_hostage->SetBoneController( EYEBROW, 0 );
	m_hostage->SetBoneController( SNEER, 0 );
	m_hostage->SetBoneController( MOUTH, 0 );
	m_path.Invalidate();
	m_visiblePlayerCount = 0;
	m_chatterTimer.Invalidate();
	m_isDelayedChatterPending = false;
	m_animateState.Reset();
	m_didFidget = false;
	m_lastSawCT.Start();
	m_lastSawT.Start();
	m_scaredTimer.Invalidate();
	m_jumpTimer.Invalidate();
	m_hasJumped = false;
	m_isFriendInTheWay = false;
	m_aggression = RANDOM_FLOAT( 0, 1 );
	StandUp();
	m_behavior.Reset( this );
	m_behavior.SetState( &m_idleState );
}

CHostageImprov::~CHostageImprov()
{
}

void HostageStateMachine::OnMoveToSuccess( const Vector &goal )
{
	if ( m_state )
		m_state->OnMoveToSuccess( goal );
}

void HostageStateMachine::OnMoveToFailure( const Vector &goal, MoveToFailureType reason )
{
	if ( m_state )
		m_state->OnMoveToFailure( goal, reason );
}

void HostageStateMachine::OnInjury( float amount )
{
	if ( m_state )
		m_state->OnInjury( amount );
}

bool CHostageImprov::IsAlive() const
{
	return m_hostage->pev->deadflag != DEAD_DEAD;
}

const Vector &CHostageImprov::GetFeet() const
{
	return m_hostage->pev->origin;
}

const Vector &CHostageImprov::GetCentroid() const
{
	m_centroid = m_hostage->pev->origin;
	m_centroid.z += 36;
	return m_centroid;
}

const Vector &CHostageImprov::GetEyes() const
{
	m_eye = m_hostage->pev->origin;
	m_eye.z += 72 - 7;
	return m_eye;
}

float CHostageImprov::GetMoveAngle() const { return m_moveAngle; }
float CHostageImprov::GetFaceAngle() const { return m_moveAngle; }
bool CHostageImprov::IsRunning() const { return m_moveType == Running; }
bool CHostageImprov::IsWalking() const { return m_moveType == Walking; }
bool CHostageImprov::IsStopped() const { return m_moveType == Stopped; }
bool CHostageImprov::IsCrouching() const { return m_isCrouching; }
bool CHostageImprov::IsJumping() const { return !m_jumpTimer.IsElapsed(); }
bool CHostageImprov::IsUsingLadder() const { return false; }
bool CHostageImprov::IsOnGround() const { return (m_hostage->pev->flags & FL_ONGROUND) != 0; }
bool CHostageImprov::IsMoving() const { return m_actualVel.LengthSquared() > 100; }
bool CHostageImprov::CanRun() const { return true; }
bool CHostageImprov::CanCrouch() const { return true; }
bool CHostageImprov::CanJump() const { return true; }
bool CHostageImprov::HasLookAt() const { return m_isLookingAt; }
bool CHostageImprov::HasFaceTo() const { return m_isFacingTo; }
bool CHostageImprov::IsAtFaceGoal() const { return false; }
CNavArea *CHostageImprov::GetLastKnownArea() const { return m_lastKnownArea; }
float CHostageImprov::GetSpeed() { return -1; }

void CHostageImprov::LookAt( const Vector &target )
{
	m_viewGoal = target;
	m_isLookingAt = true;
}

void CHostageImprov::ClearLookAt() { m_isLookingAt = false; }

void CHostageImprov::FaceTo( const Vector &goal )
{
	m_faceGoal = goal;
	m_isFacingTo = true;
}

void CHostageImprov::ClearFaceTo() { m_isFacingTo = false; }
void CHostageImprov::Run() { m_moveType = m_moveLimit; }
void CHostageImprov::Walk() { m_moveType = m_moveLimit > Stopped ? Walking : m_moveLimit; }
void CHostageImprov::MoveForward() { m_moveFlags |= IN_FORWARD; }
void CHostageImprov::MoveBackward() { m_moveFlags |= IN_BACK; }
void CHostageImprov::StrafeLeft() { m_moveFlags |= IN_MOVELEFT; }
void CHostageImprov::StrafeRight() { m_moveFlags |= IN_MOVERIGHT; }

void CHostageImprov::MoveTo( const Vector &goal )
{
	m_moveGoal = goal;
	m_path.Invalidate();
	if ( !m_hasKnownGoodPos )
	{
		m_hasKnownGoodPos = true;
		SetKnownGoodPosition( GetFeet() );
	}
}

void CHostageImprov::Stop()
{
	MoveTo( GetFeet() );
	m_hostage->pev->velocity = Vector( 0, 0, 0 );
	m_moveType = m_moveLimit >= Stopped ? Stopped : m_moveLimit;
}

bool CHostageImprov::IsAtMoveGoal( float error ) const
{
	return (GetFeet() - m_moveGoal).LengthSquared() < error * error;
}

void CHostageImprov::StartLadder( const CNavLadder *, NavTraverseType, const Vector *, const Vector * )
{
	m_traversingLadder = true;
}

bool CHostageImprov::Jump()
{
	if ( IsCrouching() || g_pHostages->IsNearbyHostageJumping( this ) || !m_jumpTimer.IsElapsed() )
		return false;
	m_moveFlags |= IN_JUMP;
	m_hasJumped = false;
	if ( m_hostage->pev->velocity.LengthSquared() < 1 )
		m_hostage->pev->origin.z += 2;
	m_jumpTimer.Start( 3 );
	m_animateState.Reset();
	m_animateState.SetPerformance( HostageAnimateState::Jump );
	m_animateState.AddSequence( this, ACT_HOP, -1, 1 );
	return true;
}

void CHostageImprov::Crouch()
{
	if ( IsCrouching() )
		return;
	m_isCrouching = true;
	m_minCrouchTimer.Start( 1 );
	UTIL_SetSize( m_hostage->pev, Vector( -10, -10, 0 ), Vector( 10, 10, 30 ) );
}

void CHostageImprov::StandUp()
{
	if ( !IsCrouching() || !m_minCrouchTimer.IsElapsed() )
		return;
	for ( int y = -24; y != 36; y += 12 )
	{
		for ( int x = -24; x != 36; x += 12 )
		{
			Vector end = GetFeet() + Vector( (float)x, (float)y, 72 );
			Vector start = GetFeet() + Vector( 0, 0, 3 );
			TraceResult trace;
			UTIL_TraceLine( start, end, ignore_monsters, ignore_glass, m_hostage->edict(), &trace );
			if ( trace.flFraction < 1 )
				return;
		}
	}
	m_animateState.Reset();
	m_animateState.SetPerformance( HostageAnimateState::None );
	UTIL_SetSize( m_hostage->pev, Vector( -10, -10, 0 ), Vector( 10, 10, 62 ) );
	m_isCrouching = false;
}

void CHostageImprov::ApplyForce( Vector force )
{
	m_vel.x += force.x;
	m_vel.y += force.y;
}

void CHostageImprov::OnMoveToSuccess( const Vector &goal )
{
	m_behavior.OnMoveToSuccess( goal );
}

void CHostageImprov::OnMoveToFailure( const Vector &goal, MoveToFailureType reason )
{
	m_behavior.OnMoveToFailure( goal, reason );
	if ( reason == FAIL_STUCK )
		ResetToKnownGoodPosition();
}

void CHostageImprov::OnUpkeep( float deltaT )
{
	if ( IsAlive() )
		UpdatePosition( deltaT );
}

bool CHostageImprov::IsAtHome() const
{
	return m_hostage->IsAtHome();
}

bool CHostageImprov::IsScared() const
{
	return !m_scaredTimer.IsElapsed() || m_animateState.GetPerformance() == HostageAnimateState::Afraid;
}

void CHostageImprov::Frighten( ScareType intensity )
{
	if ( !IsScared() )
	{
		m_animateState.Reset();
		m_blinkTimer.Invalidate();
	}
	m_scareIntensity = intensity;
	switch ( intensity )
	{
	case NERVOUS: m_scaredTimer.Start( RANDOM_FLOAT( 2, 4 ) ); break;
	case SCARED: m_scaredTimer.Start( RANDOM_FLOAT( 3, 8 ) ); break;
	case TERRIFIED:
		m_scaredTimer.Start( RANDOM_FLOAT( 5, 10 ) );
		m_ignoreTerroristTimer.Start( 10 );
		break;
	}
}

float CHostageImprov::GetTimeSinceLastInjury()
{
	return m_lastInjuryTimer.HasStarted() ? m_lastInjuryTimer.GetElapsedTime() : 99999.9f;
}

float CHostageImprov::GetTimeSinceLastNoise()
{
	return m_lastNoiseTimer.HasStarted() ? m_lastNoiseTimer.GetElapsedTime() : 99999.9f;
}

void CHostageImprov::CheckForNearbyTerrorists()
{
	if ( m_checkNearbyTerroristTimer.IsElapsed() )
	{
		m_checkNearbyTerroristTimer.Start( 2 );
		m_isTerroristNearby = false;
		float range;
		if ( GetClosestPlayerByTravelDistance( TERRORIST, &range ) && range < 2000 )
		{
			m_isTerroristNearby = true;
			m_nearbyTerroristTimer.Start( 10 );
		}
	}
}

bool CHostageImprov::GetSimpleGroundHeightWithFloor( const Vector *position, float *height, Vector *normal )
{
	bool found = GetSimpleGroundHeight( position, height, normal );
	if ( found && m_lastKnownArea && m_lastKnownArea->IsOverlapping( position ) )
	{
		float floor = m_lastKnownArea->GetZ( position );
		if ( floor > *height )
			*height = floor;
	}
	return found;
}

void CHostageImprov::DelayedChatter( float delay, HostageChatterType type, bool mustSpeak )
{
	if ( IsAlive() && (unsigned int)m_isDelayedChatterPending <= (unsigned int)mustSpeak )
	{
		m_isDelayedChatterPending = true;
		m_delayedChatterTimer.Start( delay );
		m_delayedChatterMustSpeak = mustSpeak;
		m_delayedChatterType = type;
	}
}

void CHostageImprov::UpdateStationaryAnimation()
{
	if ( m_behavior.m_state )
		m_behavior.m_state->UpdateStationaryAnimation( this );
}

void CHostageImprov::UpdateIdleActivity( Activity activity, Activity fidget )
{
	if ( m_animateState.m_sequenceCount > 0 )
		return;
	m_animateState.Reset();
	if ( m_didFidget || fidget == ACT_RESET )
	{
		float rate = RANDOM_FLOAT( 0.9f, 1.1f );
		float hold = RANDOM_FLOAT( 1, 10 );
		m_animateState.AddSequence( this, activity, hold, rate );
		m_didFidget = false;
	}
	else
	{
		float rate = RANDOM_FLOAT( 0.9f, 1.5f );
		m_animateState.AddSequence( this, fidget, -1, rate );
		m_didFidget = true;
	}
}

void CHostageImprov::Wave()
{
	m_animateState.Reset();
	m_animateState.SetPerformance( HostageAnimateState::Waving );
	m_animateState.AddSequence( this, ACT_WAVE, -1, 1 );
}

void CHostageImprov::Agree()
{
	m_animateState.Reset();
	m_animateState.AddSequence( this, ACT_YES, -1, 1 );
}

void CHostageImprov::Disagree()
{
	if ( m_animateState.GetPerformance() != HostageAnimateState::Disagreeing )
	{
		m_animateState.Reset();
		m_animateState.SetPerformance( HostageAnimateState::Disagreeing );
		m_animateState.AddSequence( this, ACT_NO, -1, RANDOM_FLOAT( 1.5f, 3 ) );
	}
}

void CHostageImprov::CrouchDie()
{
	m_animateState.Reset();
	m_animateState.AddSequence( this, ACT_CROUCH_DIE, -1, 1 );
}

void CHostageImprov::SetKnownGoodPosition( const Vector &position )
{
	if ( IsJumping() || IsCrouching() )
		return;
	if ( m_hasKnownGoodPos && m_priorKnownGoodPosTimer.IsElapsed() )
	{
		m_priorKnownGoodPos = m_knownGoodPos;
		m_hasPriorKnownGoodPos = true;
		m_priorKnownGoodPosTimer.Start( 1 );
	}
	m_knownGoodPos = position;
	m_hasKnownGoodPos = true;
}

void CHostageImprov::ResetToKnownGoodPosition()
{
	if ( m_hasKnownGoodPos )
	{
		if ( (m_hostage->pev->origin - m_knownGoodPos).LengthSquared() < 100 )
		{
			if ( m_hasPriorKnownGoodPos )
			{
				m_hasPriorKnownGoodPos = false;
				m_knownGoodPos = m_priorKnownGoodPos;
				m_priorKnownGoodPosTimer.Start( 5 );
			}
			else
			{
				m_hasKnownGoodPos = false;
				m_knownGoodPos = m_hostage->GetHomePosition();
			}
		}
	}
	else
		m_knownGoodPos = m_hostage->GetHomePosition();
	m_hostage->pev->origin = m_knownGoodPos;
	m_hostage->pev->velocity = Vector( 0, 0, 0 );
	m_path.Invalidate();
	Stop();
}

bool CHostageImprov::IsTerroristNearby()
{
	if ( !m_nearbyTerroristTimer.IsElapsed() )
		return true;
	CheckForNearbyTerrorists();
	return m_isTerroristNearby;
}

float CHostageImprov::GetTimeSinceLastSawPlayer( int team )
{
	if ( team == TERRORIST )
		return m_lastSawT.GetElapsedTime();
	if ( team == CT )
		return m_lastSawCT.GetElapsedTime();
	float ctTime = m_lastSawCT.GetElapsedTime();
	float tTime = m_lastSawT.GetElapsedTime();
	return ctTime > tTime ? tTime : ctTime;
}

void CHostageImprov::Chatter( HostageChatterType type, bool mustSpeak )
{
	if ( (!IsAlive() && type != HOSTAGE_CHATTER_DEATH_CRY) ||
		(g_pHostages->IsNearbyHostageTalking( this ) && !mustSpeak) ||
		(!m_chatterTimer.IsElapsed() && !mustSpeak) )
		return;
	m_chatterTimer.Start( RANDOM_FLOAT( 5, 15 ) );
	float duration = g_pHostages->GetChatter()->PlaySound( m_hostage, type );
	m_talkingTimer.Start( duration );
}

void CHostageImprov::UpdateDelayedChatter()
{
	if ( !m_isDelayedChatterPending || !m_delayedChatterTimer.IsElapsed() )
		return;
	HostageChatterType type = m_delayedChatterType;
	m_isDelayedChatterPending = false;
	if ( type == HOSTAGE_CHATTER_LOOK_OUT || type == HOSTAGE_CHATTER_PLEASE_RESCUE_ME ||
		type == HOSTAGE_CHATTER_IMPATIENT_FOR_RESCUE )
	{
		if ( RANDOM_FLOAT( 0, 100 ) > 60 )
			return;
		type = m_delayedChatterType;
	}
	else if ( type == HOSTAGE_CHATTER_SCARED_OF_GUNFIRE )
	{
		if ( RANDOM_FLOAT( 0, 100 ) > 25 )
			return;
		type = m_delayedChatterType;
	}
	Chatter( type, m_delayedChatterMustSpeak );
}

void CHostageImprov::Flinch( Activity activity )
{
	if ( IsAlive() )
	{
		g_pHostages->IsNearbyHostageTalking( this );
		m_chatterTimer.Start( RANDOM_FLOAT( 5, 15 ) );
		float duration = g_pHostages->GetChatter()->PlaySound( m_hostage, HOSTAGE_CHATTER_PAIN );
		m_talkingTimer.Start( duration );
	}
	if ( !IsCrouching() && !IsMoving() )
	{
		m_animateState.Reset();
		m_animateState.SetPerformance( HostageAnimateState::Flinching );
		m_animateState.AddSequence( this, activity, -1, 1 );
	}
}

void CHostageImprov::Afraid()
{
	if ( IsCrouching() || m_animateState.GetPerformance() == HostageAnimateState::Flinching ||
		m_animateState.GetPerformance() == HostageAnimateState::Afraid || IsMoving() )
		return;
	m_animateState.Reset();
	m_animateState.SetPerformance( HostageAnimateState::Afraid );
	int variant = RANDOM_LONG( 0, 100 ) % 3 + 1;
	char enter[32], loop[32], exit[32];
	sprintf( enter, "cower_into_%d", variant );
	sprintf( loop, "cower_loop_%d", variant );
	sprintf( exit, "cower_exit_%d", variant );
	m_animateState.AddSequence( this, enter, -1, 1 );
	m_animateState.AddSequence( this, loop, RANDOM_FLOAT( 3, 10 ), 1 );
	m_animateState.AddSequence( this, exit, -1, 1 );
}

void CHostageImprov::OnReset()
{
	m_moveFlags = 0;
	m_moveType = Stopped;
	m_moveLimit = Running;
	m_vel = Vector2D( 0, 0 );
	m_actualVel = Vector( 0, 0, 0 );
	m_lastKnownArea = NULL;
	m_hasKnownGoodPos = false;
	m_hasPriorKnownGoodPos = false;
	m_checkNearbyTerroristTimer.Invalidate();
	m_isTerroristNearby = false;
	m_isCrouching = false;
	m_moveAngle = m_hostage->pev->angles.y;
	m_moveGoal = m_hostage->GetHomePosition();
	ClearLookAt();
	ClearFaceTo();
	m_blinkTimer.Invalidate();
	m_blinkCounter = 0;
	m_hostage->SetBoneController( EYEBROW, 0 );
	m_hostage->SetBoneController( SNEER, 0 );
	m_hostage->SetBoneController( MOUTH, 0 );
	m_visiblePlayerCount = 0;
	m_path.Invalidate();
	m_isDelayedChatterPending = false;
	m_chatterTimer.Invalidate();
	m_animateState.Reset();
	m_didFidget = false;
	m_lastSawCT.Start();
	m_lastSawT.Start();
	m_scaredTimer.Invalidate();
	m_jumpTimer.Invalidate();
	m_hasJumped = false;
	m_isFriendInTheWay = false;
	m_aggression = RANDOM_FLOAT( 0, 1 );
	StandUp();
	m_behavior.Reset( this );
	m_behavior.SetState( &m_idleState );
}

bool CHostageImprov::IsVisible( const Vector &position, bool ) const
{
	Vector start = GetEyes();
	TraceResult trace;
	UTIL_TraceLine( start, position, ignore_monsters, ignore_glass, m_hostage->edict(), &trace );
	return trace.flFraction == 1;
}

bool CHostageImprov::IsPlayerLookingAtMe( CBasePlayer *player, float cosTolerance ) const
{
	Vector2D toPlayer = (player->pev->origin - GetCentroid()).Make2D();
	toPlayer.NormalizeInPlace();
	UTIL_MakeVectors( player->pev->v_angle + player->pev->punchangle );
	Vector2D forward = gpGlobals->v_forward.Make2D();
	forward.NormalizeInPlace();
	if ( DotProduct( forward, toPlayer ) >= -cosTolerance )
		return false;
	return IsVisible( player->EyePosition() );
}

CBasePlayer *CHostageImprov::IsAnyPlayerLookingAtMe( int team, float cosTolerance ) const
{
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !player || !player->pev || FNullEnt( player->edict() ) ||
			!STRING( player->pev->netname )[0] || (player->pev->flags & FL_DORMANT) || !player->IsAlive() )
			continue;
		if ( (!team || player->m_iTeam == team) && IsPlayerLookingAtMe( player, cosTolerance ) )
			return player;
	}
	return NULL;
}

void CHostageImprov::UpdateVision()
{
	if ( !m_visionTimer.IsElapsed() )
		return;
	m_visiblePlayerCount = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !player || !player->pev || FNullEnt( player->edict() ) ||
			!STRING( player->pev->netname )[0] || !player->IsAlive() || !IsVisible( player->pev->origin, true ) )
			continue;
		m_visiblePlayer[m_visiblePlayerCount++] = player;
		if ( player->m_iTeam == TERRORIST )
			m_lastSawT.Start();
		else
			m_lastSawCT.Start();
		if ( m_visiblePlayerCount == 32 )
			break;
	}
	m_visionTimer.Start( RANDOM_FLOAT( 0.4f, 0.6f ) );
}

CBasePlayer *CHostageImprov::GetClosestVisiblePlayer( int team )
{
	float closestRange = 100000000;
	CBasePlayer *closest = NULL;
	for ( int i = 0; i < m_visiblePlayerCount; ++i )
	{
		CBasePlayer *player = (CBasePlayer *)(CBaseEntity *)m_visiblePlayer[i];
		if ( player && (team <= 0 || player->m_iTeam == team) )
		{
			float range = (GetCentroid() - player->pev->origin).LengthSquared();
			if ( closestRange > range )
			{
				closestRange = range;
				closest = player;
			}
		}
	}
	return closest;
}

CBasePlayer *CHostageImprov::GetClosestPlayerByTravelDistance( int team, float *range ) const
{
	if ( !GetLastKnownArea() )
		return NULL;
	CBasePlayer *closest = NULL;
	float closestRange = 1.0e11f;
	ShortestPathCost cost;
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( !player || !player->pev || FNullEnt( player->edict() ) || !STRING( player->pev->netname )[0] ||
			(player->pev->flags & FL_DORMANT) || !player->IsAlive() || (team && player->m_iTeam != team) )
			continue;
		Vector center = player->Center();
		CNavArea *area = GetLastKnownArea();
		float distance = NavAreaTravelDistance( &GetFeet(), area, &center, cost );
		if ( distance > 0 && closestRange > distance )
		{
			closestRange = distance;
			closest = player;
		}
	}
	if ( range )
		*range = closestRange;
	return closest;
}

bool CHostageImprov::CanSeeRescueZone() const
{
	if ( !TheCSBots() )
		return false;
	const CCSBotManager::Zone *zone = TheCSBots()->GetClosestZone( &GetCentroid() );
	return zone && IsVisible( zone->m_center );
}

bool CHostageImprov::FaceTowards( const Vector &target, float deltaT )
{
	Vector2D direction = (target - GetFeet()).Make2D();
	direction.NormalizeInPlace();
	float angle = GetMoveAngle();
	float cosine = BotCOS( angle );
	float sine = BotSIN( angle );
	float cross = direction.y * cosine - direction.x * sine;
	if ( direction.x * cosine + direction.y * sine < 0 )
		cross = cross >= 0 ? 1.0f : -1.0f;
	else if ( fabs( cross ) < 0.05f )
		return true;
	angle = angle + deltaT * 300.0f * cross;
	BotCOS( angle );
	BotSIN( angle );
	m_moveAngle = angle;
	m_hostage->pev->angles.y = angle;
	return false;
}

void CHostageImprov::FaceOutwards()
{
	static Vector corner[4] =
	{
		Vector( -1000, 1000, 0 ), Vector( 1000, 1000, 0 ),
		Vector( -1000, -1000, 0 ), Vector( 1000, -1000, 0 )
	};
	float farthestRange = 0;
	int farthest = 0;
	for ( int i = 0; i < 4; ++i )
	{
		Vector end = GetCentroid() + corner[i];
		TraceResult trace;
		UTIL_TraceLine( GetCentroid(), end, ignore_monsters, ignore_glass, m_hostage->edict(), &trace );
		float range = (trace.vecEndPos - GetCentroid()).LengthSquared();
		if ( range > farthestRange )
		{
			farthestRange = range;
			farthest = i;
		}
	}
	FaceTo( GetCentroid() + corner[farthest] );
}

bool CHostageImprov::IsFriendInTheWay( CBaseEntity *other, const Vector &goal ) const
{
	if ( other == m_hostage )
		return false;
	Vector direction = goal - GetFeet();
	float distance = direction.NormalizeInPlace();
	Vector feet = other->pev->origin;
	feet.z = other->IsPlayer() ? other->pev->absmin.z : other->pev->origin.z;
	Vector toFriend = feet - GetFeet();
	if ( toFriend.LengthSquared() > 10000 )
		return false;
	float along = DotProduct( toFriend, direction );
	if ( along <= 0 )
		return false;
	Vector closest = along < distance ? GetFeet() + direction * along : goal;
	if ( (closest - feet).LengthSquared() >= 900 )
		return false;
	const Vector &velocity = other->pev->velocity;
	if ( velocity.x > -0.01f && velocity.x < 0.01f && velocity.y > -0.01f &&
		velocity.y < 0.01f && velocity.z > -0.01f && velocity.z < 0.01f )
		return true;
	return DotProduct( velocity, m_hostage->pev->velocity ) < 0;
}

bool CHostageImprov::IsFriendInTheWay( const Vector &goal ) const
{
	if ( !m_avoidFriendTimer.IsElapsed() )
		return m_isFriendInTheWay;
	m_avoidFriendTimer.Start( 0.5f );
	m_isFriendInTheWay = false;
	if ( g_pHostages )
	{
		Vector destination = goal;
		for ( int i = 0; i < g_pHostages->m_hostageCount; ++i )
		{
			CHostage *hostage = g_pHostages->m_hostage[i];
			if ( hostage->pev->deadflag != DEAD_DEAD && hostage->IsAlive() && IsFriendInTheWay( hostage, destination ) )
			{
				m_isFriendInTheWay = true;
				return true;
			}
		}
	}
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = (CBasePlayer *)UTIL_PlayerByIndex( i );
		if ( player && player->pev && !FNullEnt( player->edict() ) && player->IsAlive() &&
			player->m_iTeam != TERRORIST && IsFriendInTheWay( player, goal ) )
		{
			m_isFriendInTheWay = true;
			return true;
		}
	}
	return m_isFriendInTheWay;
}

class CheckAhead
{
public:
	bool operator()( CBaseEntity *entity )
	{
		if ( entity != m_me->GetEntity() )
		{
			Vector to = entity->Center() - m_me->GetCentroid();
			float range = to.NormalizeInPlace();
			if ( range <= 60 && DotProduct( to, m_dir ) >= 0.95f )
				m_isBlocked = true;
		}
		return true;
	}
	const CHostageImprov *m_me;
	Vector m_dir;
	bool m_isBlocked;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

bool CHostageImprov::IsFriendInTheWay() const
{
	CheckAhead check;
	check.m_me = this;
	float sine = BotSIN( GetMoveAngle() );
	float cosine = BotCOS( GetMoveAngle() );
	check.m_dir = Vector( cosine, sine, 0 );
	check.m_isBlocked = false;
	for ( int i = 0; i < g_pHostages->m_hostageCount; ++i )
	{
		CHostage *hostage = g_pHostages->m_hostage[i];
		if ( hostage->pev->deadflag != DEAD_DEAD )
			check( hostage );
	}
	ForEachPlayer( check );
	return check.m_isBlocked;
}

bool CHostageImprov::DiscontinuityJump( float ground, bool onlyJumpDown, bool )
{
	if ( IsJumping() || IsCrouching() || IsUsingLadder() )
		return false;
	float height = ground - GetFeet().z;
	if ( (height <= 18 || onlyJumpDown) && height >= -41.8f )
		return false;
	Jump();
	return true;
}

void CHostageImprov::Wiggle()
{
	if ( m_wiggleTimer.IsElapsed() )
	{
		m_wiggleDirection = (NavRelativeDirType)RANDOM_LONG( 0, 3 );
		m_wiggleTimer.Start( RANDOM_FLOAT( 0.3f, 0.5f ) );
	}
	float sine = BotSIN( m_moveAngle );
	float cosine = BotCOS( m_moveAngle );
	switch ( m_wiggleDirection )
	{
	case FORWARD: m_vel = m_vel + Vector2D( cosine, sine ) * 15; break;
	case BACKWARD: m_vel = m_vel - Vector2D( cosine, sine ) * 15; break;
	case LEFT: m_vel = m_vel + Vector2D( -sine, cosine ) * 15; break;
	case RIGHT: m_vel = m_vel - Vector2D( -sine, cosine ) * 15; break;
	}
	if ( m_follower.IsStuck() && m_follower.GetStuckDuration() > 0.5f && m_wiggleJumpTimer.IsElapsed() && Jump() )
		m_wiggleJumpTimer.Start( RANDOM_FLOAT( 0.75f, 1.2f ) );
}

void CHostageImprov::ClearPath()
{
	if ( !m_clearPathTimer.IsElapsed() )
		return;
	m_clearPathTimer.Start( RANDOM_FLOAT( 0.3f, 0.5f ) );
	Vector start = GetEyes();
	UTIL_MakeVectors( m_hostage->pev->angles );
	Vector end = start + gpGlobals->v_forward * 64;
	TraceResult trace;
	UTIL_TraceLine( start, end, ignore_monsters, dont_ignore_glass, m_hostage->edict(), &trace );
	if ( trace.flFraction == 1 || !trace.pHit )
		return;
	const char *classname = STRING( trace.pHit->v.classname );
	if ( FStrEq( classname, "func_door" ) || FStrEq( classname, "func_door_rotating" ) )
	{
		CBaseEntity *door = CBaseEntity::Instance( &trace.pHit->v );
		if ( door )
		{
			door->Touch( m_hostage );
			return;
		}
		classname = STRING( trace.pHit->v.classname );
	}
	if ( FStrEq( classname, "func_breakable" ) && trace.pHit->v.takedamage == DAMAGE_YES )
	{
		CBaseEntity *breakable = CBaseEntity::Instance( &trace.pHit->v );
		if ( breakable )
			breakable->TakeDamage( m_hostage->pev, m_hostage->pev, 9999.9f, DMG_BULLET );
	}
}

void CHostageImprov::MoveTowards( const Vector &position, float deltaT )
{
	ClearPath();
	if ( (!m_lastKnownArea || !(m_lastKnownArea->GetAttributes() & NAV_NO_JUMP)) &&
		!IsUsingLadder() && !IsJumping() && IsOnGround() && !IsCrouching() )
	{
		Vector ahead = position - GetFeet();
		ahead.z = 0;
		ahead.NormalizeInPlace();
		float ground;
		Vector normal;
		bool jumped = false;
		if ( IsRunning() )
		{
			Vector step = GetFeet() + ahead * 80 + Vector( 0, 0, 72 );
			if ( GetSimpleGroundHeightWithFloor( &step, &ground, &normal ) && normal.z > 0.9f )
				jumped = DiscontinuityJump( ground, true, false );
		}
		if ( !jumped )
		{
			Vector step = GetFeet() + ahead * 30 + Vector( 0, 0, 72 );
			if ( GetSimpleGroundHeightWithFloor( &step, &ground, NULL ) )
				jumped = DiscontinuityJump( ground, false, false );
			if ( !jumped )
			{
				step = GetFeet() + ahead * 10 + Vector( 0, 0, 72 );
				if ( GetSimpleGroundHeightWithFloor( &step, &ground, NULL ) )
					DiscontinuityJump( ground, true, true );
			}
		}
	}
	Vector direction = position - GetFeet();
	direction.z = 0;
	if ( direction.x <= -0.01f || direction.x >= 0.01f || direction.y <= -0.01f || direction.y >= 0.01f )
		direction.NormalizeInPlace();
	float acceleration;
	switch ( m_moveType )
	{
	case Stopped: acceleration = 0; break;
	case Walking: acceleration = IsCrouching() ? 250.0f : 400.0f; break;
	case Running: acceleration = IsCrouching() ? 250.0f : 1000.0f; break;
	default: return;
	}
	m_vel = m_vel + direction.Make2D() * (acceleration * deltaT);
}

void CHostageImprov::TrackPath( const Vector &goal, float deltaT )
{
	FaceTowards( goal, deltaT );
	MoveTowards( goal, deltaT );
	m_jumpTarget = goal;
	if ( cv_hostage_debug.value > 0 )
	{
		Vector start = goal - Vector( 10, 0, 0 );
		Vector end = goal + Vector( 10, 0, 0 );
		UTIL_DrawBeamPoints( start, end, 2, 255, 0, 255 );
		start = goal - Vector( 0, 10, 0 );
		end = goal + Vector( 0, 10, 0 );
		UTIL_DrawBeamPoints( start, end, 2, 255, 0, 255 );
		start = goal + Vector( 0, 0, 10 );
		end = goal - Vector( 0, 0, 10 );
		UTIL_DrawBeamPoints( start, end, 2, 255, 0, 255 );
	}
}

bool CHostageImprov::TraverseLadder( const CNavLadder *ladder, NavTraverseType how,
	const Vector *, const Vector *depart, float deltaT )
{
	Vector goal = how == GO_LADDER_DOWN ? ladder->m_bottom : ladder->m_top;
	switch ( ladder->m_dir )
	{
	case NORTH: goal.y -= 16; break;
	case EAST: goal.x += 16; break;
	case SOUTH: goal.y += 16; break;
	case WEST: goal.x -= 16; break;
	}
	if ( how == GO_LADDER_DOWN )
	{
		if ( GetFeet().z < ladder->m_top.z - 36 && (GetFeet() - goal).Make2D().LengthSquared() < 2500 )
			m_hostage->pev->velocity.z = -100;
		if ( GetFeet().z < ladder->m_bottom.z + 36 )
			return true;
	}
	else if ( !m_traversingLadder )
	{
		if ( depart )
		{
			float closestRange = 999999.9f;
			CNavArea *areas[3] = { ladder->m_topForwardArea, ladder->m_topLeftArea, ladder->m_topRightArea };
			for ( int i = 0; i < 3; ++i )
			{
				if ( !areas[i] )
					continue;
				float range = (*depart - *areas[i]->GetCenter()).LengthSquared();
				if ( range < closestRange )
				{
					closestRange = range;
					goal = *areas[i]->GetCenter();
					if ( areas[i]->GetAttributes() & NAV_CROUCH )
						Crouch();
				}
			}
		}
		if ( (GetFeet() - goal).Make2D().LengthSquared() < 400 )
			return true;
	}
	else
	{
		if ( (GetFeet() - goal).Make2D().LengthSquared() < 10000 )
			Walk();
		if ( (GetFeet() - goal).Make2D().LengthSquared() < 1600 )
			m_hostage->pev->velocity.z = 150;
		if ( GetFeet().z > ladder->m_top.z )
			m_traversingLadder = false;
	}
	TrackPath( goal, deltaT );
	return false;
}

void CHostageImprov::OnTouch( CBaseEntity *other )
{
	const char *classname = STRING( other->pev->classname );
	if ( cv_hostage_debug.value != 0 )
		CONSOLE_ECHO( "%5.1f: Hostage hit '%s'\n", gpGlobals->time, classname );
	m_collisionTimer.Start();
	if ( FStrEq( classname, "worldspawn" ) )
	{
		UTIL_MakeVectors( m_hostage->pev->angles );
		Vector normal( 0, 0, 1 );
		float ground;
		if ( !GetSimpleGroundHeightWithFloor( &GetEyes(), &ground, &normal ) )
			return;
		if ( cv_hostage_debug.value < 0 )
		{
			Vector start = GetFeet() + normal * 50;
			Vector end = GetFeet();
			UTIL_DrawBeamPoints( start, end, 2, 255, 255, 0 );
		}
		Vector alongFloor = CrossProduct( normal, gpGlobals->v_right );
		bool obstacle = false;
		for ( int i = 0; i < 6; ++i )
		{
			Vector start = GetFeet() + Vector( 0, 0, (float)(1 + 3 * i) );
			Vector end = start + alongFloor * 30;
			TraceResult trace;
			UTIL_TraceLine( start, end, ignore_monsters, dont_ignore_glass, m_hostage->edict(), &trace );
			if ( trace.flFraction < 1 && trace.vecPlaneNormal.z < 0.7f )
			{
				obstacle = true;
				m_inhibitObstacleAvoidance.Start( 0.5f );
				for ( int j = 0; j < 6; ++j )
				{
					Vector step = GetFeet() + alongFloor * (float)(1 + 5 * j);
					step.z = GetEyes().z;
					Vector stepNormal;
					float stepGround;
					if ( GetSimpleGroundHeightWithFloor( &step, &stepGround, &stepNormal ) )
					{
						float height = stepGround - GetFeet().z;
						if ( height > 0 && height < 18 )
						{
							m_hostage->pev->origin.z = stepGround + 3;
							break;
						}
					}
				}
				break;
			}
		}
		if ( !obstacle && !IsMoving() && !IsUsingLadder() )
		{
			bool seam = false;
			Vector start = GetEyes() - alongFloor * 50;
			Vector end = start - Vector( 0, 0, 9999 );
			TraceResult trace;
			UTIL_TraceLine( start, end, ignore_monsters, dont_ignore_glass, m_hostage->edict(), &trace );
			if ( trace.flFraction < 1 && DotProduct( normal, trace.vecPlaneNormal ) < 1 )
				seam = true;
			else
			{
				start = GetEyes() + alongFloor * 50;
				end = start - Vector( 0, 0, 9999 );
				UTIL_TraceLine( start, end, ignore_monsters, dont_ignore_glass, m_hostage->edict(), &trace );
				seam = trace.flFraction < 1 && DotProduct( normal, trace.vecPlaneNormal ) < 1;
			}
			if ( seam )
			{
				if ( cv_hostage_debug.value != 0 )
					CONSOLE_ECHO( "Hostage stuck on seam.\n" );
				m_hostage->pev->origin.z += 3;
			}
		}
	}
	if ( FStrEq( classname, "func_breakable" ) )
		other->TakeDamage( m_hostage->pev, m_hostage->pev, 9999.9f, DMG_BULLET );
	else if ( other->IsPlayer() || FStrEq( STRING( other->pev->classname ), "hostage_entity" ) )
	{
		Vector2D direction = (m_hostage->pev->origin - other->pev->origin).Make2D();
		direction.NormalizeInPlace();
		m_vel = m_vel + direction * 20;
	}
}

class KeepPersonalSpace
{
public:
	bool operator()( CBaseEntity *entity )
	{
		if ( entity == m_improv->GetEntity() || (entity->IsPlayer() && !entity->IsAlive()) )
			return true;
		Vector direction = entity->pev->origin - m_improv->GetCentroid();
		float range = direction.NormalizeInPlace();
		if ( range >= 50 )
			return true;
		if ( entity->IsPlayer() && ((CBasePlayer *)entity)->m_iTeam == CT && !m_improv->IsFollowing() &&
			m_improv->IsPlayerLookingAtMe( (CBasePlayer *)entity, 0.8f ) )
			return true;
		m_improv->ApplyForce( direction * -(50 - range) );
		m_improv->ApplyForce( m_velDir * (-0.1f * m_speed) );
		return true;
	}
	CHostageImprov *m_improv;
	Vector m_velDir;
	float m_speed;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

void CHostageImprov::UpdatePosition( float deltaT )
{
	CNavArea *area = TheNavAreaGrid.GetNavArea( &m_hostage->pev->origin );
	if ( area )
		m_lastKnownArea = area;
	if ( cv_hostage_debug.value > 0 )
	{
		Vector axes[3] = { Vector( 10, 0, 0 ), Vector( 0, 10, 0 ), Vector( 0, 0, -10 ) };
		for ( int i = 0; i < 3; ++i )
		{
			Vector start = m_moveGoal - axes[i];
			Vector end = m_moveGoal + axes[i];
			UTIL_DrawBeamPoints( start, end, 2, 255, 255, 0 );
		}
	}
	if ( IsJumping() )
	{
		if ( m_hasJumped )
		{
			if ( m_hasJumpedIntoAir )
			{
				if ( IsOnGround() )
					m_jumpTimer.Invalidate();
			}
			else if ( !IsOnGround() )
				m_hasJumpedIntoAir = true;
		}
		else
		{
			m_hasJumped = true;
			m_hostage->pev->velocity.z += 300;
			m_hasJumpedIntoAir = false;
		}
		Vector direction = m_jumpTarget - GetFeet();
		direction.z = 0;
		direction.NormalizeInPlace();
		m_hostage->pev->velocity.x = direction.x * 100;
		m_hostage->pev->velocity.y = direction.y * 100;
		m_hostage->SetBoneController( HEAD_YAW, 0 );
		m_hostage->SetBoneController( HEAD_PITCH, 0 );
		FaceTowards( m_jumpTarget, deltaT );
		return;
	}
	if ( m_isLookingAt )
	{
		Vector angles = UTIL_VecToAngles( m_viewGoal - GetEyes() );
		float yaw = angles.y - m_hostage->pev->angles.y;
		while ( yaw > 180 ) yaw -= 360;
		while ( yaw < -180 ) yaw += 360;
		float pitch = angles.x - m_hostage->pev->angles.x;
		while ( pitch > 180 ) pitch -= 360;
		while ( pitch < -180 ) pitch += 360;
		m_hostage->SetBoneController( HEAD_YAW, yaw );
		m_hostage->SetBoneController( HEAD_PITCH, -pitch );
		if ( IsAtMoveGoal() && !HasFaceTo() && (yaw < -45 || yaw > 45) )
			FaceTowards( m_viewGoal, deltaT );
	}
	else
	{
		m_hostage->SetBoneController( HEAD_YAW, 0 );
		m_hostage->SetBoneController( HEAD_PITCH, 0 );
	}
	if ( HasFaceTo() && FaceTowards( m_faceGoal, deltaT ) )
		ClearFaceTo();
	bool atGoal = IsAtMoveGoal();
	if ( !atGoal || m_path.IsValid() )
	{
		if ( !atGoal && !m_path.IsValid() )
		{
			HostagePathCost cost;
			if ( m_path.Compute( &GetFeet(), &m_moveGoal, cost ) )
			{
				m_follower.SetImprov( this );
				m_follower.SetPath( &m_path );
				m_follower.Reset();
				m_follower.Debug( cv_hostage_debug.value > 0 );
			}
		}
		m_follower.Update( deltaT, m_inhibitObstacleAvoidance.IsElapsed() );
		if ( m_moveType == Stopped )
			m_follower.ResetStuck();
		if ( m_follower.IsStuck() )
			Wiggle();
	}
	m_vel.x = m_vel.x - m_vel.x * 3.0f * deltaT;
	m_vel.y = m_vel.y - m_vel.y * 3.0f * deltaT;
	Vector2D direction = m_vel;
	float speed = direction.NormalizeInPlace();
	if ( speed > 285 )
		speed = 285;
	m_vel = direction * speed;
	KeepPersonalSpace spacer;
	spacer.m_improv = this;
	spacer.m_velDir = Vector( m_actualVel.x, m_actualVel.y, 0 );
	spacer.m_speed = spacer.m_velDir.NormalizeInPlace();
	ForEachPlayer( spacer );
	if ( g_pHostages )
	{
		for ( int i = 0; i < g_pHostages->m_hostageCount; ++i )
		{
			CHostage *hostage = g_pHostages->m_hostage[i];
			if ( hostage->pev->deadflag != DEAD_DEAD )
				spacer( hostage );
		}
	}
	m_hostage->pev->velocity.x = m_vel.x;
	m_hostage->pev->velocity.y = m_vel.y;
	m_moveFlags = 0;
}

void CHostageImprov::UpdateGrenadeReactions()
{
	if ( m_coughTimer.IsElapsed() && TheBots->IsInsideSmokeCloud( &GetCentroid() ) )
	{
		m_coughTimer.Start( RANDOM_FLOAT( 1, 3 ) );
		Chatter( HOSTAGE_CHATTER_COUGH, true );
		Frighten( SCARED );
	}
	if ( !m_grenadeTimer.IsElapsed() )
		return;
	m_grenadeTimer.Start( RANDOM_FLOAT( 0.4f, 0.6f ) );
	CBaseEntity *entity = NULL;
	while ( (entity = UTIL_FindEntityInSphere( entity, GetCentroid(), 500 )) != NULL )
	{
		if ( !FStrEq( STRING( entity->pev->classname ), "grenade" ) || ((CGrenade *)entity)->m_iBounceCount > 1 ||
			!IsVisible( entity->Center() ) )
			continue;
		Chatter( HOSTAGE_CHATTER_SAW_HE_GRENADE, true );
		if ( entity->pev->dmg > 50 )
		{
			m_idleState.m_fleeTimer.Invalidate();
			m_idleState.m_mustFlee = true;
			Frighten( TERRIFIED );
		}
		else
			Frighten( SCARED );
		m_grenadeTimer.Start( 10 );
		return;
	}
}

void CHostageImprov::OnUpdate( float )
{
	if ( !IsAlive() || cv_hostage_stop.value > 0 )
		return;
	if ( m_blinkTimer.IsElapsed() )
	{
		if ( IsScared() )
		{
			m_blinkTimer.Start( RANDOM_FLOAT( 0.5f, 2 ) );
			m_blinkCounter = (char)RANDOM_LONG( 1, 2 );
		}
		else
		{
			m_blinkTimer.Start( RANDOM_FLOAT( 3, 10 ) );
			m_blinkCounter = (char)RANDOM_LONG( 2, 4 );
		}
	}
	if ( m_blinkCounter )
	{
		m_hostage->pev->body = 1;
		--m_blinkCounter;
	}
	else
		m_hostage->pev->body = 0;
	UpdateGrenadeReactions();
	UpdateDelayedChatter();
	UpdateVision();
	m_behavior.Update();
	m_actualVel.x = m_hostage->pev->origin.x - m_lastPosition.x;
	m_actualVel.y = m_hostage->pev->origin.y - m_lastPosition.y;
	float speedSquared = m_actualVel.LengthSquared();
	if ( !IsOnGround() )
	{
		if ( m_hostage->pev->velocity.z < -1000 && m_animateState.GetPerformance() != HostageAnimateState::Fall )
		{
			m_animateState.Reset();
			m_animateState.SetPerformance( HostageAnimateState::Fall );
			m_animateState.AddSequence( this, ACT_FALL, 99.9f, 1 );
		}
	}
	else if ( IsCrouching() )
	{
		if ( speedSquared <= 9 )
		{
			if ( m_animateState.GetPerformance() != HostageAnimateState::Crouch )
			{
				m_animateState.Reset();
				m_animateState.SetPerformance( HostageAnimateState::Crouch );
				if ( m_scaredTimer.IsElapsed() )
					m_animateState.AddSequence( this, ACT_CROUCH_IDLE, 99.9f, 1 );
				else
					m_animateState.AddSequence( this, ACT_CROUCH_IDLE_SCARED, -1, 1 );
			}
		}
		else if ( m_animateState.GetPerformance() != HostageAnimateState::CrouchWalk )
		{
			m_animateState.Reset();
			m_animateState.SetPerformance( HostageAnimateState::CrouchWalk );
			ClearLookAt();
			m_animateState.AddSequence( this, IsScared() ? ACT_CROUCH_WALK_SCARED : ACT_CROUCH_WALK, 99.9f, 2 );
		}
	}
	else
	{
		UTIL_MakeVectors( m_hostage->pev->angles );
		if ( gpGlobals->v_forward.x * m_actualVel.x + gpGlobals->v_forward.y * m_actualVel.y < -3 )
		{
			if ( m_animateState.GetPerformance() != HostageAnimateState::Walk )
			{
				m_animateState.Reset();
				m_animateState.SetPerformance( HostageAnimateState::Walk );
				ClearLookAt();
				m_animateState.AddSequence( this, ACT_WALK_BACK, 99.9f, speedSquared <= 289 ? 1.0f : 2.0f );
			}
		}
		else if ( speedSquared > 289 )
		{
			if ( m_animateState.GetPerformance() != HostageAnimateState::Run )
			{
				m_animateState.Reset();
				m_animateState.SetPerformance( HostageAnimateState::Run );
				ClearLookAt();
				m_animateState.AddSequence( this, IsScared() || IsEscaping() ? ACT_RUN_SCARED : ACT_RUN, 99.9f, 2 );
			}
		}
		else if ( speedSquared > 9 )
		{
			if ( m_animateState.GetPerformance() != HostageAnimateState::Walk )
			{
				m_animateState.Reset();
				m_animateState.SetPerformance( HostageAnimateState::Walk );
				ClearLookAt();
				Activity activity = IsEscaping() ? ACT_WALK_SNEAKY : (IsScared() ? ACT_WALK_SCARED : ACT_WALK);
				m_animateState.AddSequence( this, activity, 99.9f, 1.5f );
			}
		}
		else
		{
			if ( m_animateState.GetPerformance() == HostageAnimateState::Walk ||
				m_animateState.GetPerformance() == HostageAnimateState::Run )
				m_animateState.Reset();
			UpdateStationaryAnimation();
		}
	}
	if ( m_collisionTimer.GetElapsedTime() > 0.4f )
		SetKnownGoodPosition( m_lastPosition );
	m_lastPosition = m_hostage->pev->origin;
	m_animateState.OnUpdate( this );
}

void CHostageImprov::OnGameEvent( GameEventType event, CBaseEntity *entity, CBaseEntity *other )
{
	if ( event == EVENT_PLAYER_DIED || event == EVENT_HOSTAGE_KILLED )
	{
		if ( IsVisible( entity->pev->origin, true ) )
		{
			Frighten( TERRIFIED );
			if ( !entity->IsPlayer() || (entity->IsPlayer() && ((CBasePlayer *)entity)->m_iTeam != TERRORIST) )
				DelayedChatter( RANDOM_FLOAT( 0.5f, 0.7f ), HOSTAGE_CHATTER_SCARED_OF_MURDER, true );
			if ( !entity->IsPlayer() )
			{
				m_idleState.m_fleeTimer.Invalidate();
				m_idleState.m_mustFlee = true;
			}
		}
	}
	else if ( event == EVENT_TERRORISTS_WIN )
	{
		Frighten( SCARED );
		m_isDelayedChatterPending = false;
		DelayedChatter( RANDOM_FLOAT( 1, 4 ), HOSTAGE_CHATTER_TERRORISTS_WIN, false );
		return;
	}
	else if ( event == EVENT_CTS_WIN )
	{
		m_scaredTimer.Invalidate();
		m_isDelayedChatterPending = false;
		DelayedChatter( RANDOM_FLOAT( 1, 4 ), HOSTAGE_CHATTER_CTS_WIN, false );
		return;
	}
	else if ( event == EVENT_BULLET_IMPACT )
	{
		if ( (GetCentroid() - *(const Vector *)other).LengthSquared() < 10000 )
			Frighten( TERRIFIED );
		if ( !entity )
			return;
	}
	else if ( event == EVENT_HOSTAGE_RESCUED )
	{
		if ( !entity )
			return;
		if ( other == m_hostage )
			Chatter( HOSTAGE_CHATTER_RESCUED, false );
	}
	if ( entity )
	{
		float range;
		PriorityType priority;
		bool hostile;
		if ( IsGameEventAudible( event, entity, other, &range, &priority, &hostile ) &&
			(m_hostage->pev->origin - entity->pev->origin).LengthSquared() < (0.4f * range) * (0.4f * range) )
		{
			m_lastNoiseTimer.Start();
			if ( hostile )
			{
				Frighten( SCARED );
				switch ( event )
				{
				case EVENT_HE_GRENADE_EXPLODED:
				case EVENT_SMOKE_GRENADE_EXPLODED:
				case EVENT_BREAK_GLASS:
				case EVENT_BREAK_WOOD:
				case EVENT_BREAK_METAL:
				case EVENT_BREAK_FLESH:
				case EVENT_BREAK_CONCRETE:
					DelayedChatter( RANDOM_FLOAT( 0.3f, 1 ), HOSTAGE_CHATTER_LOOK_OUT, false );
					break;
				case EVENT_WEAPON_FIRED:
					DelayedChatter( RANDOM_FLOAT( 0.3f, 1 ), HOSTAGE_CHATTER_SCARED_OF_GUNFIRE, false );
					break;
				default: break;
				}
			}
		}
	}
	if ( event == EVENT_FLASHBANG_GRENADE_EXPLODED &&
		(GetEyes() - *(const Vector *)other).LengthSquared() < 1000000 && IsVisible( *(const Vector *)other ) )
	{
		DelayedChatter( RANDOM_FLOAT( 0, 1 ), HOSTAGE_CHATTER_BLINDED, true );
		Frighten( TERRIFIED );
	}
}
