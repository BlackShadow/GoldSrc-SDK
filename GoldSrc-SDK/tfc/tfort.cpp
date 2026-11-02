#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"
#include "decals.h"

float number_of_teams;
int illegalclasses[5];
int civilianteams;
Vector rgbcolors[5];
team_color_t teamcolors[5][PC_LASTCLASS];
int g_iOrderedTeams[5];
int teamscores[5];
int teamfrags[5];
int teamlives[5];
int teammaxplayers[5];
float teamadvantage[5];
int teamallies[5];
string_t team_names[5];
BOOL CTF_Map;
BOOL birthday;
BOOL christmas;
float num_world_flames;
float clan_scores_dumped;
float cb_prematch_time;
float fOldPrematch;
float fOldCeaseFire;
float cb_ceasefire_time;
float last_id;
float spy_off;
float old_grens;
float flagem_checked;
float flNextEqualisationCalc;
BOOL cease_fire;
BOOL no_cease_fire_text;
BOOL initial_cease_fire;
BOOL last_cease_fire;
float autokick_kills;
float deathmsg;
string_t team_menu_string;
int toggleflags;
CBaseEntity *g_pLastSpawns[5];

char *sTeamSpawnNames[] =
{
	"", "ts1", "ts2", "ts3", "ts4"
};

char *sClassNames[] =
{
	"Observer", "Scout", "Sniper", "Soldier", "Demoman", "Medic",
	"HWGuy", "Pyro", "Spy", "Engineer", "RandomPC", "Civilian"
};

char *sNewClassModelFiles[] =
{
	NULL,
	"models/player/scout/scout.mdl",
	"models/player/sniper/sniper.mdl",
	"models/player/soldier/soldier.mdl",
	"models/player/demo/demo.mdl",
	"models/player/medic/medic.mdl",
	"models/player/hvyweapon/hvyweapon.mdl",
	"models/player/pyro/pyro.mdl",
	"models/player/spy/spy.mdl",
	"models/player/engineer/engineer.mdl",
	"models/player.mdl",
	"models/player/civilian/civilian.mdl"
};

char *sOldClassModelFiles[] =
{
	NULL,
	"models/player/scout/scout2.mdl",
	"models/player/sniper/sniper2.mdl",
	"models/player/soldier/soldier2.mdl",
	"models/player/demo/demo2.mdl",
	"models/player/medic/medic2.mdl",
	"models/player/hvyweapon/hvyweapon2.mdl",
	"models/player/pyro/pyro2.mdl",
	"models/player/spy/spy2.mdl",
	"models/player/engineer/engineer2.mdl",
	"models/player.mdl",
	"models/player/civilian/civilian.mdl"
};

char *sClassModels[] =
{
	"", "scout", "sniper", "soldier", "demo", "medic",
	"hvyweapon", "pyro", "spy", "engineer", "", "civilian"
};

char *sClassCfgs[] =
{
	"", "exec scout.cfg\n", "exec sniper.cfg\n", "exec soldier.cfg\n",
	"exec demoman.cfg\n", "exec medic.cfg\n", "exec hwguy.cfg\n", "exec pyro.cfg\n",
	"exec spy.cfg\n", "exec engineer.cfg\n", "", "exec civilian.cfg\n"
};

char *sGrenadeNames[] =
{
	"", "Normal", "Concussion", "Nail", "Mirv", "Napalm", "", "Gas", "EMP", "Caltrop", ""
};

class CTimer : public CBaseEntity
{
};

LINK_ENTITY_TO_CLASS( timer, CTimer );

extern short g_sModelIndexFireball;
extern short g_sModelIndexGrenade;
extern short g_sModelIndexMirvGrenade;
extern short g_sModelIndexNapalmGrenade;
extern short g_sModelIndexNailGrenade;
extern short g_sModelIndexPlayerFlame;
extern short g_sModelIndexNail;
extern short g_sModelIndexSaveMe;

CBaseEntity *CBaseEntity::FindTimer( int iTimerType )
{
	CBaseEntity *pTimer = NULL;
	while ( ( pTimer = UTIL_FindEntityByClassname( pTimer, "timer" ) ) != NULL )
	{
		if ( pTimer->pev->owner == edict() && ( !iTimerType || pTimer->timer_type == iTimerType ) )
			return pTimer;
	}
	return NULL;
}

CBaseEntity *CBaseEntity::CreateTimer( int iTimerType )
{
	CBaseEntity *pTimer = CBaseEntity::Create( "timer", g_vecZero, g_vecZero, edict() );
	pTimer->timer_type = iTimerType;
	return pTimer;
}

void CBaseEntity::Timer_HealthRot()
{
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner->pev->health > pOwner->pev->max_health )
	{
		pOwner->pev->health--;
		pev->nextthink = gpGlobals->time + 1;
		return;
	}
	pOwner->items &= ~IT_SUPERHEALTH;
	if ( FNullEnt( pev->enemy ) )
	{
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
	}
	else
		CBaseEntity::Instance( pev->enemy )->Respawn();
}

void CBaseEntity::Timer_Tranquilisation()
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Tranq_finished" );
	pOwner->tfstate &= ~TFSTATE_TRANQUILISED;
	pOwner->TeamFortress_SetSpeed();
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CBaseEntity::Timer_Regeneration()
{
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	pev->nextthink = gpGlobals->time + 3;
	if ( pOwner->pev->health < pOwner->pev->max_health )
	{
		pOwner->pev->health += 2;
		if ( pOwner->pev->health > pOwner->pev->max_health )
			pOwner->pev->health = pOwner->pev->max_health;
	}
}

void CBaseEntity::Timer_DetpackSet()
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	pOwner->tfstate &= ~TFSTATE_CANT_MOVE;
	pOwner->is_detpacking = FALSE;
	pOwner->TeamFortress_SetSpeed();
	pOwner->m_pActiveItem->Deploy();
	CBaseEntity *pDetpack = CBaseEntity::Create( "detpack", pOwner->pev->origin, g_vecZero, pOwner->edict() );
	pDetpack->pev->health = gpGlobals->time + pev->health;
	ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Detpack_finishset" );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Detpack_Set\"\n",
		STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ),
		pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR" );
	dremove( this );
}

void CBaseEntity::Timer_DetpackDisarm()
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	CBaseEntity *pDetpack = CBaseEntity::Instance( pev->enemy );
	CBaseEntity *pDetpackOwner = CBaseEntity::Instance( pDetpack->pev->owner );
	if ( !pOwner->IsAlive() || pDetpack->weaponmode == 0 )
	{
		dremove( this );
		return;
	}
	if ( birthday )
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Detpack_disarm_bday",
			STRING( pOwner->pev->netname ), STRING( pDetpackOwner->pev->netname ) );
	else
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Detpack_disarm",
			STRING( pDetpackOwner->pev->netname ), STRING( pOwner->pev->netname ) );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Detpack_Disarmed\" against \"%s<%i><%s><%s>\"\n",
		STRING( pDetpackOwner->pev->netname ), GETPLAYERUSERID( pDetpackOwner->edict() ),
		GETPLAYERAUTHID( pDetpackOwner->edict() ),
		pDetpackOwner->team_no ? GetTeamName( pDetpackOwner->team_no ) : "SPECTATOR",
		STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ),
		pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR" );
	pOwner->tfstate &= ~TFSTATE_CANT_MOVE;
	pOwner->TeamFortress_SetSpeed();
	pOwner->TF_AddFrags( 1 );
	dremove( pDetpack );
	dremove( this );
}

void CBaseEntity::Timer_CheckBuildDistance()
{
	if ( FNullEnt( pev->owner ) || FNullEnt( pev->enemy ) )
	{
		UTIL_Remove( this );
		return;
	}
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	CBaseEntity *pBuilding = CBaseEntity::Instance( pev->enemy );
	if ( !pOwner->building || pOwner->building != pBuilding )
	{
		UTIL_Remove( this );
		return;
	}
	if ( ( pBuilding->pev->origin - pOwner->pev->origin ).Length() > 64 )
	{
		pOwner->ResetMenu();
		pOwner->building = NULL;
		UTIL_Remove( this );
		return;
	}
	pev->nextthink = gpGlobals->time + 0.3;
}

void CBaseEntity::Timer_FinishedBuilding()
{
	if ( FNullEnt( pev->owner ) )
	{
		UTIL_Remove( this );
		return;
	}
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	EMIT_SOUND_DYN( pOwner->edict(), CHAN_STATIC, "weapons/building.wav", 0, 0, SND_STOP, PITCH_NORM );
	if ( pOwner->is_building )
	{
		pOwner->tfstate &= ~TFSTATE_CANT_MOVE;
		pOwner->is_building = FALSE;
		pOwner->TeamFortress_SetSpeed();
		if ( pOwner->m_pActiveItem )
			pOwner->m_pActiveItem->Deploy();
		if ( pOwner->building )
		{
			pOwner->building->Finished();
			pOwner->building = NULL;
		}
	}
	dremove( this );
}

void CBaseEntity::Timer_SpyUndercoverThink()
{
	CBasePlayer *pOwner = (CBasePlayer *)CBaseEntity::Instance( pev->owner );
	if ( pOwner->is_undercover != 2 )
	{
		dremove( this );
		return;
	}
	pOwner->immune_to_check = gpGlobals->time + 10;
	if ( pev->skin )
	{
		ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Disguise_asclass", UTIL_VarArgs( "#%s", sClassNames[pev->skin] ) );
		pOwner->undercover_skin = pev->skin;
	}
	if ( team_no )
	{
		ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Disguise_asteam", UTIL_dtos1( team_no ) );
		pOwner->undercover_team = team_no;
	}
	ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Disguise_now" );
	pOwner->is_undercover = TRUE;
	pOwner->TeamFortress_SpyCalcName();
	pOwner->Spy_DisguiseExternalWeaponModel();
	pOwner->TeamFortress_SetSkin();
	dremove( this );
}

void CBaseEntity::Timer_CeaseFireThink()
{
	if ( gpGlobals->time > cb_ceasefire_time && weapon == 1 && cease_fire )
	{
		cease_fire = FALSE;
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_resumefire" );
		UTIL_LogPrintf( "World triggered \"Resume_Fire\"\n" );
		CBasePlayer *pPlayer = NULL;
		while ( ( pPlayer = (CBasePlayer *)UTIL_FindEntityByClassname( pPlayer, "player" ) ) != NULL )
		{
			if ( FNullEnt( pPlayer->edict() ) )
				break;
			if ( !pPlayer->pev )
				continue;
			pPlayer->pev->iuser4 = 0;
			pPlayer->tfstate &= ~TFSTATE_CANT_MOVE;
			pPlayer->immune_to_check = gpGlobals->time + 10;
			pPlayer->TeamFortress_SetSpeed();
		}
	}
	if ( cease_fire )
	{
		UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#Game_ceasefire" );
		UTIL_LogPrintf( "World triggered \"Cease_Fire\"\n" );
		pev->nextthink = gpGlobals->time + 5;
	}
	else
		dremove( this );
}

void CBaseEntity::Timer_AutokickThink()
{
	CBaseEntity::Instance( pev->owner )->teamkills = 0;
	dremove( this );
}

void CBasePlayer::ExplodeOldPipebomb( BOOL bAll, BOOL bForceDetonation )
{
	CTFGrenade *pPipebomb = NULL;
	CTFGrenade *pOldest = NULL;
	while ( ( pPipebomb = (CTFGrenade *)UTIL_FindEntityByClassname( pPipebomb, "tf_gl_pipebomb" ) ) != NULL )
	{
		if ( CBaseEntity::Instance( pPipebomb->pev->owner ) != this )
			continue;
		if ( bAll )
		{
			if ( bForceDetonation || pPipebomb->m_flCreationTime + 0.6 <= gpGlobals->time )
				pPipebomb->pev->nextthink = gpGlobals->time;
		}
		else if ( !pOldest || pOldest->m_flCreationTime > pPipebomb->m_flCreationTime )
			pOldest = pPipebomb;
	}
	if ( !bAll && pOldest )
		pOldest->pev->nextthink = gpGlobals->time;
}

void CBasePlayer::RemovePipebombs()
{
	CBaseEntity *pPipebomb = NULL;
	while ( ( pPipebomb = UTIL_FindEntityByClassname( pPipebomb, "tf_gl_pipebomb" ) ) != NULL )
	{
		if ( CBaseEntity::Instance( pPipebomb->pev->owner ) == this )
		{
			m_iPipebombCount--;
			pPipebomb->pev->flags |= FL_KILLME;
		}
	}
}

void CBasePlayer::RemoveOldAmmobox()
{
	CWeaponBox *pBox = NULL;
	CWeaponBox *pOldest = NULL;
	while ( ( pBox = (CWeaponBox *)UTIL_FindEntityByClassname( pBox, "tf_ammobox" ) ) != NULL )
	{
		if ( CBaseEntity::Instance( pBox->pev->enemy ) == this &&
			( !pOldest || pOldest->m_flCreationTime > pBox->m_flCreationTime ) )
			pOldest = pBox;
	}
	if ( pOldest )
		pOldest->pev->nextthink = gpGlobals->time;
}

BOOL CBaseEntity::IsAlly( CBaseEntity *pOther )
{
	return pOther->team_no && team_no &&
		( ( teamallies[team_no] & ( 1 << ( pOther->team_no - 1 ) ) ) || pOther->team_no == team_no );
}

BOOL CBaseEntity::IsAlly( int iTeamNo )
{
	return iTeamNo && team_no && ( ( teamallies[team_no] & ( 1 << ( iTeamNo - 1 ) ) ) || iTeamNo == team_no );
}

BOOL CBaseEntity::IsTeammate( CBaseEntity *pOther )
{
	return pOther->team_no == team_no || team_no == 0;
}

CBaseEntity *CBaseEntity::FindTeamSpawnPoint()
{
	int blocked = 0;
	int removed = 0;
	CTFSpawn *pSpawn = (CTFSpawn *)g_pLastSpawns[team_no];
	for ( ;; )
	{
		pSpawn = (CTFSpawn *)UTIL_FindEntityByClassname( pSpawn, "info_player_teamspawn" );
		if ( !pSpawn )
		{
			pSpawn = (CTFSpawn *)UTIL_FindEntityByClassname( NULL, "info_player_teamspawn" );
			if ( !pSpawn )
				return NULL;
		}
		if ( !pSpawn->CheckTeam( team_no ) )
			continue;
		if ( removed > 127 && blocked == 0 )
			return NULL;
		if ( pSpawn->goal_state == TFGS_REMOVED )
		{
			removed++;
			continue;
		}
		if ( !IsSpawnPointValid( pSpawn->pev->origin ) && blocked <= 31 )
		{
			blocked++;
			continue;
		}
		if ( cb_prematch_time > gpGlobals->time )
			return pSpawn;
		if ( ActivationSucceeded( pSpawn, (CBasePlayer *)this, NULL ) )
		{
			g_pLastSpawns[team_no] = pSpawn;
			return pSpawn;
		}
	}
}

void CBasePlayer::TeamFortress_SaveMe()
{
	if ( gpGlobals->time > last_saveme_sound )
	{
		EMIT_SOUND_DYN( edict(), CHAN_WEAPON, RANDOM_FLOAT( 0, 1 ) < 0.8 ? "speech/saveme1.wav" : "speech/saveme2.wav",
			VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		last_saveme_sound = gpGlobals->time + 4;
	}
	CBaseEntity *pPlayer = NULL;
	while ( ( pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" ) ) != NULL )
	{
		if ( FNullEnt( pPlayer->edict() ) )
			break;
		if ( !pPlayer->pev )
			continue;
		int playerclass = pPlayer->pev->playerclass;
		if ( playerclass == PC_SPY || playerclass == PC_ENGINEER || playerclass == PC_MEDIC )
		{
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pPlayer->edict() );
				WRITE_BYTE( TE_PLAYERATTACHMENT );
				WRITE_BYTE( entindex() );
				WRITE_COORD( 50 );
				WRITE_SHORT( g_sModelIndexSaveMe );
				WRITE_SHORT( 40 );
			MESSAGE_END();
		}
	}
}

void CBasePlayer::TeamFortress_DisplayDetectionItems()
{
	CBaseEntity *pDetect = UTIL_FindEntityByClassname( NULL, "info_tfdetect" );
	if ( !pDetect )
		return;
	for ( int i = 0; i < 4; i++ )
	{
		if ( !pDetect->display_item_status[i] )
			continue;
		CBaseEntity *pItem = Finditem( pDetect->display_item_status[i] );
		if ( pItem )
			DisplayItemStatus( pDetect, this, pItem );
		else
			ClientPrint( pev, HUD_PRINTTALK, "#Item_missing" );
	}
}

void CBasePlayer::DisplayLocalItemStatus( CBaseEntity *pGoal )
{
	for ( int i = 0; i < 4; i++ )
	{
		if ( !pGoal->display_item_status[i] )
			continue;
		CBaseEntity *pItem = Finditem( pGoal->display_item_status[i] );
		if ( pItem )
			DisplayItemStatus( pGoal, this, pItem );
		else
			ClientPrint( pev, HUD_PRINTTALK, "#Item_missing" );
	}
}


void CBaseEntity::Timer_Infection()
{
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	CBaseEntity *pInfector = CBaseEntity::Instance( pev->enemy );
	BOOL bFriendly = ( (int)gpGlobals->teamplay & TEAMPLAY_TEAMDAMAGE ) &&
		pInfector->team_no == pOwner->team_no && pOwner->team_no != 0;
	if ( pOwner->invincible_finished > gpGlobals->time || !( pOwner->tfstate & TFSTATE_INFECTED ) ||
		pOwner->pev->playerclass == PC_MEDIC || bFriendly )
	{
		pOwner->tfstate &= ~TFSTATE_INFECTED;
		SetThink( &CBaseEntity::SUB_Remove );
		pev->nextthink = gpGlobals->time;
		return;
	}

	Vector vecSrc = pOwner->pev->origin;
	CBaseEntity *pPlayer = NULL;
	while ( ( pPlayer = UTIL_FindEntityInSphere( pPlayer, vecSrc, 128 ) ) != NULL )
	{
		if ( pPlayer->Classify() != CLASS_PLAYER || !pPlayer->IsAlive() || !pPlayer->pev->playerclass ||
			( pPlayer->tfstate & TFSTATE_INFECTED ) || pPlayer->pev->playerclass == PC_MEDIC )
			continue;
		if ( ( (int)gpGlobals->teamplay & TEAMPLAY_TEAMDAMAGE ) && pPlayer->IsAlly( pInfector ) )
			continue;
		Vector vecTarget = pPlayer->BodyTarget( vecSrc );
		TraceResult tr;
		UTIL_TraceLine( vecSrc, vecTarget, dont_ignore_monsters, edict(), &tr );
		if ( tr.flFraction != 1 && tr.pHit != pPlayer->edict() )
			continue;
		EMIT_SOUND_DYN( pPlayer->edict(), CHAN_VOICE, "player/death2.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		pPlayer->tfstate |= TFSTATE_INFECTED;
		CBaseEntity *pTimer = pPlayer->CreateTimer( TF_TIMER_INFECTION );
		pTimer->pev->nextthink = gpGlobals->time + 2;
		pTimer->SetThink( &CBaseEntity::Timer_Infection );
		pTimer->pev->enemy = pev->enemy;
		pPlayer->infection_team_no = pOwner->team_no;
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Medic_infected", STRING( pOwner->pev->netname ) );
		ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Medic_infect", STRING( pPlayer->pev->netname ) );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Passed_On_Infection\" against \"%s<%i><%s><%s>\"\n",
			STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ),
			pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR",
			STRING( pPlayer->pev->netname ), GETPLAYERUSERID( pPlayer->edict() ), GETPLAYERAUTHID( pPlayer->edict() ),
			pPlayer->team_no ? GetTeamName( pPlayer->team_no ) : "SPECTATOR" );
	}
	pev->nextthink = gpGlobals->time + 3;
	SpawnBlood( pOwner->pev->origin, pOwner->BloodColor(), 8 );
	if ( pOwner->IsAlive() )
	{
		pOwner->TakeDamage( pev, pInfector->pev, 8, DMG_IGNOREARMOR | DMG_NERVEGAS );
		if ( !pOwner->IsAlive() )
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" killed \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING( pInfector->pev->netname ), GETPLAYERUSERID( pInfector->edict() ), GETPLAYERAUTHID( pInfector->edict() ),
				pInfector->team_no ? GetTeamName( pInfector->team_no ) : "SPECTATOR",
				STRING( pOwner->pev->netname ), GETPLAYERUSERID( pOwner->edict() ), GETPLAYERAUTHID( pOwner->edict() ),
				pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR", "infection" );
	}
}

void CBasePlayer::TeamFortress_Discard()
{
	int iNailsDropped = 0;
	int iRocketsDropped = 0;
	int iCellsDropped = 0;
	int iShellsDropped = 0;
	switch ( pev->playerclass )
	{
	case PC_SCOUT:
	case PC_SNIPER:
	case PC_MEDIC:
	case PC_SPY:
		iRocketsDropped = ammo_rockets;
		iCellsDropped = ammo_cells;
		ammo_rockets = 0;
		ammo_cells = 0;
		break;
	case PC_SOLDIER:
	case PC_DEMOMAN:
		iNailsDropped = ammo_nails;
		iCellsDropped = ammo_cells;
		ammo_nails = 0;
		ammo_cells = 0;
		break;
	case PC_HVYWEAP:
		iNailsDropped = ammo_nails;
		iRocketsDropped = ammo_rockets;
		iCellsDropped = ammo_cells;
		ammo_nails = 0;
		ammo_rockets = 0;
		ammo_cells = 0;
		break;
	case PC_PYRO:
		iNailsDropped = ammo_nails;
		ammo_nails = 0;
		break;
	case PC_ENGINEER:
		iRocketsDropped = ammo_rockets;
		ammo_rockets = 0;
		break;
	default:
		return;
	}
	if ( pev->playerclass == PC_ENGINEER )
	{
		int iShares = 4;
		if ( iNailsDropped >= 20 )
			iShares--;
		if ( iRocketsDropped >= 10 )
			iShares--;
		int iCellsForEach = (int)floorf( (float)ammo_cells / iShares );
		float flHalfShare = iCellsForEach * 0.5;
		iShellsDropped = (int)floorf( flHalfShare );
		if ( iShellsDropped > 20 )
			iShellsDropped = 20;
		if ( iShellsDropped )
		{
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Drop_shells" );
			ammo_cells -= iShellsDropped * 2;
			iCellsDropped = iShellsDropped < 10 ? iShellsDropped : 10;
			ClientPrint( pev, HUD_PRINTNOTIFY, "#Drop_cells" );
			ammo_cells -= iCellsDropped * 2;
		}
		if ( iNailsDropped < 20 )
		{
			int iNails = iCellsForEach + iNailsDropped;
			if ( iNails >= 20 - iNailsDropped )
				iNails = 20 - iNailsDropped;
			if ( iNails )
			{
				ClientPrint( pev, HUD_PRINTNOTIFY, "#Drop_nails" );
				ammo_cells -= iNails;
			}
			iNailsDropped += iNails;
		}
		if ( iRocketsDropped < 10 )
		{
			int iRockets = (int)floorf( flHalfShare ) + iRocketsDropped;
			if ( iRockets >= 10 - iRocketsDropped )
				iRockets = 10 - iRocketsDropped;
			if ( iRockets )
			{
				ClientPrint( pev, HUD_PRINTNOTIFY, "#Drop_rockets" );
				ammo_cells -= iRockets * 2;
			}
			iRocketsDropped += iRockets;
		}
	}
	if ( !( iCellsDropped | iRocketsDropped | iNailsDropped ) )
		return;
	m_iAmmoboxCount++;
	if ( m_iAmmoboxCount > MAX_PLAYER_AMMOBOXES )
		RemoveOldAmmobox();
	EMIT_SOUND_DYN( edict(), CHAN_ITEM, "items/ammopickup1.wav", VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
	CWeaponBox *pBox = GetClassPtr( (CWeaponBox *)NULL );
	( (CBaseEntity *)pBox )->Spawn();
	pBox->pev->netname = MAKE_STRING( "tf_ammobox" );
	pBox->pev->classname = MAKE_STRING( "tf_ammobox" );
	pBox->pev->enemy = edict();
	pBox->m_flTouchableAt = gpGlobals->time + 2;
	pBox->team_no = team_no;
	pBox->m_flCreationTime = gpGlobals->time;
	pBox->ammo_nails = iNailsDropped;
	pBox->ammo_rockets = iRocketsDropped;
	pBox->ammo_cells = iCellsDropped;
	pBox->ammo_shells = iShellsDropped;
	UTIL_SetOrigin( pBox->pev, pev->origin );
	UTIL_MakeVectors( pev->v_angle );
	pBox->pev->angles = g_vecZero;
	pBox->pev->velocity = gpGlobals->v_forward * 400 + gpGlobals->v_up * 200;
	pBox->SetThink( &CWeaponBox::Kill );
	pBox->pev->nextthink = gpGlobals->time + 30;
}

void CBaseEntity::Timer_Hallucination()
{
	CBaseEntity *pOwner = CBaseEntity::Instance( pev->owner );
	pev->health -= 2.5;
	if ( pOwner->pev->playerclass == PC_MEDIC )
		pev->health -= 2.5;
	if ( pev->health <= 0 || pOwner->pev->deadflag || pOwner->has_disconnected )
		pOwner->tfstate &= ~TFSTATE_HALLUCINATING;
	if ( pOwner->pev->deadflag || pOwner->has_disconnected || !( pOwner->tfstate & TFSTATE_HALLUCINATING ) )
	{
		UTIL_Remove( this );
		return;
	}
	pev->nextthink = gpGlobals->time + 0.3;
	Vector vecPosition;
	Vector vecAngles;
	Vector vecVelocity;
	Vector vecSource;
	switch ( RANDOM_LONG( 0, 20 ) )
	{
	case 0:
		CLIENT_COMMAND( pOwner->edict(), "play weapons/cbar_hit1.wav\n" );
		break;
	case 1:
		CLIENT_COMMAND( pOwner->edict(), "play weapons/cbar_hitbod1.wav\n" );
		UTIL_ScreenShakeOne( pOwner->edict(), 12, 10, 0.9 );
		break;
	case 2:
		UTIL_MakeVectors( pOwner->pev->angles );
		vecPosition = pOwner->pev->origin + gpGlobals->v_forward * RANDOM_FLOAT( 100, 300 );
		vecPosition = vecPosition + gpGlobals->v_right * RANDOM_FLOAT( -100, 100 );
		MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOwner->edict() );
			WRITE_BYTE( TE_EXPLOSION );
			WRITE_COORD( vecPosition.x );
			WRITE_COORD( vecPosition.y );
			WRITE_COORD( vecPosition.z );
			WRITE_SHORT( g_sModelIndexFireball );
			WRITE_BYTE( 20 );
			WRITE_BYTE( 12 );
			WRITE_BYTE( TE_EXPLFLAG_NOADDITIVE | TE_EXPLFLAG_NODLIGHTS );
		MESSAGE_END();
		break;
	case 3:
		{
			UTIL_MakeVectors( pOwner->pev->angles );
			vecPosition = pOwner->pev->origin;
			vecPosition.z += RANDOM_FLOAT( 0, 128 );
			vecPosition = vecPosition + gpGlobals->v_right * RANDOM_FLOAT( -128, 128 );
			float flSpeed = pOwner->pev->velocity.Length();
			int iModel;
			switch ( RANDOM_LONG( 0, 3 ) )
			{
			case 0:
				iModel = g_sModelIndexMirvGrenade;
				break;
			case 1:
				iModel = g_sModelIndexGrenade;
				break;
			case 2:
				iModel = g_sModelIndexNapalmGrenade;
				break;
			default:
				iModel = g_sModelIndexNailGrenade;
				break;
			}
			if ( UTIL_PointContents( vecPosition ) == CONTENTS_SOLID )
				break;
			vecAngles = pOwner->pev->v_angle;
			vecAngles.y = UTIL_AngleMod( vecAngles.y + RANDOM_FLOAT( -50, 50 ) );
			UTIL_MakeVectors( vecAngles );
			vecVelocity = gpGlobals->v_forward * ( RANDOM_FLOAT( 200, 400 ) + flSpeed );
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOwner->edict() );
				WRITE_BYTE( TE_MODEL );
				WRITE_COORD( vecPosition.x );
				WRITE_COORD( vecPosition.y );
				WRITE_COORD( vecPosition.z );
				WRITE_COORD( vecVelocity.x );
				WRITE_COORD( vecVelocity.y );
				WRITE_COORD( vecVelocity.z );
				WRITE_ANGLE( 0 );
				WRITE_SHORT( iModel );
				WRITE_BYTE( 0 );
				WRITE_BYTE( 25 );
			MESSAGE_END();
		}
		break;
	case 4:
		MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOwner->edict() );
			WRITE_BYTE( TE_PLAYERSPRITES );
			WRITE_SHORT( pOwner->entindex() );
			WRITE_SHORT( g_sModelIndexPlayerFlame );
			WRITE_BYTE( 5 );
			WRITE_BYTE( 25 );
		MESSAGE_END();
		break;
	case 5:
		UTIL_MakeVectors( pOwner->pev->v_angle );
		gpGlobals->v_forward.z = 0;
		vecSource = pOwner->pev->origin - gpGlobals->v_forward * 200;
		vecSource = vecSource + gpGlobals->v_right * RANDOM_FLOAT( -100, 100 );
		vecSource.z += pOwner->pev->view_ofs.z * 0.8;
		vecAngles = UTIL_VecToAngles( gpGlobals->v_forward );
		vecAngles.y += RANDOM_FLOAT( -6, 6 );
		if ( UTIL_PointContents( vecSource ) == CONTENTS_SOLID )
			break;
		for ( int iSpeed = 600; iSpeed > 540; iSpeed -= 20 )
		{
			UTIL_MakeVectors( vecAngles );
			vecVelocity = gpGlobals->v_forward * iSpeed;
			MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOwner->edict() );
				WRITE_BYTE( TE_PROJECTILE );
				WRITE_COORD( vecSource.x );
				WRITE_COORD( vecSource.y );
				WRITE_COORD( vecSource.z );
				WRITE_COORD( vecVelocity.x );
				WRITE_COORD( vecVelocity.y );
				WRITE_COORD( vecVelocity.z );
				WRITE_SHORT( g_sModelIndexNail );
				WRITE_BYTE( 6 );
				WRITE_BYTE( pOwner->entindex() );
			MESSAGE_END();
			vecAngles.y += RANDOM_FLOAT( 4, 8 );
			vecSource.z += RANDOM_FLOAT( -10, 10 );
		}
		break;
	case 6:
		vecAngles = pOwner->pev->v_angle;
		vecAngles.x += RANDOM_FLOAT( -15, 15 );
		vecAngles.y += RANDOM_FLOAT( -50, 50 );
		vecAngles.z += RANDOM_FLOAT( -25, 25 );
		UTIL_VecToAngles( vecAngles );
		if ( gDecals[DECAL_GUNSHOT1].index < 0 )
			break;
		CLIENT_COMMAND( pOwner->edict(), "play weapons/sbarrel1.wav\n" );
		MESSAGE_BEGIN( MSG_ONE, SVC_TEMPENTITY, NULL, pOwner->edict() );
			WRITE_BYTE( TE_MULTIGUNSHOT );
			WRITE_COORD( pOwner->pev->origin.x );
			WRITE_COORD( pOwner->pev->origin.y );
			WRITE_COORD( pOwner->pev->origin.z );
			WRITE_COORD( vecAngles.x * 10.0 );
			WRITE_COORD( vecAngles.y * 10.0 );
			WRITE_COORD( vecAngles.z * 10.0 );
			WRITE_COORD( 80 );
			WRITE_COORD( 80 );
			WRITE_BYTE( 6 );
			WRITE_BYTE( gDecals[DECAL_GUNSHOT1].index );
		MESSAGE_END();
		break;
	}
}

#pragma warning( push )
#pragma warning( disable : 4477 )
void CBaseEntity::Timer_Birthday()
{
	if ( weapon <= 5 )
	{
		pev->nextthink = gpGlobals->time + 1.5;
		weapon++;
		UTIL_ClientPrintAll( HUD_PRINTCENTER, STRING( team_str_moved ) );
		CBaseEntity *pPlayer = NULL;
		while ( ( pPlayer = UTIL_FindEntityByClassname( pPlayer, "player" ) ) != NULL )
		{
			if ( FNullEnt( pPlayer->edict() ) )
				break;
			if ( !pPlayer->pev || pPlayer->has_disconnected )
				continue;
			if ( weapon == 1 )
				EMIT_SOUND_DYN( pPlayer->edict(), CHAN_VOICE, christmas ? "misc/b2.wav" : "misc/b1.wav",
					VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
			else if ( weapon <= 4 )
				EMIT_SOUND_DYN( pPlayer->edict(), CHAN_ITEM, RANDOM_LONG( 0, 1 ) ? "misc/party2.wav" : "misc/party1.wav",
					VOL_NORM, ATTN_NORM, 0, PITCH_NORM );
		}
		return;
	}
	weapon = 0;
	float flTime = gpGlobals->time;
	pev->nextthink = flTime + RANDOM_FLOAT( 180, 240 );
	float flChoice = RANDOM_FLOAT( 0, 1 );
	const char *pszMessage;
	if ( flChoice < 0.02 )
		pszMessage = christmas ? "\n\n\n\nMERRY CHRISTMAS!!\n" : "\n\n\n\nHAPPY BIRTHDAY TEAMFORTRESS!\n";
	else if ( flChoice < 0.04 )
		pszMessage = UTIL_VarArgs( "\n\n\n\nCurrent number of Snipers on\nthe Battlements Worldwide: %d\n",
			(double)RANDOM_FLOAT( 1000, 5000 ) );
	else if ( flChoice < 0.06 )
		pszMessage = "\n\n\n\nYeeha!\nThe 505,672nd detpack was\njust laid on the bridge!\n";
	else if ( flChoice < 0.08 )
		pszMessage = "\n\n\n\nIs that an assault cannon in your\n pocket, or are you just playing TF?\n(Blame Ian)\n";
	else if ( flChoice < 0.1 )
		pszMessage = "\n\n\n\n25,000 downloads\n50 donations\nDamn, but we rock!\n";
	else if ( flChoice < 0.12 )
		pszMessage = "\n\n\n\nTeamFortress is one year old today!\n";
	else if ( flChoice < 0.14 )
		pszMessage = "\n\n\n\nTF II available soon!\n";
	else if ( flChoice < 0.16 )
		pszMessage = "\n\n\n\nhmm.... I wonder what the next\nmessage will be...\n";
	else if ( flChoice < 0.18 )
		pszMessage = "\n\n\n\nKill your friends! Kill your family!\nSnipe your pets! Detpack your house!\nGet TF Now!\n";
	else if ( flChoice < 0.2 )
		pszMessage = "\n\n\n\nDeath to Potatoes!\n";
	else if ( flChoice < 0.22 )
		pszMessage = "\n\n\n\n\"You guys have really screwed up\n urinals, you know that?\"\n   - TyR in Australia\n";
	else if ( flChoice < 0.24 )
		pszMessage = "\n\n\n\nMany potatoes were harmed in the\nmaking of this product.\n";
	else if ( flChoice < 0.25 )
		pszMessage = "\n\n\n\nBAH! I shoulda put the Demoman\nblast armor back while I was at it!\n   - Gudlyf\n";
	else if ( flChoice < 0.26 )
		pszMessage = "\n\n\n\nTwin Peaks will be out... uhh...\nerr... soon.\n";
	else if ( flChoice < 0.28 )
		pszMessage = "\n\n\n\n\"Twin Peaks will be finished tomorrow.\"\n   - TyR, 16th July, 25th July, 2nd August...";
	else if ( flChoice < 0.3 )
		pszMessage = "\n\n\n\n\"After all, TF is an INSULT to the\npurity of CTF!\"\n   - Mjollnir, who was joking... really!";
	else if ( flChoice < 0.32 )
		pszMessage = "\n\n\n\n\"I force them to work.\nElse I cut back their supplies\"\n   - Vomitgod, on TFS\n";
	else if ( flChoice < 0.34 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen you had to use imin1 & imin2\nto join a team.\n";
	else if ( flChoice < 0.36 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen there were 2 TF servers...\nand they were always down.";
	else if ( flChoice < 0.38 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen no-one knew how to use\nhand grenades...\n";
	else if ( flChoice < 0.4 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen 2fort was the only TF map.\nWe sure have come a long\nway since then... sigh";
	else if ( flChoice < 0.42 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen the sniper rifle just\nkilled in one hit... everytime\n";
	else if ( flChoice < 0.44 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen I could download TF in\n1 minute 20 seconds\n";
	else if ( flChoice < 0.46 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen I didn't have to download\nANYTHING to play TF\n";
	else if ( flChoice < 0.48 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen players used to say\n\"What's that bubble?\"\n";
	else if ( flChoice < 0.5 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen The Well actually had a well...\n";
	else if ( flChoice < 0.52 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen the motion detector was officially\nthe Scout's best friend, at least\naccording to the readme.txt\n";
	else if ( flChoice < 0.54 )
		pszMessage = "\n\n\n\nI remember the good ol' days...\nwhen  Ian went to jail for punching\nRobin out\n";
	else if ( flChoice < 0.56 )
		pszMessage = "\n\n\n\nTF Newbie Quote #1:\nUmm... How do I set a detpack?\n";
	else if ( flChoice < 0.58 )
		pszMessage = "\n\n\n\nTF Newbie Quote #2:\nUmm... How do I throw a grenade?\n";
	else if ( flChoice < 0.6 )
		pszMessage = "\n\n\n\nTF Newbie Quote #3:\nHey, he is cheating, that soldier just jumped up to the roof!!\n";
	else if ( flChoice < 0.62 )
		pszMessage = "\n\n\n\nTF Newbie Quote #4:\nHow do I throw the pins?\n";
	else if ( flChoice < 0.64 )
		pszMessage = "\n\n\n\nTF Newbie Quote #5:\nWhere do I take the flag!?!?!\n";
	else if ( flChoice < 0.66 )
		pszMessage = "\n\n\n\nTF Newbie Quote #6:\nWhere's the control room?\n";
	else if ( flChoice < 0.68 )
		pszMessage = "\n\n\n\nTF Newbie Quote #7:\nWhy the hell are these backpacks\nexploding?\n";
	else if ( flChoice < 0.7 )
		pszMessage = "\n\n\n\nTF Newbie Quote #8:\nWho's the maniac with the axe?\n";
	else if ( flChoice < 0.72 )
		pszMessage = "\n\n\n\nTF Newbie Quote #9:\nStop shooting me!\nI'm on your team!\nYou tried to attack my with the axe!\n";
	else if ( flChoice < 0.74 )
		pszMessage = "\n\n\n\nTF Newbie Quote #10:\nWhat's this key? I thought this\nwas a CTF style map, guys?\n";
	else if ( flChoice < 0.76 )
		pszMessage = "\n\n\n\nTF Veteran Quote #1:\nUm, Red Medics, please don't\nheal the Hunted.\n";
	else if ( flChoice < 0.78 )
		pszMessage = "\n\n\n\nTF Veteran Quote #2:\nGuys could you NOT set up\nsentries in Hunted?\n";
	else if ( flChoice < 0.8 )
		pszMessage = "\n\n\n\nTF Veteran Quote #3:\nWho the hell is on\nthe lift?\n";
	else if ( flChoice < 0.82 )
		pszMessage = "\n\n\n\nTF Veteran Quote #4:\nDISCARD!!\n";
	else if ( flChoice < 0.84 )
		pszMessage = "\n\n\n\nTF Veteran Quote #5:\nDamn Snipers!\n";
	else if ( flChoice < 0.86 )
		pszMessage = "\n\n\n\nTF Veteran Quote #6:\nDamn Snipers!\n";
	else if ( flChoice < 0.88 )
		pszMessage = "\n\n\n\nTF Veteran Quote #7:\nGet a mask or\nget in the WATER!!\n";
	else if ( flChoice < 0.9 )
		pszMessage = "\n\n\n\nTF Veteran Quote #8:\nSpy's, please change colors\noutside our base.\n";
	else if ( flChoice < 0.92 )
		pszMessage = "\n\n\n\nTF Veteran Quote #9:\nI was typing!!\n";
	else if ( flChoice < 0.94 )
		pszMessage = "\n\n\n\nTF Veteran Quote #10:\nWhy is that guy running\nout of our yard with\nthe flag???\n";
	else if ( flChoice < 0.96 )
		pszMessage = "\n\n\n\nTF's original design docs!\nCheck it out!\nwww.teamfortress.com/files/teamfortress/tfdes_1.gif";
	else if ( flChoice < 0.98 )
		pszMessage = "\n\n\n\nTF's original design docs!\nCheck it out!\nwww.teamfortress.com/files/teamfortress/tfdes_2.gif";
	else
		pszMessage = "\n\n\n\n\"MJ: Did you hear John Cash wrote TF?\"\n   - VomitGod";
	team_str_moved = MAKE_STRING( pszMessage );
}
#pragma warning( pop )
