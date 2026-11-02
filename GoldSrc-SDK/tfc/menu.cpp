#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "game.h"
#include "tf_defs.h"

extern int gmsgVGUIMenu;

static char cbuf[512];

void teamsprint( int tno, CBaseEntity*, int msg_dest, const char* st,
	const char* param1, const char* param2, const char* param3 )
{
	if ( !tno )
		return;
	for ( int playerIndex = 1; playerIndex <= gpGlobals->maxClients; ++playerIndex )
	{
		CBasePlayer* player = (CBasePlayer*)UTIL_PlayerByIndex( playerIndex );
		if ( player && player->IsAlly( tno ) )
			ClientPrint( player->pev, msg_dest, st, param1, param2, param3 );
	}
}

float anglemod( float angle )
{
	while ( angle >= 360 )
		angle -= 360;
	while ( angle < 0 )
		angle += 360;
	return angle;
}

char* infokey( CBasePlayer* player, const char* key )
{
	if ( !player || !key )
		return "";
	return g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( player->edict() ), (char*)key );
}

void CBasePlayer::ResetMenu()
{
	menu_displaytime = 0;
	m_MenuStringBuffer[0] = 0;
	menu_refreshtime = 0;
	current_menu = MENU_DEFAULT;
}

void MenuPrint( CBasePlayer* player, const char* message, int allowed, int )
{
	if ( player->m_MenuUpdateTime > gpGlobals->time &&
		player->m_MenuSelectionBuffer == allowed && !strcmp( message, player->m_MenuStringBuffer ) )
		return;
	player->m_MenuUpdateTime = gpGlobals->time + 10;
	strncpy( player->m_MenuStringBuffer, message, sizeof( player->m_MenuStringBuffer ) );
	player->m_MenuSelectionBuffer = allowed;

	char buffer[51];
	while ( strlen( message ) > 49 )
	{
		strncpy( buffer, message, 50 );
		message += 50;
	}
}

void CBasePlayer::Menu_MapBriefing()
{
	const char* displayMenu = infokey( this, "dm" );
	if ( !displayMenu || !*displayMenu || ( strcmp( displayMenu, "0" ) && strcmp( displayMenu, "off" ) ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, edict() );
			WRITE_BYTE( MENU_MAPBRIEFING );
			WRITE_STRING( STRING( gpGlobals->mapname ) );
		MESSAGE_END();
	}
}

void CBasePlayer::Menu_Team()
{
	if ( gpGlobals->time > 30 && tfc_autoteam.value && g_pGameRules->IsTeamplay() &&
		TeamFortress_TeamPutPlayerInTeam() )
		return;
	current_menu = MENU_TEAM;
	MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, edict() );
		WRITE_BYTE( MENU_TEAM );
	MESSAGE_END();
}

void CBasePlayer::Menu_Class()
{
	current_menu = MENU_CLASS;
	MESSAGE_BEGIN( MSG_ONE, gmsgVGUIMenu, NULL, edict() );
		WRITE_BYTE( MENU_CLASS );
	MESSAGE_END();
}

void CBasePlayer::Menu_RepeatHelp()
{
	MenuPrint( this, "#Menu_repeathelp", 0xFFFF, 0 );
}

void CBasePlayer::Menu_Intro()
{
	MenuPrint( this, "Welcome to TeamFortress v2.9 Beta\n"
		"by TeamFortress Software\n"
		"======================================\n"
		"www.planetquake.com/teamfortress", 0xFFFF, 0 );
}

void CBasePlayer::Menu_ClassHelp()
{
	switch ( pev->playerclass )
	{
	case PC_SCOUT: MenuPrint( this, "#Help_scout", 0xFFFF, 0 ); break;
	case PC_SNIPER: MenuPrint( this, "#Help_sniper", 0xFFFF, 0 ); break;
	case PC_SOLDIER: MenuPrint( this, "#Help_soldier", 0xFFFF, 0 ); break;
	case PC_DEMOMAN: MenuPrint( this, "#Help_demoman", 0xFFFF, 0 ); break;
	case PC_MEDIC: MenuPrint( this, "#Help_medic", 0xFFFF, 0 ); break;
	case PC_HVYWEAP: MenuPrint( this, "#Help_hwguy", 0xFFFF, 0 ); break;
	case PC_PYRO: MenuPrint( this, "#Help_pyro", 0xFFFF, 0 ); break;
	case PC_SPY: MenuPrint( this, "#Help_spy", 0xFFFF, 0 ); break;
	case PC_ENGINEER: MenuPrint( this, "#Help_engineer", 0xFFFF, 0 ); break;
	}
}

void CBasePlayer::Menu_ClassHelp_Input( float )
{
	ResetMenu();
}

void CBasePlayer::Menu_Spy()
{
	if ( ( pev->effects & ( EF_BRIGHTFIELD | EF_BRIGHTLIGHT ) ) || is_unableto_spy_or_teleport )
	{
		ResetMenu();
		return;
	}

	const char* message;
	if ( is_feigning )
	{
		if ( undercover_team && undercover_skin )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_stopfeign \n4. #Spy_resetskinandcolor \n5. #Nothing \n\n";
		else if ( undercover_team )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_stopfeign \n4. #Spy_resetcolor \n5. #Nothing \n\n";
		else if ( undercover_skin )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_stopfeign \n4. #Spy_resetskin \n5. #Nothing \n\n";
		else
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_stopfeign \n4. #Nothing \n\n";
	}
	else
	{
		if ( undercover_team && undercover_skin )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_startfeign \n4. #Spy_resetskinandcolor \n5. #Nothing \n\n";
		else if ( undercover_team )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_startfeign \n4. #Spy_resetcolor \n5. #Nothing \n\n";
		else if ( undercover_skin )
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_startfeign \n4. #Spy_resetskin \n5. #Nothing \n\n";
		else
			message = "#Action \n1. #Spy_changeskin \n2. #Spy_changecolor \n3. #Spy_startfeign \n4. #Nothing \n\n";
	}
	MenuPrint( this, message, 0xFFFF, 0 );
}

void CBasePlayer::Menu_Spy_Skin()
{
	MenuPrint( this, "#Spy_skinmenu \n1. #Scout \n2. #Sniper \n3. #Soldier \n4. #Demoman \n"
		"5. #Medic \n6. #HWGuy \n7. #Pyro \n8. #Spy \n9. #Engineer \n\n", 0xFFFF, 0 );
}

void CBasePlayer::Menu_Spy_Skin_Input( float input )
{
	if ( input < 10 && input > 0 )
	{
		TeamFortress_SpyChangeSkin( (int)input );
		ResetMenu();
		pev->impulse = 0;
	}
}

void CBasePlayer::Menu_Spy_Color()
{
	if ( number_of_teams == 0 )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Game_nocolorchange" );
		ResetMenu();
		pev->impulse = 0;
	}
	else if ( number_of_teams == 1 )
		MenuPrint( this, "#Spy_colormenu \n\n1. #BlueTeam \n\n", 0xFFFF, 0 );
	else if ( number_of_teams == 2 )
		MenuPrint( this, "#Spy_colormenu \n\n1. #BlueTeam \n2. #RedTeam \n\n", 0xFFFF, 0 );
	else if ( number_of_teams == 3 )
		MenuPrint( this, "#Spy_colormenu \n\n1. #BlueTeam \n2. #RedTeam \n3. #YellowTeam \n\n", 0xFFFF, 0 );
	else
		MenuPrint( this, "#Spy_colormenu \n\n1. #BlueTeam \n2. #RedTeam \n3. #YellowTeam \n4. #GreenTeam \n\n", 0xFFFF, 0 );
}

void CBasePlayer::Menu_Spy_Input( float input )
{
	if ( input == 1 || input == 2 )
	{
		if ( pev->effects & ( EF_BRIGHTFIELD | EF_BRIGHTLIGHT ) )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Spy_glowing" );
			ResetMenu();
			pev->impulse = 0;
			return;
		}
		if ( is_unableto_spy_or_teleport )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Spy_unable" );
			ResetMenu();
			pev->impulse = 0;
			return;
		}
		if ( input == 1 )
		{
			Menu_Spy_Skin();
			current_menu = MENU_SPY_SKIN;
		}
		else
		{
			Menu_Spy_Color();
			current_menu = MENU_SPY_COLOR;
		}
		menu_displaytime = 0;
		pev->impulse = 0;
		return;
	}
	if ( input == 3 )
		TeamFortress_SpyFeignDeath( FALSE );
	else if ( input == 4 )
		Spy_RemoveDisguise();
	else if ( input != 5 || ( !undercover_team && !undercover_skin ) )
		return;
	ResetMenu();
	pev->impulse = 0;
}

void CBasePlayer::Menu_Spy_Color_Input( float input )
{
	if ( input >= 1 && input <= number_of_teams )
	{
		TeamFortress_SpyChangeColor( (int)input );
		ResetMenu();
		pev->impulse = 0;
	}
}

void CBasePlayer::Menu_Engineer()
{
	const char* dispenser = has_dispenser ? "7. #Menu_DetDisp \n" :
		( ammo_cells >= BUILD_COST_DISPENSER ? "1. #Menu_BuildDisp \n" : "\n" );
	const char* sentry = has_sentry ? "8. #Menu_DetSentry \n" :
		( ammo_cells >= BUILD_COST_SENTRYGUN ? "2. #Menu_BuildSentry \n" : "\n" );
	sprintf( cbuf, "%s%s%s%s%s%s%s", "#Action \n", dispenser, sentry,
		has_sentry ? "3. #Menu_DismantleSentry \n" : "\n",
		has_sentry ? "4. #Menu_RotateSentry \n" : "\n",
		has_dispenser ? "9. #Menu_DismantleDisp \n" : "\n", "5. #Nothing \n\n" );
	MenuPrint( this, cbuf, 0xFFFF, 0 );
}

void CBasePlayer::Player_Menu()
{
	if ( !m_bDisplayedMOTD )
	{
		g_pGameRules->SendMOTDToClient( edict() );
		Menu_Team();
		m_bDisplayedMOTD = TRUE;
	}
	if ( current_menu == 0 || current_menu == MENU_TEAM )
	{
		if ( pev->playerclass == 0 && pev->team == 0 && pev->iuser1 == 0 )
		{
			if ( m_flSafetyCheckTime == 0 )
				m_flSafetyCheckTime = gpGlobals->time + 2;
			else if ( gpGlobals->time > m_flSafetyCheckTime )
			{
				Menu_Team();
				m_flSafetyCheckTime = 0;
				return;
			}
		}
		if ( current_menu == 0 )
		{
			menu_refreshtime = 0;
			return;
		}
	}
	else
		m_flSafetyCheckTime = 0;

	if ( menu_refreshtime > gpGlobals->time )
	{
		menu_refreshtime = min( menu_refreshtime, gpGlobals->time + 2 );
		return;
	}
	menu_refreshtime = gpGlobals->time + 2;
	if ( current_menu == MENU_INTRO )
	{
		Menu_Intro();
		if ( menu_displaytime > 4 )
			ResetMenu();
		else
			menu_displaytime += 1;
	}
	else if ( current_menu == MENU_CLASSHELP )
	{
		Menu_ClassHelp();
		if ( menu_displaytime > 7.5 )
			ResetMenu();
		else
			menu_displaytime += 1;
	}
	else if ( current_menu == MENU_SPY )
		Menu_Spy();
	else if ( current_menu == MENU_SPY_SKIN )
		Menu_Spy_Skin();
	else if ( current_menu == MENU_SPY_COLOR )
		Menu_Spy_Color();
	else if ( current_menu == MENU_ENGINEER )
		Menu_Engineer();
	else if ( current_menu == MENU_ENGINEER_FIX_SENTRYGUN )
		Menu_EngineerFix_SentryGun();
	else if ( current_menu == MENU_DISPENSER )
		Menu_Dispenser();
	else if ( current_menu == MENU_REPEATHELP )
	{
		Menu_RepeatHelp();
		current_menu = MENU_DEFAULT;
	}
	else if ( current_menu != MENU_CLASS && current_menu != MENU_TEAM )
	{
		current_menu = 0;
		menu_refreshtime = 0;
	}
}

void CBasePlayer::Menu_Engineer_Input( float input )
{
	if ( input == 1 )
	{
		if ( ammo_cells < BUILD_COST_DISPENSER || has_dispenser == 1 )
			return;
		TeamFortress_EngineerBuild( BUILD_DISPENSER );
	}
	else if ( input == 2 )
	{
		if ( ammo_cells < BUILD_COST_SENTRYGUN || has_sentry == 1 )
			return;
		TeamFortress_EngineerBuild( BUILD_SENTRYGUN );
	}
	else if ( ( input == 3 || input == 4 || input == 6 ) && has_sentry == 1 )
	{
		ResetMenu();
		pev->impulse = 0;
		CBaseEntity* sentry = NULL;
		while ( ( sentry = UTIL_FindEntityByClassname( sentry, "building_sentrygun" ) ) != NULL )
		{
			if ( (CBaseEntity*)sentry->real_owner == this )
				break;
		}
		if ( !sentry )
			return;
		if ( input == 4 || input == 6 )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, input == 4 ? "#Sentry_rotate" : "#Sentry_rotate180" );
			const int rotation = input == 4 ? 45 : 180;
			sentry->m_iRightBound = (int)anglemod( (float)( sentry->m_iRightBound + rotation ) );
			sentry->m_iLeftBound = (int)anglemod( (float)( sentry->m_iLeftBound + rotation ) );
			return;
		}
		if ( ( pev->origin - sentry->pev->origin ).Length() > 128 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Sentry_DistDismantle" );
			return;
		}
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Sentry_dismantle" );
		const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Sentry_Dismantle\"\n",
			STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ), team );
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, sentry->edict(), sentry->m_usBuildingEvent,
			0, sentry->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		ammo_cells += 65 * sentry->weapon;
		TeamFortress_CheckClassStats();
		dremove( sentry->m_pOtherSection );
		dremove( sentry );
		has_sentry = 0;
		return;
	}
	else if ( input == 7 )
	{
		if ( has_dispenser != 1 )
			return;
		DestroyBuilding( this, "building_dispenser" );
	}
	else if ( input == 8 )
	{
		if ( has_sentry != 1 )
			return;
		DestroyBuilding( this, "building_sentrygun" );
	}
	else if ( input == 9 )
	{
		if ( has_dispenser != 1 )
			return;
		ResetMenu();
		pev->impulse = 0;
		CBaseEntity* dispenser = NULL;
		while ( ( dispenser = UTIL_FindEntityByClassname( dispenser, "building_dispenser" ) ) != NULL )
		{
			if ( (CBaseEntity*)dispenser->real_owner == this )
				break;
		}
		if ( !dispenser )
			return;
		if ( ( pev->origin - dispenser->pev->origin ).Length() > 128 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Dispenser_DistDismantle" );
			return;
		}
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Dispenser_dismantle" );
		const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dispenser_Dismantle\"\n",
			STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ), team );
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, dispenser->edict(), dispenser->m_usBuildingEvent,
			0, dispenser->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		ammo_cells += BUILD_COST_DISPENSER / 2;
		TeamFortress_CheckClassStats();
		dremove( dispenser );
		has_dispenser = 0;
		return;
	}
	else if ( input == 10 || input == 11 )
	{
		const int type = input == 10 ? BUILD_TELEPORTER_ENTRANCE : BUILD_TELEPORTER_EXIT;
		if ( ( input == 10 && has_entry_teleporter != 1 ) || ( input == 11 && has_exit_teleporter != 1 ) )
			return;
		ResetMenu();
		pev->impulse = 0;
		CTFTeleporter* teleporter = NULL;
		while ( ( teleporter = (CTFTeleporter*)UTIL_FindEntityByClassname( teleporter, "building_teleporter" ) ) != NULL )
		{
			if ( (CBaseEntity*)teleporter->real_owner == this && teleporter->m_iType == type )
				break;
		}
		if ( !teleporter )
			return;
		if ( !( ( pev->origin - teleporter->pev->origin ).Length() <= 128 ) )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Teleporter_DistDismantle" );
			return;
		}
		CTFTeleporter* match = teleporter->FindMatch();
		if ( teleporter->m_pPlayer || ( match && match->m_pPlayer ) )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Teleporter_UseDismantle" );
			return;
		}
		ClientPrint( pev, HUD_PRINTNOTIFY,
			input == 10 ? "#Teleporter_entrance_dismantle" : "#Teleporter_exit_dismantle" );
		const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
		UTIL_LogPrintf( input == 10 ? "\"%s<%i><%s><%s>\" triggered \"Teleporter_Entrance_Dismantle\"\n" :
			"\"%s<%i><%s><%s>\" triggered \"Teleporter_Exit_Dismantle\"\n",
			STRING( pev->netname ), GETPLAYERUSERID( edict() ), GETPLAYERAUTHID( edict() ), team );
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, teleporter->edict(), teleporter->m_usBuildingEvent,
			0, teleporter->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		ammo_cells += BUILD_COST_TELEPORTER / 2;
		TeamFortress_CheckClassStats();
		dremove( teleporter );
		if ( input == 10 )
			has_entry_teleporter = 0;
		else
			has_exit_teleporter = 0;
		return;
	}
	else if ( input != 5 )
		return;
	ResetMenu();
	pev->impulse = 0;
}

void CBasePlayer::Menu_EngineerFix_Dispenser()
{
	MenuPrint( this, "#Action  \n1. #Menu_AmmoDisp \n2. #Menu_ArmorDisp \n3. #Menu_RepairDisp \n"
		"4. #Menu_DismantleDisp \n5. #Nothing \n\n", 0xFFFF, 0 );
}

void CBasePlayer::Menu_EngineerFix_Dispenser_Input( float )
{
}

void CBasePlayer::Menu_EngineerFix_SentryGun()
{
	if ( !building )
		return;
	if ( building->weapon > 2 || ammo_cells < BUILD_COST_SENTRYGUN )
		MenuPrint( this, "#Action  \n1. #Menu_AmmoSentry \n\n3. #Menu_RepairSentry \n"
			"4. #Menu_DismantleSentry \n5. #Nothing \n\n6. #Menu_RotateSentry \n", 0xFFFF, 0 );
	else
		MenuPrint( this, "#Action  \n1. #Menu_AmmoSentry \n2. #Menu_UpgradeSentry \n3. #Menu_RepairSentry \n"
			"4. #Menu_DismantleSentry \n5. #Nothing \n\n6. #Menu_RotateSentry \n", 0xFFFF, 0 );
}

void CBasePlayer::Menu_EngineerFix_SentryGun_Input( float input )
{
	if ( !building )
	{
		ResetMenu();
		return;
	}
	if ( !building->real_owner || !building->real_owner->has_sentry )
	{
		ResetMenu();
		building = NULL;
		return;
	}
	if ( input == 1 )
	{
		float amount = min( 40.0f, (float)ammo_shells );
		amount = min( amount, (float)( building->maxammo_shells - building->ammo_shells ) );
		ammo_shells = (int)( ammo_shells - amount );
		building->ammo_shells = (int)( building->ammo_shells + amount );
		if ( building->weapon == 3 )
		{
			amount = min( 20.0f, (float)ammo_rockets );
			amount = min( amount, (float)( building->maxammo_rockets - building->ammo_rockets ) );
			ammo_rockets = (int)( ammo_rockets - amount );
			building->ammo_rockets = (int)( building->ammo_rockets + amount );
		}
	}
	else if ( building->weapon <= 2 && input == 2 && ammo_cells >= BUILD_COST_SENTRYGUN )
	{
		ammo_cells -= BUILD_COST_SENTRYGUN;
		((CTFSentrygun*)(CBaseEntity*)building)->Upgrade();
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Sentry_upgrade", UTIL_dtos1( building->weapon ) );
	}
	else if ( input == 3 )
	{
		float metal = ( building->pev->max_health - building->pev->health ) / 5;
		metal = min( metal, (float)ammo_cells );
		ammo_cells = (int)( ammo_cells - metal );
		building->pev->health += metal * 5;
	}
	else if ( input == 4 )
	{
		if ( building->real_owner )
		{
			if ( (CBaseEntity*)building->real_owner != this )
				return;
			building->real_owner->has_sentry = 0;
		}
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Sentry_dismantle" );
		ammo_cells += BUILD_COST_SENTRYGUN / 2;
		TeamFortress_CheckClassStats();
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, building->edict(), building->m_usBuildingEvent,
			0, building->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		dremove( building->m_pOtherSection );
		dremove( building );
	}
	else if ( input == 6 )
	{
		ClientPrint( pev, HUD_PRINTNOTIFY, "#Sentry_rotate" );
		building->m_iRightBound = (int)anglemod( (float)( building->m_iRightBound + 45 ) );
		building->m_iLeftBound = (int)anglemod( (float)( building->m_iLeftBound + 45 ) );
	}
	else if ( !( input >= 1 && input <= 6 ) )
		return;

	ResetMenu();
	pev->impulse = 0;
	building = NULL;
	if ( pev->armorvalue == 0 )
	{
		pev->armortype = 0;
		items &= ~( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 );
		armorclass = 0;
	}
}

void CBasePlayer::Menu_Input( float input )
{
	if ( current_menu == MENU_SPY )
		Menu_Spy_Input( input );
	else if ( current_menu == MENU_SPY_SKIN )
		Menu_Spy_Skin_Input( input );
	else if ( current_menu == MENU_SPY_COLOR )
		Menu_Spy_Color_Input( input );
	else if ( current_menu == MENU_ENGINEER )
		Menu_Engineer_Input( input );
	else if ( current_menu == MENU_ENGINEER_FIX_MORTAR )
		Menu_EngineerFix_Mortar_Input( input );
	else if ( current_menu == MENU_ENGINEER_FIX_SENTRYGUN )
		Menu_EngineerFix_SentryGun_Input( input );
	else if ( current_menu == MENU_CLASSHELP )
		ResetMenu();
}

void CBasePlayer::Menu_EngineerFix_Mortar()
{
}

void CBasePlayer::Menu_EngineerFix_Mortar_Input( float input )
{
	if ( input >= 1 && input <= 5 )
	{
		ResetMenu();
		pev->impulse = 0;
		if ( pev->armorvalue == 0 )
		{
			pev->armortype = 0;
			items &= ~( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 );
			armorclass = 0;
		}
	}
}

void CBasePlayer::Menu_Dispenser()
{
	MenuPrint( this, "#Menu_Dispenser", 0xFFFF, 0 );
}

void CBasePlayer::Menu_Dispenser_Input( float )
{
}
