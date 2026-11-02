#include "dal_a2d.h"
#include "a2dbuffer.h"
#include "emubuffer.h"
#include "HrtfMgr.h"
#include "a2d.h"
#include "A3dGeom.h"
#include "backend_legacy.h"
#include "A3d3.h"
#include "LinkList.h"
#include "rmstatbuffer.h"
#include "dalinfo.h"

#include <limits.h>
#include <new>
#include <string.h>

#define A3D_SOUND_LIST_BLOCK_NODES		10
#define A3D_SOUND_MIN_SECONDARY_RATE		6000
#define A3D_SOUND_MAX_SECONDARY_RATE		100000
#define A3D_TIMING_MAX_BUFFERS			65535
#define A3D_MIXER_MAX_BUFFERS			8
#define A3D_MIXER_WAIT_MILLISECONDS		20
#define A3D_MIXER_LOW_WATER_BYTES		4096
#define A3D_MIXER_OUTPUT_BUFFER_BYTES	8192
#define A3D_MIXER_SAMPLE_RATE			22050
#define A3D_MIXER_CHANNELS				2
#define A3D_MIXER_BITS_PER_SAMPLE		16
#define A3D_MIXER_BLOCK_ALIGN			4
#define A3D_MIXER_WORK_ALLOCATION_SCALE	4
#define A3D_MIXER_FILTER_RATE_KHZ		22
#define A3D_MIXER_FILTER_COEFFICIENTS	1
#define A3D_MIXER_FILTER_SELECTOR		0
#define A3D_MIXER_MAX_RENDER_BYTES		32768

#define A3D_SOUND_CAPS	(DSCAPS_PRIMARYMONO | DSCAPS_PRIMARYSTEREO | DSCAPS_PRIMARY8BIT | \
	DSCAPS_PRIMARY16BIT | DSCAPS_CONTINUOUSRATE | DSCAPS_SECONDARYMONO | DSCAPS_SECONDARYSTEREO | \
	DSCAPS_SECONDARY8BIT | DSCAPS_SECONDARY16BIT)

static const IID IID_A3dPlatformAudio =
	{ 0xcbd91fa7, 0x41c6, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };
static const IID IID_A3dPlatformDeviceBackend =
	{ 0xa3c62f2a, 0x2671, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::QueryInterface( REFIID iid, void** ppObject )
{
	if ( ppObject == NULL )
		return E_INVALIDARG;

	IUnknown* pObject;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_A3dPlatformAudio, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IDirectSound, sizeof( IID ) ) )
	{
		pObject = (IDirectSound*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dPlatformDeviceBackend, sizeof( IID ) ) )
	{
		pObject = (IA3dDal*)this;
	}
	else if ( !memcmp( &iid, &IID_IA3d, sizeof( IID ) ) || !memcmp( &iid, &IID_IA3d2, sizeof( IID ) ) )
	{
		pObject = (IA3d2*)this;
	}
	else
	{
		*ppObject = NULL;
		return E_NOINTERFACE;
	}

	*ppObject = pObject;
	pObject->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dSoundBackend::AddRef()
{
	return InterlockedIncrement( &m_cReferences );
}

ULONG STDMETHODCALLTYPE CA3dSoundBackend::Release()
{
	if ( !InterlockedDecrement( &m_cReferences ) )
		return 0;

	return m_cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::CreateSoundBuffer( LPCDSBUFFERDESC,
	LPDIRECTSOUNDBUFFER*, LPUNKNOWN pOuter )
{
	return pOuter == NULL ? E_NOTIMPL : E_INVALIDARG;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::DuplicateSoundBuffer( LPDIRECTSOUNDBUFFER, LPDIRECTSOUNDBUFFER* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::SetCooperativeLevel( HWND, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::Compact()
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetSpeakerConfig( LPDWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::SetSpeakerConfig( DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::Initialize( LPCGUID )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::InitializeEx( DWORD, DWORD, void*, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetA3dCaps( void*, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetDriverInfo( void*, void*, void*, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetDS( IDirectSound** ppDirectSound )
{
	*ppDirectSound = (IDirectSound*)this;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetDSDriverDesc( void*, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::QueryFunctionality( DWORD, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::DeviceCompatibility5( void*, void*, void* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::SetOutputMode( DWORD, DWORD, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetOutputMode( LPDWORD, LPDWORD, LPDWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::SetResourceManagerMode( DWORD dwMode )
{
	return dwMode > A3D_RESOURCE_MODE_LAST ? E_INVALIDARG : E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetResourceManagerMode( LPDWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::SetHFAbsorbFactor( FLOAT )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetHFAbsorbFactor( FLOAT* )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::RegisterVersion( DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetSoftwareCaps( LPA3DCAPS_SOFTWARE )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dSoundBackend::GetHardwareCaps( LPA3DCAPS_HARDWARE )
{
	return E_NOTIMPL;
}

DAL_A2D::DAL_A2D()
{
	memset( &m_Buffers, 0, sizeof( m_Buffers ) );
	m_Buffers.cBlockNodes = A3D_SOUND_LIST_BLOCK_NODES;
	m_cReferences = 0;
	m_dwThreadId = 0;
	m_hThread = NULL;
	m_hWakeEvent = NULL;
	m_dwWaitTime = 0;
	m_dwRenderThreshold = 0;
	m_dwReserved = 0;
	m_dwBufferBytes = 0;
	m_dwSampleRate = 0;
	m_dwChannels = 0;
	m_dwBitsPerSample = 0;
	m_dwMaxBuffers = 0;
	m_dwWritePosition = 0;
	m_dwWorkBufferBytes = 0;
	m_pOutputSamples = NULL;
	m_pMixSamples = NULL;
	m_pOutputDirectSound = NULL;
	m_pOutputBuffer = NULL;
	m_pFilter = NULL;
}

HRESULT STDMETHODCALLTYPE DAL_A2D::GetCaps( LPDSCAPS pCaps )
{
	DWORD cPlaying = CountPlayingBuffers();
	pCaps->dwSize = sizeof( *pCaps );
	pCaps->dwFlags = A3D_SOUND_CAPS;
	pCaps->dwMinSecondarySampleRate = A3D_SOUND_MIN_SECONDARY_RATE;
	pCaps->dwMaxSecondarySampleRate = A3D_SOUND_MAX_SECONDARY_RATE;
	pCaps->dwPrimaryBuffers = 0;
	pCaps->dwMaxHwMixingAllBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHwMixingStaticBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHwMixingStreamingBuffers = m_dwMaxBuffers;
	pCaps->dwFreeHwMixingAllBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHwMixingStaticBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHwMixingStreamingBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwMaxHw3DAllBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHw3DStaticBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHw3DStreamingBuffers = m_dwMaxBuffers;
	pCaps->dwFreeHw3DAllBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHw3DStaticBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHw3DStreamingBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwTotalHwMemBytes = 0;
	pCaps->dwFreeHwMemBytes = 0;
	pCaps->dwMaxContigFreeHwMemBytes = 0;
	pCaps->dwUnlockTransferRateHwBuffers = 0;
	pCaps->dwPlayCpuOverheadSwBuffers = 0;
	return S_OK;
}

DWORD DAL_A2D::CountPlayingBuffers()
{
	DWORD count = 0;
	for ( a3d_sound_buffer_node_t* pNode = m_Buffers.pHead; pNode != NULL; pNode = pNode->pNext )
	{
		if ( pNode->pBuffer->m_dwStatus == A3D_BUFFER_STATE_PLAYING )
			count++;
	}
	return count;
}

HRESULT DAL_A2D::Init( IDirectSound* pDirectSound )
{
	m_dwWaitTime = A3D_MIXER_WAIT_MILLISECONDS;
	m_dwRenderThreshold = A3D_MIXER_LOW_WATER_BYTES;
	m_dwReserved = 0;
	m_dwBufferBytes = A3D_MIXER_OUTPUT_BUFFER_BYTES;
	m_dwSampleRate = A3D_MIXER_SAMPLE_RATE;
	m_dwChannels = A3D_MIXER_CHANNELS;
	m_dwBitsPerSample = A3D_MIXER_BITS_PER_SAMPLE;
	m_dwMaxBuffers = A3D_MIXER_MAX_BUFFERS;
	m_pOutputDirectSound = pDirectSound;

	try
	{
		if ( m_pOutputDirectSound == NULL )
			throw (char*)"DAL_A2D: ERROR - Invalid pointer to DirectSound\n";

		CreateOutputBuffer( m_pOutputDirectSound, &m_pOutputBuffer );
		if ( m_pOutputBuffer == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not output buffer\n";

		m_dwWorkBufferBytes = m_dwBufferBytes * A3D_MIXER_WORK_ALLOCATION_SCALE /
			(m_dwBitsPerSample / CHAR_BIT) * sizeof( float );
		m_pOutputSamples = (SHORT*)GlobalAlloc( GMEM_FIXED, m_dwWorkBufferBytes );
		if ( m_pOutputSamples == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not create XTalk work buffer\n";

		m_pMixSamples = (float*)GlobalAlloc( GMEM_FIXED, m_dwWorkBufferBytes );
		if ( m_pMixSamples == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not create NoXTalk work buffer\n";

		m_hWakeEvent = CreateEventA( NULL, FALSE, FALSE, NULL );
		if ( m_hWakeEvent == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not create event\n";

		m_hThread = CreateThread( NULL, 0, ThreadProc, this, 0, &m_dwThreadId );
		if ( m_hThread == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not create thread\n";

		m_pFilter = new (std::nothrow) CHrtfMgr;
		if ( m_pFilter == NULL )
			throw (char*)"DAL_A2D: ERROR - Could not create HRTF manager\n";

		if ( FAILED( m_pFilter->Initialize( A3D_MIXER_SAMPLE_RATE,
			A3D_MIXER_FILTER_COEFFICIENTS, A3D_MIXER_FILTER_SELECTOR ) ) )
			throw (char*)"DAL_A2D: ERROR - Could not init HRTF manager\n";

		Mixer_InitializeFilters( m_FilterParameters, &m_FilterState, A3D_MIXER_FILTER_RATE_KHZ );
		return S_OK;
	}
	catch ( char* )
	{
		if ( m_hWakeEvent != NULL )
		{
			CloseHandle( m_hWakeEvent );
			m_hWakeEvent = NULL;
		}
		if ( m_pMixSamples != NULL )
		{
			GlobalFree( m_pMixSamples );
			m_pMixSamples = NULL;
		}
		if ( m_pOutputSamples != NULL )
		{
			GlobalFree( m_pOutputSamples );
			m_pOutputSamples = NULL;
		}
		if ( m_pOutputBuffer != NULL )
		{
			m_pOutputBuffer->Release();
			m_pOutputBuffer = NULL;
		}
		return E_FAIL;
	}
}

HRESULT DAL_A2D::CreateOutputBuffer( IDirectSound* pDirectSound, IDirectSoundBuffer** ppBuffer )
{
	PCMWAVEFORMAT format = {};
	format.wf.wFormatTag = WAVE_FORMAT_PCM;
	format.wf.nChannels = (WORD)m_dwChannels;
	format.wf.nSamplesPerSec = m_dwSampleRate;
	format.wf.nAvgBytesPerSec = m_dwSampleRate * A3D_MIXER_BLOCK_ALIGN;
	format.wf.nBlockAlign = A3D_MIXER_BLOCK_ALIGN;
	format.wBitsPerSample = (WORD)m_dwBitsPerSample;

	a3d_platform_buffer_desc_t desc = {};
	desc.dwSize = sizeof( desc );
	desc.dwFlags = DSBCAPS_GLOBALFOCUS;
	desc.dwBufferBytes = m_dwBufferBytes;
	desc.lpwfxFormat = (LPWAVEFORMATEX)&format;
	HRESULT status = pDirectSound->CreateSoundBuffer( (LPCDSBUFFERDESC)&desc, ppBuffer, NULL );
	if ( FAILED( status ) )
	{
		*ppBuffer = NULL;
		return status;
	}

	( *ppBuffer )->Play( 0, 0, DSBPLAY_LOOPING );
	( *ppBuffer )->SetVolume( DSBVOLUME_MAX );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DAL_A2D::CreateSoundBufferEx( const a3d_platform_buffer_desc_t* pDesc,
	BYTE* pbMode, IDirectSoundBuffer** ppBuffer, IUnknown* pOuter )
{
	return CreateSecondaryBuffer( pDesc, &pbMode, ppBuffer, pOuter );
}

HRESULT DAL_A2D::CreateSecondaryBuffer( const a3d_platform_buffer_desc_t* pDesc, BYTE**,
	IDirectSoundBuffer** ppBuffer, IUnknown* )
{
	if ( CountPlayingBuffers() >= m_dwMaxBuffers )
		return E_FAIL;

	*ppBuffer = NULL;
	A2DBuffer* pBuffer = new (std::nothrow) A2DBuffer;
	if ( pBuffer == NULL )
		return E_OUTOFMEMORY;

	LONG* pcSharedReferences = (LONG*)GlobalAlloc( GMEM_FIXED, sizeof( *pcSharedReferences ) );
	if ( pcSharedReferences == NULL )
	{
		pBuffer->~A2DBuffer();
		operator delete( pBuffer );
		return E_OUTOFMEMORY;
	}

	HRESULT status = pBuffer->Init( pDesc, m_pFilter, pcSharedReferences );
	if ( FAILED( status ) )
	{
		GlobalFree( pcSharedReferences );
		pBuffer->~A2DBuffer();
		operator delete( pBuffer );
		return status;
	}

	pBuffer->AddRef();
	a3d_sound_buffer_node_t* pPrevious = m_Buffers.pTail;
	if ( m_Buffers.pFreeNodes == NULL )
	{
		CPlex* pBlock = CPlex::Create( &m_Buffers.pBlocks,
			(DWORD)m_Buffers.cBlockNodes, sizeof( a3d_sound_buffer_node_t ) );
		a3d_sound_buffer_node_t* pNode = (a3d_sound_buffer_node_t*)( pBlock + 1 ) + m_Buffers.cBlockNodes - 1;
		for ( LONG i = m_Buffers.cBlockNodes - 1; i >= 0; i-- )
		{
			pNode->pNext = m_Buffers.pFreeNodes;
			m_Buffers.pFreeNodes = pNode;
			pNode--;
		}
	}

	a3d_sound_buffer_node_t* pNode = m_Buffers.pFreeNodes;
	m_Buffers.pFreeNodes = pNode->pNext;
	pNode->pPrevious = pPrevious;
	pNode->pNext = NULL;
	m_Buffers.cBuffers = (LONG)( (DWORD)m_Buffers.cBuffers + 1 );
	pNode->pBuffer = pBuffer;
	if ( m_Buffers.pTail != NULL )
		m_Buffers.pTail->pNext = pNode;
	else
		m_Buffers.pHead = pNode;

	m_Buffers.pTail = pNode;
	*ppBuffer = pBuffer;
	return S_OK;
}

HRESULT DAL_A2D::GetBytesQueued( DWORD, LPDWORD pdwQueuedBytes )
{
	DWORD dwPlayPosition;
	DWORD dwWritePosition;
	HRESULT status = m_pOutputBuffer->GetCurrentPosition( &dwPlayPosition, &dwWritePosition );
	if ( FAILED( status ) )
		return status;

	if ( dwPlayPosition > m_dwWritePosition )
		*pdwQueuedBytes = m_dwBufferBytes - dwPlayPosition + m_dwWritePosition;
	else
		*pdwQueuedBytes = m_dwWritePosition - dwPlayPosition;

	return S_OK;
}

BOOL DAL_A2D::IsStarved( DWORD dwTime, LPDWORD pdwQueuedBytes )
{
	if ( FAILED( GetBytesQueued( dwTime, pdwQueuedBytes ) ) )
		return FALSE;

	return *pdwQueuedBytes < m_dwRenderThreshold;
}

void DAL_A2D::MixVoice( SHORT*, float* pMix, A2DBuffer* pBuffer, DWORD dwBytes, BOOL )
{
	if ( pBuffer->m_dwOverridePosition != A3D_BUFFER_POSITION_UNSET )
	{
		pBuffer->m_dwReadPosition = pBuffer->m_dwOverridePosition;
		AR2Dsetposition( &pBuffer->m_Voice, &pBuffer->m_Input, pBuffer->m_dwOverridePosition );
		pBuffer->m_dwOverridePosition = A3D_BUFFER_POSITION_UNSET;
	}

	AR2Dproc( &pBuffer->m_Input, pMix, &pBuffer->m_Voice, dwBytes / A3D_MIXER_BLOCK_ALIGN, FALSE );
	if ( pBuffer->m_Input.pCurrent >= pBuffer->m_Input.pEnd &&
		!( pBuffer->m_dwFlags & A3D_LEGACY_BUFFER_LOOPING ) )
	{
		pBuffer->m_dwStatus = A3D_BUFFER_STATE_STOPPED;
		pBuffer->m_dwReadPosition = pBuffer->m_dwBufferBytes;
		return;
	}

	pBuffer->m_dwReadPosition = (DWORD)( pBuffer->m_Input.pCurrent - pBuffer->m_Input.pSamples );
}

HRESULT DAL_A2D::CopyWorkToOutput( DWORD dwBytes )
{
	void* pFirst;
	void* pSecond;
	DWORD dwFirstBytes;
	DWORD dwSecondBytes;
	HRESULT status = m_pOutputBuffer->Lock( m_dwWritePosition, dwBytes, &pFirst, &dwFirstBytes,
		&pSecond, &dwSecondBytes, 0 );
	if ( FAILED( status ) )
		return status;

	memcpy( pFirst, m_pOutputSamples, dwFirstBytes );
	if ( dwSecondBytes )
		memcpy( pSecond, (BYTE*)m_pOutputSamples + dwFirstBytes, dwSecondBytes );

	m_dwWritePosition = (m_dwWritePosition + dwBytes) % m_dwBufferBytes;
	status = m_pOutputBuffer->Unlock( pFirst, dwFirstBytes, pSecond, dwSecondBytes );
	return FAILED( status ) ? status : S_OK;
}

DWORD WINAPI DAL_A2D::ThreadProc( LPVOID pParameter )
{
	DAL_A2D* pBackend = (DAL_A2D*)pParameter;
	for ( ;; )
	{
		WaitForSingleObject( pBackend->m_hWakeEvent, pBackend->m_dwWaitTime );
		DWORD dwQueuedBytes;
		if ( !pBackend->IsStarved( GetTickCount(), &dwQueuedBytes ) )
			continue;

		DWORD dwBytes = pBackend->m_dwBufferBytes - dwQueuedBytes;
		if ( dwBytes > A3D_MIXER_MAX_RENDER_BYTES )
			dwBytes = A3D_MIXER_MAX_RENDER_BYTES;

		DWORD dwClearBytes = dwBytes * (sizeof( float ) / sizeof( SHORT ));
		memset( pBackend->m_pMixSamples, 0, dwClearBytes );
		memset( pBackend->m_pOutputSamples, 0, dwClearBytes );
		BOOL bMixed = FALSE;
		a3d_sound_buffer_node_t* pNode = pBackend->m_Buffers.pHead;
		while ( pNode != NULL )
		{
			a3d_sound_buffer_node_t* pNext = pNode->pNext;
			A2DBuffer* pBuffer = (A2DBuffer*)pNode->pBuffer;
			if ( pBuffer->m_dwStatus == A3D_BUFFER_STATE_PLAYING )
			{
				pBackend->MixVoice( pBackend->m_pOutputSamples, pBackend->m_pMixSamples,
					pBuffer, dwBytes, bMixed );
				bMixed = TRUE;
			}

			if ( pBuffer->m_dwStatus == A3D_BUFFER_STATE_RELEASED )
			{
				if ( pNode == pBackend->m_Buffers.pHead )
					pBackend->m_Buffers.pHead = pNode->pNext;
				else
					pNode->pPrevious->pNext = pNode->pNext;
				if ( pNode == pBackend->m_Buffers.pTail )
					pBackend->m_Buffers.pTail = pNode->pPrevious;
				else
					pNode->pNext->pPrevious = pNode->pPrevious;

				pNode->pNext = pBackend->m_Buffers.pFreeNodes;
				pBackend->m_Buffers.pFreeNodes = pNode;
				pBackend->m_Buffers.cBuffers = (LONG)( (DWORD)pBackend->m_Buffers.cBuffers - 1 );
				if ( !pBackend->m_Buffers.cBuffers )
				{
					pBackend->m_Buffers.cBuffers = 0;
					pBackend->m_Buffers.pFreeNodes = NULL;
					pBackend->m_Buffers.pTail = NULL;
					pBackend->m_Buffers.pHead = NULL;
					CPlex* pBlocksToFree = pBackend->m_Buffers.pBlocks;
					if ( pBlocksToFree )
						pBlocksToFree->FreeDataChain();
					pBackend->m_Buffers.pBlocks = NULL;
				}

				if ( !*pBuffer->m_pcSharedReferences )
				{
					GlobalFree( pBuffer->m_pcSharedReferences );
					GlobalFree( pBuffer->m_pSamples );
				}
				if ( pBuffer != NULL )
				{
					pBuffer->~A2DBuffer();
					operator delete( pBuffer );
				}
			}
			pNode = pNext;
		}

		if ( bMixed )
			Mixer_FloatToPcm16( pBackend->m_pMixSamples, pBackend->m_pOutputSamples,
				dwBytes / sizeof( SHORT ) );
		pBackend->CopyWorkToOutput( dwBytes );
	}
}

DAL_EMU::DAL_EMU()
{
	memset( &m_Buffers, 0, sizeof( m_Buffers ) );
	m_Buffers.cBlockNodes = A3D_SOUND_LIST_BLOCK_NODES;
	m_cReferences = 0;
	m_dwMaxBuffers = A3D_TIMING_MAX_BUFFERS;
}

HRESULT STDMETHODCALLTYPE DAL_EMU::GetCaps( LPDSCAPS pCaps )
{
	DWORD cPlaying = CountPlayingBuffers();
	pCaps->dwSize = sizeof( *pCaps );
	pCaps->dwFlags = A3D_SOUND_CAPS;
	pCaps->dwMinSecondarySampleRate = A3D_SOUND_MIN_SECONDARY_RATE;
	pCaps->dwMaxSecondarySampleRate = A3D_SOUND_MAX_SECONDARY_RATE;
	pCaps->dwPrimaryBuffers = 0;
	pCaps->dwMaxHwMixingAllBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHwMixingStaticBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHwMixingStreamingBuffers = m_dwMaxBuffers;
	pCaps->dwFreeHwMixingAllBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHwMixingStaticBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHwMixingStreamingBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwMaxHw3DAllBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHw3DStaticBuffers = m_dwMaxBuffers;
	pCaps->dwMaxHw3DStreamingBuffers = m_dwMaxBuffers;
	pCaps->dwFreeHw3DAllBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHw3DStaticBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwFreeHw3DStreamingBuffers = m_dwMaxBuffers - cPlaying;
	pCaps->dwTotalHwMemBytes = 0;
	pCaps->dwFreeHwMemBytes = 0;
	pCaps->dwMaxContigFreeHwMemBytes = 0;
	pCaps->dwUnlockTransferRateHwBuffers = 0;
	pCaps->dwPlayCpuOverheadSwBuffers = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE DAL_EMU::CreateSoundBufferEx( const a3d_platform_buffer_desc_t* pDesc,
		BYTE* pbMode, IDirectSoundBuffer** ppBuffer, IUnknown* pOuter )
{
	return CreateSecondaryBuffer( pDesc, &pbMode, ppBuffer, pOuter );
}

HRESULT DAL_EMU::CreateSecondaryBuffer( const a3d_platform_buffer_desc_t* pDesc, BYTE**,
		IDirectSoundBuffer** ppBuffer, IUnknown* )
{
	if ( CountPlayingBuffers() >= m_dwMaxBuffers )
		return E_FAIL;

	*ppBuffer = NULL;
	EMUBuffer* pBuffer = new (std::nothrow) EMUBuffer;
	if ( pBuffer == NULL )
		return E_OUTOFMEMORY;

	HRESULT hr = pBuffer->Init( pDesc );
	if ( FAILED( hr ) )
	{
		pBuffer->~EMUBuffer();
		::operator delete( pBuffer );
		return hr;
	}

	pBuffer->AddRef();
	a3d_sound_buffer_node_t* pTail = m_Buffers.pTail;
	if ( m_Buffers.pFreeNodes == NULL )
	{
		CPlex* pBlock = CPlex::Create( &m_Buffers.pBlocks,
			m_Buffers.cBlockNodes, sizeof( a3d_sound_buffer_node_t ) );
		a3d_sound_buffer_node_t* pNodes = (a3d_sound_buffer_node_t*)( pBlock + 1 );
		for ( LONG i = (LONG)( (DWORD)m_Buffers.cBlockNodes - 1 ); i >= 0; i-- )
		{
			pNodes[i].pNext = m_Buffers.pFreeNodes;
			m_Buffers.pFreeNodes = &pNodes[i];
		}
	}

	a3d_sound_buffer_node_t* pNode = m_Buffers.pFreeNodes;
	m_Buffers.pFreeNodes = pNode->pNext;
	pNode->pPrevious = pTail;
	pNode->pNext = NULL;
	m_Buffers.cBuffers++;
	pNode->pBuffer = pBuffer;
	if ( m_Buffers.pTail != NULL )
		m_Buffers.pTail->pNext = pNode;
	else
		m_Buffers.pHead = pNode;
	m_Buffers.pTail = pNode;
	*ppBuffer = pBuffer;
	return S_OK;
}

DWORD DAL_EMU::CountPlayingBuffers()
{
	DWORD cPlaying = 0;
	a3d_sound_buffer_node_t* pNode = m_Buffers.pHead;
	while ( pNode != NULL )
	{
		CA3dLegacySoundBuffer* pBuffer = pNode->pBuffer;
		pNode = pNode->pNext;
		if ( pBuffer->m_dwStatus == A3D_BUFFER_STATE_PLAYING )
			cPlaying++;
	}

	return cPlaying;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetCurrentPosition( LPDWORD pdwReadPosition, LPDWORD pdwWritePosition )
{
	if ( m_dwOverridePosition == A3D_BUFFER_POSITION_UNSET )
	{
		*pdwReadPosition = m_dwReadPosition;
		*pdwWritePosition = m_dwReadPosition;
	}
	else
	{
		*pdwReadPosition = m_dwOverridePosition;
		*pdwWritePosition = m_dwOverridePosition;
	}

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetVolume( LPLONG plVolume )
{
	if ( plVolume == NULL )
		return DSERR_INVALIDPARAM;

	*plVolume = m_lVolume;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetPan( LPLONG plPan )
{
	if ( plPan == NULL )
		return DSERR_INVALIDPARAM;

	*plPan = m_lPan;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetFrequency( LPDWORD pdwFrequency )
{
	*pdwFrequency = m_dwFrequency;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::SetVolume( LONG lVolume )
{
	if ( lVolume < DSBVOLUME_MIN || lVolume > DSBVOLUME_MAX )
		return DSERR_INVALIDPARAM;

	m_lVolume = lVolume;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::SetFrequency( DWORD dwFrequency )
{
	if ( dwFrequency == DSBFREQUENCY_ORIGINAL )
		m_dwFrequency = m_dwOriginalFrequency;
	else
		m_dwFrequency = dwFrequency;

	return DS_OK;
}

CA3dListNode* CA3dListNode::Append( void* pObject )
{
	CA3dListNode* pTail = this;
	while ( pTail->m_pNext != NULL )
		pTail = pTail->m_pNext;

	CA3dListNode* pNode = new (std::nothrow) CA3dListNode( pTail, pObject );
	pTail->m_pNext = pNode;
	return pNode;
}

void CA3d::RemoveSource( CA3dSource* pSource )
{
	CA3dListNode* pNode = m_Sources.m_pFirst;
	for ( int i = 0; i < m_Sources.m_nCount; i++ )
	{
		if ( pNode->m_pObject == pSource )
		{
			m_Sources.m_pCurrent = pNode;

			if ( m_Sources.m_pFirst == pNode )
				m_Sources.m_pFirst = pNode->m_pNext;
			if ( m_Sources.m_pLast == pNode )
				m_Sources.m_pLast = pNode->m_pPrevious;

			m_Sources.m_pCurrent = pNode->m_pPrevious;
			if ( m_Sources.m_pCurrent == NULL )
				m_Sources.m_pCurrent = m_Sources.m_pFirst;

			if ( pNode->m_pNext != NULL )
				pNode->m_pNext->m_pPrevious = pNode->m_pPrevious;
			if ( pNode->m_pPrevious != NULL )
				pNode->m_pPrevious->m_pNext = pNode->m_pNext;

			operator delete( pNode );
			m_Sources.m_nCount--;
			return;
		}

		pNode = pNode->m_pNext;
	}
}

HRESULT STDMETHODCALLTYPE CA3d::SetNumFallbackSources( DWORD dwSources )
{
	m_dwNumFallbackSources = dwSources;
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

HRESULT STDMETHODCALLTYPE CA3d::GetCoordinateSystem( LPDWORD pSystem )
{
	if ( pSystem == NULL )
		return E_INVALIDARG;

	*pSystem = A3D_RIGHT_HANDED_CS;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::GetNumFallbackSources( LPDWORD pSources )
{
	if ( pSources == NULL )
		return E_INVALIDARG;

	*pSources = m_dwNumFallbackSources;
	return A3DERROR_UNIMPLEMENTED_FUNCTION;
}

ULONG STDMETHODCALLTYPE CA3dLegacySoundBuffer::AddRef()
{
	return InterlockedIncrement( &m_cReferences );
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetStatus( LPDWORD pdwStatus )
{
	*pdwStatus = 0;
	if ( m_dwStatus == A3D_BUFFER_STATE_PLAYING )
		*pdwStatus = DSBSTATUS_PLAYING;
	if ( m_dwFlags == A3D_LEGACY_BUFFER_LOOPING )
		*pdwStatus |= DSBSTATUS_LOOPING;

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::SetPan( LONG lPan )
{
	if ( lPan < DSBPAN_LEFT || lPan > DSBPAN_RIGHT )
		return DSERR_INVALIDPARAM;

	m_lPan = lPan;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::Stop()
{
	m_dwStatus = A3D_BUFFER_STATE_STOPPED;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::SetNotificationPositions( DWORD, LPCDSBPOSITIONNOTIFY )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetPriority( LPDWORD pdwControl )
{
	*pdwControl = m_dwControl;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::GetBufferState( LPDWORD pdwStatus )
{
	*pdwStatus = m_dwStatus;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::SetPriority( DWORD dwFlags )
{
	m_dwControl = dwFlags;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::Mute()
{
	m_dwFlags |= A3D_LEGACY_BUFFER_RENDER_ENABLED;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dLegacySoundBuffer::UnMute()
{
	m_dwFlags &= ~A3D_LEGACY_BUFFER_RENDER_ENABLED;
	return S_OK;
}

static const IID IID_A3dRenderBuffer =
	{ 0xe04814d8, 0x26fc, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };
static const IID IID_A3dBufferControl =
	{ 0xcbd91fa8, 0x41c6, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };
static const IID IID_A3dHardwareBufferControl =
	{ 0x2e56ef65, 0x74ce, 0x11d2, { 0x97, 0x1b, 0x00, 0x60, 0x08, 0x8d, 0x87, 0x88 } };

ResManStatBuffer::ResManStatBuffer( IUnknown* pOwner, DWORD dwBufferKind )
{
	m_pOwner = pOwner;
	m_pOwner->AddRef();
	m_cReferences = 0;
	m_pBufferToken = NULL;
	m_pStream = NULL;
	m_pRenderData = NULL;
	m_lVolume = 0;
	m_hData = NULL;
	m_lPan = 0;
	m_dwControl = 0;
	m_dwOriginalFrequency = 0;
	m_dwFlags = 0;
	m_dwNotificationCount = 0;
	m_pNotifications = NULL;
	m_hNotificationMutex = NULL;
	m_dwPreviousPosition = 0;
	m_dwBufferKind = dwBufferKind;
	m_dwFrequency = DSBFREQUENCY_MIN;
	m_dwStatus = 2;
}

ResManStatBuffer::~ResManStatBuffer()
{
	if ( m_hNotificationMutex != NULL )
	{
		CloseHandle( m_hNotificationMutex );
		m_hNotificationMutex = NULL;
	}

	if ( m_pOwner != NULL )
	{
		m_pOwner->Release();
		m_pOwner = NULL;
	}
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::QueryInterface( REFIID iid, void** ppObject )
{
	if ( ppObject == NULL )
		return E_INVALIDARG;

	IUnknown* pObject;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IDirectSoundBuffer, sizeof( IID ) ) )
	{
		pObject = (IDirectSoundBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dRenderBuffer, sizeof( IID ) ) )
	{
		pObject = (IA3dDalBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dBufferControl, sizeof( IID ) ) )
	{
		pObject = (IResManBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_IDirectSoundNotify, sizeof( IID ) ) )
	{
		pObject = (IDirectSoundBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dHardwareBufferControl, sizeof( IID ) ) )
	{
		pObject = (IResManBuffer*)this;
	}
	else
	{
		*ppObject = NULL;
		return E_NOINTERFACE;
	}

	*ppObject = pObject;
	pObject->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE ResManStatBuffer::Release()
{
	if ( !InterlockedDecrement( &m_cReferences ) )
	{
		m_pStream->GetDSBuffer()->Stop();
		m_pStream->m_pPool->RemoveStream( m_pStream );
		SetStream( NULL );
		m_dwStatus = 3;
		return 0;
	}

	return m_cReferences;
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetCaps( LPDSBCAPS pCaps )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetCaps( pCaps );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetCurrentPosition( LPDWORD pdwReadPosition,
		LPDWORD pdwWritePosition )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetCurrentPosition( pdwReadPosition, pdwWritePosition );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetFormat( LPWAVEFORMATEX pFormat, DWORD dwFormatBytes,
		LPDWORD pdwBytesWritten )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetFormat( pFormat, dwFormatBytes, pdwBytesWritten );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetVolume( LPLONG plVolume )
{
	if ( plVolume == NULL )
		return E_INVALIDARG;
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetVolume( plVolume );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetPan( LPLONG plPan )
{
	if ( plPan == NULL )
		return E_INVALIDARG;
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetPan( plPan );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetFrequency( LPDWORD pdwFrequency )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetFrequency( pdwFrequency );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetStatus( LPDWORD pdwStatus )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->GetStatus( pdwStatus );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::Lock( DWORD dwOffset, DWORD dwBytes, LPVOID* ppFirstBlock,
		LPDWORD pdwFirstBytes, LPVOID* ppSecondBlock, LPDWORD pdwSecondBytes, DWORD dwFlags )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->Lock( dwOffset, dwBytes, ppFirstBlock, pdwFirstBytes,
		ppSecondBlock, pdwSecondBytes, dwFlags );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::Play( DWORD dwReserved, DWORD dwPriority, DWORD dwFlags )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->Play( dwReserved, dwPriority, dwFlags );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetCurrentPosition( DWORD dwPosition )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	m_dwPreviousPosition = dwPosition;
	return m_pStream->GetDSBuffer()->SetCurrentPosition( dwPosition );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetFormat( LPCWAVEFORMATEX pFormat )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->SetFormat( pFormat );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetVolume( LONG lVolume )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->SetVolume( lVolume );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetPan( LONG lPan )
{
	(void)lPan;

	if ( m_pStream == NULL )
		return E_FAIL;

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetFrequency( DWORD dwFrequency )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->SetFrequency( dwFrequency );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::Stop()
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->Stop();
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::Unlock( LPVOID pFirstBlock, DWORD dwFirstBytes,
		LPVOID pSecondBlock, DWORD dwSecondBytes )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->Unlock( pFirstBlock, dwFirstBytes, pSecondBlock, dwSecondBytes );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::Restore()
{
	if ( m_pStream == NULL )
		return E_FAIL;

	return m_pStream->GetDSBuffer()->Restore();
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetA3dSuperCtrl( void* pData, DWORD dwFlags )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	m_pRenderData = pData;
	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->SetA3dSuperCtrl( pData, dwFlags );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetAllocationStatus( DWORD dwValue )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->GetAllocationStatus( dwValue );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetWave( DWORD dwValue )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->GetWave( dwValue );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::GetDriverInfo( DWORD dwFirst, DWORD dwSecond )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->GetDriverInfo( dwFirst, dwSecond );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetNewBuffer( DWORD dwFirst, DWORD dwSecond )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->SetNewBuffer( dwFirst, dwSecond );
}

HRESULT STDMETHODCALLTYPE ResManStatBuffer::SetA3dDirectCtrl( DWORD dwFirst, DWORD dwSecond )
{
	if ( m_pStream == NULL )
		return E_FAIL;

	IA3dPlatformRenderBuffer* pRenderBuffer = (IA3dPlatformRenderBuffer*)m_pStream->GetIDalBuffer();
	return pRenderBuffer->SetA3dDirectCtrl( dwFirst, dwSecond );
}

HRESULT STDMETHODCALLTYPE ResManBuffer::GetPriority( LPDWORD pdwControl )
{
	*pdwControl = m_dwControl;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManBuffer::GetBufferState( LPDWORD pdwStatus )
{
	*pdwStatus = m_dwStatus;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManBuffer::SetPriority( DWORD dwFlags )
{
	m_dwControl = dwFlags;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManBuffer::Mute()
{
	m_dwFlags |= A3D_RENDER_WRITE_BUFFER;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManBuffer::UnMute()
{
	m_dwFlags &= ~A3D_RENDER_WRITE_BUFFER;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE ResManBuffer::SetNotificationPositions( DWORD dwCount,
		LPCDSBPOSITIONNOTIFY pNotifications )
{
	if ( dwCount )
	{
		if ( pNotifications == NULL )
			return E_INVALIDARG;
		if ( m_hNotificationMutex != NULL )
			return DS_OK;

		m_hNotificationMutex = CreateMutexA( NULL, FALSE, NULL );
		if ( m_hNotificationMutex == NULL )
			return E_OUTOFMEMORY;

		WaitForSingleObject( m_hNotificationMutex, INFINITE );
		m_dwNotificationCount = dwCount;
		m_pNotifications = (DSBPOSITIONNOTIFY*)pNotifications;
		ReleaseMutex( m_hNotificationMutex );
	}
	else if ( m_hNotificationMutex != NULL )
	{
		WaitForSingleObject( m_hNotificationMutex, INFINITE );
		m_dwNotificationCount = 0;
		m_pNotifications = NULL;
		ReleaseMutex( m_hNotificationMutex );
	}

	return DS_OK;
}

HRESULT ResManBuffer::Init( const a3d_platform_buffer_desc_t* pDesc, void* pBufferToken )
{
	if ( pDesc == NULL )
		return E_POINTER;
	if ( pBufferToken == NULL )
		return E_POINTER;

	m_Desc.dwSize = pDesc->dwSize;
	m_Desc.dwFlags = pDesc->dwFlags;
	m_Desc.dwBufferBytes = pDesc->dwBufferBytes;
	m_Desc.dwReserved = pDesc->dwReserved;
	m_Desc.lpwfxFormat = pDesc->lpwfxFormat;
	m_pBufferToken = pBufferToken;
	m_dwOriginalFrequency = pDesc->lpwfxFormat->nSamplesPerSec;
	m_hData = GlobalAlloc( GMEM_FIXED, pDesc->dwBufferBytes );
	if ( m_hData == NULL )
		return E_OUTOFMEMORY;

	if ( pDesc->lpwfxFormat->nChannels == 2 )
		m_dwFlags |= A3D_RENDER_STEREO;
	if ( pDesc->lpwfxFormat->wBitsPerSample == 16 )
		m_dwFlags |= A3D_RENDER_16BIT;

	return DS_OK;
}

HRESULT ResManBuffer::GetRenderStream( DalBufferInfo** ppStream )
{
	*ppStream = m_pStream;
	return DS_OK;
}

HRESULT ResManStatBuffer::ProcessNotifications( DWORD dwTime )
{
	(void)dwTime;

	DWORD dwPreviousPosition = m_dwPreviousPosition;
	DWORD dwReadPosition;
	DWORD dwWritePosition;
	GetCurrentPosition( &dwReadPosition, &dwWritePosition );
	m_dwPreviousPosition = dwReadPosition;

	if ( m_hNotificationMutex != NULL )
	{
		WaitForSingleObject( m_hNotificationMutex, INFINITE );

		for ( DWORD i = 0; i < m_dwNotificationCount; i++ )
		{
			if ( m_pNotifications[i].dwOffset >= dwPreviousPosition &&
				m_pNotifications[i].dwOffset < dwReadPosition )
				SetEvent( m_pNotifications[i].hEventNotify );
		}

		ReleaseMutex( m_hNotificationMutex );
	}

	return DS_OK;
}

#define A3D_TIMING_BYTES_PER_MILLISECOND	11

EMUBuffer::EMUBuffer()
{
	m_cReferences = 0;
	m_dwFlags = 0;
	m_lVolume = 0;
	m_lPan = 0;
	m_dwOriginalFrequency = 0;
	m_dwFrequency = 0;
	m_dwBufferBytes = 0;
	m_dwReadPosition = 0;
	m_dwOverridePosition = A3D_BUFFER_POSITION_UNSET;
	m_dwStatus = A3D_BUFFER_STATE_STOPPED;
	m_dwStartTick = 0;
	m_dwLastTick = 0;
}

EMUBuffer::~EMUBuffer()
{
}

HRESULT STDMETHODCALLTYPE EMUBuffer::QueryInterface( REFIID iid, void** ppObject )
{
	if ( ppObject == NULL )
		return E_INVALIDARG;

	IUnknown* pObject;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) ||
		!memcmp( &iid, &IID_IDirectSoundBuffer, sizeof( IID ) ) )
	{
		pObject = (IDirectSoundBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dRenderBuffer, sizeof( IID ) ) )
	{
		pObject = (IA3dDalBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_A3dBufferControl, sizeof( IID ) ) )
	{
		pObject = (IResManBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_IDirectSoundNotify, sizeof( IID ) ) )
	{
		pObject = (IDirectSoundBuffer*)this;
	}
	else
	{
		*ppObject = NULL;
		return E_NOINTERFACE;
	}

	*ppObject = pObject;
	pObject->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE EMUBuffer::Release()
{
	if ( !InterlockedDecrement( &m_cReferences ) )
	{
		m_dwStatus = A3D_BUFFER_STATE_RELEASED;
		this->~EMUBuffer();
		::operator delete( this );
		return 0;
	}

	return m_cReferences;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetCaps( LPDSBCAPS )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetCurrentPosition( LPDWORD pdwReadPosition,
		LPDWORD pdwWritePosition )
{
	DWORD dwNow = GetTickCount();
	DWORD dwElapsed = dwNow - m_dwLastTick;
	if ( dwNow < m_dwLastTick )
		dwElapsed--;

	if ( m_dwStatus == A3D_BUFFER_STATE_PLAYING )
		dwElapsed *= A3D_TIMING_BYTES_PER_MILLISECOND;
	else
		dwElapsed = 0;

	if ( !( m_dwFlags & A3D_LEGACY_BUFFER_LOOPING ) &&
		m_dwReadPosition + dwElapsed >= m_Desc.dwBufferBytes )
	{
		m_dwReadPosition = m_Desc.dwBufferBytes;
		m_dwStatus = A3D_BUFFER_STATE_STOPPED;
	}
	else
	{
		m_dwReadPosition = ( m_dwReadPosition + dwElapsed ) % m_Desc.dwBufferBytes;
	}

	m_dwLastTick = dwNow;
	if ( m_dwOverridePosition == A3D_BUFFER_POSITION_UNSET )
	{
		*pdwReadPosition = m_dwReadPosition;
		*pdwWritePosition = m_dwReadPosition;
	}
	else
	{
		*pdwReadPosition = m_dwOverridePosition;
		*pdwWritePosition = m_dwOverridePosition;
		m_dwOverridePosition = A3D_BUFFER_POSITION_UNSET;
	}

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetFormat( LPWAVEFORMATEX, DWORD, LPDWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::Initialize( LPDIRECTSOUND, LPCDSBUFFERDESC )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::Lock( DWORD, DWORD, LPVOID*, LPDWORD, LPVOID*, LPDWORD, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::Play( DWORD, DWORD, DWORD dwFlags )
{
	if ( m_dwStatus == A3D_BUFFER_STATE_STOPPED && m_dwReadPosition == m_dwBufferBytes )
		m_dwReadPosition = 0;

	m_dwStatus = A3D_BUFFER_STATE_PLAYING;
	if ( dwFlags & DSBPLAY_LOOPING )
		m_dwFlags |= A3D_LEGACY_BUFFER_LOOPING;
	else
		m_dwFlags &= ~A3D_LEGACY_BUFFER_LOOPING;

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::SetCurrentPosition( DWORD dwPosition )
{
	m_dwOverridePosition = dwPosition;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::SetFormat( LPCWAVEFORMATEX )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::Unlock( LPVOID, DWORD, LPVOID, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::Restore()
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::SetA3dSuperCtrl( void*, DWORD )
{
	return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetAllocationStatus( DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetWave( DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetDriverInfo( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::SetNewBuffer( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::SetA3dDirectCtrl( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE EMUBuffer::GetStatusEx( DWORD )
{
	return DS_OK;
}

HRESULT EMUBuffer::Init( const a3d_platform_buffer_desc_t* pDesc )
{
	DWORD dwNow = GetTickCount();
	m_dwStartTick = dwNow;
	m_dwLastTick = dwNow;
	m_Desc.dwSize = pDesc->dwSize;
	m_Desc.dwFlags = pDesc->dwFlags;
	m_Desc.dwBufferBytes = pDesc->dwBufferBytes;
	m_Desc.dwReserved = pDesc->dwReserved;
	m_Desc.lpwfxFormat = pDesc->lpwfxFormat;
	m_dwOriginalFrequency = pDesc->lpwfxFormat->nSamplesPerSec;
	m_dwFrequency = m_dwOriginalFrequency;
	m_dwBufferBytes = pDesc->dwBufferBytes;
	return DS_OK;
}
