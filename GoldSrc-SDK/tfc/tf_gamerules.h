#pragma once

#include "teamplay_gamerules.h"
#include "voice_gamemgr.h"

class CTeamFortress : public CHalfLifeTeamplay
{
public:
	CTeamFortress();
	BOOL IsTeamplay();
	void PlayerSpawn( CBasePlayer* pPlayer );
	int DeadPlayerWeapons( CBasePlayer* pPlayer );
	int DeadPlayerAmmo( CBasePlayer* pPlayer );
	float FlItemRespawnTime( CItem* pItem );
	void ClientUserInfoChanged( CBasePlayer* pPlayer, char* infobuffer );
	const char* GetTeamID( CBaseEntity* pEntity );
	BOOL CanHaveItem( CBasePlayer* pPlayer, CItem* pItem );
	void GoToIntermission();
	void Think();
	BOOL ClientConnected( edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128] );
	BOOL ClientCommand( CBasePlayer* pPlayer, const char* pcmd );
	void InitHUD( CBasePlayer* pl );
	const char* GetGameDescription() { return "TF Classic"; }

	CVoiceGameMgr	m_VoiceGameMgr;
};
