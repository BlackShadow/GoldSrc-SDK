#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "player.h"
#include "gamerules.h"
#include "grapple.h"
#include "customentity.h"

#ifdef CLIENT_DLL
#include "hud_iface.h"
extern int giTeamplay;
extern int giOldWeapons;
#else
extern cvar_t oldweapons;
extern cvar_t oldgrapple;
#endif

void FindHullIntersection( const Vector& source, TraceResult& trace, float* mins, float* maxs, edict_t* entity );

LINK_ENTITY_TO_CLASS( weapon_grapple, CGrapple );
LINK_ENTITY_TO_CLASS( grapple_tip, CGrappleTip );

enum grapple_e
{
	GRAPPLE_IDLE,
	GRAPPLE_LONGIDLE,
	GRAPPLE_UNUSED,
	GRAPPLE_COUGH,
	GRAPPLE_HOLSTER,
	GRAPPLE_DRAW,
	GRAPPLE_FIRE,
	GRAPPLE_FIREWAIT,
	GRAPPLE_RELEASE,
	GRAPPLE_TRAVEL
};

enum grapple_target_e
{
	GRAPPLE_NOTARGET,
	GRAPPLE_SMALL,
	GRAPPLE_MEDIUM,
	GRAPPLE_LARGE,
	GRAPPLE_FIXED
};

const char* grapple_small[] =
{
	"monster_bloater", "monster_snark", "monster_shockroach", "monster_rat",
	"monster_alien_babyvoltigore", "monster_babycrab", "monster_cockroach",
	"monster_flyer_flock", "monster_headcrab", "monster_leech", "monster_penguin"
};

const char* grapple_medium[] =
{
	"monster_alien_controller", "monster_alien_slave", "monster_barney",
	"monster_bullchicken", "monster_cleansuit_scientist", "monster_houndeye",
	"monster_human_assassin", "monster_human_grunt", "monster_human_grunt_ally",
	"monster_human_medic_ally", "monster_human_torch_ally", "monster_male_assassin",
	"monster_otis", "monster_pitdrone", "monster_scientist", "monster_zombie",
	"monster_zombie_barney", "monster_zombie_soldier"
};

const char* grapple_large[] =
{
	"monster_alien_grunt", "monster_alien_voltigore", "monster_assassin_repel",
	"monster_grunt_ally_repel", "monster_bigmomma", "monster_gargantua",
	"monster_geneworm", "monster_gonome", "monster_grunt_repel", "monster_ichthyosaur",
	"monster_nihilanth", "monster_pitworm", "monster_pitworm_up", "monster_shocktrooper"
};

const char* grapple_fixed[] =
{
	"monster_barnacle", "monster_sitting_cleansuit_scientist",
	"monster_sitting_scientist", "monster_tentacle", "ammo_spore"
};

#ifndef CLIENT_DLL
TYPEDESCRIPTION CGrapple::m_SaveData[] =
{
	DEFINE_FIELD( CGrapple, m_pBeam, FIELD_CLASSPTR ),
	DEFINE_FIELD( CGrapple, m_shootTime, FIELD_TIME ),
	DEFINE_FIELD( CGrapple, m_fireState, FIELD_INTEGER )
};

IMPLEMENT_SAVERESTORE( CGrapple, CBasePlayerWeapon );
#endif

static bool GrappleFastAttack()
{
#ifdef CLIENT_DLL
	return bIsMultiplayer() && ( giTeamplay == 2 || gEngfuncs.pfnGetCvarFloat( "sv_oldgrapple" ) != 1 );
#else
	return g_pGameRules->IsMultiplayer() && ( g_pGameRules->IsCTF() || oldgrapple.value != 1 );
#endif
}

static bool GrappleMultiplayer()
{
#ifdef CLIENT_DLL
	return bIsMultiplayer();
#else
	return g_pGameRules->IsMultiplayer() != FALSE;
#endif
}

void CGrapple::Precache()
{
	PRECACHE_MODEL( "models/v_bgrap.mdl" );
	PRECACHE_MODEL( "models/w_bgrap.mdl" );
	PRECACHE_MODEL( "models/p_bgrap.mdl" );
	PRECACHE_SOUND( "weapons/bgrapple_release.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_impact.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_fire.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_cough.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_pull.wav" );
	PRECACHE_SOUND( "weapons/bgrapple_wait.wav" );
	PRECACHE_SOUND( "weapons/alienweap_draw.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew1.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew2.wav" );
	PRECACHE_SOUND( "barnacle/bcl_chew3.wav" );
	PRECACHE_MODEL( "sprites/tongue.spr" );
	UTIL_PrecacheOther( "grapple_tip" );
}

void CGrapple::Spawn()
{
	Precache();
	m_iId = WEAPON_GRAPPLE;
	SET_MODEL( ENT( pev ), "models/w_bgrap.mdl" );
	m_iClip = -1;
	m_pTip = NULL;
	m_bGrappling = false;
	FallInit();
}

int CGrapple::GetItemInfo( ItemInfo* info )
{
	info->pszName = STRING( pev->classname );
	info->pszAmmo1 = NULL;
	info->iMaxAmmo1 = -1;
	info->pszAmmo2 = NULL;
	info->iMaxAmmo2 = -1;
	info->iMaxClip = -1;
	info->iSlot = 0;
	info->iPosition = 3;
	m_iId = info->iId = WEAPON_GRAPPLE;
	info->iFlags = 0;
	info->iWeight = 21;
	return 1;
}

BOOL CGrapple::Deploy()
{
	return DefaultDeploy( "models/v_bgrap.mdl", "models/p_bgrap.mdl", GRAPPLE_DRAW, "gauss", 0 );
}

int CGrapple::AddToPlayer( CBasePlayer* player )
{
	if ( !CBasePlayerWeapon::AddToPlayer( player ) )
		return FALSE;
	MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, player->pev );
		WRITE_BYTE( m_iId );
	MESSAGE_END();
	return TRUE;
}

BOOL CGrapple::UseDecrement()
{
#ifdef CLIENT_DLL
	return giOldWeapons != 1;
#else
	return oldweapons.value != 1;
#endif
}

void CGrapple::Holster( int )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( GRAPPLE_HOLSTER, 1 );
	if ( m_fireState != FIRE_OFF )
		EndAttack();
}

void CGrapple::EndAttack()
{
	m_fireState = FIRE_OFF;
	SendWeaponAnim( GRAPPLE_RELEASE, 1 );
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125 );
	EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0, 0, SND_STOP, PITCH_NORM );
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase();
	m_flNextPrimaryAttack = m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + ( GrappleFastAttack() ? 0 : 1 );
	DestroyEffect();
	if ( m_bGrappling && m_pPlayer->IsAlive() )
		m_pPlayer->SetAnimation( PLAYER_IDLE );
	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->flags &= ~FL_IMMUNE_LAVA;
}

void CGrapple::WeaponIdle()
{
	ResetEmptySound();
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;
	if ( m_fireState != FIRE_OFF )
		EndAttack();
	m_bMissed = false;
	float choice = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	int animation;
	if ( choice <= 0.5 )
	{
		animation = GRAPPLE_LONGIDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10;
	}
	else if ( choice > 0.95 )
	{
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_cough.wav", 1, ATTN_NORM, 0, PITCH_NORM );
		animation = GRAPPLE_COUGH;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 4.6;
	}
	else
	{
		animation = GRAPPLE_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 77.0 / 30.0;
	}
	SendWeaponAnim( animation, 1 );
}

void CGrapple::SecondaryAttack()
{
#ifndef CLIENT_DLL
	if ( m_pTip && m_pTip->m_bIsStuck && ( m_pTip->m_TargetEdict->free || CBaseEntity::Instance( m_pTip->m_TargetEdict )->IsPlayer() ) )
	{
		EndAttack();
		return;
	}
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	m_pPlayer->GetGunPosition();
	m_pPlayer->pev->movetype = MOVETYPE_WALK;
	m_pPlayer->pev->flags &= ~FL_IMMUNE_LAVA;
#endif
}

void CGrapple::CreateEffect()
{
#ifndef CLIENT_DLL
	DestroyEffect();
	m_pTip = CGrappleTip::CreateGrappleTip();
	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector origin = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 16 + gpGlobals->v_right * 8 - gpGlobals->v_up * 8;
	Vector angles = m_pPlayer->pev->v_angle;
	angles.x = -angles.x;
	m_pTip->SetPosition( origin, angles, m_pPlayer );
	if ( !m_pBeam )
	{
		m_pBeam = CBeam::BeamCreate( "sprites/tongue.spr", 12 );
		m_pBeam->EntsInit( m_pTip->entindex(), m_pPlayer->entindex() );
		m_pBeam->SetFlags( BEAM_FSOLID );
		m_pBeam->SetBrightness( 100 );
		m_pBeam->SetEndAttachment( 1 );
		m_pBeam->pev->spawnflags |= SF_BEAM_TEMPORARY;
	}
#endif
}

void CGrapple::UpdateEffect( const Vector&, const Vector&, float )
{
#ifndef CLIENT_DLL
	if ( !m_pBeam || !m_pTip )
		CreateEffect();
#endif
}

void CGrapple::DestroyEffect()
{
	if ( m_pBeam )
	{
		UTIL_Remove( m_pBeam );
		m_pBeam = NULL;
	}
	if ( m_pTip )
	{
		m_pTip->Killed( NULL, GIB_ALWAYS );
		m_pTip = NULL;
	}
}

void CGrapple::Fire( const Vector& source, const Vector& direction )
{
#ifndef CLIENT_DLL
	TraceResult trace;
	UTIL_TraceLine( source, source + direction * 2048, dont_ignore_monsters, m_pPlayer->edict(), &trace );
	if ( !trace.fAllSolid && CBaseEntity::Instance( trace.pHit ) )
	{
		if ( !m_pBeam || !m_pTip )
			CreateEffect();
		m_flDamageTime = gpGlobals->time;
	}
#else
	(void)source;
	(void)direction;
#endif
}

void CGrapple::PrimaryAttack()
{
	if ( m_bMissed )
	{
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		return;
	}
	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );
	Vector direction = gpGlobals->v_forward;
	Vector source = m_pPlayer->GetGunPosition();
	if ( m_pTip )
	{
		if ( m_pTip->m_bIsStuck )
		{
			edict_t* targetEdict = m_pTip->m_TargetEdict;
			CBaseEntity* target = CBaseEntity::Instance( targetEdict );
			if ( targetEdict->free || ( target->IsPlayer() && !target->IsAlive() ) )
			{
				EndAttack();
				return;
			}
			if ( m_pTip->m_iGrappleType > GRAPPLE_SMALL )
			{
				m_pPlayer->pev->movetype = MOVETYPE_FLY;
				m_pPlayer->pev->flags |= FL_IMMUNE_LAVA;
			}
			if ( m_MomentaryStuck )
			{
				SendWeaponAnim( GRAPPLE_TRAVEL, 1 );
				EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );
				if ( target->IsPlayer() )
					EMIT_SOUND_DYN( target->edict(), CHAN_STATIC, "weapons/bgrapple_impact.wav", 0.98, ATTN_NORM, 0, 125 );
				m_MomentaryStuck = 0;
			}
			int type = m_pTip->m_iGrappleType;
			if ( type >= GRAPPLE_SMALL && type <= GRAPPLE_LARGE )
				m_pTip->pev->origin = m_pTip->m_pGrappleTarget->Center();
			if ( type == GRAPPLE_SMALL )
			{
				entvars_t* targetVars = m_pTip->m_pGrappleTarget->pev;
				targetVars->velocity = targetVars->velocity + m_pPlayer->pev->origin - targetVars->origin;
				if ( targetVars->velocity.Length() > 450 )
					targetVars->velocity = targetVars->velocity.Normalize() * 450;
			}
			else if ( type >= GRAPPLE_MEDIUM && type <= GRAPPLE_FIXED )
			{
				m_pPlayer->pev->velocity = m_pPlayer->pev->velocity + m_pTip->pev->origin - m_pPlayer->pev->origin;
				if ( m_pPlayer->pev->velocity.Length() <= 450 )
				{
					m_bGrappling = false;
					m_pPlayer->SetAnimation( PLAYER_IDLE );
				}
				else
				{
					m_pPlayer->pev->velocity = m_pPlayer->pev->velocity.Normalize() * 450;
					float pitch = UTIL_VecToAngles( m_pPlayer->pev->velocity ).x;
					m_bGrappling = !( ( pitch > 55 && pitch < 205 ) || pitch < -55 );
					m_pPlayer->SetAnimation( m_bGrappling ? PLAYER_GRAPPLE : PLAYER_IDLE );
				}
			}
		}
		if ( m_pTip->m_bMissed )
		{
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_release.wav", 0.98, ATTN_NORM, 0, 125 );
			EndAttack();
			return;
		}
	}
	if ( m_fireState == FIRE_OFF )
	{
		m_MomentaryStuck = 1;
		SendWeaponAnim( GRAPPLE_FIRE, 1 );
		m_pPlayer->m_iWeaponVolume = 450;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.1;
		m_shootTime = gpGlobals->time + ( GrappleMultiplayer() ? 0 : 0.35 );
		EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_WEAPON, "weapons/bgrapple_fire.wav", 0.98, ATTN_NORM, 0, 125 );
		m_fireState = FIRE_CHARGE;
	}
	else if ( m_fireState == FIRE_CHARGE )
	{
		m_pPlayer->m_iWeaponVolume = 450;
		if ( m_shootTime != 0 && gpGlobals->time > m_shootTime )
		{
			SendWeaponAnim( GRAPPLE_FIREWAIT, 1 );
			m_pPlayer->pev->punchangle.x += 2;
			Fire( source, direction );
			EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_STATIC, "weapons/bgrapple_pull.wav", 0.98, ATTN_NORM, 0, 125 );
			m_shootTime = 0;
		}
	}
	if ( !m_pTip )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		return;
	}
	if ( m_pTip->m_iGrappleType != GRAPPLE_FIXED && m_pTip->m_bIsStuck )
	{
		UTIL_MakeVectors( m_pPlayer->pev->v_angle );
		source = m_pPlayer->GetGunPosition();
		Vector end = source + gpGlobals->v_forward * 16;
		TraceResult trace;
		UTIL_TraceLine( source, end, dont_ignore_monsters, m_pPlayer->edict(), &trace );
#ifndef CLIENT_DLL
		if ( trace.flFraction >= 1 )
		{
			UTIL_TraceHull( source, end, dont_ignore_monsters, head_hull, m_pPlayer->edict(), &trace );
			if ( trace.flFraction < 1 )
			{
				CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
				if ( !hit || hit->IsBSPModel() )
					FindHullIntersection( source, trace, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict() );
			}
		}
#endif
		if ( trace.flFraction < 1 )
		{
			CBaseEntity* hit = CBaseEntity::Instance( trace.pHit );
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
			if ( hit && m_pTip->ClassifyTarget( hit ) != GRAPPLE_NOTARGET && gpGlobals->time > m_flDamageTime + 0.5 )
			{
				ClearMultiDamage();
				float damage = gSkillData.plrDmgGrapple * ( GrappleMultiplayer() ? 2 : 1 );
				hit->TraceAttack( m_pPlayer->pev, damage, gpGlobals->v_forward, &trace, DMG_CLUB | DMG_ALWAYSGIB );
				ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );
				m_flDamageTime = gpGlobals->time;
				const char* sounds[] = { "barnacle/bcl_chew1.wav", "barnacle/bcl_chew2.wav", "barnacle/bcl_chew3.wav" };
				EMIT_SOUND_DYN( m_pPlayer->edict(), CHAN_VOICE, sounds[RANDOM_LONG( 0, 2 )], 1, ATTN_NORM, 0, 125 );
			}
		}
	}
	if ( GrappleFastAttack() )
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase();
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase();
	}
	else
	{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.01;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
	}
}

void CGrappleTip::Precache()
{
	PRECACHE_MODEL( "models/shock_effect.mdl" );
}

void CGrappleTip::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( ENT( pev ), "models/shock_effect.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "grapple_tip" );
	SetThink( &CGrappleTip::FlyThink );
	SetTouch( &CGrappleTip::TongueTouch );
	pev->angles.x -= 30;
	UTIL_MakeVectors( pev->angles );
	pev->angles.x = -( 30 + pev->angles.x );
	pev->velocity = gpGlobals->v_forward * 0;
	pev->gravity = 1;
	pev->nextthink = gpGlobals->time + 0.02;
	m_bIsStuck = false;
	m_bMissed = false;
}

CGrappleTip* CGrappleTip::CreateGrappleTip()
{
	CGrappleTip* tip = GetClassPtr( (CGrappleTip*)NULL );
	tip->Spawn();
	return tip;
}

void CGrappleTip::SetPosition( Vector origin, Vector angles, CBaseEntity* owner )
{
	UTIL_SetOrigin( pev, origin );
	pev->angles = angles;
	pev->owner = owner->edict();
}

void CGrappleTip::FlyThink()
{
#ifndef CLIENT_DLL
	UTIL_MakeAimVectors( pev->angles );
	Vector direction = gpGlobals->v_forward;
	pev->angles = UTIL_VecToAngles( direction );
	float speed = pev->velocity.Length() * 0.8 + 400;
	pev->velocity = direction * speed + pev->velocity * 0.2;
	float maximum;
	if ( !g_pGameRules->IsMultiplayer() )
		maximum = oldgrapple.value == 1 ? 1600 : 750;
	else
		maximum = !g_pGameRules->IsCTF() && oldgrapple.value == 1 ? 1600 : 2000;
	if ( pev->velocity.Length() > maximum )
		pev->velocity = pev->velocity.Normalize() * maximum;
#endif
	pev->nextthink = gpGlobals->time + 0.02;
}

void CGrappleTip::OffsetThink()
{
}

int CGrappleTip::ClassifyTarget( CBaseEntity* target )
{
	if ( !target )
		return GRAPPLE_NOTARGET;
	if ( target->IsPlayer() )
	{
		m_pGrappleTarget = target;
		m_TargetEdict = target->edict();
		return GRAPPLE_MEDIUM;
	}
	TraceResult trace;
	Vector start = pev->origin;
	Vector end = start + pev->velocity * 1024;
	UTIL_TraceLine( start, end, ignore_monsters, edict(), &trace );
	const char* texture = TRACE_TEXTURE( trace.pHit ? trace.pHit : INDEXENT( 0 ), start, end );
	int type = GRAPPLE_NOTARGET;
	if ( texture && !strnicmp( texture, "xeno_grapple", 12 ) )
		type = GRAPPLE_FIXED;
	else
	{
		const char* classname = STRING( target->pev->classname );
		for ( int i = 0; i < ARRAYSIZE( grapple_small ); i++ )
		{
			if ( !strcmp( classname, grapple_small[i] ) )
			{
				type = GRAPPLE_SMALL;
				break;
			}
		}
		for ( int i = 0; type == GRAPPLE_NOTARGET && i < ARRAYSIZE( grapple_medium ); i++ )
		{
			if ( !strcmp( classname, grapple_medium[i] ) )
				type = GRAPPLE_MEDIUM;
		}
		for ( int i = 0; type == GRAPPLE_NOTARGET && i < ARRAYSIZE( grapple_large ); i++ )
		{
			if ( !strcmp( classname, grapple_large[i] ) )
				type = GRAPPLE_LARGE;
		}
		for ( int i = 0; type == GRAPPLE_NOTARGET && i < ARRAYSIZE( grapple_fixed ); i++ )
		{
			if ( !strcmp( classname, grapple_fixed[i] ) )
				type = GRAPPLE_FIXED;
		}
	}
	if ( type != GRAPPLE_NOTARGET )
	{
		m_pGrappleTarget = target;
		m_TargetEdict = target->edict();
		if ( type != GRAPPLE_FIXED )
			m_OriginOffset = pev->origin - target->pev->origin;
	}
	return type;
}

void CGrappleTip::TongueTouch( CBaseEntity* other )
{
	int type = ClassifyTarget( other );
	pev->velocity = pev->velocity * 0;
	if ( type )
		m_bIsStuck = true;
	else
		m_bMissed = true;
	m_iGrappleType = type;
	pev->solid = SOLID_NOT;
	SetThink( &CGrappleTip::OffsetThink );
	pev->nextthink = gpGlobals->time + 0.02;
	SetTouch( NULL );
}
