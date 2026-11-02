#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"

void UpdateEntityEvents( CBaseEntity* pEntity );
void DoDamageEffects( entvars_t* pev, float damageLevelOne, float damageLevelTwo );

class CDispenserRefillThinker : public CBaseEntity
{
public:
	void Spawn() override;
	void Think() override;
	void StartRefillingPlayer( CBaseEntity* pPlayer, CTFDispenser* pDispenser, float flStartTime );

	EHANDLE	m_hDispenser;
	EHANDLE	m_hPlayer;
	Vector	m_InitialUsePoint;
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
};

LINK_ENTITY_TO_CLASS( building_dispenser, CTFDispenser );
LINK_ENTITY_TO_CLASS( dispenser_refill_timer, CDispenserRefillThinker );

void CTFDispenser::Precache()
{
	PRECACHE_MODEL( "models/dispenser.mdl" );
	m_iShardIndex = PRECACHE_MODEL( "models/computergibs.mdl" );
	m_usBuildingEvent = PRECACHE_EVENT( 1, "events/misc/tf_buildingevent.sc" );
}

void CTFDispenser::Spawn()
{
	Precache();
	pev->classname = MAKE_STRING( "building_dispenser" );
	pev->solid = SOLID_BBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->flags &= ~FL_ONGROUND;
	pev->velocity = Vector( 0, 0, 8 );
	pev->max_health = 150;
	pev->health = pev->max_health;
	pev->takedamage = DAMAGE_NO;
	pev->flags |= FL_MONSTER;
	SET_MODEL( edict(), "models/dispenser.mdl" );
	UTIL_SetSize( pev, Vector( -16, -16, 0 ), Vector( 16, 16, 24 ) );
}

CTFDispenser* CTFDispenser::CreateDispenser( Vector vecOrigin, Vector vecAngles, CBaseEntity* pOwner )
{
	(void)vecAngles;
	CTFDispenser* pDispenser = GetClassPtr( (CTFDispenser*)NULL );
	UTIL_SetOrigin( pDispenser->pev, vecOrigin );
	pDispenser->pev->angles = g_vecZero;
	pDispenser->pev->angles.y = UTIL_AngleMod( pOwner->pev->angles.y + 180 );
	pDispenser->Spawn();
	pDispenser->real_owner = pOwner;
	pDispenser->team_no = pOwner->team_no;
	pDispenser->m_iDestroyed = 0;
	pDispenser->m_flNextUseTime = gpGlobals->time + 3;
	return pDispenser;
}

void CTFDispenser::Finished()
{
	CBaseEntity* pOwner = real_owner;
	pOwner->has_dispenser = 1;
	ClientPrint( pOwner->pev, HUD_PRINTNOTIFY, "#Dispenser_finish" );
	teamsprint( team_no, pOwner, HUD_PRINTNOTIFY, "#Dispenser_built", STRING( pOwner->pev->netname ), NULL, NULL );
	const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	const char* authid = GETPLAYERAUTHID( pOwner->edict() );
	int userid = GETPLAYERUSERID( pOwner->edict() );
	UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Built_Dispenser\"\n",
		STRING( pOwner->pev->netname ), userid, authid, team );
	pOwner->ammo_cells -= 100;
	pev->takedamage = DAMAGE_AIM;
	SetTouch( &CTFDispenser::DispenserTouch );
	SetThink( &CTFDispenser::DispenserThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextRefillTime = gpGlobals->time + 12;
	pev->movetype = MOVETYPE_TOSS;
	pev->velocity = Vector( 0, 0, 8 );
	pev->flags &= ~FL_ONGROUND;
	ammo_shells = (int)ceil( pOwner->ammo_shells * 0.25 );
	ammo_nails = (int)ceil( pOwner->ammo_nails * 0.25 );
	ammo_rockets = (int)ceil( pOwner->ammo_rockets * 0.25 );
	ammo_cells = (int)ceil( pOwner->ammo_cells * 0.25 );
	pev->armorvalue = (float)ceil( pOwner->pev->armorvalue * 0.25 );
	pOwner->ammo_shells = (int)ceil( ammo_shells * 0.75 );
	pOwner->ammo_nails = (int)ceil( ammo_nails * 0.75 );
	pOwner->ammo_rockets = (int)ceil( ammo_rockets * 0.75 );
	pOwner->ammo_cells = (int)ceil( ammo_cells * 0.75 );
	pOwner->pev->armorvalue = (float)ceil( pev->armorvalue * 0.75 );
	m_vOldOrigin = pev->origin;
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/turrset.wav", 1, ATTN_NORM, 0, PITCH_NORM );
}

void CTFDispenser::DispenserThink()
{
	CheckBelowBuilding( 24 );
	UpdateEntityEvents( this );
	float damageLevelTwo = pev->max_health * 0.33;
	float damageLevelOne = pev->max_health * 0.66;
	DoDamageEffects( pev, damageLevelOne, damageLevelTwo );
	if ( gpGlobals->time > m_flNextRefillTime )
	{
		ammo_shells += 20;
		ammo_cells += 20;
		ammo_nails += 30;
		ammo_rockets += 15;
		pev->armorvalue += 50;
		if ( ammo_shells > 400 ) ammo_shells = 400;
		if ( ammo_nails > 600 ) ammo_nails = 600;
		if ( ammo_rockets > 300 ) ammo_rockets = 300;
		if ( ammo_cells > 400 ) ammo_cells = 400;
		if ( pev->armorvalue > 500 ) pev->armorvalue = 500;
		m_flNextRefillTime = gpGlobals->time + 12;
	}
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTFDispenser::DispenserTouch( CBaseEntity* pOther )
{
	if ( !( pOther->pev->flags & FL_CLIENT ) )
		return;
	if ( pOther->team_no && pOther->team_no != team_no )
		ClientPrint( real_owner->pev, HUD_PRINTCENTER, "#Dispenser_used" );
	if ( gpGlobals->time < m_flNextUseTime )
		return;
	if ( !FindTimer( 14 ) )
	{
		CDispenserRefillThinker* pTimer = GetClassPtr( (CDispenserRefillThinker*)NULL );
		pTimer->StartRefillingPlayer( pOther, this, 0.2f );
	}
	m_flNextUseTime = gpGlobals->time + 1;
}

BOOL CTFDispenser::EngineerUse( CBasePlayer* pPlayer )
{
	m_flNextUseTime = gpGlobals->time + 3.0;
	float repair = ( pev->max_health - pev->health ) / 5;
	if ( repair != 0 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Menu_RepairDisp" );
		float cells = min( repair, (float)pPlayer->ammo_cells );
		pPlayer->ammo_cells = (int)( pPlayer->ammo_cells - cells );
		pev->health += cells * 5;
		return TRUE;
	}
	int shells = min( min( 40, pPlayer->ammo_shells ), 400 - ammo_shells );
	pPlayer->ammo_shells -= shells;
	ammo_shells += shells;
	int nails = min( min( 40, pPlayer->ammo_nails ), 600 - ammo_nails );
	pPlayer->ammo_nails -= nails;
	ammo_nails += nails;
	int rockets = min( min( 10, pPlayer->ammo_rockets ), 300 - ammo_rockets );
	pPlayer->ammo_rockets -= rockets;
	ammo_rockets += rockets;
	int cells = min( min( 40, pPlayer->ammo_cells ), 400 - ammo_cells );
	pPlayer->ammo_cells -= cells;
	ammo_cells += cells;
	ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY,
		shells + nails + rockets + cells > 0 ? "#Menu_AmmoDisp" : "#Dispenser_AmmoNoMore" );
	char* cellText = UTIL_dtos4( ammo_cells );
	char* armorText = UTIL_dtos3( (int)pev->armorvalue );
	char* rocketText = UTIL_dtos2( ammo_rockets );
	char* healthText = UTIL_dtos1( (int)pev->health );
	ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Dispenser_has", healthText, rocketText, armorText, cellText );
	return TRUE;
}

BOOL CTFDispenser::RefillPlayer( CBasePlayer* pPlayer )
{
	if ( !ammo_shells && !ammo_nails && !ammo_rockets && !ammo_cells && pev->armorvalue == 0 )
	{
		ClientPrint( pPlayer->pev, HUD_PRINTNOTIFY, "#Dispenser_empty" );
		return FALSE;
	}
	int shells = min( min( 20, ammo_shells ), pPlayer->maxammo_shells - pPlayer->ammo_shells );
	ammo_shells -= shells;
	int nails = min( min( 20, pPlayer->maxammo_nails - pPlayer->ammo_nails ), ammo_nails );
	ammo_nails -= nails;
	int rockets = min( min( 10, pPlayer->maxammo_rockets - pPlayer->ammo_rockets ), ammo_rockets );
	ammo_rockets -= rockets;
	int cells = min( min( pPlayer->pev->playerclass == 9 ? 75 : 10,
		pPlayer->maxammo_cells - pPlayer->ammo_cells ), ammo_cells );
	ammo_cells -= cells;
	pPlayer->GiveTFAmmo( shells, nails, rockets, cells );
	int armor = (int)min( min( pPlayer->maxarmor - pPlayer->pev->armorvalue, pev->armorvalue ), 20.0f );
	pev->armorvalue -= armor;
	pPlayer->pev->armorvalue += armor;
	if ( pev->armortype == 0 )
	{
		pev->armortype = 0.3f;
		items |= 0x2000;
	}
	if ( shells + nails + rockets + cells + armor == 0 )
		return FALSE;
	EMIT_SOUND_DYN( pPlayer->edict(), CHAN_ITEM, "items/ammopickup2.wav", 1, ATTN_NORM, 0, PITCH_NORM );
	return TRUE;
}

void CTFDispenser::TeamFortress_TakeEMPBlast( entvars_t* pevGren )
{
	CBaseEntity* pAttacker = CBaseEntity::Instance( pevGren->owner );
	ENT( pevGren );
	if ( !( (int)gpGlobals->teamplay & 0x1e ) || !IsAlly( pAttacker ) || (CBaseEntity*)real_owner == pAttacker )
		TakeDamage( pevGren, pAttacker->pev, 200, DMG_BLAST );
}

void CTFDispenser::Dispenser_Explode()
{
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLODEMODEL );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_COORD( 400 ); WRITE_SHORT( m_iShardIndex ); WRITE_SHORT( 10 ); WRITE_BYTE( 25 );
	MESSAGE_END();
	if ( !real_owner->has_disconnected )
	{
		float damage = ammo_rockets * 1.5 + 25 + ammo_cells;
		if ( damage > 250 ) damage = 250;
		RadiusDamage( pev, real_owner->pev, damage, 0, DMG_BLAST | 0x04000000 );
	}
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
}

void CTFDispenser::Remove()
{
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	if ( !pOwner->no_dispenser_message )
		ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Dispenser_destroyed" );
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
	const char* authid = GETPLAYERAUTHID( pOwner->edict() );
	int userid = GETPLAYERUSERID( pOwner->edict() );
	UTIL_LogPrintf( "\"<-1><><>\" triggered \"Dispenser_destroyed\" against \"%s<%i><%s><%s>\"\n",
		STRING( pOwner->pev->netname ), userid, authid, team );
	pOwner->no_dispenser_message = 0;
	pOwner->has_dispenser = 0;
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_EXPLOSION );
	WRITE_COORD( pev->origin.x ); WRITE_COORD( pev->origin.y ); WRITE_COORD( pev->origin.z );
	WRITE_SHORT( g_sModelIndexFireball ); WRITE_BYTE( 30 ); WRITE_BYTE( 15 ); WRITE_BYTE( 3 );
	MESSAGE_END();
	dremove( this );
}

void GetWeapon( entvars_t* pevInflictor, entvars_t* pKiller, char* weapon )
{
	ENT( pKiller );
	const char* name;
	if ( pKiller->flags & FL_CLIENT )
	{
		ENTINDEX( ENT( pKiller ) );
		if ( !pevInflictor )
		{
			strcpy( weapon, "world" );
			return;
		}
	}
	if ( ( pKiller->flags & FL_CLIENT ) && pKiller == pevInflictor )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)CBaseEntity::Instance( pKiller );
		CBasePlayerItem* pItem = pPlayer->m_pActiveItem;
		if ( !pItem )
		{
			strcpy( weapon, "world" );
			return;
		}
		if ( pItem->deathtype )
			name = STRING( pItem->deathtype );
		else
		{
			ItemInfo info;
			name = pItem->GetItemInfo( &info ) ? info.pszName : "world";
		}
	}
	else
	{
		CBaseEntity* pInflictor = CBaseEntity::Instance( pevInflictor );
		name = STRING( pInflictor && pInflictor->deathtype ? pInflictor->deathtype : pevInflictor->classname );
	}
	if ( !strncmp( name, "weapon_", 7 ) ) name += 7;
	else if ( !strncmp( name, "tf_weapon_", 10 ) ) name += 10;
	else if ( !strncmp( name, "monster_", 8 ) ) name += 8;
	else if ( !strncmp( name, "func_", 5 ) ) name += 5;
	strcpy( weapon, name );
}

void CTFDispenser::Killed( entvars_t* pevInflictor, entvars_t* pevAttacker, int iGib )
{
	(void)iGib;
	if ( m_iDestroyed )
		return;
	CBasePlayer* pOwner = (CBasePlayer*)(CBaseEntity*)real_owner;
	if ( !pOwner->no_dispenser_message )
		ClientPrint( pOwner->pev, HUD_PRINTCENTER, "#Dispenser_destroyed" );
	pOwner->has_dispenser = 0;
	pOwner->no_dispenser_message = 0;
	PLAYBACK_EVENT_FULL( FEV_GLOBAL | FEV_RELIABLE, edict(), m_usBuildingEvent, 0, pev->origin, (float*)&g_vecZero,
		0, 0, 1024, 0, 0, 0 );
	SetThink( &CTFDispenser::Dispenser_Explode );
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pevAttacker && real_owner )
	{
		CBasePlayer* pAttacker = (CBasePlayer*)CBaseEntity::Instance( pevAttacker );
		char weaponName[30];
		GetWeapon( pevInflictor, pevAttacker, weaponName );
		const char* team = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		if ( pevAttacker == pevInflictor )
		{
			const char* authid = GETPLAYERAUTHID( pOwner->edict() );
			int userid = GETPLAYERUSERID( pOwner->edict() );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dispenser_Destroyed\"\n",
				STRING( pOwner->pev->netname ), userid, authid, team );
		}
		else if ( pevAttacker != pOwner->pev )
		{
			const char* ownerAuth = GETPLAYERAUTHID( pOwner->edict() );
			int ownerID = GETPLAYERUSERID( pOwner->edict() );
			const char* ownerName = STRING( pOwner->pev->netname );
			const char* attackerTeam = pAttacker->team_no ? GetTeamName( pAttacker->team_no ) : "SPECTATOR";
			const char* attackerAuth = GETPLAYERAUTHID( pAttacker->edict() );
			int attackerID = GETPLAYERUSERID( pAttacker->edict() );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Dispenser_Destroyed\" against \"%s<%i><%s><%s>\" with \"%s\"\n",
				STRING( pevAttacker->netname ), attackerID, attackerAuth, attackerTeam,
				ownerName, ownerID, ownerAuth, team, weaponName );
		}
	}
	m_iDestroyed = 1;
}

void CDispenserRefillThinker::Spawn()
{
	pev->classname = MAKE_STRING( "timer" );
	timer_type = 14;
}

void CDispenserRefillThinker::StartRefillingPlayer( CBaseEntity* pPlayer, CTFDispenser* pDispenser, float flStartTime )
{
	pev->nextthink = gpGlobals->time + flStartTime;
	m_hDispenser = pDispenser;
	m_hPlayer = pPlayer;
	m_InitialUsePoint = pPlayer->pev->origin;
}

void CDispenserRefillThinker::Think()
{
	CBasePlayer* pPlayer = (CBasePlayer*)(CBaseEntity*)m_hPlayer;
	CTFDispenser* pDispenser = (CTFDispenser*)(CBaseEntity*)m_hDispenser;
	if ( pPlayer && pDispenser && pDispenser->m_flNextUseTime <= gpGlobals->time + 1.5 &&
		( pPlayer->pev->origin - m_InitialUsePoint ).Length() <= 24 && pDispenser->RefillPlayer( pPlayer ) )
		pev->nextthink = gpGlobals->time + 1.0;
	else
		UTIL_Remove( this );
}
