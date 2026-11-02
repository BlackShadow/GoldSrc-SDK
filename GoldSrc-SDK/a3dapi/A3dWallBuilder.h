#pragma once

#include "A3dGeom.h"

#include <unknwn.h>

class CA3dOpening;
class CA3dOpeningBuilder;
class CA3dGeometryFace;

class CA3dWallBuilder : public IUnknown, public CA3dGeometryBuilder, public CA3dMaterialReference, public CA3dName
{
public:
	CA3dWallBuilder( int nSceneType );
	~CA3dWallBuilder();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppvObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();

	virtual HRESULT STDMETHODCALLTYPE SetMaterial( IA3dMaterial* pMaterial );
	virtual HRESULT STDMETHODCALLTYPE GetMaterial( IA3dMaterial** ppMaterial );
	virtual HRESULT STDMETHODCALLTYPE AddOpening( CA3dOpeningBuilder* pOpening,
			const A3DVAL* pPosition, const A3DVAL* pDirection, const A3DVAL* pUp );
	virtual HRESULT STDMETHODCALLTYPE RemoveOpening( int nOpening );
	virtual HRESULT STDMETHODCALLTYPE GetOpening( int nOpening, CA3dOpening** ppOpening );

	virtual HRESULT STDMETHODCALLTYPE Begin( DWORD dwMode );
	virtual HRESULT STDMETHODCALLTYPE Vertex3f( A3DVAL x, A3DVAL y, A3DVAL z );
	virtual HRESULT STDMETHODCALLTYPE Vertex3fv( const A3DVAL* pVertex );
	virtual HRESULT STDMETHODCALLTYPE End();
	virtual HRESULT STDMETHODCALLTYPE RemovePolygon( int nPolygon );
	virtual int STDMETHODCALLTYPE GetPolygonCount();
	virtual HRESULT STDMETHODCALLTYPE SetVertex( int nPolygon, int nVertex, A3DVAL x, A3DVAL y, A3DVAL z );
	virtual HRESULT STDMETHODCALLTYPE GetVertex( int nPolygon, int nVertex, A3DVAL* pVertex );
	virtual HRESULT STDMETHODCALLTYPE Validate();
	virtual HRESULT STDMETHODCALLTYPE Clear();

	virtual HRESULT STDMETHODCALLTYPE UnsupportedRequest1( DWORD );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedRequest2( DWORD );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedRequest3( DWORD, DWORD );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedRequest4( DWORD, DWORD );
	virtual HRESULT STDMETHODCALLTYPE UnsupportedRequest5( DWORD );
	virtual HRESULT STDMETHODCALLTYPE SetNameID( const char* pszName );
	virtual HRESULT STDMETHODCALLTYPE GetNameID( char* pszName, DWORD dwSize );

private:
	friend class CA3dGeometryFace;

	void ClearOpenings();
	BOOL IsOpeningCoplanar( CA3dOpening* pOpening );

	ULONG			m_ulReferences;
	CA3dObjectList	m_Openings;
};
