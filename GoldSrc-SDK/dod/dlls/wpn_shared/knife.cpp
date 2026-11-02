#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "weapons.h"
#include "skill.h"
#include "gamerules.h"
#include "dod_gamerules.h"
#include "../../cl_dll/dod_shared.h"

void FindHullIntersection( const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity )
{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = { mins, maxs };
	TraceResult tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ( ( vecHullEnd - vecSrc ) * 2 );
	UTIL_TraceLine( vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace );
	if ( tmpTrace.flFraction < 1.0 )
	{
		tr = tmpTrace;
		return;
	}

	for ( i = 0; i < 2; i++ )
	{
		for ( j = 0; j < 2; j++ )
		{
			for ( k = 0; k < 2; k++ )
			{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace );
				if ( tmpTrace.flFraction < 1.0 )
				{
					float thisDistance = ( tmpTrace.vecEndPos - vecSrc ).Length();
					if ( thisDistance < distance )
					{
						tr = tmpTrace;
						distance = thisDistance;
					}
				}
			}
		}
	}
}

void CMeleeWeapon::Spawn( int weapon_id )
{
	m_iId = weapon_id;
	SET_MODEL( ENT( pev ), P_WpnInfo[m_iId].wmodel );
	m_iDefaultAmmo = P_WpnInfo[m_iId].ammo_default;
	FallInit();
	Precache();
}

void CMeleeWeapon::Precache()
{
	PRECACHE_SOUND( "weapons/knife_slash1.wav" );
	PRECACHE_SOUND( "weapons/knife_slash2.wav" );
	PRECACHE_SOUND( "weapons/knife_hitwall1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit1.wav" );
	PRECACHE_SOUND( "weapons/knife_hit2.wav" );
	PRECACHE_SOUND( "weapons/knife_hit3.wav" );
	PRECACHE_SOUND( "weapons/knife_hit4.wav" );
	PRECACHE_SOUND( "weapons/knifeswing.wav" );
	PRECACHE_SOUND( "weapons/knifeswing2.wav" );
	m_iFireEvent = PRECACHE_EVENT( 1, "events/weapons/melee.sc" );
}

int CMeleeWeapon::AddToPlayer( CBasePlayer* pPlayer )
{
#ifdef CLIENT_DLL
	return 0;
#else
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		if ( pPlayer->pev->team == 2 )
		{
			if ( pPlayer->IsParatrooper() && m_iId == WEAPON_GERKNIFE )
				SET_MODEL( ENT( pev ), P_WpnInfo[35].wmodel );
			else if ( m_iId == WEAPON_GERKNIFE )
				SET_MODEL( ENT( pev ), P_WpnInfo[WEAPON_GERKNIFE].wmodel );
		}

		return TRUE;
	}

	return FALSE;
#endif
}

BOOL CMeleeWeapon::Deploy()
{
	m_pPlayer->m_iFOV = ZoomOut();
	UpdateZoomSpeed();

	int i_MID = m_iId;

	m_pPlayer->m_iFOV = ZoomOut();
	SendWeaponAnim( 3, 1 );

#ifndef CLIENT_DLL
	CDoDTeamPlay *pRules = (CDoDTeamPlay *)g_pGameRules;

	if ( pRules && pRules->IsAlliesBrit() && m_pPlayer->pev->team == 1 )
		i_MID = 40;
#endif

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.75;
	return DefaultDeploy( P_WpnInfo[i_MID].vmodel, P_WpnInfo[i_MID].pmodel, GetDrawAnim(), P_WpnInfo[m_iId].szAnimExt, P_WpnInfo[m_iId].szAnimReloadExt, 0 );
}

void CMeleeWeapon::Holster( int skiplocal )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
}

void CMeleeWeapon::WeaponIdle()
{
	if ( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flNextPrimaryAttack > UTIL_WeaponTimeBase() || m_flNextSecondaryAttack > UTIL_WeaponTimeBase() )
		return;

	int i_Anim = GetIdleAnim();

	if ( i_Anim > -1 )
		SendWeaponAnim( i_Anim, 1 );

	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 2.5;
}

void CMeleeWeapon::PrimaryAttack()
{
	Swing( 1 );
}

void CMeleeWeapon::SwingAgain()
{
	Swing( 0 );
}

void CMeleeWeapon::Smack()
{
	DecalGunshot( &m_trHit, BULLET_PLAYER_CROWBAR );
}

int CMeleeWeapon::Swing( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 48;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_HULL_MIN, VEC_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction >= 1.0 )
	{
		if ( fFirst )
		{
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, GetSlashAnim( m_iSwing ), 0, 1, 0 );

			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.35;
			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		fDidHit = TRUE;

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, GetSlashAnim( m_iSwing ), 0, 0, 0 );

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.4;
		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;

		// play thwack, smack, or dong sound
		float flVol = 1.0;

#ifndef CLIENT_DLL
		int fHitWorld = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage();
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_BULLET | DMG_CLUB | DMG_NEVERGIB );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		if ( pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
		{
			// play thwack or smack sound
			switch ( RANDOM_LONG( 0, 3 ) )
			{
			case 0:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM );
				break;
			case 1:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM );
				break;
			case 3:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit4.wav", 1, ATTN_NORM );
				break;
			}

			m_pPlayer->m_iWeaponVolume = 128;

			if ( !pEntity->IsAlive() )
				return TRUE;
			else
				flVol = 0.1f;

			fHitWorld = FALSE;
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if ( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			fvolbar = 1;

			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
		}
#endif

		m_trHit = tr;
		SetThink( &CMeleeWeapon::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_iWeaponVolume = (int)( flVol * 512 );
	}

	return fDidHit;
}

int CMeleeWeapon::Stab( int fFirst )
{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors( m_pPlayer->pev->v_angle );
	Vector vecSrc = m_pPlayer->GetGunPosition();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( m_pPlayer->pev ), &tr );

	if ( tr.flFraction >= 1.0 )
	{
		UTIL_TraceHull( vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT( m_pPlayer->pev ), &tr );
		if ( tr.flFraction < 1.0 )
		{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity *pHit = CBaseEntity::Instance( tr.pHit );
			if ( !pHit || pHit->IsBSPModel() )
				FindHullIntersection( vecSrc, tr, VEC_HULL_MIN, VEC_HULL_MAX, m_pPlayer->edict() );
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
		}
	}

	if ( tr.flFraction >= 1.0 )
	{
		if ( fFirst )
		{
			// miss
			PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, GetSlashAnim( m_iSwing ), 0, 1, 0 );

			m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1;
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1;

			// player "shoot" animation
			m_pPlayer->SetAnimation( PLAYER_ATTACK1 );
		}
	}
	else
	{
		// hit
		fDidHit = TRUE;

		PLAYBACK_EVENT_FULL( FEV_NOTHOST, m_pPlayer->edict(), m_iFireEvent, 0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, GetSlashAnim( m_iSwing ), 0, 0, 0 );

		m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 1.1;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.1;

		float flVol = 1.0;

#ifndef CLIENT_DLL
		int fHitWorld = TRUE;

		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );

		m_pPlayer->SetAnimation( PLAYER_ATTACK1 );

		ClearMultiDamage();
		pEntity->TraceAttack( m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, &tr, DMG_BULLET | DMG_CLUB | DMG_NEVERGIB );
		ApplyMultiDamage( m_pPlayer->pev, m_pPlayer->pev );

		if ( pEntity && pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
		{
			switch ( RANDOM_LONG( 1, 4 ) )
			{
			case 1:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit1.wav", 1, ATTN_NORM );
				break;
			case 2:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit2.wav", 1, ATTN_NORM );
				break;
			case 3:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit3.wav", 1, ATTN_NORM );
				break;
			case 4:
				EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "weapons/knife_hit4.wav", 1, ATTN_NORM );
				break;
			}

			m_pPlayer->m_iWeaponVolume = 128;

			if ( !pEntity->IsAlive() )
				return TRUE;
			else
				flVol = 0.1f;

			fHitWorld = FALSE;
		}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if ( fHitWorld )
		{
			float fvolbar = TEXTURETYPE_PlaySound( &tr, vecSrc, vecSrc + ( vecEnd - vecSrc ) * 2, BULLET_PLAYER_CROWBAR );

			fvolbar = 1;

			EMIT_SOUND_DYN( ENT( m_pPlayer->pev ), CHAN_ITEM, "weapons/knife_hitwall1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG( 0, 3 ) );
		}
#endif

		// delay the decal a bit
		m_trHit = tr;
		SetThink( &CMeleeWeapon::Smack );
		pev->nextthink = UTIL_WeaponTimeBase() + 0.2;

		m_pPlayer->m_iWeaponVolume = (int)( flVol * 512 );
	}

	return fDidHit;
}
