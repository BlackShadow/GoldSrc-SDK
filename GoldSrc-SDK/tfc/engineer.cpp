#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

class CNoBuildArea : public CBaseEntity
{
public:
	void Spawn() override;
};

LINK_ENTITY_TO_CLASS( func_nobuild, CNoBuildArea );

void CNoBuildArea::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	pev->effects |= EF_NODRAW;
	SET_MODEL( edict(), STRING( pev->model ) );
	UTIL_SetSize( pev, pev->mins, pev->maxs );
	UTIL_SetOrigin( pev, pev->origin );
}

void DestroyBuilding( CBaseEntity* pEngineer, char* pszBuilding )
{
	CBaseEntity* pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, pszBuilding )) != NULL )
	{
		if ( pBuilding->real_owner != pEngineer )
			continue;

		int contents = UTIL_PointContents( pBuilding->pev->origin );
		if ( contents == CONTENTS_SKY || contents == CONTENTS_SOLID )
		{
			pEngineer->ammo_cells += 100;
			((CBasePlayer*)pEngineer)->TeamFortress_CheckClassStats();
		}
		pBuilding->TakeDamage( pBuilding->pev, pBuilding->pev, 500, 0 );
	}
}

void DestroyTeleporter( CBaseEntity* pEngineer, int iType )
{
	CBaseEntity* pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_teleporter" )) != NULL )
	{
		if ( pBuilding->real_owner != pEngineer || ((CTFTeleporter*)pBuilding)->m_iType != iType )
			continue;

		int contents = UTIL_PointContents( pBuilding->pev->origin );
		if ( contents == CONTENTS_SKY || contents == CONTENTS_SOLID )
		{
			pEngineer->ammo_cells += 100;
			((CBasePlayer*)pEngineer)->TeamFortress_CheckClassStats();
		}
		pBuilding->TakeDamage( pBuilding->pev, pBuilding->pev, 500, 0 );
	}
}

void CBasePlayer::TeamFortress_RemoveBuildings()
{
	CBaseEntity* pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_dispenser" )) != NULL )
	{
		if ( pBuilding->real_owner == this )
			break;
	}
	if ( pBuilding )
	{
		if ( !no_dispenser_message )
			ClientPrint( pev, HUD_PRINTCENTER, "#Dispenser_destroyed" );
		has_dispenser = 0;
		no_dispenser_message = 0;
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pBuilding->edict(), pBuilding->m_usBuildingEvent,
			0, pBuilding->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		dremove( pBuilding );
	}

	pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_teleporter" )) != NULL )
	{
		if ( pBuilding->real_owner == this && ((CTFTeleporter*)pBuilding)->m_iType == 4 )
			break;
	}
	if ( pBuilding )
	{
		if ( !no_entry_teleporter_message )
			ClientPrint( pev, HUD_PRINTCENTER, "#Teleporter_Entrance_Destroyed" );
		no_entry_teleporter_message = 0;
		has_entry_teleporter = 0;
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pBuilding->edict(), pBuilding->m_usBuildingEvent,
			0, pBuilding->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		dremove( pBuilding );
	}

	pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_teleporter" )) != NULL )
	{
		if ( pBuilding->real_owner == this && ((CTFTeleporter*)pBuilding)->m_iType == 5 )
			break;
	}
	if ( pBuilding )
	{
		if ( !no_exit_teleporter_message )
			ClientPrint( pev, HUD_PRINTCENTER, "#Teleporter_Exit_Destroyed" );
		no_exit_teleporter_message = 0;
		has_exit_teleporter = 0;
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pBuilding->edict(), pBuilding->m_usBuildingEvent,
			0, pBuilding->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		dremove( pBuilding );
	}

	pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_sentrygun" )) != NULL )
	{
		if ( pBuilding->real_owner == this )
			break;
	}
	if ( pBuilding )
	{
		if ( !no_sentry_message )
			ClientPrint( pev, HUD_PRINTCENTER, "#Sentry_destroyed" );
		has_sentry = 0;
		no_sentry_message = 0;
		PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, pBuilding->edict(), pBuilding->m_usBuildingEvent,
			0, pBuilding->pev->origin, (float*)&g_vecZero, 0, 0, 1024, 0, 0, 0 );
		for ( int i = 1; i <= gpGlobals->maxClients; ++i )
		{
			CBasePlayer* pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( i );
			if ( pPlayer && pPlayer->building == pBuilding )
			{
				pPlayer->ResetMenu();
				pPlayer->building = NULL;
			}
		}
		dremove( pBuilding );
	}

	pBuilding = NULL;
	while ( (pBuilding = UTIL_FindEntityByClassname( pBuilding, "building_sentrygun_base" )) != NULL )
	{
		if ( pBuilding->real_owner == this )
		{
			dremove( pBuilding );
			break;
		}
	}
}

int CBaseEntity::CheckArea( CBaseEntity* pIgnore )
{
	Vector vecOrg = pev->origin;
	if ( (UTIL_PointContents( vecOrg ) & ~2) != CONTENTS_WATER )
		return 1;

	Vector vecEntityOrg = pev->origin;
	if ( pIgnore->pev->flags & FL_DUCKING )
		vecEntityOrg.z += 18;
	TraceResult tr;
	UTIL_TraceLine( pIgnore->EyePosition(), vecEntityOrg, ignore_monsters, edict(), &tr );
	if ( tr.flFraction != 1 || tr.fAllSolid == 1 )
		return 1;
	UTIL_TraceHull( vecEntityOrg, vecEntityOrg, ignore_monsters, head_hull, edict(), &tr );
	if ( !tr.pHit )
		g_engfuncs.pfnPEntityOfEntOffset( 0 );
	if ( tr.flFraction != 1 || tr.fAllSolid == 1 )
		return 1;

	CBaseEntity* pEntity = NULL;
	while ( (pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, 48 )) != NULL )
	{
		if ( pEntity != pIgnore && pEntity != this && pEntity->pev->solid > SOLID_TRIGGER )
			return 1;
	}
	while ( (pEntity = UTIL_FindEntityByClassname( pEntity, "func_nobuild" )) != NULL )
	{
		if ( vecOrg.x >= pEntity->pev->mins.x && vecOrg.y >= pEntity->pev->mins.y && vecOrg.z >= pEntity->pev->mins.z &&
			vecOrg.x <= pEntity->pev->maxs.x && vecOrg.y <= pEntity->pev->maxs.y && vecOrg.z <= pEntity->pev->maxs.z )
			return 2;
	}
	UTIL_TraceLine( vecOrg, vecOrg - Vector( 0, 0, 64 ), dont_ignore_monsters, edict(), &tr );
	return tr.flFraction == 1 ? 1 : 0;
}

void CBaseEntity::CheckBelowBuilding( int iDistance )
{
	TraceResult tr;
	UTIL_TraceLine( pev->origin + Vector( 0, 0, 3 ), pev->origin - Vector( 0, 0, (float)iDistance ),
		dont_ignore_monsters, edict(), &tr );
	if ( tr.flFraction == 1 )
	{
		pev->movetype = MOVETYPE_TOSS;
		pev->flags &= ~FL_ONGROUND;
	}
}

void CBasePlayer::TeamFortress_Build( int iBuildingID )
{
	UTIL_MakeAimVectors( pev->angles );
	Vector vecDirection( gpGlobals->v_forward.x, gpGlobals->v_forward.y, 0 );
	Vector vecOrigin = pev->origin + vecDirection.Normalize() * 64;
	vecOrigin.x = (float)(int)vecOrigin.x;
	vecOrigin.y = (float)(int)vecOrigin.y;
	if ( current_menu == 15 )
	{
		ResetMenu();
		pev->impulse = 0;
	}

	float flBuildTime;
	switch ( iBuildingID )
	{
	case 1:
		if ( has_dispenser )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Build_onedispenser" );
			return;
		}
		if ( ammo_cells < 100 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Build_notenoughmetal" );
			return;
		}
		building = CTFDispenser::CreateDispenser( vecOrigin, pev->v_angle, this );
		flBuildTime = 2;
		break;
	case 2:
		if ( has_sentry )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Build_onesentry" );
			return;
		}
		if ( ammo_cells < 130 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Build_notenoughmetal" );
			return;
		}
		building = CTFSentrygunBase::CreateSentryBase( vecOrigin, pev->v_angle, this );
		flBuildTime = 5;
		break;
	case 4:
	case 5:
		if ( iBuildingID == 4 ? has_entry_teleporter != 0 : has_exit_teleporter != 0 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, iBuildingID == 4 ? "#Build_entryteleporter" : "#Build_exitteleporter" );
			return;
		}
		if ( ammo_cells < 125 )
		{
			ClientPrint( pev, HUD_PRINTCENTER, "#Build_notenoughmetal" );
			return;
		}
		building = CTFTeleporter::CreateTeleporter( vecOrigin, pev->v_angle, this, iBuildingID );
		flBuildTime = 4;
		break;
	default:
		return;
	}

	int iArea = building->CheckArea( this );
	if ( iArea || !(pev->flags & FL_ONGROUND) )
	{
		ClientPrint( pev, HUD_PRINTCENTER, iArea ? (iArea == 2 ? "#Build_nobuild" : "#Build_noroom") : "#Build_air" );
		UTIL_Remove( building );
		building = NULL;
		return;
	}
	is_building = TRUE;
	immune_to_check = gpGlobals->time + 5;
	tfstate |= 0x10000;
	TeamFortress_SetSpeed();
	if ( m_pActiveItem )
		m_pActiveItem->Holster( 0 );
	CBaseEntity* pTimer = CreateTimer( 11 );
	pTimer->SetThink( &CBaseEntity::Timer_FinishedBuilding );
	pTimer->pev->nextthink = gpGlobals->time + flBuildTime;
	EMIT_SOUND_DYN( edict(), CHAN_STATIC, "weapons/building.wav", 0.98f, ATTN_NORM, 0, 125 );
}

void CBasePlayer::TeamFortress_EngineerBuild( int iBuildingNo )
{
	if ( !IsAlive() || cb_prematch_time > gpGlobals->time )
		return;
	if ( !(pev->flags & FL_ONGROUND) )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Build_air" );
		return;
	}
	if ( is_building )
	{
		ClientPrint( pev, HUD_PRINTCENTER, "#Build_stop" );
		tfstate &= ~0x10000;
		TeamFortress_SetSpeed();
		CBaseEntity* pTimer = FindTimer( 11 );
		if ( pTimer )
			dremove( pTimer );
		UTIL_Remove( building );
		building = NULL;
		is_building = FALSE;
		EMIT_SOUND_DYN( edict(), CHAN_STATIC, "weapons/building.wav", 0, 0, SND_STOP, PITCH_NORM );
		if ( m_pActiveItem )
			m_pActiveItem->Deploy();
	}
	else if ( iBuildingNo > 0 )
		TeamFortress_Build( iBuildingNo );
	else if ( ammo_cells < 100 && !has_dispenser && !has_sentry )
		ClientPrint( pev, HUD_PRINTCENTER, "#Build_nometal" );
	else
		current_menu = 15;
}
