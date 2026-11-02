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
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"saverestore.h"
#include	"client.h"
#include	"decals.h"
#include	"gamerules.h"
#include	"game.h"
#include "utlvector.h"
#include "MemPool.h"
#include <stdarg.h>
#include <time.h>
#include "perf_counter.h"

CUtlVector<hash_item_t> stringsHashTable;
CMemoryPool hashItemMemPool( sizeof( hash_item_t ), 256 );

char* pszDodEntityNames[] =
{
	"info_alias", "info_group", "multi_alias", "trigger_changealias", "func_wall", "func_wall_toggle",
	"func_conveyor", "func_illusionary", "func_monsterclip", "func_rotating", "func_wall", "func_wall_toggle",
	"func_conveyor", "func_illusionary", "func_monsterclip", "func_rotating", "func_pendulum", "env_global",
	"env_state", "multisource", "func_button", "func_rot_button", "momentary_rot_button", "env_spark",
	"env_debris", "button_target", "info_doddetect", "para_roundtimer", "dod_preround", "dod_stats", "func_door",
	"func_water", "func_door_rotating", "momentary_door", "info_target", "env_bubbles", "beam", "env_laser",
	"env_glow", "env_sprite", "gibshooter", "env_shooter", "test_effect", "env_blood", "env_shake", "env_fade",
	"env_message", "env_funnel", "env_rain", "env_warpball", "env_beverage", "item_sodacan", "env_model",
	"env_rocketshooter", "spark_shower", "env_explosion", "func_breakable", "func_pushable", "func_tank",
	"func_tanklaser", "func_tankrocket", "func_tankmortar", "func_tankcontrols", "monster_generic", "grenade",
	"cine_blood", "cycler", "cycler_prdroid", "cycler_sprite", "cycler_weapon", "cycler_wreckage", "light",
	"light_spot", "light_environment", "trigger_lightstyle", "dod_mapmarker", "game_score", "game_end", "game_text",
	"game_team_master", "game_team_set", "game_zone_player", "game_player_hurt", "game_counter", "game_counter_set",
	"game_player_equip", "game_player_team", "func_mortar_field", "monster_mortar", "info_node", "info_node_air",
	"testhull", "node_viewer", "node_viewer_human", "node_viewer_fly", "node_viewer_large", "dod_object",
	"dod_score_ent", "particle_shooter", "path_corner", "path_track", "func_plat", "func_platrot", "func_train",
	"func_tracktrain", "func_traincontrols", "func_trackchange", "func_trackautochange", "func_guntarget", "player",
	"monster_hevsuit_dead", "player_weaponstrip", "player_loadsaved", "info_intermission", "scripted_sequence",
	"scripted_action", "aiscripted_sequence", "scripted_sentence", "monster_furniture", "ambient_generic", "env_sound",
	"speaker", "soundent", "info_null", "info_player_deathmatch", "info_player_axis", "info_player_allies",
	"info_player_observer", "info_initial_player_axis", "info_initial_player_allies", "info_player_start",
	"info_landmark", "DelayedUse", "weapon_stickgrenade", "func_friction", "trigger_auto", "trigger_relay",
	"multi_manager", "multi_watcher", "watcher", "env_render", "env_customize", "trigger", "trigger_hurt",
	"trigger_monsterjump", "trigger_cdaudio", "target_cdaudio", "trigger_multiple", "trigger_once", "trigger_counter",
	"trigger_transition", "fireanddie", "trigger_changelevel", "func_ladder", "trigger_push", "trigger_teleport",
	"info_teleport_destination", "trigger_autosave", "trigger_endsection", "trigger_gravity", "trigger_startpatrol",
	"trigger_changetarget", "trigger_camera", "dod_object_goal", "dod_location", "dod_control_point",
	"dod_control_point_master", "dod_trigger_sandbag", "dod_capture_area", "dod_point_relay", "dod_round_timer",
	"dod_camera", "monster_turret", "monster_miniturret", "monster_sentry", "weaponbox", "dod_ammo",
	"ammo_generic_american", "ammo_generic_german", "ammo_generic_british", "infodecal", "worldspawn", "weapon_30cal",
	"ammo_30cal", "weapon_amerknife", "weapon_bar", "ammo_bar", "weapon_bazooka", "shell_bazooka", "ammo_bazooka",
	"weapon_bren", "ammo_bren", "weapon_colt", "ammo_colt", "weapon_enfield", "weapon_scopedenfield", "ammo_enfield",
	"weapon_fg42", "weapon_scopedfg42", "ammo_fg42", "weapon_garand", "ammo_garand", "weapon_gerknife",
	"weapon_greasegun", "ammo_greasegun", "weapon_handgrenade", "weapon_handgrenade_ex", "weapon_k43", "ammo_k43",
	"weapon_kar", "ammo_kar", "weapon_luger", "ammo_luger", "weapon_m1carbine", "ammo_m1carbine", "weapon_mg34",
	"ammo_mg34", "weapon_mg42", "ammo_mg42", "weapon_mp40", "ammo_mp40", "weapon_mp44", "ammo_mp44", "weapon_piat",
	"shell_piat", "ammo_piat", "weapon_pschreck", "shell_pschreck", "ammo_pschreck", "weapon_scopedkar",
	"ammo_scopedkar", "weapon_spade", "weapon_spring", "ammo_spring", "weapon_sten", "ammo_sten",
	"weapon_stickgrenade", "weapon_stickgrenade_ex", "weapon_thompson", "ammo_thompson", "weapon_webley", "ammo_webley",
	NULL
};

void OnFreeEntPrivateData( edict_t* entity );
void GameShutdown();
int ShouldCollide( edict_t* touched, edict_t* other );
static NEW_DLL_FUNCTIONS gNewFunctionTable =
{
	OnFreeEntPrivateData,
	GameShutdown,
	ShouldCollide,
	NULL,
	NULL
};

void GameShutdown()
{
}

int ShouldCollide( edict_t* touched, edict_t* other )
{
	return TRUE;
}

extern "C" CBASE_DLLEXPORT int GetNewDLLFunctions( NEW_DLL_FUNCTIONS* table, int* version )
{
	if ( !table || *version != NEW_DLL_FUNCTIONS_VERSION )
	{
		*version = NEW_DLL_FUNCTIONS_VERSION;
		return FALSE;
	}
	memcpy( table, &gNewFunctionTable, sizeof( gNewFunctionTable ) );
	return TRUE;
}

void EntvarsKeyvalue( entvars_t *pev, KeyValueData *pkvd );

extern "C" void PM_Move ( struct playermove_s *ppmove, int server );
extern "C" void PM_Init ( struct playermove_s *ppmove  );
extern "C" char PM_FindTextureType( char *name );

extern Vector VecBModelOrigin( entvars_t* pevBModel );
extern DLL_GLOBAL Vector		g_vecAttackDir;
extern DLL_GLOBAL int			g_iSkillLevel;

static DLL_FUNCTIONS gFunctionTable =
{
	GameDLLInit,				//pfnGameInit
	DispatchSpawn,				//pfnSpawn
	DispatchThink,				//pfnThink
	DispatchUse,				//pfnUse
	DispatchTouch,				//pfnTouch
	DispatchBlocked,			//pfnBlocked
	DispatchKeyValue,			//pfnKeyValue
	DispatchSave,				//pfnSave
	DispatchRestore,			//pfnRestore
	DispatchObjectCollsionBox,	//pfnAbsBox

	SaveWriteFields,			//pfnSaveWriteFields
	SaveReadFields,				//pfnSaveReadFields

	SaveGlobalState,			//pfnSaveGlobalState
	RestoreGlobalState,			//pfnRestoreGlobalState
	ResetGlobalState,			//pfnResetGlobalState

	ClientConnect,				//pfnClientConnect
	ClientDisconnect,			//pfnClientDisconnect
	ClientKill,					//pfnClientKill
	ClientPutInServer,			//pfnClientPutInServer
	ClientCommand,				//pfnClientCommand
	ClientUserInfoChanged,		//pfnClientUserInfoChanged
	ServerActivate,				//pfnServerActivate
	ServerDeactivate,			//pfnServerDeactivate

	PlayerPreThink,				//pfnPlayerPreThink
	PlayerPostThink,			//pfnPlayerPostThink

	StartFrame,					//pfnStartFrame
	ParmsNewLevel,				//pfnParmsNewLevel
	ParmsChangeLevel,			//pfnParmsChangeLevel

	GetGameDescription,         //pfnGetGameDescription    Returns string describing current .dll game.
	PlayerCustomization,        //pfnPlayerCustomization   Notifies .dll of new customization for player.

	SpectatorConnect,			//pfnSpectatorConnect      Called when spectator joins server
	SpectatorDisconnect,        //pfnSpectatorDisconnect   Called when spectator leaves the server
	SpectatorThink,				//pfnSpectatorThink        Called when spectator sends a command packet (usercmd_t)

	Sys_Error,					//pfnSys_Error				Called when engine has encountered an error

	PM_Move,					//pfnPM_Move
	PM_Init,					//pfnPM_Init				Server version of player movement initialization
	PM_FindTextureType,			//pfnPM_FindTextureType

	SetupVisibility,			//pfnSetupVisibility        Set up PVS and PAS for networking for this client
	UpdateClientData,			//pfnUpdateClientData       Set up data sent only to specific client
	AddToFullPack,				//pfnAddToFullPack
	CreateBaseline,				//pfnCreateBaseline			Tweak entity baseline for network encoding, allows setup of player baselines, too.
	RegisterEncoders,			//pfnRegisterEncoders		Callbacks for network encoding
	GetWeaponData,				//pfnGetWeaponData
	CmdStart,					//pfnCmdStart
	CmdEnd,						//pfnCmdEnd
	ConnectionlessPacket,		//pfnConnectionlessPacket
	GetHullBounds,				//pfnGetHullBounds
	CreateInstancedBaselines,   //pfnCreateInstancedBaselines
	InconsistentFile,			//pfnInconsistentFile
	AllowLagCompensation,		//pfnAllowLagCompensation
};

void SetObjectCollisionBox( entvars_t *pev );

extern "C" {

	int GetEntityAPI( DLL_FUNCTIONS *pFunctionTable, int interfaceVersion )
{
	if ( !pFunctionTable || interfaceVersion != INTERFACE_VERSION )
	{
		return FALSE;
	}

	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
	return TRUE;
}

int GetEntityAPI2( DLL_FUNCTIONS *pFunctionTable, int *interfaceVersion )
{
	if ( !pFunctionTable || *interfaceVersion != INTERFACE_VERSION )
	{
		// Tell engine what version we had, so it can figure out who is out of date.
		*interfaceVersion = INTERFACE_VERSION;
		return FALSE;
	}

	memcpy( pFunctionTable, &gFunctionTable, sizeof( DLL_FUNCTIONS ) );
	return TRUE;
}

}


int DispatchSpawn( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if (pEntity)
	{
		// Initialize these or entities who don't link to the world won't have anything in here
		pEntity->pev->absmin = pEntity->pev->origin - Vector(1,1,1);
		pEntity->pev->absmax = pEntity->pev->origin + Vector(1,1,1);

		pEntity->Spawn();

		// Try to get the pointer again, in case the spawn function deleted the entity.
		// UNDONE: Spawn() should really return a code to ask that the entity be deleted, but
		// that would touch too much code for me to do that right now.
		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

		if ( pEntity )
		{
			if ( g_pGameRules && !g_pGameRules->IsAllowedToSpawn( pEntity ) )
				return -1;	// return that this entity should be deleted
			if ( pEntity->pev->flags & FL_KILLME )
				return -1;
		}


		// Handle global stuff here
		if ( pEntity && pEntity->pev->globalname )
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if ( pGlobal )
			{
				// Already dead? delete
				if ( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if ( !FStrEq( STRING(gpGlobals->mapname), pGlobal->levelName ) )
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				// In this level & not dead, continue on as normal
			}
			else
			{
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
//				ALERT( at_console, "Added global entity %s (%s)\n", STRING(pEntity->pev->classname), STRING(pEntity->pev->globalname) );
			}
		}

	}

	return 0;
}

void DispatchKeyValue( edict_t *pentKeyvalue, KeyValueData *pkvd )
{
	if ( !pkvd || !pentKeyvalue )
		return;

	EntvarsKeyvalue( VARS(pentKeyvalue), pkvd );

	// If the key was an entity variable, or there's no class set yet, don't look for the object, it may
	// not exist yet.
	if ( pkvd->fHandled || pkvd->szClassName == NULL )
		return;

	// Get the actualy entity object
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentKeyvalue);

	if ( !pEntity )
		return;

	pEntity->KeyValue( pkvd );
}


// HACKHACK -- this is a hack to keep the node graph entity from "touching" things (like triggers)
// while it builds the graph
BOOL gTouchDisabled = FALSE;
void DispatchTouch( edict_t *pentTouched, edict_t *pentOther )
{
	if ( gTouchDisabled )
		return;

	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentTouched);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if ( pEntity && pOther && ! ((pEntity->pev->flags | pOther->pev->flags) & FL_KILLME) )
		pEntity->Touch( pOther );
}


void DispatchUse( edict_t *pentUsed, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pentUsed);
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE(pentOther);

	if (pEntity && !(pEntity->pev->flags & FL_KILLME) )
		pEntity->Use( pOther, pOther, USE_TOGGLE, 0 );
}

void DispatchThink( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	if (pEntity)
	{
		if ( FBitSet( pEntity->pev->flags, FL_DORMANT ) )
			ALERT( at_error, "Dormant entity %s is thinking!!\n", STRING(pEntity->pev->classname) );

		pEntity->Think();
	}
}

void DispatchBlocked( edict_t *pentBlocked, edict_t *pentOther )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE( pentBlocked );
	CBaseEntity *pOther = (CBaseEntity *)GET_PRIVATE( pentOther );

	if (pEntity)
		pEntity->Blocked( pOther );
}

void DispatchSave( edict_t *pent, SAVERESTOREDATA *pSaveData )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if ( pEntity && pSaveData )
	{
		ENTITYTABLE *pTable = &pSaveData->pTable[ pSaveData->currentIndex ];

		if ( pTable->pent != pent )
			ALERT( at_error, "ENTITY TABLE OR INDEX IS WRONG!!!!\n" );

		if ( pEntity->ObjectCaps() & FCAP_DONT_SAVE )
			return;

		// These don't use ltime & nextthink as times really, but we'll fudge around it.
		if ( pEntity->pev->movetype == MOVETYPE_PUSH )
		{
			float delta = pEntity->pev->nextthink - pEntity->pev->ltime;
			pEntity->pev->ltime = gpGlobals->time;
			pEntity->pev->nextthink = pEntity->pev->ltime + delta;
		}

		pTable->location = pSaveData->size;		// Remember entity position for file I/O
		pTable->classname = pEntity->pev->classname;	// Remember entity class for respawn

		CSave saveHelper( pSaveData );
		pEntity->Save( saveHelper );

		pTable->size = pSaveData->size - pTable->location;	// Size of entity block is data size written to block
	}
}


// Find the matching global entity.  Spit out an error if the designer made entities of
// different classes with the same global name
CBaseEntity *FindGlobalEntity( string_t classname, string_t globalname )
{
	edict_t *pent = FIND_ENTITY_BY_STRING( NULL, "globalname", STRING(globalname) );
	CBaseEntity *pReturn = CBaseEntity::Instance( pent );
	if ( pReturn )
	{
		if ( !FClassnameIs( pReturn->pev, STRING(classname) ) )
		{
			ALERT( at_console, "Global entity found %s, wrong class %s\n", STRING(globalname), STRING(pReturn->pev->classname) );
			pReturn = NULL;
		}
	}

	return pReturn;
}


int DispatchRestore( edict_t *pent, SAVERESTOREDATA *pSaveData, int globalEntity )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);

	if ( pEntity && pSaveData )
	{
		entvars_t tmpVars;
		Vector oldOffset;

		CRestore restoreHelper( pSaveData );
		if ( globalEntity )
		{
			CRestore tmpRestore( pSaveData );
			tmpRestore.PrecacheMode( 0 );
			tmpRestore.ReadEntVars( "ENTVARS", &tmpVars );

			// HACKHACK - reset the save pointers, we're going to restore for real this time
			pSaveData->size = pSaveData->pTable[pSaveData->currentIndex].location;
			pSaveData->pCurrentData = pSaveData->pBaseData + pSaveData->size;
			// -------------------


			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( tmpVars.globalname );

			// Don't overlay any instance of the global that isn't the latest
			// pSaveData->szCurrentMapName is the level this entity is coming from
			// pGlobla->levelName is the last level the global entity was active in.
			// If they aren't the same, then this global update is out of date.
			if ( !FStrEq( pSaveData->szCurrentMapName, pGlobal->levelName ) )
				return 0;

			// Compute the new global offset
			oldOffset = pSaveData->vecLandmarkOffset;
			CBaseEntity *pNewEntity = FindGlobalEntity( tmpVars.classname, tmpVars.globalname );
			if ( pNewEntity )
			{
//				ALERT( at_console, "Overlay %s with %s\n", STRING(pNewEntity->pev->classname), STRING(tmpVars.classname) );
				// Tell the restore code we're overlaying a global entity from another level
				restoreHelper.SetGlobalMode( 1 );	// Don't overwrite global fields
				pSaveData->vecLandmarkOffset = (pSaveData->vecLandmarkOffset - pNewEntity->pev->mins) + tmpVars.mins;
				pEntity = pNewEntity;// we're going to restore this data OVER the old entity
				pent = ENT( pEntity->pev );
				// Update the global table to say that the global definition of this entity should come from this level
				gGlobalState.EntityUpdate( pEntity->pev->globalname, gpGlobals->mapname );
			}
			else
			{
				// This entity will be freed automatically by the engine.  If we don't do a restore on a matching entity (below)
				// or call EntityUpdate() to move it to this level, we haven't changed global state at all.
				return 0;
			}

		}

		if ( pEntity->ObjectCaps() & FCAP_MUST_SPAWN )
		{
			pEntity->Restore( restoreHelper );
			pEntity->Spawn();
		}
		else
		{
			pEntity->Restore( restoreHelper );
			pEntity->Precache( );
		}

		// Again, could be deleted, get the pointer again.
		pEntity = (CBaseEntity *)GET_PRIVATE(pent);

#if 0
		if ( pEntity && pEntity->pev->globalname && globalEntity )
		{
			ALERT( at_console, "Global %s is %s\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->model) );
		}
#endif

		// Is this an overriding global entity (coming over the transition), or one restoring in a level
		if ( globalEntity )
		{
//			ALERT( at_console, "After: %f %f %f %s\n", pEntity->pev->origin.x, pEntity->pev->origin.y, pEntity->pev->origin.z, STRING(pEntity->pev->model) );
			pSaveData->vecLandmarkOffset = oldOffset;
			if ( pEntity )
			{
				UTIL_SetOrigin( pEntity, pEntity->pev->origin );
				pEntity->OverrideReset();
			}
		}
		else if ( pEntity && pEntity->pev->globalname )
		{
			const globalentity_t *pGlobal = gGlobalState.EntityFromTable( pEntity->pev->globalname );
			if ( pGlobal )
			{
				// Already dead? delete
				if ( pGlobal->state == GLOBAL_DEAD )
					return -1;
				else if ( !FStrEq( STRING(gpGlobals->mapname), pGlobal->levelName ) )
				{
					pEntity->MakeDormant();	// Hasn't been moved to this level yet, wait but stay alive
				}
				// In this level & not dead, continue on as normal
			}
			else
			{
				ALERT( at_error, "Global Entity %s (%s) not in table!!!\n", STRING(pEntity->pev->globalname), STRING(pEntity->pev->classname) );
				// Spawned entities default to 'On'
				gGlobalState.EntityAdd( pEntity->pev->globalname, gpGlobals->mapname, GLOBAL_ON );
			}
		}
	}
	return 0;
}


void DispatchObjectCollsionBox( edict_t *pent )
{
	CBaseEntity *pEntity = (CBaseEntity *)GET_PRIVATE(pent);
	if (pEntity)
	{
		pEntity->SetObjectCollisionBox();
	}
	else
		SetObjectCollisionBox( &pent->v );
}


void SaveWriteFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CSave saveHelper( pSaveData );
	saveHelper.WriteFields( pname, pBaseData, pFields, fieldCount );
}


void SaveReadFields( SAVERESTOREDATA *pSaveData, const char *pname, void *pBaseData, TYPEDESCRIPTION *pFields, int fieldCount )
{
	CRestore restoreHelper( pSaveData );
	restoreHelper.ReadFields( pname, pBaseData, pFields, fieldCount );
}


edict_t * EHANDLE::Get( void )
{
	if (m_pent)
	{
		if (m_pent->serialnumber == m_serialnumber)
			return m_pent;
		else
			return NULL;
	}
	return NULL;
};

edict_t * EHANDLE::Set( edict_t *pent )
{
	m_pent = pent;
	if (pent)
		m_serialnumber = m_pent->serialnumber;
	return pent;
};


EHANDLE :: operator CBaseEntity *()
{
	return (CBaseEntity *)GET_PRIVATE( Get( ) );
};


CBaseEntity * EHANDLE :: operator = (CBaseEntity *pEntity)
{
	if (pEntity)
	{
		m_pent = ENT( pEntity->pev );
		if (m_pent)
			m_serialnumber = m_pent->serialnumber;
	}
	else
	{
		m_pent = NULL;
		m_serialnumber = 0;
	}
	return pEntity;
}

EHANDLE :: operator int ()
{
	return Get() != NULL;
}

CBaseEntity * EHANDLE :: operator -> ()
{
	return (CBaseEntity *)GET_PRIVATE( Get( ) );
}


// give health
int CBaseEntity :: TakeHealth( float flHealth, int bitsDamageType )
{
	if (!pev->takedamage)
		return 0;

// heal
	if ( pev->health >= pev->max_health )
		return 0;

	pev->health += flHealth;

	if (pev->health > pev->max_health)
		pev->health = pev->max_health;

	return 1;
}

// inflict damage on this entity.  bitsDamageType indicates type of damage inflicted, ie: DMG_CRUSH

int CBaseEntity :: TakeDamage( entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType )
{
	Vector			vecTemp;

	if (!pev->takedamage)
		return 0;

	// UNDONE: some entity types may be immune or resistant to some bitsDamageType

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin).
	if ( pevAttacker == pevInflictor )
	{
		vecTemp = pevInflictor->origin - ( VecBModelOrigin(pev) );
	}
	else
	// an actual missile was involved.
	{
		vecTemp = pevInflictor->origin - ( VecBModelOrigin(pev) );
	}

// this global is still used for glass and other non-monster killables, along with decals.
	g_vecAttackDir = vecTemp.Normalize();

// save damage based on the target's armor level

// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
	{
		Killed( pevAttacker, GIB_NORMAL );
		return 0;
	}

	return 1;
}


void CBaseEntity :: Killed( entvars_t *pevAttacker, int iGib )
{
	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_DEAD;
	UTIL_Remove( this );
}


CBaseEntity *CBaseEntity::GetNextTarget( void )
{
	if ( FStringNull( pev->target ) )
		return NULL;
	return UTIL_FindEntityByTargetname( NULL, STRING( pev->target ) );
}

// Global Savedata for Delay
TYPEDESCRIPTION	CBaseEntity::m_SaveData[] =
{
	DEFINE_FIELD( CBaseEntity, m_pGoalEnt, FIELD_CLASSPTR ),

	DEFINE_FIELD( CBaseEntity, m_pfnThink, FIELD_FUNCTION ),		// UNDONE: Build table of these!!!
	DEFINE_FIELD( CBaseEntity, m_pfnTouch, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnUse, FIELD_FUNCTION ),
	DEFINE_FIELD( CBaseEntity, m_pfnBlocked, FIELD_FUNCTION ),
};


int CBaseEntity::Save( CSave &save )
{
	if ( save.WriteEntVars( "ENTVARS", pev ) )
		return save.WriteFields( "BASE", this, m_SaveData, ARRAYSIZE(m_SaveData) );

	return 0;
}

int CBaseEntity::Restore( CRestore &restore )
{
	int status;

	status = restore.ReadEntVars( "ENTVARS", pev );
	if ( status )
		status = restore.ReadFields( "BASE", this, m_SaveData, ARRAYSIZE(m_SaveData) );

    if ( pev->modelindex != 0 && !FStringNull(pev->model) )
	{
		Vector mins, maxs;
		mins = pev->mins;	// Set model is about to destroy these
		maxs = pev->maxs;


		PRECACHE_MODEL( (char *)STRING(pev->model) );
		SET_MODEL(ENT(pev), STRING(pev->model));
		UTIL_SetSize(pev, mins, maxs);	// Reset them
	}

	return status;
}


// Initialize absmin & absmax to the appropriate box
void SetObjectCollisionBox( entvars_t *pev )
{
	if ( (pev->solid == SOLID_BSP) &&
		 (pev->angles.x || pev->angles.y|| pev->angles.z) )
	{	// expand for rotation
		float		max, v;
		int			i;

		max = 0;
		for (i=0 ; i<3 ; i++)
		{
			v = fabs( ((float *)pev->mins)[i]);
			if (v > max)
				max = v;
			v = fabs( ((float *)pev->maxs)[i]);
			if (v > max)
				max = v;
		}
		for (i=0 ; i<3 ; i++)
		{
			((float *)pev->absmin)[i] = ((float *)pev->origin)[i] - max;
			((float *)pev->absmax)[i] = ((float *)pev->origin)[i] + max;
		}
	}
	else
	{
		pev->absmin = pev->origin + pev->mins;
		pev->absmax = pev->origin + pev->maxs;
	}

	pev->absmin.x -= 1;
	pev->absmin.y -= 1;
	pev->absmin.z -= 1;
	pev->absmax.x += 1;
	pev->absmax.y += 1;
	pev->absmax.z += 1;
}


void CBaseEntity::SetObjectCollisionBox( void )
{
	::SetObjectCollisionBox( pev );
}


int	CBaseEntity :: Intersects( CBaseEntity *pOther )
{
	if ( pOther->pev->absmin.x > pev->absmax.x ||
		 pOther->pev->absmin.y > pev->absmax.y ||
		 pOther->pev->absmin.z > pev->absmax.z ||
		 pOther->pev->absmax.x < pev->absmin.x ||
		 pOther->pev->absmax.y < pev->absmin.y ||
		 pOther->pev->absmax.z < pev->absmin.z )
		 return 0;
	return 1;
}

void CBaseEntity :: MakeDormant( void )
{
	SetBits( pev->flags, FL_DORMANT );

	// Don't touch
	pev->solid = SOLID_NOT;
	// Don't move
	pev->movetype = MOVETYPE_NONE;
	// Don't draw
	SetBits( pev->effects, EF_NODRAW );
	// Don't think
	pev->nextthink = 0;
	// Relink
	UTIL_SetOrigin( this, pev->origin );
}

int CBaseEntity :: IsDormant( void )
{
	return FBitSet( pev->flags, FL_DORMANT );
}

BOOL CBaseEntity :: IsInWorld( void )
{
	// position
	if (pev->origin.x >= 4096) return FALSE;
	if (pev->origin.y >= 4096) return FALSE;
	if (pev->origin.z >= 4096) return FALSE;
	if (pev->origin.x <= -4096) return FALSE;
	if (pev->origin.y <= -4096) return FALSE;
	if (pev->origin.z <= -4096) return FALSE;
	// speed
	if (pev->velocity.x >= 2000) return FALSE;
	if (pev->velocity.y >= 2000) return FALSE;
	if (pev->velocity.z >= 2000) return FALSE;
	if (pev->velocity.x <= -2000) return FALSE;
	if (pev->velocity.y <= -2000) return FALSE;
	if (pev->velocity.z <= -2000) return FALSE;

	return TRUE;
}

int CBaseEntity::ShouldToggle( USE_TYPE useType, BOOL currentState )
{
	if ( useType != USE_TOGGLE && useType != USE_SET )
	{
		if ( (currentState && useType == USE_ON) || (!currentState && useType == USE_OFF) )
			return 0;
	}
	return 1;
}

BOOL CBaseEntity::ShouldToggle( USE_TYPE useType )
{
	STATE state = GetState();
	if ( useType == USE_TOGGLE || useType == USE_SET )
		return TRUE;
	if ( state == STATE_TURN_ON || state == STATE_ON )
		return useType != USE_ON;
	if ( state == STATE_OFF || state == STATE_TURN_OFF )
		return useType != USE_OFF;
	return TRUE;
}


int	CBaseEntity :: DamageDecal( int bitsDamageType )
{
	if ( pev->rendermode == kRenderTransAlpha )
		return -1;

	if ( pev->rendermode != kRenderNormal )
		return DECAL_BPROOF1;

	return DECAL_GUNSHOT1 + RANDOM_LONG(0,4);
}



// NOTE: szName must be a pointer to constant memory, e.g. "monster_class" because the entity
// will keep a pointer to it after this call.
CBaseEntity * CBaseEntity::Create( char *szName, const Vector &vecOrigin, const Vector &vecAngles, edict_t *pentOwner )
{
	edict_t	*pent;
	CBaseEntity *pEntity;

	pent = CREATE_NAMED_ENTITY( MAKE_STRING( szName ));
	if ( FNullEnt( pent ) )
	{
		ALERT ( at_console, "NULL Ent in Create!\n" );
		return NULL;
	}
	pEntity = Instance( pent );
	pEntity->pev->owner = pentOwner;
	pEntity->pev->origin = vecOrigin;
	pEntity->pev->angles = vecAngles;
	DispatchSpawn( pEntity->edict() );
	return pEntity;
}

int CaseInsensitiveHash( const char* string, int bounds )
{
	unsigned int hash = 0;
	for ( ; *string; string++ )
	{
		char c = *string;
		if ( c >= 'A' && c <= 'Z' )
			c += 'a' - 'A';
		hash = hash * 33 + c;
	}
	return hash % (bounds + 1);
}

void EmptyEntityHashTable()
{
	for ( int i = 0; i < stringsHashTable.Count(); i++ )
	{
		hash_item_t* bucket = &stringsHashTable[i];
		bucket->pev = NULL;
		bucket->pevIndex = 0;
		bucket->lastHash = NULL;
		hash_item_t* item = bucket->next;
		bucket->next = NULL;
		while ( item )
		{
			hash_item_t* next = item->next;
			hashItemMemPool.Free( item );
			item = next;
		}
	}
}

void InitHashTable()
{
	stringsHashTable.AddMultipleToTail( 4176 );
	for ( int i = 0; i < stringsHashTable.Count(); i++ )
		stringsHashTable[i].next = NULL;
	EmptyEntityHashTable();
}

void AddEntityHashValue( entvars_t* pev, const char* value, hash_types_e fieldType )
{
	if ( stringsHashTable.Count() <= 0 )
		InitHashTable();
	if ( fieldType != CLASSNAME || !pev->classname )
		return;

	int hash = CaseInsensitiveHash( value, stringsHashTable.Count() );
	int index = ENTINDEX( ENT( pev ) );
	hash_item_t* item = &stringsHashTable[hash];
	while ( item->pev && strcmp( STRING( item->pev->classname ), STRING( pev->classname ) ) )
	{
		hash = (hash + 1) % stringsHashTable.Count();
		item = &stringsHashTable[hash];
	}
	if ( !item->pev )
	{
		item->pev = pev;
		item->lastHash = NULL;
		item->pevIndex = ENTINDEX( ENT( pev ) );
		return;
	}

	while ( item->next && item->pev != pev && item->pevIndex < index )
		item = item->next;
	if ( index < item->pevIndex )
	{
		entvars_t* old = item->pev;
		item->pev = pev;
		item->lastHash = NULL;
		item->pevIndex = index;
		pev = old;
		index = ENTINDEX( ENT( old ) );
	}
	if ( pev != item->pev )
	{
		hash_item_t* next = item->next;
		item->next = (hash_item_t*)hashItemMemPool.Alloc( sizeof( hash_item_t ) );
		if ( item->next )
		{
			item->next->pev = pev;
			item->next->lastHash = NULL;
			item->next->next = next;
			item->next->pevIndex = index;
		}
		else
			ALERT( at_error, "ERROR - AddEntityHashValue exhausted memory in hashItemMemPool\n" );
	}
}

void RemoveEntityHashValue( entvars_t* pev, const char* value, hash_types_e fieldType )
{
	int hash = CaseInsensitiveHash( value, stringsHashTable.Count() );
	ENTINDEX( ENT( pev ) );
	if ( fieldType != CLASSNAME )
		return;

	hash_item_t* bucket = &stringsHashTable[hash];
	while ( bucket->pev && strcmp( STRING( bucket->pev->classname ), STRING( pev->classname ) ) )
	{
		hash = (hash + 1) % stringsHashTable.Count();
		bucket = &stringsHashTable[hash];
	}
	if ( !bucket->pev )
		return;

	hash_item_t* item = bucket;
	hash_item_t* previous = bucket;
	while ( item->next && item->pev != pev )
	{
		previous = item;
		item = item->next;
	}
	if ( item->pev != pev )
		return;
	if ( item == previous )
	{
		if ( item->next )
		{
			item->pev = item->next->pev;
			item->pevIndex = item->next->pevIndex;
			item->next = item->next->next;
			item->lastHash = NULL;
		}
		else
		{
			item->pev = NULL;
			item->lastHash = NULL;
			item->pevIndex = 0;
		}
	}
	else
	{
		if ( bucket->lastHash == item )
			bucket->lastHash = NULL;
		previous->next = item->next;
		hashItemMemPool.Free( item );
	}
}

edict_t* CREATE_NAMED_ENTITY( string_t classname )
{
	edict_t* entity = g_engfuncs.pfnCreateNamedEntity( classname );
	if ( !FNullEnt( entity ) )
		AddEntityHashValue( &entity->v, STRING( classname ), CLASSNAME );
	return entity;
}

void REMOVE_ENTITY( edict_t* entity )
{
	g_engfuncs.pfnRemoveEntity( entity );
}

void OnFreeEntPrivateData( edict_t* entity )
{
	CBaseEntity* object = CBaseEntity::Instance( entity );
	if ( object )
	{
		object->CBaseEntity::UpdateOnRemove();
		RemoveEntityHashValue( object->pev, STRING( object->pev->classname ), CLASSNAME );
	}
}

static void CONSOLE_ECHO( char* message, ... )
{
	static char szStr[1024];
	va_list args;
	va_start( args, message );
	vsprintf( szStr, message, args );
	va_end( args );
	g_engfuncs.pfnServerPrint( szStr );
}

void printEntities()
{
	int count = 0;
	for ( int i = 0; i < stringsHashTable.Count(); i++ )
	{
		hash_item_t* item = &stringsHashTable[i];
		if ( item->pev )
			CONSOLE_ECHO( "%i - %i: Head: %s %i %p\n", i, count++, STRING( item->pev->classname ),
				ENTINDEX( ENT( item->pev ) ), item->pev );
		for ( item = item->next; item; item = item->next )
			if ( item->pev )
				CONSOLE_ECHO( "%i - %i: Child: %s %i %p\n", i, count++, STRING( item->pev->classname ),
					ENTINDEX( ENT( item->pev ) ), item->pev );
	}
}

void TestForEntityHashCollisions()
{
	EmptyEntityHashTable();
	for ( int i = 0; pszDodEntityNames[i]; i++ )
	{
		string_t classname = MAKE_STRING( pszDodEntityNames[i] );
		edict_t* entity = g_engfuncs.pfnCreateNamedEntity( classname );
		if ( entity )
		{
			if ( !FNullEnt( entity ) )
				AddEntityHashValue( &entity->v, STRING( classname ), CLASSNAME );
			UTIL_Remove( CBaseEntity::Instance( entity ) );
		}
	}
	EmptyEntityHashTable();
}

void loopPerformance()
{
	CPerformanceCounter counter;
	double start = counter.GetCurTime();
	const char* names[] = { "info_player_allies", "info_player_axis", "player", "dod_control_point" };
	for ( int i = 0; i < 100; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			CBaseEntity* entity = NULL;
			while ( (entity = UTIL_FindEntityByString_Old( entity, "classname", names[j] )) != NULL )
			{
			}
		}
	}
	CONSOLE_ECHO( " Time in old search loop %.4f\n", (counter.GetCurTime() - start) * 1000 );
	start = counter.GetCurTime();
	for ( int i = 0; i < 100; i++ )
	{
		for ( int j = 0; j < 4; j++ )
		{
			CBaseEntity* entity = NULL;
			while ( (entity = UTIL_FindEntityByString( entity, "classname", names[j] )) != NULL )
			{
			}
		}
	}
	CONSOLE_ECHO( " Time in new search loop %.4f\n", (counter.GetCurTime() - start) * 1000 );
}
