#pragma once

#include "resman.h"
#include "mixer_filters.h"

#define A3D_MIX_FILTER_PARAMETER_WORDS	4

class CHrtfMgr;
class A2DBuffer;
class EMUBuffer;
struct CPlex;

typedef struct a3d_sound_buffer_node_s
{
	struct a3d_sound_buffer_node_s*	pNext;
	struct a3d_sound_buffer_node_s*	pPrevious;
	CA3dLegacySoundBuffer*			pBuffer;
} a3d_sound_buffer_node_t;

typedef struct a3d_sound_buffer_list_s
{
	a3d_sound_buffer_node_t*		pHead;
	a3d_sound_buffer_node_t*		pTail;
	LONG						cBuffers;
	a3d_sound_buffer_node_t*		pFreeNodes;
	struct CPlex*	pBlocks;
	LONG						cBlockNodes;
} a3d_sound_buffer_list_t;

class CA3dSoundBackend : public IDirectSound, public IA3dDal, public IA3d2
{
public:
	HRESULT STDMETHODCALLTYPE QueryInterface( REFIID iid, void** ppObject );
	ULONG STDMETHODCALLTYPE AddRef();
	ULONG STDMETHODCALLTYPE Release();
	HRESULT STDMETHODCALLTYPE CreateSoundBuffer( LPCDSBUFFERDESC pDesc, LPDIRECTSOUNDBUFFER* ppBuffer,
		LPUNKNOWN pOuter );
	HRESULT STDMETHODCALLTYPE DuplicateSoundBuffer( LPDIRECTSOUNDBUFFER pSource,
		LPDIRECTSOUNDBUFFER* ppBuffer );
	HRESULT STDMETHODCALLTYPE SetCooperativeLevel( HWND hWindow, DWORD dwLevel );
	HRESULT STDMETHODCALLTYPE Compact();
	HRESULT STDMETHODCALLTYPE GetSpeakerConfig( LPDWORD pdwConfig );
	HRESULT STDMETHODCALLTYPE SetSpeakerConfig( DWORD dwConfig );
	HRESULT STDMETHODCALLTYPE Initialize( LPCGUID pDevice );

	HRESULT STDMETHODCALLTYPE InitializeEx( DWORD dwFirst, DWORD dwSecond,
		void* pConfiguration, void* pBackend );
	HRESULT STDMETHODCALLTYPE GetA3dCaps( void* pFirst, void* pSecond );
	HRESULT STDMETHODCALLTYPE GetDriverInfo( void* pFirst, void* pSecond,
		void* pThird, void* pFourth );
	HRESULT STDMETHODCALLTYPE GetDS( IDirectSound** ppDirectSound );
	HRESULT STDMETHODCALLTYPE GetDSDriverDesc( void* pFirst, void* pSecond );
	HRESULT STDMETHODCALLTYPE QueryFunctionality( DWORD dwFirst, void* pSecond );
	HRESULT STDMETHODCALLTYPE DeviceCompatibility5( void* pFirst, void* pSecond, void* pThird );

	HRESULT STDMETHODCALLTYPE SetOutputMode( DWORD dwFront, DWORD dwBack, DWORD dwQuad );
	HRESULT STDMETHODCALLTYPE GetOutputMode( LPDWORD pdwFront, LPDWORD pdwBack, LPDWORD pdwQuad );
	HRESULT STDMETHODCALLTYPE SetResourceManagerMode( DWORD dwMode );
	HRESULT STDMETHODCALLTYPE GetResourceManagerMode( LPDWORD pdwMode );
	HRESULT STDMETHODCALLTYPE SetHFAbsorbFactor( FLOAT flFactor );
	HRESULT STDMETHODCALLTYPE GetHFAbsorbFactor( FLOAT* pflFactor );
	HRESULT STDMETHODCALLTYPE RegisterVersion( DWORD dwVersion );
	HRESULT STDMETHODCALLTYPE GetSoftwareCaps( LPA3DCAPS_SOFTWARE pCaps );
	HRESULT STDMETHODCALLTYPE GetHardwareCaps( LPA3DCAPS_HARDWARE pCaps );

protected:
	LONG	m_cReferences;
};

class DAL_A2D : public CA3dSoundBackend
{
public:
	DAL_A2D();

	HRESULT STDMETHODCALLTYPE GetCaps( LPDSCAPS pCaps );
	HRESULT STDMETHODCALLTYPE CreateSoundBufferEx( const a3d_platform_buffer_desc_t* pDesc,
		BYTE* pbMode, IDirectSoundBuffer** ppBuffer, IUnknown* pOuter );

	HRESULT Init( IDirectSound* pDirectSound );
	HRESULT CreateOutputBuffer( IDirectSound* pDirectSound, IDirectSoundBuffer** ppBuffer );
	HRESULT CreateSecondaryBuffer( const a3d_platform_buffer_desc_t* pDesc, BYTE** ppMode,
		IDirectSoundBuffer** ppBuffer, IUnknown* pOuter );
	BOOL IsStarved( DWORD dwTime, LPDWORD pdwQueuedBytes );
	HRESULT GetBytesQueued( DWORD dwTime, LPDWORD pdwQueuedBytes );
	void MixVoice( SHORT* pOutput, float* pMix, A2DBuffer* pBuffer, DWORD dwBytes, BOOL bAlreadyMixed );
	HRESULT CopyWorkToOutput( DWORD dwBytes );
	DWORD CountPlayingBuffers();
	static DWORD WINAPI ThreadProc( LPVOID pParameter );

private:
	DWORD					m_dwThreadId;
	HANDLE					m_hThread;
	HANDLE					m_hWakeEvent;
	DWORD					m_dwWaitTime;
	DWORD					m_dwRenderThreshold;
	DWORD					m_dwReserved;
	DWORD					m_dwBufferBytes;
	DWORD					m_dwSampleRate;
	DWORD					m_dwChannels;
	DWORD					m_dwBitsPerSample;
	DWORD					m_dwMaxBuffers;
	DWORD					m_dwWritePosition;
	DWORD					m_dwWorkBufferBytes;
	SHORT*					m_pOutputSamples;
	float*					m_pMixSamples;
	DWORD					m_FilterParameters[A3D_MIX_FILTER_PARAMETER_WORDS];
	mixer_filter_state_t	m_FilterState;
	DWORD					m_dwReservedFilter;
	IDirectSound*			m_pOutputDirectSound;
	IDirectSoundBuffer*		m_pOutputBuffer;
	a3d_sound_buffer_list_t	m_Buffers;
	CHrtfMgr*			m_pFilter;
};

class DAL_EMU : public CA3dSoundBackend
{
public:
	DAL_EMU();

	HRESULT STDMETHODCALLTYPE GetCaps( LPDSCAPS pCaps );
	HRESULT STDMETHODCALLTYPE CreateSoundBufferEx( const a3d_platform_buffer_desc_t* pDesc,
		BYTE* pbMode, IDirectSoundBuffer** ppBuffer, IUnknown* pOuter );
	HRESULT CreateSecondaryBuffer( const a3d_platform_buffer_desc_t* pDesc, BYTE** ppMode,
		IDirectSoundBuffer** ppBuffer, IUnknown* pOuter );
	DWORD CountPlayingBuffers();

private:
	DWORD					m_dwMaxBuffers;
	a3d_sound_buffer_list_t	m_Buffers;
};
