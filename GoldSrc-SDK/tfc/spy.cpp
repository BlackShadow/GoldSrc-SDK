#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

const struct
{
	char* szWeaponModel;
	char* szAnimExt;
} SpyFakeWeaponArray[] =
{
	{ "", "" },
	{ "models/p_nailgun.mdl", "mp5" },
	{ "models/p_sniper.mdl", "shotgun" },
	{ "models/p_srpg.mdl", "rpg" },
	{ "models/p_glauncher.mdl", "mp5" },
	{ "models/p_snailgun.mdl", "mp5" },
	{ "models/p_mini.mdl", "ac" },
	{ "models/p_egon.mdl", "egon" },
	{ "models/p_knife.mdl", "knife" },
	{ "models/p_9mmhandgun.mdl", "onehanded" },
	{ "", "" }
};

BOOL CBasePlayer::CanFeign()
{
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 64 ) ) != NULL )
	{
		if ( ( ( pEntity->pev->flags & FL_CLIENT ) && pEntity->IsAlive() && pEntity != this ) ||
			pEntity->Classify() == CLASS_MACHINE )
			return FALSE;
	}
	return TRUE;
}

void CBasePlayer::TeamFortress_SpyFeignDeath( BOOL bSilent )
{
	if ( !( m_flNextAttack <= 0 ) )
		return;
	m_flNextAttack = 0.5f;
	if ( is_feigning )
	{
		if ( !CanFeign() )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Feign_unabletogetup" );
			return;
		}
		is_feigning = FALSE;
		pev->velocity = g_vecZero;
		if ( pev->flags & FL_DUCKING )
			UTIL_SetSize( pev, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX );
		else
			UTIL_SetSize( pev, VEC_HULL_MIN, VEC_HULL_MAX );
		pev->view_ofs = Vector( 0, 0, 28 );
		pev->movetype = MOVETYPE_WALK;
		pev->deadflag = DEAD_NO;
		pev->flags &= ~FL_ONGROUND;
		tfstate &= ~TFSTATE_CANT_MOVE;
		TeamFortress_SetSpeed();
		if ( m_pActiveItem )
			m_pActiveItem->Deploy();
		pev->framerate = -1;
		m_iRespawnFrames = 0;
		SetAnimation( PLAYER_FEIGN );
		pev->iuser3 = 0;
		return;
	}
	if ( is_unableto_spy_or_teleport )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Feign_unable" );
		return;
	}
	if ( !( pev->flags & FL_ONGROUND ) && pev->waterlevel == 0 )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Feign_air" );
		return;
	}
	if ( !CanFeign() )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Feign_onspy" );
		return;
	}
	if ( pev->flags & FL_DUCKING )
	{
		Vector origin = pev->origin;
		if ( pev->flags & FL_ONGROUND )
			origin = origin + Vector( 0, 0, 18 );
		TraceResult trace;
		UTIL_TraceHull( origin, origin, dont_ignore_monsters, human_hull, edict(), &trace );
		if ( trace.fStartSolid )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Feign_noroom" );
			return;
		}
		if ( pev->flags & FL_ONGROUND )
			pev->origin = pev->origin + Vector( 0, 0, 18 );
	}
	m_iRespawnFrames = 0;
	SetAnimation( PLAYER_FEIGN );
	tfstate |= TFSTATE_CANT_MOVE;
	immune_to_check = gpGlobals->time + 5;
	TeamFortress_SetSpeed();
	pev->deadflag = DEAD_DISCARDBODY + 1;
	is_feigning = TRUE;
	invisible_finished = 0;
	TeamFortress_SetSkin();
	if ( m_pActiveItem )
	{
		m_pActiveItem->Holster();
		pev->viewmodel = 0;
		pev->weaponmodel = 0;
	}
	UTIL_SetSize( pev, VEC_HULL_MIN, Vector( 16, 16, -16 ) );
	pev->view_ofs = Vector( 0, 0, 4 );
	pev->movetype = MOVETYPE_TOSS;
	pev->flags &= ~FL_ONGROUND;
	if ( pev->velocity.z < 10 )
		pev->velocity.z += RANDOM_FLOAT( 0, 300 );
	if ( !bSilent )
		DeathSound();
	pev->angles.x = 0;
	pev->angles.z = 0;
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem->pev->owner != edict() )
			continue;
		if ( !( pItem->goal_activation & TFGI_KEEP ) || has_disconnected )
			tfgoalitem_RemoveFromPlayer( pItem, this, 0 );
		if ( CTF_Map )
		{
			if ( pItem->goal_no == 1 )
			{
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s LOST the BLUE flag!\n", STRING( pev->netname ) ) );
				const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
				const char* auth = GETPLAYERAUTHID( edict() );
				int id = GETPLAYERUSERID( edict() );
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dropped_Blue_Flag\"\n", STRING( pev->netname ), id, auth, team );
			}
			else if ( pItem->goal_no == 2 )
			{
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s LOST the RED flag!\n", STRING( pev->netname ) ) );
				const char* team = team_no ? GetTeamName( team_no ) : "SPECTATOR";
				const char* auth = GETPLAYERAUTHID( edict() );
				int id = GETPLAYERUSERID( edict() );
				UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dropped_Red_Flag\"\n", STRING( pev->netname ), id, auth, team );
			}
		}
	}
	pev->iuser3 = 1;
}

void CBasePlayer::Spy_DisguiseExternalWeaponModel()
{
	if ( pev->playerclass == PC_SPY && is_undercover &&
		undercover_skin >= PC_SCOUT && undercover_skin <= PC_ENGINEER && !is_feigning )
	{
		pev->weaponmodel = MAKE_STRING( SpyFakeWeaponArray[undercover_skin].szWeaponModel );
		strcpy( m_szAnimExtention, SpyFakeWeaponArray[undercover_skin].szAnimExt );
		m_iCurrentAnimationState = 0;
	}
}

void CBasePlayer::Spy_ResetExternalWeaponModel()
{
	if ( !is_feigning )
	{
		pev->weaponmodel = MAKE_STRING( m_pszSavedWeaponModel );
		strcpy( m_szAnimExtention, m_szSavedAnimExtention );
		m_iCurrentAnimationState = 0;
	}
}

void CBasePlayer::TeamFortress_SpyGoUndercover()
{
	if ( ( pev->effects & ( EF_BRIGHTFIELD | EF_DIMLIGHT ) ) || is_unableto_spy_or_teleport )
		ClientPrint( pev, HUD_PRINTCENTER, "#Spy_unable" );
	else if ( is_undercover == 2 )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_stop" );
		CBaseEntity* pTimer = FindTimer( TF_TIMER_DISGUISE );
		if ( pTimer )
			UTIL_Remove( pTimer );
		is_undercover = FALSE;
	}
	else
		current_menu = MENU_SPY;
}

void CBasePlayer::TeamFortress_SpyCalcName()
{
	CBaseEntity* pOldTarget = undercover_target;
	undercover_target = NULL;
	if ( !undercover_team )
		return;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer != pOldTarget && pPlayer && pPlayer->team_no == undercover_team &&
			pPlayer->pev->playerclass == undercover_skin )
		{
			undercover_target = pPlayer;
			return;
		}
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->team_no == undercover_team )
		{
			undercover_target = pPlayer;
			return;
		}
	}
}

void CBasePlayer::Spy_RemoveDisguise()
{
	if ( pev->playerclass != PC_SPY )
		return;
	if ( undercover_team || undercover_skin )
		ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_Lost" );
	undercover_team = 0;
	undercover_skin = 0;
	is_undercover = FALSE;
	immune_to_check = gpGlobals->time + 10;
	TeamFortress_SetSkin();
	TeamFortress_SpyCalcName();
	Spy_ResetExternalWeaponModel();
	if ( current_menu == MENU_SPY || current_menu == MENU_SPY_SKIN || current_menu == MENU_SPY_COLOR )
		ResetMenu();
	CBaseEntity* pTimer = FindTimer( TF_TIMER_DISGUISE );
	if ( pTimer )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_stop" );
		UTIL_Remove( pTimer );
	}
}

void CBasePlayer::TeamFortress_SpyChangeColor( int iTeamNo )
{
	if ( team_no == iTeamNo )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_resetteam" );
		undercover_team = 0;
		if ( !undercover_skin )
			is_undercover = FALSE;
		TeamFortress_SetSkin();
	}
	else if ( !m_iTeamToDisguiseAs && iTeamNo != undercover_team )
	{
		if ( ( pev->effects & ( EF_BRIGHTFIELD | EF_DIMLIGHT ) ) || is_unableto_spy_or_teleport )
			ClientPrint( pev, HUD_PRINTCENTER, "#Spy_unable" );
		else
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_start" );
			is_undercover = 2;
			CBaseEntity* pTimer = CreateTimer( TF_TIMER_DISGUISE );
			pTimer->SetThink( &CBaseEntity::Timer_SpyUndercoverThink );
			pTimer->pev->nextthink = gpGlobals->time + 4;
			pTimer->team_no = iTeamNo;
		}
	}
}

void CBasePlayer::TeamFortress_SpyChangeSkin( int iClass )
{
	int delay = 4;
	int team = m_iTeamToDisguiseAs;
	if ( team )
	{
		if ( team != undercover_team )
			delay = team != team_no ? 8 : 4;
		if ( iClass == PC_SPY || iClass == undercover_skin )
			m_iTeamToDisguiseAs = 0;
		TeamFortress_SpyChangeColor( team );
		m_iTeamToDisguiseAs = 0;
	}
	if ( iClass == PC_SPY && team == team_no )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_resetclass" );
		undercover_skin = 0;
		undercover_team = 0;
		is_undercover = FALSE;
		Spy_ResetExternalWeaponModel();
		TeamFortress_SetSkin();
		return;
	}
	if ( iClass == undercover_skin )
		return;
	if ( ( pev->effects & ( EF_BRIGHTFIELD | EF_DIMLIGHT ) ) || is_unableto_spy_or_teleport )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Spy_unable" );
		return;
	}
	ClientPrint( pev, HUD_PRINTCENTER, "#Disguise_start" );
	is_undercover = 2;
	CBaseEntity* pTimer = FindTimer( TF_TIMER_DISGUISE );
	if ( pTimer )
		UTIL_Remove( pTimer );
	pTimer = CreateTimer( TF_TIMER_DISGUISE );
	pTimer->SetThink( &CBaseEntity::Timer_SpyUndercoverThink );
	pTimer->pev->nextthink = gpGlobals->time + (float)delay;
	pTimer->pev->skin = iClass;
	pTimer->team_no = team;
	TeamFortress_SetSkin();
}

void CBasePlayer::TeamFortress_SpyDisguiseEnemy( BOOL bEnemy, int iClass )
{
	int team;
	if ( !bEnemy )
		team = team_no;
	else if ( number_of_teams > 1 )
	{
		do
		{
			team = RANDOM_LONG( 1, (int)number_of_teams );
		} while ( team == team_no );
	}
	else
		team = 1;
	TeamFortress_SpyDisguise( team, iClass );
}

void CBasePlayer::TeamFortress_SpyDisguise( int iTeam, int iClass )
{
	static float flLastDisguesChange = 0;
	if ( flLastDisguesChange > gpGlobals->time )
		flLastDisguesChange = 0;
	if ( !( gpGlobals->time - flLastDisguesChange >= 1 ) )
		return;
	flLastDisguesChange = gpGlobals->time;
	if ( !( iTeam <= number_of_teams ) || iTeam <= 0 || iClass < PC_SCOUT || iClass > PC_ENGINEER )
		return;
	if ( ( pev->effects & ( EF_BRIGHTFIELD | EF_DIMLIGHT ) ) || is_unableto_spy_or_teleport )
		ClientPrint( pev, HUD_PRINTCENTER, "#Spy_unable" );
	else
	{
		m_iTeamToDisguiseAs = iTeam;
		TeamFortress_SpyChangeSkin( iClass );
	}
}
