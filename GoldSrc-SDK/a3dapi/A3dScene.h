#pragma once

#include "A3dGeom.h"

class CA3d;
class CA3dGeometryFace;
class CA3dList;
class CA3dRoom;
class CA3dRoomBuilder;
class CA3dWallBuilder;

class CA3dScene : public IUnknown, public CA3dMaterialReference, public CA3dName
{
public:
	CA3dScene( DWORD dwContext, CA3d* pA3d );
	~CA3dScene();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual INT STDMETHODCALLTYPE NewRoom( DWORD dwContext, CA3dRoomBuilder* pBuilder,
		const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp );
	virtual INT STDMETHODCALLTYPE NewWall( DWORD dwContext, CA3dWallBuilder* pBuilder,
		const A3DVAL* pPosition, const A3DVAL* pForward, const A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE RemoveRoom( INT nRoom );
	virtual HRESULT STDMETHODCALLTYPE RemoveWall( INT nWall );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationA( DWORD dwArgument );
	virtual BOOL STDMETHODCALLTYPE CompatibilityCheck();
	virtual HRESULT STDMETHODCALLTYPE GetRoom( INT nRoom, CA3dRoom** ppRoom );
	virtual HRESULT STDMETHODCALLTYPE GetWall( INT nRoom, INT nWall, IUnknown** ppWall );
	virtual HRESULT STDMETHODCALLTYPE GetOpening( INT nRoom, INT nWall, INT nOpening, IUnknown** ppOpening );
	virtual HRESULT STDMETHODCALLTYPE FindRoom( A3DVAL x, A3DVAL y, A3DVAL z, CA3dRoom** ppRoom );
	virtual HRESULT STDMETHODCALLTYPE NewTransform( CA3dTransform** ppTransform );
	virtual HRESULT STDMETHODCALLTYPE SetMaterial( IA3dMaterial* pMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetMaterial( IA3dMaterial** ppMaterial );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationB( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationC( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE EnableRoom( INT nRoom, DWORD dwFlags );
	virtual HRESULT STDMETHODCALLTYPE DisableRoom( INT nRoom, DWORD dwFlags );
	virtual HRESULT STDMETHODCALLTYPE SelectRoom( INT nRoom );
	virtual HRESULT STDMETHODCALLTYPE BeginGeometryQuery();
	virtual BOOL STDMETHODCALLTYPE GetNextVertex( A3DVAL* pVertex, INT* pnVertex, DWORD* pdwTag );
	virtual HRESULT STDMETHODCALLTYPE EndGeometryQuery();
	virtual HRESULT STDMETHODCALLTYPE Validate();
	virtual HRESULT STDMETHODCALLTYPE Clear();
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationD( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationE( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationF( DWORD dwArgument, DWORD dwArgument2 );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationG( DWORD dwArgument, DWORD dwArgument2 );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationH( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE SetName( const char* pszName );
	virtual HRESULT STDMETHODCALLTYPE GetName( char* pszName, DWORD dwSize );

	void UpdateCurrentRoom();
	HRESULT Render();

private:
	friend class CA3dRoom;
	friend class CA3d;

	BOOL				m_bValid;
	CA3dRoom*			m_pCurrentRoom;
	CA3dObjectList		m_Rooms;
	CA3dObjectList		m_Walls;
	CA3dObjectList		m_NeighborRooms;
	CA3dObjectList		m_UnpairedRooms;
	CA3dObjectList		m_ZeroContextRooms;
	CA3dObjectList		m_ContextRooms;
	DWORD				m_dwContext;
	CA3dList*			m_pList;
	CA3d*				m_pA3d;
	a3d_primitive_t*	m_pReadPolygon;
	INT					m_nReadVertex;
	BOOL				m_bRestartRoom;
	ULONG				m_cReferences;
};
