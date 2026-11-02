#pragma once

class CLocation : public CBaseEntity
{
public:
	void Spawn();
	void RoundRespawn();
	void KeyValue(KeyValueData *pkvd);
	string_t m_sMaster;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CWeatherSystem : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	void EXPORT InitThink();
	void SendForecast(CBasePlayer *pPlayer);
	void SendData();

	int m_iType;
	float m_flDensity;
};

class CParticleShooter : public CBaseDelay
{
public:
	void Spawn();
	void Precache();
	void KeyValue(KeyValueData *pkvd);
	void RoundRespawn();
	void Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	void UpdateJoiningPlayer(CBasePlayer *pPlayer);
	void FindTargetEntity();
	void SendUpdate(CBasePlayer *pPlayer);

	byte m_iGroupId;
	int m_iSpriteIndex;
	Vector m_vShootDir;
	int m_iNumParticles;
	int m_iState;
	float m_flSpeed;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CObject : public CBaseEntity
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	void Materialize();
	void EXPORT ObjectThink();
	void EXPORT ObjectTouch( CBaseEntity* pOther );
	void ResetObject();
	void RoundRespawn();
	void SendPickupMsg( CBasePlayer* pPlayer );
	void SendDroppedMsg( CBasePlayer* pPlayer );
	void EXPORT AbsentPlayerThink();
	void Drop();
	void Disappear();

	Vector		m_vStartOrigin;
	int			m_iTeam;
	int			m_iMain;
	int			m_iResetOnTouch;
	int			m_iResetTime;
	int			m_iOtherPoints;
	int			m_iCapPoints;
	int			ObjectIndex;
	int			m_iObjectHIcon;
	float		m_fCarrySpeed;
	float		m_flNextTouchTime;
	float		m_flReset;
	char		sz_Name[256];
	char		sz_Group[256];
	char		sz_DoneTarget[256];
	char		m_szObjectPIcon[256];
	char		m_szObjectHIcon[256];
	string_t	m_sMaster;
	EHANDLE		m_hOwner;
	BOOL		m_bDropped;
	BOOL		m_bCarried;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CControlPoint : public CBaseAnimating
{
public:
	CControlPoint();

	void SetOwner( int owner, BOOL bMakeSound, CBasePlayer* pPlayer );
	int GetOwner();
	const char* GetName() { return STRING( pev->netname ); }
	char* GetGroup();
	void SetIndex( int i_index );
	int PointValue();
	void Reset();
	void RoundRespawn();
	int GetPointIndex() { return m_iPointIndex; }
	int GetIndex() { return m_iIndex; }
	void SetActive( bool active );
	BOOL PointIsVisible() { return !( pev->spawnflags & 1 ); }
	int GetAlliesIcon() { return m_iAlliesIcon; }
	int GetAxisIcon() { return m_iAxisIcon; }
	int GetNeutralIcon() { return m_iNeutralIcon; }
	int GetPointValue() { return m_iCapPoints; }
	void SendCapString( int team, CBasePlayer* pPlayer );

	void Spawn();
	void Touch( CBaseEntity* pOther );
	void KeyValue( KeyValueData* pkvd );
	void Precache();

	string_t	m_sMaster;
	int			m_iTeam;
	int			m_iResetTime;
	int			m_iMainObjective;
	int			m_iDefaultOwner;
	int			m_iIndex;
	int			m_iPointValue;
	int			m_iCapPoints;
	int			m_iTeamPoints;
	float		m_fCapDelayTime;
	float		m_fNextCapTime;
	char		sz_WinString[256];
	string_t	m_sAlliesCapString;
	string_t	m_sAxisCapString;
	char		sz_AlliesTarget[256];
	char		sz_AxisTarget[256];
	char		sz_ResetTarget[256];
	char		sz_AlliesModel[256];
	char		sz_AxisModel[256];
	char		sz_ResetModel[256];
	int			m_iAlliesModelBodygroup;
	int			m_iAxisModelBodygroup;
	int			m_iResetModelBodygroup;
	int			m_iAlliesIcon;
	int			m_iAxisIcon;
	int			m_iNeutralIcon;
	bool		m_bAlliesCantTouch;
	bool		m_bAxisCantTouch;
	int			m_bPointVisible;
	int			m_iPointIndex;
	char		m_szGroup[256];
	bool		m_bActive;
};

class CControlPointMaster : public CBaseEntity
{
public:
	CControlPointMaster();
	int GetNumPoints() { return m_iNumPoints; }
	int GetPointOwner( int point );
	void Reset();
	void SendObjectiveStatus( CBasePlayer* pPlayer );
	bool IsMasteredOn();
	void RoundRespawn();
	void BecomeActive();
	void BecomeInactive();
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	private:
	void EXPORT CPMThink();
	public:
	int TeamOwnsAllPoints();

	CControlPoint*	ControlPointArray[20];
	int			m_iNumPoints;
	BOOL		FoundPoints;
	BOOL		m_bActive;
	float		m_fGivePointsTime;
	int			m_iGivePointsDelay;
	int			m_iMainObjective;
	char		sz_AlliesTarget[256];
	char		sz_AxisTarget[256];
	char		sz_InterimPointsMessage[256];
	char		sz_AllPointsCapMessage[256];
	string_t	m_sMaster;
	int			m_bMasteredOn;
	bool		m_bTeamHasWon;
	char		m_szGroup[256];
	float		m_flNextSpectatorUpdateTime;
};

class CDodRoundTimer : public CBaseEntity
{
public:
	void Spawn();
	private:
	void EXPORT Think();
	public:
	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void StartTimer();
	void StopTimer();
	void ResetTimer();
	void SendStatus();
	void SendStatus( CBasePlayer* pPlayer );

	float		m_fTimerLength;
	float		m_fRoundTime;
	bool		m_bTimer;
	float		m_fLastThinkTime;
	float		m_fUpdateTimer;
	string_t	m_sMaster;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CDoDCamera : public CBaseDelay
{
public:
	void Spawn();
	void Precache();
	void KeyValue( KeyValueData* pkvd );
	int Save( CSave& save );
	int Restore( CRestore& restore );
	int ObjectCaps();
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void EXPORT FollowTarget();
	void Move();
	void SetCameraMode( const char* Targetname, int fade );
	float ReturnTimeLeft();
	static TYPEDESCRIPTION m_SaveData[];

	EHANDLE &PlayerSlot( int i ) { return ( i < 32 ) ? m_hPlayer[i] : m_hTarget; }

	EHANDLE		m_hPlayer[32];
	EHANDLE		m_hTarget;
	CBaseEntity*	m_pentPath;
	int			m_sPath;
	int			m_teamBound;
	int			m_sTrigrStart;
	int			m_sTrigrEnd;
	int			m_iOverlay;
	int			m_iFOV;
	int			m_iSubtitle;
	int			m_iCamFade;
	float		m_flWait;
	float		m_flReturnTime;
	float		m_flStopTime;
	float		m_moveDistance;
	float		m_targetSpeed;
	float		m_initialSpeed;
	float		m_acceleration;
	float		m_deceleration;
	int			m_state;
	Vector		m_angles;
	Vector		m_origin;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};
