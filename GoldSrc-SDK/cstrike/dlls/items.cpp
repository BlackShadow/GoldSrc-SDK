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
#include "skill.h"
#include "items.h"
#include "gamerules.h"
#include "tutor.h"

#define MAX_NORMAL_BATTERY	100

extern int gmsgItemPickup;
extern int gmsgArmorType;

class CWorldItem : public CBaseEntity
{
public:
	void	KeyValue(KeyValueData *pkvd ); 
	void	Spawn( void );
	int		m_iType;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS(world_items, CWorldItem);

void CWorldItem::KeyValue(KeyValueData *pkvd)
{
	if (FStrEq(pkvd->szKeyName, "type"))
	{
		m_iType = atoi(pkvd->szValue);
		pkvd->fHandled = TRUE;
	}
	else
		CBaseEntity::KeyValue( pkvd );
}

void CWorldItem::Spawn( void )
{
	CBaseEntity *pEntity = NULL;

	switch (m_iType) 
	{
	case 44: // ITEM_BATTERY:
		pEntity = CBaseEntity::Create( "item_battery", pev->origin, pev->angles );
		break;
	case 42: // ITEM_ANTIDOTE:
		pEntity = CBaseEntity::Create( "item_antidote", pev->origin, pev->angles );
		break;
	case 43: // ITEM_SECURITY:
		pEntity = CBaseEntity::Create( "item_security", pev->origin, pev->angles );
		break;
	case 45: // ITEM_SUIT:
		pEntity = CBaseEntity::Create( "item_suit", pev->origin, pev->angles );
		break;
	}

	if ( pEntity != NULL )
	{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
	}

	REMOVE_ENTITY(edict());
}


void CItem::Spawn( void )
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize(pev, Vector(-16, -16, 0), Vector(16, 16, 16));
	SetTouch(&CItem::ItemTouch);

	if (DROP_TO_FLOOR(ENT(pev)) == 0)
	{
		UTIL_Remove( this );
		return;
	}
}

extern int gEvilImpulse101;

void CItem::ItemTouch( CBaseEntity *pOther )
{
	// if it's not a player, ignore
	if ( !pOther->IsPlayer() || pOther->pev->deadflag != DEAD_NO )
	{
		return;
	}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if ( !g_pGameRules->CanHaveItem( pPlayer, this ) )
	{
		// no? Ignore the touch.
		return;
	}

	if (MyTouch( pPlayer ))
	{
		SUB_UseTargets( pOther, USE_TOGGLE, 0 );
		SetTouch( NULL );
		
		// player grabbed the item. 
		g_pGameRules->PlayerGotItem( pPlayer, this );
		if ( g_pGameRules->ItemShouldRespawn( this ) == GR_ITEM_RESPAWN_YES )
		{
			Respawn(); 
		}
		else
		{
			UTIL_Remove( this );
		}
	}
	else if (gEvilImpulse101)
	{
		UTIL_Remove( this );
	}
}

CBaseEntity* CItem::Respawn( void )
{
	SetTouch( NULL );
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin( pev, g_pGameRules->VecItemRespawnSpot( this ) );// blip to whereever you should respawn.

	SetThink ( &CItem::Materialize );
	pev->nextthink = g_pGameRules->FlItemRespawnTime( this ); 
	return this;
}

void CItem::Materialize( void )
{
	if ( pev->effects & EF_NODRAW )
	{
		// changing from invisible state to visible.
		if ( g_pGameRules->IsMultiplayer() )
			EMIT_SOUND_DYN( ENT(pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_NORM, 0, 150 );
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
	}

	SetTouch( &CItem::ItemTouch );
}

class CItemSuit : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_kevlar.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_kevlar.mdl");
		PRECACHE_SOUND( "items/tr_kevlar.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) )
			return FALSE;

		EMIT_SOUND( pPlayer->edict(), CHAN_VOICE, "items/tr_kevlar.wav", 1, ATTN_NORM );
		pPlayer->pev->weapons |= (1U << WEAPON_SUIT);
		pPlayer->m_iHideHUD &= ~(HIDEHUD_HEALTH | HIDEHUD_MONEY);
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_suit, CItemSuit);



class CItemBattery : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_battery.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_battery.mdl");
		PRECACHE_SOUND( "items/gunpickup2.wav" );
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( !(pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) )
			return FALSE;

		if ( pPlayer->pev->weapons & (1U << WEAPON_SUIT) )
		{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			if ( pPlayer->pev->armorvalue > MAX_NORMAL_BATTERY )
				pPlayer->pev->armorvalue = MAX_NORMAL_BATTERY;

			EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			
			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)(100.0 * pPlayer->pev->armorvalue * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;
		
			sprintf( szcharge,"!HEV_%1dP", pct );
			
			//EMIT_SOUND_SUIT(ENT(pev), szcharge);
			pPlayer->SetSuitUpdate(szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS(item_battery, CItemBattery);


class CItemAntidote : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_antidote.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_antidote.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->SetSuitUpdate("!HEV_DET4", FALSE, SUIT_NEXT_IN_1MIN);
		
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_antidote, CItemAntidote);


class CItemSecurity : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_security.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_security.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS(item_security, CItemSecurity);

class CItemLongJump : public CItem
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_longjump.mdl");
		CItem::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_longjump.mdl");
	}
	BOOL MyTouch( CBasePlayer *pPlayer )
	{
		if ( pPlayer->m_fLongJump )
		{
			return FALSE;
		}

		if ( ( pPlayer->pev->weapons & (1<<WEAPON_SUIT) ) )
		{
			pPlayer->m_fLongJump = TRUE;// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "slj", "1" );

			MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
				WRITE_STRING( STRING(pev->classname) );
			MESSAGE_END();

			EMIT_SOUND_SUIT( pPlayer->edict(), "!HEV_A1" );	// Play the longjump sound UNDONE: Kelly? correct sound?
			return TRUE;		
		}
		return FALSE;
	}
};

LINK_ENTITY_TO_CLASS( item_longjump, CItemLongJump );

class CItemKevlar : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_kevlar.mdl" );
		CItem::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_kevlar.mdl" );
	}

	BOOL MyTouch( CBasePlayer* player ) override
	{
		if ( player->m_iKevlar == 0 )
			player->m_iKevlar = 1;
		player->pev->armorvalue = 100;
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM );
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( STRING( pev->classname ) );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ONE, gmsgArmorType, NULL, player->pev );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		if ( TheTutor != NULL )
			TheTutor->OnEvent( EVENT_PLAYER_BOUGHT_SOMETHING, player, NULL );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_kevlar, CItemKevlar );

class CItemAssaultSuit : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_assault.mdl" );
		CItem::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_assault.mdl" );
	}

	BOOL MyTouch( CBasePlayer* player ) override
	{
		player->m_iKevlar = 2;
		player->pev->armorvalue = 100;
		EMIT_SOUND( player->edict(), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM );
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, player->pev );
		WRITE_STRING( STRING( pev->classname ) );
		MESSAGE_END();
		MESSAGE_BEGIN( MSG_ONE, gmsgArmorType, NULL, player->pev );
		WRITE_BYTE( 1 );
		MESSAGE_END();
		if ( TheTutor != NULL )
			TheTutor->OnEvent( EVENT_PLAYER_BOUGHT_SOMETHING, player, NULL );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_assaultsuit, CItemAssaultSuit );

class CItemThighPack : public CItem
{
	void Spawn() override
	{
		Precache();
		SET_MODEL( edict(), "models/w_thighpack.mdl" );
		CItem::Spawn();
	}

	void Precache() override
	{
		PRECACHE_MODEL( "models/w_thighpack.mdl" );
	}

	BOOL MyTouch( CBasePlayer* player ) override
	{
		if ( player->m_iTeam != CT || player->m_bHasDefuser )
			return FALSE;
		player->m_bHasDefuser = true;
		player->pev->body = 1;
		ClientPrint( player->pev, HUD_PRINTCENTER, "#Got_defuser" );
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, player->pev );
		WRITE_BYTE( 1 );
		WRITE_STRING( "defuser" );
		WRITE_BYTE( 0 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
		MESSAGE_END();
		SendItemStatus( player );
		EMIT_SOUND( player->edict(), CHAN_VOICE, "items/kevlar.wav", 1, ATTN_NORM );
		player->SetScoreboardAttributes( NULL );
		if ( TheTutor != NULL )
			TheTutor->OnEvent( EVENT_PLAYER_BOUGHT_SOMETHING, player, NULL );
		return TRUE;
	}
};

LINK_ENTITY_TO_CLASS( item_thighpack, CItemThighPack );
