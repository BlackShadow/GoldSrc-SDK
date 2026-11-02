#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "bot_util.h"
#include "tutor.h"
#include "tutor_cs_tutor.h"

CBaseTutor* TheTutor = NULL;

cvar_t cv_tutor_message_repeats = { "_tutor_message_repeats", "5", FCVAR_SERVER };
cvar_t cv_tutor_debug_level = { "_tutor_debug_level", "0", FCVAR_SERVER };
cvar_t cv_tutor_view_distance = { "_tutor_view_distance", "1000", FCVAR_SERVER };
cvar_t cv_tutor_viewable_check_interval = { "_tutor_bomb_viewable_check_interval", "0.5", FCVAR_SERVER };
cvar_t cv_tutor_look_distance = { "_tutor_look_distance", "200", FCVAR_SERVER };
cvar_t cv_tutor_look_angle = { "_tutor_look_angle", "10", FCVAR_SERVER };
cvar_t cv_tutor_examine_time = { "_tutor_examine_time", "0.5", FCVAR_SERVER };
cvar_t cv_tutor_message_minimum_display_time = { "_tutor_message_minimum_display_time", "1", FCVAR_SERVER };
cvar_t cv_tutor_message_character_display_time_coefficient = { "_tutor_message_character_display_time_coefficient", "0.07", FCVAR_SERVER };
cvar_t cv_tutor_hint_interval_time = { "_tutor_hint_interval_time", "10.0", FCVAR_SERVER };

static bool s_tutorDisabledThisGame = false;
static float s_nextCvarCheckTime = 0;

void InstallTutor( bool start )
{
	if ( TheTutor != NULL )
	{
		delete TheTutor;
		TheTutor = NULL;
	}
	if ( start )
		TheTutor = new CCSTutor;
	s_tutorDisabledThisGame = false;
	s_nextCvarCheckTime = gpGlobals->time;
}

void Tutor_RegisterCVars()
{
	CVAR_REGISTER( &cv_tutor_message_repeats );
	CVAR_REGISTER( &cv_tutor_debug_level );
	CVAR_REGISTER( &cv_tutor_view_distance );
	CVAR_REGISTER( &cv_tutor_viewable_check_interval );
	CVAR_REGISTER( &cv_tutor_look_distance );
	CVAR_REGISTER( &cv_tutor_look_angle );
	CVAR_REGISTER( &cv_tutor_examine_time );
	CVAR_REGISTER( &cv_tutor_message_minimum_display_time );
	CVAR_REGISTER( &cv_tutor_message_character_display_time_coefficient );
	CVAR_REGISTER( &cv_tutor_hint_interval_time );
}

void MonitorTutorStatus()
{
	static cvar_t* tutor_enable = NULL;
	static bool tutor_enableCvarExists = true;
	if ( !tutor_enableCvarExists || !(s_nextCvarCheckTime <= gpGlobals->time) )
		return;
	if ( tutor_enable == NULL )
	{
		tutor_enable = CVAR_GET_POINTER( "tutor_enable" );
		if ( tutor_enable == NULL )
			tutor_enableCvarExists = false;
	}
	bool enabled = tutor_enableCvarExists && !s_tutorDisabledThisGame && tutor_enable->value > 0;
	int humans = 0;
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer* player = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( player != NULL && !player->IsBot() )
			humans++;
	}
	if ( humans > 1 )
	{
		if ( enabled )
			s_tutorDisabledThisGame = true;
		enabled = false;
	}
	if ( enabled )
	{
		if ( TheTutor == NULL )
		{
			CBasePlayer* player = UTIL_GetLocalPlayer();
			if ( player != NULL )
				ClientPrint( player->pev, HUD_PRINTCENTER, "#CZero_Tutor_Turned_On" );
			TheTutor = new CCSTutor;
		}
	}
	else if ( TheTutor != NULL )
	{
		CBasePlayer* player = UTIL_GetLocalPlayer();
		if ( player != NULL )
			ClientPrint( player->pev, HUD_PRINTCENTER, "#CZero_Tutor_Turned_Off" );
		delete TheTutor;
		TheTutor = NULL;
	}
	s_nextCvarCheckTime = gpGlobals->time + 1.0f;
}
