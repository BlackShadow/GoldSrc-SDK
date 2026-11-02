#pragma once

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#define A3D_BUFFER_POSITION_UNSET		0xFFFFFFFF

#define A3D_BUFFER_STATE_PLAYING		1
#define A3D_BUFFER_STATE_STOPPED		2
#define A3D_BUFFER_STATE_RELEASED		3

#define A3D_LEGACY_BUFFER_RENDER_ENABLED	0x01
#define A3D_LEGACY_BUFFER_LOOPING			0x02

class DAL_A2D;
class DAL_EMU;

class IA3dDalBuffer : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE SetA3dSuperCtrl( void* pData, DWORD dwFlags ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetAllocationStatus( DWORD dwValue ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetWave( DWORD dwValue ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetDriverInfo( DWORD dwFirst, DWORD dwSecond ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetNewBuffer( DWORD dwFirst, DWORD dwSecond ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetA3dDirectCtrl( DWORD dwFirst, DWORD dwSecond ) = 0;
};

class IResManBuffer : public IUnknown
{
public:
	virtual HRESULT STDMETHODCALLTYPE GetStatusEx( DWORD dwValue ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetPriority( LPDWORD pdwControl ) = 0;
	virtual HRESULT STDMETHODCALLTYPE GetBufferState( LPDWORD pdwStatus ) = 0;
	virtual HRESULT STDMETHODCALLTYPE SetPriority( DWORD dwFlags ) = 0;
	virtual HRESULT STDMETHODCALLTYPE Mute() = 0;
	virtual HRESULT STDMETHODCALLTYPE UnMute() = 0;
};

class CA3dLegacySoundBuffer : public IDirectSoundBuffer,
	public IA3dDalBuffer, public IResManBuffer
{
public:
	ULONG STDMETHODCALLTYPE AddRef();

	HRESULT STDMETHODCALLTYPE GetCurrentPosition( LPDWORD pdwReadPosition, LPDWORD pdwWritePosition );
	HRESULT STDMETHODCALLTYPE GetVolume( LPLONG plVolume );
	HRESULT STDMETHODCALLTYPE GetPan( LPLONG plPan );
	HRESULT STDMETHODCALLTYPE GetFrequency( LPDWORD pdwFrequency );
	HRESULT STDMETHODCALLTYPE GetStatus( LPDWORD pdwStatus );
	HRESULT STDMETHODCALLTYPE SetVolume( LONG lVolume );
	HRESULT STDMETHODCALLTYPE SetPan( LONG lPan );
	HRESULT STDMETHODCALLTYPE SetFrequency( DWORD dwFrequency );
	HRESULT STDMETHODCALLTYPE Stop();
	virtual HRESULT STDMETHODCALLTYPE SetNotificationPositions( DWORD dwCount, LPCDSBPOSITIONNOTIFY pNotifications );

	HRESULT STDMETHODCALLTYPE GetPriority( LPDWORD pdwControl );
	HRESULT STDMETHODCALLTYPE GetBufferState( LPDWORD pdwStatus );
	HRESULT STDMETHODCALLTYPE SetPriority( DWORD dwFlags );
	HRESULT STDMETHODCALLTYPE Mute();
	HRESULT STDMETHODCALLTYPE UnMute();

protected:
	friend class DAL_A2D;
	friend class DAL_EMU;

	LONG	m_cReferences;
	DWORD	m_dwFlags;
	LONG	m_lVolume;
	LONG	m_lPan;
	DWORD	m_dwOriginalFrequency;
	DWORD	m_dwFrequency;
	DWORD	m_dwBufferBytes;
	DWORD	m_dwReadPosition;
	DWORD	m_dwOverridePosition;
	DWORD	m_dwControl;
	DWORD	m_dwStatus;
};
