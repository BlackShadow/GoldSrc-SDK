#ifndef HOSTAGE_LOCALNAV_H
#define HOSTAGE_LOCALNAV_H

#include "hostage.h"

typedef int node_index_t;

typedef struct localnode_s
{
	Vector vecLoc;
	int offsetX;
	int offsetY;
	byte bDepth;
	BOOL fSearched;
	node_index_t nindexParent;
#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
} localnode_t;

class CLocalNav
{
public:
	CLocalNav( CHostage *owner );
	virtual ~CLocalNav();
	void SetTargetEnt( CBaseEntity *entity ) { m_pTargetEnt = entity ? entity->edict() : NULL; }
	node_index_t FindPath( Vector &start, Vector &destination, float targetRadius, int noMonsters );
	int SetupPathNodes( node_index_t index, Vector *nodes, int noMonsters );
	int GetFurthestTraversableNode( Vector &start, Vector *nodes, int count, int noMonsters );
	int PathTraversable( Vector &source, Vector &destination, int noMonsters );
	BOOL PathClear( Vector &source, Vector &destination, int noMonsters, TraceResult &trace );
	BOOL PathClear( Vector &source, Vector &destination, int noMonsters )
	{
		TraceResult trace;
		return PathClear( source, destination, noMonsters, trace );
	}
	static void Think();
	void RequestNav( CHostage *caller );
	static void Reset();
	static void HostagePrethink();
	static float s_flStepSize;

private:
	node_index_t AddNode( node_index_t parent, Vector &position, int offsetX, int offsetY, byte depth );
	localnode_t *GetNode( node_index_t index ) { return &m_nodeArr[index]; }
	node_index_t NodeExists( int offsetX, int offsetY );
	void AddPathNodes( node_index_t source, int noMonsters );
	void AddPathNode( node_index_t source, int offsetX, int offsetY, int noMonsters );
	node_index_t GetBestNode( Vector &origin, Vector &destination );
	BOOL SlopeTraversable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace );
	BOOL LadderTraversable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace );
	BOOL StepTraversable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace );
	BOOL StepJumpable( Vector &source, Vector &destination, int noMonsters, TraceResult &trace );
	node_index_t FindDirectPath( Vector &start, Vector &destination, float targetRadius, int noMonsters );
	BOOL LadderHit( Vector &source, Vector &destination, TraceResult &trace );
	CHostage *m_pOwner;
	edict_t *m_pTargetEnt;
	BOOL m_fTargetEntHit;
	localnode_t *m_nodeArr;
	node_index_t m_nindexAvailableNode;
	Vector m_vecStartingLoc;
	static EHANDLE queue[20];
	static int qptr;
	static int tot_inqueue;
	static float nodeval;
	static float flNextCvarCheck;
	static float flLastThinkTime;
	static EHANDLE hostages[20];
	static int tot_hostages;
};

#endif
