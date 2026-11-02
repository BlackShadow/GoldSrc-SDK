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
/*

===== items.cpp ========================================================

  functions governing the selection/use of weapons for players

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "items.h"
#include "gamerules.h"

extern int gEvilImpulse101;

class CWorldItem : public CBaseEntity
{
public:
	void KeyValue( KeyValueData* pkvd ) override;
	void Spawn() override;

	int m_iType;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( world_items, CWorldItem );

void CWorldItem::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "type" ) )
	{
		m_iType = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CWorldItem::Spawn()
{
	CBaseEntity* pEntity = NULL;
	switch ( m_iType )
	{
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 44:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}
	if ( pEntity )
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}
	else
		ALERT( at_console, "unable to create world_item %d\n", m_iType );
	REMOVE_ENTITY( edict() );
}

void CItem::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( this, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
	SetTouch( &CItem::ItemTouch );
	if ( DROP_TO_FLOOR( edict() ) == 0 )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

void CItem::ItemTouch( CBaseEntity* pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;
	CBasePlayer* pPlayer = (CBasePlayer*)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
		return;
	if ( MyTouch( pPlayer ) )
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );

		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
			Respawn();
		else
			UTIL_Remove( this );
	}
	else if ( gEvilImpulse101 )
		UTIL_Remove( this );
}

CBaseEntity* CItem::Respawn()
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;
	UTIL_SetOrigin( this, g_pGameRules->VecItemRespawnSpot( this ) ); // blip to whereever you should respawn.
	SetThink( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this );
	return this;
}

void CItem::Materialize()
{
	if ( pev->effects & EF_NODRAW )
	{
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}
	SetTouch( &CItem::ItemTouch );
}

BOOL CItem::MyTouch( CBasePlayer* pPlayer )
{
	(void)pPlayer;
	return FALSE;
}
