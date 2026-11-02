#pragma once

#include "listener.h"
#include "LinkList.h"
#include "A3dGeom.h"

class CA3d;
class CA3dScene;
class CA3dGeometryFace;
class CA3dRoomBuilder;

class CA3dRoom : public IUnknown, public CA3dMaterialReference
{
public:
	CA3dRoom( CA3dRoomBuilder* pBuilder, DWORD dwContext, const A3DVAL* pPosition,
		const A3DVAL* pForward, const A3DVAL* pUp, CA3dScene* pScene );
	~CA3dRoom();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationA( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationB( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE GetScene( CA3dScene** ppScene );
	virtual HRESULT STDMETHODCALLTYPE SetMaterial( IA3dMaterial* pMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetMaterial( IA3dMaterial** ppMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetWall( INT nIndex, IUnknown** ppWall );
	virtual HRESULT STDMETHODCALLTYPE SetPosition( CA3dTransform* pReference, const A3DVAL* pPosition );
	virtual HRESULT STDMETHODCALLTYPE GetPosition( CA3dTransform* pReference, A3DVAL* pPosition );
	virtual HRESULT STDMETHODCALLTYPE SetOrientation( CA3dTransform* pReference,
		const A3DVAL* pForward, const A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE GetOrientation( CA3dTransform* pReference, A3DVAL* pForward, A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE SetTransform( const CA3dTransform* pTransform );
	virtual HRESULT STDMETHODCALLTYPE NewTransform( CA3dTransform** ppTransform );
	virtual HRESULT STDMETHODCALLTYPE CompatibilityCheck();

	void ClearConnections();
	BOOL ContainsPoint( const A3DVAL* pPoint );
	void FindAdjacentRoom( CA3dRoom* pRoom );
	BOOL HasUnpairedWall();
	HRESULT ConnectRoom( CA3dRoom* pRoom );
	void Render( CA3d* pA3d );
	void RebuildRenderList( CA3d* pA3d );
	void InvalidateScene();
	void UpdateWalls();

private:
	friend class CA3dScene;
	friend class CA3dGeometryFace;

	CA3dTransform	m_Transform;
	CA3dObjectList	m_Walls;
	CA3dObjectList	m_ShellWalls;
	CA3dObjectList	m_AdjacentRooms;
	DWORD			m_dwContext;
	IA3dList*		m_pRenderList;
	DWORD			m_dwRenderFlags;
	CA3dScene*		m_pScene;
	BOOL			m_bPointTesting;
	BOOL			m_bDynamicWalls;
	ULONG			m_cReferences;
};

class CA3dRoomBuilder : public CA3dPrivateUnknown, public CA3dMaterialReference, public CA3dName
{
public:
	CA3dRoomBuilder( DWORD dwMode );
	~CA3dRoomBuilder();

	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationA( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationB( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE SetMaterial( IA3dMaterial* pMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetMaterial( IA3dMaterial** ppMaterial );
	virtual INT STDMETHODCALLTYPE AddWall( IUnknown* pGeometry, DWORD dwFlags, const A3DVAL* pPosition,
		const A3DVAL* pForward, const A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE RemoveWall( INT nIndex );
	virtual HRESULT STDMETHODCALLTYPE GetWall( INT nIndex, IUnknown** ppWall );
	virtual HRESULT STDMETHODCALLTYPE Build();
	virtual HRESULT STDMETHODCALLTYPE Clear();
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationC( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationD( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationE( DWORD dwArgument, DWORD dwArgument2 );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationF( DWORD dwArgument, DWORD dwArgument2 );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedOperationG( DWORD dwArgument );
	virtual HRESULT STDMETHODCALLTYPE SetName( const char* pszName );
	virtual HRESULT STDMETHODCALLTYPE GetName( char* pszName, DWORD dwSize );

private:
	friend class CA3dRoom;
	friend class CA3dScene;

	BOOL IsUniqueNormal( const A3DVAL* pNormal );
	void ClearNormals();

	ULONG			m_cReferences;
	DWORD			m_dwMode;
	CA3dObjectList	m_Walls;
	CA3dObjectList	m_Auxiliary;
	BOOL			m_bPointTesting;
};
