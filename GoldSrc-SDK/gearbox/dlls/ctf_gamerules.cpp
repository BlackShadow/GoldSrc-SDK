#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "ctf_gamerules.h"
#include "game.h"
#include "ofctf.h"
#include "ofctfitems.h"
#include "cdll_dll.h"
#include "pm_shared.h"

extern int gmsgGameMode;
extern int gmsgDeathMsg;
extern int gmsgTeamFull;
extern int gmsgPlayerBrowse;
extern int gmsgHudColor;
extern int gmsgOldWeapon;
extern int gmsgVGUIMenu;
extern int gmsgStatsPlayer;
extern int gmsgStatsInfo;
extern int gmsgSpectator;
extern int gmsgScoreInfo;
extern int gmsgTeamInfo;
extern int gmsgCTFScore;
extern int gmsgSetMenuTeam;
extern DLL_GLOBAL BOOL g_fGameOver;
extern char* pszPlayerIPs[64];

static const char team_names[3][16] =
{
	"", "Black Mesa", "Opposing Force"
};

static const char team_chars[2][6][16] =
{
	{ "ctf_barney", "cl_suit", "ctf_gina", "ctf_gordon", "otis", "ctf_scientist" },
	{ "beret", "drill", "grunt", "recruit", "shephard", "tower" }
};

static int team_scores[2];
static int team_count[2];

CHalfLifeCTFplay::CHalfLifeCTFplay()
{
	team_scores[0] = 0;
	team_scores[1] = 0;
	team_count[0] = 0;
	team_count[1] = 0;
	m_DisableDeathMessages = FALSE;
	m_DisableDeathPenalty = FALSE;
	m_fRefreshScores = FALSE;
	m_iStatsPhase = 0;
	RecountTeams();
}

BOOL CHalfLifeCTFplay::IsCTF()
{
	return TRUE;
}

BOOL CHalfLifeCTFplay::IsTeamplay()
{
	return TRUE;
}

const char* CHalfLifeCTFplay::GetGameDescription()
{
	return "OpFor CTF";
}

void CHalfLifeCTFplay::UpdateGameMode( CBasePlayer* pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgGameMode, NULL, pPlayer->pev );
		WRITE_BYTE( 2 );
	MESSAGE_END();
}

const char* CHalfLifeCTFplay::SetDefaultPlayerTeam( CBasePlayer* pPlayer )
{
	const char* pTeamName = TeamWithFewestPlayers();
	int team = GetTeamIndex( pTeamName );
	BOOL kill = !pPlayer->pev->iuser1;
	ChangePlayerTeam( pPlayer, team_chars[team][RANDOM_LONG( 0, 5 )], kill, kill );
	return pTeamName;
}

int CHalfLifeCTFplay::PlayerRelationship( CBaseEntity* pPlayer, CBaseEntity* pTarget )
{
	if ( !pPlayer || !pTarget || !pTarget->IsPlayer() )
		return GR_NOTTEAMMATE;

	return ( (CBasePlayer*)pPlayer )->m_iTeamNum == ( (CBasePlayer*)pTarget )->m_iTeamNum;
}

BOOL CHalfLifeCTFplay::ShouldAutoAim( CBasePlayer* pPlayer, edict_t* target )
{
	CBaseEntity* pTarget = CBaseEntity::Instance( target );
	if ( pTarget && pTarget->IsPlayer() )
		return PlayerRelationship( pPlayer, pTarget ) != GR_TEAMMATE;

	return TRUE;
}

int CHalfLifeCTFplay::IPointsForKill( CBasePlayer* pAttacker, CBasePlayer* pKilled )
{
	if ( !pKilled )
		return 0;

	if ( pAttacker && pAttacker != pKilled && PlayerRelationship( pAttacker, pKilled ) == GR_TEAMMATE )
		return -1;

	return 1;
}

const char* CHalfLifeCTFplay::GetTeamID( CBaseEntity* pEntity )
{
	if ( pEntity && pEntity->pev )
		return pEntity->TeamID();

	return "";
}

const char* CHalfLifeCTFplay::GetIndexedTeamName( int teamIndex )
{
	if ( (unsigned int)teamIndex <= 1 )
		return team_names[teamIndex + 1];

	return "";
}

BOOL CHalfLifeCTFplay::IsValidTeam( const char* pTeamName )
{
	return GetTeamIndex( pTeamName ) != -1;
}

const char* CHalfLifeCTFplay::GetCharacterType( int iTeamNum, int iCharNum )
{
	if ( iCharNum <= 5 )
		return team_chars[iTeamNum][iCharNum];

	return "";
}

int CHalfLifeCTFplay::GetNumTeams()
{
	return 2;
}

BOOL CHalfLifeCTFplay::CanHaveAmmo( CBasePlayer* pPlayer, const char* pszAmmoName, int iMaxCarry )
{
	if ( !pszAmmoName )
		return FALSE;

	int ammo = pPlayer->GetAmmoIndex( pszAmmoName );
	return ammo >= 0 && pPlayer->AmmoInventory( ammo ) < iMaxCarry;
}

BOOL CHalfLifeCTFplay::TeamsBalanced()
{
	int players[3] = { 0, 0, 0 };
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->m_iTeamNum > 0 )
			players[pPlayer->m_iTeamNum]++;
	}

	return players[1] == players[2];
}

int CHalfLifeCTFplay::GetTeamIndex( const char* pTeamName )
{
	if ( !pTeamName || !*pTeamName )
		return -1;

	if ( !stricmp( pTeamName, team_names[1] ) )
		return 0;

	if ( !stricmp( pTeamName, team_names[2] ) )
		return 1;

	return -1;
}

BOOL CHalfLifeCTFplay::FPlayerCanTakeDamage( CBasePlayer* pPlayer, CBaseEntity* pAttacker )
{
	if ( pAttacker && pAttacker != pPlayer && PlayerRelationship( pPlayer, pAttacker ) == GR_TEAMMATE
		&& !friendlyfire.value )
		return FALSE;

	return CHalfLifeMultiplay::FPlayerCanTakeDamage( pPlayer, pAttacker );
}

const char* CHalfLifeCTFplay::TeamWithFewestPlayers()
{
	int players[3] = { 0, 0, 0 };
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->m_iTeamNum > 0 )
			players[pPlayer->m_iTeamNum]++;
	}

	int fewest = 100;
	const char* pTeamName = NULL;
	if ( players[1] < fewest )
	{
		fewest = players[1];
		pTeamName = team_names[1];
	}
	if ( players[2] < fewest )
		return team_names[2];

	if ( !pTeamName )
		return GetIndexedTeamName( 0 );

	return pTeamName;
}

void CHalfLifeCTFplay::RecountTeams()
{
	team_count[0] = 0;
	team_count[1] = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBaseEntity* pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;

		int team = GetTeamIndex( pPlayer->TeamID() );
		if ( team >= 0 )
			team_count[team]++;
	}
}

const char* GetTeamName( edict_t* pEntity )
{
	if ( g_pGameRules->IsCTF() )
		return team_names[( (CBasePlayer*)CBaseEntity::Instance( pEntity ) )->m_iTeamNum];

	if ( g_pGameRules->IsDeathmatch() && g_pGameRules->IsTeamplay() )
		return g_engfuncs.pfnInfoKeyValue( g_engfuncs.pfnGetInfoKeyBuffer( pEntity ), "model" );

	static char szTeam[256];
	sprintf( szTeam, "%i", GETPLAYERUSERID( pEntity ) );
	return szTeam;
}

void CHalfLifeCTFplay::SendTeamStatInfo( int iTeamNum )
{
	enum
	{
		STAT_POINTS,
		STAT_FRAGS,
		STAT_CTF_POINTS,
		STAT_OFFENSE,
		STAT_DEFENSE,
		STAT_SNIPER_KILLS,
		STAT_BARNACLE_KILLS,
		STAT_DEATHS,
		STAT_SUICIDES,
		STAT_DAMAGE,
		STAT_ACCEL_TIME,
		STAT_BACKPACK_TIME,
		STAT_HEALTH_TIME,
		STAT_SHIELD_TIME,
		STAT_JUMP_TIME,

		STAT_COUNT
	};

	const char* pszStatNames[STAT_COUNT] =
	{
		"Most Points",
		"Most Frags",
		"Most CTF Points",
		"Most Offense",
		"Most Defense",
		"Most Sniper Kills",
		"Most Barnacle Kills",
		"Most Deaths",
		"Most Suicides",
		"Most Damage",
		"Most Damage Powerup Time",
		"Most Backpack Powerup Time",
		"Most Health Powerup Time",
		"Most Shield Powerup Time",
		"Most Jumppack Powerup Time"
	};
	int iMost[STAT_COUNT] = { 0 };
	int iValues[STAT_ACCEL_TIME] = { 0 };
	float flPowerupTimes[STAT_COUNT - STAT_ACCEL_TIME] = { 0 };
	int iNumPlayers = 0;

	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pPlayer || !pPlayer->IsPlayer() || ( iTeamNum && pPlayer->m_iTeamNum != iTeamNum ) )
			continue;

		if ( !pPlayer->pev->netname || !STRING( pPlayer->pev->netname ) )
			continue;

		double flPoints = (double)pPlayer->m_iCTFScore + pPlayer->pev->frags;
		if ( flPoints > iValues[STAT_POINTS] )
		{
			iMost[STAT_POINTS] = i;
			iValues[STAT_POINTS] = (int)(float)flPoints;
		}

		if ( pPlayer->pev->frags > iValues[STAT_FRAGS] )
		{
			iMost[STAT_FRAGS] = i;
			iValues[STAT_FRAGS] = (int)pPlayer->pev->frags;
		}

		int iPlayerValues[STAT_ACCEL_TIME - STAT_CTF_POINTS] =
		{
			pPlayer->m_iCTFScore,
			pPlayer->m_iOffense,
			pPlayer->m_iDefense,
			pPlayer->m_iSnipeKills,
			pPlayer->m_iBarnacleKills,
			pPlayer->m_iDeaths,
			pPlayer->m_iSuicides,
			pPlayer->m_iMostDamage
		};
		for ( int j = STAT_CTF_POINTS; j < STAT_ACCEL_TIME; j++ )
		{
			if ( iPlayerValues[j - STAT_CTF_POINTS] > iValues[j] )
			{
				iMost[j] = i;
				iValues[j] = iPlayerValues[j - STAT_CTF_POINTS];
			}
		}

		float flPlayerTimes[STAT_COUNT - STAT_ACCEL_TIME] =
		{
			pPlayer->m_flAccelTime,
			pPlayer->m_flBackpackTime,
			pPlayer->m_flHealthTime,
			pPlayer->m_flShieldTime,
			pPlayer->m_flJumpTime
		};
		for ( int j = 0; j < STAT_COUNT - STAT_ACCEL_TIME; j++ )
		{
			if ( flPlayerTimes[j] > flPowerupTimes[j] )
			{
				iMost[STAT_ACCEL_TIME + j] = i;
				flPowerupTimes[j] = flPlayerTimes[j];
			}
		}

		iNumPlayers++;
		if ( iTeamNum )
			continue;

		int iAccel = (int)pPlayer->m_flAccelTime;
		int iBackpack = (int)pPlayer->m_flBackpackTime;
		int iHealth = (int)pPlayer->m_flHealthTime;
		int iShield = (int)pPlayer->m_flShieldTime;
		int iJump = (int)pPlayer->m_flJumpTime;
		int iFrags = (int)pPlayer->pev->frags;
		const char* pszTeam = GetTeamName( pPlayer->edict() );
		unsigned int iWONId = g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() );
		int iUserId = GETPLAYERUSERID( pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" scored \"%d\" (kills \"%d\") (deaths \"%d\") "
			"(suicides \"%d\") (ctf_points \"%d\") (ctf_offense \"%d\") (ctf_defense \"%d\") "
			"(snipe_kills \"%d\") (barnacle_kills \"%d\") (best_damage \"%d\") "
			"(accel_time \"%0d:%02d\") (ammo_time \"%0d:%02d\") (health_time \"%0d:%02d\") "
			"(shield_time \"%0d:%02d\") (jump_time \"%0d:%02d\")\n",
			STRING( pPlayer->pev->netname ), iUserId, iWONId, pszTeam, (int)(float)flPoints,
			iFrags, iPlayerValues[STAT_DEATHS - STAT_CTF_POINTS], iPlayerValues[STAT_SUICIDES - STAT_CTF_POINTS],
			iPlayerValues[STAT_CTF_POINTS - STAT_CTF_POINTS], iPlayerValues[STAT_OFFENSE - STAT_CTF_POINTS],
			iPlayerValues[STAT_DEFENSE - STAT_CTF_POINTS], iPlayerValues[STAT_SNIPER_KILLS - STAT_CTF_POINTS],
			iPlayerValues[STAT_BARNACLE_KILLS - STAT_CTF_POINTS], iPlayerValues[STAT_DAMAGE - STAT_CTF_POINTS],
			iAccel / 60, iAccel % 60, iBackpack / 60, iBackpack % 60,
			iHealth / 60, iHealth % 60, iShield / 60, iShield % 60, iJump / 60, iJump % 60 );
	}

	if ( iTeamNum == 1 || iTeamNum == 2 )
	{
		UTIL_LogPrintf( "// === Team %s Statistics ===\n", team_names[iTeamNum] );
	}
	else
	{
		UTIL_LogPrintf( "Team \"%s\" scored \"%d\" with \"%d\" players\n", team_names[1], teamscores[0], 0 );
		UTIL_LogPrintf( "Team \"%s\" scored \"%d\" with \"%d\" players\n", team_names[2], teamscores[1], 0 );
		UTIL_LogPrintf( "// === End-Game Overall Statistics ===\n" );
		if ( teamscores[0] == teamscores[1] )
		{
			UTIL_LogPrintf( "// %-25s : %s\n", "Winning Team", "Draw" );
			UTIL_LogPrintf( "World triggered \"Draw\"\n" );
		}
		else
		{
			const char* pszWinner = team_names[teamscores[0] > teamscores[1] ? 1 : 2];
			UTIL_LogPrintf( "// %-25s : %s\n", "Winning Team", pszWinner );
			UTIL_LogPrintf( "Team \"%s\" triggered \"Victory\"\n", pszWinner );
		}
	}

	for ( int iPage = 0; iPage < 2; iPage++ )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgStatsInfo );
			WRITE_BYTE( iTeamNum );
			WRITE_BYTE( GetWinningTeam() );
			WRITE_BYTE( iNumPlayers );
			WRITE_BYTE( iPage );

		int iFirst = iPage ? STAT_DEATHS : STAT_POINTS;
		int iLast = iPage ? STAT_COUNT : STAT_DEATHS;
		for ( int i = iFirst; i < iLast; i++ )
		{
			int iValue = i < STAT_ACCEL_TIME ? iValues[i] : (int)flPowerupTimes[i - STAT_ACCEL_TIME];
			BOOL fPowerup = i >= STAT_ACCEL_TIME;
			BOOL fPoints = i == STAT_OFFENSE || i == STAT_DEFENSE;
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( iMost[i] );
			if ( pPlayer && pPlayer->pev->netname && STRING( pPlayer->pev->netname ) )
			{
				char szBuf[21];
				strncpy( szBuf, STRING( pPlayer->pev->netname ), sizeof( szBuf ) - 1 );
				szBuf[sizeof( szBuf ) - 1] = 0;
				WRITE_STRING( szBuf );

				if ( i == STAT_BARNACLE_KILLS )
				{
					int iUserId = GETPLAYERUSERID( pPlayer->edict() );
					UTIL_LogPrintf( "// %-20s : \"%s<%i><%u><%s>\" with %d\n",
						pszStatNames[i], STRING( pPlayer->pev->netname ), iUserId, iValue );
				}
				else
				{
					const char* pszTeam = GetTeamName( pPlayer->edict() );
					unsigned int iWONId = g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() );
					int iUserId = GETPLAYERUSERID( pPlayer->edict() );
					if ( fPowerup )
					{
						UTIL_LogPrintf( "// %-40s : \"%s<%i><%u><%s>\" for %d:%02d\n",
							pszStatNames[i], STRING( pPlayer->pev->netname ), iUserId, iWONId,
							pszTeam, iValue / 60, iValue % 60 );
					}
					else
					{
						UTIL_LogPrintf( fPoints ? "// %-20s : \"%s<%i><%u><%s>\" with %d points\n"
							: "// %-20s : \"%s<%i><%u><%s>\" with %d\n",
							pszStatNames[i], STRING( pPlayer->pev->netname ), iUserId, iWONId, pszTeam, iValue );
					}
				}
			}
			else
			{
				WRITE_STRING( "*Nobody*" );
				if ( fPowerup )
				{
					UTIL_LogPrintf( "// %-40s : *Nobody* for %d:%02d\n", pszStatNames[i], iValue / 60, iValue % 60 );
				}
				else
				{
					UTIL_LogPrintf( fPoints ? "// %-20s : *Nobody* with %d points\n"
						: "// %-20s : *Nobody* with %d\n", pszStatNames[i], iValue );
				}
			}

			WRITE_SHORT( fPowerup ? (short)iValue : iValue );
		}
		MESSAGE_END();
	}
}

extern int gmsgHealth;
extern int gmsgCurWeapon;
extern int gmsgSetFOV;
extern int gmsgFlagTimer;

extern void respawn( entvars_t* pev, BOOL fCopyCorpse );

BOOL CHalfLifeCTFplay::ClientConnected( edict_t* pEntity, const char* pszName, const char* pszAddress,
		char szRejectReason[128] )
{
	m_fRefreshScores = TRUE;
	int players = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->IsPlayer() && pPlayer->m_iTeamNum > 0 )
			players++;
	}

	if ( players <= 1 )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( !pPlayer || !pPlayer->m_iItems )
				continue;

			if ( pPlayer->m_pFlag )
				( (CTFGoalFlag*)(CBaseEntity*)pPlayer->m_pFlag )->ReturnFlag();
			if ( pPlayer->m_iItems & CTF_POWERUP_MASK )
				RespawnPlayerCTFPowerups( pPlayer, TRUE );
			ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#CTFGameReset" );
		}
		ResetTeamScores();
	}
	return CHalfLifeMultiplay::ClientConnected( pEntity, pszName, pszAddress, szRejectReason );
}

void CHalfLifeCTFplay::PlayerThink( CBasePlayer* pPlayer )
{
	Vector vecSrc = pPlayer->GetGunPosition();
	UTIL_MakeVectors( pPlayer->pev->v_angle );
	double distance = pPlayer->m_iFOV ? 4096.0 : 1280.0;
	Vector vecEnd;
	vecEnd.x = vecSrc.x + distance * gpGlobals->v_forward.x;
	vecEnd.y = vecSrc.y + distance * gpGlobals->v_forward.y;
	vecEnd.z = vecSrc.z + distance * gpGlobals->v_forward.z;
	TraceResult tr;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pPlayer->edict(), &tr );
	if ( gmsgPlayerBrowse && tr.flFraction < 1.0f && pPlayer->m_iLastPlayerTrace != ENTINDEX( tr.pHit ) )
	{
		CBasePlayer* pTarget = (CBasePlayer*)CBaseEntity::Instance( tr.pHit );
		if ( pTarget->IsPlayer() )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPlayerBrowse, NULL, pPlayer->pev );
				WRITE_BYTE( pTarget->m_iTeamNum == pPlayer->m_iTeamNum );
				WRITE_BYTE( pPlayer->pev->iuser1 ? 0 : pTarget->m_iTeamNum );
				WRITE_STRING( STRING( pTarget->pev->netname ) );
				WRITE_BYTE( (unsigned char)pTarget->m_iItems );
				WRITE_BYTE( (unsigned char)(int)pTarget->pev->health );
				WRITE_BYTE( (unsigned char)(int)pTarget->pev->armorvalue );
			MESSAGE_END();
		}
		else
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgPlayerBrowse, NULL, pPlayer->pev );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 0 );
				WRITE_STRING( "" );
			MESSAGE_END();
		}
		pPlayer->m_iLastPlayerTrace = ENTINDEX( tr.pHit );
	}
	CHalfLifeMultiplay::PlayerThink( pPlayer );
}

BOOL CHalfLifeCTFplay::ClientCommand( CBasePlayer* pPlayer, const char* pcmd )
{
	if ( !strcmp( pcmd, "cancelmenu" ) )
	{
		if ( pPlayer->m_iCurrentMenu == 3 )
		{
			pPlayer->m_iCurrentMenu = 2;
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamFull, NULL, pPlayer->pev );
				WRITE_BYTE( 0 );
			MESSAGE_END();
			pPlayer->m_iNewTeamNum = 0;
			pPlayer->Player_Menu();
		}
		else if ( pPlayer->m_iCurrentMenu == 2 )
		{
			if ( pPlayer->m_iNewTeamNum || pPlayer->m_iTeamNum )
				pPlayer->m_iCurrentMenu = 0;
			else
				pPlayer->Menu_Team_Input( -1 );
		}
		return TRUE;
	}
	if ( !strcmp( pcmd, "endmotd" ) )
	{
		pPlayer->m_iCurrentMenu = 2;
		pPlayer->Player_Menu();
		return TRUE;
	}
	if ( !strcmp( pcmd, "jointeam" ) )
	{
		pPlayer->Menu_Team_Input( (int)strtol( CMD_ARGV( 1 ), NULL, 10 ) );
		return TRUE;
	}
	if ( !strcmp( pcmd, "spectate" ) )
	{
		pPlayer->Menu_Team_Input( -1 );
		return TRUE;
	}
	if ( !strcmp( pcmd, "selectchar" ) )
	{
		if ( CMD_ARGC() > 1 )
			pPlayer->Menu_Char_Input( (int)strtol( CMD_ARGV( 1 ), NULL, 10 ) );
		return TRUE;
	}
	return FALSE;
}

void CHalfLifeCTFplay::GoToIntermission()
{
	m_iStatsPhase = 1;
	m_iStatsPlayer = 1;
	m_flNextStatsSend = gpGlobals->time + 0.2;
	CHalfLifeMultiplay::GoToIntermission();
}

void CHalfLifeCTFplay::PlayerSpawn( CBasePlayer* pPlayer )
{
	if ( !pPlayer->m_iTeamNum || pPlayer->pev->iuser1 )
		return;

	int autoSwitch = pPlayer->m_iAutoWepSwitch;
	pPlayer->m_iAutoWepSwitch = 1;
	pPlayer->pev->weapons |= ( 1 << WEAPON_SUIT );
	BOOL addDefault = TRUE;
	CBaseEntity* pEquip = NULL;
	while ( ( pEquip = UTIL_FindEntityByClassname( pEquip, "game_player_equip" ) ) != NULL )
	{
		addDefault = FALSE;
		pEquip->Touch( pPlayer );
	}
	MESSAGE_BEGIN( MSG_ONE, gmsgOldWeapon, NULL, pPlayer->pev );
		WRITE_BYTE( oldweapons.value == 1.0f );
	MESSAGE_END();
	if ( addDefault )
	{
		pPlayer->GiveNamedItem( "weapon_pipewrench" );
		pPlayer->GiveNamedItem( "weapon_eagle" );
		pPlayer->GiveNamedItem( "weapon_grapple" );
		pPlayer->GiveAmmo( 21, "357", 36 );
	}
	CTFGoalFlag* pFlag = NULL;
	while ( ( pFlag = (CTFGoalFlag*)UTIL_FindEntityByClassname( pFlag, "item_ctfflag" ) ) != NULL )
	{
		if ( pFlag->m_iGoalState == CTF_GOAL_CARRIED )
			pFlag->TurnOnLight( pPlayer );
	}
	CTFGoalBase* pBase = NULL;
	while ( ( pBase = (CTFGoalBase*)UTIL_FindEntityByClassname( pBase, "item_ctfbase" ) ) != NULL )
		pBase->TurnOnLight( pPlayer );

	MESSAGE_BEGIN( MSG_ONE, gmsgHudColor, NULL, pPlayer->pev );
		if ( pPlayer->m_iTeamNum == 1 || pPlayer->m_iTeamNum == 2 )
		{
			WRITE_BYTE( pPlayer->m_iTeamNum == 1 ? 255 : 0 );
			WRITE_BYTE( 160 );
			WRITE_BYTE( 0 );
		}
		else
		{
			WRITE_BYTE( 128 );
			WRITE_BYTE( 128 );
			WRITE_BYTE( 128 );
		}
	MESSAGE_END();
	InitItemsForPlayer( pPlayer );
	DisplayTeamFlags( pPlayer );
	pPlayer->m_iAutoWepSwitch = autoSwitch;
}

void CHalfLifeCTFplay::DeathNotice( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor )
{
	if ( m_DisableDeathMessages )
		return;

	CBaseEntity* pAttacker = NULL;
	if ( pKiller && pVictim && ( pKiller->flags & FL_CLIENT ) )
		pAttacker = CBaseEntity::Instance( pKiller );
	if ( pAttacker && pAttacker != pVictim && PlayerRelationship( pVictim, pAttacker ) == GR_TEAMMATE )
	{
		MESSAGE_BEGIN( MSG_ALL, gmsgDeathMsg );
			WRITE_BYTE( ENTINDEX( ENT( pKiller ) ) );
			WRITE_BYTE( pVictim->entindex() );
			WRITE_STRING( "teammate" );
		MESSAGE_END();
	}
	else
	{
		CHalfLifeMultiplay::DeathNotice( pVictim, pKiller, pInflictor );
	}
}

void CHalfLifeCTFplay::ClientDisconnected( edict_t* pClient )
{
	if ( !pClient )
		return;
	CBasePlayer* pPlayer = (CBasePlayer*)pClient->pvPrivateData;
	if ( !pPlayer )
		return;

	if ( !g_fGameOver )
	{
		if ( pPlayer->m_pFlag )
			( (CTFGoalFlag*)(CBaseEntity*)pPlayer->m_pFlag )->DropFlag( pPlayer );
		if ( pPlayer->m_iItems & CTF_POWERUP_MASK )
			ScatterPlayerCTFPowerups( pPlayer );
	}
	pPlayer->m_iTeamNum = 0;
	pPlayer->m_iNewTeamNum = 0;
	FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );
	UTIL_LogPrintf( "\"%s<%i><%u><%s>\" disconnected\n", STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
		GetTeamName( pPlayer->edict() ) );
	free( pPlayer->ip );
	free( pszPlayerIPs[ENTINDEX( pClient )] );
	pszPlayerIPs[ENTINDEX( pClient )] = NULL;
	pPlayer->ip = NULL;
	pPlayer->RemoveAllItems( TRUE );
	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
		WRITE_BYTE( ENTINDEX( pClient ) );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	CBasePlayer* pObserver = NULL;
	while ( ( pObserver = (CBasePlayer*)UTIL_FindEntityByClassname( pObserver, "player" ) ) != NULL )
	{
		if ( FNullEnt( pObserver->edict() ) )
			break;
		if ( pObserver->pev && pObserver != pPlayer && pObserver->m_hObserverTarget == pPlayer )
		{
			int mode = pObserver->pev->iuser1;
			pObserver->pev->iuser1 = 0;
			pObserver->m_hObserverTarget = NULL;
			pObserver->Observer_SetMode( mode );
		}
	}
}

void CHalfLifeCTFplay::ChangePlayerTeam( CBasePlayer* pPlayer, const char* pCharName, BOOL bKill, BOOL bGib )
{
	int playerIndex = pPlayer->entindex();
	if ( pCharName )
	{
		int team = -1;
		for ( int i = 0; i < 2 && team == -1; i++ )
		{
			for ( int j = 0; j < 6; j++ )
			{
				if ( !strcmp( pCharName, team_chars[i][j] ) )
				{
					team = i;
					break;
				}
			}
		}
		if ( team == -1 )
			return;

		if ( pPlayer->m_iTeamNum - 1 != team )
		{
			if ( bKill )
			{
				m_DisableDeathMessages = TRUE;
				m_DisableDeathPenalty = TRUE;
				if ( !g_fGameOver )
				{
					if ( pPlayer->m_pFlag )
						( (CTFGoalFlag*)(CBaseEntity*)pPlayer->m_pFlag )->DropFlag( pPlayer );
					if ( pPlayer->m_iItems & CTF_POWERUP_MASK )
						ScatterPlayerCTFPowerups( pPlayer );
				}
				entvars_t* pevWorld = VARS( INDEXENT( 0 ) );
				pPlayer->TakeDamage( pevWorld, pevWorld, 900, bGib ? DMG_ALWAYSGIB : DMG_NEVERGIB );
				m_DisableDeathMessages = FALSE;
				m_DisableDeathPenalty = FALSE;
			}
			pPlayer->m_iTeamNum = team + 1;
		}
		pPlayer->m_afPhysicsFlags &= ~PFLAG_OBSERVER;
		strncpy( pPlayer->m_szTeamName, team_names[team + 1], sizeof( pPlayer->m_szTeamName ) );
		pPlayer->m_szTeamModel = (char*)pCharName;
		g_engfuncs.pfnSetClientKeyValue( playerIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ),
			"model", (char*)pCharName );
		g_engfuncs.pfnSetClientKeyValue( playerIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ),
			"team", pPlayer->m_szTeamName );
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
			WRITE_BYTE( playerIndex );
			WRITE_STRING( pPlayer->m_szTeamName );
			WRITE_BYTE( pPlayer->m_iTeamNum );
		MESSAGE_END();
		const char* pTeamName = GetTeamName( pPlayer->edict() );
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" joined team \"%s\"\n", STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
			pTeamName, pTeamName );
		const char* pName = STRING( pPlayer->pev->netname );
		if ( pName && *pName )
		{
			UTIL_LogPrintf( "\"%s<%i><%u><%s>\" changed role to \"%s\"\n", pName,
				GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
				pTeamName, pCharName );
		}
		return;
	}

	if ( pPlayer->pev->health <= 0 )
		respawn( pPlayer->pev, FALSE );
	pPlayer->pev->effects |= EF_NODRAW;
	pPlayer->pev->flags |= FL_SPECTATOR;
	pPlayer->pev->solid = SOLID_NOT;
	pPlayer->pev->movetype = MOVETYPE_NOCLIP;
	pPlayer->pev->takedamage = DAMAGE_NO;
	pPlayer->m_iHideHUD |= HIDEHUD_WEAPONS | HIDEHUD_HEALTH;
	pPlayer->m_afPhysicsFlags |= PFLAG_OBSERVER;
	pPlayer->m_iNewTeamNum = 0;
	pPlayer->m_iCurrentMenu = 0;
	pPlayer->m_iTeamNum = 0;
	pPlayer->SetSuitUpdate( NULL, FALSE, 0 );
	pPlayer->m_iClientHealth = 100;
	MESSAGE_BEGIN( MSG_ONE, gmsgHealth, NULL, pPlayer->pev );
		WRITE_BYTE( pPlayer->m_iClientHealth );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ONE, gmsgCurWeapon, NULL, pPlayer->pev );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 255 );
	MESSAGE_END();
	pPlayer->m_iClientFOV = 0;
	pPlayer->m_iFOV = 0;
	MESSAGE_BEGIN( MSG_ONE, gmsgSetFOV, NULL, pPlayer->pev );
		WRITE_BYTE( 0 );
	MESSAGE_END();
	if ( pPlayer->m_pTank )
	{
		pPlayer->m_pTank->Use( pPlayer, pPlayer, USE_OFF, 0 );
		pPlayer->m_pTank = NULL;
	}
	pPlayer->m_afPhysicsFlags &= ~PFLAG_DUCKING;
	pPlayer->pev->flags &= ~FL_DUCKING;
	pPlayer->Observer_SetMode( OBS_CHASE_FREE );
	pPlayer->pev->deadflag = DEAD_RESPAWNABLE;
	if ( pPlayer->HasNamedPlayerItem( "weapon_satchel" ) )
		DeactivateSatchels( pPlayer );
	pPlayer->RemoveAllItems( FALSE );
	if ( !g_fGameOver )
	{
		if ( pPlayer->m_pFlag )
			( (CTFGoalFlag*)(CBaseEntity*)pPlayer->m_pFlag )->DropFlag( pPlayer );
		if ( pPlayer->m_iItems & CTF_POWERUP_MASK )
			ScatterPlayerCTFPowerups( pPlayer );
	}
	pPlayer->m_iDeaths = 0;
	pPlayer->m_iFlagCaptures = 0;
	pPlayer->pev->frags = 0;
	MESSAGE_BEGIN( MSG_ALL, gmsgScoreInfo );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iDeaths );
	MESSAGE_END();
	pPlayer->m_szTeamName[0] = 0;
	g_engfuncs.pfnSetClientKeyValue( playerIndex, g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ),
		"team", pPlayer->m_szTeamName );
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
		WRITE_BYTE( playerIndex );
		WRITE_STRING( pPlayer->m_szTeamName );
		WRITE_BYTE( pPlayer->m_iTeamNum );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_ALL, gmsgSpectator );
		WRITE_BYTE( pPlayer->entindex() );
		WRITE_BYTE( 1 );
	MESSAGE_END();
	UTIL_LogPrintf( "\"%s<%i><%u><spectator>\" joined team \"spectator\"\n", STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ) );
}

void CHalfLifeCTFplay::ClientUserInfoChanged( CBasePlayer* pPlayer, char* infobuffer )
{
	pPlayer->SetPrefsFromUserinfo( infobuffer );
	if ( !pPlayer->m_szTeamModel )
		return;
	const char* pModel = g_engfuncs.pfnInfoKeyValue( infobuffer, "model" );
	if ( *pModel == '=' || !strcmp( pModel, pPlayer->m_szTeamModel ) )
		return;

	BOOL validModel = FALSE;
	for ( int i = 0; i < 2 && !validModel; i++ )
	{
		for ( int j = 0; j < 6; j++ )
		{
			if ( !strcmp( pPlayer->m_szTeamModel, team_chars[i][j] ) )
			{
				validModel = TRUE;
				break;
			}
		}
	}
	if ( !validModel )
		return;
	const char* pName = STRING( pPlayer->pev->netname );
	if ( pName && *pName )
	{
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" changed role to \"%s\"\n", pName,
			GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
			GetTeamName( pPlayer->edict() ), pPlayer->m_szTeamModel );
	}
	g_engfuncs.pfnSetClientKeyValue( pPlayer->entindex(), g_engfuncs.pfnGetInfoKeyBuffer( pPlayer->edict() ),
		"model", pPlayer->m_szTeamModel );
	UTIL_SayText( "* Not allowed to change player model through the console in CTF!\n", pPlayer );
}

void CHalfLifeCTFplay::InitHUD( CBasePlayer* pPlayer )
{
	CHalfLifeMultiplay::InitHUD( pPlayer );
	DisplayTeamFlags( pPlayer );
	float timeLimit = (float)( 60.0 * timelimit.value );
	MESSAGE_BEGIN( MSG_ONE, gmsgFlagTimer, NULL, pPlayer->pev );
		WRITE_BYTE( timeLimit != 0 );
		if ( timeLimit != 0 )
		{
			float timeLeft = timeLimit - gpGlobals->time;
			WRITE_SHORT( (int)timeLeft );
		}
	MESSAGE_END();
	RecountTeams();
	if ( pPlayer->m_iTeamNum > 0 && !pPlayer->pev->iuser1 )
	{
		char text[1052];
		sprintf( text, "* you are on team '%s'\n", team_names[pPlayer->m_iTeamNum] );
		UTIL_SayText( text, pPlayer );
		ChangePlayerTeam( pPlayer, pPlayer->m_szTeamName, FALSE, FALSE );
	}
	pPlayer->entindex();
	RecountTeams();
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pClient = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pClient && IsValidTeam( pClient->TeamID() ) )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgTeamInfo, NULL, pPlayer->pev );
				WRITE_BYTE( pClient->entindex() );
				WRITE_STRING( pClient->TeamID() );
				WRITE_BYTE( pClient->m_iTeamNum );
			MESSAGE_END();
		}
	}
}

void CHalfLifeCTFplay::PlayerKilled( CBasePlayer* pVictim, entvars_t* pKiller, entvars_t* pInflictor )
{
	if ( !pVictim || pVictim->pev->iuser1 )
		return;

	if ( !m_DisableDeathPenalty && !g_fGameOver )
	{
		CBaseEntity* pKillerEntity = pKiller ? CBaseEntity::Instance( pKiller ) : NULL;
		if ( pVictim->m_pFlag )
		{
			if ( pKillerEntity && pKillerEntity->IsPlayer()
				&& pVictim->m_iTeamNum != ( (CBasePlayer*)pKillerEntity )->m_iTeamNum )
			{
				CBasePlayer* pAttacker = (CBasePlayer*)pKillerEntity;
				pAttacker->m_iCTFScore++;
				pAttacker->m_iDefense++;
				MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
					WRITE_BYTE( pAttacker->entindex() );
					WRITE_BYTE( pAttacker->m_iCTFScore );
				MESSAGE_END();
				pAttacker->m_iCTFScore++;
				ClientPrint( pAttacker->pev, HUD_PRINTTALK, "#CTFScorePoint" );
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s", STRING( pAttacker->pev->netname ) ) );
				if ( pVictim->m_iTeamNum == 1 )
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagKilledBM" );
				else if ( pVictim->m_iTeamNum == 2 )
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagKilledOF" );

				char killerTeam[256];
				char victimTeam[256];
				strncpy( killerTeam, GetTeamName( ENT( pKiller ) ), sizeof( killerTeam ) );
				strncpy( victimTeam, GetTeamName( pVictim->edict() ), sizeof( victimTeam ) );
				killerTeam[255] = 0;
				victimTeam[255] = 0;
				UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"FlagDefense\" against \"%s<%i><%u><%s>\"\n",
					STRING( pKiller->netname ), GETPLAYERUSERID( ENT( pKiller ) ),
					g_engfuncs.pfnGetPlayerWONId( ENT( pKiller ) ), killerTeam,
					STRING( pVictim->pev->netname ), GETPLAYERUSERID( pVictim->edict() ),
					g_engfuncs.pfnGetPlayerWONId( pVictim->edict() ), victimTeam );
			}
			UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"LostFlag\"\n", STRING( pVictim->pev->netname ),
				GETPLAYERUSERID( pVictim->edict() ), g_engfuncs.pfnGetPlayerWONId( pVictim->edict() ),
				GetTeamName( pVictim->edict() ) );
		}

		if ( pKillerEntity && pKillerEntity->IsPlayer() )
		{
			CBasePlayer* pAttacker = (CBasePlayer*)pKillerEntity;
			const char* pWeapon = NULL;
			if ( pKiller == pInflictor )
			{
				if ( pAttacker->m_pActiveItem )
					pWeapon = CBasePlayerItem::ItemInfoArray[pAttacker->m_pActiveItem->m_iId].pszName;
			}
			else if ( pInflictor && pInflictor->classname )
			{
				pWeapon = STRING( pInflictor->classname );
			}
			if ( pWeapon )
			{
				if ( !strcmp( pWeapon, "weapon_sniperrifle" ) || !strcmp( pWeapon, "weapon_crossbow" ) )
					pAttacker->m_iSnipeKills++;
				else if ( !strcmp( pWeapon, "weapon_grapple" ) )
					pAttacker->m_iBarnacleKills++;
			}
			if ( pKiller == pVictim->pev )
				pAttacker->m_iSuicides++;
			CTFGoalBase* pBase = NULL;
			while ( ( pBase = (CTFGoalBase*)UTIL_FindEntityByClassname( pBase, "item_ctfbase" ) ) != NULL )
			{
				if ( pKiller == pVictim->pev || pBase->m_iGoalNum != pAttacker->m_iTeamNum )
					continue;
				double x = (double)pVictim->pev->origin.x - pBase->pev->origin.x;
				double y = (double)pVictim->pev->origin.y - pBase->pev->origin.y;
				double z = (double)pVictim->pev->origin.z - pBase->pev->origin.z;
				if ( sqrt( x * x + y * y + z * z ) <= g_flBaseDefendDist )
				{
					pAttacker->m_iCTFScore++;
					pAttacker->m_iDefense++;
					MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
						WRITE_BYTE( pAttacker->entindex() );
						WRITE_BYTE( pAttacker->m_iCTFScore );
					MESSAGE_END();
					ClientPrint( pAttacker->pev, HUD_PRINTTALK, "#CTFScorePoint" );
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s", STRING( pAttacker->pev->netname ) ) );
					if ( pAttacker->m_iTeamNum == 1 )
						UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagDefendedBM" );
					else if ( pAttacker->m_iTeamNum == 2 )
						UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagDefendedOF" );
					break;
				}
			}
			if ( pKiller != pVictim->pev )
			{
				for ( int i = 1; i <= gpGlobals->maxClients; i++ )
				{
					CBasePlayer* pCarrier = (CBasePlayer*)UTIL_PlayerByIndex( i );
					if ( !pCarrier || !pCarrier->m_pFlag || pCarrier->m_nLastShotBy != pVictim->entindex()
						|| !( g_flDefendCarrierTime > (double)gpGlobals->time - pCarrier->m_flLastShotTime ) )
						continue;
					pAttacker->m_iCTFScore++;
					pAttacker->m_iDefense++;
					MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
						WRITE_BYTE( pAttacker->entindex() );
						WRITE_BYTE( pAttacker->m_iCTFScore );
					MESSAGE_END();
					ClientPrint( pAttacker->pev, HUD_PRINTTALK, "#CTFScorePoint" );
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s", STRING( pAttacker->pev->netname ) ) );
					if ( pVictim->m_iTeamNum == 1 )
						UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFCarrierDefendedOF" );
					else if ( pVictim->m_iTeamNum == 2 )
						UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFCarrierDefendedBM" );
				}
			}
		}
		CHalfLifeMultiplay::PlayerKilled( pVictim, pKiller, pInflictor );
	}
	if ( pVictim->IsPlayer() && !g_fGameOver )
	{
		if ( pVictim->m_pFlag )
			( (CTFGoalFlag*)(CBaseEntity*)pVictim->m_pFlag )->DropFlag( pVictim );
		if ( pVictim->m_iItems & CTF_POWERUP_MASK )
			ScatterPlayerCTFPowerups( pVictim );
	}
}

void CHalfLifeCTFplay::SendPlayerStatInfo( CBasePlayer* pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgStatsPlayer, NULL, pPlayer->pev );
		WRITE_BYTE( pPlayer->entindex() );
		float score = (float)( (double)pPlayer->m_iCTFScore + pPlayer->pev->frags );
		WRITE_SHORT( (int)score );
		WRITE_SHORT( (int)pPlayer->pev->frags );
		WRITE_SHORT( pPlayer->m_iCTFScore );
		WRITE_SHORT( pPlayer->m_iOffense );
		WRITE_SHORT( pPlayer->m_iDefense );
		WRITE_SHORT( pPlayer->m_iSnipeKills );
		WRITE_SHORT( pPlayer->m_iBarnacleKills );
		WRITE_SHORT( pPlayer->m_iDeaths );
		WRITE_SHORT( pPlayer->m_iSuicides );
		WRITE_SHORT( pPlayer->m_iMostDamage );
		WRITE_SHORT( (short)(int)pPlayer->m_flAccelTime );
		WRITE_SHORT( (short)(int)pPlayer->m_flBackpackTime );
		WRITE_SHORT( (short)(int)pPlayer->m_flHealthTime );
		WRITE_SHORT( (short)(int)pPlayer->m_flShieldTime );
		WRITE_SHORT( (short)(int)pPlayer->m_flJumpTime );
	MESSAGE_END();
}

void CHalfLifeCTFplay::Think()
{
	if ( m_iStatsPhase && gpGlobals->time >= m_flNextStatsSend )
	{
		switch ( m_iStatsPhase )
		{
		case 1:
			SendTeamStatInfo( 0 );
			m_iStatsPhase = 2;
			break;
		case 2:
			SendTeamStatInfo( 1 );
			m_iStatsPhase = 3;
			break;
		case 3:
			SendTeamStatInfo( 2 );
			m_iStatsPhase = 4;
			break;
		case 4:
			{
				int playerIndex = m_iStatsPlayer;
				int sent = 0;
				while ( playerIndex <= gpGlobals->maxClients )
				{
					CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( playerIndex );
					if ( pPlayer && pPlayer->IsPlayer() )
					{
						sent++;
						SendPlayerStatInfo( pPlayer );
					}
					playerIndex++;
					if ( sent > 5 )
						break;
				}
				m_iStatsPlayer = playerIndex - 1;
				if ( gpGlobals->maxClients <= playerIndex )
					m_iStatsPhase = 5;
			}
			break;
		case 5:
			MESSAGE_BEGIN( MSG_ALL, gmsgVGUIMenu );
				WRITE_BYTE( 9 );
			MESSAGE_END();
			m_iStatsPhase = 0;
			break;
		}
		m_flNextStatsSend = gpGlobals->time + 0.2;
	}
	if ( m_fRefreshScores )
	{
		DisplayTeamFlags( NULL );
		m_fRefreshScores = FALSE;
	}
	if ( g_fGameOver )
	{
		CHalfLifeMultiplay::Think();
		return;
	}
	double timeLimit = 60.0 * timelimit.value;
	if ( timeLimit != 0 && gpGlobals->time >= timeLimit )
	{
		GoToIntermission();
		return;
	}
	float fragLimit = fraglimit.value;
	if ( fragLimit != 0 )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBaseEntity* pPlayer = UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->pev->frags >= fragLimit )
			{
				GoToIntermission();
				return;
			}
		}
	}
	if ( CheckForLevelEnd( (int)ctf_capture.value ) )
		GoToIntermission();
}
