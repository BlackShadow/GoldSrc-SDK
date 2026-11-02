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

void GameDLLInit();
void GameDLLShutdown();

extern cvar_t displaysoundlist;

// multiplayer server rules
extern cvar_t teamplay;
extern cvar_t fraglimit;
extern cvar_t timelimit;
extern cvar_t fragsleft;
extern cvar_t timeleft;
extern cvar_t friendlyfire;
extern cvar_t falldamage;
extern cvar_t weaponstay;
extern cvar_t forcerespawn;
extern cvar_t flashlight;
extern cvar_t aimcrosshair;
extern cvar_t decalfrequency;
extern cvar_t mp_chattime;
extern cvar_t allow_spectators;

extern cvar_t tfc_spam_penalty1;
extern cvar_t tfc_spam_penalty2;
extern cvar_t tfc_spam_limit;
extern cvar_t tfc_autoteam;
extern cvar_t tfc_birthday;
extern cvar_t tfc_fragscoring;
extern cvar_t tfc_respawndelay;
extern cvar_t tfc_clanbattle;
extern cvar_t tfc_clanbattle_prematch;
extern cvar_t tfc_prematch;
extern cvar_t tfc_clanbattle_ceasefire;
extern cvar_t tfc_clanbattle_locked;
extern cvar_t tfc_adminpwd;
extern cvar_t tfc_balance_teams;
extern cvar_t tfc_balance_scores;
extern cvar_t tfc_autokick_time;
extern cvar_t tfc_autokick_kills;
extern cvar_t tfc_spectchat;
extern cvar_t tfc_playerid;

extern cvar_t cr_scout;
extern cvar_t cr_sniper;
extern cvar_t cr_soldier;
extern cvar_t cr_demoman;
extern cvar_t cr_medic;
extern cvar_t cr_hwguy;
extern cvar_t cr_pyro;
extern cvar_t cr_spy;
extern cvar_t cr_engineer;
extern cvar_t cr_random;
extern cvar_t sv_pushable_fixed_tick_fudge;

extern cvar_t* g_footsteps;

#endif
