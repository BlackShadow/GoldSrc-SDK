#pragma once

#include "backend_legacy.h"
#include "a2d.h"
#include "resman.h"

#define A3D_MIXER_BUFFER_PREFIX_BYTES	0x460

class CHrtfMgr;

class A2DBuffer : public CA3dLegacySoundBuffer
{
public:
	A2DBuffer();
	~A2DBuffer();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetCaps( LPDSBCAPS pCaps );
	HRESULT STDMETHODCALLTYPE GetFormat( LPWAVEFORMATEX pFormat, DWORD dwFormatBytes,
		LPDWORD pdwBytesWritten );
	HRESULT STDMETHODCALLTYPE Initialize( LPDIRECTSOUND pDirectSound, LPCDSBUFFERDESC pDesc );
	HRESULT STDMETHODCALLTYPE Lock( DWORD dwOffset, DWORD dwBytes, LPVOID* ppFirstBlock,
		LPDWORD pdwFirstBytes, LPVOID* ppSecondBlock, LPDWORD pdwSecondBytes, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE Play( DWORD dwReserved, DWORD dwPriority, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE SetCurrentPosition( DWORD dwPosition );
	HRESULT STDMETHODCALLTYPE SetFormat( LPCWAVEFORMATEX pFormat );
	HRESULT STDMETHODCALLTYPE Unlock( LPVOID pFirstBlock, DWORD dwFirstBytes,
		LPVOID pSecondBlock, DWORD dwSecondBytes );
	HRESULT STDMETHODCALLTYPE Restore();

	HRESULT STDMETHODCALLTYPE SetA3dSuperCtrl( void* pData, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE GetAllocationStatus( DWORD dwValue );
	HRESULT STDMETHODCALLTYPE GetWave( DWORD dwValue );
	HRESULT STDMETHODCALLTYPE GetDriverInfo( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE SetNewBuffer( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE SetA3dDirectCtrl( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE GetStatusEx( DWORD dwValue );

	HRESULT Init( const a3d_platform_buffer_desc_t* pDesc, CHrtfMgr* pHrtfFilter,
		LONG* pcSharedReferences );

private:
	friend class DAL_A2D;

	DWORD SetA2DFrequency( DWORD dwFrequency );

	LONG*						m_pcSharedReferences;
	BYTE*						m_pSamples;
	a3d_platform_buffer_desc_t	m_Desc;
	BYTE	m_ReservedAllocation[A3D_MIXER_BUFFER_PREFIX_BYTES - sizeof( CA3dLegacySoundBuffer ) -
		sizeof( m_pcSharedReferences ) - sizeof( m_pSamples ) - sizeof( m_Desc )];
	a3d_mix_input_t				m_Input;
	a3d_mix_voice_t				m_Voice;
	CHrtfMgr*				m_pHrtfFilter;
};
