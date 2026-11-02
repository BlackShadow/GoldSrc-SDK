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

void GameDLLInit(void);

// Engine Cvars
extern cvar_t *g_psv_gravity;
extern cvar_t *g_psv_aim;
extern cvar_t *g_footsteps;

extern cvar_t cvar_weaponstay;
extern cvar_t cvar_combinemglimits;
extern cvar_t cvar_alliesclasses;
extern cvar_t cvar_axisclasses;
extern cvar_t cvar_spawnbazookas;
extern cvar_t cvar_timelimit;
extern cvar_t cvar_friendlyfire;
extern cvar_t cvar_falldamage;
extern cvar_t cvar_flashlight;
extern cvar_t cvar_fadetoblack;
extern cvar_t cvar_forcechasecam;
extern cvar_t cvar_forcecamera;
extern cvar_t cvar_waveTime;
extern cvar_t cvar_clanRestartRound;
extern cvar_t cvar_clanReadyRestart;
extern cvar_t cvar_clanMatchWarmup;
extern cvar_t cvar_clanMatch;
extern cvar_t cvar_clanTimer;
extern cvar_t cvar_clanScoring;
extern cvar_t cvar_clanScoringValuesAllies;
extern cvar_t cvar_clanScoringValuesAxis;
extern cvar_t cvar_clanScoringDelay;
extern cvar_t cvar_clanReadyString;
extern cvar_t cvar_clanScoringBonusAllies;
extern cvar_t cvar_clanScoringBonusAxis;
extern cvar_t cvar_limitAlliesGarand;
extern cvar_t cvar_limitAlliesCarbine;
extern cvar_t cvar_limitAlliesThompson;
extern cvar_t cvar_limitAlliesGreaseGun;
extern cvar_t cvar_limitAlliesSpring;
extern cvar_t cvar_limitAlliesBAR;
extern cvar_t cvar_limitAllies30cal;
extern cvar_t cvar_limitAlliesBazooka;
extern cvar_t cvar_limitAlliesMortar;
extern cvar_t cvar_limitAxisKar;
extern cvar_t cvar_limitAxisK43;
extern cvar_t cvar_limitAxisScopedKar;
extern cvar_t cvar_limitAxisMP40;
extern cvar_t cvar_limitAxisMP44;
extern cvar_t cvar_limitAxisFG42;
extern cvar_t cvar_limitAxisFG42scope;
extern cvar_t cvar_limitAxisMG34;
extern cvar_t cvar_limitAxisMG42;
extern cvar_t cvar_limitAxisPschreck;
extern cvar_t cvar_limitAxisMortar;
extern cvar_t cvar_limitBritLight;
extern cvar_t cvar_limitBritMedium;
extern cvar_t cvar_limitBritSniper;
extern cvar_t cvar_limitBritMger;
extern cvar_t cvar_limitBritPiat;
extern cvar_t cvar_limitBritMortar;
extern cvar_t cvar_displaysoundlist;
extern cvar_t cvar_teamplay;
extern cvar_t cvar_decalfrequency;
extern cvar_t cvar_teamoverride;
extern cvar_t cvar_defaultteam;
extern cvar_t cvar_chattime;
extern cvar_t cvar_nummapmarkers;
extern cvar_t cvar_markerstaytime;
extern cvar_t cvar_dev_serverhitbox;
extern cvar_t cvar_teamSpectators;
extern cvar_t cvar_showCutscenes;
extern cvar_t cvar_logScores;
extern cvar_t cvar_logScoresDelay;
extern cvar_t cvar_tkPenalty;
extern cvar_t cvar_teamOver;
extern cvar_t cvar_deathMsg;
extern cvar_t cvar_chatMsg;
extern cvar_t cvar_teamlist;

#endif
