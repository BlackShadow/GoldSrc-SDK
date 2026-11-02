#ifndef CS_BOT_H
#define CS_BOT_H

#include "bot.h"
#include "nav_area.h"
#include "nav_node.h"
#include "cs_bot_chatter.h"

class CCSBot;
class CHostage;

const Vector* FindNearbyRetreatSpot( CCSBot* bot, float maxRange );
int GetBotFollowCount( CBasePlayer* leader );

class BotState
{
public:
	virtual void OnEnter( CCSBot* bot );
	virtual void OnUpdate( CCSBot* bot );
	virtual void OnExit( CCSBot* bot );
	virtual const char* GetName() const = 0;
};

class IdleState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	const char* GetName() const override;
};

class HuntState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;

private:
	friend class CCSBot;
	CNavArea* m_huntArea;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class InvestigateNoiseState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
	void AttendCurrentNoise( CCSBot* bot );

private:
	Vector m_noisePosition;
};

class FetchBombState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	const char* GetName() const override;
};

class PlantBombState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
};

class DefuseBombState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
};

class EscapeFromBombState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
};

class UseEntityState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
	void SetEntity( CBaseEntity* entity ) { m_entity = entity; }

private:
	EHANDLE m_entity;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class MoveToState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;

private:
	friend class CCSBot;
	Vector m_goal;
	RouteType m_route;
	bool m_announcedGoingToPlant;
	bool m_announcedPlanting;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class AttackState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
	void StopAttacking( CCSBot* bot );

private:
	friend class CCSBot;
	int m_dodgeState;
	float m_nextDodgeTime;
	CountdownTimer m_repathTimer;
	float m_scopeTimestamp;
	bool m_haveSeenEnemy;
	bool m_lostEnemy;
	float m_reacquireFireTime;
	float m_shieldToggleTime;
	bool m_shieldAttackMode;
	float m_nextRetreatTime;
	bool m_crouchAndHold;
	bool m_checkedRetreatWhenLost;
	bool m_shouldDodge;
	bool m_firstDodge;
	bool m_retreatWhenOutnumbered;
	CountdownTimer m_retreatTimer;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class BuyState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;

private:
	bool m_initialDelay;
	int m_preferredBuyRetries;
	int m_preferredWeaponIndex;
	int m_randomBuyRetries;
	bool m_done;
	bool m_buyDefuser;
	bool m_buyGrenade;
	bool m_buyShield;
	bool m_buyPistol;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class HideState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;

private:
	friend class CCSBot;
	CNavArea* m_searchArea;
	float m_maxRange;
	Vector m_hidingSpot;
	bool m_atSpot;
	float m_duration;
	bool m_holdPosition;
	float m_holdTime;
	bool m_heardEnemy;
	float m_holdTimestamp;
	int m_spotRetries;
	Vector m_leaderPosition;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class FollowState : public BotState
{
public:
	void OnEnter( CCSBot* bot ) override;
	void OnUpdate( CCSBot* bot ) override;
	void OnExit( CCSBot* bot ) override;
	const char* GetName() const override;
	void ComputeLeaderMotionState( float speed );

private:
	friend class CCSBot;
	EHANDLE m_leader;
	Vector m_leaderPosition;
	unsigned char m_initialFollowFlag;
	unsigned int m_initialFollowValue;
	int m_leaderMotion;
	IntervalTimer m_motionChangedTimer;
	bool m_sneaking;
	float m_lastSawLeader;
	CountdownTimer m_repathTimer;
	IntervalTimer m_walkHysteresisTimer;
	bool m_hysteresisStarted;
	float m_nextStopHideDelay;
	CountdownTimer m_boredTimer;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class PathCost
{
public:
	PathCost( CCSBot* bot, RouteType route ) : m_bot( bot ), m_route( route ) {}
	float operator()( CNavArea* area, CNavArea* fromArea, const CNavLadder* ladder );

private:
	CCSBot* m_bot;
	RouteType m_route;
};

class CSGameState
{
public:
	enum BombState
	{
		BOMB_CARRIED,
		BOMB_LOOSE,
		BOMB_PLANTED,
		BOMB_DEFUSED,
		BOMB_EXPLODED
	};

	explicit CSGameState( CCSBot* owner );
	void Reset();
	void OnEvent( GameEventType event, CBaseEntity* entity, CBaseEntity* other );
	bool IsRoundOver() const;
	BombState GetBombState() const { return m_bombState; }
	void SetHostageTaken( bool taken ) { m_isHostageTaken = taken; }
	void SetBombState( BombState state );
	void UpdateBomber( const Vector* position );
	void UpdateLooseBomb( const Vector* position );
	void UpdatePlantedBomb( const Vector* position );
	float TimeSinceLastSawBomber() const;
	float TimeSinceLastSawLooseBomb() const;
	bool IsLooseBombLocationKnown() const;
	bool IsPlantedBombLocationKnown() const;
	const Vector* GetBombPosition() const;
	int GetPlantedBombsite() const;
	bool IsAtPlantedBombsite() const;
	void MarkBombsiteAsPlanted( int bombsite );
	void ClearBombsite( int bombsite );
	bool IsBombsiteClear( int bombsite ) const;
	int GetNextBombsiteToSearch();
	void InitializeHostageInfo();
	unsigned int ValidateHostagePositions();
	void AllHostagesGone();
	bool AreAllHostagesGone() const;
	bool AreAllHostagesBeingRescued() const;
	CHostage* GetNearestFreeHostage( Vector* position ) const;
	CHostage* GetNearestVisibleFreeHostage() const;
	const Vector* GetRandomFreeHostagePosition() const;

private:
	friend class IdleState;
	struct HostageInfo
	{
		CHostage* hostage;
		Vector knownPosition;
		bool isAtKnownPosition;
		bool isAlive;
		bool isValid;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	};

	CCSBot* m_owner;
	bool m_isRoundOver;
	BombState m_bombState;
	IntervalTimer m_lastSawBomber;
	Vector m_bomberPosition;
	IntervalTimer m_lastSawLooseBomb;
	Vector m_looseBombPosition;
	bool m_isBombsiteClear[4];
	int m_bombsiteSearchOrder[4];
	int m_bombsiteCount;
	int m_bombsiteSearchIndex;
	int m_plantedBombsite;
	bool m_isPlantedBombPositionKnown;
	Vector m_plantedBombPosition;
	HostageInfo m_hostage[12];
	int m_hostageCount;
	CountdownTimer m_validateHostagePositionsTimer;
	bool m_areAllHostagesRescued;
	bool m_isHostageTaken;
};

class CCSBot : public CBot
{
public:
	enum TaskType
	{
		TASK_SEEK_AND_DESTROY = 0,
		TASK_PLANT_BOMB = 1,
		TASK_FIND_TICKING_BOMB = 2,
		TASK_DEFUSE_BOMB = 3,
		TASK_GUARD_TICKING_BOMB = 4,
		TASK_GUARD_BOMB_DEFUSER = 5,
		TASK_GUARD_LOOSE_BOMB = 6,
		TASK_FETCH_BOMB = 7,
		TASK_ESCAPE_FROM_BOMB = 8,
		TASK_HOLD_POSITION = 9,
		TASK_FOLLOW = 10,
		TASK_VIP_ESCAPE = 11,
		TASK_GUARD_VIP_ESCAPE_ZONE = 12,
		TASK_COLLECT_HOSTAGES = 13,
		TASK_RESCUE_HOSTAGES = 14,
		TASK_GUARD_HOSTAGES = 15,
		TASK_GUARD_HOSTAGE_RESCUE_ZONE = 16,
		TASK_CHASE_ENEMY = 17,
		TASK_MOVE_TO_SNIPER_SPOT = 18,
		TASK_SNIPING = 19
	};

	enum DispositionType
	{
		ENGAGE_AND_INVESTIGATE,
		OPPORTUNITY_FIRE,
		SELF_DEFENSE,
		IGNORE_ENEMIES
	};

	CCSBot();
	bool Initialize( const BotProfile* profile ) override;
	void SpawnBot() override;
	void Upkeep() override;
	void Update() override;
	void Walk() override;
	bool Jump( bool mustJump = false ) override;
	int TakeDamage( entvars_t* inflictor, entvars_t* attacker, float damage, int damageType ) override;
	void Killed( entvars_t* attacker, int gib ) override;
	void RoundRespawn() override;
	void Blind( float duration, float holdTime, float fadeTime, int alpha ) override;
	void OnTouchingWeapon( CWeaponBox* box ) override;
	void OnEvent( GameEventType event, CBaseEntity* entity = NULL, CBaseEntity* other = NULL ) override;
	bool IsVisible( const Vector* position, bool testFOV = false ) const override;
	bool IsVisible( CBasePlayer* player, bool testFOV = false, unsigned char* visibleParts = NULL ) const override;
	bool IsEnemyPartVisible( VisiblePartType part ) const override;

	void ResetValues();
	void Disconnect();

private:
	void EXPORT BotTouch( CBaseEntity* entity );

public:
	void BotDeathThink();
	CBasePlayer* FindNearbyPlayer();
	void SetEnemy( CBasePlayer* enemy );
	void Panic( CBasePlayer* enemy );
	CBasePlayer* GetAttacker() const;
	bool IsDoingScenario() const;
	bool NoticeLooseBomb() const;
	bool CanSeeLooseBomb() const;
	bool CanSeePlantedBomb() const;
	void GetOffLadder();
	float GetHidingSpotCheckTimestamp( HidingSpot* spot ) const;
	void SetHidingSpotCheckTimestamp( HidingSpot* spot );
	void UpdateHostageEscortCount();
	bool IsOutnumbered() const;
	int OutnumberedCount() const;
	CBasePlayer* GetImportantEnemy( bool checkVisibility ) const;
	void SetDisposition( DispositionType disposition );
	DispositionType GetDisposition() const;
	void IgnoreEnemies( float duration );
	void IncreaseMorale();
	void DecreaseMorale();
	bool IsRogue() const;
	bool IsHurrying() const;
	bool IsSafe() const;
	bool IsWellPastSafe() const;
	bool IsEndOfSafeTime() const;
	float GetSafeTimeRemaining() const;
	void AdjustSafeTime();
	bool HasNotSeenEnemyForLongTime() const;
	bool GuardRandomZone( float range );
	float GetRangeToFarthestEscortedHostage() const;

	void SetState( BotState* state );
	void StopAttacking();
	void Attack( CBasePlayer* enemy );
	void Idle();
	void Hunt();
	void Follow( CBasePlayer* leader );
	void ContinueFollowing();
	void StopFollowing();
	void Hide( CNavArea* area, float duration = -1.0f, float range = 750.0f, bool holdPosition = false );
	void Hide( const Vector* position, float duration = -1.0f, bool holdPosition = false );
	bool TryToHide( CNavArea* area = NULL, float duration = -1.0f, float range = 750.0f, bool holdPosition = false, bool useNearest = false );
	bool TryToRetreat();
	void MoveTo( const Vector* position, RouteType route = SAFEST_ROUTE );
	void UseEntity( CBaseEntity* entity );
	void FetchBomb();
	void PlantBomb();
	void DefuseBomb();
	void EscapeFromBomb();
	void InvestigateNoise();
	void RescueHostages();
	bool IsAttacking() const;
	bool IsEscapingFromBomb() const;
	bool IsDefusingBomb() const;
	bool IsHiding() const;
	bool IsAtHidingSpot() const;
	bool IsHunting() const;
	bool IsMovingTo() const;
	bool IsBuying() const;
	bool IsBusy() const;
	Place GetPlace() const;

	bool ComputePath( CNavArea* area, const Vector* position, RouteType route );
	void BuildTrivialPath( const Vector* position );
	bool ComputePathPositions();
	void SetPathIndex( int index );
	float GetPathDistanceRemaining() const;
	int UpdatePathMovement( bool allowSpeedChange = true );
	bool IsNearJump() const;
	bool IsStraightLinePathWalkable( const Vector* position ) const;
	int FindClosestPointOnPath( const Vector* position, int first, int last, Vector* closest ) const;
	int FindOurPositionOnPath( Vector* position, bool local = false ) const;
	int FindPathPoint( float distance, Vector* position, int* previousIndex );
	void DrawPath();
	void ComputeApproachPoints();
	void DrawApproachPoints();
	bool FindApproachPointNearestPath( Vector* position );
	void SetupLadderMovement();
	bool UpdateLadderMovement();
	void ComputeLadderEndpoint( bool descending );
	void FeelerReflexAdjustment( Vector* goal );
	void MoveTowardsPosition( const Vector* position );
	void MoveAwayFromPosition( const Vector* position );
	void StrafeAwayFromPosition( const Vector* position );
	bool DiscontinuityJump( float distance, bool mustJump, bool onlyJumpDown );
	bool GetSimpleGroundHeightWithFloor( const Vector* position, float* height, Vector* normal );
	float GetApproximateFallDamage( float height ) const;
	bool IsFriendInTheWay( const Vector* goal ) const;
	void ResetStuckMonitor();
	void StuckCheck();
	void Wiggle();
	bool StayOnNavMesh();

	void UpdateLookAngles();
	void UpdateLookAt();
	void UpdateLookAround( bool updateApproachPoints = true );
	bool UpdateLookAtNoise();
	void UpdatePeripheralVision();
	void SetLookAt( const char* description, const Vector* position, PriorityType priority, float duration, bool clearIfClose = false, float tolerance = 5.0f );
	void InhibitLookAround( float duration );
	bool BendLineOfSight( const Vector* eye, const Vector* target, Vector* visible ) const;
	bool CanSeeNoisePosition() const;
	bool CanHearNearbyEnemyGunfire( float range ) const;
	bool ShouldInvestigateNoise( float* range );
	CBasePlayer* FindMostDangerousThreat();
	float ComputeWeaponSightRange();
	void UpdateReactionQueue();
	CBasePlayer* GetRecognizedEnemy();
	bool IsRecognizedEnemyReloading();
	bool IsRecognizedEnemyProtectedByShield();
	float GetRangeToNearestRecognizedEnemy();
	void SetAimOffset( float accuracy );
	void UpdateAimOffset();
	bool AdjustZoom( float range );
	bool IsUsingAWP() const;
	bool DoesActiveWeaponHaveSilencer() const;
	bool IsUsingSniperRifle() const;
	bool IsSniper() const;
	bool IsSniping() const;
	bool IsUsingShotgun() const;
	bool IsUsingMachinegun() const;
	bool IsPrimaryWeaponEmpty() const;
	bool IsPistolEmpty() const;
	bool IsUsingKnife() const;
	bool IsUsingPistol() const;
	bool IsUsingGrenade() const;
	bool IsUsingHEGrenade() const;
	bool HasGrenade() const;
	bool DoEquip( CBasePlayerWeapon* weapon );
	void EquipKnife();
	void EquipPistol();
	void EquipBestWeapon( bool force = false );
	bool EquipGrenade( bool avoidSmoke = false );
	void ThrowGrenade( const Vector* target );
	bool FindGrenadeTossPathTarget( Vector* target );
	void ReloadCheck();
	void SilencerCheck();
	bool IsFriendInLineOfFire();
	void FireWeaponAtEnemy();

	void StartVoiceFeedback( float duration );
	void EndVoiceFeedback( bool force = false );
	bool IsUsingVoice() const { return m_voiceEndTime != 0.0f; }
	void SendRadioMessage( GameEventType event );
	bool IsRadioCommand( GameEventType event ) const;
	void RespondToRadioCommands();
	bool RespondToHelpRequest( CBasePlayer* player, Place place, float range );
	void StartNormalProcess();
	void StartLearnProcess();
	void UpdateLearnProcess();
	void StartAnalyzeAlphaProcess();
	void UpdateAnalyzeAlphaProcess();
	void StartAnalyzeBetaProcess();
	void UpdateAnalyzeBetaProcess();
	void StartSaveProcess();
	void UpdateSaveProcess();
	bool LearnStep();
	CNavNode* AddNode( const Vector* position, const Vector* normal, NavDirType direction, CNavNode* parent );

	CSGameState* GetGameState() { return &m_gameState; }
	const CSGameState* GetGameState() const { return &m_gameState; }
	BotChatterInterface* GetChatter() { return &m_chatter; }
	int GetNearbyEnemyCount() const { return m_nearbyEnemyCount; }
	int GetNearbyFriendCount() const { return m_nearbyFriendCount; }
	Place GetLastEnemyPlace() const { return m_enemyPlace; }
	TaskType GetTask() const { return m_task; }
	void SetTask( TaskType task ) { m_task = task; }
	void SetTaskEntity( CBaseEntity* entity ) { m_taskEntity = entity; }
	CNavArea* GetLastKnownArea() const { return m_lastKnownArea; }
	float GetEnemyLastSeenTimestamp() const { return m_lastSawEnemyTime; }
	float GetLastEnemyDeathTimestamp() const { return m_enemyDeathTimestamp; }
	int GetHostageEscortCount() const { return m_hostageEscortCount; }

private:
	friend class IdleState;
	friend class CCSBotManager;
	friend class HuntState;
	friend class AttackState;
	friend class InvestigateNoiseState;
	friend class BuyState;
	friend class MoveToState;
	friend class FetchBombState;
	friend class PlantBombState;
	friend class DefuseBombState;
	friend class HideState;
	friend class EscapeFromBombState;
	friend class FollowState;
	friend class UseEntityState;
	friend class PathCost;
	friend int GetBotFollowCount( CBasePlayer* leader );

	struct PathSegment
	{
		CNavArea* area;
		NavTraverseType how;
		Vector position;
		const CNavLadder* ladder;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
	};
	struct HidingSpotCheck
	{
		HidingSpot* spot;
		float timestamp;
	};
	struct PlayerVisibility
	{
		float timestamp;
		bool isEnemy;
	};
	struct Reaction
	{
		EHANDLE enemy;
		bool reloading;
		bool shield;
	};

	char m_name[64];
	float m_combatRange;
	mutable bool m_isRogue;
	mutable CountdownTimer m_rogueTimer;
	int m_morale;
	bool m_diedLastRound;
	float m_safeTime;
	bool m_wasSafe;
	int m_blindMoveDirection;
	bool m_blindFire;
	float m_surpriseDelay;
	float m_lastPanicTime;
	bool m_isFollowing;
	EHANDLE m_leader;
	float m_followStartTime;
	float m_allowAutoFollowTime;
	CountdownTimer m_inhibitFollowingTimer;
	IdleState m_idleState;
	HuntState m_huntState;
	AttackState m_attackState;
	InvestigateNoiseState m_investigateNoiseState;
	BuyState m_buyState;
	MoveToState m_moveToState;
	FetchBombState m_fetchBombState;
	PlantBombState m_plantBombState;
	DefuseBombState m_defuseBombState;
	HideState m_hideState;
	EscapeFromBombState m_escapeFromBombState;
	FollowState m_followState;
	UseEntityState m_useEntityState;
	BotState* m_state;
	float m_stateTimestamp;
	bool m_isAttacking;
	TaskType m_task;
	EHANDLE m_taskEntity;
	Vector m_goalPosition;
	EHANDLE m_avoidEntity;
	CNavArea* m_area;
	CNavArea* m_lastKnownArea;
	EHANDLE m_nearbyPlayer;
	float m_nearbyPlayerTimestamp;
	bool m_isJumpCrouching;
	bool m_hasJumpCrouched;
	float m_jumpCrouchTimestamp;
	PathSegment m_path[256];
	int m_pathLength;
	int m_pathIndex;
	float m_areaEnteredTimestamp;
	CountdownTimer m_repathTimer;
	mutable CountdownTimer m_friendCheckTimer;
	mutable bool m_isFriendInTheWay;
	CountdownTimer m_friendAvoidanceTimer;
	bool m_friendAvoidanceFlag;
	int m_ladderState;
	bool m_isAscendingLadder;
	const CNavLadder* m_ladder;
	int m_ladderDismountDirection;
	float m_ladderDismountTimestamp;
	float m_ladderEndHeight;
	float m_ladderStartTimestamp;
	CountdownTimer m_hurryTimer;
	CSGameState m_gameState;
	unsigned char m_hostageEscortCount;
	float m_hostageEscortTimestamp;
	bool m_isWaitingForHostage;
	CountdownTimer m_hostageWaitTimer;
	CountdownTimer m_hostageWaitTimeout;
	Vector m_noisePosition;
	float m_noiseTimestamp;
	CNavArea* m_noiseArea;
	float m_noiseCheckTimestamp;
	PriorityType m_noisePriority;
	bool m_noiseTravelDistanceChecked;
	float m_lastLookAroundTimestamp;
	float m_lookAheadYaw;
	float m_lookAheadMovementYaw;
	float m_inhibitLookAroundTimestamp;
	int m_lookAtState;
	Vector m_lookAtPosition;
	PriorityType m_lookAtPriority;
	float m_lookAtDuration;
	float m_lookAtAcquiredTimestamp;
	float m_lookAtTolerance;
	bool m_lookAtClearIfClose;
	const char* m_lookAtDescription;
	float m_peripheralVisionTimestamp;
	Vector m_approachPoint[16];
	unsigned char m_approachPointCount;
	Vector m_lastOrigin;
	bool m_isWaitingToThrowGrenade;
	CountdownTimer m_grenadeTossTimer;
	SpotEncounter* m_spotEncounter;
	float m_spotEncounterTimestamp;
	HidingSpotCheck m_hidingSpotCheck[64];
	int m_hidingSpotCheckCount;
	float m_desiredPitch;
	float m_pitchAngularVelocity;
	float m_desiredYaw;
	float m_yawAngularVelocity;
	mutable Vector m_eyePosition;
	Vector m_aimOffset;
	Vector m_targetAimOffset;
	float m_nextAimOffsetTime;
	float m_aimAccuracyTimestamp;
	Vector m_aimSpot;
	DispositionType m_disposition;
	CountdownTimer m_ignoreEnemiesTimer;
	EHANDLE m_enemy;
	bool m_enemyVisible;
	unsigned char m_visibleEnemyParts;
	Vector m_lastEnemyPosition;
	float m_lastSawEnemyTime;
	float m_firstNearbyEnemyTime;
	float m_enemyChangeTimestamp;
	float m_enemyDeathTimestamp;
	bool m_enemyDead;
	int m_nearbyEnemyCount;
	Place m_enemyPlace;
	PlayerVisibility m_playerVisibility[32];
	EHANDLE m_visibleBomber;
	int m_nearbyFriendCount;
	EHANDLE m_closestFriend;
	EHANDLE m_closestHumanFriend;
	CBasePlayer* m_attacker;
	float m_attackerTimestamp;
	int m_lastVictimIndex;
	bool m_isAimingAtEnemy;
	bool m_isRapidFire;
	IntervalTimer m_lastEquipTime;
	float m_nextFireTime;
	Reaction m_reactionQueue[20];
	unsigned char m_reactionWriteIndex;
	unsigned char m_reactionCount;
	unsigned char m_recognizedIndex;
	bool m_stuck;
	float m_stuckTimestamp;
	Vector m_stuckPosition;
	int m_wiggleDirection;
	float m_nextWiggleTime;
	float m_nextJumpTime;
	float m_velocitySamples[5];
	int m_velocitySampleIndex;
	int m_velocitySampleCount;
	Vector m_lastPosition;
	GameEventType m_lastRadioCommand;
	float m_radioCommandTimestamp;
	float m_lastSentRadioTimestamp;
	EHANDLE m_radioSender;
	Vector m_radioPosition;
	float m_voiceStartTime;
	float m_voiceEndTime;
	BotChatterInterface m_chatter;
	unsigned int m_navigationState;
	CNavNode* m_currentNode;
	NavDirType m_currentLearnDirection;
	NavAreaList::iterator m_currentAnalyzeArea;
	int m_process;
	CountdownTimer m_processTimer[3];
};

#endif
