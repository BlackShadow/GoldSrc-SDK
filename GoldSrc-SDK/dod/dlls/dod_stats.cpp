#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "dod_stats.h"

CDoDStatMgr *g_DoDStats;
char StatNames[4][64] = { "Kills", "Deaths", "Hits", "Shots" };

LINK_ENTITY_TO_CLASS(dod_stats, CDoDStatMgr);

CDoDStatMgr::CDoDStatMgr()
{
	memset(m_PlayerStats, 0, sizeof(m_PlayerStats));
}

void CDoDStatMgr::InitPlayer(int index)
{
	if ((unsigned int)index < 32)
	{
		memset(&m_PlayerStats[index], 0, sizeof(m_PlayerStats[index]));
		m_PlayerStats[index].starttime = gpGlobals->time;
	}
}

DoDPlayerStats_t *CDoDStatMgr::GetPlayer(int index)
{
	if ((unsigned int)index < 32)
		return &m_PlayerStats[index];
	return NULL;
}

void CDoDStatMgr::PrintPlayerStats(int index)
{
	DoDPlayerStats_t *pStats = GetPlayer(index);
	CBaseEntity *pPlayer = UTIL_PlayerByIndex(index);
	if (pStats && pPlayer)
	{
		char buf[44];
		for (int i = 0; i < 4; ++i)
		{
			sprintf(buf, "%s: %d\n", StatNames[i], pStats->stat[i]);
			ClientPrint(pPlayer->pev, HUD_PRINTCONSOLE, buf);
		}
	}
}
