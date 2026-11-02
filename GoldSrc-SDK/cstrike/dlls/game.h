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

#ifndef GAME_H
#define GAME_H

// Engine Cvars
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_aim;
extern cvar_t *g_footsteps;
extern cvar_t *g_psv_accelerate;
extern cvar_t *g_psv_friction;
extern cvar_t *g_psv_stopspeed;

extern cvar_t displaysoundlist;
extern cvar_t timelimit;
extern cvar_t friendlyfire;
extern cvar_t infiniteAmmo;
extern cvar_t flashlight;
extern cvar_t decalfrequency;
extern cvar_t allowmonsters;
extern cvar_t roundtime;
extern cvar_t buytime;
extern cvar_t freezetime;
extern cvar_t c4timer;
extern cvar_t ghostfrequency;
extern cvar_t autokick;
extern cvar_t autokick_timeout;
extern cvar_t restartround;
extern cvar_t sv_restart;
extern cvar_t limitteams;
extern cvar_t autoteambalance;
extern cvar_t tkpunish;
extern cvar_t hostagepenalty;
extern cvar_t mirrordamage;
extern cvar_t logmessages;
extern cvar_t forcecamera;
extern cvar_t forcechasecam;
extern cvar_t mapvoteratio;
extern cvar_t maxrounds;
extern cvar_t winlimit;
extern cvar_t windifference;
extern cvar_t fadetoblack;
extern cvar_t logdetail;
extern cvar_t startmoney;
extern cvar_t playerid;
extern cvar_t allow_spectators;
extern cvar_t mp_chattime;
extern cvar_t kick_percent;
extern cvar_t fragsleft;
extern cvar_t timeleft;
extern cvar_t humans_join_team;
extern cvar_t scoreboard_showhealth;
extern cvar_t scoreboard_showmoney;
extern cvar_t sv_pushable_fixed_tick_fudge;

void GameDLLInit();

#endif // GAME_H
