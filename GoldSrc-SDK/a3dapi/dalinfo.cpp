#include "dalinfo.h"
#include "A3dGeom.h"
#include "resman.h"

#include <new>
#include <string.h>

#define A3D_RENDER_MAX_WRITE_BYTES	0x4000
#define A3D_PCM8_BIAS					0x80
#define A3D_PCM8_SIGN_EXTENSION_MASK	0xFF80

DWORD DalInfo::GetNumAvailable()
{
	IDirectSound* pDirectSound = NULL;
	if ( FAILED( m_pBackend->GetDS( &pDirectSound ) ) )
		return 0;

	DSCAPS caps = {};
	caps.dwSize = sizeof( caps );
	if ( FAILED( pDirectSound->GetCaps( &caps ) ) )
		return 0;

	return caps.dwFreeHw3DAllBuffers;
}

HRESULT DalInfo::RefreshCapacity()
{
	IDirectSound* pDirectSound = NULL;
	HRESULT hr = m_pBackend->GetDS( &pDirectSound );
	if ( FAILED( hr ) )
		return hr;

	DSCAPS caps = {};
	caps.dwSize = sizeof( caps );
	hr = pDirectSound->GetCaps( &caps );
	if ( FAILED( hr ) )
		return hr;

	m_cMaxStreams = (LONG)caps.dwMaxHwMixingAllBuffers;
	return S_OK;
}

HRESULT DalInfo::FindUnused( DalBufferInfo** ppStream )
{
	*ppStream = NULL;
	a3d_render_stream_node_t* pNode = m_Streams.pHead;
	while ( pNode != NULL )
	{
		DalBufferInfo* pStream = pNode->pStream;
		pNode = pNode->pNext;
		if ( pStream->IsUnused() )
		{
			*ppStream = pStream;
			return S_OK;
		}
	}

	return S_OK;
}

void a3d_render_stream_list_t::Clear()
{
	cStreams = 0;
	pFreeNodes = NULL;
	pTail = NULL;
	pHead = NULL;
	CPlex* pBlocksToFree = pBlocks;
	if ( pBlocksToFree )
		pBlocksToFree->FreeDataChain();
	pBlocks = NULL;
}

HRESULT DalInfo::RemoveStream( DalBufferInfo* pStream )
{
	a3d_render_stream_node_t* pNode = m_Streams.pHead;
	while ( pNode != NULL )
	{
		a3d_render_stream_node_t* pNext = pNode->pNext;
		if ( pNode->pStream == pStream )
		{
			if ( pNode == m_Streams.pHead )
				m_Streams.pHead = pNode->pNext;
			else
				pNode->pPrevious->pNext = pNode->pNext;

			if ( pNode == m_Streams.pTail )
				m_Streams.pTail = pNode->pPrevious;
			else
				pNode->pNext->pPrevious = pNode->pPrevious;

			pNode->pNext = m_Streams.pFreeNodes;
			m_Streams.pFreeNodes = pNode;
			m_Streams.cStreams--;
			if ( m_Streams.cStreams == 0 )
				m_Streams.Clear();

			if ( pStream != NULL )
			{
				pStream->~DalBufferInfo();
				::operator delete( pStream );
			}
		}

		pNode = pNext;
	}

	pNode = m_Duplicates.pHead;
	while ( pNode != NULL )
	{
		a3d_render_stream_node_t* pNext = pNode->pNext;
		if ( pNode->pStream == pStream )
		{
			if ( pNode == m_Duplicates.pHead )
				m_Duplicates.pHead = pNode->pNext;
			else
				pNode->pPrevious->pNext = pNode->pNext;

			if ( pNode == m_Duplicates.pTail )
				m_Duplicates.pTail = pNode->pPrevious;
			else
				pNode->pNext->pPrevious = pNode->pPrevious;

			pNode->pNext = m_Duplicates.pFreeNodes;
			m_Duplicates.pFreeNodes = pNode;
			m_Duplicates.cStreams--;
			if ( m_Duplicates.cStreams == 0 )
				m_Duplicates.Clear();

			if ( pStream != NULL )
			{
				pStream->~DalBufferInfo();
				::operator delete( pStream );
			}
		}

		pNode = pNext;
	}

	return S_OK;
}

DalBufferInfo::~DalBufferInfo()
{
	if ( m_pRenderBuffer != NULL )
	{
		m_pRenderBuffer->Release();
		m_pRenderBuffer = NULL;
	}

	if ( m_pSoundBuffer != NULL )
	{
		m_pSoundBuffer->Release();
		m_pSoundBuffer = NULL;
	}
}

BOOL DalBufferInfo::IsUnused()
{
	return m_pOwner == NULL;
}

BOOL DalBufferInfo::IsExpired( DWORD dwTime )
{
	if ( !IsUnused() )
		return FALSE;

	return dwTime - m_dwUnusedSince > m_dwTimeout;
}

BOOL DalBufferInfo::Refresh( LPDWORD pdwQueuedBytes )
{
	if ( FAILED( GetBytesQueued( pdwQueuedBytes ) ) )
		return FALSE;

	return *pdwQueuedBytes < m_Description.dwUpdateThreshold;
}

HRESULT DalBufferInfo::GetBytesQueued( LPDWORD pdwQueuedBytes )
{
	DWORD dwReadPosition;
	DWORD dwWritePosition;
	HRESULT hr = m_pSoundBuffer->GetCurrentPosition( &dwReadPosition, &dwWritePosition );
	if ( FAILED( hr ) )
		return hr;

	if ( dwReadPosition == m_Description.dwBufferBytes )
		*pdwQueuedBytes = 0;
	else if ( dwReadPosition > m_dwWritePosition )
		*pdwQueuedBytes = m_dwWritePosition - dwReadPosition + m_Description.dwBufferBytes;
	else
		*pdwQueuedBytes = m_dwWritePosition - dwReadPosition;

	return S_OK;
}

HRESULT DalBufferInfo::GetAudioBytesQueued( LPDWORD pdwRemainingBytes )
{
	if ( m_dwStopPosition == A3D_RENDER_STOP_UNSET )
	{
		GetBytesQueued( pdwRemainingBytes );
		return S_OK;
	}

	DWORD dwReadPosition;
	DWORD dwWritePosition;
	HRESULT hr = m_pSoundBuffer->GetCurrentPosition( &dwReadPosition, &dwWritePosition );
	if ( FAILED( hr ) )
		return hr;

	if ( dwReadPosition > m_dwStopPosition )
	{
		DWORD dwStatus;
		hr = m_pSoundBuffer->GetStatus( &dwStatus );
		if ( FAILED( hr ) )
			return hr;

		if ( dwStatus & DSBSTATUS_LOOPING )
			*pdwRemainingBytes = m_Description.dwBufferBytes - dwReadPosition + m_dwStopPosition;
		else
			*pdwRemainingBytes = 0;
	}
	else
	{
		*pdwRemainingBytes = m_dwStopPosition - dwReadPosition;
	}

	return S_OK;
}

HRESULT DalBufferInfo::ConvertPcm( LPWORD pDestination, const BYTE* pSource,
		DWORD dwSamples, DWORD dwFlags )
{
	if ( !( dwFlags & A3D_RENDER_16BIT ) && !( dwFlags & A3D_RENDER_STEREO ) )
	{
		for ( DWORD i = 0; i < dwSamples; i++ )
		{
			WORD sample = (WORD)( ( pSource[0] ^ A3D_PCM8_BIAS ) << 8 );
			*pDestination = sample;
			*pDestination = (WORD)( sample + pSource[0] );
			pDestination++;
			pSource++;
		}
	}
	else if ( ( dwFlags & A3D_RENDER_16BIT ) && !( dwFlags & A3D_RENDER_STEREO ) )
	{
		DWORD dwBytes = dwSamples * sizeof( WORD );
		DWORD dwWords = dwBytes / sizeof( DWORD );
		DWORD* pDestinationWords = (DWORD*)pDestination;
		const DWORD* pSourceWords = (const DWORD*)pSource;

		for ( DWORD i = 0; i < dwWords; i++ )
			*pDestinationWords++ = *pSourceWords++;

		BYTE* pDestinationBytes = (BYTE*)pDestinationWords;
		pSource = (const BYTE*)pSourceWords;
		for ( DWORD i = 0; i < dwBytes % sizeof( DWORD ); i++ )
			*pDestinationBytes++ = *pSource++;
	}
	else if ( !( dwFlags & A3D_RENDER_16BIT ) && ( dwFlags & A3D_RENDER_STEREO ) )
	{
		for ( DWORD i = 0; i < dwSamples; i++ )
		{
			BYTE right = pSource[1] >> 1;
			BYTE left = pSource[0] >> 1;
			SHORT sample = (CHAR)( right + left );
			*pDestination++ = (WORD)( ( ( (WORD)sample ^ A3D_PCM8_SIGN_EXTENSION_MASK ) << 8 ) + sample );
			pSource += 2;
		}
	}
	else
	{
		for ( DWORD i = 0; i < dwSamples; i++ )
		{
			SHORT left = ( (const SHORT*)pSource )[0] >> 1;
			*pDestination = (WORD)left;
			SHORT right = ( (const SHORT*)pSource )[1] >> 1;
			*pDestination = (WORD)( left + right );
			pDestination++;
			pSource += 2 * sizeof( SHORT );
		}
	}

	return S_OK;
}

void DalBufferInfo::Reset()
{
	if ( m_Description.dwFlags & A3D_RENDER_WRITE_BUFFER )
	{
		LPVOID pFirst = NULL;
		DWORD dwFirstBytes = 0;
		LPVOID pSecond = NULL;
		DWORD dwSecondBytes = 0;
		m_pSoundBuffer->Lock( 0, m_Description.dwBufferBytes, &pFirst, &dwFirstBytes,
			&pSecond, &dwSecondBytes, DSBLOCK_ENTIREBUFFER );
		if ( pFirst )
			memset( pFirst, 0, m_Description.dwBufferBytes );
		m_pSoundBuffer->Unlock( pFirst, dwFirstBytes, pFirst, dwSecondBytes );
	}

	m_dwWritePosition = 0;
	m_dwStopPosition = A3D_RENDER_STOP_UNSET;
}

HRESULT DalBufferInfo::FormatAndFillBuffer( LPVOID pDestination, DWORD dwDestinationBytes,
		const BYTE* pSource, DWORD dwSourceBytes, LPDWORD pdwSourcePosition, DWORD dwFlags )
{
	if ( pDestination == NULL || pSource == NULL || pdwSourcePosition == NULL )
		return E_INVALIDARG;

	DWORD dwShift = 0;
	if ( dwFlags & A3D_RENDER_16BIT )
		dwShift = 1;
	if ( dwFlags & A3D_RENDER_STEREO )
		dwShift++;

	DWORD dwWrittenBytes = 0;
	while ( dwWrittenBytes < dwDestinationBytes )
	{
		DWORD dwAvailableSamples = ( dwSourceBytes - *pdwSourcePosition ) >> dwShift;
		DWORD dwRequestedSamples = ( dwDestinationBytes - dwWrittenBytes ) >> 1;
		if ( dwRequestedSamples > dwAvailableSamples )
		{
			ConvertPcm( (LPWORD)( (BYTE*)pDestination + dwWrittenBytes ), pSource + *pdwSourcePosition,
				dwAvailableSamples, dwFlags );
			dwWrittenBytes += dwAvailableSamples * sizeof( WORD );
			if ( dwFlags & A3D_RENDER_LOOPING )
			{
				*pdwSourcePosition = 0;
				continue;
			}

			memset( (BYTE*)pDestination + dwWrittenBytes, 0, dwDestinationBytes - dwWrittenBytes );
			*pdwSourcePosition = dwSourceBytes;
			if ( m_dwStopPosition == A3D_RENDER_STOP_UNSET )
				m_dwStopPosition = m_dwWritePosition + dwWrittenBytes;
			break;
		}

		ConvertPcm( (LPWORD)( (BYTE*)pDestination + dwWrittenBytes ), pSource + *pdwSourcePosition,
			dwRequestedSamples, dwFlags );
		*pdwSourcePosition += dwRequestedSamples << dwShift;
		break;
	}

	if ( dwSourceBytes == *pdwSourcePosition && !( dwFlags & A3D_RENDER_LOOPING ) )
	{
		DWORD dwStatus = 0;
		HRESULT hr = m_pSoundBuffer->GetStatus( &dwStatus );
		if ( FAILED( hr ) )
			return hr;

		if ( ( dwStatus & DSBSTATUS_PLAYING ) && ( dwStatus & DSBSTATUS_LOOPING ) )
			m_pSoundBuffer->Play( 0, 0, 0 );
	}

	return S_OK;
}

HRESULT DalBufferInfo::Write( DWORD, const BYTE* pSource, DWORD dwSourceBytes,
		LPDWORD pdwSourcePosition, DWORD dwFlags )
{
	DWORD dwQueuedBytes;
	if ( !Refresh( &dwQueuedBytes ) )
		return S_OK;

	DWORD dwWriteBytes = m_Description.dwBufferBytes - dwQueuedBytes;
	if ( dwWriteBytes > A3D_RENDER_MAX_WRITE_BYTES )
		dwWriteBytes = A3D_RENDER_MAX_WRITE_BYTES;

	if ( m_Description.dwFlags & A3D_RENDER_WRITE_BUFFER )
	{
		LPVOID pFirst = NULL;
		DWORD dwFirstBytes = 0;
		LPVOID pSecond = NULL;
		DWORD dwSecondBytes = 0;
		m_pSoundBuffer->Lock( m_dwWritePosition, dwWriteBytes, &pFirst, &dwFirstBytes,
			&pSecond, &dwSecondBytes, 0 );
		if ( pFirst != NULL )
			FormatAndFillBuffer( pFirst, dwFirstBytes, pSource, dwSourceBytes, pdwSourcePosition, dwFlags );
		if ( pSecond != NULL )
			FormatAndFillBuffer( pSecond, dwSecondBytes, pSource, dwSourceBytes, pdwSourcePosition, dwFlags );
		m_pSoundBuffer->Unlock( pFirst, dwFirstBytes, pSecond, dwSecondBytes );
	}
	else
	{
		if ( !( dwFlags & A3D_RENDER_LOOPING ) && *pdwSourcePosition + dwWriteBytes > dwSourceBytes )
			*pdwSourcePosition = dwSourceBytes;
		else
			*pdwSourcePosition = ( *pdwSourcePosition + dwWriteBytes ) % dwSourceBytes;
	}

	m_dwWritePosition = ( m_dwWritePosition + dwWriteBytes ) % m_Description.dwBufferBytes;
	return S_OK;
}

#define A3D_RENDER_LIST_BLOCK_NODES	10
#define A3D_RENDER_CACHE_TIMEOUT_MS	10000

#define A3D_RENDER_STREAM_DIRECT		1
#define A3D_RENDER_STREAM_POOLED		2

static const IID IID_A3dRenderBuffer =
	{ 0xe04814d8, 0x26fc, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };

DalInfo::DalInfo( IA3dDal* pBackend,
		const a3d_render_description_t* pDescription )
{
	m_Streams = {};
	m_Streams.cBlockNodes = A3D_RENDER_LIST_BLOCK_NODES;
	m_Duplicates = {};
	m_Duplicates.cBlockNodes = A3D_RENDER_LIST_BLOCK_NODES;

	try
	{
		m_pBackend = pBackend;
		m_pBackend->AddRef();
		m_Description = *pDescription;
		m_cMaxStreams = 0;
	}
	catch ( ... )
	{
		m_Duplicates.Clear();
		m_Streams.Clear();
		throw;
	}
}

HRESULT DalInfo::Update( DWORD dwTime )
{
	a3d_render_stream_node_t* pNode = m_Streams.pHead;
	while ( pNode != NULL )
	{
		a3d_render_stream_node_t* pNext = pNode->pNext;
		DalBufferInfo* pStream = pNode->pStream;
		if ( pStream->IsUnused() && pStream->IsExpired( dwTime ) )
		{
			if ( pNode == m_Streams.pHead )
				m_Streams.pHead = pNode->pNext;
			else
				pNode->pPrevious->pNext = pNode->pNext;

			if ( pNode == m_Streams.pTail )
				m_Streams.pTail = pNode->pPrevious;
			else
				pNode->pNext->pPrevious = pNode->pPrevious;

			pNode->pNext = m_Streams.pFreeNodes;
			m_Streams.pFreeNodes = pNode;
			m_Streams.cStreams--;
			if ( m_Streams.cStreams == 0 )
				m_Streams.Clear();

			if ( pStream != NULL )
			{
				pStream->~DalBufferInfo();
				::operator delete( pStream );
			}
		}

		pNode = pNext;
	}

	return S_OK;
}

HRESULT DalInfo::CreateStaticDalBuffer( const a3d_platform_buffer_desc_s* pDescription,
		DalBufferInfo** ppStream )
{
	if ( m_Streams.cStreams >= m_cMaxStreams )
		return E_OUTOFMEMORY;

	IDirectSoundBuffer* pSoundBuffer = (IDirectSoundBuffer*)pDescription;
	HRESULT hr = m_pBackend->CreateSoundBufferEx( pDescription, NULL, &pSoundBuffer, NULL );
	if ( FAILED( hr ) )
		return hr;

	DalBufferInfo* pStream = new( std::nothrow ) DalBufferInfo( this, pSoundBuffer,
		&m_Description, A3D_RENDER_STREAM_DIRECT );
	if ( pStream == NULL )
		return E_OUTOFMEMORY;

	if ( pSoundBuffer != NULL )
	{
		pSoundBuffer->Release();
		pSoundBuffer = NULL;
	}

	a3d_render_stream_node_t* pTail = m_Duplicates.pTail;
	if ( m_Duplicates.pFreeNodes == NULL )
	{
		CPlex* pBlock = CPlex::Create( &m_Duplicates.pBlocks,
			m_Duplicates.cBlockNodes, sizeof( a3d_render_stream_node_t ) );
		a3d_render_stream_node_t* pNodes = (a3d_render_stream_node_t*)( pBlock + 1 );
		for ( LONG i = m_Duplicates.cBlockNodes; i > 0; i-- )
		{
			pNodes[i - 1].pNext = m_Duplicates.pFreeNodes;
			m_Duplicates.pFreeNodes = &pNodes[i - 1];
		}
	}

	a3d_render_stream_node_t* pNode = m_Duplicates.pFreeNodes;
	m_Duplicates.pFreeNodes = pNode->pNext;
	pNode->pPrevious = pTail;
	pNode->pNext = NULL;
	m_Duplicates.cStreams++;
	pNode->pStream = pStream;
	if ( m_Duplicates.pTail != NULL )
		m_Duplicates.pTail->pNext = pNode;
	else
		m_Duplicates.pHead = pNode;
	m_Duplicates.pTail = pNode;

	if ( ppStream != NULL )
		*ppStream = pStream;
	return S_OK;
}

#pragma warning( push )
#pragma warning( disable : 4701 4703 )
HRESULT DalInfo::CreateDalBuffer( DalBufferInfo* pSource, DalBufferInfo** ppStream )
{
	if ( m_Streams.cStreams >= m_cMaxStreams )
		return E_OUTOFMEMORY;

	IDirectSound* pDirectSound = (IDirectSound*)m_pBackend;
	m_pBackend->QueryInterface( IID_IDirectSound, (void**)&pDirectSound );
	IDirectSoundBuffer* pSoundBuffer;
	if ( pDirectSound != NULL )
	{
		if ( FAILED( pDirectSound->DuplicateSoundBuffer( pSource->GetDSBuffer(), &pSoundBuffer ) ) )
			return E_OUTOFMEMORY;
	}

	DalBufferInfo* pStream = new( std::nothrow ) DalBufferInfo( this, pSoundBuffer,
		&m_Description, A3D_RENDER_STREAM_DIRECT );
	if ( pStream == NULL )
		return E_OUTOFMEMORY;

	if ( pDirectSound != NULL )
	{
		pDirectSound->Release();
		pDirectSound = NULL;
	}
	if ( pSoundBuffer != NULL )
	{
		pSoundBuffer->Release();
		pSoundBuffer = NULL;
	}

	a3d_render_stream_node_t* pTail = m_Duplicates.pTail;
	if ( m_Duplicates.pFreeNodes == NULL )
	{
		CPlex* pBlock = CPlex::Create( &m_Duplicates.pBlocks,
			m_Duplicates.cBlockNodes, sizeof( a3d_render_stream_node_t ) );
		a3d_render_stream_node_t* pNodes = (a3d_render_stream_node_t*)( pBlock + 1 );
		for ( LONG i = m_Duplicates.cBlockNodes; i > 0; i-- )
		{
			pNodes[i - 1].pNext = m_Duplicates.pFreeNodes;
			m_Duplicates.pFreeNodes = &pNodes[i - 1];
		}
	}

	a3d_render_stream_node_t* pNode = m_Duplicates.pFreeNodes;
	m_Duplicates.pFreeNodes = pNode->pNext;
	pNode->pPrevious = pTail;
	pNode->pNext = NULL;
	m_Duplicates.cStreams++;
	pNode->pStream = pStream;
	if ( m_Duplicates.pTail != NULL )
		m_Duplicates.pTail->pNext = pNode;
	else
		m_Duplicates.pHead = pNode;
	m_Duplicates.pTail = pNode;

	if ( ppStream != NULL )
		*ppStream = pStream;
	return S_OK;
}
#pragma warning( pop )

HRESULT DalInfo::CreateDalBuffer( DalBufferInfo** ppStream )
{
	if ( m_Streams.cStreams >= m_cMaxStreams )
		return E_OUTOFMEMORY;

	PCMWAVEFORMAT format = {};
	format.wf.wFormatTag = WAVE_FORMAT_PCM;
	format.wf.nChannels = 1;
	format.wf.nSamplesPerSec = m_Description.dwSampleRate;
	format.wBitsPerSample = (WORD)m_Description.dwBitsPerSample;
	format.wf.nBlockAlign = (WORD)( (WORD)m_Description.dwBitsPerSample >> 3 );
	format.wf.nAvgBytesPerSec = format.wf.nSamplesPerSec * format.wf.nBlockAlign;

	a3d_platform_buffer_desc_t description = {};
	description.dwSize = sizeof( description );
	description.dwFlags = DSBCAPS_LOCHARDWARE | DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY |
		DSBCAPS_CTRLPAN | DSBCAPS_CTRLVOLUME;
	description.dwBufferBytes = m_Description.dwBufferBytes;
	description.lpwfxFormat = (LPWAVEFORMATEX)&format;
	IDirectSoundBuffer* pSoundBuffer;
	HRESULT hr = m_pBackend->CreateSoundBufferEx( &description, NULL, &pSoundBuffer, NULL );
	if ( FAILED( hr ) )
		return hr;

	DalBufferInfo* pStream = new( std::nothrow ) DalBufferInfo( this, pSoundBuffer,
		&m_Description, A3D_RENDER_STREAM_POOLED );
	if ( pStream == NULL )
		return E_OUTOFMEMORY;

	if ( pSoundBuffer != NULL )
	{
		pSoundBuffer->Release();
		pSoundBuffer = NULL;
	}

	a3d_render_stream_node_t* pTail = m_Streams.pTail;
	if ( m_Streams.pFreeNodes == NULL )
	{
		CPlex* pBlock = CPlex::Create( &m_Streams.pBlocks,
			m_Streams.cBlockNodes, sizeof( a3d_render_stream_node_t ) );
		a3d_render_stream_node_t* pNodes = (a3d_render_stream_node_t*)( pBlock + 1 );
		for ( LONG i = m_Streams.cBlockNodes; i > 0; i-- )
		{
			pNodes[i - 1].pNext = m_Streams.pFreeNodes;
			m_Streams.pFreeNodes = &pNodes[i - 1];
		}
	}

	a3d_render_stream_node_t* pNode = m_Streams.pFreeNodes;
	m_Streams.pFreeNodes = pNode->pNext;
	pNode->pPrevious = pTail;
	pNode->pNext = NULL;
	m_Streams.cStreams++;
	pNode->pStream = pStream;
	if ( m_Streams.pTail != NULL )
		m_Streams.pTail->pNext = pNode;
	else
		m_Streams.pHead = pNode;
	m_Streams.pTail = pNode;

	if ( ppStream != NULL )
		*ppStream = pStream;
	return S_OK;
}

DalBufferInfo::DalBufferInfo( DalInfo* pPool, IDirectSoundBuffer* pSoundBuffer,
		const a3d_render_description_t* pDescription, DWORD dwType )
{
	m_pSoundBuffer = pSoundBuffer;
	m_pSoundBuffer->AddRef();
	m_pPool = pPool;
	m_Description = *pDescription;
	m_dwUnusedSince = GetTickCount();
	m_dwType = dwType;
	m_dwTimeout = A3D_RENDER_CACHE_TIMEOUT_MS;
	m_dwWritePosition = 0;
	m_dwStopPosition = A3D_RENDER_STOP_UNSET;
	m_dwState = 1;
	m_pOwner = NULL;
	if ( FAILED( m_pSoundBuffer->QueryInterface( IID_A3dRenderBuffer, (void**)&m_pRenderBuffer ) ) )
		m_pRenderBuffer = NULL;
}
