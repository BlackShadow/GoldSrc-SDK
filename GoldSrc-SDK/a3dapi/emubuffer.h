#pragma once

#include "backend_legacy.h"
#include "resman.h"

#define A3D_TIMING_BUFFER_ALLOCATION_BYTES	0x460

class EMUBuffer : public CA3dLegacySoundBuffer
{
public:
	EMUBuffer();
	~EMUBuffer();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE GetCaps( LPDSBCAPS pCaps );
	HRESULT STDMETHODCALLTYPE GetCurrentPosition( LPDWORD pdwReadPosition, LPDWORD pdwWritePosition );
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

	HRESULT Init( const a3d_platform_buffer_desc_t* pDesc );

private:
	DWORD						m_dwStartTick;
	DWORD						m_dwLastTick;
	a3d_platform_buffer_desc_t	m_Desc;
	BYTE	m_ReservedAllocation[A3D_TIMING_BUFFER_ALLOCATION_BYTES - sizeof( CA3dLegacySoundBuffer ) -
		sizeof( m_dwStartTick ) - sizeof( m_dwLastTick ) - sizeof( m_Desc )];
};
