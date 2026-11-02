#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "gamerules.h"
#include "tf_defs.h"

LINK_ENTITY_TO_CLASS( tf_nailgun_nail, CTFNailgunNail );
LINK_ENTITY_TO_CLASS( tf_flamethrower_burst, CTFFlamethrowerBurst );
LINK_ENTITY_TO_CLASS( tf_ammo_rpgclip, CTFRpgAmmo );
LINK_ENTITY_TO_CLASS( tf_gl_grenade, CTFGrenade );
LINK_ENTITY_TO_CLASS( laser_spot, CLaserSpot );

void CTFNailgunNail::Precache()
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CTFFlamethrowerBurst::Precache()
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
	sModelIndexFlameThrowerFlame = PRECACHE_MODEL( "sprites/fthrow.spr" );
}

void CTFRpgRocket::Precache()
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
}

void CTFGrenade::Precache()
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
	m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_pipe.sc" );
}

void CTFIncendiaryCRocket::Precache()
{
	PRECACHE_MODEL( "models/rpgrocket.mdl" );
	m_iTrail = PRECACHE_MODEL( "sprites/smoke.spr" );
	PRECACHE_SOUND( "weapons/rocket1.wav" );
	PRECACHE_SOUND( "weapons/sgun1.wav" );
	UTIL_PrecacheOther( "tf_flame" );
}

BOOL CGrenade::setBirthdayModel()
{
	if ( birthday && RANDOM_FLOAT( 0, 1 ) < 0.5 )
	{
		SET_MODEL( edict(), "models/presentsm.mdl" );
		return TRUE;
	}
	return FALSE;
}

void CGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/w_grenade.mdl" );
}

void CTFGrenade::setModel()
{
	if ( !setBirthdayModel() )
	{
		SET_MODEL( edict(), "models/pipebomb.mdl" );
		pev->skin = 1;
	}
}

void CTFNailgunNail::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "tf_nailgun_nail" );
	deathtype = MAKE_STRING( "nails" );
	UTIL_MakeVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 1000;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 6;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CTFRpgRocket::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	m_bDontSpark = TRUE;
	m_bDontSmoke = TRUE;
	SET_MODEL( edict(), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "tf_rpg_rocket" );
	deathtype = MAKE_STRING( "rocket" );
	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30 );
	UTIL_MakeAimVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 900;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 5;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CTFIncendiaryCRocket::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/rpgrocket.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "tf_rpg_rocket" );
	deathtype = MAKE_STRING( "rocket" );
	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30 );
	UTIL_MakeAimVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 600;
	pev->gravity = 0.5;
	pev->nextthink = gpGlobals->time + 5;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CTFFlamethrowerBurst::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLYMISSILE;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/rpgrocket.mdl" );
	pev->effects |= EF_NODRAW;
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "tf_flamethrower_burst" );
	deathtype = MAKE_STRING( "flames" );
	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( pev->angles.x + 30 );
	UTIL_MakeAimVectors( pev->angles );
	pev->velocity = gpGlobals->v_forward * 600;
	pev->gravity = 0.5;
	ENTINDEX( pev->owner );
	pev->nextthink = gpGlobals->time + 1;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CTFGrenade::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	pev->gravity = 1;
	pev->friction = 0.5;
	m_bDontSpark = TRUE;
	m_bDontSmoke = TRUE;
	setModel();
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_MakeVectors( pev->angles );
	Vector randomUp = gpGlobals->v_up * RANDOM_FLOAT( -10, 10 );
	float randomRight = RANDOM_FLOAT( -10, 10 );
	pev->velocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * 200
		+ gpGlobals->v_right * randomRight + randomUp;
	pev->avelocity = Vector( 300, 300, 300 );
	pev->nextthink = gpGlobals->time + 2.5;
	SetThink( &CGrenade::Detonate );
	pev->dmg = 120;
}

void CTFFlamethrowerBurst::BurstTouch( CBaseEntity* pOther )
{
	if ( pOther && pOther->pev->takedamage )
		pOther->TakeDamage( pev, VARS( pev->owner ), 15, DMG_BURN | DMG_IGNITE );
	SUB_Remove();
}

void CTFNailgunNail::NailTouch( CBaseEntity* pOther )
{
	entvars_t* pevAttacker = real_owner ? real_owner->pev : VARS( pev->owner );
	if ( pOther && pOther->pev->takedamage )
	{
		if ( pOther->TakeDamage( pev, pevAttacker, pev->dmg, DMG_NAIL ) )
			SpawnBlood( pev->origin, pOther->BloodColor(), pev->dmg );
	}
	if ( !pOther || pOther->pev->modelindex != pev->modelindex )
		SUB_Remove();
}

void CTFNailgunNail::RailgunNailTouch( CBaseEntity* pOther )
{
	if ( pOther->pev->takedamage )
	{
		CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
		if ( !pev->enemy )
			INDEXENT( 0 );
		if ( pOther->TakeDamage( pev, pOwner->pev, 25, DMG_ENERGYBEAM ) )
			SpawnBlood( pev->origin, pOther->BloodColor(), pev->dmg );
	}
	SUB_Remove();
}

void CTFNailgunNail::RailgunNail_Think()
{
	pev->velocity = pev->oldorigin;
	SetTouch( &CTFNailgunNail::RailgunNailTouch );
	pev->nextthink = gpGlobals->time + 1;
	SetThink( &CBaseEntity::SUB_Remove );
}

void CTFGrenade::GrenadeTouch( CBaseEntity* pOther )
{
	if ( pOther && pOther->pev->takedamage == DAMAGE_AIM )
		ExplodeTouch( pOther );
	else
	{
		BounceSound();
		if ( pev->flags & FL_ONGROUND )
		{
			pev->velocity = pev->velocity * 0.75;
			if ( pev->velocity.Length() <= 20 )
				pev->avelocity = g_vecZero;
		}
	}
}

void CTFIncendiaryCRocket::RocketTouch( CBaseEntity* pOther )
{
	pev->dmg = RANDOM_FLOAT( 0, 20 ) + 10;
	m_flExplosionScale = 0.5 * pev->dmg;
	m_bDontSpark = TRUE;
	m_flSmokeScale = pev->dmg + 30;
	if ( pOther && pOther->pev->takedamage )
		pOther->TakeDamage( pev, VARS( pev->owner ), pev->dmg, DMG_BLAST | DMG_IGNITE );
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	ExplodeTouch( pOther );
}

void CTFRpgRocket::RocketTouch( CBaseEntity* pOther )
{
	pev->dmg = RANDOM_FLOAT( 0, 20 ) + 92;
	STOP_SOUND( edict(), CHAN_VOICE, "weapons/rocket1.wav" );
	ExplodeTouch( pOther );
}

void CTFIncendiaryCRocket::RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, 15, 180, iClassIgnore,
		bitsDamageType | DMG_IGNITE | DMG_RADIUS_MAX | DMG_WALLPIERCING );
}

void CTFRpgRocket::RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage,
		iClassIgnore, bitsDamageType | DMG_RADIUS_QUAKE );
}

void CTFGrenade::RadiusDamage( entvars_t* pevInflictor, entvars_t* pevAttacker,
	float flDamage, int iClassIgnore, int bitsDamageType )
{
	::RadiusDamage( pev->origin, pevInflictor, pevAttacker, flDamage, flDamage,
		iClassIgnore, bitsDamageType | DMG_RADIUS_QUAKE );
}

void CTFGrenade::TeamFortress_TakeEMPBlast( entvars_t* pevGren )
{
	if ( !strcmp( STRING( pev->classname ), "tf_gl_pipebomb" ) )
		pev->nextthink = gpGlobals->time + 0.1;
}

void CTFGrenade::PipebombDetonate()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner->Classify() == CLASS_PLAYER )
		( (CBasePlayer*)pOwner )->m_iPipebombCount--;
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -32 ), ignore_monsters, edict(), &tr );
	Explode( &tr, DMG_BLAST | DMG_RADIUS_QUAKE );
}

void UTIL_ParametricRocket( entvars_t* pev, Vector vecOrigin, Vector vecAngles, edict_t* owner )
{
	pev->startpos = vecOrigin;
	UTIL_MakeVectors( vecAngles );
	Vector vecEnd = pev->startpos + gpGlobals->v_forward * 8192;
	TraceResult tr;
	UTIL_TraceLine( pev->startpos, vecEnd, ignore_monsters, owner, &tr );
	pev->endpos = tr.vecEndPos;
	float travelTime = pev->velocity.Length() <= 0 ? 0 : ( pev->endpos - pev->startpos ).Length() / pev->velocity.Length();
	pev->starttime = gpGlobals->time;
	pev->impacttime = gpGlobals->time + travelTime;
}

CTFNailgunNail* CTFNailgunNail::CreateNail( BOOL fSendClientNail, Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CBaseEntity* pLauncher, BOOL fMakeClientNail )
{
	CTFNailgunNail* pNail = GetClassPtr( (CTFNailgunNail*)NULL );
	UTIL_SetOrigin( pNail->pev, vecOrigin );
	pNail->pev->angles = vecAngles;
	pNail->pev->owner = pOwner->edict();
	pNail->Spawn();
	pNail->SetTouch( &CTFNailgunNail::NailTouch );
	pNail->pev->dmg = 9;
	if ( fMakeClientNail )
	{
		pNail->pev->effects |= EF_NODRAW;
		int ownerIndex = ENTINDEX( pNail->pev->owner );
		if ( fSendClientNail )
			UTIL_ClientProjectile( pNail->pev->origin, pNail->pev->velocity, g_sModelIndexNail, ownerIndex, 6 );
	}
	return pNail;
}

CTFNailgunNail* CTFNailgunNail::CreateSuperNail( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CBaseEntity* pLauncher )
{
	CTFNailgunNail* pNail = CreateNail( FALSE, vecOrigin, vecAngles, pOwner, pLauncher, TRUE );
	pNail->deathtype = MAKE_STRING( "supernails" );
	pNail->pev->dmg = 13;
	return pNail;
}

CTFNailgunNail* CTFNailgunNail::CreateNailGrenNail( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CBaseEntity* pNailGren )
{
	CBaseEntity* pEntity = CBaseEntity::Instance( pOwner->pev->owner );
	CTFNailgunNail* pNail = CreateNail( FALSE, vecOrigin, vecAngles, pEntity, pNailGren, TRUE );
	pNail->real_owner = pEntity;
	pNail->pev->owner = pOwner->edict();
	pNail->pev->classname = MAKE_STRING( "tf_weapon_nailgrenadenail" );
	pNail->deathtype = MAKE_STRING( "nailgrenade" );
	pNail->pev->dmg = 18;
	pNail->team_no = pOwner->team_no;
	return pNail;
}

CTFNailgunNail* CTFNailgunNail::CreateTranqNail( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CBaseEntity* pLauncher )
{
	CTFNailgunNail* pNail = CreateNail( FALSE, vecOrigin, vecAngles, pOwner, pLauncher, FALSE );
	UTIL_MakeVectors( pNail->pev->angles );
	pNail->pev->velocity = gpGlobals->v_forward * 1500;
	pNail->pev->effects |= EF_NODRAW;
	ENTINDEX( pNail->pev->owner );
	pNail->SetTouch( &CTFNailgunNail::TranqTouch );
	return pNail;
}

CTFNailgunNail* CTFNailgunNail::CreateRailgunNail( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CBaseEntity* pLauncher )
{
	CTFNailgunNail* pNail = CreateNail( FALSE, vecOrigin, vecAngles, pOwner, pLauncher, FALSE );
	pNail->SetTouch( &CTFNailgunNail::RailgunNailTouch );
	pNail->deathtype = MAKE_STRING( "railgun" );
	UTIL_MakeVectors( vecAngles );
	pNail->pev->velocity = gpGlobals->v_forward * 1500;
	pNail->m_vecPreviousVelocity = pNail->pev->velocity;
	pNail->pev->effects |= EF_NODRAW;
	ENTINDEX( pNail->pev->owner );
	return pNail;
}

CTFFlamethrowerBurst* CTFFlamethrowerBurst::CreateBurst( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CTFFlamethrower* pLauncher )
{
	CTFFlamethrowerBurst* pBurst = GetClassPtr( (CTFFlamethrowerBurst*)NULL );
	UTIL_SetOrigin( pBurst->pev, vecOrigin );
	pBurst->pev->angles = vecAngles;
	pBurst->pev->owner = pOwner->edict();
	pBurst->Spawn();
	pBurst->SetTouch( &CTFFlamethrowerBurst::BurstTouch );
	return pBurst;
}

CTFIncendiaryCRocket* CTFIncendiaryCRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CTFIncendiaryC* pLauncher )
{
	CTFIncendiaryCRocket* pRocket = GetClassPtr( (CTFIncendiaryCRocket*)NULL );
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( &CTFIncendiaryCRocket::RocketTouch );
	pRocket->pev->owner = pOwner->edict();
	pRocket->pev->effects |= EF_LIGHT;
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( pRocket->entindex() );
	WRITE_SHORT( pRocket->m_iTrail );
	WRITE_BYTE( 30 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 32 );
	WRITE_BYTE( 32 );
	WRITE_BYTE( 200 );
	MESSAGE_END();
	UTIL_ParametricRocket( pRocket->pev, vecOrigin, vecAngles, pOwner->edict() );
	return pRocket;
}

CTFRpgRocket* CTFRpgRocket::CreateRpgRocket( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CTFRpg* pLauncher )
{
	CTFRpgRocket* pRocket = GetClassPtr( (CTFRpgRocket*)NULL );
	UTIL_SetOrigin( pRocket->pev, vecOrigin );
	pRocket->pev->angles = vecAngles;
	pRocket->Spawn();
	pRocket->SetTouch( &CTFRpgRocket::RocketTouch );
	pRocket->pev->owner = pOwner->edict();
	pRocket->pev->effects |= EF_LIGHT;
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( pRocket->entindex() );
	WRITE_SHORT( pRocket->m_iTrail );
	WRITE_BYTE( 10 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 128 );
	MESSAGE_END();
	UTIL_ParametricRocket( pRocket->pev, vecOrigin, vecAngles, pOwner->edict() );
	return pRocket;
}

CTFGrenade* CTFGrenade::CreateTFGrenade( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CTFGrenadeLauncher* pLauncher )
{
	CTFGrenade* pGrenade = GetClassPtr( (CTFGrenade*)NULL );
	UTIL_SetOrigin( pGrenade->pev, vecOrigin );
	pGrenade->pev->angles = vecAngles;
	pGrenade->Spawn();
	pGrenade->SetTouch( &CTFGrenade::GrenadeTouch );
	pGrenade->pev->owner = pOwner->edict();
	pGrenade->team_no = pOwner->team_no;
	pGrenade->pev->classname = MAKE_STRING( "tf_gl_grenade" );
	pGrenade->deathtype = MAKE_STRING( "gl_grenade" );
	pGrenade->m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_gren.sc" );
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( pGrenade->entindex() );
	WRITE_SHORT( pGrenade->m_iTrail );
	WRITE_BYTE( 15 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 80 );
	MESSAGE_END();
	return pGrenade;
}

CTFGrenade* CTFGrenade::CreateTFPipebomb( Vector vecOrigin, Vector vecAngles,
	CBaseEntity* pOwner, CTFGrenadeLauncher* pLauncher )
{
	CTFGrenade* pGrenade = GetClassPtr( (CTFGrenade*)NULL );
	UTIL_SetOrigin( pGrenade->pev, vecOrigin );
	pGrenade->pev->angles = vecAngles;
	pGrenade->Spawn();
	pGrenade->SetTouch( &CGrenade::BounceTouch );
	pGrenade->pev->owner = pOwner->edict();
	pGrenade->team_no = pOwner->team_no;
	pGrenade->pev->classname = MAKE_STRING( "tf_gl_pipebomb" );
	pGrenade->deathtype = MAKE_STRING( "pipebomb" );
	if ( !strcmp( STRING( pGrenade->pev->model ), "models/pipebomb.mdl" ) )
		pGrenade->pev->skin = 0;
	pGrenade->pev->nextthink = gpGlobals->time + 120;
	pGrenade->SetThink( &CTFGrenade::PipebombDetonate );
	pGrenade->m_flCreationTime = gpGlobals->time;
	MESSAGE_BEGIN( MSG_BROADCAST, SVC_TEMPENTITY );
	WRITE_BYTE( TE_BEAMFOLLOW );
	WRITE_SHORT( pGrenade->entindex() );
	WRITE_SHORT( pGrenade->m_iTrail );
	WRITE_BYTE( 10 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 224 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 80 );
	MESSAGE_END();
	if ( pOwner->Classify() == CLASS_PLAYER )
	{
		CBasePlayer* pPlayer = (CBasePlayer*)pOwner;
		if ( ++pPlayer->m_iPipebombCount > MAX_PLAYER_PIPEBOMBS )
			pPlayer->ExplodeOldPipebomb( FALSE, TRUE );
	}
	return pGrenade;
}

void CLaserSpot::Precache()
{
	PRECACHE_MODEL( "sprites/laserdot.spr" );
}

void CLaserSpot::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_NONE;
	pev->solid = SOLID_NOT;
	pev->rendermode = kRenderGlow;
	pev->renderfx = kRenderFxNoDissipation;
	pev->renderamt = 255;
	SET_MODEL( edict(), "sprites/laserdot.spr" );
	UTIL_SetOrigin( pev, pev->origin );
}

CLaserSpot* CLaserSpot::CreateSpot()
{
	CLaserSpot* pSpot = GetClassPtr( (CLaserSpot*)NULL );
	pSpot->Spawn();
	pSpot->pev->classname = MAKE_STRING( "laser_spot" );
	return pSpot;
}

void CLaserSpot::Revive()
{
	pev->effects &= ~EF_NODRAW;
	SetThink( NULL );
}

void CLaserSpot::Suspend( float flSuspendTime )
{
	pev->effects |= EF_NODRAW;
	SetThink( &CLaserSpot::Revive );
	pev->nextthink = gpGlobals->time + flSuspendTime;
}

void CTFRpgAmmo::Precache()
{
	PRECACHE_MODEL( "models/w_rpgammo.mdl" );
	PRECACHE_SOUND( "items/9mmclip1.wav" );
}

void CTFRpgAmmo::Spawn()
{
	Precache();
	SET_MODEL( edict(), "models/w_rpgammo.mdl" );
	CBasePlayerAmmo::Spawn();
}

BOOL CTFRpgAmmo::AddAmmo( CBaseEntity* pOther )
{
	if ( pOther->GiveAmmo( g_pGameRules->IsMultiplayer() ? 2 : 1, "rockets", 5 ) == -1 )
		return FALSE;
	EMIT_SOUND( edict(), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM );
	return TRUE;
}
