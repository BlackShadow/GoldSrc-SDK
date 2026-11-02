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
// Robin, 4-22-98: Moved set_suicide_frame() here from player.cpp to allow us to 
//				   have one without a hardcoded player.mdl in tf_client.cpp

/*

===== client.cpp ========================================================

  client/server game specific stuff

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "player.h"
#include "spectator.h"
#include "client.h"
#include "soundent.h"
#include "gamerules.h"
#include "game.h"
#include "skill.h"
#include "customentity.h"
#include "weapons.h"
#include "weaponinfo.h"
#include "usercmd.h"
#include "netadr.h"
#include "pm_shared.h"
#include "entity_state.h"
#include "tf_defs.h"
#include "tf_gamerules.h"
#include "tf_wpn_grenades.h"

#include <ctype.h>

extern BOOL g_fGameOver;
extern int g_iSkillLevel;
extern ULONG g_ulFrameCount;
extern int gmsgSayText;

extern void CopyToBodyQue( entvars_t* pev );
extern void LinkUserMessages();
extern edict_t* EntSelectSpawnPoint( edict_t* player );
extern void DestroyTeleporter( CBaseEntity* eng, int type );
extern void SendBuildingEventInfo( CBasePlayer* pPlayer );

static char* sNewWeaponPModels[] =
{
	"models/p_9mmhandgun.mdl", "models/p_crowbar.mdl", "models/p_egon.mdl",
	"models/p_glauncher.mdl", "models/p_grenade.mdl", "models/p_knife.mdl",
	"models/p_medkit.mdl", "models/p_mini.mdl", "models/p_nailgun.mdl",
	"models/p_srpg.mdl", "models/p_shotgun.mdl", "models/p_snailgun.mdl",
	"models/p_sniper.mdl", "models/p_spanner.mdl", "models/p_umbrella.mdl",
	"models/p_rpg.mdl", "models/p_spygun.mdl", "models/p_smallshotgun.mdl"
};

static char* sOldWeaponPModels[] =
{
	"models/p_9mmhandgun2.mdl", "models/p_crowbar2.mdl", "models/p_egon2.mdl",
	"models/p_glauncher2.mdl", "models/p_grenade2.mdl", "models/p_knife2.mdl",
	"models/p_medkit2.mdl", "models/p_mini2.mdl", "models/p_nailgun2.mdl",
	"models/p_rpg2.mdl", "models/p_shotgun2.mdl", "models/p_snailgun2.mdl",
	"models/p_sniper2.mdl", "models/p_spanner2.mdl", "models/p_umbrella.mdl",
	"models/p_rpg2.mdl", "models/p_9mmhandgun2.mdl", "models/p_shotgun2.mdl"
};

/*
 * used by kill command and disconnect command
 * ROBIN: Moved here from player.cpp, to allow multiple player models
 */
void set_suicide_frame( entvars_t* pev )
{
	if ( !FStrEq( STRING( pev->model ), "models/player/scout/scout.mdl" ) )
		return; // allready gibbed

//	pev->frame		= $deatha11;
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_TOSS;
	pev->deadflag = DEAD_DEAD;
	pev->nextthink = -1;
}


/*
===========
ClientConnect

called when a player connects to a server
============
*/
BOOL ClientConnect( edict_t* pEntity, const char* pszName, const char* pszAddress, char szRejectReason[128] )
{
	return g_pGameRules->ClientConnected( pEntity, pszName, pszAddress, szRejectReason );

// a client connecting during an intermission can cause problems
//	if (intermission_running)
//		ExitIntermission ();

}


/*
===========
ClientDisconnect

called when a player disconnects from a server

GLOBALS ASSUMED SET:  g_fGameOver
============
*/
void ClientDisconnect( edict_t* pEntity )
{
	if ( g_fGameOver )
		return;

	char text[256];
	sprintf( text, "- %s has left the game\n", STRING( pEntity->v.netname ) );
	MESSAGE_BEGIN( MSG_ALL, gmsgSayText );
		WRITE_BYTE( ENTINDEX( pEntity ) );
		WRITE_STRING( text );
	MESSAGE_END();

	CSound* pSound = CSoundEnt::SoundPointerForIndex( CSoundEnt::ClientSoundIndex( pEntity ) );
	if ( pSound )
		pSound->Reset();

// since the edict doesn't get deleted, fix it so it doesn't interfere.
	pEntity->v.takedamage = DAMAGE_NO; // don't attract autoaim
	pEntity->v.solid = SOLID_NOT; // nonsolid
	UTIL_SetOrigin( &pEntity->v, pEntity->v.origin );
	EMIT_SOUND_DYN( pEntity, CHAN_BODY, "player/gib.wav", 1.0, ATTN_NONE, 0, PITCH_NORM );
	g_pGameRules->ClientDisconnected( pEntity );
}


// called by ClientKill and DeadThink
void respawn( entvars_t* pev, BOOL fCopyCorpse )
{
	if ( cease_fire )
		return;

	if ( gpGlobals->coop || gpGlobals->deathmatch )
	{
		if ( fCopyCorpse )
			// make a copy of the dead body for appearances sake
			CopyToBodyQue( pev );

		// respawn player
		GetClassPtr( (CBasePlayer*)pev )->Spawn();
	}
	else
	{       // restart the entire server
		SERVER_COMMAND( "reload\n" );
	}
}

/*
============
ClientKill

Player entered the suicide command

GLOBALS ASSUMED SET:  g_ulModelIndexPlayer
============
*/
void ClientKill( edict_t* pEntity )
{
	entvars_t* pev = &pEntity->v;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pev );
	if ( !pev->playerclass || player->m_fNextSuicideTime > gpGlobals->time )
		return;

	player->m_fNextSuicideTime = gpGlobals->time + 5.0;  // don't let them suicide for 5 seconds after suiciding

	// have the player kill themself
	pev->health = 0;
	if ( player->tfstate & TFSTATE_INFECTED )
	{
		CBaseEntity* timer = player->FindTimer( TF_TIMER_INFECTION );
		if ( timer )
			((CBasePlayer*)CBaseEntity::Instance( timer->pev->enemy ))->TF_AddFrags( 1 );
	}
	player->Killed( NULL, pev, GIB_NEVER );

//	pev->modelindex = g_ulModelIndexPlayer;
//	pev->frags -= 2;		// extra penalty
//	respawn( pev );
}

void ClientPutInServer( edict_t* pEntity )
{
	CBasePlayer* player = GetClassPtr( (CBasePlayer*)&pEntity->v );
	player->SetCustomDecalFrames( -1 );
	player->lastpc = 0;
	player->pev->playerclass = PC_UNDEFINED;
	g_pGameRules->ClientUserInfoChanged( player, g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ) );
	if ( tfc_clanbattle.value )
		player->ConnectDuringBattle();
	player->Spawn();
	player->pev->effects |= EF_NOINTERP;
	player->has_disconnected = FALSE;

	for ( int i = 0; i <= gpGlobals->maxClients; i++ )
	{
		if ( g_IpStorage[i].pEdict == pEntity )
		{
			player->ip = g_IpStorage[i].ip;
			g_IpStorage[i].pEdict = NULL;
			g_IpStorage[i].ip = 0;
			break;
		}
	}
}

int FailedSpamCheck( edict_t* pEntity, int teamonly )
{
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pEntity );
	if ( !player )
		return 0;

	if ( player->m_flGagTime > gpGlobals->time )
	{
		player->m_flGagTime += tfc_spam_penalty2.value;
		char message[60];
		sprintf( message, "You may not speak for %d seconds for spamming.\n",
			(int)( player->m_flGagTime - gpGlobals->time ) );
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, player->pev );
			WRITE_BYTE( player->entindex() );
			WRITE_STRING( message );
		MESSAGE_END();
		player->m_flLastTalkTime = gpGlobals->time;
		return 1;
	}

	if ( player->m_flGagTime != -1 )
	{
		player->m_flGagTime = -1;
		player->m_cSpamPoints = 0;
	}
	float elapsed = gpGlobals->time - player->m_flLastTalkTime;
	if ( elapsed >= 2.0 )
	{
		player->m_cSpamPoints = (int)( player->m_cSpamPoints - elapsed );
		if ( player->m_cSpamPoints < 0 )
			player->m_cSpamPoints = 0;
	}
	else
	{
		player->m_cSpamPoints += teamonly ? 1 : 2;
	}

	if ( player->m_cSpamPoints <= tfc_spam_limit.value )
		return 0;
	player->m_flGagTime = gpGlobals->time + tfc_spam_penalty1.value;
	return 1;
}

void Host_Say( edict_t* pEntity, int teamonly )
{
	const char* command = CMD_ARGV( 0 );
	float spectChat = CVAR_GET_FLOAT( "tfc_spectchat" );
	if ( !CMD_ARGC() )
		return;

	char temporary[256];
	char* message;
	if ( !stricmp( command, "say" ) || !stricmp( command, "say_team" ) )
	{
		if ( CMD_ARGC() <= 1 )
			return;
		message = (char*)CMD_ARGS();
	}
	else
	{
		if ( CMD_ARGC() > 1 )
			sprintf( temporary, "%s %s", command, CMD_ARGS() );
		else
			sprintf( temporary, "%s", command );
		message = temporary;
	}
	if ( FailedSpamCheck( pEntity, teamonly ) )
		return;

	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pEntity );
	if ( *message == '"' )
	{
		message++;
		message[strlen( message ) - 1] = 0;
	}
	if ( message )
	{
		char* scan = message;
		while ( scan && *scan && ( !isprint( *scan ) || isspace( *scan ) ) )
			scan++;
		if ( scan && !*scan )
			return;

		for ( scan = message; scan && *scan; scan++ )
		{
			if ( *scan != '%' )
				continue;
			if ( strlen( message ) > 124 )
				break;
			if ( scan[1] == 'h' )
			{
				*scan = 0;
				strcat( message, UTIL_VarArgs( "%d%s", (int)player->pev->health, scan + 2 ) );
			}
			if ( scan[1] == 'a' )
			{
				*scan = 0;
				strcat( message, UTIL_VarArgs( "%d%s", (int)player->pev->armorvalue, scan + 2 ) );
			}
			if ( scan[1] == 'i' )
			{
				*scan = 0;
				if ( player->m_hLastIDTarget )
					strcat( message, UTIL_VarArgs( "%s%s", STRING( player->m_hLastIDTarget->pev->netname ), scan + 2 ) );
				else
					strcat( message, UTIL_VarArgs( "%s", scan + 2 ) );
			}
		}
	}

	player->m_flLastTalkTime = gpGlobals->time;
	char text[128];
	if ( player->IsObserver() && ( !spectChat || teamonly ) )
		sprintf( text, "%c(SPEC) %s: ", 2, STRING( pEntity->v.netname ) );
	else if ( !player->IsObserver() && teamonly )
		sprintf( text, "%c(TEAM) %s: ", 2, STRING( pEntity->v.netname ) );
	else
		sprintf( text, "%c%s: ", 2, STRING( pEntity->v.netname ) );
	int remaining = sizeof( text ) - 2 - strlen( text );
	if ( (int)strlen( message ) > remaining )
		message[remaining] = 0;
	strcat( text, message );
	strcat( text, "\n" );

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* receiver = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !receiver || receiver->edict() == pEntity || !( receiver->pev->flags & FL_CLIENT ) )
			continue;
		if ( ((CTeamFortress*)g_pGameRules)->m_VoiceGameMgr.PlayerHasBlockedPlayer( receiver, player ) )
			continue;
		if ( !player->IsObserver() && teamonly && g_pGameRules->PlayerRelationship( receiver, player ) != GR_TEAMMATE )
			continue;
		if ( player->IsObserver() && ( teamonly || !spectChat ) && !receiver->IsObserver() )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, receiver->pev );
			WRITE_BYTE( ENTINDEX( pEntity ) );
			WRITE_STRING( text );
		MESSAGE_END();
	}

	// print to the sending client
	MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, &pEntity->v );
		WRITE_BYTE( ENTINDEX( pEntity ) );
		WRITE_STRING( text );
	MESSAGE_END();

	// echo to server console
	g_engfuncs.pfnServerPrint( text );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" %s \"%s\"\n", STRING( pEntity->v.netname ),
		GETPLAYERUSERID( pEntity ), GETPLAYERAUTHID( pEntity ),
		player->team_no ? GetTeamName( player->team_no ) : "SPECTATOR", teamonly ? "say_team" : "say", message );
}

void Host_Tell( edict_t* pEntity )
{
	if ( CMD_ARGC() <= 2 || FailedSpamCheck( pEntity, 1 ) )
		return;
	const char* message = CMD_ARGS();
	if ( !message )
		return;
	if ( message )
	{
		const char* scan = message;
		while ( scan && *scan && ( !isprint( *scan ) || isspace( *scan ) ) )
			scan++;
		if ( scan && !*scan )
			return;
	}
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pEntity );
	player->m_flLastTalkTime = gpGlobals->time;
	char text[128];
	sprintf( text, "%c(TELL) %s: ", 2, STRING( pEntity->v.netname ) );
	const char* name = strstr( message, CMD_ARGV( 1 ) );
	int remaining = sizeof( text ) - 1 - strlen( text );
	if ( remaining > 0 )
		strncat( text, name ? name + strlen( CMD_ARGV( 1 ) ) : message, remaining );
	if ( strlen( text ) > sizeof( text ) - 2 )
		strcpy( text + sizeof( text ) - 2, "\n" );
	else
		strcat( text, "\n" );

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* receiver = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !receiver || receiver->edict() == pEntity || !( receiver->pev->flags & FL_CLIENT ) )
			continue;
		if ( player->IsObserver() && !receiver->IsObserver() )
			continue;
		if ( strcmp( STRING( receiver->pev->netname ), CMD_ARGV( 1 ) ) )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgSayText, NULL, receiver->pev );
			WRITE_BYTE( ENTINDEX( pEntity ) );
			WRITE_STRING( text );
		MESSAGE_END();
		char log[256];
		const char* receiverTeam = receiver->team_no ? GetTeamName( receiver->team_no ) : "SPECTATOR";
		const char* senderTeam = player->team_no ? GetTeamName( player->team_no ) : "SPECTATOR";
		snprintf( log, sizeof( log ) - 1,
			"\"%s<%i><%s><%s>\" tell \"%s<%i><%s><%s>\" message \"%s\"\n",
			STRING( player->pev->netname ), GETPLAYERUSERID( pEntity ), GETPLAYERAUTHID( pEntity ), senderTeam,
			STRING( receiver->pev->netname ), GETPLAYERUSERID( receiver->edict() ),
			GETPLAYERAUTHID( receiver->edict() ), receiverTeam, name + strlen( CMD_ARGV( 1 ) ) + 1 );
		strcpy( log + sizeof( log ) - 2, "\n" );
		g_engfuncs.pfnServerPrint( log );
		UTIL_LogPrintf( log );
		break;
	}
}

CBaseEntity* Bench_FindSpot( int spot )
{
	CBaseEntity* entity = NULL;
	while ( ( entity = UTIL_FindEntityByClassname( entity, "trigger_camera" ) ) != NULL )
	{
		if ( (int)entity->pev->speed == spot )
			return entity;
	}
	return NULL;
}

void ClientCommand( edict_t* pEntity )
{
	const char* command = CMD_ARGV( 0 );
	if ( !pEntity->pvPrivateData )
		return;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pEntity );
	entvars_t* pev = &pEntity->v;

	if ( FStrEq( command, "special" ) || FStrEq( command, "_special" ) )
		player->UseSpecialSkill();
	else if ( FStrEq( command, "say" ) )
		Host_Say( pEntity, 0 );
	else if ( FStrEq( command, "say_team" ) )
		Host_Say( pEntity, 1 );
	else if ( FStrEq( command, "fullupdate" ) )
		player->ForceClientDllUpdate();
	else if ( FStrEq( command, "tell" ) )
		Host_Tell( pEntity );
	else if ( FStrEq( command, "give" ) )
	{
		if ( CVAR_GET_FLOAT( "sv_cheats" ) )
			player->GiveNamedItem( (char*)CMD_ARGV( 1 ) );
	}
	else if ( FStrEq( command, "fov" ) )
	{
		if ( CMD_ARGC() > 1 )
		{
			player->m_iFOV = atoi( CMD_ARGV( 1 ) );
			player->pev->fov = (float)player->m_iFOV;
		}
		else
		{
			ClientPrint( pev, HUD_PRINTCONSOLE, UTIL_VarArgs( "\"fov\" is \"%d\"\n", player->m_iFOV ) );
		}
	}
	else if ( FStrEq( command, "spectate" ) )
	{
		if ( pev->flags & FL_PROXY )
		{
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"SPECTATOR\"\n", STRING( player->pev->netname ),
				GETPLAYERUSERID( player->edict() ), GETPLAYERAUTHID( player->edict() ), GetTeamName( player->team_no ) );
			edict_t* spawn = EntSelectSpawnPoint( pEntity );
			player->StartObserver( VARS( spawn )->origin, VARS( spawn )->angles );
		}
		else if ( allow_spectators.value )
		{
			if ( player->m_fNextTeamOrClassChange == 0 ||
				( gpGlobals->time > player->m_fNextTeamOrClassChange && player->team_no != 0 ) )
			{
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" joined team \"SPECTATOR\"\n", STRING( player->pev->netname ),
					GETPLAYERUSERID( player->edict() ), GETPLAYERAUTHID( player->edict() ), GetTeamName( player->team_no ) );
				player->team_no = 0;
				player->m_fNextTeamOrClassChange = gpGlobals->time + 1;
				edict_t* spawn = EntSelectSpawnPoint( pEntity );
				player->StartObserver( VARS( spawn )->origin, VARS( spawn )->angles );
			}
		}
		else if ( !player->pev->playerclass && !player->pev->team )
		{
			player->Menu_Team();
		}
	}
	else if ( FStrEq( command, "specmode" ) )	// new spectator mode
	{
		if ( player->IsObserver() )
			player->Observer_SetMode( atoi( CMD_ARGV( 1 ) ) );
	}
	else if ( FStrEq( command, "follownext" ) )
	{
		if ( player->IsObserver() )
			player->Observer_FindNextPlayer( atoi( CMD_ARGV( 1 ) ) != 0 );
	}
	else if ( FStrEq( command, "scout" ) )
		player->ChangeClass( PC_SCOUT );
	else if ( FStrEq( command, "sniper" ) )
		player->ChangeClass( PC_SNIPER );
	else if ( FStrEq( command, "soldier" ) )
		player->ChangeClass( PC_SOLDIER );
	else if ( FStrEq( command, "demoman" ) )
		player->ChangeClass( PC_DEMOMAN );
	else if ( FStrEq( command, "medic" ) )
		player->ChangeClass( PC_MEDIC );
	else if ( FStrEq( command, "hvyweap" ) || FStrEq( command, "hwguy" ) )
		player->ChangeClass( PC_HVYWEAP );
	else if ( FStrEq( command, "pyro" ) )
		player->ChangeClass( PC_PYRO );
	else if ( FStrEq( command, "spy" ) )
		player->ChangeClass( PC_SPY );
	else if ( FStrEq( command, "engineer" ) )
		player->ChangeClass( PC_ENGINEER );
	else if ( FStrEq( command, "randompc" ) )
	{
		if ( player->PositionAvailable() )
			player->ChangeClass( PC_RANDOM );
		else
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Game_enoughofclass" );
			player->Menu_Class();
		}
	}
	else if ( FStrEq( command, "civilian" ) )
	{
		if ( TeamFortress_TeamIsCivilian( (float)player->team_no ) )
			player->ChangeClass( PC_CIVILIAN );
	}
	else if ( FStrEq( command, "changeclass" ) )
	{
		if ( player->IsObserver() )
			player->Menu_Team();
		else if ( player->pev->playerclass )
			player->Menu_Class();
	}
	else if ( FStrEq( command, "changeteam" ) )
		player->Menu_Team();
	else if ( FStrEq( command, "closemenus" ) )
		player->ResetMenu();
	else if ( FStrEq( command, "jointeam" ) )
	{
		if ( CMD_ARGC() > 1 )
		{
			int team = atoi( CMD_ARGV( 1 ) );
			if ( team == 5 )
				player->TeamFortress_TeamPutPlayerInTeam();
			else if ( team > 0 && team <= number_of_teams )
				player->TeamFortress_TeamSet( team );
		}
	}
	else if ( FStrEq( command, "admin" ) )
		player->AdminAccess( (char*)CMD_ARGV( 1 ) );
	else if ( player->is_admin && FStrEq( command, "adm_countplayers" ) )
		player->Admin_CountPlayers();
	else if ( player->is_admin && FStrEq( command, "adm_ceasefire" ) )
		Admin_CeaseFire();
	else if ( player->is_admin && ( FStrEq( command, "adm_deal" ) ||
		FStrEq( command, "adm_next" ) || FStrEq( command, "next" ) ) )
		player->Admin_CycleDeal();
	else if ( player->is_admin && player->admin_mode == 1 &&
		( FStrEq( command, "adm_kick" ) || FStrEq( command, "kick" ) ) )
		player->Admin_DoKick();
	else if ( player->is_admin && player->admin_mode == 1 &&
		( FStrEq( command, "adm_ban" ) || FStrEq( command, "ban" ) ) )
		player->Admin_DoBan();
	else if ( player->is_admin && FStrEq( command, "adm_listplayers" ) )
		player->Admin_ListIPs();
	else if ( player->is_admin && FStrEq( command, "adm_changelevel" ) )
		player->Admin_Changelevel();
	else if ( !g_pGameRules->ClientCommand( player, command ) )
	{
		if ( !player->IsAlive() )
			return;
		if ( !player->is_building && !player->is_detpacking && !player->is_feigning )
		{
			if ( FStrEq( command, "use" ) )
				player->SelectItem( CMD_ARGV( 1 ) );
			else if ( strstr( command, "weapon_" ) == command || strstr( command, "tf_weapon_" ) == command )
				player->SelectItem( command );
			else if ( FStrEq( command, "lastinv" ) )
				player->SelectLastItem();
			else if ( FStrEq( command, "primeone" ) || FStrEq( command, "+gren1" ) )
				player->TeamFortress_PrimeGrenade( 1 );
			else if ( FStrEq( command, "primetwo" ) || FStrEq( command, "+gren2" ) )
				player->TeamFortress_PrimeGrenade( 2 );
			else if ( FStrEq( command, "detstart" ) )
			{
				int time = 20;
				if ( CMD_ARGC() > 1 )
					time = max( 5, min( 50, atoi( CMD_ARGV( 1 ) ) ) );
				player->TeamFortress_SetDetpack( time );
			}
			else if ( FStrEq( command, "+det5" ) )
				player->TeamFortress_SetDetpack( 5 );
			else if ( FStrEq( command, "+det20" ) )
				player->TeamFortress_SetDetpack( 20 );
			else if ( FStrEq( command, "+det50" ) )
				player->TeamFortress_SetDetpack( 50 );
			else if ( FStrEq( command, "discard" ) || FStrEq( command, "drop" ) )
				player->TeamFortress_Discard();
			else if ( player->pev->playerclass && FStrEq( command, "saveme" ) )
				player->TeamFortress_SaveMe();
		}
	}

	if ( FStrEq( command, "flaginfo" ) )
	{
		player->TeamFortress_DisplayDetectionItems();
		return;
	}
	int playerclass = player->pev->playerclass;
	if ( playerclass )
	{
		if ( FStrEq( command, "dropitems" ) )
		{
			player->DropGoalItems();
			return;
		}
		if ( playerclass == PC_ENGINEER )
		{
			if ( FStrEq( command, "detsentry" ) )
			{
				DestroyBuilding( player, "building_sentrygun" );
				return;
			}
			if ( FStrEq( command, "detentryteleporter" ) )
			{
				DestroyTeleporter( player, BUILD_TELEPORTER_ENTRANCE );
				return;
			}
			if ( FStrEq( command, "detexitteleporter" ) )
			{
				DestroyTeleporter( player, BUILD_TELEPORTER_EXIT );
				return;
			}
			if ( FStrEq( command, "detdispenser" ) || FStrEq( command, "detdispensor" ) )
			{
				DestroyBuilding( player, "building_dispenser" );
				return;
			}
			if ( FStrEq( command, "dismantle" ) )
			{
				if ( CMD_ARGC() > 1 )
				{
					switch ( atoi( CMD_ARGV( 1 ) ) )
					{
					case 1: player->Menu_Engineer_Input( 9 ); break;
					case 2: player->Menu_Engineer_Input( 3 ); break;
					case 4: player->Menu_Engineer_Input( 10 ); break;
					case 5: player->Menu_Engineer_Input( 11 ); break;
					}
				}
				return;
			}
			if ( FStrEq( command, "rotatesentry" ) || FStrEq( command, "rotatesentry180" ) )
			{
				player->Menu_Engineer_Input( FStrEq( command, "rotatesentry" ) ? 4.0f : 6.0f );
				return;
			}
		}
	}
	if ( FStrEq( command, "detpipe" ) )
	{
		if ( player->m_pActiveItem && ((CBasePlayerWeapon*)player->m_pActiveItem)->m_flNextPrimaryAttack <= 0 )
			player->ExplodeOldPipebomb( TRUE, FALSE );
	}
	else if ( FStrEq( command, "throwgren" ) || FStrEq( command, "-gren1" ) || FStrEq( command, "-gren2" ) )
		player->TeamFortress_ThrowPrimedGrenade();
	else if ( FStrEq( command, "detstop" ) || FStrEq( command, "-det5" ) ||
		FStrEq( command, "-det20" ) || FStrEq( command, "-det50" ) )
		player->TeamFortress_DetpackStop();
	else if ( playerclass == PC_ENGINEER && FStrEq( command, "build" ) )
		player->TeamFortress_EngineerBuild( CMD_ARGC() == 1 ? 0 : atoi( CMD_ARGV( 1 ) ) );
	else if ( playerclass == PC_SPY && FStrEq( command, "disguise" ) )
	{
		if ( CMD_ARGC() == 1 )
			player->TeamFortress_SpyGoUndercover();
		else if ( CMD_ARGC() == 2 )
			player->TeamFortress_SpyDisguise( atoi( CMD_ARGV( 1 ) ), 0 );
		else if ( CMD_ARGC() == 3 )
			player->TeamFortress_SpyDisguise( atoi( CMD_ARGV( 1 ) ), atoi( CMD_ARGV( 2 ) ) );
	}
	else if ( playerclass == PC_SPY && ( FStrEq( command, "disguise_enemy" ) || FStrEq( command, "disguise_friendly" ) ) )
		player->TeamFortress_SpyDisguiseEnemy( FStrEq( command, "disguise_enemy" ),
			CMD_ARGC() == 1 ? 0 : atoi( CMD_ARGV( 1 ) ) );
	else if ( playerclass == PC_SPY && ( FStrEq( command, "feign" ) || FStrEq( command, "sfeign" ) ) )
		player->TeamFortress_SpyFeignDeath( FStrEq( command, "sfeign" ) );
	else if ( FStrEq( command, "timeleft" ) )
	{
		if ( gpGlobals->time <= player->m_iTimeCheckAllowed )
			return;
		if ( !timelimit.value )
			ClientPrint( player->pev, HUD_PRINTTALK, "#Game_no_timelimit" );
		else
		{
			float time = timelimit.value * 60 - gpGlobals->time;
			int seconds = max( 0, (int)time );
			int iMinutes = seconds / 60;
			int iSeconds = seconds % 60;
			char secs[3];
			char* temp = UTIL_dtos2( iSeconds );
			if ( iSeconds >= 10 )
			{
				secs[0] = temp[0];
				secs[1] = temp[1];
				secs[2] = '\0';
			}
			else
			{
				secs[0] = '0';
				secs[1] = temp[0];
				secs[2] = '\0';
			}
			ClientPrint( player->pev, HUD_PRINTTALK, "#Game_timelimit", UTIL_dtos1( iMinutes ), secs );
		}
		player->m_iTimeCheckAllowed = (int)( gpGlobals->time + 1 );
	}
	else if ( FStrEq( command, "sendevents" ) && !player->m_bSentBuildingEvents )
	{
		SendBuildingEventInfo( player );
		player->m_bSentBuildingEvents = TRUE;
	}
}

void ClientUserInfoChanged( edict_t* pEntity, char* infobuffer )
{
	// Is the client spawned yet?
	if ( !pEntity->pvPrivateData )
		return;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pEntity );
	if ( pEntity->v.netname && STRING( pEntity->v.netname )[0] &&
		!FStrEq( STRING( pEntity->v.netname ), g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) ) )
	{
		char name[256];
		strncpy( name, g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ), sizeof( name ) - 1 );
		name[sizeof( name ) - 1] = 0;
		for ( char* scan = name; scan && *scan; scan++ )
		{
			if ( *scan == '%' )
				*scan = ' ';
		}
		g_engfuncs.pfnSetClientKeyValue( ENTINDEX( pEntity ), infobuffer, "name", name );
		char text[256];
		sprintf( text, "* %s changed name to %s\n", STRING( pEntity->v.netname ),
			g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
		MESSAGE_BEGIN( MSG_ALL, gmsgSayText );
			WRITE_BYTE( ENTINDEX( pEntity ) );
			WRITE_STRING( text );
		MESSAGE_END();
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" changed name to \"%s\"\n", STRING( pEntity->v.netname ),
			GETPLAYERUSERID( pEntity ), GETPLAYERAUTHID( pEntity ),
			player->team_no ? GetTeamName( player->team_no ) : "SPECTATOR", g_engfuncs.pfnInfoKeyValue( infobuffer, "name" ) );
	}
	g_pGameRules->ClientUserInfoChanged( GetClassPtr( (CBasePlayer*)&pEntity->v ), infobuffer );
}

static int g_serveractive;

void ServerDeactivate()
{
	if ( g_serveractive == 1 )
		g_serveractive = 0;

	// Peform any shutdown operations here...
	//
}

void ServerActivate( edict_t* pEdictList, int edictCount, int clientMax )
{

	// Every call to ServerActivate should be matched by a call to ServerDeactivate
	g_serveractive = 1;

	// Clients have not been initialized yet
	for ( int i = 0; i < edictCount; i++ )
	{
		if ( pEdictList[i].free || i < clientMax || !pEdictList[i].pvPrivateData )
			continue;
		CBaseEntity* entity = CBaseEntity::Instance( &pEdictList[i] );
		if ( entity && !( entity->pev->flags & FL_DORMANT ) )
			entity->Activate();
		else
			ALERT( at_console, "Can't instance %s\n", STRING( pEdictList[i].v.classname ) );
	}
	ParseTFMapSettings();

	// Link user messages here to make sure first client can get them...
	LinkUserMessages();
	CVAR_SET_FLOAT( "sv_maxspeed", 400 );
	SERVER_COMMAND( "sv_maxspeed 400\n" );
	static BOOL firstTime = TRUE;
	if ( firstTime )
	{
		g_engfuncs.pfnAddServerCommand( "tf_ceasefire", Admin_CeaseFire );
		firstTime = FALSE;
	}
}


/*
================
PlayerPreThink

Called every frame before physics are run
================
*/
void PlayerPreThink( edict_t* pEntity )
{
	CBasePlayer* player = (CBasePlayer*)GET_PRIVATE( pEntity );
	if ( player )
		player->PreThink();
}

/*
================
PlayerPostThink

Called every frame after physics are run
================
*/
void PlayerPostThink( edict_t* pEntity )
{
	CBasePlayer* player = (CBasePlayer*)GET_PRIVATE( pEntity );
	if ( player )
		player->PostThink();
}

void ParmsNewLevel()
{
}

void ParmsChangeLevel()
{
	SAVERESTOREDATA* save = (SAVERESTOREDATA*)gpGlobals->pSaveData;
	if ( save )
		save->connectionCount = BuildChangeList( save->levelList, MAX_LEVEL_CONNECTIONS );
}


//
// GLOBALS ASSUMED SET:  g_ulFrameCount
//
void StartFrame()
{
	if ( g_pGameRules )
		g_pGameRules->Think();
	if ( g_fGameOver )
		return;

	gpGlobals->teamplay = teamplay.value;
	gpGlobals->deathmatch = gpGlobals->maxClients > 7 ? 1.0f : CVAR_GET_FLOAT( "deathmatch" );
	if ( ( tfc_balance_teams.value || tfc_balance_scores.value ) && gpGlobals->time > flNextEqualisationCalc )
	{
		CalculateTeamEqualiser();
		flNextEqualisationCalc = gpGlobals->time + 10.0;
	}
	if ( cb_prematch_time )
	{
		float prematch = (float)( 60.0 *
			( tfc_clanbattle_prematch.value ? tfc_clanbattle_prematch.value : tfc_prematch.value ) );
		if ( ceilf( prematch ) != ceilf( fOldPrematch ) )
		{
			fOldPrematch = prematch;
			cb_prematch_time = prematch + gpGlobals->time;
			g_fNextPrematchAlert = 0;
		}
		Display_Prematch();
	}
	if ( initial_cease_fire )
	{
		float ceasefire = (float)( 60.0 * tfc_clanbattle_ceasefire.value );
		if ( ceilf( ceasefire ) != ceilf( fOldCeaseFire ) )
		{
			fOldCeaseFire = ceasefire;
			cb_ceasefire_time = ceasefire + gpGlobals->time;
		}
	}
	if ( last_cease_fire )
		Check_Ceasefire();
	g_ulFrameCount++;
	g_iSkillLevel = SKILL_EASY;
}

void ClientPrecache()
{
	// setup precaches always needed
	PRECACHE_SOUND( "player/sprayer.wav" );			// spray paint sound for PreAlpha
	PRECACHE_SOUND( "player/plyrjmp8.wav" );
	PRECACHE_SOUND( "misc/talk.wav" );
	PRECACHE_SOUND( "player/pain1.wav" );
	PRECACHE_SOUND( "player/pain2.wav" );
	PRECACHE_SOUND( "player/pain3.wav" );
	PRECACHE_SOUND( "player/pain4.wav" );
	PRECACHE_SOUND( "player/pain5.wav" );
	PRECACHE_SOUND( "player/pain6.wav" );
	PRECACHE_SOUND( "player/axhit1.wav" );
	PRECACHE_SOUND( "player/drown1.wav" );
	PRECACHE_SOUND( "player/drown2.wav" );
	PRECACHE_SOUND( "player/h2odeath.wav" );
	PRECACHE_SOUND( "player/death1.wav" );
	PRECACHE_SOUND( "player/death2.wav" );
	PRECACHE_SOUND( "player/death3.wav" );
	PRECACHE_SOUND( "player/death4.wav" );
	PRECACHE_SOUND( "player/death5.wav" );
	PRECACHE_SOUND( "player/gib.wav" );
	PRECACHE_SOUND( "player/pl_fallpain2.wav" );
	PRECACHE_SOUND( "player/pl_fallpain3.wav" );
	PRECACHE_SOUND( "player/pl_step1.wav" );		// walk on concrete
	PRECACHE_SOUND( "player/pl_step2.wav" );
	PRECACHE_SOUND( "player/pl_step3.wav" );
	PRECACHE_SOUND( "player/pl_step4.wav" );
	PRECACHE_SOUND( "common/npc_step1.wav" );		// NPC walk on concrete
	PRECACHE_SOUND( "common/npc_step2.wav" );
	PRECACHE_SOUND( "common/npc_step3.wav" );
	PRECACHE_SOUND( "common/npc_step4.wav" );
	PRECACHE_SOUND( "player/pl_metal1.wav" );		// walk on metal
	PRECACHE_SOUND( "player/pl_metal2.wav" );
	PRECACHE_SOUND( "player/pl_metal3.wav" );
	PRECACHE_SOUND( "player/pl_metal4.wav" );
	PRECACHE_SOUND( "player/pl_dirt1.wav" );		// walk on dirt
	PRECACHE_SOUND( "player/pl_dirt2.wav" );
	PRECACHE_SOUND( "player/pl_dirt3.wav" );
	PRECACHE_SOUND( "player/pl_dirt4.wav" );
	PRECACHE_SOUND( "player/pl_duct1.wav" );		// walk in duct
	PRECACHE_SOUND( "player/pl_duct2.wav" );
	PRECACHE_SOUND( "player/pl_duct3.wav" );
	PRECACHE_SOUND( "player/pl_duct4.wav" );
	PRECACHE_SOUND( "player/pl_grate1.wav" );		// walk on grate
	PRECACHE_SOUND( "player/pl_grate2.wav" );
	PRECACHE_SOUND( "player/pl_grate3.wav" );
	PRECACHE_SOUND( "player/pl_grate4.wav" );
	PRECACHE_SOUND( "player/pl_slosh1.wav" );		// walk in shallow water
	PRECACHE_SOUND( "player/pl_slosh2.wav" );
	PRECACHE_SOUND( "player/pl_slosh3.wav" );
	PRECACHE_SOUND( "player/pl_slosh4.wav" );
	PRECACHE_SOUND( "player/pl_tile1.wav" );		// walk on tile
	PRECACHE_SOUND( "player/pl_tile2.wav" );
	PRECACHE_SOUND( "player/pl_tile3.wav" );
	PRECACHE_SOUND( "player/pl_tile4.wav" );
	PRECACHE_SOUND( "player/pl_tile5.wav" );
	PRECACHE_SOUND( "player/pl_swim1.wav" );		// breathe bubbles
	PRECACHE_SOUND( "player/pl_swim2.wav" );
	PRECACHE_SOUND( "player/pl_swim3.wav" );
	PRECACHE_SOUND( "player/pl_swim4.wav" );
	PRECACHE_SOUND( "player/pl_ladder1.wav" );	// climb ladder rung
	PRECACHE_SOUND( "player/pl_ladder2.wav" );
	PRECACHE_SOUND( "player/pl_ladder3.wav" );
	PRECACHE_SOUND( "player/pl_ladder4.wav" );
	PRECACHE_SOUND( "player/pl_wade1.wav" );		// wade in water
	PRECACHE_SOUND( "player/pl_wade2.wav" );
	PRECACHE_SOUND( "player/pl_wade3.wav" );
	PRECACHE_SOUND( "player/pl_wade4.wav" );
	PRECACHE_SOUND( "debris/wood1.wav" );			// hit wood texture
	PRECACHE_SOUND( "debris/wood2.wav" );
	PRECACHE_SOUND( "debris/wood3.wav" );
	PRECACHE_SOUND( "plats/train_use1.wav" );		// use a train
	PRECACHE_SOUND( "buttons/spark5.wav" );
	PRECACHE_SOUND( "buttons/spark6.wav" );
	PRECACHE_SOUND( "debris/glass1.wav" );
	PRECACHE_SOUND( "debris/glass2.wav" );
	PRECACHE_SOUND( "debris/glass3.wav" );
	PRECACHE_SOUND( "misc/r_tele3.wav" );
	PRECACHE_SOUND( "misc/r_tele4.wav" );
	PRECACHE_SOUND( "items/medshot4.wav" );
	PRECACHE_SOUND( "items/medshot5.wav" );
	PRECACHE_SOUND( "items/r_item2.wav" );
	PRECACHE_SOUND( "weapons/building.wav" );
	PRECACHE_SOUND( "weapons/ax1.wav" );
	PRECACHE_SOUND( "weapons/timer.wav" );
	PRECACHE_SOUND( "items/itembk2.wav" );
	PRECACHE_SOUND( SOUND_FLASHLIGHT_ON );
	PRECACHE_SOUND( SOUND_FLASHLIGHT_OFF );

// player gib sounds
	PRECACHE_SOUND( "common/bodysplat.wav" );

// player pain sounds
	PRECACHE_SOUND( "player/pl_pain2.wav" );
	PRECACHE_SOUND( "player/pl_pain4.wav" );
	PRECACHE_SOUND( "player/pl_pain5.wav" );
	PRECACHE_SOUND( "player/pl_pain6.wav" );
	PRECACHE_SOUND( "player/pl_pain7.wav" );
	PRECACHE_SOUND( "speech/saveme1.wav" );
	PRECACHE_SOUND( "speech/saveme2.wav" );
	PRECACHE_SOUND( "vox/stolen.wav" );

	for ( int i = 1; i < PC_LASTCLASS; i++ )
	{
		if ( sNewClassModelFiles[i] )
			PRECACHE_MODEL( sNewClassModelFiles[i] );
		if ( sOldClassModelFiles[i] )
			PRECACHE_MODEL( sOldClassModelFiles[i] );
	}
	for ( int i = 0; i < 18; i++ )
	{
		PRECACHE_MODEL( sNewWeaponPModels[i] );
		PRECACHE_MODEL( sOldWeaponPModels[i] );
	}
	Vector mins( -43, -14, -49 );
	Vector maxs( 43, 11, 49 );
	for ( int i = 1; i < PC_LASTCLASS; i++ )
	{
		if ( sNewClassModelFiles[i] )
			ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, sNewClassModelFiles[i] );
		if ( sOldClassModelFiles[i] )
			ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, sOldClassModelFiles[i] );
	}
	for ( int i = 0; i < 18; i++ )
	{
		ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, sNewWeaponPModels[i] );
		ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, sOldWeaponPModels[i] );
	}
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/w_grenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/spy_grenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/ngrenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/napalm.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/pipebomb.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/mirv_grenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/bomblet.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/emp_grenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/conc_grenade.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/caltrop.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/flag.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/ball.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/detpack.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/w_rpg.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "models/rpgrocket.mdl" );
	mins = Vector( -32, -32, -32 );
	maxs = Vector( 32, 32, 32 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/base.mdl" );
	mins = Vector( -24, -24, -24 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/sentry1.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/sentry2.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/sentry3.mdl" );
	maxs = Vector( 16, 64, 16 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/dispenser.mdl" );
	mins = Vector( -29, -29, -1 );
	maxs = Vector( 29, 26, 12 );
	ENGINE_FORCE_UNMODIFIED( force_model_specifybounds, mins, maxs, "models/teleporter.mdl" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "sprites/playerflame.spr" );
	ENGINE_FORCE_UNMODIFIED( force_exactfile, NULL, NULL, "sprites/fthrow.spr" );
	UTIL_PrecacheOther( "detpack" );
	PRECACHE_SOUND( "ambience/flameburst1.wav" );
	PRECACHE_SOUND( "ambience/steamburst1.wav" );
	PRECACHE_SOUND( "misc/b1.wav" );
	PRECACHE_SOUND( "misc/b2.wav" );
	PRECACHE_SOUND( "misc/party1.wav" );
	PRECACHE_SOUND( "misc/party2.wav" );

	// hud sounds

	PRECACHE_SOUND( "common/wpn_hudoff.wav" );
	PRECACHE_SOUND( "common/wpn_hudon.wav" );
	PRECACHE_SOUND( "common/wpn_moveselect.wav" );
	PRECACHE_SOUND( "common/wpn_select.wav" );
	PRECACHE_SOUND( "common/wpn_denyselect.wav" );
}

const char* GetGameDescription()
{
	return "Team Fortress Classic";
}

/*
================
Sys_Error

Engine is going to shut down, allows setting a breakpoint in game .dll to catch that occasion
================
*/
void Sys_Error( const char* )
{
	// Default case, do nothing.  MOD AUTHORS:  Add code ( e.g., _asm { int 3 }; here to cause a breakpoint for debugging your game .dlls
}

/*
================
PlayerCustomization

A new player customization has been registered on the server
UNDONE:  This only sets the # of frames of the spray can logo
animation right now.
================
*/
void PlayerCustomization( edict_t* pEntity, customization_t* pCust )
{
	CBasePlayer* player = (CBasePlayer*)GET_PRIVATE( pEntity );
	if ( !player )
	{
		ALERT( at_console, "PlayerCustomization:  Couldn't get player!\n" );
		return;
	}
	if ( !pCust )
	{
		ALERT( at_console, "PlayerCustomization:  NULL customization!\n" );
		return;
	}
	switch ( pCust->resource.type )
	{
	case t_decal: player->SetCustomDecalFrames( pCust->nUserData2 ); break;
	case t_sound:
	case t_skin:
	case t_model: break;
	default: ALERT( at_console, "PlayerCustomization:  Unknown customization type!\n" ); break;
	}
}

/*
================
SpectatorConnect

A spectator has joined the game
================
*/
void SpectatorConnect( edict_t* pEntity )
{
	CBaseSpectator* player = (CBaseSpectator*)GET_PRIVATE( pEntity );
	if ( player )
		player->SpectatorConnect();
}

/*
================
SpectatorConnect

A spectator has left the game
================
*/
void SpectatorDisconnect( edict_t* pEntity )
{
	CBaseSpectator* player = (CBaseSpectator*)GET_PRIVATE( pEntity );
	if ( player )
		player->SpectatorDisconnect();
}

/*
================
SpectatorConnect

A spectator has sent a usercmd
================
*/
void SpectatorThink( edict_t* pEntity )
{
	CBaseSpectator* player = (CBaseSpectator*)GET_PRIVATE( pEntity );
	if ( player )
		player->SpectatorThink();
}

void SetupVisibility( edict_t* pViewEntity, edict_t* pClient, unsigned char** pvs, unsigned char** pas )
{
	edict_t* view = pViewEntity ? pViewEntity : pClient;
	if ( pClient->v.flags & FL_PROXY )
	{
		*pvs = NULL;	// the spectator proxy sees
		*pas = NULL;	// and hears everything
		return;
	}
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pClient );
	if ( player->pev->iuser2 && player->m_hObserverTarget )
		view = player->m_hObserverTarget->edict();
	Vector origin = view->v.origin + view->v.view_ofs;
	if ( view->v.flags & FL_DUCKING )
		origin = origin + ( VEC_HULL_MIN - VEC_DUCK_HULL_MIN );
	*pvs = ENGINE_SET_PVS( origin );
	*pas = ENGINE_SET_PAS( origin );
}

int AddToFullPack( entity_state_t* state, int e, edict_t* ent, edict_t* host,
	int hostflags, int player, unsigned char* pSet )
{
	CBaseEntity* entity = CBaseEntity::Instance( ent );
	if ( ( ( ent->v.effects & EF_NODRAW ) && ent != host ) || !ent->v.modelindex || !STRING( ent->v.model ) )
		return 0;
	if ( ( ent->v.flags & FL_SPECTATOR ) && ent != host )
		return 0;
	if ( ent != host && !ENGINE_CHECK_VISIBILITY( ent, pSet ) )
		return 0;
	if ( ( ent->v.flags & FL_SKIPLOCALHOST ) && ( hostflags & 1 ) && ent->v.owner == host )
		return 0;
	if ( host->v.groupinfo )
	{
		UTIL_SetGroupTrace( host->v.groupinfo, GROUP_OP_AND );

		// Should always be set, of course
		if ( ent->v.groupinfo )
		{
			if ( g_groupop == GROUP_OP_AND && !( ent->v.groupinfo & host->v.groupinfo ) )
				return 0;
			if ( g_groupop == GROUP_OP_NAND && ( ent->v.groupinfo & host->v.groupinfo ) )
				return 0;
		}
		UTIL_UnsetGroupTrace();
	}
	memset( state, 0, sizeof( *state ) );

	// Assign index so we can track this entity from frame to frame and
	//  delta from it.
	state->number = e;
	state->entityType = ( ent->v.flags & FL_CUSTOMENTITY ) ? ENTITY_BEAM : ENTITY_NORMAL;

	// 
	// Copy state data
	//

	// Round animtime to nearest millisecond
	state->animtime = (int)( 1000.0 * ent->v.animtime ) / 1000.0;
	state->origin = ent->v.origin;
	state->angles = ent->v.angles;
	state->mins = ent->v.mins;
	state->maxs = ent->v.maxs;
	state->startpos = ent->v.startpos;
	state->endpos = ent->v.endpos;
	state->impacttime = ent->v.impacttime;
	state->starttime = ent->v.starttime;
	state->modelindex = ent->v.modelindex;
	state->frame = ent->v.frame;
	state->skin = (short)( entity && entity->replacement_model_skin ? entity->replacement_model_skin : ent->v.skin );
	state->effects = ent->v.effects;
	if ( !player && ent->v.animtime && ent->v.velocity == g_vecZero )
		state->eflags |= EFLAG_SLERP;
	state->scale = ent->v.scale;
	state->solid = (short)ent->v.solid;
	state->colormap = ent->v.colormap;
	state->movetype = ent->v.movetype;
	state->sequence = ent->v.sequence;
	state->framerate = ent->v.framerate;
	state->body = entity && entity->replacement_model_body ? entity->replacement_model_body : ent->v.body;
	memcpy( state->controller, ent->v.controller, sizeof( ent->v.controller ) );
	memcpy( state->blending, ent->v.blending, sizeof( ent->v.blending ) );
	state->rendermode = ent->v.rendermode;
	state->renderamt = (int)ent->v.renderamt;
	state->renderfx = ent->v.renderfx;
	state->rendercolor.r = (byte)ent->v.rendercolor.x;
	state->rendercolor.g = (byte)ent->v.rendercolor.y;
	state->rendercolor.b = (byte)ent->v.rendercolor.z;
	if ( ent->v.aiment )
		state->aiment = ENTINDEX( ent->v.aiment );
	if ( ent->v.owner )
	{
		int owner = ENTINDEX( ent->v.owner );

		// Only care if owned by a player
		if ( owner >= 1 && owner <= gpGlobals->maxClients )
			state->owner = owner;
	}
	state->fuser1 = 0;
	if ( player )
	{
		state->basevelocity = ent->v.basevelocity;
		state->weaponmodel = entity && ( entity->replacement_model_flags & 1 ) ?
			0 : MODEL_INDEX( STRING( ent->v.weaponmodel ) );
		state->gaitsequence = ent->v.gaitsequence;
		state->spectator = ent->v.flags & FL_SPECTATOR;
		state->friction = ent->v.friction;
		state->gravity = ent->v.gravity;
		state->team = ent->v.team;
		state->playerclass = ent->v.playerclass;
		state->usehull = ( ent->v.flags & FL_DUCKING ) != 0;
		state->health = (int)ent->v.health;
		if ( entity && entity->replacement_model )
			state->fuser1 = 1;
	}
	CBaseEntity* soundEntity = (CBaseEntity*)GET_PRIVATE( ent );
	if ( soundEntity && soundEntity->Classify() != CLASS_NONE && soundEntity->Classify() != CLASS_MACHINE )
		state->eflags |= EFLAG_FLESH_SOUND;
	else
		state->eflags &= ~EFLAG_FLESH_SOUND;
	return 1;
}

void CreateBaseline( int player, int eindex, entity_state_t* baseline, edict_t* entity,
	int playermodelindex, vec3_t player_mins, vec3_t player_maxs )
{
	baseline->origin = entity->v.origin;
	baseline->angles = entity->v.angles;
	baseline->frame = entity->v.frame;
	baseline->skin = (short)entity->v.skin;

	// render information
	baseline->rendermode = (byte)entity->v.rendermode;
	baseline->renderamt = (byte)entity->v.renderamt;
	baseline->rendercolor.r = (byte)entity->v.rendercolor.x;
	baseline->rendercolor.g = (byte)entity->v.rendercolor.y;
	baseline->rendercolor.b = (byte)entity->v.rendercolor.z;
	baseline->renderfx = (byte)entity->v.renderfx;
	if ( player )
	{
		baseline->mins = player_mins;
		baseline->maxs = player_maxs;
		baseline->colormap = eindex;
		baseline->modelindex = playermodelindex;
		baseline->friction = 1;
		baseline->movetype = MOVETYPE_WALK;
		baseline->scale = entity->v.scale;
		baseline->solid = SOLID_SLIDEBOX;
		baseline->framerate = 1;
		baseline->gravity = 1;
		baseline->playerclass = entity->v.playerclass;
		baseline->fov = entity->v.fov;
	}
	else
	{
		baseline->mins = entity->v.mins;
		baseline->maxs = entity->v.maxs;
		baseline->colormap = 0;
		baseline->modelindex = entity->v.modelindex; //SV_ModelIndex(pr_strings + entity->v.model);
		baseline->movetype = entity->v.movetype;
		baseline->scale = entity->v.scale;
		baseline->solid = (short)entity->v.solid;
		baseline->framerate = entity->v.framerate;
		baseline->gravity = entity->v.gravity;
	}
}

typedef struct
{
	char	name[32];
	int		field;
} entity_field_alias_t;

#define FIELD_ORIGIN0	0
#define FIELD_ORIGIN1	1
#define FIELD_ORIGIN2	2
#define FIELD_ANGLES0	3
#define FIELD_ANGLES1	4
#define FIELD_ANGLES2	5

static entity_field_alias_t entity_field_alias[] =
{
	{ "origin[0]", 0 }, { "origin[1]", 0 }, { "origin[2]", 0 },
	{ "angles[0]", 0 }, { "angles[1]", 0 }, { "angles[2]", 0 }
};

void Entity_FieldInit( delta_s* pFields )
{
	for ( int i = 0; i < 6; i++ )
		entity_field_alias[i].field = DELTA_FINDFIELD( pFields, entity_field_alias[i].name );
}

void Entity_Encode( delta_s* pFields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Entity_FieldInit( pFields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	if ( current->number - 1 == ENGINE_CURRENT_PLAYER() )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_UNSETBYINDEX( pFields, entity_field_alias[i].field );
	}
	if ( current->impacttime && current->starttime )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ANGLES2; i++ )
			DELTA_UNSETBYINDEX( pFields, entity_field_alias[i].field );
	}
	if ( current->movetype == MOVETYPE_FOLLOW && current->aiment )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_UNSETBYINDEX( pFields, entity_field_alias[i].field );
	}
	else if ( current->aiment != previous->aiment )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_SETBYINDEX( pFields, entity_field_alias[i].field );
	}
}

static entity_field_alias_t player_field_alias[] =
{
	{ "origin[0]", 0 }, { "origin[1]", 0 }, { "origin[2]", 0 }
};

void Player_FieldInit( delta_s* pFields )
{
	for ( int i = 0; i < 3; i++ )
		player_field_alias[i].field = DELTA_FINDFIELD( pFields, player_field_alias[i].name );
}

void Player_Encode( delta_s* pFields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Player_FieldInit( pFields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	if ( current->number - 1 == ENGINE_CURRENT_PLAYER() )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_UNSETBYINDEX( pFields, entity_field_alias[i].field );
	}
	if ( current->movetype == MOVETYPE_FOLLOW && current->aiment )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_UNSETBYINDEX( pFields, entity_field_alias[i].field );
	}
	else if ( current->aiment != previous->aiment )
	{
		for ( int i = FIELD_ORIGIN0; i <= FIELD_ORIGIN2; i++ )
			DELTA_SETBYINDEX( pFields, entity_field_alias[i].field );
	}
}

#define CUSTOMFIELD_ORIGIN0	0
#define CUSTOMFIELD_ORIGIN1	1
#define CUSTOMFIELD_ORIGIN2	2
#define CUSTOMFIELD_ANGLES0	3
#define CUSTOMFIELD_ANGLES1	4
#define CUSTOMFIELD_ANGLES2	5
#define CUSTOMFIELD_SKIN		6
#define CUSTOMFIELD_SEQUENCE	7
#define CUSTOMFIELD_ANIMTIME	8

entity_field_alias_t custom_entity_field_alias[] =
{
	{ "origin[0]", 0 }, { "origin[1]", 0 }, { "origin[2]", 0 },
	{ "angles[0]", 0 }, { "angles[1]", 0 }, { "angles[2]", 0 },
	{ "skin", 0 }, { "sequence", 0 }, { "animtime", 0 }
};

void Custom_Entity_FieldInit( delta_s* pFields )
{
	for ( int i = 0; i < 9; i++ )
		custom_entity_field_alias[i].field = DELTA_FINDFIELD( pFields, custom_entity_field_alias[i].name );
}

/*
==================
Custom_Encode

Callback for sending entity_state_t info ( for custom entities ) over network. 
FIXME:  Move to script
==================
*/
void Custom_Encode( delta_s* pFields, const unsigned char* from, const unsigned char* to )
{
	static int initialized;
	if ( !initialized )
	{
		Custom_Entity_FieldInit( pFields );
		initialized = 1;
	}
	const entity_state_t* previous = (const entity_state_t*)from;
	const entity_state_t* current = (const entity_state_t*)to;
	int beamType = current->rendermode & 0x0f;
	if ( beamType != BEAM_POINTS && beamType != BEAM_ENTPOINT )
	{
		for ( int i = CUSTOMFIELD_ORIGIN0; i <= CUSTOMFIELD_ORIGIN2; i++ )
			DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[i].field );
	}
	if ( beamType != BEAM_POINTS )
	{
		for ( int i = CUSTOMFIELD_ANGLES0; i <= CUSTOMFIELD_ANGLES2; i++ )
			DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[i].field );
	}
	if ( beamType != BEAM_ENTS && beamType != BEAM_ENTPOINT )
	{
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_SKIN].field );
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_SEQUENCE].field );
	}
	if ( (int)previous->animtime == (int)current->animtime )
		DELTA_UNSETBYINDEX( pFields, custom_entity_field_alias[CUSTOMFIELD_ANIMTIME].field );
}

/*
=================
RegisterEncoders

Allows game .dll to override network encoding of certain types of entities and tweak values, etc.
=================
*/
void RegisterEncoders()
{
	DELTA_ADDENCODER( "Entity_Encode", Entity_Encode );
	DELTA_ADDENCODER( "Custom_Encode", Custom_Encode );
	DELTA_ADDENCODER( "Player_Encode", Player_Encode );
}

int GetWeaponData( edict_t* player, weapon_data_t* info )
{
	CBasePlayer* pl = (CBasePlayer*)CBaseEntity::Instance( &player->v );
	memset( info, 0, 32 * sizeof( *info ) );
	if ( !pl )
		return 1;

	// go through all of the weapons and make a list of the ones to pack
	for ( int i = 0; i < MAX_ITEM_TYPES; i++ )
	{
		for ( CBasePlayerItem* item = pl->m_rgpPlayerItems[i]; item; item = item->m_pNext )
		{
			CBasePlayerWeapon* weapon = (CBasePlayerWeapon*)item->GetWeaponPtr();
			if ( !weapon )
				continue;
			ItemInfo details;
			memset( &details, 0, sizeof( details ) );
			weapon->GetItemInfo( &details );
			if ( details.iId < 0 || details.iId >= 32 )
				continue;
			weapon_data_t* data = &info[details.iId];
			data->m_iId = details.iId;
			data->m_iClip = weapon->m_iClip;
			data->m_flPumpTime = max( weapon->m_flPumpTime, -0.001f );
			data->m_flTimeWeaponIdle = max( weapon->m_flTimeWeaponIdle, -0.001f );
			data->m_flNextPrimaryAttack = max( weapon->m_flNextPrimaryAttack, -0.001f );
			data->m_flNextSecondaryAttack = max( weapon->m_flNextSecondaryAttack, -0.001f );
			data->m_flNextReload = max( weapon->m_flNextReload, -0.001f );
			data->m_fNextAimBonus = max( weapon->m_fNextAimBonus, -1.0f );
			data->m_fInReload = weapon->m_fInReload;
			data->m_fInSpecialReload = weapon->m_fInSpecialReload;
			data->m_fReloadTime = weapon->m_fReloadTime;
			data->m_fAimedDamage = weapon->m_fAimedDamage;
			data->m_fInZoom = weapon->m_fInZoom;
			data->m_iWeaponState = weapon->m_iWeaponState;
		}
	}
	return 1;
}

/*
=================
UpdateClientData

Data sent to current client only
engine sets cd to 0 before calling.
=================
*/
void UpdateClientData( const edict_t* ent, int sendweapons, clientdata_t* cd )
{
	entvars_t* pev = (entvars_t*)&ent->v;
	CBasePlayer* player = (CBasePlayer*)CBaseEntity::Instance( pev );
	entvars_t* original = NULL;
	if ( player->pev->iuser1 == OBS_IN_EYE && player->m_hObserverTarget )
	{
		original = pev;
		pev = player->m_hObserverTarget->pev;
		player = (CBasePlayer*)CBaseEntity::Instance( pev );
	}
	cd->flags = pev->flags;
	cd->health = pev->health;
	cd->viewmodel = MODEL_INDEX( STRING( pev->viewmodel ) );
	cd->waterlevel = pev->waterlevel;
	cd->watertype = pev->watertype;
	cd->weapons = pev->weapons;

	// Vectors
	cd->origin = pev->origin;
	cd->velocity = pev->velocity;
	cd->view_ofs = pev->view_ofs;
	cd->punchangle = pev->punchangle;
	cd->bInDuck = pev->bInDuck;
	cd->flTimeStepSound = pev->flTimeStepSound;
	cd->flDuckTime = pev->flDuckTime;
	cd->flSwimTime = pev->flSwimTime;
	cd->waterjumptime = (int)pev->teleport_time;
	strcpy( cd->physinfo, ENGINE_GETPHYSINFO( ent ) );
	cd->maxspeed = pev->maxspeed;
	cd->fov = pev->fov;
	cd->weaponanim = pev->weaponanim;
	cd->pushmsec = pev->pushmsec;
	if ( original )
	{
		cd->iuser1 = original->iuser1;
		cd->iuser2 = original->iuser2;
		cd->iuser3 = original->iuser2;
	}
	else
	{
		cd->iuser1 = pev->iuser1;
		cd->iuser2 = pev->iuser2;
		cd->iuser3 = pev->iuser3;
		cd->iuser4 = pev->iuser4;
	}
	cd->deadflag = pev->deadflag;
	if ( sendweapons && player )
	{
		cd->ammo_shells = player->ammo_shells;
		cd->ammo_nails = player->ammo_nails;
		cd->ammo_cells = player->ammo_cells;
		cd->ammo_rockets = player->ammo_rockets;
		cd->m_flNextAttack = player->m_flNextAttack;
		if ( player->m_pActiveItem )
		{
			CBasePlayerWeapon* weapon = (CBasePlayerWeapon*)player->m_pActiveItem->GetWeaponPtr();
			if ( weapon )
			{
				ItemInfo details;
				memset( &details, 0, sizeof( details ) );
				weapon->GetItemInfo( &details );
				cd->m_iId = details.iId;
			}
		}
	}
}

/*
=================
CmdStart

We're about to run this usercmd for the specified player.  We can set up groupinfo and masking here, etc.
This is the time to examine the usercmd for anything extra.  This call happens even if think does not.
=================
*/
void CmdStart( const edict_t* player, const usercmd_t*, unsigned int random_seed )
{
	CBasePlayer* pl = (CBasePlayer*)CBaseEntity::Instance( (entvars_t*)&player->v );
	if ( !pl )
		return;
	if ( pl->pev->groupinfo )
		UTIL_SetGroupTrace( pl->pev->groupinfo, GROUP_OP_AND );
	pl->random_seed = random_seed;
}

/*
=================
CmdEnd

Each cmdstart is exactly matched with a cmd end, clean up any group trace flags, etc. here
=================
*/
void CmdEnd( const edict_t* player )
{
	CBasePlayer* pl = (CBasePlayer*)CBaseEntity::Instance( (entvars_t*)&player->v );
	if ( !pl )
		return;
	if ( pl->pev->groupinfo )
		UTIL_UnsetGroupTrace();
	if ( pl->pev->deadflag == DEAD_DISCARDBODY + 1 )
		UTIL_SetSize( pl->pev, Vector( -16, -16, -36 ), Vector( 16, 16, -32 ) );
}

/*
================================
ConnectionlessPacket

 Return 1 if the packet is valid.  Set response_buffer_size if you want to send a response packet.  Incoming, it holds the max
  size of the response_buffer, so you must zero it out if you choose not to respond.
================================
*/
int ConnectionlessPacket( const netadr_t*, const char*, char*, int* response_buffer_size )
{

	// Zero it out since we aren't going to respond.
	// If we wanted to response, we'd write data into response_buffer
	*response_buffer_size = 0;

	// Since we don't listen for anything here, just respond that it's a bogus message
	// If we didn't reject the message, we'd return 1 for success instead.
	return 0;
}

int GetHullBounds( int hullnumber, float*, float* )
{
	return (unsigned int)hullnumber <= 2;
}

/*
================================
CreateInstancedBaselines

Create pseudo-baselines for items that aren't placed in the map at spawn time, but which are likely
to be created during play ( e.g., grenades, ammo packs, projectiles, corpses, etc. )
================================
*/
void CreateInstancedBaselines()
{
	entity_state_t state;
	memset( &state, 0, sizeof( state ) );
	CTFCaltrop* caltrop = GetClassPtr( (CTFCaltrop*)NULL );
	caltrop->ScatterDrop( NULL );
	CreateBaseline( 0, 0, &state, caltrop->edict(), 0, g_vecZero, g_vecZero );
	ENGINE_INSTANCE_BASELINE( caltrop->pev->classname, &state );
	CBaseEntity* box = GetClassPtr( (CWeaponBox*)NULL );
	box->pev->classname = MAKE_STRING( "weaponbox" );
	box->Spawn();
	CreateBaseline( 0, 0, &state, box->edict(), 0, g_vecZero, g_vecZero );
	ENGINE_INSTANCE_BASELINE( box->pev->classname, &state );
	UTIL_Remove( caltrop );
	UTIL_Remove( box );
}

/*
================================
InconsistentFile

One of the ENGINE_FORCE_UNMODIFIED files failed the consistency check for the specified player
 Return 0 to allow the client to continue, 1 to force immediate disconnection ( with an optional disconnect message of up to 256 characters )
================================
*/
int InconsistentFile( const edict_t*, const char* filename, char* disconnect_message )
{
	// Server doesn't care?
	if ( CVAR_GET_FLOAT( "mp_consistency" ) != 1 )
		return 0;

	// Default behavior is to kick the player
	sprintf( disconnect_message, "Server is enforcing file consistency for %s\n", filename );

	// Kick now with specified disconnect message.
	return 1;
}

/*
================================
AllowLagCompensation

 The game .dll should return 1 if lag compensation should be allowed ( could also just set
  the sv_unlag cvar.
 Most games right now should return 0, until client-side weapon prediction code is written
  and tested for them ( note you can predict weapons, but not do lag compensation, too, 
  if you want.
================================
*/
int AllowLagCompensation()
{
	return 1;
}

int ShouldCollide( edict_t*, edict_t* )
{
	return 1;
}
