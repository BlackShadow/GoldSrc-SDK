#pragma once

#define CTF_FLAG_BM		1
#define CTF_FLAG_OF		2
#define CTF_FLAG_MASK	3
#define CTF_GOAL_HOME	1
#define CTF_GOAL_CARRIED	2
#define CTF_GOAL_DROPPED	3

class CBasePlayer;

class CTFGoal : public CBaseAnimating
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	void SetObjectCollisionBox();
	int Classify() { return CLASS_NONE; }
	void EXPORT PlaceGoal();
	void StartGoal();

	int		m_iGoalNum;
	int		m_iGoalState;
	Vector	m_GoalMin;
	Vector	m_GoalMax;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTFGoalFlag : public CTFGoal
{
public:
	void Spawn();
	void Precache();
	void SetObjectCollisionBox();
	int Classify() { return CLASS_NONE; }
	void EXPORT PlaceItem();
	void EXPORT goal_item_touch( CBaseEntity* pOther );
	void EXPORT goal_item_dropthink();
	void EXPORT FlagCarryThink();
	void EXPORT ReturnFlag();
	void EXPORT ReturnFlagThink();
	void StartItem();
	void EXPORT SetDropTouch();
	void DoDrop( Vector vecOrigin );
	void DropFlag( CBasePlayer* pPlayer );
	void GiveFlagToPlayer( CBasePlayer* pPlayer );
	void ScoreFlagTouch( CBasePlayer* pPlayer );
	void DisplayFlagStatus( CBasePlayer* pPlayer );
	void TurnOnLight( CBasePlayer* pPlayer );

	unsigned int	drop_time;
	Vector			m_OriginalAngles;
	int				m_nReturnPlayer;
	float			m_flReturnTime;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTFGoalBase : public CTFGoal
{
public:
	void Spawn();
	int Classify() { return CLASS_NONE; }
	void EXPORT BaseThink();
	void TurnOnLight( CBasePlayer* pPlayer );
};

class CTFSpawn : public CBaseEntity
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	int Classify() { return CLASS_NONE; }
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	BOOL IsTriggered( CBaseEntity* pEntity );

	int		team_no;
	bool	m_fState;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CTFDetect : public CBaseEntity
{
public:
	void Spawn() {}
	void KeyValue( KeyValueData* pkvd );
	int Classify() { return CLASS_NONE; }

	int		is_ctf;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

extern const char*	sTeamSpawnNames[3];
extern int			teamscores[2];
extern int			g_iMapScoreMax;
extern float		g_flFlagReturnTime;
extern float		g_flBaseDefendDist;
extern float		g_flDefendCarrierTime;
extern float		g_flCaptureAssistTime;
extern float		g_flPowerupRespawnTime;

void	DisplayTeamFlags( CBasePlayer* pPlayer = NULL );
void	ResetTeamScores();
int		CheckForLevelEnd( int iMaxScore );
void	GetLosingTeam( int& iTeamNum, int& iScoreDiff );
int		GetWinningTeam();
void	DumpCTFFlagInfo( CBasePlayer* pPlayer );
void	UpdateCTFCustomIcons( CBasePlayer* pPlayer );
