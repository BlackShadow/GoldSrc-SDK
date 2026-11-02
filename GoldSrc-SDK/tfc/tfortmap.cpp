#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "tf_defs.h"
#include "monsterevent.h"
#include "skill.h"
#include "shake.h"
#include <time.h>

int GetTeamCheckTeam( const char* pTargetName );
void InactivateGoal( CBaseEntity* Goal );
void RestoreGoal( CBaseEntity* Goal );
void InactivateSpawn( CBaseEntity* Spawn );
void ActivateSpawn( CBaseEntity* Spawn );
BOOL AllGoalsInState( int iGroupNo, int iState );
BOOL HasItemFromGroup( CBaseEntity* AP, int iGroupNo );
BOOL IsAffectedBy( CBaseEntity* Goal, CBasePlayer* Player, CBasePlayer* AP );
void Apply_Results( CBaseEntity* Goal, CBasePlayer* Player, CBasePlayer* AP, BOOL bAddBonuses );
void RemoveResults( CBaseEntity* Goal, CBasePlayer* pPlayer );
BOOL APMeetsCriteria( CBaseEntity* Goal, CBaseEntity* AP );
void SetupRespawn( CBaseEntity* pGoal );
BOOL ShouldActivate( CBaseEntity* Goal, CBaseEntity* AP );
void DoTriggerWork( CBaseEntity* Goal, CBasePlayer* AP );
void DoItemGroupWork( CBaseEntity* Item, CBasePlayer* AP );
void ParseTFDetect( CBaseEntity* Detect );
void EndRound( CBaseEntity* pGoal );

class CNoGrenadeArea : public CBaseEntity
{
public:
	void Spawn();
};

LINK_ENTITY_TO_CLASS( info_player_teamspawn, CTFSpawn );
LINK_ENTITY_TO_CLASS( info_tfdetect, CTFDetect );
LINK_ENTITY_TO_CLASS( info_tfgoal, CTFGoal );
LINK_ENTITY_TO_CLASS( info_tfgoal_timer, CTFTimerGoal );
LINK_ENTITY_TO_CLASS( item_tfgoal, CTFGoalItem );
LINK_ENTITY_TO_CLASS( info_tf_teamcheck, CTeamCheck );
LINK_ENTITY_TO_CLASS( info_tf_teamset, CTeamSet );
LINK_ENTITY_TO_CLASS( i_t_t, CTFTimerGoal );
LINK_ENTITY_TO_CLASS( i_t_g, CTFGoal );
LINK_ENTITY_TO_CLASS( i_p_t, CTFSpawn );
LINK_ENTITY_TO_CLASS( func_nogrenades, CNoGrenadeArea );

BOOL CBaseEntity::CheckExistence()
{
	if ( ex_skill_min == -1 )
	{
		if ( g_iSkillLevel < 0 )
			return FALSE;
	}
	else if ( ex_skill_min && g_iSkillLevel < ex_skill_min )
	{
		return FALSE;
	}
	if ( ex_skill_max == -1 )
		return g_iSkillLevel <= 0;
	return !ex_skill_max || g_iSkillLevel <= ex_skill_max;
}

void CBaseEntity::KeyValue( KeyValueData* pkvd )
{
	const char* key = pkvd->szKeyName;
	if ( FStrEq( key, "goal_no" ) )
		goal_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "mdl" ) )
		pev->model = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "group_no" ) )
		group_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team_no" ) )
		team_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "playerclass" ) )
		pev->playerclass = atoi( pkvd->szValue );
	else if ( FStrEq( key, "items_allowed" ) )
		items_allowed = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_state" ) )
		goal_state = atoi( pkvd->szValue );
	else if ( FStrEq( key, "items" ) )
		items = atoi( pkvd->szValue );
	else if ( FStrEq( key, "owned_by" ) )
		owned_by = atoi( pkvd->szValue );
	else if ( FStrEq( key, "armorclass" ) )
		armorclass = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_activation" ) || FStrEq( key, "g_a" ) )
		goal_activation = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_effects" ) || FStrEq( key, "g_e" ) )
		goal_effects = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_result" ) )
		goal_result = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_group" ) )
		goal_group = atoi( pkvd->szValue );
	else if ( FStrEq( key, "else_goal" ) )
		else_goal = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_goal_is_active" ) )
		if_goal_is_active = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_goal_is_inactive" ) )
		if_goal_is_inactive = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_goal_is_removed" ) )
		if_goal_is_removed = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_group_is_active" ) )
		if_group_is_active = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_group_is_inactive" ) )
		if_group_is_inactive = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_group_is_removed" ) )
		if_group_is_removed = atoi( pkvd->szValue );
	else if ( FStrEq( key, "search_time" ) )
		search_time = atof( pkvd->szValue );
	else if ( FStrEq( key, "t_length" ) )
		t_length = atof( pkvd->szValue );
	else if ( FStrEq( key, "activate_goal_no" ) )
		activate_goal_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "inactivate_goal_no" ) )
		inactivate_goal_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "remove_goal_no" ) || FStrEq( key, "rv_g" ) )
		remove_goal_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "restore_goal_no" ) || FStrEq( key, "rs_g" ) )
		restore_goal_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "activate_group_no" ) )
		activate_group_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "inactivate_group_no" ) )
		inactivate_group_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "remove_group_no" ) || FStrEq( key, "rv_gr" ) )
		remove_group_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "restore_group_no" ) || FStrEq( key, "rs_gr" ) )
		restore_group_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "goal_min" ) )
	{
		Vector vec;
		UTIL_StringToVector( vec, pkvd->szValue );
		if ( vec != g_vecZero )
			goal_min = vec;
	}
	else if ( FStrEq( key, "goal_max" ) )
	{
		Vector vec;
		UTIL_StringToVector( vec, pkvd->szValue );
		if ( vec != g_vecZero )
			goal_max = vec;
	}
	else if ( FStrEq( key, "has_item_from_group" ) || FStrEq( key, "h_i_g" ) )
		has_item_from_group = atoi( pkvd->szValue );
	else if ( FStrEq( key, "hasnt_item_from_group" ) || FStrEq( key, "hn_i_g" ) )
		hasnt_item_from_group = atoi( pkvd->szValue );
	else if ( FStrEq( key, "remove_item_group" ) || FStrEq( key, "r_i_g" ) )
		remove_item_group = atoi( pkvd->szValue );
	else if ( FStrEq( key, "return_item_no" ) )
		return_item_no = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_item_has_moved" ) )
		if_item_has_moved = atoi( pkvd->szValue );
	else if ( FStrEq( key, "if_item_hasnt_moved" ) )
		if_item_hasnt_moved = atoi( pkvd->szValue );
	else if ( FStrEq( key, "remove_spawnpoint" ) )
		remove_spawnpoint = atoi( pkvd->szValue );
	else if ( FStrEq( key, "restore_spawnpoint" ) )
		restore_spawnpoint = atoi( pkvd->szValue );
	else if ( FStrEq( key, "remove_spawngroup" ) || FStrEq( key, "rv_s_h" ) )
		remove_spawngroup = atoi( pkvd->szValue );
	else if ( FStrEq( key, "restore_spawngroup" ) || FStrEq( key, "rs_s_h" ) )
		restore_spawngroup = atoi( pkvd->szValue );
	else if ( FStrEq( key, "display_item_status1" ) )
		display_item_status[0] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "display_item_status2" ) )
		display_item_status[1] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "display_item_status3" ) )
		display_item_status[2] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "display_item_status4" ) )
		display_item_status[3] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team_str_home" ) || FStrEq( key, "t_s_g" ) )
		team_str_home = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "team_str_moved" ) || FStrEq( key, "t_s_m" ) )
		team_str_moved = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "team_str_carried" ) || FStrEq( key, "t_s_c" ) )
		team_str_carried = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_str_home" ) || FStrEq( key, "t_s_h" ) )
		non_team_str_home = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_str_moved" ) )
		non_team_str_moved = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_str_carried" ) || FStrEq( key, "n_s_c" ) )
		non_team_str_carried = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "ex_skill_min" ) )
		ex_skill_min = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ex_skill_max" ) )
		ex_skill_max = atoi( pkvd->szValue );
	else if ( FStrEq( key, "increase_team1" ) )
		increase_team[0] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "increase_team2" ) )
		increase_team[1] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "increase_team3" ) )
		increase_team[2] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "increase_team4" ) )
		increase_team[3] = atoi( pkvd->szValue );
	else
	{
		KeyValuePartTwo( pkvd );
		return;
	}
	pkvd->fHandled = TRUE;
}

void CBaseEntity::KeyValuePartTwo( KeyValueData* pkvd )
{
	const char* key = pkvd->szKeyName;
	if ( FStrEq( key, "wait" ) )
		wait = atof( pkvd->szValue );
	else if ( FStrEq( key, "increase_team" ) )
		count = atoi( pkvd->szValue );
	else if ( FStrEq( key, "axhitme" ) )
		axhitme = atoi( pkvd->szValue );
	else if ( FStrEq( key, "pausetime" ) || FStrEq( key, "delay" ) )
		drop_time = atof( pkvd->szValue );
	else if ( FStrEq( key, "attack_finished" ) )
		attack_finished = atof( pkvd->szValue );
	else if ( FStrEq( key, "distance" ) )
		distance = atof( pkvd->szValue );
	else if ( FStrEq( key, "killtarget" ) )
		killtarget = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "noise4" ) )
		noise4 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "broadcast" ) || FStrEq( key, "b_b" ) )
		broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "team_broadcast" ) || FStrEq( key, "b_t" ) )
		team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "speak" ) || FStrEq( key, "team1_name" ) )
		speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "AP_speak" ) || FStrEq( key, "team2_name" ) )
		AP_speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "team_speak" ) || FStrEq( key, "team3_name" ) )
		team_speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "owners_team_speak" ) || FStrEq( key, "team4_name" ) )
		owners_team_speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_owners_team_speak" ) )
		non_owners_team_speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_broadcast" ) || FStrEq( key, "b_n" ) )
		non_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_speak" ) )
		non_team_speak = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "owners_team_broadcast" ) || FStrEq( key, "b_o" ) )
		owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_owners_team_broadcast" ) )
		non_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_broadcast" ) || FStrEq( key, "n_b" ) )
		netname_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_team_broadcast" ) || FStrEq( key, "n_t" ) )
		netname_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_non_team_broadcast" ) || FStrEq( key, "n_n" ) )
		netname_non_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_owners_team_broadcast" ) || FStrEq( key, "n_o" ) )
		netname_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "team_drop" ) || FStrEq( key, "d_t" ) )
		team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "non_team_drop" ) || FStrEq( key, "d_n" ) )
		non_team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_team_drop" ) || FStrEq( key, "d_n_t" ) )
		netname_team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "netname_non_team_drop" ) || FStrEq( key, "d_n_n" ) )
		netname_non_team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "all_active" ) )
		all_active = atoi( pkvd->szValue );
	else if ( FStrEq( key, "delay_time" ) )
		delay_time = atof( pkvd->szValue );
	else if ( FStrEq( key, "ammo_shells" ) || FStrEq( key, "a_s" ) )
		ammo_shells = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ammo_nails" ) || FStrEq( key, "a_n" ) )
		ammo_nails = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ammo_rockets" ) || FStrEq( key, "a_r" ) )
		ammo_rockets = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ammo_cells" ) || FStrEq( key, "a_c" ) )
		ammo_cells = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ammo_medikit" ) )
		ammo_medikit = atoi( pkvd->szValue );
	else if ( FStrEq( key, "ammo_detpack" ) )
		ammo_detpack = atoi( pkvd->szValue );
	else if ( FStrEq( key, "no_grenades_1" ) )
		no_grenades_1 = atoi( pkvd->szValue );
	else if ( FStrEq( key, "no_grenades_2" ) )
		no_grenades_2 = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_shells" ) )
		maxammo_shells = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_nails" ) )
		maxammo_nails = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_rockets" ) )
		maxammo_rockets = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_cells" ) )
		maxammo_cells = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_medikit" ) )
		maxammo_medikit = atoi( pkvd->szValue );
	else if ( FStrEq( key, "maxammo_detpack" ) )
		maxammo_detpack = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team1_allies" ) )
		teamallies[1] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team2_allies" ) )
		teamallies[2] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team3_allies" ) )
		teamallies[3] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "team4_allies" ) )
		teamallies[4] = atoi( pkvd->szValue );
	else if ( FStrEq( key, "invincible_finished" ) )
		invincible_finished = atof( pkvd->szValue );
	else if ( FStrEq( key, "invisible_finished" ) )
		invisible_finished = atof( pkvd->szValue );
	else if ( FStrEq( key, "super_damage_finished" ) )
		super_damage_finished = atof( pkvd->szValue );
	else if ( FStrEq( key, "last_impulse" ) || FStrEq( key, "number_of_teams" ) )
		last_impulse = atoi( pkvd->szValue );
	else if ( FStrEq( key, "radsuit_finished" ) )
		radsuit_finished = atof( pkvd->szValue );
	else if ( FStrEq( key, "deathtype" ) )
		deathtype = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "speed_reduction" ) )
		speed_reduction = atoi( pkvd->szValue );
	else if ( FStrEq( key, "endround_time" ) )
		m_flEndRoundTime = atof( pkvd->szValue );
	else if ( FStrEq( key, "endround_team1" ) )
		m_iszEndRoundMsg_Team1 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_team2" ) )
		m_iszEndRoundMsg_Team2 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_team3" ) )
		m_iszEndRoundMsg_Team3 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_team4" ) )
		m_iszEndRoundMsg_Team4 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_win_team1" ) )
		m_iszEndRoundMsg_Team1_Win = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_win_team2" ) )
		m_iszEndRoundMsg_Team2_Win = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_win_team3" ) )
		m_iszEndRoundMsg_Team3_Win = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_win_team4" ) )
		m_iszEndRoundMsg_Team4_Win = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_lose_team1" ) )
		m_iszEndRoundMsg_Team1_Lose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_lose_team2" ) )
		m_iszEndRoundMsg_Team2_Lose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_lose_team3" ) )
		m_iszEndRoundMsg_Team3_Lose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_lose_team4" ) )
		m_iszEndRoundMsg_Team4_Lose = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "teamcheck" ) )
		teamcheck = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "owned_by_teamcheck" ) )
		owned_by_teamcheck = ALLOC_STRING( pkvd->szValue );
	else
	{
		KeyValuePartThree( pkvd );
		return;
	}
	pkvd->fHandled = TRUE;
}

void CBaseEntity::KeyValuePartThree( KeyValueData* pkvd )
{
	const char* key = pkvd->szKeyName;
	if ( FStrEq( key, "endround_owned_by" ) )
		m_iszEndRoundMsg_OwnedBy = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "endround_non_owned_by" ) )
		m_iszEndRoundMsg_NonOwnedBy = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_broadcast" ) || FStrEq( key, "org_b_b" ) )
		org_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_team_broadcast" ) || FStrEq( key, "org_b_t" ) )
		org_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_message" ) )
		org_message = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_owners_team_broadcast" ) || FStrEq( key, "org_b_o" ) )
		org_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_non_owners_team_broadcast" ) )
		org_non_owners_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_non_team_broadcast" ) || FStrEq( key, "org_b_n" ) )
		org_non_team_broadcast = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_noise3" ) )
		org_noise3 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_noise4" ) )
		org_noise4 = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_team_drop" ) || FStrEq( key, "org_d_t" ) )
		org_team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "org_non_team_drop" ) || FStrEq( key, "org_d_n" ) )
		org_non_team_drop = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "increase_team_owned_by" ) )
		increase_team_owned_by = atoi( pkvd->szValue );
	else if ( FStrEq( key, "replacement_model" ) )
		replacement_model = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( key, "replacement_model_body" ) )
		replacement_model_body = atoi( pkvd->szValue );
	else if ( FStrEq( key, "replacement_model_skin" ) )
		replacement_model_skin = atoi( pkvd->szValue );
	else if ( FStrEq( key, "replacement_model_flags" ) )
		replacement_model_flags = atoi( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

void CTFDetect::Spawn()
{
}

void CTeamCheck::Spawn()
{
}

void CTeamSet::Spawn()
{
}

void CTeamSet::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( team_no )
		SUB_UseTargets( pActivator, USE_SET, (float)team_no );
	else
		SUB_UseTargets( pActivator, USE_TOGGLE, 0 );
}

void CTeamCheck::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	if ( useType == USE_TOGGLE )
		team_no = ( team_no == 1 ) + 1;
	else if ( useType == USE_SET && value >= 1 && value <= 4 )
		team_no = (int)value;
}

BOOL CTeamCheck::TeamMatches( int iTeam )
{
	return team_no == iTeam;
}

int GetTeamCheckTeam( const char* pTargetName )
{
	CBaseEntity* pCheck = UTIL_FindEntityByTargetname( NULL, pTargetName );
	if ( pCheck && FClassnameIs( pCheck->pev, "info_tf_teamcheck" ) )
		return pCheck->team_no;
	return 0;
}

void CTFSpawn::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	if ( ( team_no >= 1 && team_no <= 4 ) || teamcheck )
	{
		if ( team_no > number_of_teams )
			number_of_teams = (float)team_no;
		pev->classname = MAKE_STRING( "info_player_teamspawn" );
		pev->netname = MAKE_STRING( "info_player_teamspawn" );
	}
	else
	{
		ALERT( at_console, "Teamspawnpoint with an invalid team_no of %d\n", team_no );
	}
}

void CTFSpawn::Activate()
{
	m_pTeamCheck = NULL;
	CBaseEntity* pCheck = UTIL_FindEntityByTargetname( NULL, STRING( teamcheck ) );
	if ( pCheck && FClassnameIs( pCheck->pev, "info_tf_teamcheck" ) )
		m_pTeamCheck = pCheck;
}

BOOL CTFSpawn::CheckTeam( int iTeamNo )
{
	if ( team_no )
		return team_no == iTeamNo;
	if ( m_pTeamCheck )
		return ( (CBaseEntity*)m_pTeamCheck )->team_no == iTeamNo;
	return FALSE;
}

void CNoGrenadeArea::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( edict(), STRING( pev->model ) );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
}

void CTFGoal::SetObjectCollisionBox()
{
	if ( STRING( pev->model )[0] == '*' )
	{
		double radius = 0;
		for ( int i = 0; i < 3; i++ )
		{
			double extent = fabs( (double)pev->mins[i] );
			radius = extent <= radius ? radius : extent;
			extent = fabs( (double)pev->maxs[i] );
			radius = extent <= radius ? radius : extent;
		}
		for ( int i = 0; i < 3; i++ )
		{
			pev->absmin[i] = (float)( pev->origin[i] - radius );
			pev->absmax[i] = (float)( pev->origin[i] + radius );
		}
		pev->absmin = pev->absmin - Vector( 1, 1, 1 );
		pev->absmax = pev->absmax + Vector( 1, 1, 1 );
	}
	else
	{
		pev->absmin = pev->origin + Vector( -24, -24, 0 );
		pev->absmax = pev->origin + Vector( 24, 24, 16 );
	}
}

void CTFGoal::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	pev->classname = MAKE_STRING( "info_tfgoal" );
	if ( pev->model )
	{
		if ( STRING( pev->model )[0] == '*' )
			pev->effects |= EF_NODRAW;
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
		SET_MODEL( edict(), STRING( pev->model ) );
	}
	if ( pev->noise )
		PRECACHE_SOUND( (char*)STRING( pev->noise ) );
	PRECACHE_SOUND( "items/protect.wav" );
	PRECACHE_SOUND( "items/protect2.wav" );
	PRECACHE_SOUND( "items/protect3.wav" );
	PRECACHE_SOUND( "FVox/HEV_logon.wav" );
	PRECACHE_SOUND( "FVox/hev_shutdown.wav" );
	PRECACHE_SOUND( "items/inv1.wav" );
	PRECACHE_SOUND( "items/inv2.wav" );
	PRECACHE_SOUND( "items/inv3.wav" );
	PRECACHE_SOUND( "items/damage.wav" );
	PRECACHE_SOUND( "items/damage2.wav" );
	PRECACHE_SOUND( "items/damage3.wav" );
	pev->solid = SOLID_TRIGGER;
	if ( !goal_state )
		goal_state = TFGS_INACTIVE;
	if ( goal_min != g_vecZero && goal_max != g_vecZero )
		UTIL_SetSize( pev, goal_min, goal_max );
	UTIL_SetOrigin( pev, pev->origin );
	StartGoal();
}

void CTFGoal::StartGoal()
{
	m_bAddBonuses = FALSE;
	SetThink( &CTFGoal::PlaceGoal );
	pev->nextthink = gpGlobals->time + 0.2;
	if ( goal_state == TFGS_REMOVED )
		RemoveGoal( this );
}

void CTFGoal::PlaceGoal()
{
	if ( FClassnameIs( pev, "info_tfgoal_timer" ) )
	{
		SetThink( &CBaseEntity::tfgoal_timer_tick );
		pev->nextthink = gpGlobals->time + search_time;
	}
	else if ( goal_activation & TFGA_TOUCH )
	{
		SetTouch( &CBaseEntity::tfgoal_touch );
	}
	pev->classname = MAKE_STRING( "info_tfgoal" );
	if ( goal_activation & TFGA_DROPTOGROUND )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->origin.z += 6;
		UTIL_SetOrigin( pev, pev->origin );
		if ( !DROP_TO_FLOOR( edict() ) )
		{
			ALERT( at_error, "TF Goal %s fell out of level at %f,%f,%f", STRING( pev->netname ),
				pev->origin.x, pev->origin.y, pev->origin.z );
			UTIL_Remove( this );
			return;
		}
	}
	pev->movetype = MOVETYPE_NONE;
	pev->velocity = g_vecZero;
	pev->oldorigin = pev->origin;
}

void CTFTimerGoal::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	CTFGoal::Spawn();
	pev->classname = MAKE_STRING( "info_tfgoal_timer" );
	if ( search_time > 0 )
	{
		pev->solid = SOLID_NOT;
		return;
	}
	ALERT( at_console, "Timer Goal %s created with no specified time.\n", STRING( pev->netname ) );
	dremove( this );
}

void CTFGoalItem::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	pev->classname = MAKE_STRING( "item_tfgoal" );
	if ( pev->model )
	{
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
		SET_MODEL( edict(), STRING( pev->model ) );
		pev->sequence = LookupSequence( "not_carried" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	PRECACHE_SOUND( "items/itembk2.wav" );
	if ( pev->noise )
		PRECACHE_SOUND( (char*)STRING( pev->noise ) );
	if ( !goal_state )
		goal_state = TFGS_INACTIVE;
	if ( goal_activation & TFGI_SOLID )
	{
		pev->solid = SOLID_BBOX;
		if ( goal_min == g_vecZero )
			goal_min = Vector( -16, -16, -24 );
		if ( goal_max == g_vecZero )
			goal_max = Vector( 16, 16, 32 );
	}
	else
	{
		pev->solid = SOLID_TRIGGER;
	}
	if ( !pev->netname )
		pev->netname = MAKE_STRING( "goalitem" );
	if ( drop_time <= 0 )
		drop_time = 60;
	UTIL_SetSize( pev, goal_min, goal_max );
	UTIL_SetOrigin( pev, pev->origin );
	SetTouch( &CBaseEntity::item_tfgoal_touch );
	StartItem();
}

void CTFGoalItem::StartItem()
{
	SetThink( &CTFGoalItem::PlaceItem );
	pev->nextthink = gpGlobals->time + 0.2;
	if ( goal_state == TFGS_REMOVED )
		RemoveGoal( this );
}

void CTFGoalItem::PlaceItem()
{
	static int item_list_bit = 1;
	pev->velocity = g_vecZero;
	if ( goal_activation & TFGI_DROPTOGROUND )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->origin.z += 6;
		UTIL_SetOrigin( pev, pev->origin );
		if ( !DROP_TO_FLOOR( edict() ) )
		{
			ALERT( at_error, "TF GoalItem %s fell out of level at %f,%f,%f", STRING( pev->netname ),
				pev->origin.x, pev->origin.y, pev->origin.z );
			UTIL_Remove( this );
			return;
		}
	}
	pev->movetype = MOVETYPE_NONE;
	pev->oldorigin = pev->origin;
	if ( goal_activation & TFGI_ITEMGLOWS )
	{
		pev->renderfx = kRenderFxGlowShell;
		if ( owned_by_teamcheck )
			owned_by = GetTeamCheckTeam( STRING( owned_by_teamcheck ) );
		pev->rendercolor = rgbcolors[owned_by >= 1 && owned_by <= 4 ? owned_by : 0];
		pev->renderamt = 100;
	}
	item_list = item_list_bit;
	item_list_bit *= 2;
}

CBaseEntity* Finditem( int ino )
{
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem->goal_no == ino )
			return pItem;
	}
	ALERT( at_console, "Could not find an item with a goal_no of %d.\n", ino );
	return NULL;
}

CBaseEntity* Findgoal( int gno )
{
	CBaseEntity* pGoal = NULL;
	while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
	{
		if ( pGoal->goal_no == gno )
			return pGoal;
	}
	ALERT( at_console, "Could not find a goal with a goal_no of %d.\n", gno );
	return NULL;
}

CBaseEntity* Findteamspawn( int gno )
{
	CBaseEntity* pSpawn = NULL;
	while ( ( pSpawn = UTIL_FindEntityByString( pSpawn, "netname", "info_player_teamspawn" ) ) != NULL )
	{
		if ( pSpawn->goal_no == gno )
			return pSpawn;
	}
	ALERT( at_console, "Could not find a Teamspawn with a goal_no of %d.\n", gno );
	return NULL;
}

void InactivateGoal( CBaseEntity* Goal )
{
	if ( Goal->goal_state != TFGS_ACTIVE )
		return;
	if ( Goal->Classify() != CLASS_TFGOAL_TIMER )
	{
		if ( ( Goal->goal_activation & TFGI_SOLID )
			&& ( Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_ITEM ) )
			Goal->pev->solid = SOLID_BBOX;
		else
			Goal->pev->solid = SOLID_TRIGGER;
	}
	Goal->goal_state = TFGS_INACTIVE;
	if ( STRING( Goal->pev->model )[0] != '*' )
		Goal->pev->effects &= ~EF_NODRAW;
}

void RestoreGoal( CBaseEntity* Goal )
{
	if ( Goal->goal_state != TFGS_REMOVED )
		return;
	if ( !Goal->search_time )
	{
		if ( ( Goal->goal_activation & TFGI_SOLID ) && FClassnameIs( Goal->pev, "item_tfgoal" ) )
			Goal->pev->solid = SOLID_BBOX;
		else
			Goal->pev->solid = SOLID_TRIGGER;
	}
	else
	{
		Goal->pev->nextthink = Goal->search_time + gpGlobals->time;
	}
	Goal->goal_state = TFGS_INACTIVE;
	if ( STRING( Goal->pev->model )[0] != '*' )
		Goal->pev->effects &= ~EF_NODRAW;
}

void RemoveGoal( CBaseEntity* Goal )
{
	Goal->pev->solid = SOLID_NOT;
	Goal->goal_state = TFGS_REMOVED;
	Goal->pev->effects |= EF_NODRAW;
}

void InactivateSpawn( CBaseEntity* Spawn )
{
	Spawn->goal_state = TFGS_REMOVED;
}

void ActivateSpawn( CBaseEntity* Spawn )
{
	Spawn->goal_state = TFGS_INACTIVE;
}

void CBaseEntity::DoRespawn()
{
	RestoreGoal( this );
	InactivateGoal( this );
}

BOOL AllGoalsInState( int iGroupNo, int iState )
{
	CBaseEntity* pGoal = NULL;
	while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
	{
		if ( pGoal->group_no == iGroupNo && pGoal->goal_state != iState )
			return FALSE;
	}
	return TRUE;
}

BOOL HasItemFromGroup( CBaseEntity* AP, int iGroupNo )
{
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem->group_no == iGroupNo && pItem->pev->owner == AP->edict() )
			return TRUE;
	}
	return FALSE;
}

BOOL APMeetsCriteria( CBaseEntity* Goal, CBaseEntity* AP )
{
	CBaseEntity* pEntity;
	if ( AP && AP->Classify() == CLASS_PLAYER )
	{
		if ( Goal->team_no && ( Goal->team_no != AP->team_no || AP->pev->deadflag != DEAD_NO ) )
			return FALSE;
		if ( Goal->teamcheck && AP->team_no != GetTeamCheckTeam( STRING( Goal->teamcheck ) ) )
			return FALSE;
		if ( Goal->pev->playerclass && Goal->pev->playerclass != AP->pev->playerclass )
			return FALSE;
		if ( Goal->items_allowed )
		{
			pEntity = Finditem( Goal->items_allowed );
			if ( !pEntity || pEntity->pev->owner != AP->edict() )
				return FALSE;
		}
	}
	if ( Goal->if_goal_is_active )
	{
		pEntity = Findgoal( Goal->if_goal_is_active );
		if ( !pEntity || pEntity->goal_state != TFGS_ACTIVE )
			return FALSE;
	}
	if ( Goal->if_goal_is_inactive )
	{
		pEntity = Findgoal( Goal->if_goal_is_inactive );
		if ( !pEntity || pEntity->goal_state != TFGS_INACTIVE )
			return FALSE;
	}
	if ( Goal->if_goal_is_removed )
	{
		pEntity = Findgoal( Goal->if_goal_is_removed );
		if ( !pEntity || pEntity->goal_state != TFGS_REMOVED )
			return FALSE;
	}
	if ( Goal->if_group_is_active && !AllGoalsInState( Goal->if_group_is_active, TFGS_ACTIVE ) )
		return FALSE;
	if ( Goal->if_group_is_inactive && !AllGoalsInState( Goal->if_group_is_inactive, TFGS_INACTIVE ) )
		return FALSE;
	if ( Goal->if_group_is_removed && !AllGoalsInState( Goal->if_group_is_removed, TFGS_REMOVED ) )
		return FALSE;
	if ( Goal->if_item_has_moved )
	{
		pEntity = Finditem( Goal->if_item_has_moved );
		if ( !pEntity || ( pEntity->goal_state != TFGS_ACTIVE && pEntity->pev->origin == pEntity->pev->oldorigin ) )
			return FALSE;
	}
	if ( Goal->if_item_hasnt_moved )
	{
		pEntity = Finditem( Goal->if_item_hasnt_moved );
		if ( !pEntity || pEntity->goal_state == TFGS_ACTIVE || pEntity->pev->origin != pEntity->pev->oldorigin )
			return FALSE;
	}
	if ( !AP || AP->Classify() != CLASS_PLAYER )
		return TRUE;
	if ( Goal->has_item_from_group && !HasItemFromGroup( AP, Goal->has_item_from_group ) )
		return FALSE;
	if ( Goal->hasnt_item_from_group && HasItemFromGroup( AP, Goal->hasnt_item_from_group ) )
		return FALSE;
	return TRUE;
}

BOOL ShouldActivate( CBaseEntity* Goal, CBaseEntity* AP )
{
	if ( Goal->goal_state == TFGS_REMOVED || Goal->goal_state == TFGS_DELAYED || Goal->goal_state == TFGS_ACTIVE )
		return FALSE;
	BOOL fMeets = APMeetsCriteria( Goal, AP );
	int iReverse = FClassnameIs( Goal->pev, "item_tfgoal" ) ? TFGI_REVERSE_AP : TFGA_REVERSE_AP;
	return ( Goal->goal_activation & iReverse ) ? !fMeets : fMeets;
}

void SetupRespawn( CBaseEntity* pGoal )
{
	pGoal->m_bAddBonuses = FALSE;
	if ( pGoal->goal_result & TFGR_SINGLE )
	{
		RemoveGoal( pGoal );
		return;
	}
	if ( pGoal->Classify() == CLASS_TFGOAL_TIMER )
	{
		InactivateGoal( pGoal );
		pGoal->SetThink( &CBaseEntity::tfgoal_timer_tick );
		pGoal->pev->nextthink = gpGlobals->time + pGoal->search_time;
	}
	else if ( pGoal->wait > 0 )
	{
		pGoal->SetThink( &CBaseEntity::DoRespawn );
		pGoal->pev->nextthink = pGoal->wait + gpGlobals->time;
	}
	else if ( pGoal->wait != -1 )
	{
		InactivateGoal( pGoal );
	}
}

void DoTriggerWork( CBaseEntity* Goal, CBasePlayer* AP )
{
	if ( Goal->killtarget )
	{
		CBaseEntity* pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( Goal->killtarget ) ) ) != NULL )
			UTIL_Remove( pEntity );
	}
	if ( Goal->pev->target )
	{
		CBaseEntity* pEntity = NULL;
		while ( ( pEntity = UTIL_FindEntityByTargetname( pEntity, STRING( Goal->pev->target ) ) ) != NULL )
		{
			if ( !( pEntity->pev->flags & FL_KILLME ) )
				pEntity->Use( AP, Goal, USE_TOGGLE, 0 );
		}
	}
}

BOOL IsAffectedBy( CBaseEntity* Goal, CBasePlayer* Player, CBasePlayer* AP )
{
	if ( !Goal || !Player )
		return FALSE;

	if ( !Player->pev->playerclass )
		return FALSE;
	if ( ( Goal->goal_effects & TFGE_SAME_ENVIRONMENT )
		&& UTIL_PointContents( Goal->pev->origin ) != UTIL_PointContents( Player->pev->origin ) )
		return FALSE;
	if ( Goal->t_length && ( Goal->pev->origin - Player->pev->origin ).Length() <= Goal->t_length )
	{
		if ( !( Goal->goal_effects & TFGE_WALL ) )
			return TRUE;
		TraceResult tr;
		UTIL_TraceLine( Goal->pev->origin, Player->pev->origin, dont_ignore_monsters, Goal->edict(), &tr );
		if ( tr.flFraction == 1 )
			return TRUE;
	}
	if ( Goal->Classify() != CLASS_TFGOAL_TIMER && AP )
	{
		if ( Goal->Classify() == CLASS_TFSPAWN && Player == AP )
			return TRUE;
		if ( ( Goal->goal_effects & TFGE_AP ) && Player == AP )
			return TRUE;
		if ( ( Goal->goal_effects & TFGE_AP_TEAM ) && AP->team_no == Player->team_no )
			return TRUE;
	}
	if ( ( Goal->goal_effects & TFGE_NOT_AP_TEAM ) && ( !AP || AP->team_no != Player->team_no ) )
		return TRUE;
	if ( ( Goal->goal_effects & TFGE_NOT_AP ) && Player != AP )
		return TRUE;
	if ( Goal->maxammo_shells && Goal->maxammo_shells == Player->team_no )
		return TRUE;
	return Goal->maxammo_nails && Goal->maxammo_nails != Player->team_no;
}

BOOL ActivationSucceeded( CBaseEntity* Goal, CBasePlayer* AP, CBaseEntity* ActivatingGoal )
{
	if ( cb_prematch_time > gpGlobals->time && Goal->Classify() != CLASS_TFGOAL_TIMER )
		return FALSE;
	if ( ShouldActivate( Goal, AP ) )
		return TRUE;
	if ( Goal->else_goal )
	{
		CBaseEntity* pElse = Findgoal( Goal->else_goal );
		if ( pElse )
			ActivateDoResults( pElse, AP, Goal );
	}
	return FALSE;
}

BOOL ActivateDoResults( CBaseEntity* Goal, CBasePlayer* AP, CBaseEntity* ActivatingGoal )
{
	if ( !ActivationSucceeded( Goal, AP, ActivatingGoal ) )
		return FALSE;
	BOOL fBonuses = Goal == ActivatingGoal || Goal->m_bAddBonuses;
	if ( !fBonuses && ActivatingGoal )
		fBonuses = ActivatingGoal->goal_result & TFGR_ADD_BONUSES;
	DoResults( Goal, AP, fBonuses );
	return TRUE;
}

void CTFGoal::Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value )
{
	CBasePlayer* pPlayer = NULL;
	if ( pActivator && pActivator->Classify() == CLASS_PLAYER )
		pPlayer = (CBasePlayer*)pActivator;
	m_bAddBonuses = TRUE;
	ActivateDoResults( this, pPlayer, pCaller );
}

void CBaseEntity::tfgoal_touch( CBaseEntity* pOther )
{
	if ( cb_prematch_time > gpGlobals->time || !( goal_activation & TFGA_TOUCH )
		|| pOther->Classify() != CLASS_PLAYER || !pOther->IsAlive() || goal_state == TFGS_ACTIVE )
		return;
	if ( CTF_Map )
	{
		if ( goal_no == CTF_DROPOFF1 && pOther->team_no == 1 )
		{
			CBaseEntity* pFlag = Finditem( CTF_FLAG1 );
			if ( pFlag->goal_state == TFGS_ACTIVE || pFlag->pev->origin != pFlag->pev->oldorigin )
				return;
		}
		if ( goal_no == CTF_DROPOFF2 && pOther->team_no == 2 )
		{
			CBaseEntity* pFlag = Finditem( CTF_FLAG2 );
			if ( pFlag->goal_state == TFGS_ACTIVE || pFlag->pev->origin != pFlag->pev->oldorigin )
				return;
		}
	}
	ActivateDoResults( this, (CBasePlayer*)pOther, this );
}

void CBaseEntity::DelayedResult()
{
	CBasePlayer* pPlayer = (CBasePlayer*)Instance( pev->enemy );
	if ( goal_state == TFGS_DELAYED )
		DoResults( this, pPlayer, weapon );
}

void CBaseEntity::tfgoal_timer_tick()
{
	if ( goal_state == TFGS_REMOVED )
		return;
	if ( cb_prematch_time > gpGlobals->time )
	{
		SetThink( &CBaseEntity::tfgoal_timer_tick );
		pev->nextthink = cb_prematch_time + search_time;
		return;
	}
	if ( !APMeetsCriteria( this, NULL ) )
	{
		SetThink( &CBaseEntity::tfgoal_timer_tick );
		pev->nextthink = gpGlobals->time + search_time;
		return;
	}
	DoResults( this, NULL, TRUE );
}

void DoGoalWork( CBaseEntity* Goal, CBasePlayer* AP )
{
	CBaseEntity* pEntity;
	if ( Goal->activate_goal_no )
	{
		pEntity = Findgoal( Goal->activate_goal_no );
		if ( pEntity )
			ActivateDoResults( pEntity, AP, Goal );
	}
	if ( Goal->inactivate_goal_no )
	{
		pEntity = Findgoal( Goal->inactivate_goal_no );
		if ( pEntity )
			InactivateGoal( pEntity );
	}
	if ( Goal->restore_goal_no )
	{
		pEntity = Findgoal( Goal->restore_goal_no );
		if ( pEntity )
			RestoreGoal( pEntity );
	}
	if ( Goal->remove_goal_no )
	{
		pEntity = Findgoal( Goal->remove_goal_no );
		if ( pEntity )
			RemoveGoal( pEntity );
	}
	if ( Goal->return_item_no )
	{
		pEntity = Finditem( Goal->return_item_no );
		if ( pEntity )
		{
			CBasePlayer* pOwner = (CBasePlayer*)CBaseEntity::Instance( pEntity->pev->owner );
			if ( pEntity->goal_state == TFGS_ACTIVE )
				tfgoalitem_RemoveFromPlayer( pEntity, pOwner, GI_DROP_REMOVEGOAL );
			CBaseEntity* pTimer = pEntity->CreateTimer( TF_TIMER_RETURNITEM );
			pTimer->weapon = GI_RET_TIME;
			pTimer->SetThink( &CBaseEntity::ReturnItem );
			pTimer->pev->nextthink = gpGlobals->time + 0.1;
			pEntity->pev->solid = SOLID_NOT;
			UTIL_SetOrigin( pEntity->pev, pEntity->pev->origin );
		}
	}
	if ( Goal->remove_spawnpoint )
	{
		pEntity = Findteamspawn( Goal->remove_spawnpoint );
		if ( pEntity )
			InactivateSpawn( pEntity );
	}
	if ( Goal->restore_spawnpoint )
	{
		pEntity = Findteamspawn( Goal->restore_spawnpoint );
		if ( pEntity && pEntity->goal_state == TFGS_REMOVED )
			ActivateSpawn( pEntity );
	}
}

void DoGroupWork( CBaseEntity* Goal, CBasePlayer* AP )
{
	if ( Goal->all_active )
	{
		if ( !Goal->last_impulse )
		{
			ALERT( at_console, "Goal %d has .all_active specified, but no .last_impulse\n", Goal->goal_no );
		}
		else if ( AllGoalsInState( Goal->all_active, TFGS_ACTIVE ) )
		{
			CBaseEntity* pGoal = Findgoal( Goal->last_impulse );
			if ( pGoal )
				DoResults( pGoal, AP, Goal->goal_result & TFGR_ADD_BONUSES );
		}
	}
	CBaseEntity* pGoal = NULL;
	if ( Goal->activate_group_no )
	{
		while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
		{
			if ( pGoal->group_no == Goal->activate_group_no )
				ActivateDoResults( pGoal, AP, Goal );
		}
	}
	if ( Goal->inactivate_group_no )
	{
		while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
		{
			if ( pGoal->group_no == Goal->inactivate_group_no )
				InactivateGoal( pGoal );
		}
	}
	if ( Goal->remove_group_no )
	{
		while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
		{
			if ( pGoal->group_no == Goal->remove_group_no )
				RemoveGoal( pGoal );
		}
	}
	if ( Goal->restore_group_no )
	{
		while ( ( pGoal = UTIL_FindEntityByClassname( pGoal, "info_tfgoal" ) ) != NULL )
		{
			if ( pGoal->group_no == Goal->restore_group_no )
				RestoreGoal( pGoal );
		}
	}
}

void tfgoalitem_checkgoalreturn( CBaseEntity* Item )
{
	if ( Item->pev->impulse )
	{
		CBaseEntity* pGoal = Findgoal( Item->pev->impulse );
		if ( pGoal )
			ActivateDoResults( pGoal, NULL, Item );
	}
}

void CBaseEntity::tfgoalitem_droptouch()
{
	SetTouch( &CBaseEntity::item_tfgoal_touch );
	SetThink( &CBaseEntity::tfgoalitem_dropthink );
	pev->nextthink = gpGlobals->time + 4.25;
}

void CBaseEntity::DoDrop( Vector vecOrigin )
{
	pev->origin = vecOrigin;
	UTIL_SetOrigin( pev, pev->origin );
	pev->movetype = MOVETYPE_TOSS;
	pev->aiment = NULL;
	pev->velocity.z = 400;
	if ( redrop_count > 1 )
	{
		pev->velocity.x = RANDOM_FLOAT( -50, 50 );
		pev->velocity.y = RANDOM_FLOAT( -50, 50 );
	}
	goal_state = TFGS_INACTIVE;
	pev->angles = g_vecZero;
	pev->solid = ( goal_activation & TFGI_SOLID ) ? SOLID_BBOX : SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	UTIL_SetSize( pev, goal_min, goal_max );
	redrop_count++;
	SetThink( &CBaseEntity::tfgoalitem_dropthink );
	pev->nextthink = gpGlobals->time + 5.0;
}

void tfgoalitem_drop( CBaseEntity* Item, BOOL PAlive, CBasePlayer* P )
{
	CBaseEntity* pOwner = CBaseEntity::Instance( Item->pev->owner );
	Item->redrop_origin = pOwner->pev->origin + Vector( 0, 0, ( pOwner->pev->flags & FL_DUCKING ) ? 26 : 8 );
	Item->redrop_count = 0;
	Item->SetTouch( &CBaseEntity::item_tfgoal_touch );
	Item->DoDrop( Item->redrop_origin );
	Item->pev->owner = P->edict();
	if ( PAlive )
	{
		UTIL_MakeAimVectors( P->pev->angles );
		Item->pev->velocity = gpGlobals->v_forward * 400 + gpGlobals->v_up * 200;
		Item->SetTouch( NULL );
		Item->SetThink( &CBaseEntity::tfgoalitem_droptouch );
		Item->pev->nextthink = gpGlobals->time + 0.75;
		Item->pev->enemy = P->edict();
		( (CTFGoalItem*)Item )->m_flDroppedAt = gpGlobals->time;
	}
}

void CBaseEntity::tfgoalitem_dropthink()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->aiment = NULL;
	if ( !drop_time )
		return;
	int contents = UTIL_PointContents( pev->origin );
	if ( contents == CONTENTS_SLIME )
		pev->nextthink = 0.25f * drop_time + gpGlobals->time;
	else if ( contents == CONTENTS_LAVA )
		pev->nextthink = gpGlobals->time + 5;
	else if ( contents != CONTENTS_SOLID && contents != CONTENTS_SKY )
		pev->nextthink = gpGlobals->time + drop_time;
	else if ( redrop_count > 2 )
		pev->nextthink = gpGlobals->time + 2;
	else
	{
		DoDrop( redrop_origin );
		return;
	}
	SetThink( &CBaseEntity::tfgoalitem_remove );
}

void CBaseEntity::tfgoalitem_remove()
{
	if ( goal_state == TFGS_ACTIVE )
		return;
	if ( goal_activation & TFGI_RETURN_REMOVE )
	{
		CBaseEntity* pTimer = CreateTimer( TF_TIMER_RETURNITEM );
		pTimer->weapon = GI_RET_TIME;
		pTimer->SetThink( &CBaseEntity::ReturnItem );
		pTimer->pev->nextthink = gpGlobals->time + 0.1;
	}
	else
	{
		dremove( this );
	}
}

void Apply_Results( CBaseEntity* Goal, CBasePlayer* Player, CBasePlayer* AP, BOOL bAddBonuses )
{
	BOOL fItem = FClassnameIs( Goal->pev, "item_tfgoal" );
	if ( fItem )
		Player->item_list |= Goal->item_list;
	if ( Player == AP && Goal->count && Player->team_no > 0 )
		TeamFortress_TeamIncreaseScore( Player->team_no, Goal->count );
	if ( bAddBonuses )
	{
		if ( Player->IsAlive() )
		{
			if ( Goal->pev->health > 0 )
				Player->TakeHealth( Goal->pev->health, 0 );
			if ( Goal->pev->health < 0 )
				Player->TakeDamage( Goal->pev, Goal->pev, -Goal->pev->health, DMG_IGNOREARMOR | DMG_NEVERGIB );
		}
		if ( Player->IsAlive() )
		{
			if ( Goal->pev->armortype > 0 )
				Player->pev->armortype = Goal->pev->armortype;
			else if ( Goal->pev->armorvalue > 0 )
				Player->pev->armortype = Player->armor_allowed;
			Player->pev->armorvalue += Goal->pev->armorvalue;
			if ( Goal->armorclass > 0 )
				Player->armorclass = Goal->armorclass;
			Player->GiveTFAmmo( Goal->ammo_shells, Goal->ammo_nails, Goal->ammo_rockets, Goal->ammo_cells );
			Player->ammo_medikit += Goal->ammo_medikit;
			Player->ammo_detpack = min( Player->ammo_detpack + Goal->ammo_detpack, Player->maxammo_detpack );
			if ( Player->tp_grenades_1 )
				Player->no_grenades_1 += Goal->no_grenades_1;
			if ( Player->tp_grenades_2 )
				Player->no_grenades_2 += Goal->no_grenades_2;
			if ( ( Player->tfstate & TFSTATE_GRENPRIMED )
				&& ( ( Player->m_iPrimedGrenType == 1 && Player->no_grenades_1 <= 0 && Goal->no_grenades_1 < 0 )
				|| ( Player->m_iPrimedGrenType == 2 && Player->no_grenades_2 <= 0 && Goal->no_grenades_2 < 0 ) ) )
			{
				Player->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
				Player->bRemoveGrenade = TRUE;
			}
			if ( Goal->invincible_finished > 0 )
			{
				Player->items |= IT_INVULNERABILITY;
				Player->invincible_finished = gpGlobals->time + Goal->invincible_finished;
				if ( fItem )
				{
					Player->tfstate |= TFSTATE_INVINCIBLE;
					Player->invincible_finished = gpGlobals->time + 666;
				}
				Player->pev->renderfx = kRenderFxNone;
			}
			if ( Goal->invisible_finished > 0 )
			{
				Player->items |= IT_INVISIBILITY;
				Player->invisible_finished = gpGlobals->time + Goal->invisible_finished;
				if ( fItem )
				{
					Player->tfstate |= TFSTATE_INVISIBLE;
					Player->invisible_finished = gpGlobals->time + 666;
				}
				Player->pev->renderfx = kRenderFxNone;
			}
			if ( Goal->super_damage_finished > 0 )
			{
				Player->items |= IT_QUAD;
				Player->super_damage_finished = gpGlobals->time + Goal->super_damage_finished;
				if ( fItem )
				{
					Player->tfstate |= TFSTATE_QUAD;
					Player->super_damage_finished = gpGlobals->time + 666;
				}
				Player->pev->renderfx = kRenderFxNone;
			}
			if ( Goal->radsuit_finished > 0 )
			{
				Player->items |= IT_SUIT;
				Player->radsuit_finished = gpGlobals->time + Goal->radsuit_finished;
				if ( fItem )
				{
					Player->tfstate |= TFSTATE_RADSUIT;
					Player->radsuit_finished = gpGlobals->time + 666;
				}
			}
		}
		Player->lives += Goal->lives;
		if ( Goal->pev->frags )
			Player->TF_AddFrags( (int)Goal->pev->frags );
		Player->TeamFortress_CheckClassStats();
	}
	if ( Player->pev->playerclass == PC_SPY && ( Goal->goal_result & TFGR_REMOVE_DISGUISE ) )
	{
		Player->immune_to_check = gpGlobals->time + 10;
		Player->Spy_RemoveDisguise();
	}
	if ( Goal->items && !fItem )
	{
		CBaseEntity* pItem = Finditem( Goal->items );
		if ( pItem && Goal != pItem )
			tfgoalitem_GiveToPlayer( pItem, Player, Goal );
	}
	if ( Goal->axhitme )
	{
		CBaseEntity* pItem = Finditem( Goal->axhitme );
		if ( pItem->pev->owner == Player->edict() )
			tfgoalitem_RemoveFromPlayer( pItem, Player, GI_DROP_REMOVEGOAL );
	}
	if ( Goal->remove_item_group )
	{
		CBaseEntity* pItem = UTIL_FindEntityByClassname( NULL, "item_tfgoal" );
		while ( pItem )
		{
			CBaseEntity* pNext = UTIL_FindEntityByClassname( pItem, "item_tfgoal" );
			if ( pItem->group_no == Goal->remove_item_group && pItem->pev->owner == Player->edict() )
				tfgoalitem_RemoveFromPlayer( pItem, Player, GI_DROP_REMOVEGOAL );
			pItem = pNext;
		}
	}
	Player->DisplayLocalItemStatus( Goal );
	if ( Goal->goal_result & TFGR_DESTROY_BUILDINGS )
	{
		Player->no_entry_teleporter_message = TRUE;
		Player->no_exit_teleporter_message = TRUE;
		Player->no_sentry_message = TRUE;
		Player->no_dispenser_message = TRUE;
		Player->Engineer_RemoveBuildings();
		Player->TeamFortress_RemoveLiveGrenades();
		Player->TeamFortress_RemoveRockets();
		Player->RemovePipebombs();
		if ( Player->is_detpacking )
			Player->TeamFortress_DetpackStop();
		else if ( Player->TeamFortress_RemoveDetpacks() )
			Player->ammo_detpack++;
	}
	if ( ( Goal->goal_result & TFGR_FORCE_RESPAWN ) && Player->IsAlive() )
		Player->ForceRespawn();
	if ( Goal->replacement_model && !Player->replacement_model )
	{
		Player->replacement_model = Goal->replacement_model;
		Player->replacement_model_body = Goal->replacement_model_body;
		Player->replacement_model_skin = Goal->replacement_model_skin;
		Player->replacement_model_flags = Goal->replacement_model_flags;
		Player->TeamFortress_SetSkin();
	}
}

void RemoveResults( CBaseEntity* Goal, CBasePlayer* pPlayer )
{
	if ( FClassnameIs( Goal->pev, "item_tfgoal" ) )
	{
		if ( !( pPlayer->item_list & Goal->item_list ) || ( Goal->goal_activation & TFGI_DONTREMOVERES ) )
			return;
		pPlayer->item_list &= ~Goal->item_list;
	}
	if ( Goal->pev->health > 0 )
		pPlayer->TakeDamage( Goal->pev, Goal->pev, Goal->pev->health, DMG_IGNOREARMOR );
	if ( Goal->pev->health < 0 )
		pPlayer->TakeHealth( -Goal->pev->health, 0 );
	pPlayer->lives -= Goal->lives;
	pPlayer->pev->armortype -= Goal->pev->armortype;
	pPlayer->pev->armorvalue -= Goal->pev->armorvalue;
	pPlayer->armorclass &= ~Goal->armorclass;
	if ( Goal->pev->frags )
		pPlayer->TF_AddFrags( (int)Goal->pev->frags );
	pPlayer->ammo_shells -= Goal->ammo_shells;
	pPlayer->ammo_nails -= Goal->ammo_nails;
	pPlayer->ammo_rockets -= Goal->ammo_rockets;
	pPlayer->ammo_cells -= Goal->ammo_cells;
	pPlayer->ammo_medikit -= Goal->ammo_medikit;
	pPlayer->ammo_detpack = min( pPlayer->ammo_detpack - Goal->ammo_detpack, pPlayer->maxammo_detpack );
	pPlayer->no_grenades_1 -= Goal->no_grenades_1;
	pPlayer->no_grenades_2 -= Goal->no_grenades_2;
	if ( ( pPlayer->tfstate & TFSTATE_GRENPRIMED ) && ( pPlayer->no_grenades_1 <= 0 || pPlayer->no_grenades_2 <= 0 ) )
	{
		pPlayer->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
		pPlayer->bRemoveGrenade = TRUE;
	}
	BOOL fInvincible = FALSE;
	BOOL fInvisible = FALSE;
	BOOL fQuad = FALSE;
	BOOL fRadsuit = FALSE;
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem != Goal && pItem->pev->owner == pPlayer->edict() )
		{
			if ( pItem->invincible_finished > 0 )
				fInvincible = TRUE;
			if ( pItem->invisible_finished > 0 )
				fInvisible = TRUE;
			if ( pItem->super_damage_finished > 0 )
				fQuad = TRUE;
			if ( pItem->radsuit_finished > 0 )
				fRadsuit = TRUE;
		}
	}
	if ( Goal->invincible_finished > 0 && !fInvincible )
	{
		pPlayer->tfstate &= ~TFSTATE_INVINCIBLE;
		pPlayer->items |= IT_INVULNERABILITY;
		pPlayer->invincible_finished = gpGlobals->time + Goal->invincible_finished;
	}
	if ( Goal->invisible_finished > 0 && !fInvisible )
	{
		pPlayer->tfstate &= ~TFSTATE_INVISIBLE;
		pPlayer->items |= IT_INVISIBILITY;
		pPlayer->invisible_finished = gpGlobals->time + Goal->invisible_finished;
	}
	if ( Goal->super_damage_finished > 0 && !fQuad )
	{
		pPlayer->tfstate &= ~TFSTATE_QUAD;
		pPlayer->items |= IT_QUAD;
		pPlayer->super_damage_finished = gpGlobals->time + Goal->super_damage_finished;
	}
	if ( Goal->radsuit_finished > 0 && !fRadsuit )
	{
		pPlayer->tfstate &= ~TFSTATE_RADSUIT;
		pPlayer->items |= IT_SUIT;
		pPlayer->radsuit_finished = gpGlobals->time + Goal->radsuit_finished;
	}
	pPlayer->TeamFortress_CheckClassStats();
	if ( Goal->replacement_model && Goal->replacement_model == pPlayer->replacement_model )
	{
		pPlayer->replacement_model = 0;
		pPlayer->replacement_model_body = 0;
		pPlayer->replacement_model_skin = 0;
		pPlayer->replacement_model_flags = 0;
		pPlayer->TeamFortress_SetSkin();
	}
}

void DoResults( CBaseEntity* Goal, CBasePlayer* AP, BOOL bAddBonuses )
{
	if ( cb_prematch_time > gpGlobals->time && Goal->Classify() != CLASS_TFGOAL_TIMER )
		return;
	if ( Goal->goal_state == TFGS_ACTIVE )
		return;
	if ( Goal->goal_state != TFGS_DELAYED && Goal->delay_time > 0 )
	{
		Goal->goal_state = TFGS_DELAYED;
		Goal->CreateTimer( TF_TIMER_DELAYEDGOAL );
		Goal->pev->enemy = AP->edict();
		Goal->SetThink( &CBaseEntity::DelayedResult );
		Goal->pev->nextthink = gpGlobals->time + Goal->delay_time;
		Goal->weapon = bAddBonuses;
		return;
	}
	if ( Goal->owned_by_teamcheck )
		Goal->owned_by = GetTeamCheckTeam( STRING( Goal->owned_by_teamcheck ) );
	Goal->goal_state = TFGS_INACTIVE;
	if ( Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_TIMER )
		Goal->pev->effects |= EF_NODRAW;
	if ( Goal->pev->noise )
		EMIT_SOUND( Goal->edict(), CHAN_VOICE, STRING( Goal->pev->noise ), 1, ATTN_NORM );
	for ( int i = 0; i < 4; i++ )
	{
		if ( Goal->increase_team[i] )
			TeamFortress_TeamIncreaseScore( i + 1, Goal->increase_team[i] );
	}
	if ( Goal->increase_team_owned_by && Goal->owned_by )
		TeamFortress_TeamIncreaseScore( Goal->owned_by, Goal->increase_team_owned_by );
	if ( *STRING( Goal->pev->netname ) )
	{
		if ( AP )
		{
			const char* pszTeam = AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR";
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\"\n", STRING( AP->pev->netname ),
				GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ), pszTeam, STRING( Goal->pev->netname ) );
		}
		else
		{
			UTIL_LogPrintf( "World triggered \"%s\"\n", STRING( Goal->pev->netname ) );
		}
	}
	if ( AP && CTF_Map == TRUE && ( Goal->goal_no & ~2 ) == 1 )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			int goal = Goal->goal_no;
			if ( ( pPlayer->team_no == 2 && goal == 1 ) || ( pPlayer->team_no == 1 && goal == 2 ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, pPlayer == AP
					? "You got the enemy flag!\n\nReturn to base!" : "Your team GOT the ENEMY flag!!" );
			}
			else if ( ( pPlayer->team_no == 2 && goal == 3 ) || ( pPlayer->team_no == 1 && goal == 4 ) )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER,
					pPlayer == AP ? "You CAPTURED the FLAG!!" : "Your flag was CAPTURED!!" );
			}
			else if ( goal == 1 || goal == 2 )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Your flag has been TAKEN!!" );
			}
			else if ( goal == 3 || goal == 4 )
			{
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Your team CAPTURED the flag!!" );
			}
		}
		switch ( Goal->goal_no )
		{
		case CTF_FLAG1:
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s GOT the BLUE flag!", STRING( AP->pev->netname ) ) );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Stole_Blue_Flag\"\n", STRING( AP->pev->netname ),
				GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ),
				AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR" );
			AP->items |= IT_KEY1;
			break;
		case CTF_FLAG2:
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s GOT the RED flag!", STRING( AP->pev->netname ) ) );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Stole_Red_Flag\"\n", STRING( AP->pev->netname ),
				GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ),
				AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR" );
			AP->items |= IT_KEY2;
			break;
		case CTF_DROPOFF1:
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s CAPTURED the RED flag!", STRING( AP->pev->netname ) ) );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Captured_Red_Flag\"\n", STRING( AP->pev->netname ),
				GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ),
				AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR" );
			AP->items &= ~IT_KEY2;
			break;
		case CTF_DROPOFF2:
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s CAPTURED the BLUE flag!", STRING( AP->pev->netname ) ) );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Captured_Blue_Flag\"\n", STRING( AP->pev->netname ),
				GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ),
				AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR" );
			AP->items &= ~IT_KEY1;
			break;
		}
	}
	if ( Goal->remove_spawngroup )
	{
		CBaseEntity* pSpawn = NULL;
		while ( ( pSpawn = UTIL_FindEntityByString( pSpawn, "netname", "info_player_teamspawn" ) ) != NULL )
		{
			if ( pSpawn->group_no == Goal->remove_spawngroup )
				pSpawn->goal_state = TFGS_REMOVED;
		}
	}
	if ( Goal->restore_spawngroup )
	{
		CBaseEntity* pSpawn = NULL;
		while ( ( pSpawn = UTIL_FindEntityByString( pSpawn, "netname", "info_player_teamspawn" ) ) != NULL )
		{
			if ( pSpawn->group_no == Goal->restore_spawngroup )
				pSpawn->goal_state = TFGS_INACTIVE;
		}
	}
	if ( Goal->broadcast && !CTF_Map )
		UTIL_LogPrintf( "World triggered \"%s\"\n", STRING( Goal->broadcast ) );
	if ( Goal->netname_broadcast && !CTF_Map && AP )
	{
		const char* pszTeam = AP->team_no ? GetTeamName( AP->team_no ) : "SPECTATOR";
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"%s\"\n", STRING( AP->pev->netname ),
			GETPLAYERUSERID( AP->edict() ), GETPLAYERAUTHID( AP->edict() ), pszTeam, STRING( Goal->netname_broadcast ) );
	}
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pPlayer )
			continue;
		if ( !CTF_Map )
		{
			if ( Goal->broadcast )
				UTIL_ShowMessage( STRING( Goal->broadcast ), pPlayer->edict() );
			if ( Goal->netname_broadcast && AP )
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, STRING( Goal->netname_broadcast ), STRING( AP->pev->netname ) );
			if ( Goal->org_broadcast )
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, STRING( Goal->org_broadcast ) );
		}
		if ( Goal->speak )
			pPlayer->ClientHearVox( STRING( Goal->speak ) );
		if ( pPlayer == AP )
		{
			if ( Goal->pev->message && Goal->Classify() != CLASS_TFSPAWN )
				UTIL_ShowMessage( STRING( Goal->pev->message ), AP->edict() );
			if ( Goal->org_message && Goal->Classify() != CLASS_TFSPAWN )
				ClientPrint( AP->pev, HUD_PRINTCENTER, STRING( Goal->org_message ) );
			if ( Goal->AP_speak )
				AP->ClientHearVox( STRING( Goal->AP_speak ) );
		}
		else
		{
			BOOL fTeam = AP && AP->team_no == pPlayer->team_no;
			BOOL fOwner = pPlayer->team_no == Goal->owned_by;
			string_t text = fOwner ? Goal->owners_team_broadcast : Goal->non_owners_team_broadcast;
			if ( !text )
				text = fTeam ? Goal->team_broadcast : Goal->non_team_broadcast;
			if ( text )
				UTIL_ShowMessage( STRING( text ), pPlayer->edict() );
			text = fOwner ? Goal->org_owners_team_broadcast : Goal->org_non_owners_team_broadcast;
			if ( !text )
				text = fTeam ? Goal->org_team_broadcast : Goal->org_non_team_broadcast;
			if ( text )
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, STRING( text ) );
			text = fOwner ? Goal->owners_team_speak : Goal->non_owners_team_speak;
			if ( !text )
				text = fTeam ? Goal->team_speak : Goal->non_team_speak;
			if ( text )
				pPlayer->ClientHearVox( STRING( text ) );
			text = fOwner ? Goal->netname_owners_team_broadcast : 0;
			if ( !text )
				text = fTeam ? Goal->netname_team_broadcast : Goal->netname_non_team_broadcast;
			if ( text && AP )
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, STRING( text ), STRING( AP->pev->netname ) );
		}
		if ( pPlayer->pev->playerclass
			&& ( !( Goal->goal_effects & TFGE_SAME_ENVIRONMENT )
			|| UTIL_PointContents( Goal->pev->origin ) == UTIL_PointContents( pPlayer->pev->origin ) )
			&& IsAffectedBy( Goal, pPlayer, AP )
			&& ( !Goal->search_time || !( Goal->goal_effects & TFGE_TIMER_CHECK_AP ) || APMeetsCriteria( Goal, pPlayer ) ) )
			Apply_Results( Goal, pPlayer, AP, bAddBonuses );
	}
	if ( Goal->Classify() == CLASS_TFGOAL_TIMER || Goal->Classify() == CLASS_TFGOAL )
		Goal->goal_state = TFGS_ACTIVE;
	if ( Goal->goal_result & TFGR_ENDGAME )
	{
		TeamFortress_TeamShowScores( TRUE, NULL );
		if ( g_pGameRules->IsMultiplayer() )
			( (CHalfLifeMultiplay*)g_pGameRules )->GoToIntermission();
		return;
	}
	if ( Goal->m_flEndRoundTime )
		EndRound( Goal );
	DoGroupWork( Goal, AP );
	DoGoalWork( Goal, AP );
	if ( Goal->Classify() == CLASS_TFGOAL_TIMER || Goal->Classify() == CLASS_TFGOAL_ITEM
		|| Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFSPAWN || Goal->do_triggerwork )
		DoTriggerWork( Goal, AP );
	if ( Goal->Classify() == CLASS_TFGOAL || Goal->Classify() == CLASS_TFGOAL_TIMER )
		SetupRespawn( Goal );
}

void tfgoalitem_GiveToPlayer( CBaseEntity* Item, CBasePlayer* AP, CBaseEntity* Goal )
{
	if ( Item->redrop_count )
		Item->SetThink( NULL );
	Item->pev->owner = AP->edict();
	Item->pev->movetype = MOVETYPE_FOLLOW;
	Item->pev->aiment = AP->edict();
	if ( Item->pev->model )
	{
		Item->pev->effects &= ~EF_NODRAW;
		CBaseAnimating* pAnimating = (CBaseAnimating*)Item;
		Item->pev->sequence = pAnimating->LookupSequence( "carried" );
		if ( Item->pev->sequence != -1 )
		{
			pAnimating->ResetSequenceInfo();
			Item->pev->frame = 0;
		}
	}
	Item->pev->solid = SOLID_NOT;
	if ( Item->goal_activation & TFGI_GLOW )
		AP->pev->effects |= EF_BRIGHTFIELD;
	if ( Item->goal_activation & TFGI_SLOW )
		AP->TeamFortress_SetSpeed();
	if ( Item->speed_reduction )
		AP->TeamFortress_SetSpeed();
	if ( Item->goal_activation & TFGI_ITEMGLOWS )
	{
		Item->pev->renderfx = kRenderFxNone;
		Item->pev->rendercolor = g_vecZero;
		Item->pev->renderamt = 0;
	}
	AP->items |= Item->items & ( IT_KEY1 | IT_KEY2 | IT_KEY3 | IT_KEY4 );
	if ( Item != Goal && ( Goal->goal_result & TFGR_NO_ITEM_RESULTS ) )
	{
		Item->goal_state = TFGS_ACTIVE;
	}
	else
	{
		if ( Item->goal_result & TFGR_REMOVE_DISGUISE )
			AP->is_unableto_spy_or_teleport = TRUE;
		DoResults( Item, AP, TRUE );
		DoItemGroupWork( Item, AP );
	}
}

void DoItemGroupWork( CBaseEntity* Item, CBasePlayer* AP )
{
	if ( Item->distance )
	{
		if ( !Item->pev->pain_finished )
			ALERT( at_console, "GoalItem %d has .distance specified, but no .pain_finished\n", Item->goal_no );
		BOOL fAll = TRUE;
		CBaseEntity* pItem = NULL;
		while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
		{
			if ( pItem->group_no == Item->distance && pItem->goal_state != TFGS_ACTIVE )
			{
				UTIL_FindEntityByClassname( pItem, "item_tfgoal" );
				fAll = FALSE;
				break;
			}
		}
		if ( fAll )
		{
			CBaseEntity* pGoal = Findgoal( (int)Item->pev->pain_finished );
			if ( pGoal )
				DoResults( pGoal, AP, Item->goal_result & TFGR_ADD_BONUSES );
		}
	}
	if ( !Item->pev->speed )
		return;
	if ( !Item->attack_finished )
		ALERT( at_console, "GoalItem %d has .speed specified, but no .attack_finished\n", Item->goal_no );
	CBaseEntity* pOwner = NULL;
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem->group_no != Item->pev->speed )
			continue;
		if ( pItem->goal_state != TFGS_ACTIVE )
		{
			UTIL_FindEntityByClassname( pItem, "item_tfgoal" );
			return;
		}
		CBaseEntity* pCarrier = CBaseEntity::Instance( pItem->pev->owner );
		if ( !pOwner )
			pOwner = pCarrier;
		else if ( pOwner != pCarrier )
		{
			UTIL_FindEntityByClassname( pItem, "item_tfgoal" );
			return;
		}
	}
	CBaseEntity* pGoal = Findgoal( (int)Item->attack_finished );
	if ( pGoal )
		DoResults( pGoal, AP, Item->goal_result & TFGR_ADD_BONUSES );
}

void tfgoalitem_RemoveFromPlayer( CBaseEntity* Item, CBasePlayer* AP, int iMethod )
{
	if ( Item->owned_by_teamcheck )
		Item->owned_by = GetTeamCheckTeam( STRING( Item->owned_by_teamcheck ) );
	BOOL fLight = FALSE;
	BOOL fSpyOff = FALSE;
	int keys = 0;
	CBaseEntity* pItem = NULL;
	while ( ( pItem = UTIL_FindEntityByClassname( pItem, "item_tfgoal" ) ) != NULL )
	{
		if ( pItem->pev->owner == AP->edict() && pItem != Item )
		{
			if ( pItem->goal_activation & TFGI_GLOW )
				fLight = TRUE;
			keys |= pItem->items & ( IT_KEY1 | IT_KEY2 | IT_KEY3 | IT_KEY4 );
			if ( pItem->goal_result & TFGR_REMOVE_DISGUISE )
				fSpyOff = TRUE;
		}
	}
	if ( !fLight && AP->invincible_finished <= gpGlobals->time + 3 && AP->super_damage_finished <= gpGlobals->time + 3 )
		AP->pev->effects &= ~( EF_BRIGHTFIELD | EF_DIMLIGHT );
	if ( Item->goal_activation & TFGI_ITEMGLOWS )
	{
		Item->pev->renderfx = kRenderFxGlowShell;
		Item->pev->rendercolor = rgbcolors[Item->owned_by >= 1 && Item->owned_by <= 4 ? Item->owned_by : 0];
		Item->pev->renderamt = 100;
	}
	if ( !fSpyOff )
		AP->is_unableto_spy_or_teleport = FALSE;
	AP->items &= ~( ( IT_KEY1 | IT_KEY2 | IT_KEY3 | IT_KEY4 ) & ~keys );
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( pPlayer && pPlayer->pev->playerclass
			&& ( !( Item->goal_effects & TFGE_SAME_ENVIRONMENT )
			|| UTIL_PointContents( Item->pev->origin ) == UTIL_PointContents( pPlayer->pev->origin ) )
			&& IsAffectedBy( Item, pPlayer, AP ) )
			RemoveResults( Item, pPlayer );
	}
	if ( Item->pev->model )
	{
		CBaseAnimating* pAnimating = (CBaseAnimating*)Item;
		Item->pev->sequence = pAnimating->LookupSequence( "not_carried" );
		if ( Item->pev->sequence != -1 )
		{
			pAnimating->ResetSequenceInfo();
			Item->pev->frame = 0;
		}
	}
	if ( iMethod == GI_DROP_PLAYERDEATH || iMethod == GI_DROP_PLAYERDROP )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			BOOL fTeam = pPlayer->team_no == Item->owned_by;
			string_t text = fTeam ? Item->team_drop : Item->non_team_drop;
			if ( text )
				UTIL_ShowMessage( STRING( text ), pPlayer->edict() );
			text = fTeam ? Item->netname_team_drop : Item->netname_non_team_drop;
			if ( text )
				ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, STRING( text ), STRING( AP->pev->netname ) );
			text = fTeam ? Item->org_team_drop : Item->org_non_team_drop;
			if ( text )
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, STRING( text ) );
		}
		if ( Item->goal_activation & TFGI_RETURN_DROP )
		{
			CBaseEntity* pTimer = Item->CreateTimer( TF_TIMER_RETURNITEM );
			pTimer->weapon = iMethod != 0;
			pTimer->SetThink( &CBaseEntity::ReturnItem );
			pTimer->pev->nextthink = gpGlobals->time + 0.5;
		}
		else if ( !( Item->goal_activation & TFGI_DROP ) )
		{
			Item->pev->owner = NULL;
			Item->pev->nextthink = gpGlobals->time;
			Item->SetThink( &CBaseEntity::SUB_Remove );
			AP->TeamFortress_SetSpeed();
			return;
		}
		else
		{
			tfgoalitem_drop( Item, iMethod == GI_DROP_PLAYERDROP && ( Item->goal_activation & TFGI_CANBEDROPPED ), AP );
		}
		Item->pev->owner = NULL;
		Item->pev->flags &= ~FL_ONGROUND;
		UTIL_SetSize( Item->pev, Item->goal_min, Item->goal_max );
		AP->TeamFortress_SetSpeed();
	}
	else if ( iMethod == GI_DROP_REMOVEGOAL )
	{
		Item->pev->owner = NULL;
		if ( Item->goal_activation & TFGI_RETURN_GOAL )
		{
			CBaseEntity* pTimer = Item->CreateTimer( TF_TIMER_RETURNITEM );
			pTimer->SetThink( &CBaseEntity::ReturnItem );
			pTimer->pev->nextthink = gpGlobals->time + 0.5;
			pTimer->weapon = GI_RET_GOAL;
		}
		else
		{
			Item->pev->solid = SOLID_NOT;
			Item->pev->effects |= EF_NODRAW;
			Item->pev->movetype = MOVETYPE_NONE;
			Item->pev->aiment = NULL;
		}
		AP->TeamFortress_SetSpeed();
	}
}

void CBaseEntity::ReturnItem()
{
	CBaseEntity* Item = Instance( pev->owner );
	Item->SetThink( NULL );
	Item->goal_state = TFGS_INACTIVE;
	Item->pev->solid = ( Item->goal_activation & TFGI_SOLID ) && FClassnameIs( Item->pev, "item_tfgoal" )
		? SOLID_BBOX : SOLID_TRIGGER;
	Item->pev->movetype = MOVETYPE_NONE;
	Item->pev->aiment = NULL;
	Item->SetTouch( &CBaseEntity::item_tfgoal_touch );
	Item->pev->angles = g_vecZero;
	Item->pev->origin = Item->pev->oldorigin;
	Item->pev->effects &= ~EF_NODRAW;
	UTIL_SetOrigin( Item->pev, Item->pev->origin );
	EMIT_SOUND_DYN( Item->edict(), CHAN_WEAPON, "items/itembk2.wav", 1, ATTN_NORM, 0, 150 );
	tfgoalitem_checkgoalreturn( Item );
	if ( weapon != GI_RET_GOAL && ( Item->pev->noise3 || Item->noise4 || Item->org_noise3 || Item->org_noise4 ) )
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( !pPlayer )
				continue;
			if ( Item->owned_by_teamcheck )
				Item->owned_by = GetTeamCheckTeam( STRING( Item->owned_by_teamcheck ) );
			BOOL fTeam = pPlayer->team_no == Item->owned_by;
			string_t text = fTeam ? Item->pev->noise3 : Item->noise4;
			if ( text )
			{
				UTIL_ShowMessage( STRING( text ), pPlayer->edict() );
			}
			else
			{
				text = fTeam ? Item->org_noise3 : Item->org_noise4;
				if ( text )
					ClientPrint( pPlayer->pev, HUD_PRINTCENTER, STRING( text ) );
			}
		}
	}
	UTIL_Remove( this );
}

void DisplayItemStatus( CBaseEntity* Goal, CBasePlayer* Player, CBaseEntity* Item )
{
	if ( Item->owned_by_teamcheck )
		Item->owned_by = GetTeamCheckTeam( STRING( Item->owned_by_teamcheck ) );
	BOOL fTeam = Player->team_no == Item->owned_by;
	if ( Item->goal_state == TFGS_ACTIVE )
	{
		if ( !Goal->team_str_carried && !Goal->non_team_str_carried )
			return;
		CBaseEntity* pOwner = CBaseEntity::Instance( Item->pev->owner );
		const char* pszCarrier = Player->edict() == Item->pev->owner ? "you" : STRING( pOwner->pev->netname );
		ClientPrint( Player->pev, HUD_PRINTTALK, STRING( fTeam ? Goal->team_str_carried : Goal->non_team_str_carried ),
			pszCarrier );
	}
	else if ( Item->pev->origin == Item->pev->oldorigin )
	{
		if ( Goal->team_str_home || Goal->non_team_str_home )
			ClientPrint( Player->pev, HUD_PRINTTALK, STRING( fTeam ? Goal->team_str_home : Goal->non_team_str_home ) );
	}
	else
	{
		if ( Goal->team_str_moved || Goal->non_team_str_moved )
			ClientPrint( Player->pev, HUD_PRINTTALK, STRING( fTeam ? Goal->team_str_moved : Goal->non_team_str_moved ) );
	}
}

void CBaseEntity::item_tfgoal_touch( CBaseEntity* pOther )
{
	if ( pOther->Classify() != CLASS_PLAYER || !pOther->IsAlive() || cb_prematch_time > gpGlobals->time
		|| pOther->is_feigning || ( replacement_model && pOther->replacement_model ) )
		return;
	CTFGoalItem* pItem = (CTFGoalItem*)this;
	if ( pev->enemy && pItem->m_flDroppedAt && pev->enemy == pOther->edict()
		&& pItem->m_flDroppedAt + 5 > gpGlobals->time )
		return;
	pItem->m_flDroppedAt = 0;
	TraceResult tr;
	Vector vecEnd = pOther->Center();
	Vector vecStart = Center();
	UTIL_TraceLine( vecStart, vecEnd, ignore_monsters, edict(), &tr );
	if ( tr.flFraction != 1 && tr.pHit != pOther->edict() )
		return;
	if ( CTF_Map )
	{
		if ( pev->oldorigin == pev->origin )
		{
			if ( ( pOther->team_no == 1 && goal_no == CTF_FLAG1 ) || ( pOther->team_no == 2 && goal_no == CTF_FLAG2 ) )
				return;
		}
		else if ( team_no == 1 )
		{
			BOOL fBlue = goal_no == CTF_FLAG1;
			UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( fBlue ? "%s RETURNED the BLUE flag!"
				: "%s RETURNED the RED flag!", STRING( pOther->pev->netname ) ) );
			const char* pszTeam = pOther->team_no ? GetTeamName( pOther->team_no ) : "SPECTATOR";
			UTIL_LogPrintf( fBlue ? "\"%s<%i><%s><%s>\" triggered \"Returned_Blue_Flag\"\n"
				: "\"%s<%i><%s><%s>\" triggered \"Returned_Red_Flag\"\n", STRING( pOther->pev->netname ),
				GETPLAYERUSERID( pOther->edict() ), GETPLAYERAUTHID( pOther->edict() ), pszTeam );
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
				if ( !pPlayer )
					continue;
				if ( ( pPlayer->team_no == 1 && goal_no == CTF_FLAG1 )
					|| ( pPlayer->team_no == 2 && goal_no == CTF_FLAG2 ) )
					ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "Your flag was RETURNED!!\n" );
				else if ( pPlayer->team_no != 1 && goal_no == CTF_FLAG1 )
					ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "The ENEMY flag was RETURNED!!\n" );
			}
			goal_state = TFGS_INACTIVE;
			pev->solid = SOLID_TRIGGER;
			SetTouch( &CBaseEntity::item_tfgoal_touch );
			pev->origin = pev->oldorigin;
			UTIL_SetOrigin( pev, pev->origin );
			EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "items/itembk2.wav", 1, ATTN_NORM, 0, 150 );
			return;
		}
	}
	if ( ActivationSucceeded( this, (CBasePlayer*)pOther, NULL ) )
	{
		tfgoalitem_GiveToPlayer( this, (CBasePlayer*)pOther, this );
		if ( pOther->pev->health > 0 )
			goal_state = TFGS_ACTIVE;
	}
}

void ParseTFDetect( CBaseEntity* Detect )
{
	if ( Detect->team_broadcast )
		team_menu_string = Detect->team_broadcast;
	if ( Detect->last_impulse )
		number_of_teams = (float)Detect->last_impulse;
	if ( Detect->speak )
		team_names[1] = Detect->speak;
	if ( Detect->AP_speak )
		team_names[2] = Detect->AP_speak;
	if ( Detect->team_speak )
		team_names[3] = Detect->team_speak;
	if ( Detect->owners_team_speak )
		team_names[4] = Detect->owners_team_speak;
	if ( Detect->pev->message )
		SERVER_COMMAND( (char*)STRING( Detect->pev->message ) );
	teamlives[1] = Detect->ammo_shells;
	teamlives[2] = Detect->ammo_nails;
	teamlives[3] = Detect->ammo_rockets;
	teamlives[4] = Detect->ammo_cells;
	for ( int i = 1; i <= 4; i++ )
	{
		if ( !teamlives[i] )
			teamlives[i] = -1;
	}
	teammaxplayers[1] = Detect->ammo_medikit;
	teammaxplayers[2] = Detect->ammo_detpack;
	teammaxplayers[3] = Detect->maxammo_medikit;
	teammaxplayers[4] = Detect->maxammo_detpack;
	for ( int i = 1; i <= 4; i++ )
	{
		if ( !teammaxplayers[i] )
			teammaxplayers[i] = 100;
	}
	illegalclasses[1] = Detect->maxammo_shells;
	illegalclasses[2] = Detect->maxammo_nails;
	illegalclasses[3] = Detect->maxammo_rockets;
	illegalclasses[0] = Detect->pev->playerclass;
	illegalclasses[4] = Detect->maxammo_cells;
	civilianteams = 0;
	for ( int i = 1; i <= 4; i++ )
	{
		if ( illegalclasses[i] == -1 )
		{
			civilianteams |= 1 << ( i - 1 );
			illegalclasses[i] = 0;
		}
	}
}

void ParseTFServerSettings()
{
	toggleflags &= ~TFLAG_CHEATCHECK;
	time_t clock;
	time( &clock );
	struct tm* date = localtime( &clock );
	if ( date->tm_mon == 11 && date->tm_mday == 25 )
		christmas = TRUE;
	if ( tfc_birthday.value || ( date->tm_mon == 7 && date->tm_mday == 24 ) || christmas )
	{
		birthday = TRUE;
		CBaseEntity* pTimer = CBaseEntity::Create( "timer", g_vecZero, g_vecZero, NULL );
		pTimer->weapon = 10;
		pTimer->SetThink( &CBaseEntity::Timer_Birthday );
		pTimer->pev->nextthink = gpGlobals->time + 60;
	}
	cb_prematch_time = 60 * tfc_clanbattle_prematch.value;
	if ( !cb_prematch_time )
		cb_prematch_time = 60 * tfc_prematch.value;
	if ( tfc_clanbattle.value )
	{
		clan_scores_dumped = 0;
		cb_ceasefire_time = 60 * tfc_clanbattle_ceasefire.value;
		if ( cb_ceasefire_time )
		{
			last_cease_fire = TRUE;
			initial_cease_fire = TRUE;
			cease_fire = TRUE;
		}
	}
	autokick_kills = tfc_autokick_kills.value;
	if ( tfc_fragscoring.value )
		toggleflags |= TFLAG_FRAGSCORING;
	else
		toggleflags &= ~TFLAG_FRAGSCORING;
	gpGlobals->teamplay = teamplay.value;
}

void ParseTFMapSettings()
{
	flagem_checked = 0;
	CBaseEntity* pDetect = UTIL_FindEntityByClassname( NULL, "info_tfdetect" );
	if ( pDetect )
	{
		if ( !teamplay.value )
		{
			CVAR_SET_FLOAT( "mp_teamplay", 1 );
			gpGlobals->teamplay = 1;
		}
		ParseTFDetect( pDetect );
		if ( number_of_teams <= 0 || number_of_teams >= 5 )
			number_of_teams = 4;
	}
	else
	{
		if ( UTIL_FindEntityByClassname( NULL, "info_player_team1" ) || CTF_Map )
		{
			CTF_Map = TRUE;
			if ( !teamplay.value )
			{
				CVAR_SET_FLOAT( "mp_teamplay", 1 );
				gpGlobals->teamplay = 1;
			}
			number_of_teams = 2;
		}
		else
		{
			number_of_teams = 4;
		}
		for ( int i = 1; i <= 4; i++ )
		{
			teamlives[i] = -1;
			illegalclasses[i] = 0;
			teammaxplayers[i] = 100;
			g_pLastSpawns[i] = NULL;
		}
		civilianteams = 0;
	}
	for ( int i = 1; i <= 4; i++ )
	{
		teamfrags[i] = 0;
		teamscores[i] = 0;
	}
	cease_fire = FALSE;
	autokick_kills = 0;
	g_fNextPrematchAlert = 0;
}

void EndRound( CBaseEntity* pGoal )
{
	UTIL_ScreenFadeAll( g_vecZero, 0.3f, pGoal->m_flEndRoundTime, 255, FFADE_OUT | FFADE_MODULATE );
	TeamFortress_TeamShowScores( TRUE, NULL );
	int iWinningTeam = 1;
	if ( pGoal->m_iszEndRoundMsg_Team1_Win )
	{
		int iBest = -99990;
		for ( int i = 1; i <= 4; i++ )
		{
			int score = TeamFortress_TeamGetScoreFrags( i );
			if ( score > iBest )
			{
				iWinningTeam = i;
				iBest = score;
			}
		}
	}
	const string_t wins[4] = { pGoal->m_iszEndRoundMsg_Team1_Win, pGoal->m_iszEndRoundMsg_Team2_Win,
		pGoal->m_iszEndRoundMsg_Team3_Win, pGoal->m_iszEndRoundMsg_Team4_Win };
	const string_t losses[4] = { pGoal->m_iszEndRoundMsg_Team1_Lose, pGoal->m_iszEndRoundMsg_Team2_Lose,
		pGoal->m_iszEndRoundMsg_Team3_Lose, pGoal->m_iszEndRoundMsg_Team4_Lose };
	const string_t messages[4] = { pGoal->m_iszEndRoundMsg_Team1, pGoal->m_iszEndRoundMsg_Team2,
		pGoal->m_iszEndRoundMsg_Team3, pGoal->m_iszEndRoundMsg_Team4 };
	no_cease_fire_text = TRUE;
	cease_fire = TRUE;
	CBasePlayer* pPlayer = NULL;
	while ( ( pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( pPlayer, "player" ) ) != NULL )
	{
		if ( !pPlayer->pev->iuser1 )
		{
			pPlayer->pev->iuser4 = 1;
			pPlayer->m_iHideHUD |= 9;
			pPlayer->tfstate |= TFSTATE_CANT_MOVE;
		}
		pPlayer->TeamFortress_SetSpeed();
		string_t message = pPlayer->team_no == pGoal->owned_by
			? pGoal->m_iszEndRoundMsg_OwnedBy : pGoal->m_iszEndRoundMsg_NonOwnedBy;
		if ( !message )
		{
			int team = pPlayer->team_no;
			if ( team >= 1 && team <= 4 )
				message = pGoal->m_iszEndRoundMsg_Team1_Win
					? ( team == iWinningTeam ? wins[team - 1] : losses[team - 1] ) : messages[team - 1];
		}
		UTIL_ShowMessage( STRING( message ), pPlayer->edict() );
	}
	CBaseEntity* pTimer = pGoal->CreateTimer( TF_TIMER_ENDROUND );
	pTimer->SetThink( &CBaseEntity::EndRoundEnd );
	pTimer->pev->nextthink = gpGlobals->time + pGoal->m_flEndRoundTime;
}

void CBaseEntity::EndRoundEnd()
{
	cease_fire = FALSE;
	CBasePlayer* pPlayer = NULL;
	while ( ( pPlayer = (CBasePlayer*)UTIL_FindEntityByClassname( pPlayer, "player" ) ) != NULL )
	{
		if ( !pPlayer->pev->iuser1 )
		{
			pPlayer->pev->iuser4 = 0;
			pPlayer->tfstate &= ~TFSTATE_CANT_MOVE;
			pPlayer->m_iHideHUD = 0;
		}
		pPlayer->TeamFortress_SetSpeed();
	}
	pev->flags |= FL_KILLME;
	no_cease_fire_text = FALSE;
}
