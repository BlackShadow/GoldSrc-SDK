#pragma once

#include "A3dGeom.h"

class CA3dOpeningBuilder : public IUnknown, public CA3dGeometryBuilder, public CA3dName
{
public:
	CA3dOpeningBuilder( int nSceneType );
	~CA3dOpeningBuilder();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE Begin( DWORD dwMode );
	virtual HRESULT STDMETHODCALLTYPE AddVertex3f( A3DVAL x, A3DVAL y, A3DVAL z );
	virtual HRESULT STDMETHODCALLTYPE AddVertex3fv( const A3DVAL* pVertex );
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
	virtual HRESULT STDMETHODCALLTYPE SetNameID( LPSTR pszName );
	virtual HRESULT STDMETHODCALLTYPE GetNameID( LPSTR pszName, INT nSize );

private:
	friend class CA3dOpening;

	ULONG	m_cReferences;
};
