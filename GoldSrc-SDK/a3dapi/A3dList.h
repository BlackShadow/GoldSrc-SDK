#pragma once

#include "A3dGeom.h"

#include <unknwn.h>
#include <mmsystem.h>
#include "ia3dapi.h"

class CA3d;
class CA3dCommandBuffer;
class CA3dScene;

class CA3dList : public IUnknown
{
public:
	CA3dList( CA3d* pOwner );

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	virtual HRESULT STDMETHODCALLTYPE Begin();
	virtual HRESULT STDMETHODCALLTYPE End();
	virtual HRESULT STDMETHODCALLTYPE Call();
	virtual ~CA3dList();

private:
	friend class CA3dScene;

	ULONG				m_cReferences;
	CA3d*				m_pOwner;
	CA3dCommandBuffer*	m_pCommands;
	CA3dCommandBuffer*	m_pBeginBuffer;
	int					m_nBeginCommand;
	CA3dCommandBuffer*	m_pEndBuffer;
	int					m_nEndCommand;
};

class CA3dMaterial : public IA3dMaterial, public CA3dName
{
public:
	CA3dMaterial();
	CA3dMaterial( const CA3dMaterial& source );
	~CA3dMaterial();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE Load( LPSTR pszFile );
	HRESULT STDMETHODCALLTYPE Save( LPSTR pszFile );
	HRESULT STDMETHODCALLTYPE UnSerialize( LPVOID pData, UINT nSize );
	HRESULT STDMETHODCALLTYPE Serialize( LPVOID* ppData, UINT* pnSize );
	HRESULT STDMETHODCALLTYPE Duplicate( LPA3DMATERIAL* ppMaterial );
	HRESULT STDMETHODCALLTYPE SetNameID( LPSTR pszName );
	HRESULT STDMETHODCALLTYPE GetNameID( LPSTR pszName, INT nSize );
	HRESULT STDMETHODCALLTYPE SelectPreset( DWORD dwPreset );
	HRESULT STDMETHODCALLTYPE GetClosestPreset( LPDWORD pdwPreset );
	HRESULT STDMETHODCALLTYPE SetReflectance( A3DVAL reflectance, A3DVAL highFrequency );
	HRESULT STDMETHODCALLTYPE GetReflectance( LPA3DVAL pReflectance, LPA3DVAL pHighFrequency );
	HRESULT STDMETHODCALLTYPE SetTransmittance( A3DVAL transmittance, A3DVAL highFrequency );
	HRESULT STDMETHODCALLTYPE GetTransmittance( LPA3DVAL pTransmittance, LPA3DVAL pHighFrequency );

private:
	friend class CA3d;

	ULONG	m_cReferences;
	A3DVAL	m_flReflectance;
	A3DVAL	m_flReflectionFilter;
	A3DVAL	m_flTransmittance;
	A3DVAL	m_flTransmissionFilter;
	A3DVAL	m_flReflectanceHF;
	A3DVAL	m_flTransmittanceHF;
	DWORD	m_dwPreset;
};
