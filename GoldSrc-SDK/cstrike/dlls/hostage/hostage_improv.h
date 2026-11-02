#ifndef HOSTAGE_IMPROV_H
#define HOSTAGE_IMPROV_H

#include "hostage.h"
#include "hostage_states.h"

class CHostageImprov : public CImprov
{
public:
	CHostageImprov( CBaseEntity *entity );
	~CHostageImprov() override;
	bool IsAlive() const override;
	void MoveTo( const Vector &goal ) override;
	void LookAt( const Vector &target ) override;
	void ClearLookAt() override;
	void FaceTo( const Vector &goal ) override;
	void ClearFaceTo() override;
	void FaceOutwards();
	bool IsAtMoveGoal( float error = 20.0f ) const override;
	bool HasLookAt() const override;
	bool HasFaceTo() const override;
	bool IsAtFaceGoal() const override;
	bool IsFriendInTheWay( const Vector &goal ) const override;
	bool IsFriendInTheWay( CBaseEntity *other, const Vector &goal ) const override;
	bool IsFriendInTheWay() const;
	void MoveForward() override;
	void MoveBackward() override;
	void StrafeLeft() override;
	void StrafeRight() override;
	bool Jump() override;
	void Crouch() override;
	void StandUp() override;
	void TrackPath( const Vector &goal, float deltaT ) override;
	void SetKnownGoodPosition( const Vector &position );
	const Vector &GetKnownGoodPosition() const { return m_knownGoodPos; }
	void ResetToKnownGoodPosition();
	void StartLadder( const CNavLadder *ladder, NavTraverseType how,
		const Vector *approach, const Vector *depart ) override;
	bool TraverseLadder( const CNavLadder *ladder, NavTraverseType how,
		const Vector *approach, const Vector *depart, float deltaT ) override;
	void Run() override;
	void Walk() override;
	void Stop() override;
	float GetMoveAngle() const override;
	float GetFaceAngle() const override;
	const Vector &GetFeet() const override;
	const Vector &GetCentroid() const override;
	const Vector &GetEyes() const override;
	bool IsRunning() const override;
	bool IsWalking() const override;
	bool IsStopped() const override;
	bool IsCrouching() const override;
	bool IsJumping() const override;
	bool IsUsingLadder() const override;
	bool IsOnGround() const override;
	bool IsMoving() const override;
	bool CanRun() const override;
	bool CanCrouch() const override;
	bool CanJump() const override;
	bool IsVisible( const Vector &position, bool testFOV = false ) const override;
	bool IsPlayerLookingAtMe( CBasePlayer *player, float cosTolerance = 0.95f ) const override;
	CBasePlayer *IsAnyPlayerLookingAtMe( int team = 0, float cosTolerance = 0.95f ) const override;
	CBasePlayer *GetClosestPlayerByTravelDistance( int team = 0, float *range = NULL ) const override;
	CNavArea *GetLastKnownArea() const override;
	void OnUpdate( float deltaT ) override;
	void OnUpkeep( float deltaT ) override;
	void OnReset() override;
	void OnMoveToSuccess( const Vector &goal ) override;
	void OnMoveToFailure( const Vector &goal, MoveToFailureType reason ) override;
	void OnInjury( float amount ) override;
	void OnGameEvent( GameEventType event, CBaseEntity *entity, CBaseEntity *other ) override;
	void OnTouch( CBaseEntity *other ) override;
	void ResetJump() { m_hasJumped = false; }
	void ApplyForce( Vector force );
	const Vector &GetActualVelocity() const { return m_actualVel; }
	enum MoveType { Stopped, Walking, Running };
	void SetMoveLimit( MoveType limit ) { m_moveLimit = limit; }
	MoveType GetMoveLimit() const { return m_moveLimit; }
	CNavPath *GetPath() { return &m_path; }
	CNavPathFollower *GetPathFollower() { return &m_follower; }
	void Idle() { m_behavior.SetState( &m_idleState ); }
	bool IsIdle() const { return m_behavior.IsState( &m_idleState ); }
	void Follow( CBaseEntity *leader )
	{
		m_followState.SetLeader( leader );
		m_behavior.SetState( &m_followState );
	}
	bool IsFollowing() const { return m_behavior.IsState( &m_followState ); }
	void Escape() { m_behavior.SetState( &m_escapeState ); }
	bool IsEscaping() const { return m_behavior.IsState( &m_escapeState ); }
	void Retreat() { m_behavior.SetState( &m_retreatState ); }
	bool IsRetreating() const { return m_behavior.IsState( &m_retreatState ); }
	bool IsAtHome() const;
	bool CanSeeRescueZone() const;
	CBaseEntity *GetFollowLeader() const { return m_followState.GetLeader(); }
	CBasePlayer *GetClosestVisiblePlayer( int team );
	float GetTimeSinceLastSawPlayer( int team );
	float GetTimeSinceLastInjury();
	float GetTimeSinceLastNoise();
	bool IsTerroristNearby();
	enum ScareType { NERVOUS, SCARED, TERRIFIED };
	void Frighten( ScareType intensity );
	bool IsScared() const;
	ScareType GetScareIntensity() const { return m_scareIntensity; }
	bool IsIgnoringTerrorists() const { return !m_ignoreTerroristTimer.IsElapsed(); }
	float GetAggression() const { return m_aggression; }
	void Chatter( HostageChatterType type, bool mustSpeak );
	void DelayedChatter( float delay, HostageChatterType type, bool mustSpeak );
	void UpdateDelayedChatter();
	bool IsTalking() const { return !m_talkingTimer.IsElapsed(); }
	CountdownTimer m_coughTimer;
	CountdownTimer m_grenadeTimer;
	void UpdateGrenadeReactions();
	void Afraid();
	void Wave();
	void Agree();
	void Disagree();
	void CrouchDie();
	void Flinch( Activity activity );
	void UpdateIdleActivity( Activity activity, Activity fidget );
	void UpdateStationaryAnimation();
	CHostage *GetEntity() const { return m_hostage; }

private:
	void CheckForNearbyTerrorists();
	void UpdatePosition( float deltaT );
	void MoveTowards( const Vector &position, float deltaT );
	bool FaceTowards( const Vector &target, float deltaT );
	float GetSpeed();
	void SetMoveAngle( float angle ) { m_moveAngle = angle; }
	void Wiggle();
	void ClearPath();
	bool DiscontinuityJump( float ground, bool onlyJumpDown, bool mustJump );
	bool GetSimpleGroundHeightWithFloor( const Vector *position, float *height, Vector *normal ) override;
	void UpdateVision();
	enum { HEAD_YAW, HEAD_PITCH, EYEBROW, SNEER, MOUTH };
	CHostage *m_hostage;
	CNavArea *m_lastKnownArea;
	mutable Vector m_centroid;
	mutable Vector m_eye;
	HostageStateMachine m_behavior;
	HostageIdleState m_idleState;
	HostageEscapeState m_escapeState;
	HostageRetreatState m_retreatState;
	HostageFollowState m_followState;
	HostageAnimateState m_animateState;
	bool m_didFidget;
	float m_aggression;
	IntervalTimer m_lastSawCT;
	IntervalTimer m_lastSawT;
	CountdownTimer m_checkNearbyTerroristTimer;
	bool m_isTerroristNearby;
	CountdownTimer m_nearbyTerroristTimer;
	CountdownTimer m_scaredTimer;
	ScareType m_scareIntensity;
	CountdownTimer m_ignoreTerroristTimer;
	CountdownTimer m_blinkTimer;
	char m_blinkCounter;
	IntervalTimer m_lastInjuryTimer;
	IntervalTimer m_lastNoiseTimer;
	mutable CountdownTimer m_avoidFriendTimer;
	mutable bool m_isFriendInTheWay;
	CountdownTimer m_chatterTimer;
	bool m_isDelayedChatterPending;
	CountdownTimer m_delayedChatterTimer;
	HostageChatterType m_delayedChatterType;
	bool m_delayedChatterMustSpeak;
	CountdownTimer m_talkingTimer;
	unsigned int m_moveFlags;
	Vector2D m_vel;
	Vector m_actualVel;
	Vector m_moveGoal;
	Vector m_knownGoodPos;
	bool m_hasKnownGoodPos;
	Vector m_priorKnownGoodPos;
	bool m_hasPriorKnownGoodPos;
	CountdownTimer m_priorKnownGoodPosTimer;
	IntervalTimer m_collisionTimer;
	Vector m_viewGoal;
	bool m_isLookingAt;
	Vector m_faceGoal;
	bool m_isFacingTo;
	CNavPath m_path;
	CNavPathFollower m_follower;
	Vector m_lastPosition;
	MoveType m_moveType;
	MoveType m_moveLimit;
	bool m_isCrouching;
	CountdownTimer m_minCrouchTimer;
	float m_moveAngle;
	NavRelativeDirType m_wiggleDirection;
	CountdownTimer m_wiggleTimer;
	CountdownTimer m_wiggleJumpTimer;
	CountdownTimer m_inhibitObstacleAvoidance;
	CountdownTimer m_jumpTimer;
	bool m_hasJumped;
	bool m_hasJumpedIntoAir;
	Vector m_jumpTarget;
	CountdownTimer m_clearPathTimer;
	bool m_traversingLadder;
	EHANDLE m_visiblePlayer[32];
	int m_visiblePlayerCount;
	CountdownTimer m_visionTimer;
};

class HostagePathCost
{
public:
	float operator()( CNavArea *area, CNavArea *fromArea, const CNavLadder *ladder )
	{
		if ( !fromArea )
			return 0;
		if ( ladder )
			return 10.0f * ladder->m_length + fromArea->GetCostSoFar();
		float distance = (*area->GetCenter() - *fromArea->GetCenter()).Length();
		float cost = distance + fromArea->GetCostSoFar();
		if ( area->GetAttributes() & NAV_CROUCH )
			cost += 10.0f * distance;
		if ( area->GetAttributes() & NAV_JUMP )
			cost += 10.0f * distance;
		return cost;
	}
};

#endif
