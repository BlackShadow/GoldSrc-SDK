#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "tf_defs.h"

void CBasePlayer::AdminAccess( char* pPassword )
{
	if ( !pPassword || !*pPassword )
		return;
	if ( !tfc_adminpwd.string || !*tfc_adminpwd.string )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_nopassword" );
		return;
	}
	if ( strcmp( tfc_adminpwd.string, pPassword ) == 0 )
	{
		is_admin = TRUE;
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_access" );
	}
	else
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_badpassword" );
}

void CBasePlayer::Admin_CountPlayers()
{
	int playerCount = TeamFortress_GetNoPlayers();
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_noplayers", UTIL_dtos1( playerCount ) );
	if ( number_of_teams > 0 )
	{
		playerCount = TeamFortress_TeamGetNoPlayers( 1 );
		const char* players = UTIL_dtos2( playerCount );
		const char* team = UTIL_dtos1( 1 );
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_noteamplayers", team, players );
	}
	if ( !( number_of_teams <= 1 ) )
	{
		playerCount = TeamFortress_TeamGetNoPlayers( 2 );
		const char* players = UTIL_dtos2( playerCount );
		const char* team = UTIL_dtos1( 2 );
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_noteamplayers", team, players );
	}
	if ( !( number_of_teams <= 2 ) )
	{
		playerCount = TeamFortress_TeamGetNoPlayers( 3 );
		const char* players = UTIL_dtos2( playerCount );
		const char* team = UTIL_dtos1( 3 );
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_noteamplayers", team, players );
	}
	if ( number_of_teams > 3 )
	{
		playerCount = TeamFortress_TeamGetNoPlayers( 4 );
		const char* players = UTIL_dtos2( playerCount );
		const char* team = UTIL_dtos1( 4 );
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_noteamplayers", team, players );
	}
}

void CBasePlayer::Admin_ListIPs()
{
	if ( TeamFortress_GetNoPlayers() <= 1 )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_gameempty" );
		return;
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer )
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_listip", STRING( pPlayer->pev->netname ), STRING( pPlayer->ip ) );
	}
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_endlist" );
}

void CBasePlayer::Admin_CycleDeal()
{
	if ( TeamFortress_GetNoPlayers() <= 1 )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_gameempty" );
		admin_use = NULL;
		admin_mode = 0;
		return;
	}
	int index = 0;
	if ( admin_use )
		index = admin_use->entindex() + 1;
	for ( ; index <= gpGlobals->maxClients; index++ )
	{
		admin_use = UTIL_PlayerByIndex( index );
		if ( admin_use && (CBaseEntity*)admin_use != this )
		{
			admin_mode = 1;
			const char* address = STRING( admin_use->ip );
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_kickban", STRING( admin_use->pev->netname ), address );
			return;
		}
	}
	admin_mode = 0;
	ClientPrint( pev, HUD_PRINTNOTIFY, "#Admin_endlist" );
}

void KickPlayer( CBaseEntity* pTarget )
{
	int userid = GETPLAYERUSERID( pTarget->edict() );
	if ( userid != -1 )
		SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", userid ) );
}

void BanPlayer( CBaseEntity* pTarget )
{
	SERVER_COMMAND( UTIL_VarArgs( "banid 30 %s kick\n", GETPLAYERAUTHID( pTarget->edict() ) ) );
	SERVER_COMMAND( UTIL_VarArgs( "addip 30 %s\n", STRING( pTarget->ip ) ) );
	int userid = GETPLAYERUSERID( pTarget->edict() );
	if ( userid != -1 )
		SERVER_COMMAND( UTIL_VarArgs( "kick # %d\n", userid ) );
}

void CBasePlayer::Admin_DoKick()
{
	if ( !admin_use )
		return;
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Admin_kick", STRING( admin_use->pev->netname ), STRING( pev->netname ) );
	const char* targetTeam = admin_use->team_no ? GetTeamName( admin_use->team_no ) : "SPECTATOR";
	const char* targetAuth = GETPLAYERAUTHID( admin_use->edict() );
	int targetID = GETPLAYERUSERID( admin_use->edict() );
	const char* targetName = STRING( admin_use->pev->netname );
	const char* adminTeam = team_no ? GetTeamName( team_no ) : "SPECTATOR";
	const char* adminAuth = GETPLAYERAUTHID( edict() );
	int adminID = GETPLAYERUSERID( edict() );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Admin_Kick\" against \"%s<%i><%s><%s>\"\n",
		STRING( pev->netname ), adminID, adminAuth, adminTeam, targetName, targetID, targetAuth, targetTeam );
	KickPlayer( admin_use );
	admin_mode = 0;
	admin_use = NULL;
}

void CBasePlayer::Admin_DoBan()
{
	if ( !admin_use )
		return;
	UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Admin_ban", STRING( admin_use->pev->netname ), STRING( pev->netname ) );
	const char* targetTeam = admin_use->team_no ? GetTeamName( admin_use->team_no ) : "SPECTATOR";
	const char* targetAuth = GETPLAYERAUTHID( admin_use->edict() );
	int targetID = GETPLAYERUSERID( admin_use->edict() );
	const char* targetName = STRING( admin_use->pev->netname );
	const char* adminTeam = team_no ? GetTeamName( team_no ) : "SPECTATOR";
	const char* adminAuth = GETPLAYERAUTHID( edict() );
	int adminID = GETPLAYERUSERID( edict() );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Admin_Ban\" against \"%s<%i><%s><%s>\"\n",
		STRING( pev->netname ), adminID, adminAuth, adminTeam, targetName, targetID, targetAuth, targetTeam );
	BanPlayer( admin_use );
	admin_mode = 0;
	admin_use = NULL;
}

void Admin_CeaseFire()
{
	if ( cease_fire )
	{
		cease_fire = FALSE;
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_resumefire" );
		UTIL_LogPrintf( "World triggered \"Resume_Fire\"\n" );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( pPlayer )
			{
				pPlayer->pev->iuser4 = 0;
				pPlayer->tfstate &= ~TFSTATE_CANT_MOVE;
				pPlayer->immune_to_check = gpGlobals->time + 10;
				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}
	else
	{
		cease_fire = TRUE;
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Admin_ceasefire" );
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( pPlayer )
			{
				pPlayer->pev->iuser4 = 1;
				pPlayer->tfstate |= TFSTATE_CANT_MOVE;
				pPlayer->immune_to_check = gpGlobals->time + 10;
				pPlayer->TeamFortress_SetSpeed();
			}
		}
	}
}

void CBasePlayer::CheckAutoKick()
{
	if ( autokick_kills == 0 )
		return;
	if ( !( teamkills < autokick_kills ) )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#AKick_kicked", STRING( pev->netname ) );
		const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
		const char* auth = GETPLAYERAUTHID( edict() );
		int userid = GETPLAYERUSERID( edict() );
		UTIL_LogPrintf( "\"<-1><><>\" triggered \"Auto_Kick_For_TK\" against \"%s<%i><%s><%s>\"\n",
			STRING( pev->netname ), userid, auth, team );
		BanPlayer( this );
		return;
	}
	if ( teamkills == autokick_kills - 1 )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#AKick_warn" );
		ClientPrint( pev, HUD_PRINTNOTIFY, "#AKick_warn" );
	}

	CBaseEntity* pTimer = UTIL_FindEntityByClassname( NULL, "ak_timer" );
	edict_t* owner = NULL;
	if ( pTimer )
	{
		do
		{
			owner = pTimer->pev->owner;
			pTimer = UTIL_FindEntityByClassname( pTimer, "ak_timer" );
		}
		while ( pTimer && owner != edict() );
	}
	if ( owner != edict() )
	{
		pTimer = CreateTimer( 0 );
		pTimer->pev->classname = MAKE_STRING( "ak_timer" );
		pTimer->SetThink( &CBaseEntity::Timer_AutokickThink );
		pTimer->pev->nextthink = gpGlobals->time + tfc_autokick_time.value;
	}
}

void CBasePlayer::Admin_Changelevel()
{
	if ( CMD_ARGC() != 2 )
		return;
	char command[256];
	sprintf( command, "changelevel %s\n", CMD_ARGV( 1 ) );
	SERVER_COMMAND( command );
}
