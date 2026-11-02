#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "gamerules.h"
#include "ofctf.h"
#include "ofctfitems.h"
#include "ctf_gamerules.h"

extern int gmsgItemPickup;
extern int gmsgPlayerIcon;
extern int gmsgCustomIcon;
extern cvar_t multipower;

CBaseEntity* CItemCTF::m_pLastSpawn;

class CItemSpawnCTF : public CPointEntity
{
public:
	void KeyValue( KeyValueData* pkvd );
	int team_no;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

class CItemLongJumpCTF : public CItemCTF
{
public:
	void Spawn();
	void Precache();
	int Classify() { return CLASS_CTFITEM; }
	void RemoveEffect( CBasePlayer* pPlayer );
	BOOL MyTouch( CBasePlayer* pPlayer );
};

class CItemPortableHEVCTF : public CItemCTF
{
public:
	void Spawn();
	void Precache();
	int Classify() { return CLASS_CTFITEM; }
	void RemoveEffect( CBasePlayer* pPlayer );
	BOOL MyTouch( CBasePlayer* pPlayer );
};

class CItemRegenerationCTF : public CItemCTF
{
public:
	void Spawn();
	void Precache();
	int Classify() { return CLASS_CTFITEM; }
	void RemoveEffect( CBasePlayer* pPlayer );
	BOOL MyTouch( CBasePlayer* pPlayer );
};

class CItemAcceleratorCTF : public CItemCTF
{
public:
	void Spawn();
	void Precache();
	int Classify() { return CLASS_CTFITEM; }
	void RemoveEffect( CBasePlayer* pPlayer );
	BOOL MyTouch( CBasePlayer* pPlayer );
};

class CItemBackpackCTF : public CItemCTF
{
public:
	void Spawn();
	void Precache();
	int Classify() { return CLASS_CTFITEM; }
	void RemoveEffect( CBasePlayer* pPlayer );
	BOOL MyTouch( CBasePlayer* pPlayer );
};

LINK_ENTITY_TO_CLASS( info_ctfspawn_powerup, CItemSpawnCTF );
LINK_ENTITY_TO_CLASS( item_ctflongjump, CItemLongJumpCTF );
LINK_ENTITY_TO_CLASS( item_ctfportablehev, CItemPortableHEVCTF );
LINK_ENTITY_TO_CLASS( item_ctfregeneration, CItemRegenerationCTF );
LINK_ENTITY_TO_CLASS( item_ctfaccelerator, CItemAcceleratorCTF );
LINK_ENTITY_TO_CLASS( item_ctfbackpack, CItemBackpackCTF );

void CItemSpawnCTF::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "team_no" ) )
	{
		team_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CItemCTF::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "team_no" ) )
	{
		team_no = atoi( pkvd->szValue );
		pkvd->fHandled = TRUE;
	}
	else
		pkvd->fHandled = FALSE;
}

void CItemCTF::Precache()
{
	if ( pev->model )
		PRECACHE_MODEL( (char*)STRING( pev->model ) );
	PRECACHE_SOUND( "ctf/itemthrow.wav" );
	PRECACHE_SOUND( "items/ammopickup1.wav" );
}

void CItemCTF::Spawn()
{
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 48 ) );
	SetTouch( &CItemCTF::ItemTouch );
	if ( !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
		return;
	}
	if ( pev->model )
	{
		SET_MODEL( edict(), STRING( pev->model ) );
		pev->sequence = LookupSequence( "idle" );
		if ( pev->sequence != -1 )
		{
			ResetSequenceInfo();
			pev->frame = 0;
		}
	}
	if ( pev->spawnflags & 4 )
	{
		SetThink( &CItemCTF::DropThink );
		pev->nextthink = gpGlobals->time + 0.1;
	}
	m_iLastTouched = 0;
	m_flPickupTime = 0;
	SetTouch( &CItemCTF::ItemTouch );
	m_flNextTouchTime = 0;
	m_pszItemName = "";
}

void CItemCTF::SetObjectCollisionBox()
{
	pev->absmin = pev->origin + Vector( -16, -16, 0 );
	pev->absmax = pev->origin + Vector( 16, 16, 48 );
}

void CItemCTF::ItemTouch( CBaseEntity* pOther )
{
	if ( !pOther->IsPlayer() || !pOther->IsAlive() ||
		( m_iLastTouched == pOther->entindex() && m_flNextTouchTime > gpGlobals->time ) )
		return;
	m_iLastTouched = 0;
	if ( !MyTouch( (CBasePlayer*)pOther ) )
		return;
	SUB_UseTargets( pOther, USE_TOGGLE, 0 );
	SetTouch( NULL );
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->effects |= EF_NODRAW;
	UTIL_SetOrigin( pev, pev->origin );
	pev->owner = pOther->edict();
	SetThink( &CItemCTF::CarryThink );
	pev->nextthink = gpGlobals->time + 20;
	m_flPickupTime = gpGlobals->time;
	UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"take_%s_Powerup\"\n", STRING( pOther->pev->netname ),
		GETPLAYERUSERID( pOther->edict() ), g_engfuncs.pfnGetPlayerWONId( pOther->edict() ),
		GetTeamName( pOther->edict() ), m_pszItemName );
}

void CItemCTF::CarryThink()
{
	CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance( pev->owner );
	if ( pPlayer && pPlayer->IsPlayer() )
	{
		if ( m_iItemFlag & pPlayer->m_iItems )
			pev->nextthink = gpGlobals->time + 20;
		else
		{
			pPlayer->m_iClientItems = m_iItemFlag;
			DropItem( pPlayer, TRUE );
		}
	}
	else
		DropItem( NULL, TRUE );
}

void CItemCTF::DropPreThink()
{
	int contents = UTIL_PointContents( pev->origin );
	if ( contents == CONTENTS_SLIME || contents == CONTENTS_LAVA ||
		contents == CONTENTS_SOLID || contents == CONTENTS_SKY )
	{
		DropThink();
		return;
	}
	SetThink( &CItemCTF::DropThink );
	pev->nextthink = gpGlobals->time + max( 0.0, g_flPowerupRespawnTime - 5.0 );
}

void CItemCTF::DropThink()
{
	SetThink( NULL );
	pev->origin = pev->oldorigin;
	CItemSpawnCTF* pSpawn = NULL;
	int tested = 0;
	BOOL exhausted = FALSE;
	BOOL selected = TRUE;
	if ( !( pev->spawnflags & 8 ) )
	{
		for ( int pass = 0; pass < 2; ++pass )
		{
			pSpawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname( pass ? NULL : m_pLastSpawn, "info_ctfspawn_powerup" );
			int losingTeam, scoreDiff;
			GetLosingTeam( losingTeam, scoreDiff );
			while ( pSpawn )
			{
				BOOL test = scoreDiff == 0;
				if ( scoreDiff == 1 )
					test = ( RANDOM_LONG( 0, 1 ) && pSpawn->team_no == 0 ) ||
						( RANDOM_LONG( 0, 1 ) && pSpawn->team_no == losingTeam + 1 );
				else if ( scoreDiff > 1 )
					test = pSpawn->team_no == losingTeam + 1;
				if ( test )
				{
					++tested;
					BOOL occupied = FALSE;
					CBaseEntity* pEntity = NULL;
					while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pSpawn->pev->origin, 128 ) ) != NULL )
					{
						if ( pEntity->Classify() == CLASS_CTFITEM && pEntity != this )
							occupied = TRUE;
					}
					if ( !occupied )
					{
						pev->origin = pSpawn->pev->origin;
						if ( RANDOM_LONG( 0, 1 ) )
							break;
					}
				}
				pSpawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname( pSpawn, "info_ctfspawn_powerup" );
			}
			exhausted = pSpawn == NULL;
			selected = !exhausted;
			if ( pSpawn == m_pLastSpawn || !exhausted )
				break;
		}
	}
	if ( pev->origin == pev->oldorigin && !selected )
	{
		pSpawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname( NULL, "info_ctfspawn_powerup" );
		while ( pSpawn )
		{
			BOOL occupied = FALSE;
			CBaseEntity* pEntity = NULL;
			while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pSpawn->pev->origin, 128 ) ) != NULL )
			{
				if ( pEntity->Classify() == CLASS_CTFITEM && pEntity != this )
					occupied = TRUE;
			}
			if ( !occupied )
			{
				pev->origin = pSpawn->pev->origin;
				if ( RANDOM_LONG( 0, 1 ) )
					break;
			}
			pSpawn = (CItemSpawnCTF*)UTIL_FindEntityByClassname( pSpawn, "info_ctfspawn_powerup" );
		}
		exhausted = pSpawn == NULL;
	}
	m_pLastSpawn = pSpawn;
	if ( exhausted && tested > 0 )
		ALERT( at_console, "Warning: No available spawn points found.  Powerup returned to original coordinates." );
	UTIL_SetOrigin( pev, pev->origin );
	if ( !DROP_TO_FLOOR( edict() ) )
	{
		ALERT( at_error, "Item %s fell out of level at %f,%f,%f", STRING( pev->classname ),
			pev->origin.x, pev->origin.y, pev->origin.z );
		UTIL_Remove( this );
	}
}

void CItemCTF::DropItem( CBasePlayer* pPlayer, int nForceRespawn )
{
	if ( pPlayer )
	{
		RemoveEffect( pPlayer );
		UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n", STRING( pPlayer->pev->netname ),
			GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
			GetTeamName( pPlayer->edict() ), m_pszItemName );
		pev->origin = pPlayer->pev->origin + Vector( 0, 0, ( pPlayer->pev->flags & FL_DUCKING ) ? 34 : 16 );
	}
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->velocity = Vector( 0, 0, -100 );
	if ( pev->owner )
	{
		CBasePlayer* pOwner = (CBasePlayer*)GET_PRIVATE( pev->owner );
		if ( pOwner && pOwner->IsPlayer() )
			pOwner->m_iItems &= ~m_iItemFlag;
		pev->owner = NULL;
	}
	SetTouch( &CItemCTF::ItemTouch );
	if ( nForceRespawn )
		DropThink();
	else
	{
		SetThink( &CItemCTF::DropPreThink );
		pev->nextthink = gpGlobals->time + 5;
	}
}

void CItemCTF::ThrowItem( CBasePlayer* pPlayer )
{
	if ( !pPlayer )
		return;
	if ( pPlayer )
	{
		RemoveEffect( pPlayer );
		pev->origin = pPlayer->pev->origin + Vector( 0, 0, ( pPlayer->pev->flags & FL_DUCKING ) ? 34 : 16 );
	}
	UTIL_SetOrigin( pev, pev->origin );
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "ctf/itemthrow.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	pev->velocity = gpGlobals->v_forward * 274 + pPlayer->pev->velocity;
	pev->avelocity.y = 400;
	if ( pev->owner )
	{
		CBasePlayer* pOwner = (CBasePlayer*)GET_PRIVATE( pev->owner );
		if ( pOwner && pOwner->IsPlayer() )
			pOwner->m_iItems &= ~m_iItemFlag;
		pev->owner = NULL;
	}
	SetTouch( &CItemCTF::ItemTouch );
	SetThink( &CItemCTF::DropPreThink );
	pev->nextthink = gpGlobals->time + 5;
	m_iLastTouched = pPlayer->entindex();
	m_flNextTouchTime = gpGlobals->time + 5;
	UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n", STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
		GetTeamName( pPlayer->edict() ), m_pszItemName );
}

void CItemCTF::ScatterItem( CBasePlayer* pPlayer )
{
	if ( !pPlayer )
		return;
	if ( pPlayer )
	{
		RemoveEffect( pPlayer );
		pev->origin = pPlayer->pev->origin + Vector( 0, 0, ( pPlayer->pev->flags & FL_DUCKING ) ? 34 : 16 );
	}
	UTIL_SetOrigin( pev, pev->origin );
	pev->flags &= ~FL_ONGROUND;
	pev->movetype = MOVETYPE_TOSS;
	pev->angles = g_vecZero;
	pev->solid = SOLID_TRIGGER;
	pev->effects &= ~EF_NODRAW;
	float y = RANDOM_FLOAT( 0, 1 );
	float x = RANDOM_FLOAT( 0, 1 );
	pev->velocity = Vector( x * 274 + pPlayer->pev->velocity.x, y * 274 + pPlayer->pev->velocity.y,
		pPlayer->pev->velocity.z );
	pev->avelocity.y = 400;
	if ( pev->owner )
	{
		CBasePlayer* pOwner = (CBasePlayer*)GET_PRIVATE( pev->owner );
		if ( pOwner && pOwner->IsPlayer() )
			pOwner->m_iItems &= ~m_iItemFlag;
		pev->owner = NULL;
	}
	SetTouch( &CItemCTF::ItemTouch );
	SetThink( &CItemCTF::DropPreThink );
	pev->nextthink = gpGlobals->time + 5;
	m_iLastTouched = pPlayer->entindex();
	m_flNextTouchTime = gpGlobals->time + 5;
	UTIL_LogPrintf( "\"%s<%i><%u><%s>\" triggered \"drop_%s_Powerup\"\n", STRING( pPlayer->pev->netname ),
		GETPLAYERUSERID( pPlayer->edict() ), g_engfuncs.pfnGetPlayerWONId( pPlayer->edict() ),
		GetTeamName( pPlayer->edict() ), m_pszItemName );
}

void CItemLongJumpCTF::Precache()
{
	PRECACHE_MODEL( "models/w_jumppack.mdl" );
	PRECACHE_SOUND( "ctf/pow_big_jump.wav" );
}

void CItemLongJumpCTF::Spawn()
{
	CItemCTF::Precache();
	Precache();
	SET_MODEL( edict(), "models/w_jumppack.mdl" );
	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;
	CItemCTF::Spawn();
	m_iItemFlag = CTF_ITEM_JUMP;
	m_pszItemName = "Jump";
}

void CItemLongJumpCTF::RemoveEffect( CBasePlayer* pPlayer )
{
	pPlayer->m_fLongJump = FALSE;
	g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "jpj", "0" );
	pPlayer->m_flJumpTime = gpGlobals->time - m_flPickupTime + pPlayer->m_flJumpTime;
}

BOOL CItemLongJumpCTF::MyTouch( CBasePlayer* pPlayer )
{
	if ( ( pPlayer->m_iItems & CTF_ITEM_JUMP ) ||
		( multipower.value == 0 && ( pPlayer->m_iItems & CTF_POWERUP_MASK ) ) ||
		( team_no > 0 && team_no != pPlayer->m_iTeamNum ) || !( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	pPlayer->m_fLongJump = TRUE;
	g_engfuncs.pfnSetPhysicsKeyValue( pPlayer->edict(), "jpj", "1" );
	pPlayer->m_iItems |= CTF_ITEM_JUMP;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/ammopickup1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	return TRUE;
}

void CItemPortableHEVCTF::Precache()
{
	PRECACHE_MODEL( "models/w_porthev.mdl" );
	PRECACHE_SOUND( "ctf/pow_armor_charge.wav" );
}

void CItemPortableHEVCTF::Spawn()
{
	CItemCTF::Precache();
	Precache();
	SET_MODEL( edict(), "models/w_porthev.mdl" );
	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;
	CItemCTF::Spawn();
	m_iItemFlag = CTF_ITEM_SHIELD;
	m_pszItemName = "Shield";
}

void CItemPortableHEVCTF::RemoveEffect( CBasePlayer* pPlayer )
{
	pPlayer->m_flShieldTime = gpGlobals->time - m_flPickupTime + pPlayer->m_flShieldTime;
}

BOOL CItemPortableHEVCTF::MyTouch( CBasePlayer* pPlayer )
{
	if ( ( pPlayer->m_iItems & CTF_ITEM_SHIELD ) ||
		( multipower.value == 0 && ( pPlayer->m_iItems & CTF_POWERUP_MASK ) ) ||
		( team_no > 0 && team_no != pPlayer->m_iTeamNum ) || !( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	pPlayer->m_iItems |= CTF_ITEM_SHIELD;
	pPlayer->m_fPlayingAChargeSound = FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/ammopickup1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	return TRUE;
}

void CItemRegenerationCTF::Precache()
{
	PRECACHE_MODEL( "models/w_health.mdl" );
	PRECACHE_SOUND( "ctf/pow_health_charge.wav" );
}

void CItemRegenerationCTF::Spawn()
{
	CItemCTF::Precache();
	Precache();
	SET_MODEL( edict(), "models/w_health.mdl" );
	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;
	CItemCTF::Spawn();
	m_iItemFlag = CTF_ITEM_REGENERATION;
	m_pszItemName = "Health";
}

void CItemRegenerationCTF::RemoveEffect( CBasePlayer* pPlayer )
{
	pPlayer->m_flHealthTime = gpGlobals->time - m_flPickupTime + pPlayer->m_flHealthTime;
}

BOOL CItemRegenerationCTF::MyTouch( CBasePlayer* pPlayer )
{
	if ( ( pPlayer->m_iItems & CTF_ITEM_REGENERATION ) ||
		( multipower.value == 0 && ( pPlayer->m_iItems & CTF_POWERUP_MASK ) ) ||
		( team_no > 0 && team_no != pPlayer->m_iTeamNum ) || !( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	pPlayer->m_iItems |= CTF_ITEM_REGENERATION;
	pPlayer->m_fPlayingHChargeSound = FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/ammopickup1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	if ( pPlayer->pev->health < 100 )
		pPlayer->TakeHealth( gSkillData.healthkitCapacity, DMG_GENERIC );
	return TRUE;
}

void CItemAcceleratorCTF::Precache()
{
	PRECACHE_MODEL( "models/w_accelerator.mdl" );
	PRECACHE_SOUND( "turret/tu_ping.wav" );
}

void CItemAcceleratorCTF::Spawn()
{
	CItemCTF::Precache();
	Precache();
	SET_MODEL( edict(), "models/w_accelerator.mdl" );
	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;
	CItemCTF::Spawn();
	m_iItemFlag = CTF_ITEM_ACCELERATOR;
	m_pszItemName = "Damage";
}

void CItemAcceleratorCTF::RemoveEffect( CBasePlayer* pPlayer )
{
	pPlayer->m_flAccelTime = gpGlobals->time - m_flPickupTime + pPlayer->m_flAccelTime;
}

BOOL CItemAcceleratorCTF::MyTouch( CBasePlayer* pPlayer )
{
	if ( ( pPlayer->m_iItems & CTF_ITEM_ACCELERATOR ) ||
		( multipower.value == 0 && ( pPlayer->m_iItems & CTF_POWERUP_MASK ) ) ||
		( team_no > 0 && team_no != pPlayer->m_iTeamNum ) || !( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	pPlayer->m_iItems |= CTF_ITEM_ACCELERATOR;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/ammopickup1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	return TRUE;
}

void CItemBackpackCTF::Precache()
{
	PRECACHE_MODEL( "models/w_backpack.mdl" );
	PRECACHE_SOUND( "ctf/pow_backpack.wav" );
}

void CItemBackpackCTF::Spawn()
{
	CItemCTF::Precache();
	Precache();
	SET_MODEL( edict(), "models/w_backpack.mdl" );
	pev->spawnflags |= SF_NORESPAWN;
	pev->oldorigin = pev->origin;
	CItemCTF::Spawn();
	m_iItemFlag = CTF_ITEM_BACKPACK;
	m_pszItemName = "Ammo";
}

void CItemBackpackCTF::RemoveEffect( CBasePlayer* pPlayer )
{
	pPlayer->m_flBackpackTime = gpGlobals->time - m_flPickupTime + pPlayer->m_flBackpackTime;
}

BOOL CItemBackpackCTF::MyTouch( CBasePlayer* pPlayer )
{
	if ( ( pPlayer->m_iItems & CTF_ITEM_BACKPACK ) ||
		( multipower.value == 0 && ( pPlayer->m_iItems & CTF_POWERUP_MASK ) ) ||
		( team_no > 0 && team_no != pPlayer->m_iTeamNum ) || !( pPlayer->pev->weapons & ( 1 << WEAPON_SUIT ) ) )
		return FALSE;
	pPlayer->m_iItems |= CTF_ITEM_BACKPACK;
	MESSAGE_BEGIN( MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev );
		WRITE_STRING( STRING( pev->classname ) );
	MESSAGE_END();
	EMIT_SOUND_DYN( edict(), CHAN_VOICE, "items/ammopickup1.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	pPlayer->GiveAmmo( 20, "uranium", 100 );
	pPlayer->GiveAmmo( 17, "9mm", 250 );
	pPlayer->GiveAmmo( 6, "357", 36 );
	pPlayer->GiveAmmo( 12, "buckshot", 125 );
	pPlayer->GiveAmmo( 5, "bolts", 50 );
	pPlayer->GiveAmmo( 1, "rockets", 5 );
	pPlayer->GiveAmmo( 5, "Hand Grenade", 10 );
	pPlayer->GiveAmmo( 5, "Snarks", 15 );
	pPlayer->GiveAmmo( 5, "spores", 20 );
	pPlayer->GiveAmmo( 5, "762", 15 );
	pPlayer->GiveAmmo( 200, "556", 200 );
	return TRUE;
}

void RespawnPlayerCTFPowerups( CBasePlayer* pPlayer, int nForceRespawn )
{
	if ( !pPlayer || !( pPlayer->m_iItems & CTF_POWERUP_MASK ) )
		return;
	const unsigned int bits[] = { CTF_ITEM_JUMP, CTF_ITEM_SHIELD, CTF_ITEM_REGENERATION, CTF_ITEM_BACKPACK, CTF_ITEM_ACCELERATOR };
	const char* names[] = { "item_ctflongjump", "item_ctfportablehev", "item_ctfregeneration", "item_ctfbackpack", "item_ctfaccelerator" };
	for ( int i = 0; i < ARRAYSIZE( bits ); ++i )
	{
		if ( !( pPlayer->m_iItems & bits[i] ) )
			continue;
		CItemCTF* pItem = NULL;
		while ( ( pItem = (CItemCTF*)UTIL_FindEntityByClassname( pItem, names[i] ) ) != NULL )
		{
			if ( !pItem->team_no || pItem->team_no == pPlayer->m_iTeamNum )
			{
				pItem->DropItem( pPlayer, nForceRespawn );
				break;
			}
		}
		if ( bits[i] == CTF_ITEM_SHIELD && pPlayer->m_fPlayingAChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_armor_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingAChargeSound = FALSE;
		}
		if ( bits[i] == CTF_ITEM_REGENERATION && pPlayer->m_fPlayingHChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_health_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingHChargeSound = FALSE;
		}
	}
	pPlayer->m_iItems &= ~CTF_POWERUP_MASK;
}

void ScatterPlayerCTFPowerups( CBasePlayer* pPlayer )
{
	if ( !pPlayer || !( pPlayer->m_iItems & CTF_POWERUP_MASK ) )
		return;
	const unsigned int bits[] = { CTF_ITEM_JUMP, CTF_ITEM_SHIELD, CTF_ITEM_REGENERATION, CTF_ITEM_BACKPACK, CTF_ITEM_ACCELERATOR };
	const char* names[] = { "item_ctflongjump", "item_ctfportablehev", "item_ctfregeneration", "item_ctfbackpack", "item_ctfaccelerator" };
	for ( int i = 0; i < ARRAYSIZE( bits ); ++i )
	{
		if ( !( pPlayer->m_iItems & bits[i] ) )
			continue;
		CItemCTF* pItem = NULL;
		while ( ( pItem = (CItemCTF*)UTIL_FindEntityByClassname( pItem, names[i] ) ) != NULL )
		{
			if ( !pItem->team_no || pItem->team_no == pPlayer->m_iTeamNum )
			{
				pItem->ScatterItem( pPlayer );
				break;
			}
		}
		if ( bits[i] == CTF_ITEM_SHIELD && pPlayer->m_fPlayingAChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_armor_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingAChargeSound = FALSE;
		}
		if ( bits[i] == CTF_ITEM_REGENERATION && pPlayer->m_fPlayingHChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_health_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingHChargeSound = FALSE;
		}
	}
	pPlayer->m_iItems &= ~CTF_POWERUP_MASK;
}

void DropPlayerCTFPowerup( CBasePlayer* pPlayer )
{
	if ( !( pPlayer->m_iItems & CTF_POWERUP_MASK ) )
		return;
	const unsigned int bits[] = { CTF_ITEM_JUMP, CTF_ITEM_SHIELD, CTF_ITEM_REGENERATION, CTF_ITEM_BACKPACK, CTF_ITEM_ACCELERATOR };
	const char* names[] = { "item_ctflongjump", "item_ctfportablehev", "item_ctfregeneration", "item_ctfbackpack", "item_ctfaccelerator" };
	for ( int i = 0; i < ARRAYSIZE( bits ); ++i )
	{
		if ( !( pPlayer->m_iItems & bits[i] ) )
			continue;
		CItemCTF* pItem = NULL;
		while ( ( pItem = (CItemCTF*)UTIL_FindEntityByClassname( pItem, names[i] ) ) != NULL )
		{
			if ( !pItem->team_no || pItem->team_no == pPlayer->m_iTeamNum )
			{
				pItem->ThrowItem( pPlayer );
				break;
			}
		}
		if ( bits[i] == CTF_ITEM_SHIELD && pPlayer->m_fPlayingAChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_armor_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingAChargeSound = FALSE;
		}
		if ( bits[i] == CTF_ITEM_REGENERATION && pPlayer->m_fPlayingHChargeSound )
		{
			EMIT_SOUND_DYN( pPlayer->edict(), CHAN_STATIC, "ctf/pow_health_charge.wav", 0, ATTN_NONE, SND_STOP, PITCH_NORM );
			pPlayer->m_fPlayingHChargeSound = FALSE;
		}
		pPlayer->m_iItems &= ~bits[i];
		return;
	}
}

void InitItemsForPlayer( CBasePlayer* pPlayer )
{
	if ( !gmsgPlayerIcon )
		return;
	const unsigned int bits[] = { CTF_FLAG_BM, CTF_FLAG_OF, CTF_ITEM_JUMP, CTF_ITEM_SHIELD, CTF_ITEM_REGENERATION, CTF_ITEM_ACCELERATOR, CTF_ITEM_BACKPACK };
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CBasePlayer* pClient = (CBasePlayer*)UTIL_PlayerByIndex( i );
		if ( !pClient )
			continue;
		for ( int item = 0; item < ARRAYSIZE( bits ); ++item )
		{
			if ( !( pClient->m_iItems & bits[item] ) )
				continue;
			MESSAGE_BEGIN( MSG_ONE, gmsgPlayerIcon, NULL, pPlayer->pev );
				WRITE_BYTE( pClient->entindex() );
				WRITE_BYTE( 1 );
				WRITE_BYTE( item < 2 ? 1 : 0 );
				WRITE_BYTE( bits[item] );
			MESSAGE_END();
		}
	}
}

void FlushCTFPowerupTimes()
{
	const char* names[] = { "item_ctflongjump", "item_ctfportablehev", "item_ctfregeneration", "item_ctfbackpack", "item_ctfaccelerator" };
	float CBasePlayer::* times[] = { &CBasePlayer::m_flJumpTime, &CBasePlayer::m_flShieldTime,
		&CBasePlayer::m_flHealthTime, &CBasePlayer::m_flBackpackTime, &CBasePlayer::m_flAccelTime };
	for ( int i = 0; i < ARRAYSIZE( names ); ++i )
	{
		CItemCTF* pItem = NULL;
		while ( ( pItem = (CItemCTF*)UTIL_FindEntityByClassname( pItem, names[i] ) ) != NULL )
		{
			if ( !pItem->pev->owner )
				continue;
			CBasePlayer* pPlayer = (CBasePlayer*)GET_PRIVATE( pItem->pev->owner );
			if ( pPlayer && pPlayer->IsPlayer() )
				pPlayer->*times[i] = gpGlobals->time - pItem->m_flPickupTime + pPlayer->*times[i];
		}
	}
}

class CHUDIconTrigger : public CBaseToggle
{
public:
	void Spawn();
	void KeyValue( KeyValueData* pkvd );
	void Use( CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value );
	void UpdateUser( CBaseEntity* pPlayer );

	float	m_flNextActiveTime;
	int		m_nCustomIndex;
	int		m_iszCustomName;
	bool	m_fIsActive;
	int		m_nLeft;
	int		m_nTop;
	int		m_nWidth;
	int		m_nHeight;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( ctf_hudicon, CHUDIconTrigger );

void CHUDIconTrigger::Spawn()
{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL( edict(), STRING( pev->model ) );
	pev->movetype = MOVETYPE_NONE;
	m_fIsActive = false;
}

void CHUDIconTrigger::KeyValue( KeyValueData* pkvd )
{
	if ( FStrEq( pkvd->szKeyName, "icon_name" ) )
		m_iszCustomName = ALLOC_STRING( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "icon_index" ) )
		m_nCustomIndex = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "icon_left" ) )
		m_nLeft = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "icon_top" ) )
		m_nTop = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "icon_width" ) )
		m_nWidth = atoi( pkvd->szValue );
	else if ( FStrEq( pkvd->szKeyName, "icon_height" ) )
		m_nHeight = atoi( pkvd->szValue );
	else
	{
		pkvd->fHandled = FALSE;
		return;
	}
	pkvd->fHandled = TRUE;
}

void CHUDIconTrigger::Use( CBaseEntity* pActivator, CBaseEntity*, USE_TYPE useType, float )
{
	if ( m_flNextActiveTime > gpGlobals->time || !UTIL_IsMasterTriggered( m_sMaster, pActivator ) )
		return;
	if ( pev->noise )
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, STRING( pev->noise ), 1, ATTN_NORM, 0, PITCH_NORM );
	m_hActivator = pActivator;
	SUB_UseTargets( m_hActivator, USE_TOGGLE, 0 );
	if ( pev->message && pActivator->IsPlayer() )
		UTIL_ShowMessage( STRING( pev->message ), pActivator );
	if ( useType == USE_ON )
		m_fIsActive = true;
	else if ( useType == USE_OFF )
		m_fIsActive = false;
	else
		m_fIsActive = !m_fIsActive;
	MESSAGE_BEGIN( MSG_ALL, gmsgCustomIcon );
		WRITE_BYTE( m_fIsActive );
		WRITE_BYTE( m_nCustomIndex );
		if ( m_fIsActive )
		{
			WRITE_STRING( STRING( m_iszCustomName ) );
			WRITE_BYTE( (int)pev->rendercolor.x );
			WRITE_BYTE( (int)pev->rendercolor.y );
			WRITE_BYTE( (int)pev->rendercolor.z );
			WRITE_BYTE( m_nLeft );
			WRITE_BYTE( m_nTop );
			WRITE_BYTE( m_nLeft + m_nWidth );
			WRITE_BYTE( m_nTop + m_nHeight );
		}
	MESSAGE_END();
	m_flNextActiveTime = gpGlobals->time + m_flWait;
	if ( m_fIsActive )
	{
		CHUDIconTrigger* pIcon = NULL;
		while ( ( pIcon = (CHUDIconTrigger*)UTIL_FindEntityByClassname( pIcon, "ctf_hudicon" ) ) != NULL )
		{
			if ( pIcon != this && pIcon->m_nCustomIndex == m_nCustomIndex )
				pIcon->m_fIsActive = false;
		}
	}
}

void CHUDIconTrigger::UpdateUser( CBaseEntity* pPlayer )
{
	MESSAGE_BEGIN( MSG_ONE, gmsgCustomIcon, NULL, pPlayer->pev );
		WRITE_BYTE( m_fIsActive );
		WRITE_BYTE( m_nCustomIndex );
		if ( m_fIsActive )
		{
			WRITE_STRING( STRING( m_iszCustomName ) );
			WRITE_BYTE( (int)pev->rendercolor.x );
			WRITE_BYTE( (int)pev->rendercolor.y );
			WRITE_BYTE( (int)pev->rendercolor.z );
			WRITE_BYTE( m_nLeft );
			WRITE_BYTE( m_nTop );
			WRITE_BYTE( m_nLeft + m_nWidth );
			WRITE_BYTE( m_nTop + m_nHeight );
		}
	MESSAGE_END();
}

void RefreshCustomHUD( CBasePlayer* pPlayer )
{
	CHUDIconTrigger* pIcon = NULL;
	int active = 0;
	while ( ( pIcon = (CHUDIconTrigger*)UTIL_FindEntityByClassname( pIcon, "ctf_hudicon" ) ) != NULL )
	{
		if ( ( active >> pIcon->m_nCustomIndex ) & 1 )
			continue;
		if ( pIcon->m_fIsActive )
			active |= 1 << pIcon->m_nCustomIndex;
		pIcon->UpdateUser( pPlayer );
	}
}
