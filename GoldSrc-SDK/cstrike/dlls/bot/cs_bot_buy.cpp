#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "cs_bot.h"
#include "cs_bot_manager.h"
#include "gamerules.h"
#include "debug.h"

enum BuyWeaponClass
{
	BUY_PISTOL,
	BUY_SHOTGUN,
	BUY_SUB_MACHINE_GUN,
	BUY_RIFLE,
	BUY_MACHINE_GUN,
	BUY_SNIPER_RIFLE
};

struct WeaponBuyInfo
{
	BuyWeaponClass	weaponClass;
	bool			preferred;
	const char*		alias;
};

static const WeaponBuyInfo primaryWeaponBuyInfoT[13] =
{
	{ BUY_SHOTGUN, false, "m3" },
	{ BUY_SHOTGUN, false, "xm1014" },
	{ BUY_SUB_MACHINE_GUN, false, "mac10" },
	{ BUY_SUB_MACHINE_GUN, false, "mp5" },
	{ BUY_SUB_MACHINE_GUN, false, "ump45" },
	{ BUY_SUB_MACHINE_GUN, false, "p90" },
	{ BUY_RIFLE, true, "galil" },
	{ BUY_RIFLE, true, "ak47" },
	{ BUY_SNIPER_RIFLE, false, "scout" },
	{ BUY_RIFLE, true, "sg552" },
	{ BUY_SNIPER_RIFLE, true, "awp" },
	{ BUY_SNIPER_RIFLE, true, "g3sg1" },
	{ BUY_MACHINE_GUN, false, "m249" }
};

static const WeaponBuyInfo primaryWeaponBuyInfoCT[13] =
{
	{ BUY_SHOTGUN, false, "m3" },
	{ BUY_SHOTGUN, false, "xm1014" },
	{ BUY_SUB_MACHINE_GUN, false, "tmp" },
	{ BUY_SUB_MACHINE_GUN, false, "mp5" },
	{ BUY_SUB_MACHINE_GUN, false, "ump45" },
	{ BUY_SUB_MACHINE_GUN, false, "p90" },
	{ BUY_RIFLE, true, "famas" },
	{ BUY_SNIPER_RIFLE, false, "scout" },
	{ BUY_RIFLE, true, "m4a1" },
	{ BUY_RIFLE, false, "aug" },
	{ BUY_SNIPER_RIFLE, true, "sg550" },
	{ BUY_SNIPER_RIFLE, true, "awp" },
	{ BUY_MACHINE_GUN, false, "m249" }
};

static const WeaponBuyInfo secondaryWeaponBuyInfoT[3] =
{
	{ BUY_PISTOL, true, "p228" },
	{ BUY_PISTOL, true, "deagle" },
	{ BUY_PISTOL, true, "elites" }
};

static const WeaponBuyInfo secondaryWeaponBuyInfoCT[3] =
{
	{ BUY_PISTOL, true, "p228" },
	{ BUY_PISTOL, true, "deagle" },
	{ BUY_PISTOL, true, "fn57" }
};

void BuyState::OnEnter( CCSBot* bot )
{
	m_randomBuyRetries = 0;
	m_preferredBuyRetries = 0;
	m_preferredWeaponIndex = 0;
	m_done = false;
	m_initialDelay = true;
	bot->EquipBestWeapon( false );
	m_buyDefuser = false;
	m_buyShield = false;
	if ( bot->m_iTeam == CT )
	{
		if ( TheCSBots()->GetScenario() == CCSBotManager::SCENARIO_DEFUSE_BOMB
			&& !((CHalfLifeMultiplay*)g_pGameRules)->IsCareer() && RANDOM_FLOAT( 0.0f, 100.0f ) < 50.0f )
			m_buyDefuser = true;
		if ( !bot->m_bHasPrimary && cv_bot_allow_shield.value > 0.0f && bot->m_iAccount > 2500 )
		{
			if ( bot->m_iAccount > 3999 )
				m_buyShield = RANDOM_FLOAT( 0.0f, 100.0f ) < 10.0f;
			else
				m_buyShield = RANDOM_FLOAT( 0.0f, 100.0f ) < 33.3f;
		}
	}
	m_buyGrenade = cv_bot_allow_grenades.value > 0.0f && RANDOM_FLOAT( 0.0f, 100.0f ) < 33.3f;
	m_buyPistol = false;
	if ( cv_bot_allow_pistols.value > 0.0f )
	{
		CBasePlayerItem* pistol = bot->m_rgpPlayerItems[2];
		if ( !pistol )
		{
			m_buyPistol = true;
			return;
		}
		if ( bot->m_iTeam == TERRORIST )
		{
			if ( pistol->m_iId != WEAPON_GLOCK18 )
				return;
		}
		else if ( bot->m_iTeam != CT || pistol->m_iId != WEAPON_USP )
			return;

		if ( ( cv_bot_allow_shotguns.value > 0.0f || cv_bot_allow_sub_machine_guns.value > 0.0f
			|| cv_bot_allow_rifles.value > 0.0f || cv_bot_allow_machine_guns.value > 0.0f
			|| cv_bot_allow_shield.value > 0.0f || cv_bot_allow_snipers.value > 0.0f ) && bot->m_iAccount > 999 )
			m_buyPistol = RANDOM_FLOAT( 0.0f, 100.0f ) < 33.3f;
		else
			m_buyPistol = RANDOM_FLOAT( 0.0f, 100.0f ) < 75.0f;
	}
}

void BuyState::OnUpdate( CCSBot* bot )
{
	if ( TheNavAreaList.empty() )
		return;
	if ( m_initialDelay )
	{
		if ( gpGlobals->time - bot->m_stateTimestamp < 2.0f )
			return;
		m_initialDelay = false;
	}
	if ( m_done )
	{
		if ( g_pGameRules->IsMultiplayer() && g_pGameRules->IsFreezePeriod() )
			return;
		bot->Idle();
	}
	if ( !( bot->m_signals.GetState() & SIGNAL_BUY ) )
	{
		m_done = true;
		UTIL_DPrintf( "%s bot spawned outside of a buy zone (%d, %d, %d)\n",
			bot->m_iTeam == CT ? "CT" : "Terrorist", (int)bot->pev->origin.x,
			(int)bot->pev->origin.y, (int)bot->pev->origin.z );
		return;
	}
	if ( gpGlobals->time - bot->m_stateTimestamp <= 0.2f )
		return;
	bot->m_stateTimestamp = gpGlobals->time;

	const BotProfile* profile = bot->GetProfile();
	if ( m_preferredWeaponIndex < profile->GetWeaponPreferenceCount() )
	{
		if ( m_preferredBuyRetries > 1 )
		{
			m_preferredBuyRetries = 0;
			++m_preferredWeaponIndex;
			return;
		}
		int id = profile->GetWeaponPreference( m_preferredWeaponIndex );
		if ( ( bot->m_pActiveItem && bot->m_pActiveItem->m_iId == id )
			|| ( bot->HasShield() && id == WEAPON_SHIELDGUN ) )
		{
			m_preferredWeaponIndex = 9999;
			return;
		}
		const char* alias;
		bool allowed = true;
		if ( id == WEAPON_SHIELDGUN )
		{
			alias = "shield";
			allowed = cv_bot_allow_shield.value > 0.0f;
		}
		else
		{
			alias = WeaponIDToAlias( id );
			int weaponClass = -1;
			for ( int i = 0; i < 13; ++i )
			{
				if ( !stricmp( alias, primaryWeaponBuyInfoCT[i].alias ) )
				{
					weaponClass = primaryWeaponBuyInfoCT[i].weaponClass;
					break;
				}
				if ( !stricmp( alias, primaryWeaponBuyInfoT[i].alias ) )
				{
					weaponClass = primaryWeaponBuyInfoT[i].weaponClass;
					break;
				}
			}
			if ( weaponClass < 0 )
			{
				for ( int i = 0; i < 3; ++i )
				{
					if ( !stricmp( alias, secondaryWeaponBuyInfoCT[i].alias )
						|| !stricmp( alias, secondaryWeaponBuyInfoT[i].alias ) )
					{
						weaponClass = BUY_PISTOL;
						break;
					}
				}
			}
			switch ( weaponClass )
			{
			case BUY_PISTOL: allowed = cv_bot_allow_pistols.value > 0.0f; break;
			case BUY_SHOTGUN: allowed = !( cv_bot_allow_shotguns.value <= 0.0f ); break;
			case BUY_SUB_MACHINE_GUN: allowed = !( cv_bot_allow_sub_machine_guns.value <= 0.0f ); break;
			case BUY_RIFLE: allowed = !( cv_bot_allow_rifles.value <= 0.0f ); break;
			case BUY_MACHINE_GUN: allowed = !( cv_bot_allow_machine_guns.value <= 0.0f ); break;
			case BUY_SNIPER_RIFLE: allowed = !( cv_bot_allow_snipers.value <= 0.0f ); break;
			}
			allowed = allowed && alias != NULL;
		}
		if ( allowed )
		{
			bot->ClientCommand( alias );
			bot->PrintIfWatched( "Tried to buy preferred weapon %s.\n", alias );
			++m_preferredBuyRetries;
			return;
		}
		++m_preferredBuyRetries;
	}

	if ( !bot->m_bHasPrimary )
	{
		if ( m_buyShield )
		{
			bot->ClientCommand( "shield" );
			bot->PrintIfWatched( "Tried to buy a shield.\n" );
		}
		else
		{
			const WeaponBuyInfo* table = bot->m_iTeam == TERRORIST ? primaryWeaponBuyInfoT : primaryWeaponBuyInfoCT;
			const WeaponBuyInfo* choices[20];
			int count = 0;
			float sniperChance = RANDOM_FLOAT( 0.0f, 100.0f );
			for ( int i = 0; i < 13; ++i )
			{
				bool allowed = false;
				switch ( table[i].weaponClass )
				{
				case BUY_SHOTGUN: allowed = cv_bot_allow_shotguns.value > 0.0f; break;
				case BUY_SUB_MACHINE_GUN: allowed = cv_bot_allow_sub_machine_guns.value > 0.0f; break;
				case BUY_RIFLE: allowed = cv_bot_allow_rifles.value > 0.0f; break;
				case BUY_MACHINE_GUN: allowed = cv_bot_allow_machine_guns.value > 0.0f; break;
				case BUY_SNIPER_RIFLE: allowed = cv_bot_allow_snipers.value > 0.0f && sniperChance < 50.0f; break;
				}
				if ( allowed )
					choices[count++] = &table[i];
			}
			if ( count )
			{
				int preferredCount = 0;
				if ( !m_randomBuyRetries && !( cv_bot_difficulty.value < 0.9f ) && !( cv_bot_difficulty.value < 1.9f ) )
				{
					for ( int i = 0; i < count; ++i )
						if ( choices[i]->preferred )
							++preferredCount;
				}
				const WeaponBuyInfo* selected;
				if ( !preferredCount )
					selected = choices[RANDOM_LONG( 0, count - 1 )];
				else
				{
					int choice = RANDOM_LONG( 0, preferredCount - 1 );
					int i = 0;
					for ( ; i < count; ++i )
					{
						if ( choices[i]->preferred && choice-- == 0 )
							break;
					}
					selected = choices[i];
				}
				bot->ClientCommand( selected->alias );
				bot->PrintIfWatched( "Tried to buy %s.\n", selected->alias );
			}
		}
	}
	if ( bot->m_bHasPrimary )
		bot->ClientCommand( "primammo" );
	else if ( m_randomBuyRetries++ <= 5 )
		return;

	bot->ClientCommand( "vesthelm" );
	bot->ClientCommand( "vest" );
	if ( cv_bot_allow_pistols.value > 0.0f && !profile->HasPistolPreference() )
	{
		if ( m_buyPistol )
		{
			int index = RANDOM_LONG( 0, 2 );
			bot->ClientCommand( bot->m_iTeam == TERRORIST
				? secondaryWeaponBuyInfoT[index].alias : secondaryWeaponBuyInfoCT[index].alias );
			m_buyPistol = false;
		}
		bot->ClientCommand( "secammo" );
	}
	if ( m_buyGrenade && !bot->HasGrenade() )
	{
		if ( !UTIL_IsTeamAllBots( bot->m_iTeam ) )
			bot->ClientCommand( RANDOM_FLOAT( 0.0f, 100.0f ) >= 10.0f ? "hegren" : "sgren" );
		else
		{
			float chance = RANDOM_FLOAT( 0.0f, 100.0f );
			bot->ClientCommand( chance < 10.0f ? "sgren" : chance < 35.0f ? "flash" : "hegren" );
		}
	}
	if ( m_buyDefuser )
		bot->ClientCommand( "defuser" );
	m_done = true;
}

void BuyState::OnExit( CCSBot* bot )
{
	bot->ResetStuckMonitor();
	bot->EquipBestWeapon( false );
}

const char* BuyState::GetName() const
{
	return "Buy";
}
