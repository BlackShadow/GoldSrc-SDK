#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "player.h"
#include "weapons.h"
#include "tf_defs.h"
#include "tf_wpn_grenades.h"
#include "soundent.h"

extern int gmsgStatusIcon;

LINK_ENTITY_TO_CLASS( tf_weapon_genericprimedgrenade, CTFPrimeGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_normalgrenade, CTFNormalGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_concussiongrenade, CTFConcussionGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_mirvgrenade, CTFMirvGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_mirvbomblet, CTFBomblet );
LINK_ENTITY_TO_CLASS( tf_weapon_nailgrenade, CTFNailGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_caltropgrenade, CTFCaltropGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_caltrop, CTFCaltrop );
LINK_ENTITY_TO_CLASS( tf_weapon_napalmgrenade, CTFNapalmGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_empgrenade, CTFEMPGrenade );
LINK_ENTITY_TO_CLASS( tf_weapon_gasgrenade, CTFGasGrenade );

int CTFConcussionGrenade::BlastTexture;
int CTFEMPGrenade::BlastTexture;

void CTFPrimeGrenade::Spawn()
{
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;
	setModel();
	setName();
	setDamage();
	setRadius();
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	m_bDontSpark = FALSE;
	m_bDontSmoke = TRUE;
	m_fRegisteredSound = FALSE;
	m_flSmokeScale = 0;
	m_flExplosionScale = 0;
	team_no = CBaseEntity::Instance( pev->owner )->team_no;
}

void CTFPrimeGrenade::TakeEMPBlast( entvars_t* pevGren )
{
	heat = gpGlobals->time;
	SetThink( &CTFPrimeGrenade::GrenadePrimed );
}

void CTFPrimeGrenade::InitPriming( CBaseEntity* pOwner )
{
	pev->owner = pOwner->edict();
	team_no = pOwner->team_no;
	SetThink( &CTFPrimeGrenade::GrenadePrimed );
	pev->nextthink = gpGlobals->time + getPinTime();
	heat = gpGlobals->time + getPrimeTime() + getPinTime();
	team_no = pOwner->team_no;
	if ( pOwner->m_iszGrenadeName )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pOwner->pev );
		WRITE_BYTE( 0 );
		WRITE_CHAR( 'd' );
		WRITE_CHAR( '_' );
		WRITE_STRING( pOwner->m_iszGrenadeName ? STRING( pOwner->m_iszGrenadeName ) : "normalgrenade" );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	pOwner->m_iszGrenadeName = pev->classname;
}

BOOL CTFPrimeGrenade::canPrime( CBaseEntity* pOwner )
{
	return !( pOwner->tfstate & TFSTATE_GRENPRIMED );
}

void CTFPrimeGrenade::Precache()
{
	PRECACHE_MODEL( "models/presentsm.mdl" );
	g_sModelIndexGrenade = PRECACHE_MODEL( "models/w_grenade.mdl" );
	PRECACHE_MODEL( "models/v_grenade.mdl" );
	PRECACHE_MODEL( "models/p_grenade.mdl" );
	PRECACHE_MODEL( "models/p_grenade2.mdl" );
	PRECACHE_MODEL( "models/w_grenadeT.mdl" );
}

void CTFPrimeGrenade::ExplodeInHand()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	Spawn();
	UTIL_SetOrigin( pev, pOwner->pev->origin );
	pev->velocity = g_vecZero;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->dmgtime = heat;
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
}

void CTFPrimeGrenade::DeadThrow()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	Spawn();
	UTIL_SetOrigin( pev, pOwner->pev->origin );
	pev->velocity = Vector( 0, 0, 30 );
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->avelocity = g_vecZero;
	pev->dmgtime = heat;
	SetTouch( &CGrenade::BounceTouch );
	SetThink( &CGrenade::TumbleThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->sequence = RANDOM_LONG( 3, 6 );
	pev->framerate = 1;
	pev->gravity = 0.81f;
	pev->friction = 0.6f;
}

void CTFPrimeGrenade::Throw()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	Spawn();
	UTIL_MakeVectors( pOwner->pev->v_angle );
	Vector vecSide = gpGlobals->v_right * RANDOM_FLOAT( -10, 10 );
	float height = 200 + RANDOM_FLOAT( -10, 10 );
	pev->velocity = gpGlobals->v_forward * 600 + gpGlobals->v_up * height + vecSide;
	pev->avelocity = g_vecZero;
	UTIL_SetOrigin( pev, pOwner->pev->origin );
	pev->dmgtime = heat;
	SetTouch( &CGrenade::BounceTouch );
	SetThink( &CGrenade::TumbleThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->framerate = 1;
	pev->gravity = 0.81f;
	pev->friction = 0.6f;
}

void CTFPrimeGrenade::GrenadePrimed()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner->bRemoveGrenade )
	{
		pOwner->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
		pOwner->bRemoveGrenade = FALSE;
		dremove( this );
		return;
	}
	if ( ( pOwner->tfstate & TFSTATE_GRENTHROWING ) || !pOwner->IsAlive() )
	{
		pOwner->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
		if ( pOwner->IsAlive() )
			Throw();
		else
			DeadThrow();
		return;
	}
	pev->nextthink = gpGlobals->time + 0.01;
	if ( heat < gpGlobals->time )
	{
		pOwner->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
		ExplodeInHand();
	}
}

void CTFNormalGrenade::Precache()
{
	CTFPrimeGrenade::Precache();
}

CGrenade* CTFNormalGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFNormalGrenade* pGrenade = GetClassPtr( (CTFNormalGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "normalgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFNormalGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_normalgren.sc" );
	CGrenade::Explode( pTrace, bitsDamageType | DMG_RADIUS_QUAKE );
}

void CTFConcussionGrenade::Precache()
{
	BlastTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
	PRECACHE_MODEL( "models/conc_grenade.mdl" );
	PRECACHE_SOUND( "weapons/concgren_blast1.wav" );
	PRECACHE_SOUND( "weapons/concgren_blast2.wav" );
	PRECACHE_SOUND( "weapons/concgren_blast3.wav" );
	m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_concuss.sc" );
}

CGrenade* CTFConcussionGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFConcussionGrenade* pGrenade = GetClassPtr( (CTFConcussionGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "concussiongrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFConcussionGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/conc_grenade.mdl" );
}

void CTFConcussionGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_concuss.sc" );
	PLAYBACK_EVENT_FULL( 0, NULL, m_usExplode, 0, pev->origin, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, blastRadius ) ) != NULL )
		pEntity->TeamFortress_TakeConcussionBlast( pev, blastRadius );
	UTIL_Remove( this );
}

void CTFMirvGrenade::Precache()
{
	g_sModelIndexMirvGrenade = PRECACHE_MODEL( "models/mirv_grenade.mdl" );
	PRECACHE_MODEL( "models/bomblet.mdl" );
}

CGrenade* CTFMirvGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFMirvGrenade* pGrenade = GetClassPtr( (CTFMirvGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "mirvgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFMirvGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/mirv_grenade.mdl" );
}

void CTFMirvGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_mirvmain.sc" );
	for ( int i = 0; i < 4; i++ )
	{
		CTFBomblet* pBomblet = GetClassPtr( (CTFBomblet*)NULL );
		if ( pBomblet )
		{
			pBomblet->m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_mirv.sc" );
			pBomblet->LaunchFromMirv( this );
		}
	}
	CGrenade::Explode( pTrace, bitsDamageType | DMG_RADIUS_QUAKE );
}

void CTFBomblet::setModel()
{
	SET_MODEL( edict(), "models/bomblet.mdl" );
}

void CTFBomblet::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_normalgren.sc" );
	CGrenade::Explode( pTrace, bitsDamageType | DMG_RADIUS_QUAKE );
}

void CTFBomblet::LaunchFromMirv( CTFMirvGrenade* pMirvGren )
{
	Vector vecSrc = pMirvGren->pev->origin + Vector( 0, 0, 32 );
	float z = RANDOM_FLOAT( 0, 40 );
	float y = RANDOM_FLOAT( -75, 75 );
	float x = RANDOM_FLOAT( -75, 75 );
	Spawn();
	UTIL_SetOrigin( pev, vecSrc );
	pev->velocity = Vector( x * 2, y * 2, z * 5 );
	pev->avelocity = g_vecZero;
	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->owner = pMirvGren->pev->owner;
	team_no = pMirvGren->team_no;
	SetTouch( &CGrenade::BounceTouch );
	float detonateTime = gpGlobals->time + 2.0;
	pev->dmgtime = detonateTime + RANDOM_FLOAT( 0, 1 );
	SetThink( &CGrenade::TumbleThink );
	pev->nextthink = gpGlobals->time + 0.1;
	pev->sequence = RANDOM_LONG( 3, 6 );
	pev->framerate = 1;
	pev->gravity = 0.5;
	pev->friction = 0.8f;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, vecSrc );
}

void CTFNailGrenade::Precache()
{
	g_sModelIndexNailGrenade = PRECACHE_MODEL( "models/ngrenade.mdl" );
}

CGrenade* CTFNailGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFNailGrenade* pGrenade = GetClassPtr( (CTFNailGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "nailgrenade" );
	pGrenade->deathtype = MAKE_STRING( "nailgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFNailGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/ngrenade.mdl" );
}

void CTFNailGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	CBaseEntity::Instance( pev->owner );
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_NOT;
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + Vector( 0, 0, 32 ), ignore_monsters, edict(), &tr );
	UTIL_SetOrigin( pev, tr.vecEndPos );
	SetTouch( NULL );
	pev->flags &= ~FL_ONGROUND;
	pev->angles = g_vecZero;
	pev->effects |= EF_NOINTERP;
	pev->velocity = g_vecZero;
	pev->nextthink = gpGlobals->time + 0.4;
	SetThink( &CTFNailGrenade::NailGrenadeNailEm );
}

void CTFNailGrenade::NailGrenadeNailEm()
{
	pev->nextthink = gpGlobals->time + 0.1;
	SetThink( &CTFNailGrenade::NailGrenadeLaunchNail );
	pev->playerclass = 0;
}

void CTFNailGrenade::NailGrenadeLaunchNail()
{
	float step = RANDOM_FLOAT( 30, 40 );
	static unsigned short m_usNailGrenade;
	if ( !m_usNailGrenade )
		m_usNailGrenade = PRECACHE_EVENT( 1, "events/explode/tf_nailgren.sc" );
	unsigned short owner = 0;
	int client = ENTINDEX( pev->owner ) - 1;
	if ( client >= 0 && client < gpGlobals->maxClients )
		owner = (unsigned short)( client << 11 );
	PLAYBACK_EVENT_FULL( 0, edict(), m_usNailGrenade, 0, (float*)&g_vecZero.x, (float*)&g_vecZero.x,
		step, 0, owner | ( (int)( 4.0 * pev->angles.y ) & 0x7ff ), 0, 0, 0 );
	for ( int i = 0; i < 4; i++ )
	{
		pev->angles.y = UTIL_AngleMod( pev->angles.y + step );
		UTIL_MakeVectors( pev->angles );
		CTFNailgunNail::CreateNailGrenNail( pev->origin + gpGlobals->v_forward * 12, pev->angles, this, NULL );
	}
	pev->playerclass++;
	pev->nextthink = gpGlobals->time + 0.1;
	if ( pev->playerclass > 40 )
		SetThink( &CTFNailGrenade::FinishedExplode );
}

void CTFNailGrenade::FinishedExplode()
{
	Vector vecSpot = pev->origin + Vector( 0, 0, 8 );
	TraceResult tr;
	UTIL_TraceLine( vecSpot, vecSpot + Vector( 0, 0, -40 ), ignore_monsters, edict(), &tr );
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_ng.sc" );
	CGrenade::Explode( &tr, DMG_BLAST | DMG_RADIUS_QUAKE );
	CBaseEntity::Instance( pev->owner );
}

void CTFNapalmGrenade::Precache()
{
	PRECACHE_SOUND( "weapons/flmgrexp.wav" );
	g_sModelIndexNapalmGrenade = PRECACHE_MODEL( "models/napalm.mdl" );
}

CGrenade* CTFNapalmGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFNapalmGrenade* pGrenade = GetClassPtr( (CTFNapalmGrenade*)NULL );
	pGrenade->numExplosionsLeft = 7;
	pGrenade->pev->classname = MAKE_STRING( "napalmgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFNapalmGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/napalm.mdl" );
}

void CTFEMPGrenade::Precache()
{
	BlastTexture = PRECACHE_MODEL( "sprites/shockwave.spr" );
	PRECACHE_SOUND( "weapons/emp_1.wav" );
	PRECACHE_MODEL( "models/emp_grenade.mdl" );
	m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_emp.sc" );
}

CGrenade* CTFEMPGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFEMPGrenade* pGrenade = GetClassPtr( (CTFEMPGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "empgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFEMPGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/emp_grenade.mdl" );
}

void CTFEMPGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_emp.sc" );
	PLAYBACK_EVENT_FULL( 0, NULL, m_usExplode, 0, pev->origin, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, blastRadius ) ) != NULL )
		pEntity->TeamFortress_TakeEMPBlast( pev );
	UTIL_Remove( this );
}

void CTFGasGrenade::Precache()
{
	PRECACHE_MODEL( "models/spy_grenade.mdl" );
}

CGrenade* CTFGasGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFGasGrenade* pGrenade = GetClassPtr( (CTFGasGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "gasgrenade" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFGasGrenade::setModel()
{
	if ( !setBirthdayModel() )
		SET_MODEL( edict(), "models/spy_grenade.mdl" );
}

void CTFGasGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	gasCharges = 20;
	SetThink( &CTFGasGrenade::GasThePlace );
	pev->nextthink = gpGlobals->time + 0.1;
}

void CTFCaltropGrenade::Precache()
{
	CTFPrimeGrenade::Precache();
}

CGrenade* CTFCaltropGrenade::Prime( CBaseEntity* pOwner )
{
	if ( !canPrime( pOwner ) )
		return NULL;
	CTFCaltropGrenade* pGrenade = GetClassPtr( (CTFCaltropGrenade*)NULL );
	pGrenade->pev->classname = MAKE_STRING( "caltrop" );
	pGrenade->InitPriming( pOwner );
	return pGrenade;
}

void CTFCaltropGrenade::InitPriming( CBaseEntity* pOwner )
{
	pev->owner = pOwner->edict();
	SetThink( &CTFCaltropGrenade::CaltropGrenadePrimed );
	pev->nextthink = gpGlobals->time + getPinTime();
	heat = gpGlobals->time + getPrimeTime() + getPinTime();
	team_no = pOwner->team_no;
	if ( pOwner->m_iszGrenadeName )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgStatusIcon, NULL, pOwner->pev );
		WRITE_BYTE( 0 );
		WRITE_CHAR( 'd' );
		WRITE_CHAR( '_' );
		WRITE_STRING( pOwner->m_iszGrenadeName ? STRING( pOwner->m_iszGrenadeName ) : "normalgrenade" );
		WRITE_BYTE( 255 );
		WRITE_BYTE( 160 );
		WRITE_BYTE( 0 );
		MESSAGE_END();
	}
	pOwner->m_iszGrenadeName = pev->classname;
}

void CTFCaltropGrenade::TakeEMPBlast( entvars_t* pevGren )
{
}

void CTFCaltropGrenade::setModel()
{
	CGrenade::setModel();
}

void CTFCaltropGrenade::CaltropGrenadePrimed()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner->bRemoveGrenade )
	{
		pOwner->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
		pOwner->bRemoveGrenade = FALSE;
		dremove( this );
		return;
	}
	if ( !( pOwner->tfstate & TFSTATE_GRENTHROWING ) )
	{
		pev->nextthink = gpGlobals->time + 0.1;
		if ( heat >= gpGlobals->time )
			return;
	}
	pOwner->tfstate &= ~( TFSTATE_GRENPRIMED | TFSTATE_GRENTHROWING );
	pev->velocity = g_vecZero;
	pev->avelocity = g_vecZero;
	setDamage();
	pev->dmgtime = heat;
	SetThink( &CGrenade::Detonate );
	pev->nextthink = gpGlobals->time;
	pev->sequence = RANDOM_LONG( 3, 6 );
	pev->framerate = 1;
	pev->gravity = 0.5;
	pev->friction = 0.8f;
	setModel();
}

void CTFCaltrop::Precache()
{
	PRECACHE_MODEL( "models/caltrop.mdl" );
}

void CTFCaltrop::setModel()
{
	SET_MODEL( edict(), "models/caltrop.mdl" );
}

void CTFCaltrop::CanisterTouch( CBaseEntity* pOther )
{
	if ( pev->velocity == g_vecZero )
		pev->avelocity = g_vecZero;
}

void CTFCaltropGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( pOwner && InNoGrenadeZone( pOwner->pev->origin ) )
		return;
	ClientPrint( VARS( pev->owner ), HUD_PRINTNOTIFY, "#Caltrop_exc" );
	for ( int i = 0; i < 6; i++ )
	{
		CTFCaltrop* pCaltrop = GetClassPtr( (CTFCaltrop*)NULL );
		pCaltrop->ScatterDrop( CBaseEntity::Instance( pev->owner ) );
	}
	UTIL_Remove( this );
}

void CTFCaltrop::ScatterDrop( CBaseEntity* pThrower )
{
	Vector vecSrc = pThrower ? pThrower->pev->origin : g_vecZero;
	float time = gpGlobals->time;
	Spawn();
	if ( pThrower )
	{
		team_no = pThrower->team_no;
		pev->owner = pThrower->edict();
	}
	pev->classname = MAKE_STRING( "tf_weapon_caltrop" );
	SET_MODEL( edict(), "models/caltrop.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, -8 ), Vector( 4, 4, 4 ) );
	pev->angles = g_vecZero;
	pev->angles.x = RANDOM_FLOAT( 0, 360 );
	float z = RANDOM_FLOAT( 150, 200 );
	float y = RANDOM_FLOAT( -150, 150 );
	float x = RANDOM_FLOAT( -150, 150 );
	pev->velocity = Vector( x, y, z );
	z = RANDOM_FLOAT( -400, 400 );
	y = RANDOM_FLOAT( -400, 400 );
	x = RANDOM_FLOAT( -400, 400 );
	pev->avelocity = Vector( x, y, z );
	pev->gravity = 0.5;
	pev->friction = 0.8f;
	pev->movetype = MOVETYPE_TOSS;
	pev->solid = SOLID_TRIGGER;
	UTIL_SetOrigin( pev, vecSrc );
	m_flRemovalTime = time + RANDOM_FLOAT( 10, 15 );
	SetThink( &CTFCaltrop::CaltropScatterThink );
	pev->nextthink = gpGlobals->time;
}

void CTFCaltrop::CaltropScatterThink()
{
	pev->nextthink = gpGlobals->time + 0.2;
	if ( gpGlobals->time > m_flRemovalTime )
		SetThink( &CBaseEntity::SUB_Remove );
	else if ( pev->flags & FL_ONGROUND )
	{
		pev->solid = SOLID_TRIGGER;
		SetTouch( &CTFCaltrop::CaltropTouch );
		pev->angles = Vector( 90, 90, 90 );
		pev->avelocity = g_vecZero;
		pev->velocity = pev->avelocity;
		UTIL_SetOrigin( pev, pev->origin );
	}
	else
		SetTouch( &CTFCaltrop::CanisterTouch );
}

void CTFCaltrop::CaltropTouch( CBaseEntity* pOther )
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( !pOther->IsPlayer() || !( pOther->pev->flags & FL_ONGROUND ) || !pOther->IsAlive() )
		return;
	if ( !IsAlly( pOther ) || pOther == pOwner || !( (int)gpGlobals->teamplay & 0x1e ) )
	{
		pOther->leg_damage += 2;
		( (CBasePlayer*)pOther )->TeamFortress_SetSpeed();
		pOther->TakeDamage( pev, VARS( pev->owner ), 10, DMG_CALTROP );
		char* targetTeam = pOther->team_no ? GetTeamName( pOther->team_no ) : "SPECTATOR";
		const char* targetAuth = GETPLAYERAUTHID( pOther->edict() );
		int targetUser = GETPLAYERUSERID( pOther->edict() );
		const char* targetName = STRING( pOther->pev->netname );
		char* ownerTeam = pOwner->team_no ? GetTeamName( pOwner->team_no ) : "SPECTATOR";
		const char* ownerAuth = GETPLAYERAUTHID( pev->owner );
		int ownerUser = GETPLAYERUSERID( pev->owner );
		UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Caltrop_Grenade\" against \"%s<%i><%s><%s>\"\n",
			STRING( pOwner->pev->netname ), ownerUser, ownerAuth, ownerTeam,
			targetName, targetUser, targetAuth, targetTeam );
	}
	UTIL_Remove( this );
}

void CTFNapalmGrenade::Explode( TraceResult* pTrace, int bitsDamageType )
{
	if ( InNoGrenadeZone( pev->origin ) )
		return;
	pev->solid = SOLID_NOT;
	pev->takedamage = DAMAGE_NO;
	m_bitsDamageType = bitsDamageType;
	m_flDamageTime = gpGlobals->time;
	m_flEffectTime = gpGlobals->time;
	CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 1024, 0.5 );
	pev->velocity = g_vecZero;
	Vector oldPos = pev->origin;
	if ( pTrace->flFraction != 1 )
	{
		float damage = pev->dmg;
		if ( damage >= 24 )
			damage -= 24;
		pev->origin = pTrace->vecEndPos + pTrace->vecPlaneNormal * damage * 0.6f;
	}
	if ( UTIL_PointContents( pev->origin ) == CONTENTS_SOLID )
	{
		pev->origin = oldPos;
		if ( UTIL_PointContents( pev->origin ) == CONTENTS_SOLID )
		{
			UTIL_Remove( this );
			return;
		}
	}
	if ( !m_usExplode )
		m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_fire.sc" );
	PLAYBACK_EVENT_FULL( 0, NULL, m_usExplode, 0, pev->origin, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	pev->effects |= EF_NODRAW;
	SetThink( &CTFNapalmGrenade::Burn );
	pev->nextthink = gpGlobals->time;
}

void CTFNapalmGrenade::Burn()
{
	pev->nextthink = gpGlobals->time + 0.1;
	if ( gpGlobals->time >= m_flDamageTime )
	{
		EMIT_SOUND( edict(), CHAN_WEAPON, "ambience/fire1.wav", 1, ATTN_NORM );
		::RadiusDamage( pev->origin, pev, VARS( pev->owner ), pev->dmg, blastRadius,
			CLASS_NONE, m_bitsDamageType | DMG_IGNITE | DMG_RADIUS_MAX );
		if ( !pev->owner )
			INDEXENT( 0 );
		if ( --numExplosionsLeft <= 0 )
			UTIL_Remove( this );
		else
			m_flDamageTime += 1.0;
	}
	if ( gpGlobals->time >= m_flEffectTime )
	{
		static unsigned short usBurn;
		if ( !usBurn )
			usBurn = PRECACHE_EVENT( 1, "events/explode/tf_burn.sc" );
		PLAYBACK_EVENT_FULL( 0, edict(), usBurn, 0, pev->origin, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
		m_flEffectTime += 0.5;
	}
}

void CTFGasGrenade::GasThePlace()
{
	CBaseEntity* pOwner = CBaseEntity::Instance( pev->owner );
	if ( !( gasCharges & 1 ) )
	{
		if ( !m_usExplode )
			m_usExplode = PRECACHE_EVENT( 1, "events/explode/tf_gas.sc" );
		PLAYBACK_EVENT_FULL( 0, NULL, m_usExplode, 0, pev->origin, (float*)&g_vecZero.x, 0, 0, 0, 0, 0, 0 );
	}
	CBaseEntity* pEntity = NULL;
	while ( ( pEntity = UTIL_FindEntityInSphere( pEntity, pev->origin, blastRadius ) ) != NULL )
	{
		if ( !pEntity->pev->playerclass )
			continue;
		pEntity->TakeDamage( pev, VARS( pev->owner ), 10, DMG_HALLUC | DMG_RADIUS_QUAKE );
		if ( !pEntity->IsPlayer() || ( IsAlly( pEntity ) && pEntity != pOwner && ( (int)gpGlobals->teamplay & 0x1e ) ) )
			continue;
		if ( pEntity->tfstate & TFSTATE_HALLUCINATING )
		{
			CBaseEntity* pTimer = pEntity->FindTimer( TF_TIMER_HALLUCINATION );
			if ( pTimer && pTimer->pev && OFFSET( pTimer->pev ) )
			{
				pTimer->pev->health += 25;
				if ( pTimer->pev->health < 150 )
					pTimer->pev->health = 150;
				pTimer->pev->nextthink = gpGlobals->time + 0.3;
			}
		}
		else
		{
			pEntity->tfstate |= TFSTATE_HALLUCINATING;
			CBaseEntity* pTimer = pEntity->CreateTimer( TF_TIMER_HALLUCINATION );
			pTimer->pev->nextthink = gpGlobals->time + 0.3;
			pTimer->SetThink( &CBaseEntity::Timer_Hallucination );
			pTimer->pev->owner = pEntity->edict();
			pTimer->pev->health = 150;
			pTimer->team_no = pEntity->team_no;
			CBaseEntity* pGrenOwner = CBaseEntity::Instance( pev->owner );
			char* targetTeam = pEntity->team_no ? GetTeamName( pEntity->team_no ) : "SPECTATOR";
			const char* targetAuth = GETPLAYERAUTHID( pEntity->edict() );
			int targetUser = GETPLAYERUSERID( pEntity->edict() );
			const char* targetName = STRING( pEntity->pev->netname );
			char* ownerTeam = pGrenOwner->team_no ? GetTeamName( pGrenOwner->team_no ) : "SPECTATOR";
			const char* ownerAuth = GETPLAYERAUTHID( pev->owner );
			int ownerUser = GETPLAYERUSERID( pev->owner );
			UTIL_LogPrintf( "\"%s<%i><%s><%s>\" triggered \"Hallucination_Grenade\" against \"%s<%i><%s><%s>\"\n",
				STRING( pGrenOwner->pev->netname ), ownerUser, ownerAuth, ownerTeam,
				targetName, targetUser, targetAuth, targetTeam );
		}
	}
	if ( gasCharges-- == 1 )
		UTIL_Remove( this );
	else
		pev->nextthink = gpGlobals->time + 0.75;
}
