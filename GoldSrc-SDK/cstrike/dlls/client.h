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

#ifndef CLIENT_H
#define CLIENT_H

#include "weapontype.h"
#include "player.h"

class CBasePlayer;
class CBasePlayerItem;
class CBaseEntity;

void respawn( entvars_t* pev, BOOL copyCorpse );
void set_suicide_frame( entvars_t* pev );
BOOL ClientConnect( edict_t* entity, const char* name, const char* address, char rejectReason[128] );
void ClientDisconnect( edict_t* entity );
void ClientKill( edict_t* entity );
void ClientPutInServer( edict_t* entity );
void ClientCommand( edict_t* entity );
void ClientUserInfoChanged( edict_t* entity, char* info );
void ServerActivate( edict_t* entities, int count, int maxClients );
void ServerDeactivate();
void StartFrame();
void PlayerPostThink( edict_t* entity );
void PlayerPreThink( edict_t* entity );
void ParmsNewLevel();
void ParmsChangeLevel();
void ClientPrecache();
const char* GetGameDescription();
void PlayerCustomization( edict_t* entity, customization_t* customization );
void SpectatorConnect( edict_t* entity );
void SpectatorDisconnect( edict_t* entity );
void SpectatorThink( edict_t* entity );
void Sys_Error( const char* error );
void SetupVisibility( edict_t* viewEntity, edict_t* client, unsigned char** pvs, unsigned char** pas );
void UpdateClientData( const edict_t* entity, int sendWeapons, struct clientdata_s* data );
int AddToFullPack( struct entity_state_s* state, int index, edict_t* entity, edict_t* host,
	int hostFlags, int player, unsigned char* set );
void CreateBaseline( int player, int index, struct entity_state_s* baseline, edict_t* entity,
	int playerModelIndex, Vector playerMins, Vector playerMaxs );
void RegisterEncoders();
int GetWeaponData( edict_t* player, struct weapon_data_s* info );
void CmdStart( const edict_t* player, const struct usercmd_s* command, unsigned int randomSeed );
void CmdEnd( const edict_t* player );
int ConnectionlessPacket( const struct netadr_s* from, const char* args, char* response, int* responseSize );
int GetHullBounds( int hull, float* mins, float* maxs );
void CreateInstancedBaselines();
int InconsistentFile( const edict_t* player, const char* filename, char* message );
int AllowLagCompensation();

bool CanBuyThis( CBasePlayer* player, int weapon );
void BuyPistol( CBasePlayer* player, int slot );
void BuyShotgun( CBasePlayer* player, int slot );
void BuySubMachineGun( CBasePlayer* player, int slot );
void BuyRifle( CBasePlayer* player, int slot );
void BuyMachineGun( CBasePlayer* player, int slot );
void BuyItem( CBasePlayer* player, int slot );
void BuyWeaponByWeaponID( CBasePlayer* player, WeaponIdType weapon );
bool BuyGunAmmo( CBasePlayer& player, CBasePlayerItem& weapon, bool blinkMoney );
bool BuyAmmo( CBasePlayer* player, int slot, bool blinkMoney );
void DropPrimary( CBasePlayer* player );
void Radio1( CBasePlayer* player, int slot );
void Radio2( CBasePlayer* player, int slot );
void Radio3( CBasePlayer* player, int slot );
void ShowMenu( CBasePlayer* player, int slots, int time, BOOL more, char* text );
void ShowVGUIMenu( CBasePlayer* player, int menu, int slots, char* text );
void ListPlayers( CBasePlayer* player );
CBaseEntity* EntityFromUserID( int userID );
int CountPlayersInServer();
TeamName SelectDefaultTeam();
BOOL HandleMenu_ChooseTeam( CBasePlayer* player, int slot );
void HandleMenu_ChooseAppearance( CBasePlayer* player, int slot );
BOOL HandleBuyAliasCommands( CBasePlayer* player, const char* command );
BOOL HandleRadioAliasCommands( CBasePlayer* player, const char* command );
void ProcessKickVote( CBasePlayer* player, CBasePlayer* kicked );
void Host_Say( edict_t* entity, int teamOnly );
void CheckStartMoney();
void BlinkAccount( CBasePlayer* player, int count );

extern "C"
{
	int CountTeams();
	int CountTeamPlayers( int team );
	int Q_strlen( const char* string );
}

#endif
