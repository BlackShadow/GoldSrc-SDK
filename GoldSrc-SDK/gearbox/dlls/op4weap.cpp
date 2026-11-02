#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "effects.h"
#include "customentity.h"
#include "op4weap.h"
#include "player.h"
#include "gamerules.h"
#include "ofctf.h"
#include "soundent.h"
#include "decals.h"

extern int gmsgCTFScore;

LINK_ENTITY_TO_CLASS( shock_beam, CShockBeam );
LINK_ENTITY_TO_CLASS( displacer_ball, CDisplacerBall );
LINK_ENTITY_TO_CLASS( spore, CSpore );

const char* displace[] =
{
	"monster_bloater",
	"monster_snark",
	"monster_shockroach",
	"monster_rat",
	"monster_alien_babyvoltigore",
	"monster_babycrab",
	"monster_cockroach",
	"monster_flyer_flock",
	"monster_headcrab",
	"monster_leech",
	"monster_alien_controller",
	"monster_alien_slave",
	"monster_barney",
	"monster_bullchicken",
	"monster_cleansuit_scientist",
	"monster_houndeye",
	"monster_human_assassin",
	"monster_human_grunt",
	"monster_human_grunt_ally",
	"monster_human_medic_ally",
	"monster_human_torch_ally",
	"monster_male_assassin",
	"monster_otis",
	"monster_pitdrone",
	"monster_scientist",
	"monster_zombie",
	"monster_zombie_barney",
	"monster_zombie_soldier",
	"monster_alien_grunt",
	"monster_alien_voltigore",
	"monster_assassin_repel",
	"monster_grunt_ally_repel",
	"monster_gonome",
	"monster_grunt_repel",
	"monster_ichthyosaur",
	"monster_shocktrooper",
};

void CShockBeam::Precache()
{
	PRECACHE_MODEL( "sprites/flare3.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	PRECACHE_MODEL( "sprites/glow01.spr" );
	PRECACHE_MODEL( "models/shock_effect.mdl" );
	PRECACHE_SOUND( "weapons/shock_impact.wav" );
}

void CShockBeam::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/shock_effect.mdl" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, Vector( -4, -4, -4 ), Vector( 4, 4, 4 ) );
	SetTouch( &CShockBeam::BallTouch );
	SetThink( &CShockBeam::FlyThink );
	m_pSprite = CSprite::SpriteCreate( "sprites/flare3.spr", pev->origin, FALSE );
	m_pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxDistort );
	m_pSprite->SetScale( 0.35 );
	m_pSprite->SetAttachment( edict(), 0 );
	m_pBeam1 = CBeam::BeamCreate( "sprites/lgtning.spr", 60 );
	if ( !m_pBeam1 )
		return;
	UTIL_SetOrigin( m_pBeam1->pev, pev->origin );
	m_pBeam1->EntsInit( entindex(), entindex() );
	m_pBeam1->SetStartAttachment( 1 );
	m_pBeam1->SetEndAttachment( 2 );
	m_pBeam1->SetColor( 0, 253, 253 );
	m_pBeam1->SetFlags( BEAM_FSHADEOUT );
	m_pBeam1->SetBrightness( 180 );
	m_pBeam1->SetNoise( 0 );
	m_pBeam1->SetScrollRate( 10 );
	if ( !g_pGameRules->IsMultiplayer() )
	{
		m_pBeam2 = CBeam::BeamCreate( "sprites/lgtning.spr", 20 );
		if ( !m_pBeam2 )
			return;
		UTIL_SetOrigin( m_pBeam2->pev, pev->origin );
		m_pBeam2->EntsInit( entindex(), entindex() );
		m_pBeam2->SetStartAttachment( 1 );
		m_pBeam2->SetEndAttachment( 2 );
		m_pBeam2->SetColor( 255, 255, 157 );
		m_pBeam2->SetFlags( BEAM_FSHADEOUT );
		m_pBeam2->SetBrightness( 180 );
		m_pBeam2->SetNoise( 30 );
		m_pBeam2->SetScrollRate( 30 );
	}
	pev->nextthink = gpGlobals->time + 0.01;
}

void CShockBeam::FlyThink()
{
	if ( pev->waterlevel == 3 )
		SetThink( &CShockBeam::WaterExplodeThink );
	pev->nextthink = gpGlobals->time + 0.01;
}

void CShockBeam::ExplodeThink()
{
	UTIL_PointContents( pev->origin );
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
	if ( m_pBeam1 )
	{
		UTIL_Remove( m_pBeam1 );
		m_pBeam1 = NULL;
	}
	if ( !g_pGameRules->IsMultiplayer() && m_pBeam2 )
	{
		UTIL_Remove( m_pBeam2 );
		m_pBeam2 = NULL;
	}
	pev->dmg = 40;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE( 8 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 253 );
	WRITE_BYTE( 253 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 10 );
	MESSAGE_END();
	pev->owner = NULL;
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/shock_impact.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, 100 );
	UTIL_Remove( this );
}

void CShockBeam::WaterExplodeThink()
{
	UTIL_PointContents( pev->origin );
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
	if ( m_pBeam1 )
	{
		UTIL_Remove( m_pBeam1 );
		m_pBeam1 = NULL;
	}
	if ( !g_pGameRules->IsMultiplayer() && m_pBeam2 )
	{
		UTIL_Remove( m_pBeam2 );
		m_pBeam2 = NULL;
	}
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE( 8 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 253 );
	WRITE_BYTE( 253 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 10 );
	MESSAGE_END();
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	::RadiusDamage( pev->origin, pev, pevAttacker, 100, 150, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/shock_impact.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, 100 );
	UTIL_Remove( this );
}

void CShockBeam::BallTouch( CBaseEntity* pOther )
{
	SetTouch( NULL );
	SetThink( NULL );
	TraceResult tr;
	if ( pOther->pev->takedamage )
	{
		tr = UTIL_GetGlobalTrace();
		entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
		ClearMultiDamage();
		BOOL multiplayer = g_pGameRules->IsMultiplayer();
		BOOL player = pOther->IsPlayer();
		CBaseMonster* pMonster = pOther->MyMonsterPointer();
		float damage = multiplayer ? gSkillData.plrDmgShockRoachM : gSkillData.plrDmgShockRoachS;
		Vector direction = pev->velocity.Normalize();
		if ( pMonster )
		{
			int damageType = pMonster->m_flShockDuration > 1 ? DMG_ALWAYSGIB : DMG_BLAST;
			if ( player )
				pMonster->TraceAttack( pevAttacker, damage, direction, &tr, damageType );
			else
				pOther->TraceAttack( pevAttacker, damage, direction, &tr, damageType );
			pMonster->AddShockEffect( 63, 152, 208, 16, 0.5 );
		}
		else
		{
			pOther->TraceAttack( pevAttacker, damage, direction, &tr, DMG_SHOCK | DMG_ALWAYSGIB );
		}
		ApplyMultiDamage( pev, pevAttacker );
		pev->velocity = g_vecZero;
	}
	SetThink( &CShockBeam::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.01;
	if ( !pOther->pev->takedamage )
	{
		UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr );
		UTIL_DecalTrace( &tr, DECAL_OFSCORCH1 + RANDOM_LONG( 0, 2 ) );
		MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
		WRITE_BYTE( TE_SPARKS );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		MESSAGE_END();
	}
}

CShockBeam* CShockBeam::CreateShockBeam( Vector origin, Vector angles, CBaseEntity* owner )
{
	CShockBeam* pBeam = GetClassPtr( (CShockBeam*)NULL );
	angles.x = -angles.x;
	UTIL_SetOrigin( pBeam->pev, origin );
	pBeam->pev->angles = angles;
	UTIL_MakeVectors( angles );
	pBeam->pev->velocity = gpGlobals->v_forward * 2000;
	pBeam->pev->velocity.z = -pBeam->pev->velocity.z;
	pBeam->pev->classname = MAKE_STRING( "shock_beam" );
	pBeam->Spawn();
	pBeam->pev->owner = owner->edict();
	return pBeam;
}

void CDisplacerBall::Precache()
{
	PRECACHE_MODEL( "sprites/exit1.spr" );
	PRECACHE_MODEL( "sprites/exit1.spr" );
	PRECACHE_MODEL( "sprites/lgtning.spr" );
	m_iTrail = PRECACHE_MODEL( "sprites/disp_ring.spr" );
	PRECACHE_SOUND( "weapons/displacer_impact.wav" );
	PRECACHE_SOUND( "weapons/displacer_teleport.wav" );
}

void CDisplacerBall::Spawn()
{
	Precache();
	pev->movetype = MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "sprites/exit1.spr" );
	UTIL_SetOrigin( pev, pev->origin );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	pev->rendermode = kRenderTransAdd;
	pev->renderamt = 255;
	pev->scale = 0.75;
	SetTouch( &CDisplacerBall::BallTouch );
	SetThink( &CDisplacerBall::FlyThink );
	pev->nextthink = gpGlobals->time + 0.2;
	InitBeams();
}

void CDisplacerBall::InitBeams()
{
	for ( int i = 0; i < 8; i++ )
		m_pBeam[i] = NULL;
	m_iBeams = 0;
	pev->skin = 0;
}

void CDisplacerBall::ClearBeams()
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

void CDisplacerBall::FlyThink()
{
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::FlyThink2()
{
	UTIL_SetSize( pev, Vector( -8, -8, -8 ), Vector( 8, 8, 8 ) );
	ArmBeam( -1 );
	ArmBeam( 1 );
	pev->nextthink = gpGlobals->time + 0.05;
}

void CDisplacerBall::ArmBeam( int side )
{
	if ( m_iBeams > 7 )
		m_iBeams = 0;
	UTIL_MakeAimVectors( pev->angles );
	Vector vecSrc = pev->origin + gpGlobals->v_up * 36 + gpGlobals->v_right * side * 16 + gpGlobals->v_forward * 32;
	TraceResult tr;
	float flDist = 1;
	for ( int i = 0; i < 3; i++ )
	{
		Vector vecAim = gpGlobals->v_up * RANDOM_FLOAT( -1, 1 );
		vecAim = gpGlobals->v_right * (float)side * RANDOM_FLOAT( 0, 1 ) + vecAim;
		TraceResult trTest;
		UTIL_TraceLine( vecSrc, vecSrc + vecAim * 512, dont_ignore_monsters, edict(), &trTest );
		if ( trTest.flFraction < flDist )
		{
			tr = trTest;
			flDist = tr.flFraction;
		}
	}
	if ( flDist == 1 )
		return;
	if ( !m_pBeam[m_iBeams] )
		m_pBeam[m_iBeams] = CBeam::BeamCreate( "sprites/lgtning.spr", 30 );
	if ( !m_pBeam[m_iBeams] )
		return;

	CBaseEntity* pHit = CBaseEntity::Instance( tr.pHit );
	if ( pHit && pHit->pev->takedamage )
	{
		entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
		m_pBeam[m_iBeams]->EntsInit( pHit->entindex(), entindex() );
		m_pBeam[m_iBeams]->SetColor( 255, 255, 255 );
		m_pBeam[m_iBeams]->SetBrightness( 255 );
		m_pBeam[m_iBeams]->SetNoise( 10 );
		::RadiusDamage( tr.vecEndPos, pev, pevAttacker, 25, 15, CLASS_NONE, DMG_ENERGYBEAM );
	}
	else
	{
		m_pBeam[m_iBeams]->PointEntInit( tr.vecEndPos, entindex() );
		m_pBeam[m_iBeams]->SetColor( 96, 128, 16 );
		m_pBeam[m_iBeams]->SetBrightness( 255 );
		m_pBeam[m_iBeams]->SetNoise( 80 );
	}
	m_iBeams++;
}

int CDisplacerBall::ClassifyTarget( CBaseEntity* target )
{
	if ( !target )
		return 0;
	for ( int i = 0; i < ARRAYSIZE( displace ); i++ )
	{
		if ( !target->IsPlayer() && FClassnameIs( target->pev, displace[i] ) )
			return 1;
	}
	return 0;
}

CDisplacerBall* CDisplacerBall::CreateDisplacerBall( Vector origin, Vector angles, CBaseEntity* owner )
{
	CDisplacerBall* pBall = GetClassPtr( (CDisplacerBall*)NULL );
	angles.x = -angles.x;
	UTIL_SetOrigin( pBall->pev, origin );
	pBall->pev->angles = angles;
	UTIL_MakeVectors( angles );
	pBall->pev->velocity = gpGlobals->v_forward * 500;
	pBall->pev->velocity.z = -pBall->pev->velocity.z;
	pBall->pev->classname = MAKE_STRING( "displacer_ball" );
	pBall->pev->owner = owner->edict();
	pBall->Spawn();
	return pBall;
}

void CDisplacerBall::KillThink()
{
	if ( m_pDisplacedTarget )
		m_pDisplacedTarget->SetThink( &CBaseEntity::SUB_Remove );
	SetThink( &CDisplacerBall::ExplodeThink );
	pev->nextthink = gpGlobals->time + 0.2;
}

void CDisplacerBall::ExplodeThink()
{
	UTIL_PointContents( pev->origin );
	ClearBeams();
	pev->dmg = gSkillData.plrDmgDisplacerOther;
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	::RadiusDamage( pev->origin, pev, pevAttacker, pev->dmg, gSkillData.plrRadiusDisplacer, CLASS_NONE,
		DMG_ENERGYBEAM | DMG_ALWAYSGIB );
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/displacer_teleport.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, 100 );
	UTIL_Remove( this );
}

void CDisplacerBall::FizzleThink()
{
	UTIL_PointContents( pev->origin );
	ClearBeams();
	pev->dmg = gSkillData.plrDmgDisplacerOther;
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE( 16 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 180 );
	WRITE_BYTE( 96 );
	WRITE_BYTE( 10 );
	WRITE_BYTE( 10 );
	MESSAGE_END();
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	::RadiusDamage( pev->origin, pev, pevAttacker, pev->dmg, 128, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/displacer_impact.wav", RANDOM_FLOAT( 0.8, 0.9 ), ATTN_NORM, 0, 100 );
	UTIL_Remove( this );
}

void CDisplacerBall::BallTouch( CBaseEntity* other )
{
	pev->velocity = g_vecZero;
	MESSAGE_BEGIN( MSG_PAS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_BEAMCYLINDER );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z + 800 );
	WRITE_SHORT( m_iTrail );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 3 );
	WRITE_BYTE( 16 );
	WRITE_BYTE( 0 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 0 );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE( 16 );
	WRITE_BYTE( 255 );
	WRITE_BYTE( 180 );
	WRITE_BYTE( 96 );
	WRITE_BYTE( 10 );
	WRITE_BYTE( 10 );
	MESSAGE_END();
	m_pDisplacedTarget = NULL;
	SetTouch( NULL );
	SetThink( NULL );
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + pev->velocity * 10, dont_ignore_monsters, edict(), &tr );
	UTIL_DecalTrace( &tr, DECAL_SCORCH1 + RANDOM_LONG( 0, 1 ) );
	if ( other->IsPlayer() )
	{
		CBasePlayer* player = (CBasePlayer*)other;
		player->pev->flags = FL_CLIENT;
		player->m_flFallVelocity = 0;
		if ( g_pGameRules->IsCTF() && player->m_pFlag != NULL )
		{
			( (CTFGoalFlag*)(CBaseEntity*)player->m_pFlag )->DropFlag( player );
			CBaseEntity* attacker = CBaseEntity::Instance( pev->owner );
			if ( attacker->IsPlayer() && ( (CBasePlayer*)attacker )->m_iTeamNum != player->m_iTeamNum )
			{
				CBasePlayer* scorer = (CBasePlayer*)attacker;
				scorer->m_iCTFScore++;
				scorer->m_iDefense++;
				MESSAGE_BEGIN( MSG_ALL, gmsgCTFScore );
				WRITE_BYTE( scorer->entindex() );
				WRITE_BYTE( scorer->m_iCTFScore );
				MESSAGE_END();
				ClientPrint( scorer->pev, HUD_PRINTTALK, "#CTFScorePoint" );
				UTIL_ClientPrintAll( HUD_PRINTNOTIFY, UTIL_VarArgs( "%s", STRING( scorer->pev->netname ) ) );
				if ( player->m_iTeamNum == 1 )
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagDisplacedBM" );
				else if ( player->m_iTeamNum == 2 )
					UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "#CTFFlagDisplacedOF" );
			}
		}
		edict_t* spawn = g_pGameRules->GetPlayerSpawnSpot( player );
		Vector vecEnd = VARS( spawn )->origin;
		vecEnd.z -= 100;
		UTIL_TraceLine( VARS( spawn )->origin, vecEnd, ignore_monsters, edict(), &tr );
		player->pev->origin = tr.vecEndPos;
		player->pev->origin.z += 36;
		player->pev->origin.z += 1;
		player->pev->sequence = player->LookupActivity( ACT_IDLE );
		player->m_lastx = 0;
		player->m_fNoPlayerSound = FALSE;
		player->m_lasty = 0;
		player->mIsClimbing = 0;
	}
	if ( ClassifyTarget( other ) )
	{
		UTIL_GetGlobalTrace();
		ClearMultiDamage();
		m_pDisplacedTarget = other;
		other->Killed( pev, GIB_NEVER );
	}
	pev->basevelocity = g_vecZero;
	pev->velocity = pev->basevelocity;
	pev->solid = SOLID_NOT;
	UTIL_SetOrigin( pev, pev->origin );
	SetThink( &CDisplacerBall::KillThink );
	pev->nextthink = gpGlobals->time + ( g_pGameRules->IsMultiplayer() ? 0.2 : 0.5 );
}

TYPEDESCRIPTION CSpore::m_SaveData[] =
{
	DEFINE_FIELD( CSpore, m_flIgniteTime, FIELD_TIME ),
};

IMPLEMENT_SAVERESTORE( CSpore, CGrenade );

void CSpore::Precache()
{
	PRECACHE_MODEL( "models/spore.mdl" );
	PRECACHE_MODEL( "sprites/glow01.spr" );
	m_iBlow = PRECACHE_MODEL( "sprites/spore_exp_01.spr" );
	m_iBlowSmall = PRECACHE_MODEL( "sprites/spore_exp_c_01.spr" );
	m_iSpitSprite = PRECACHE_MODEL( "sprites/tinyspit.spr" );
	m_iTrail = m_iSpitSprite;
	PRECACHE_SOUND( "weapons/splauncher_impact.wav" );
	PRECACHE_SOUND( "weapons/splauncher_bounce.wav" );
}

void CSpore::Spawn()
{
	Precache();
	m_iGibCount = 0;
	pev->movetype = m_iSporeType == 2 ? MOVETYPE_BOUNCE : MOVETYPE_FLY;
	pev->solid = SOLID_BBOX;
	SET_MODEL( edict(), "models/spore.mdl" );
	UTIL_SetSize( pev, g_vecZero, g_vecZero );
	UTIL_SetOrigin( pev, pev->origin );
	pev->classname = MAKE_STRING( "spore" );
	SetThink( &CSpore::FlyThink );
	if ( m_iSporeType == 2 )
	{
		SetTouch( &CSpore::MyBounceTouch );
		if ( !m_bPuked )
			pev->angles.x -= 30 + RANDOM_LONG( -5, 5 );
	}
	else
	{
		SetTouch( &CSpore::RocketTouch );
	}
	UTIL_MakeVectors( pev->angles );
	if ( !m_bIsAI )
	{
		if ( m_iSporeType != 2 )
			pev->velocity = gpGlobals->v_forward * 1200;
		pev->gravity = 1;
	}
	else
	{
		pev->gravity = 0.5;
		pev->friction = 0.7;
	}
	pev->dmg = gSkillData.plrDmgSpore;
	m_flIgniteTime = gpGlobals->time;
	pev->nextthink = gpGlobals->time + 0.01;
	m_pSprite = CSprite::SpriteCreate( "sprites/glow01.spr", pev->origin, FALSE );
	m_pSprite->SetTransparency( kRenderTransAdd, 180, 180, 40, 100, kRenderFxDistort );
	m_pSprite->SetScale( 0.8 );
	m_pSprite->SetAttachment( edict(), 0 );
	m_fRegisteredSound = FALSE;
	m_flSoundDelay = gpGlobals->time;
}

void CSpore::BounceSound()
{
}

void CSpore::GibThink()
{
}

CSpore* CSpore::CreateSpore( Vector origin, Vector angles, CBaseEntity* owner, int type, BOOL ai, BOOL puked )
{
	CSpore* pSpore = GetClassPtr( (CSpore*)NULL );
	UTIL_SetOrigin( pSpore->pev, origin );
	pSpore->m_iSporeType = type;
	if ( ai )
	{
		pSpore->pev->velocity = angles;
		pSpore->pev->angles = UTIL_VecToAngles( angles );
	}
	else
	{
		pSpore->pev->angles = angles;
	}
	pSpore->m_bIsAI = ai;
	pSpore->m_bPuked = puked;
	pSpore->Spawn();
	pSpore->pev->owner = owner->edict();
	pSpore->pev->classname = MAKE_STRING( "spore" );
	return pSpore;
}

void CSpore::FlyThink()
{
	float flFuse = m_bIsAI ? 4.0f : 2.0f;
	if ( m_iSporeType == 2 && gpGlobals->time > m_flIgniteTime + flFuse )
	{
		SetThink( &CSpore::IgniteThink );
	}
	else
	{
		Vector direction = pev->velocity.Normalize();
		MESSAGE_BEGIN( MSG_ALL, SVC_TEMPENTITY );
		WRITE_BYTE( TE_SPRITE_SPRAY );
		WRITE_COORD( pev->origin.x );
		WRITE_COORD( pev->origin.y );
		WRITE_COORD( pev->origin.z );
		WRITE_COORD( direction.x );
		WRITE_COORD( direction.y );
		WRITE_COORD( direction.z );
		WRITE_SHORT( m_iTrail );
		WRITE_BYTE( 2 );
		WRITE_BYTE( 20 );
		WRITE_BYTE( 80 );
		MESSAGE_END();
	}
	pev->nextthink = gpGlobals->time + 0.03;
}

void CSpore::IgniteThink()
{
	SetTouch( NULL );
	SetThink( NULL );
	if ( m_pSprite )
	{
		UTIL_Remove( m_pSprite );
		m_pSprite = NULL;
	}
	EMIT_SOUND_DYN( edict(), CHAN_WEAPON, "weapons/splauncher_impact.wav", 1, ATTN_NORM, 0, 100 );
	Vector direction = pev->velocity.Normalize();
	float distance = m_iSporeType == 2 ? 64.0f : 32.0f;
	TraceResult tr;
	UTIL_TraceLine( pev->origin, pev->origin + direction * distance, dont_ignore_monsters, edict(), &tr );
	UTIL_DecalTrace( &tr, DECAL_SPORESPLAT1 + RANDOM_LONG( 0, 2 ) );
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_SPRITE_SPRAY );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( tr.vecPlaneNormal.x );
	WRITE_COORD( tr.vecPlaneNormal.y );
	WRITE_COORD( tr.vecPlaneNormal.z );
	WRITE_SHORT( m_iSpitSprite );
	WRITE_BYTE( 100 );
	WRITE_BYTE( 40 );
	WRITE_BYTE( 180 );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_DLIGHT );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_BYTE( 10 );
	WRITE_BYTE( 15 );
	WRITE_BYTE( 220 );
	WRITE_BYTE( 40 );
	WRITE_BYTE( 5 );
	WRITE_BYTE( 10 );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_SPRITE );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_SHORT( RANDOM_LONG( 0, 1 ) ? m_iBlow : m_iBlowSmall );
	WRITE_BYTE( 20 );
	WRITE_BYTE( 128 );
	MESSAGE_END();
	MESSAGE_BEGIN( MSG_PVS, SVC_TEMPENTITY, pev->origin );
	WRITE_BYTE( TE_SPRITE_SPRAY );
	WRITE_COORD( pev->origin.x );
	WRITE_COORD( pev->origin.y );
	WRITE_COORD( pev->origin.z );
	WRITE_COORD( RANDOM_FLOAT( -1, 1 ) );
	WRITE_COORD( 1 );
	WRITE_COORD( RANDOM_FLOAT( -1, 1 ) );
	WRITE_SHORT( m_iTrail );
	WRITE_BYTE( 2 );
	WRITE_BYTE( 20 );
	WRITE_BYTE( 80 );
	MESSAGE_END();
	entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
	pev->owner = NULL;
	::RadiusDamage( pev->origin, pev, pevAttacker, pev->dmg, 200, CLASS_NONE, DMG_BLAST | DMG_ALWAYSGIB );
	SetThink( &CBaseEntity::SUB_Remove );
	pev->nextthink = gpGlobals->time;
}

void CSpore::RocketTouch( CBaseEntity* other )
{
	if ( other->pev->takedamage )
	{
		entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
		other->TakeDamage( pev, pevAttacker, gSkillData.plrDmgSpore, DMG_GENERIC );
	}
	IgniteThink();
}

void CSpore::MyBounceTouch( CBaseEntity* other )
{
	if ( other->pev->takedamage )
	{
		entvars_t* pevAttacker = pev->owner ? VARS( pev->owner ) : NULL;
		other->TakeDamage( pev, pevAttacker, gSkillData.plrDmgSpore, DMG_GENERIC );
		IgniteThink();
		return;
	}
	if ( pev->owner == other->edict() )
		return;
	if ( gpGlobals->time > m_flSoundDelay )
	{
		CSoundEnt::InsertSound( bits_SOUND_DANGER, pev->origin, (int)( pev->dmg / 0.4 ), 0.3 );
		m_flSoundDelay = gpGlobals->time + 1;
	}
	if ( pev->flags & FL_ONGROUND )
	{
		pev->velocity = pev->velocity * 0.5;
		pev->sequence = RANDOM_LONG( 1, 1 );
	}
	else
	{
		EMIT_SOUND_DYN( edict(), CHAN_VOICE, "weapons/splauncher_bounce.wav", 0.25, ATTN_NORM, 0, 100 );
	}
}
