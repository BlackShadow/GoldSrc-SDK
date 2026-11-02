#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

void CBasePlayer::TeamFortress_InitStatusBar()
{
	m_flStatusBarDisappearDelay = 0;
	m_SbarString2[0] = 0;
	m_SbarString1[0] = 0;
	m_SbarString0[0] = 0;
}

CBaseEntity* CBasePlayer::GetTeleporter( int type )
{
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityByClassname( pEntity, "building_teleporter" ) ) != NULL )
	{
		if ( pEntity->real_owner->pev == pev && ( (CTFTeleporter*)pEntity )->m_iType == type )
			return pEntity;
	}
	return NULL;
}

void CBasePlayer::TeamFortress_UpdateStatusBar()
{
	int state[SBAR_END] = { 0 };
	char text[3][SBAR_STRING_SIZE];
	char teleporters[128];
	text[0][0] = 0;
	text[1][0] = 0;
	strcpy( text[2], m_SbarString2 );
	teleporters[0] = 0;
	if ( cb_prematch_time > gpGlobals->time || respawn_time > gpGlobals->time )
	{
		float deadline = cb_prematch_time;
		if ( cb_prematch_time <= gpGlobals->time )
		{
			deadline = respawn_time;
			strcpy( text[2], "#Sbar_dead" );
		}
		else
		{
			strcpy( text[2], "#Sbar_prematch" );
			if ( tfc_clanbattle.value != 0 )
			{
				strcpy( text[1], "#Sbar_battleid" );
				state[SBAR_TEAM1SCORE] = tf_id;
			}
			state[SBAR_TEAM2SCORE] = cease_fire;
		}
		int seconds = (int)ceilf( deadline - gpGlobals->time ) % 60;
		if ( seconds == 1 )
			state[SBAR_ID_TARGETARMOR] = 1;
		else
			state[SBAR_ID_TARGETNAME] = seconds;
		int minutes = (int)( ceilf( deadline - gpGlobals->time ) / 60.0 );
		if ( minutes == 1 )
			state[SBAR_ENG_BUILDINGAMMO] = 1;
		else
			state[SBAR_ID_TARGETHEALTH] = minutes;
	}
	else
	{
		UTIL_MakeVectors( pev->v_angle + pev->punchangle );
		Vector source = EyePosition();
		Vector end = source + gpGlobals->v_forward * 2048;
		TraceResult trace;
		UTIL_TraceLine( source, end, dont_ignore_monsters, edict(), &trace );
		if ( trace.flFraction != 1 )
		{
			if ( FNullEnt( trace.pHit ) )
			{
				if ( m_flStatusBarDisappearDelay > gpGlobals->time )
				{
					state[SBAR_ID_TARGETNAME] = m_izSBarState[SBAR_ID_TARGETNAME];
					state[SBAR_ID_TARGETHEALTH] = m_izSBarState[SBAR_ID_TARGETHEALTH];
					state[SBAR_ID_TARGETARMOR] = m_izSBarState[SBAR_ID_TARGETARMOR];
				}
			}
			else
			{
				CBaseEntity* pTarget = CBaseEntity::Instance( trace.pHit );
				BOOL identified = TRUE;
				if ( pTarget->Classify() == CLASS_PLAYER )
				{
					if ( pTarget->is_feigning && !IsAlly( pTarget ) && pev->iuser1 == 0 )
						identified = FALSE;
					else if ( IsAlly( pTarget ) || ( source - trace.vecEndPos ).Length() <= 1024 )
					{
						if ( pev->iuser1 == 0 && team_no && pTarget->undercover_target &&
							team_no != pTarget->team_no && pTarget->pev->playerclass == PC_SPY )
							pTarget = pTarget->undercover_target;
					}
					else if ( pev->iuser1 == 0 )
						identified = FALSE;
					if ( identified )
					{
						if ( tfc_playerid.value != 2 &&
							( tfc_playerid.value != 1 || IsAlly( pTarget ) || pev->iuser1 != 0 ) )
							strcpy( text[2], "1 %p1\n2    Health: %i2%%\n3  Armor: %i3%%" );
						else
							strcpy( text[2], " " );
						state[SBAR_ID_TARGETNAME] = pTarget->entindex();
						m_hLastIDTarget = pTarget;
					}
				}
				else
				{
					const char* format = NULL;
					if ( FClassnameIs( pTarget->pev, "building_sentrygun" ) )
						format = "1 Sentry gun built by: %p1\n2    Health: %i2%%\n3  Armor: %i3%%";
					else if ( FClassnameIs( pTarget->pev, "building_dispenser" ) )
						format = "1 Dispenser built by: %p1\n2    Health: %i2%%\n3  Armor: %i3%%";
					else if ( FClassnameIs( pTarget->pev, "building_teleporter" ) )
					{
						if ( ( (CTFTeleporter*)pTarget )->m_iType == BUILD_TELEPORTER_ENTRANCE )
							format = "1 Teleporter entrance built by: %p1\n2    Health: %i2%%\n3  Armor: %i3%%";
						else
							format = "1 Teleporter exit built by: %p1\n2    Health: %i2%%\n3  Armor: %i3%%";
					}
					if ( !format )
						identified = FALSE;
					else
					{
						state[SBAR_ID_TARGETNAME] = pTarget->real_owner->entindex();
						if ( tfc_playerid.value != 2 &&
							( tfc_playerid.value != 1 || pev->iuser1 != 0 || IsAlly( pTarget ) ) )
							strcpy( text[2], format );
						else
							strcpy( text[2], " " );
					}
				}
				if ( identified )
				{
					if ( IsAlly( pTarget ) || pev->playerclass == PC_MEDIC || pev->iuser1 != 0 )
					{
						state[SBAR_ID_TARGETHEALTH] = (int)( pTarget->pev->health / pTarget->pev->max_health * 100 );
						state[SBAR_ID_TARGETARMOR] = (int)( pTarget->pev->armorvalue / pTarget->maxarmor * 100 );
					}
					m_flStatusBarDisappearDelay = gpGlobals->time + 1.0;
				}
			}
			if ( state[SBAR_ID_TARGETNAME] < 0 || state[SBAR_ID_TARGETNAME] > gpGlobals->maxClients )
			{
				printf( "ERROR: SBAR_ID_TARGETNAME = %d\n", state[SBAR_ID_TARGETNAME] );
				state[SBAR_ID_TARGETNAME] = 0;
			}
		}
		if ( pev->playerclass == PC_SPY )
		{
			if ( is_undercover && !undercover_team && !undercover_skin )
				strcpy( text[1], "#Spy_disguising" );
			else if ( undercover_team || undercover_skin )
			{
				if ( undercover_team )
				{
					strcpy( text[1], "#Spy_disguised " );
					strcat( text[1], GetTeamName( undercover_team ) );
				}
				else
					strcpy( text[1], "#Spy_disguised" );
				strcat( text[1], " #" );
				strcat( text[1], sClassNames[undercover_skin ? undercover_skin : PC_SPY] );
			}
		}
		if ( pev->playerclass == PC_ENGINEER )
		{
			if ( has_sentry != 0 )
			{
				CBaseEntity* pSentry = NULL;
				while ( ( pSentry = UTIL_FindEntityByClassname( pSentry, "building_sentrygun" ) ) != NULL )
				{
					if ( (CBaseEntity*)pSentry->real_owner != this )
						continue;
					int health = (int)( pSentry->pev->health / pSentry->pev->max_health * 100 );
					health = health < 0 ? 0 : health > 100 ? 100 : health;
					sprintf( text[1], "#Sentry_sbar %d", health );
					strcat( text[1], "%%\n4 ammo: %i4%%\n0" );
					if ( pSentry->ammo_shells )
					{
						state[SBAR_ENG_BUILDINGAMMO] = (int)( (float)pSentry->ammo_shells / pSentry->maxammo_shells * 100 );
						if ( !pSentry->ammo_rockets && pSentry->weapon == 3 )
							strcat( text[1], " #No_Rockets" );
					}
					else if ( !pSentry->ammo_rockets && pSentry->weapon == 3 )
						strcat( text[1], " #No_Ammo" );
					else
						strcat( text[1], " #No_Shells" );
				}
			}
			if ( has_entry_teleporter != 0 || has_exit_teleporter != 0 )
			{
				CBaseEntity* pEntry = GetTeleporter( BUILD_TELEPORTER_ENTRANCE );
				CBaseEntity* pExit = GetTeleporter( BUILD_TELEPORTER_EXIT );
				char number[32];
				if ( pEntry )
				{
					strcat( teleporters, "#Teleporter_entry_sbar " );
					int health = (int)( pEntry->pev->health / pEntry->pev->max_health * 100 );
					health = health < 0 ? 0 : health > 100 ? 100 : health;
					sprintf( number, "%d", health );
					strncat( teleporters, number, 127 );
					strcat( teleporters, "%%\n0" );
				}
				if ( pExit )
				{
					if ( teleporters[0] )
						strcat( teleporters, "   " );
					strcat( teleporters, "#Teleporter_exit_sbar " );
					int health = (int)( pExit->pev->health / pExit->pev->max_health * 100 );
					health = health < 0 ? 0 : health > 100 ? 100 : health;
					sprintf( number, "%d", health );
					strncat( teleporters, number, 127 );
					strcat( teleporters, "%%\n0" );
				}
			}
			if ( teleporters[0] )
				strcpy( text[text[1][0] ? 0 : 1], teleporters );
			if ( is_building )
				strcpy( text[text[1][0] ? 0 : 1], "#Engineer_building" );
		}
		if ( pev->playerclass == PC_DEMOMAN && is_detpacking )
			strcpy( text[1], "#Demoman_detpacking" );
	}
	if ( respawn_time != 0 && gpGlobals->time > respawn_time && strcmp( text[2], "#Sbar_dead" ) == 0 )
		text[2][0] = 0;
	BOOL changed = FALSE;
	char* oldText[3] = { m_SbarString0, m_SbarString1, m_SbarString2 };
	for ( int line = 0; line < 3; line++ )
	{
		if ( strcmp( text[line], oldText[line] ) == 0 )
			continue;
		changed = TRUE;
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusText, NULL, pev );
			WRITE_BYTE( line );
			WRITE_STRING( text[line] );
		MESSAGE_END();
		strcpy( oldText[line], text[line] );
	}
	for ( int i = 1; i < SBAR_END; i++ )
	{
		if ( state[i] == m_izSBarState[i] && !changed )
			continue;
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusValue, NULL, pev );
			WRITE_BYTE( i );
			WRITE_SHORT( state[i] );
		MESSAGE_END();
		m_izSBarState[i] = state[i];
	}
}
