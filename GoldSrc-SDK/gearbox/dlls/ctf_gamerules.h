#pragma once

class CHalfLifeCTFplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeCTFplay();

	void Think() override;
	BOOL IsCTF() override;
	BOOL IsTeamplay() override;
	const char* GetGameDescription() override;
	BOOL ClientConnected( edict_t* pEntity, const char* pszName, const char* pszAddress,
		char szRejectReason[128] ) override;
	void InitHUD( CBasePlayer* pPlayer ) override;
	void ClientDisconnected( edict_t* pClient ) override;
	void UpdateGameMode( CBasePlayer* pPlayer ) override;
	BOOL FPlayerCanTakeDamage( CBasePlayer* pPlayer, CBaseEntity* pAttacker ) override;
	BOOL ShouldAutoAim( CBasePlayer* pPlayer, edict_t* target ) override;
	void PlayerSpawn( CBasePlayer* pPlayer ) override;
	void PlayerThink( CBasePlayer* pPlayer ) override;
	BOOL ClientCommand( CBasePlayer* pPlayer, const char* pcmd ) override;
	void ClientUserInfoChanged( CBasePlayer* pPlayer, char* infobuffer ) override;
	int IPointsForKill( CBasePlayer* pAttacker, CBasePlayer* pKilled ) override;
	void PlayerKilled( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor ) override;
	void DeathNotice( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor ) override;
	BOOL CanHaveAmmo( CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry ) override;
	const char* GetTeamID( CBaseEntity* pEntity ) override;
	int PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget ) override;
	int GetTeamIndex( const char* pTeamName ) override;
	const char* GetIndexedTeamName( int teamIndex ) override;
	BOOL IsValidTeam( const char* pTeamName ) override;
	void ChangePlayerTeam( CBasePlayer* pPlayer, const char* pTeamName, BOOL bKill, BOOL bGib ) override;
	const char* SetDefaultPlayerTeam( CBasePlayer* pPlayer ) override;
	const char* GetCharacterType( int iTeamNum, int iCharNum ) override;
	int GetNumTeams() override;
	const char* TeamWithFewestPlayers() override;
	BOOL TeamsBalanced() override;

	void RecountTeams();
	void SendPlayerStatInfo( CBasePlayer* pPlayer );
	void SendTeamStatInfo( int iTeamNum );

	BOOL	m_DisableDeathMessages;
	BOOL	m_DisableDeathPenalty;
	BOOL	m_fRefreshScores;
	float	m_flNextStatsSend;
	int		m_iStatsPhase;
	int		m_iStatsPlayer;

protected:
	void GoToIntermission() override;
};

const char*	GetTeamName( edict_t* pEntity );
