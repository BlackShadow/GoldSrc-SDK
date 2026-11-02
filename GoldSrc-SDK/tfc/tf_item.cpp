#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "items.h"
#include "tf_defs.h"

extern int gmsgItemPickup;

class CItemHealth : public CItem
{
public:
	void Spawn();
	void Precache();
	BOOL MyTouch( CBasePlayer* pPlayer );

	int	m_iHealAmount;
	int	m_iHealType;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CItemArmor : public CItem
{
public:
	BOOL MyTouch( CBasePlayer* pPlayer );
	int CalculateBit( float flArmorType );

	float	m_flArmorCount;
	float	m_flArmorType;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CItemArmorGreen : public CItemArmor
{
public:
	void Spawn();
	void Precache();
};

class CItemArmorYellow : public CItemArmor
{
public:
	void Spawn();
	void Precache();
};

class CItemArmorRed : public CItemArmor
{
public:
	void Spawn();
	void Precache();
};

class CItemAmmo : public CItem
{
public:
	void Spawn();
	void Precache();
	BOOL MyTouch( CBasePlayer* pPlayer );
	virtual BOOL GetGrenadePossibility( CBasePlayer* pPlayer );

	int	m_isSmallBox;
	int	m_isLargeBox;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

class CItemAmmoShells : public CItemAmmo
{
public:
	void Spawn();
	void Precache();
};

class CItemAmmoNails : public CItemAmmo
{
public:
	void Spawn();
	void Precache();
};

class CItemAmmoRockets : public CItemAmmo
{
public:
	void Spawn();
	void Precache();
	BOOL GetGrenadePossibility( CBasePlayer* pPlayer );
};

class CItemAmmoCells : public CItemAmmo
{
public:
	void Spawn();
	void Precache();
};

class CItemPowerup : public CItem
{
public:
	void Spawn();
	BOOL MyTouch( CBasePlayer* pPlayer );
};

class CItemPowerupInvincible : public CItemPowerup
{
public:
	void Spawn();
	void Precache();
};

class CItemPowerupRadsuit : public CItemPowerup
{
public:
	void Spawn();
	void Precache();
};

class CItemPowerupInvisibility : public CItemPowerup
{
public:
	void Spawn();
	void Precache();
};

class CItemPowerupQuad : public CItemPowerup
{
public:
	void Spawn();
	void Precache();
};

LINK_ENTITY_TO_CLASS( item_health, CItemHealth );
LINK_ENTITY_TO_CLASS( item_armor1, CItemArmorGreen );
LINK_ENTITY_TO_CLASS( item_armor2, CItemArmorYellow );
LINK_ENTITY_TO_CLASS( item_armor3, CItemArmorRed );
LINK_ENTITY_TO_CLASS( item_shells, CItemAmmoShells );
LINK_ENTITY_TO_CLASS( item_spikes, CItemAmmoNails );
LINK_ENTITY_TO_CLASS( item_rockets, CItemAmmoRockets );
LINK_ENTITY_TO_CLASS( item_cells, CItemAmmoCells );
LINK_ENTITY_TO_CLASS( item_artifact_invulnerability, CItemPowerupInvincible );
LINK_ENTITY_TO_CLASS( item_artifact_envirosuit, CItemPowerupRadsuit );
LINK_ENTITY_TO_CLASS( item_artifact_invisibility, CItemPowerupInvisibility );
LINK_ENTITY_TO_CLASS( item_artifact_super_damage, CItemPowerupQuad );

void CItemHealth::Precache()
{
	PRECACHE_MODEL( "models/w_medkit.mdl" );
	PRECACHE_SOUND( "items/health1.wav" );
	PRECACHE_SOUND( "items/r_item1.wav" );
	PRECACHE_SOUND( "items/smallmedkit1.wav" );
}

void CItemHealth::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}

	Precache();
	SET_MODEL( ENT( pev ), "models/w_medkit.mdl" );
	CItem::Spawn();
	if ( !m_flRespawnTime )
		m_flRespawnTime = 20;

	if ( pev->spawnflags & 1 )
	{
		pev->noise = MAKE_STRING( "items/smallmedkit1.wav" );
		m_iHealAmount = 15;
		m_iHealType = 0;
	}
	else if ( pev->spawnflags & 2 )
	{
		pev->noise = MAKE_STRING( "items/r_item1.wav" );
		m_iHealAmount = 100;
		m_iHealType = 1;
	}
	else
	{
		pev->noise = MAKE_STRING( "items/health1.wav" );
		m_iHealAmount = 25;
		m_iHealType = 0;
	}
}

BOOL CItemHealth::MyTouch( CBasePlayer* pPlayer )
{
	if ( m_iHealType == 1 && !( pPlayer->tfstate & TFSTATE_INFECTED ) )
	{
		if ( pPlayer->pev->health >= 250 || !pPlayer->TakeHealth( (float)m_iHealAmount, DMG_IGNORE_MAXHEALTH ) )
			return FALSE;

		pPlayer->items |= IT_SUPERHEALTH;
		CBaseEntity* pTimer = pPlayer->CreateTimer( TF_TIMER_ROTHEALTH );
		pTimer->pev->nextthink = gpGlobals->time + 5;
		pTimer->SetThink( &CBaseEntity::Timer_HealthRot );
		pTimer->pev->enemy = edict();
		SUB_UseTargets( this, USE_TOGGLE, 0 );
		return TRUE;
	}

	if ( m_iHealType != 1 && !pPlayer->TakeHealth( (float)m_iHealAmount, 0 ) )
		return FALSE;

	if ( ( pPlayer->tfstate & TFSTATE_INFECTED ) && m_iHealAmount > 80 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Item_medhealed" );
		pPlayer->tfstate &= ~TFSTATE_INFECTED;
	}

	if ( g_pGameRules->ItemShouldRespawn( this ) )
		Respawn();
	else
		UTIL_Remove( this );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	return TRUE;
}

int CItemArmor::CalculateBit( float flArmorType )
{
	if ( flArmorType == 0.3f )
		return IT_ARMOR1;
	if ( flArmorType == 0.6f )
		return IT_ARMOR2;
	if ( flArmorType == 0.8f )
		return IT_ARMOR3;
	ALERT( at_error, "Bad ArmorType.\n" );
	return 0;
}

BOOL CItemArmor::MyTouch( CBasePlayer* pPlayer )
{
	if ( pPlayer->pev->armorvalue >= pPlayer->maxarmor
		&& ( pPlayer->pev->playerclass != PC_ENGINEER || pPlayer->ammo_cells >= pPlayer->maxammo_cells ) )
		return FALSE;

	float flArmorType = pPlayer->pev->armorvalue;
	if ( flArmorType <= m_flArmorType )
		flArmorType = m_flArmorType;
	if ( flArmorType > pPlayer->armor_allowed )
		flArmorType = pPlayer->armor_allowed;
	int iArmorBit = CalculateBit( flArmorType );
	float flArmorCount = pPlayer->maxarmor;
	if ( m_flArmorCount <= flArmorCount )
	{
		flArmorCount = m_flArmorCount;
	}
	else if ( pPlayer->pev->playerclass == PC_ENGINEER && pPlayer->ammo_cells < pPlayer->maxammo_cells )
	{
		float flCells = (float)(int)m_flArmorCount - flArmorCount + pPlayer->ammo_cells;
		pPlayer->ammo_cells = min( (int)flCells, pPlayer->maxammo_cells );
	}

	pPlayer->pev->armortype = flArmorType;
	pPlayer->pev->armorvalue = min( flArmorCount + pPlayer->pev->armorvalue, pPlayer->maxarmor );
	pPlayer->items = ( pPlayer->items & ~( IT_ARMOR1 | IT_ARMOR2 | IT_ARMOR3 ) ) | iArmorBit;
	if ( armorclass > 0 )
		pPlayer->armorclass = armorclass;
	EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/armoron_1.wav", 1, ATTN_NORM );
	if ( g_pGameRules->ItemShouldRespawn( this ) )
		Respawn();
	else
		UTIL_Remove( this );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	return TRUE;
}

void CItemArmorGreen::Precache()
{
	PRECACHE_MODEL( "models/g_armor.mdl" );
	PRECACHE_SOUND( "items/armoron_1.wav" );
}

void CItemArmorGreen::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	Precache();
	SET_MODEL( edict(), "models/g_armor.mdl" );
	CItem::Spawn();
	m_flArmorCount = 100;
	m_flArmorType = 0.3f;
}

void CItemArmorYellow::Precache()
{
	PRECACHE_MODEL( "models/y_armor.mdl" );
	PRECACHE_SOUND( "items/armoron_1.wav" );
}

void CItemArmorYellow::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	Precache();
	SET_MODEL( edict(), "models/y_armor.mdl" );
	CItem::Spawn();
	m_flArmorCount = 150;
	m_flArmorType = 0.6f;
}

void CItemArmorRed::Precache()
{
	PRECACHE_MODEL( "models/r_armor.mdl" );
	PRECACHE_SOUND( "items/armoron_1.wav" );
}

void CItemArmorRed::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	Precache();
	SET_MODEL( edict(), "models/r_armor.mdl" );
	CItem::Spawn();
	m_flArmorCount = 200;
	m_flArmorType = 0.8f;
}

void CItemAmmo::Precache()
{
	PRECACHE_SOUND( "items/gunpickup2.wav" );
}

void CItemAmmo::Spawn()
{
	CItem::Spawn();
	PRECACHE_SOUND( "items/gunpickup2.wav" );
	if ( pev->spawnflags & 1 )
	{
		SET_MODEL( edict(), STRING( m_isLargeBox ) );
		ammo_shells *= 2;
		ammo_nails *= 2;
		ammo_rockets *= 2;
		ammo_cells *= 2;
	}
	else
	{
		SET_MODEL( edict(), STRING( m_isSmallBox ) );
	}
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
}

BOOL CItemAmmo::GetGrenadePossibility( CBasePlayer* pPlayer )
{
	return FALSE;
}

BOOL CItemAmmo::MyTouch( CBasePlayer* pPlayer )
{
	BOOL fGrenade = GetGrenadePossibility( pPlayer );
	BOOL fAmmo = pPlayer->GiveTFAmmo( ammo_shells, ammo_nails, ammo_rockets, ammo_cells );
	if ( !fGrenade && !fAmmo )
		return FALSE;
	EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_NORM );
	if ( fGrenade )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
			WRITE_STRING( "grenade" );
		MESSAGE_END();
	}
	if ( g_pGameRules->ItemShouldRespawn( this ) )
		Respawn();
	else
		UTIL_Remove( this );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	return TRUE;
}

BOOL CItemAmmoRockets::GetGrenadePossibility( CBasePlayer* pPlayer )
{
	if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
		return FALSE;

	if ( RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		int iType = pPlayer->tp_grenades_1;
		if ( !iType )
			return FALSE;
		int iMax = iType == GR_TYPE_NAIL || iType == GR_TYPE_MIRV ? 2 : iType == GR_TYPE_CALTROP ? 3 : 4;
		if ( pPlayer->no_grenades_1 >= iMax )
			return FALSE;
		pPlayer->no_grenades_1++;
		return TRUE;
	}

	int iType = pPlayer->tp_grenades_2;
	if ( !iType )
		return FALSE;
	int iMax = iType == GR_TYPE_NAIL || iType == GR_TYPE_MIRV ? 2 : iType == GR_TYPE_CONCUSSION ? 3 : 4;
	if ( pPlayer->no_grenades_2 >= iMax )
		return FALSE;
	pPlayer->no_grenades_2++;
	return TRUE;
}

void CItemAmmoShells::Precache()
{
	PRECACHE_MODEL( "models/w_shotbox.mdl" );
}

void CItemAmmoShells::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	m_isSmallBox = MAKE_STRING( "models/w_shotbox.mdl" );
	m_isLargeBox = MAKE_STRING( "models/w_shotbox.mdl" );
	pev->netname = MAKE_STRING( "shells" );
	ammo_shells = 20;
	Precache();
	CItemAmmo::Spawn();
}

void CItemAmmoNails::Precache()
{
	PRECACHE_MODEL( "models/w_crossbow_clip.mdl" );
}

void CItemAmmoNails::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	m_isSmallBox = MAKE_STRING( "models/w_crossbow_clip.mdl" );
	m_isLargeBox = MAKE_STRING( "models/w_crossbow_clip.mdl" );
	pev->netname = MAKE_STRING( "nails" );
	ammo_nails = 25;
	Precache();
	CItemAmmo::Spawn();
}

void CItemAmmoRockets::Precache()
{
	PRECACHE_MODEL( "models/w_rpgammo.mdl" );
}

void CItemAmmoRockets::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	m_isSmallBox = MAKE_STRING( "models/w_rpgammo.mdl" );
	m_isLargeBox = MAKE_STRING( "models/w_rpgammo.mdl" );
	pev->netname = MAKE_STRING( "rockets" );
	ammo_rockets = 5;
	Precache();
	CItemAmmo::Spawn();
}

void CItemAmmoCells::Precache()
{
	PRECACHE_MODEL( "models/w_battery.mdl" );
}

void CItemAmmoCells::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	m_isSmallBox = MAKE_STRING( "models/w_battery.mdl" );
	m_isLargeBox = MAKE_STRING( "models/w_battery.mdl" );
	pev->netname = MAKE_STRING( "cells" );
	ammo_cells = 5;
	Precache();
	CItemAmmo::Spawn();
}

void CItemPowerup::Spawn()
{
	if ( !CheckExistence() )
	{
		dremove( this );
		return;
	}
	Precache();
	CItem::Spawn();
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 16 ) );
}

BOOL CItemPowerup::MyTouch( CBasePlayer* pPlayer )
{
	EMIT_SOUND( pPlayer->edict(), CHAN_ITEM, STRING( pev->noise ), 1, ATTN_NORM );
	pPlayer->items |= items;
	if ( invincible_finished )
		pPlayer->invincible_finished = invincible_finished + gpGlobals->time;
	if ( radsuit_finished )
		pPlayer->radsuit_finished = radsuit_finished + gpGlobals->time;
	if ( invisible_finished )
		pPlayer->invisible_finished = invisible_finished + gpGlobals->time;
	if ( super_damage_finished )
		pPlayer->super_damage_finished = super_damage_finished + gpGlobals->time;
	pPlayer->pev->renderfx = kRenderFxNone;
	if ( g_pGameRules->ItemShouldRespawn( this ) )
		Respawn();
	else
		UTIL_Remove( this );
	SUB_UseTargets( this, USE_TOGGLE, 0 );
	return TRUE;
}

void CItemPowerupInvincible::Precache()
{
	PRECACHE_MODEL( "models/pent.mdl" );
	PRECACHE_SOUND( "items/protect.wav" );
	PRECACHE_SOUND( "items/protect2.wav" );
	PRECACHE_SOUND( "items/protect3.wav" );
}

void CItemPowerupInvincible::Spawn()
{
	Precache();
	CItemPowerup::Spawn();
	if ( !m_flRespawnTime )
		m_flRespawnTime = 300;
	invincible_finished = 30;
	SET_MODEL( edict(), "models/pent.mdl" );
	pev->netname = MAKE_STRING( "Pentagram of Protection" );
	pev->noise = MAKE_STRING( "items/protect.wav" );
	items = IT_INVULNERABILITY;
}

void CItemPowerupRadsuit::Precache()
{
	PRECACHE_MODEL( "models/w_suit.mdl" );
	PRECACHE_SOUND( "FVox/HEV_logon.wav" );
	PRECACHE_SOUND( "FVox/hev_shutdown.wav" );
}

void CItemPowerupRadsuit::Spawn()
{
	Precache();
	CItemPowerup::Spawn();
	if ( !m_flRespawnTime )
		m_flRespawnTime = 60;
	radsuit_finished = 30;
	SET_MODEL( edict(), "models/w_suit.mdl" );
	pev->netname = MAKE_STRING( "Biosuit" );
	pev->noise = MAKE_STRING( "FVox/HEV_logon.wav" );
	items = IT_SUIT;
}

void CItemPowerupInvisibility::Precache()
{
	PRECACHE_MODEL( "models/invis.mdl" );
	PRECACHE_SOUND( "items/inv1.wav" );
	PRECACHE_SOUND( "items/inv2.wav" );
	PRECACHE_SOUND( "items/inv3.wav" );
}

void CItemPowerupInvisibility::Spawn()
{
	Precache();
	CItemPowerup::Spawn();
	if ( !m_flRespawnTime )
		m_flRespawnTime = 300;
	invisible_finished = 30;
	SET_MODEL( edict(), "models/invis.mdl" );
	pev->netname = MAKE_STRING( "Ring of Shadows" );
	pev->noise = MAKE_STRING( "items/inv1.wav" );
	items = IT_INVISIBILITY;
}

void CItemPowerupQuad::Precache()
{
	PRECACHE_MODEL( "models/quad.mdl" );
	PRECACHE_SOUND( "items/damage.wav" );
	PRECACHE_SOUND( "items/damage2.wav" );
	PRECACHE_SOUND( "items/damage3.wav" );
}

void CItemPowerupQuad::Spawn()
{
	Precache();
	CItemPowerup::Spawn();
	if ( !m_flRespawnTime )
		m_flRespawnTime = 60;
	super_damage_finished = 30;
	SET_MODEL( edict(), "models/quad.mdl" );
	pev->netname = MAKE_STRING( "Quad Damage" );
	pev->noise = MAKE_STRING( "items/damage.wav" );
	items = IT_QUAD;
}
