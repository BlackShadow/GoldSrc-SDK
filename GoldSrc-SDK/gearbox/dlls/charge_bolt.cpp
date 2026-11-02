#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "charge_bolt.h"
#include "decals.h"

LINK_ENTITY_TO_CLASS( charged_bolt, COFChargedBolt );

TYPEDESCRIPTION COFChargedBolt::m_SaveData[] =
{
	DEFINE_FIELD( COFChargedBolt, m_iShowerSparks, FIELD_INTEGER ),
	DEFINE_ARRAY( COFChargedBolt, m_pBeam, FIELD_CLASSPTR, 8 ),
	DEFINE_FIELD( COFChargedBolt, m_iBeams, FIELD_INTEGER ),
	DEFINE_FIELD( COFChargedBolt, m_pAttachEnt, FIELD_CLASSPTR ),
	DEFINE_FIELD( COFChargedBolt, m_iAttachIdx, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( COFChargedBolt, CBaseEntity );

void COFChargedBolt::Precache()
{
	PRECACHE_MODEL( "sprites/blueflare2.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iShowerSparks = PRECACHE_MODEL( "sprites/spark1.spr" );
}

void COFChargedBolt::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	pev->gravity = 0.5;
	SET_MODEL( ENT( pev ), "sprites/blueflare2.spr" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 0.75;
	InitBeams();
}

COFChargedBolt* COFChargedBolt::ChargedBoltCreate()
{
	COFChargedBolt* bolt = GetClassPtr( (COFChargedBolt*)NULL );
	bolt->pev->classname = MAKE_STRING( "Charged Bolt" );
	bolt->Spawn();
	return bolt;
}

void COFChargedBolt::InitBeams()
{
	for ( int i = 0; i < 8; i++ )
		m_pBeam[i] = NULL;
	m_iBeams = 0;
	pev->skin = 0;
}

void COFChargedBolt::SetAttachment( CBaseAnimating* entity, int attachment )
{
	m_iAttachIdx = attachment;
	m_pAttachEnt = entity;
	Vector origin, angles;
	entity->GetAttachment( attachment, origin, angles );
	UTIL_SetOrigin( pev, origin );
	SetThink( &COFChargedBolt::AttachThink );
	pev->nextthink = gpGlobals->time + 0.05;
}

void COFChargedBolt::AttachThink()
{
	Vector origin, angles;
	m_pAttachEnt->GetAttachment( m_iAttachIdx, origin, angles );
	UTIL_SetOrigin( pev, origin );
	pev->nextthink = gpGlobals->time + 0.05;
}

void COFChargedBolt::LaunchChargedBolt( Vector& aim, edict_t* owner, int speed, int )
{
	pev->angles = aim;
	pev->owner = owner;
	pev->velocity = aim * speed;
	pev->speed = (float)speed;
	SetTouch( &COFChargedBolt::ChargedBoltTouch );
	SetThink( &COFChargedBolt::FlyThink );
	pev->nextthink = gpGlobals->time + 0.15;
}

void COFChargedBolt::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void COFChargedBolt::ChargedBoltTouch( CBaseEntity* other )
{
	SetTouch( NULL );
	SetThink( NULL );
	if ( other->pev->takedamage )
	{
		ClearMultiDamage();
		other->TakeDamage( pev, pev, gSkillData.voltigoreDmgBeam, DMG_SHOCK | DMG_ALWAYSGIB );
	}
	else if ( FClassnameIs( other->pev, "worldspawn" ) )
	{
		TraceResult trace;
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &trace );
		UTIL_DecalTrace( &trace, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ) );
	}
	pev->velocity = g_vecZero;
	entvars_t* attacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	ClearMultiDamage();
	::RadiusDamage( pev->origin, pev, attacker, gSkillData.voltigoreDmgBeam, 128, CLASS_NONE, DMG_SHOCK | DMG_ALWAYSGIB );
	SetThink( &COFChargedBolt::ShutdownChargedBolt );
	pev->nextthink = gpGlobals->time + 0.5;
}

void COFChargedBolt::ArmBeam( int side )
{
	if ( m_iBeams > 7 )
		m_iBeams = 0;
	UTIL_MakeAimVectors( pev->angles );
	Vector source = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
	TraceResult best;
	float distance = 1;
	for ( int i = 0; i < 3; i++ )
	{
		Vector direction = gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		direction = direction + gpGlobals->v_right * side * RANDOM_FLOAT( 0, 1 );
		TraceResult trace;
		UTIL_TraceLine( source, source + direction * 512, dont_ignore_monsters, edict(), &trace );
		if ( trace.flFraction < distance )
		{
			best = trace;
			distance = trace.flFraction;
		}
	}
	if ( distance == 1 )
		return;
	if ( !m_pBeam[m_iBeams] )
		m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	CBeam* beam = m_pBeam[m_iBeams];
	if ( !beam )
		return;
	CBaseEntity* hit = CBaseEntity::Instance( best.pHit );
	if ( hit && hit->pev->takedamage )
	{
		beam->EntsInit( hit->entindex(), entindex() );
		beam->SetColor( 255, 16, 255 );
		beam->SetBrightness( 255 );
		beam->SetNoise( 20 );
	}
	else
	{
		beam->PointEntInit( best.vecEndPos, entindex() );
		beam->SetColor( 180, 16, 255 );
		beam->SetBrightness( 255 );
		beam->SetNoise( 80 );
	}
	m_iBeams++;
}

void COFChargedBolt::ClearBeams()
{
	for ( int i = 0; i < 8; i++ )
	{
		if ( m_pBeam[i] )
		{
			UTIL_Remove( m_pBeam[i] );
			m_pBeam[i] = NULL;
		}
	}
	m_iBeams = 0;
	pev->skin = 0;
}

void COFChargedBolt::ShutdownChargedBolt()
{
	ClearBeams();
	UTIL_Remove( this );
}
