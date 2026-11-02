/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// teamplay_gamerules.h
//

#pragma once

class CHalfLifeTeamplay : public CHalfLifeMultiplay
{
public:
	CHalfLifeTeamplay();
	BOOL ClientCommand( CBasePlayer* pPlayer, const char* pcmd );
	void ClientUserInfoChanged( CBasePlayer* pPlayer, char* infobuffer );
	BOOL IsTeamplay();
	BOOL FPlayerCanTakeDamage( CBasePlayer* pPlayer, CBaseEntity* pAttacker );
	int PlayerRelationship( CBasePlayer* pPlayer, CBaseEntity* pTarget );
	const char* GetTeamID( CBaseEntity* pEntity );
	BOOL ShouldAutoAim( CBasePlayer* pPlayer, edict_t* target );
	int IPointsForKill( CBasePlayer* pAttacker, CBasePlayer* pKilled );
	void InitHUD( CBasePlayer* pl );
	void DeathNotice( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pevInflictor );
	const char* GetGameDescription() { return "HL Teamplay"; }
	void UpdateGameMode( CBasePlayer* pPlayer );
	void PlayerKilled( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor );
	void PlayerSpawn( CBasePlayer* pPlayer );

private:
	BOOL	m_DisableDeathMessages;
	BOOL	m_DisableDeathPenalty;
};
