#ifndef BSPMODEL_H
#define BSPMODEL_H

#include "engine/model.h"

#include <stddef.h>

class IBaseSystem;

class IBSPModel
{
public:
	virtual ~IBSPModel()
	{
	}

	virtual void Init( IBaseSystem* system ) = 0;
	virtual void Clear() = 0;
	virtual bool Load( const char* name, bool minimal ) = 0;

	virtual bool IsValid() = 0;
	virtual bool IsMinimal() = 0;

	virtual void SetPVS( float* point ) = 0;
	virtual bool InPVS( float* point ) = 0;

	virtual bool TraceLine( float* start, float* end, float* impact ) = 0;
	virtual int TruePointContents( float* point ) = 0;
};

class BSPModel : public IBSPModel
{
public:
	BSPModel();
	virtual ~BSPModel() override;

	virtual void Init( IBaseSystem* system ) override;
	virtual void Clear() override;
	virtual bool Load( const char* name, bool minimal ) override;

	virtual bool IsValid() override;
	virtual bool IsMinimal() override;

	virtual void SetPVS( float* point ) override;
	virtual bool InPVS( float* point ) override;

	virtual bool TraceLine( float* start, float* end, float* impact ) override;
	virtual int TruePointContents( float* point ) override;

private:
	void LoadTextures( lump_t* l );
	void LoadEdges( lump_t* l );
	bool LoadFromBuffer( unsigned int* buffer, int length, const char* name );
	void LoadLeafs( lump_t* l );
	void LoadNodes( lump_t* l );
	void LoadVisibility( lump_t* l );
	void LoadPlanes( lump_t* l );
	void LoadVertexes( lump_t* l );
	void LoadTexinfo( lump_t* l );
	void LoadClipnodes( lump_t* l );
	void LoadMarksurfaces( lump_t* l );
	void LoadFaces( lump_t* l );
	void LoadLighting( lump_t* l );
	void LoadEntities( lump_t* l );
	void LoadSurfedges( lump_t* l );
	void LoadSubmodels( lump_t* l );

	int HullPointContents( hull_t* hull, int num, const vec_t* point );
	bool RecursiveHullCheck( hull_t* hull, int num, float p1f, float p2f, vec_t* p1, vec_t* p2, trace_t* trace );
	void MakeHull0();

	mleaf_t* PointInLeaf( vec_t* point );
	void GetDimension( vec_t* min, vec_t* max );
	mnode_t* PVSNode( mnode_t* node, vec_t* emins, vec_t* emaxs );
	void PVSMark( unsigned char* ppvs );
	unsigned char* LeafPVS( mleaf_t* leaf );
	void DecompressPVS( unsigned char* in, unsigned char* decompressed, int byteCount );
	unsigned char* DecompressVis( unsigned char* in );
	void SetParent( mnode_t* node, mnode_t* parent );

	model_t			model{};
	unsigned char	mod_novis[1024]{};
	unsigned char*	mod_base = NULL;
	int				r_visframecount = 0;
	mleaf_t*		currentLeaf = NULL;
	bool			m_IsMinimal = false;
	char*			m_wadpath = NULL;
	IBaseSystem*	m_System = NULL;
};

#endif
