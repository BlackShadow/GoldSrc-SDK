#ifndef DOD_STATS_H
#define DOD_STATS_H

struct DoDPlayerStats_t
{
	int stat[4];
	float starttime;
};

class CDoDStatMgr : public CBaseEntity
{
public:
	CDoDStatMgr();
	void InitPlayer(int index);
	DoDPlayerStats_t *GetPlayer(int index);
	void PrintPlayerStats(int index);

	DoDPlayerStats_t m_PlayerStats[32];
};

extern CDoDStatMgr *g_DoDStats;

#endif
