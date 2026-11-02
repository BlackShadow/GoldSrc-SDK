#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "tf_defs.h"

char* g_szTeamColors[5] = { "", "Blue", "Red", "Yellow", "Green" };
char* sTNameCvars[12] =
{
	"team1", "team2", "team3", "team4",
	"t1", "t2", "t3", "t4",
	"Blue", "Red", "Yellow", "Green"
};

void TeamFortress_TeamShowScores( BOOL bLong, CBasePlayer* pPlayer )
{
	for ( int team = 1; team <= number_of_teams; team++ )
	{
		char* text;
		if ( bLong )
			text = UTIL_VarArgs( "Team %d (%s): %d\n", team, g_szTeamColors[team], teamscores[team] );
		else
			text = UTIL_VarArgs( "%s: %d\n", g_szTeamColors[team], teamscores[team] );
		if ( bLong && pPlayer )
			ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, text );
		else
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, text );
	}
}

void TeamFortress_TeamSetColor( int )
{
	teamcolors[1][PC_SCOUT] = { 153, 139 };
	teamcolors[1][PC_SNIPER] = { 153, 145 };
	teamcolors[1][PC_SOLDIER] = { 153, 130 };
	teamcolors[1][PC_DEMOMAN] = { 153, 145 };
	teamcolors[1][PC_MEDIC] = { 153, 140 };
	teamcolors[1][PC_HVYWEAP] = { 148, 138 };
	teamcolors[1][PC_PYRO] = { 140, 145 };
	teamcolors[1][PC_SPY] = { 150, 145 };
	teamcolors[1][PC_ENGINEER] = { 140, 148 };
	teamcolors[1][SENTRY_COLOR] = { 150, 0 };
	teamcolors[1][PC_CIVILIAN] = { 150, 140 };
	teamcolors[2][PC_SCOUT] = { 255, 10 };
	teamcolors[2][PC_SNIPER] = { 255, 10 };
	teamcolors[2][PC_SOLDIER] = { 250, 28 };
	teamcolors[2][PC_DEMOMAN] = { 255, 20 };
	teamcolors[2][PC_MEDIC] = { 255, 250 };
	teamcolors[2][PC_HVYWEAP] = { 255, 25 };
	teamcolors[2][PC_PYRO] = { 250, 25 };
	teamcolors[2][PC_SPY] = { 250, 240 };
	teamcolors[2][PC_ENGINEER] = { 5, 250 };
	teamcolors[2][SENTRY_COLOR] = { 250, 0 };
	teamcolors[2][PC_CIVILIAN] = { 250, 240 };
	teamcolors[3][PC_SCOUT] = { 45, 35 };
	teamcolors[3][PC_SNIPER] = { 45, 35 };
	teamcolors[3][PC_SOLDIER] = { 45, 35 };
	teamcolors[3][PC_DEMOMAN] = { 45, 35 };
	teamcolors[3][PC_MEDIC] = { 45, 35 };
	teamcolors[3][PC_HVYWEAP] = { 45, 40 };
	teamcolors[3][PC_PYRO] = { 45, 35 };
	teamcolors[3][PC_SPY] = { 45, 35 };
	teamcolors[3][PC_ENGINEER] = { 45, 45 };
	teamcolors[3][SENTRY_COLOR] = { 45, 0 };
	teamcolors[3][PC_CIVILIAN] = { 45, 35 };
	teamcolors[4][PC_SCOUT] = { 100, 90 };
	teamcolors[4][PC_SNIPER] = { 80, 90 };
	teamcolors[4][PC_SOLDIER] = { 100, 40 };
	teamcolors[4][PC_DEMOMAN] = { 100, 90 };
	teamcolors[4][PC_MEDIC] = { 100, 90 };
	teamcolors[4][PC_HVYWEAP] = { 100, 90 };
	teamcolors[4][PC_PYRO] = { 100, 50 };
	teamcolors[4][PC_SPY] = { 100, 90 };
	teamcolors[4][PC_ENGINEER] = { 100, 90 };
	teamcolors[4][SENTRY_COLOR] = { 100, 0 };
	teamcolors[4][PC_CIVILIAN] = { 100, 90 };
	rgbcolors[0] = Vector( 255, 255, 255 );
	rgbcolors[1] = Vector( 0, 0, 255 );
	rgbcolors[2] = Vector( 255, 0, 0 );
	rgbcolors[3] = Vector( 255, 255, 30 );
	rgbcolors[4] = Vector( 0, 255, 0 );
}

char* GetTeamName( int tno )
{
	if ( tno <= 0 )
		return "";
	if ( team_names[tno] )
		return (char*)STRING( team_names[tno] );
	return sTNameCvars[tno + 7];
}

void TeamFortress_TeamIncreaseScore( int tno, int scoretoadd )
{
	if ( !tno )
		return;
	teamscores[tno] += scoretoadd;
	MESSAGE_BEGIN( MSG_ALL, gmsgTeamScore );
		WRITE_STRING( GetTeamName( tno ) );
		WRITE_SHORT( teamscores[tno] );
		WRITE_SHORT( 0 );
	MESSAGE_END();
}

int TeamFortress_TeamGetScoreFrags( int tno )
{
	return teamscores[tno];
}

int TeamFortress_TeamGetNoPlayers( int tno )
{
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->team_no == tno )
			count++;
	}
	return count;
}

int TeamFortress_GetNoPlayers()
{
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		if ( UTIL_PlayerByIndex( i ) )
			count++;
	}
	return count;
}

BOOL ClassIsRestricted( float tno, int pc )
{
	if ( tno == 0 )
		return FALSE;
	int limit;
	switch ( pc )
	{
	case PC_SCOUT: limit = (int)cr_scout.value; break;
	case PC_SNIPER: limit = (int)cr_sniper.value; break;
	case PC_SOLDIER: limit = (int)cr_soldier.value; break;
	case PC_DEMOMAN: limit = (int)cr_demoman.value; break;
	case PC_MEDIC: limit = (int)cr_medic.value; break;
	case PC_HVYWEAP: limit = (int)cr_hwguy.value; break;
	case PC_PYRO: limit = (int)cr_pyro.value; break;
	case PC_SPY: limit = (int)cr_spy.value; break;
	case PC_ENGINEER: limit = (int)cr_engineer.value; break;
	case PC_RANDOM: limit = (int)cr_random.value; break;
	default: return FALSE;
	}
	if ( limit <= 0 )
		return limit == -1;
	int count = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pPlayer || pPlayer->team_no != tno )
			continue;
		if ( pc == PC_RANDOM )
			count += ( pPlayer->tfstate & TFSTATE_RANDOMPC ) != 0;
		else if ( pc == pPlayer->pev->playerclass || pc == pPlayer->nextpc )
			count += ( pPlayer->tfstate & TFSTATE_RANDOMPC ) == 0;
	}
	return count >= limit;
}

BOOL TeamFortress_TeamIsCivilian( float tno )
{
	if ( tno == 1 )
		return ( civilianteams & TEAM1_CIVILIANS ) != 0;
	if ( tno == 2 )
		return ( civilianteams & TEAM2_CIVILIANS ) != 0;
	if ( tno == 3 )
		return ( civilianteams & TEAM3_CIVILIANS ) != 0;
	if ( tno == 4 )
		return ( civilianteams & TEAM4_CIVILIANS ) != 0;
	if ( tno == 0 )
		return illegalclasses[0] == -1;
	return FALSE;
}

void CalculateTeamEqualiser()
{
	for ( int team = 1; team <= 4; team++ )
		teamadvantage[team] = 1;
	if ( number_of_teams != 2 )
		return;
	if ( tfc_balance_teams.value != 0 )
	{
		float players[5];
		for ( int team = 1; team <= 4; team++ )
			players[team] = (float)TeamFortress_TeamGetNoPlayers( team );
		float total = players[1] + players[2] + players[3] + players[4];
		if ( !( total >= 1 ) )
			return;
		float average = total / number_of_teams;
		for ( int team = 1; team <= number_of_teams; team++ )
		{
			if ( players[team] > 0 )
			{
				float difference = average / players[team] - 1;
				if ( difference != 0 )
					difference /= 3;
				teamadvantage[team] = difference + 1;
			}
		}
	}
	if ( tfc_balance_scores.value == 0 )
		return;
	float scores[5];
	scores[1] = (float)( teamscores[1] + 60 );
	scores[2] = number_of_teams >= 2 ? (float)( teamscores[2] + 60 ) : 0;
	scores[3] = number_of_teams >= 3 ? (float)( teamscores[3] + 60 ) : 0;
	scores[4] = number_of_teams >= 4 ? (float)( teamscores[4] + 60 ) : 0;
	for ( int team = 1; team <= number_of_teams; team++ )
	{
		float others;
		switch ( team )
		{
		case 1: others = scores[2] + scores[3] + scores[4]; break;
		case 2: others = scores[3] + scores[1] + scores[4]; break;
		case 3: others = scores[2] + scores[1] + scores[4]; break;
		default: others = scores[1] + ( scores[2] + scores[3] ); break;
		}
		float difference = others / ( number_of_teams - 1 ) / scores[team] - 1;
		if ( difference != 0 )
			difference /= 3;
		teamadvantage[team] *= difference + 1;
	}
}

float TeamEqualiseDamage( CBaseEntity* targ, CBaseEntity* attacker, float damage )
{
	if ( !FClassnameIs( targ->pev, "player" ) || !FClassnameIs( attacker->pev, "player" ) ||
		!targ->team_no || !attacker->team_no || attacker->IsAlly( targ ) )
		return damage;
	float advantage = teamadvantage[attacker->team_no];
	if ( advantage == 0 )
	{
		ALERT( at_console, "There is a team with an advantage of 0\n" );
		advantage = 0.1f;
	}
	float adjusted = advantage * damage;
	advantage = teamadvantage[targ->team_no];
	if ( advantage == 0 )
	{
		ALERT( at_console, "There is a team with an advantage of 0\n" );
		adjusted *= 10;
	}
	else
		adjusted *= 1.0f / advantage;
	if ( damage > adjusted )
		gfPrototypeLessDamage += damage - adjusted;
	else
		gfPrototypeExtraDamage += adjusted - damage;
	return adjusted;
}

void TeamFortress_StatusQuery()
{
	int four = TeamFortress_TeamGetNoPlayers( 4 );
	int three = TeamFortress_TeamGetNoPlayers( 3 );
	int two = TeamFortress_TeamGetNoPlayers( 2 );
	int one = TeamFortress_TeamGetNoPlayers( 1 );
	ALERT( at_console, "players per team: %d %d %d %d\n", one, two, three, four );
	ALERT( at_console, "    equalisation: %0.1f %0.1f %0.1f %0.1f\n",
		teamadvantage[1], teamadvantage[2], teamadvantage[3], teamadvantage[4] );
	double teamplayValue = gpGlobals->teamplay;
	int teamplayBits;
	memcpy( &teamplayBits, &teamplayValue, sizeof( teamplayBits ) );
	ALERT( at_console, "Teamplay is %d\n", teamplayBits );
}

BOOL TeamFortress_SortTeams()
{
	BOOL gotScore[5] = { FALSE, FALSE, FALSE, FALSE, FALSE };
	for ( int i = 0; i < 5; i++ )
		g_iOrderedTeams[i] = 0;
	BOOL draw = TRUE;
	int previousScore = 0;
	int previousTeam = 0;
	int selected = 0;
	for ( int place = 1; place <= number_of_teams; place++ )
	{
		int highest = 0;
		for ( int team = 1; team <= number_of_teams; team++ )
		{
			if ( teamscores[team] >= highest && !gotScore[team] )
			{
				highest = teamscores[team];
				selected = team;
			}
		}
		if ( place > 1 && draw )
			draw = ( ( teamallies[place] >> ( ( previousTeam - 1 ) & 31 ) ) & 1 ) || previousScore == highest;
		g_iOrderedTeams[place] = selected;
		gotScore[selected] = TRUE;
		previousTeam = selected;
		previousScore = highest;
	}
	return draw;
}
