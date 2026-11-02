#pragma once

#include "resman.h"

class ResManStatBuffer : public ResManBuffer
{
public:
	ResManStatBuffer( IUnknown* pOwner, DWORD dwBufferKind );
	~ResManStatBuffer();

	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE Release();

	HRESULT STDMETHODCALLTYPE GetCaps( LPDSBCAPS pCaps );
	HRESULT STDMETHODCALLTYPE GetCurrentPosition( LPDWORD pdwReadPosition, LPDWORD pdwWritePosition );
	HRESULT STDMETHODCALLTYPE GetFormat( LPWAVEFORMATEX pFormat, DWORD dwFormatBytes,
		LPDWORD pdwBytesWritten );
	HRESULT STDMETHODCALLTYPE GetVolume( LPLONG plVolume );
	HRESULT STDMETHODCALLTYPE GetPan( LPLONG plPan );
	HRESULT STDMETHODCALLTYPE GetFrequency( LPDWORD pdwFrequency );
	HRESULT STDMETHODCALLTYPE GetStatus( LPDWORD pdwStatus );

	HRESULT STDMETHODCALLTYPE Lock( DWORD dwOffset, DWORD dwBytes, LPVOID* ppFirstBlock,
		LPDWORD pdwFirstBytes, LPVOID* ppSecondBlock, LPDWORD pdwSecondBytes, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE Play( DWORD dwReserved, DWORD dwPriority, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE SetCurrentPosition( DWORD dwPosition );
	HRESULT STDMETHODCALLTYPE SetFormat( LPCWAVEFORMATEX pFormat );
	HRESULT STDMETHODCALLTYPE SetVolume( LONG lVolume );
	HRESULT STDMETHODCALLTYPE SetPan( LONG lPan );
	HRESULT STDMETHODCALLTYPE SetFrequency( DWORD dwFrequency );
	HRESULT STDMETHODCALLTYPE Stop();
	HRESULT STDMETHODCALLTYPE Unlock( LPVOID pFirstBlock, DWORD dwFirstBytes,
		LPVOID pSecondBlock, DWORD dwSecondBytes );
	HRESULT STDMETHODCALLTYPE Restore();

	HRESULT STDMETHODCALLTYPE SetA3dSuperCtrl( void* pData, DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE GetAllocationStatus( DWORD dwValue );
	HRESULT STDMETHODCALLTYPE GetWave( DWORD dwValue );
	HRESULT STDMETHODCALLTYPE GetDriverInfo( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE SetNewBuffer( DWORD dwFirst, DWORD dwSecond );
	HRESULT STDMETHODCALLTYPE SetA3dDirectCtrl( DWORD dwFirst, DWORD dwSecond );

	HRESULT ProcessNotifications( DWORD dwTime );

private:
	DWORD	m_dwPreviousPosition;
};
