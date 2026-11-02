#define _USE_MATH_DEFINES

#include "a2dbuffer.h"
#include "HrtfMgr.h"
#include "A3dSource.h"

#include <string.h>
#include <math.h>

#define A3D_MIXER_RATE_22050			22
#define A3D_MIXER_SOURCE_8BIT			0x20
#define A3D_MIXER_SOURCE_MONO			0x01
#define A3D_MIXER_SOURCE_STEREO		0x06
#define A3D_MIXER_GAIN_SCALE			49150.5f
#define A3D_MIXER_DELAY_SCALE			( 22050.0f * 65536.0f )

static const IID IID_A3dRenderBuffer =
	{ 0xe04814d8, 0x26fc, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };
static const IID IID_A3dBufferControl =
	{ 0xcbd91fa8, 0x41c6, 0x11d2, { 0xbb, 0x48, 0x00, 0x60, 0x08, 0x2f, 0x3c, 0x00 } };

A2DBuffer::A2DBuffer()
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
	m_dwControl = 0;
	m_dwStatus = 0;
	m_pcSharedReferences = NULL;
	m_pSamples = NULL;
}

A2DBuffer::~A2DBuffer()
{
	if ( m_pSamples != NULL )
	{
		GlobalFree( m_pSamples );
		m_pSamples = NULL;
	}
}

HRESULT STDMETHODCALLTYPE A2DBuffer::QueryInterface( REFIID iid, void** ppObject )
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

ULONG STDMETHODCALLTYPE A2DBuffer::AddRef()
{
	++*m_pcSharedReferences;
	return InterlockedIncrement( &m_cReferences );
}

ULONG STDMETHODCALLTYPE A2DBuffer::Release()
{
	--*m_pcSharedReferences;
	if ( !InterlockedDecrement( &m_cReferences ) )
	{
		m_dwStatus = A3D_BUFFER_STATE_RELEASED;
		return 0;
	}

	return m_cReferences;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetCaps( LPDSBCAPS )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetFormat( LPWAVEFORMATEX, DWORD, LPDWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::Initialize( LPDIRECTSOUND, LPCDSBUFFERDESC )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::Lock( DWORD dwOffset, DWORD dwBytes, LPVOID* ppFirstBlock,
		LPDWORD pdwFirstBytes, LPVOID* ppSecondBlock, LPDWORD pdwSecondBytes, DWORD dwFlags )
{
	if ( ppFirstBlock == NULL || pdwFirstBytes == NULL || dwBytes > m_Desc.dwBufferBytes )
		return DSERR_INVALIDPARAM;

	if ( ( dwFlags & DSBLOCK_ENTIREBUFFER ) == TRUE )
		dwBytes = m_Desc.dwBufferBytes;
	if ( dwFlags & DSBLOCK_FROMWRITECURSOR )
		dwOffset = m_dwReadPosition;

	*ppFirstBlock = m_pSamples + dwOffset;
	if ( dwOffset + dwBytes <= m_Desc.dwBufferBytes )
	{
		*pdwFirstBytes = dwBytes;
		if ( ppSecondBlock != NULL && pdwSecondBytes != NULL )
		{
			*ppSecondBlock = NULL;
			*pdwSecondBytes = 0;
		}
	}
	else
	{
		*pdwFirstBytes = m_Desc.dwBufferBytes - dwOffset;
		if ( ppSecondBlock != NULL && pdwSecondBytes != NULL )
		{
			*ppSecondBlock = m_pSamples;
			*pdwSecondBytes = dwBytes - *pdwFirstBytes;
		}
	}

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::Play( DWORD, DWORD, DWORD dwFlags )
{
	if ( m_dwStatus == A3D_BUFFER_STATE_STOPPED && m_dwReadPosition == m_dwBufferBytes )
	{
		m_dwReadPosition = 0;
		m_dwOverridePosition = 0;
	}

	m_dwStatus = A3D_BUFFER_STATE_PLAYING;
	if ( dwFlags & DSBPLAY_LOOPING )
	{
		m_dwFlags |= A3D_LEGACY_BUFFER_LOOPING;
		m_Input.dwFlags |= A3D_MIX_LOOPING;
	}
	else
	{
		m_dwFlags &= ~A3D_LEGACY_BUFFER_LOOPING;
		m_Input.dwFlags &= ~A3D_MIX_LOOPING;
	}

	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::SetCurrentPosition( DWORD dwPosition )
{
	m_dwOverridePosition = dwPosition;
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::SetFormat( LPCWAVEFORMATEX )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::Unlock( LPVOID, DWORD, LPVOID, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::Restore()
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetAllocationStatus( DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetWave( DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetDriverInfo( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::SetNewBuffer( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::SetA3dDirectCtrl( DWORD, DWORD )
{
	return DS_OK;
}

HRESULT STDMETHODCALLTYPE A2DBuffer::GetStatusEx( DWORD )
{
	return DS_OK;
}

HRESULT A2DBuffer::Init( const a3d_platform_buffer_desc_t* pDesc, CHrtfMgr* pHrtfFilter,
		LONG* pcSharedReferences )
{
	m_pcSharedReferences = pcSharedReferences;
	m_Desc = *pDesc;
	m_dwOriginalFrequency = pDesc->lpwfxFormat->nSamplesPerSec;
	m_dwFrequency = m_dwOriginalFrequency;
	m_pSamples = (BYTE*)GlobalAlloc( GMEM_FIXED, pDesc->dwBufferBytes );
	m_dwBufferBytes = pDesc->dwBufferBytes;
	if ( m_pSamples == NULL )
		return E_OUTOFMEMORY;

	m_Input.pSamples = m_pSamples;
	m_Input.pEnd = m_pSamples + m_dwBufferBytes;
	m_Input.cFrames = m_dwBufferBytes;
	m_Input.pCurrent = m_pSamples;
	m_Input.dwFlags = 0;
	if ( pDesc->lpwfxFormat->wBitsPerSample == 8 )
	{
		m_Input.dwFlags = A3D_MIXER_SOURCE_8BIT;
	}
	else
	{
		m_Input.cFrames >>= 1;
		m_Input.dwFlags = A3D_MIX_16BIT;
	}

	if ( pDesc->lpwfxFormat->nChannels == 1 )
	{
		m_Input.dwFlags |= A3D_MIXER_SOURCE_MONO;
	}
	else
	{
		m_Input.dwFlags |= A3D_MIXER_SOURCE_STEREO;
		m_Input.cFrames >>= 1;
	}

	m_pHrtfFilter = pHrtfFilter;
	AR2Dinit( &m_Voice, &m_Input, A3D_MIXER_RATE_22050 );
	SetA2DFrequency( pDesc->lpwfxFormat->nSamplesPerSec );
	return DS_OK;
}

DWORD A2DBuffer::SetA2DFrequency( DWORD dwFrequency )
{
	return AR2DSetFrequency( &m_Voice, dwFrequency );
}

HRESULT STDMETHODCALLTYPE A2DBuffer::SetA3dSuperCtrl( void* pData, DWORD )
{
	const A3DCTRL_SRC_SUPER* pState = (const A3DCTRL_SRC_SUPER*)pData;
	DWORD dwFrequency = (DWORD)(LONGLONG)( (double)m_dwFrequency * pState->flPitch );
	SetA2DFrequency( dwFrequency );
	m_Voice.channels[0].lGainBase = (LONG)(LONGLONG)( (double)pState->Left.flGain * A3D_MIXER_GAIN_SCALE );
	m_Voice.channels[1].lGainBase = (LONG)(LONGLONG)( (double)pState->Right.flGain * A3D_MIXER_GAIN_SCALE );

	if ( !( pState->Left.flDelay >= 0.0f ) )
	{
		m_Voice.channels[0].lDelayTarget = 0;
		m_Voice.channels[1].lDelayTarget = (LONG)(LONGLONG)(
			(double)pState->Right.flDelay * A3D_MIXER_DELAY_SCALE -
			(double)pState->Left.flDelay * A3D_MIXER_DELAY_SCALE );
	}
	else
	{
		m_Voice.channels[1].lDelayTarget = 0;
		m_Voice.channels[0].lDelayTarget = (LONG)(LONGLONG)(
			(double)pState->Left.flDelay * A3D_MIXER_DELAY_SCALE -
			(double)pState->Right.flDelay * A3D_MIXER_DELAY_SCALE );
	}

	int indices[4];
	float weights[4];
	m_pHrtfFilter->GetIDXsAndWts( pState->Left.flAzimuth, pState->Left.flElevation,
		0, indices, weights );
	m_pHrtfFilter->GetCoeffs( indices, weights, 0, &m_Voice.channels[0].lGainSpatial );
	m_pHrtfFilter->GetIDXsAndWts( pState->Right.flAzimuth, pState->Right.flElevation,
		1, indices, weights );
	m_pHrtfFilter->GetCoeffs( indices, weights, 0, &m_Voice.channels[1].lGainSpatial );
	return DS_OK;
}

#define HRTF_PI				( (float)M_PI )
#define HRTF_FULL_TURN		( 2.0f * HRTF_PI )
#define HRTF_HALF_PI		( 0.5f * HRTF_PI )
#define HRTF_INV_FULL_TURN	( 1.0f / HRTF_FULL_TURN )
#define HRTF_EAR_EXTRA_SAMPLES	3

static const SHORT filterCoefficients[] =
{
	11882, 11823, 10877, 10763, 11566, 11238, 9454, 8992,
	8457, 8495, 8645, 8761, 8367, 7796, 7194, 7049,
	6786, 6724, 8053, 7515, 8951, 8962, 7903, 7000,
	9136, 8310, 8166, 8292, 8115, 7694, 10901, 9484,
	8795, 8686, 8962, 9065, 15623, 13909, 14258, 15065,
	14655, 13924, 22476, 23215, 24734, 24398, 24846, 21578,
	25345, 28732, 31294, 30470, 31127, 26823, 24134, 26822,
	29465, 30489, 30578, 27415, 18701, 19797, 19806, 19060,
	17962, 19807, 14604, 14464, 13491, 12929, 13487, 13813,
	14896, 14495, 32767, 13339, 11702, 11382, 11350, 12364,
	12319, 16361, 14963, 15099, 14942, 15995, 17489, 20129,
	20066, 21278, 22197, 24470, 26115, 24353, 26514, 29444,
	30918, 32767, 32112, 24379, 27571, 28263, 27300, 27818,
	26070, 20528, 22712, 22446, 21809, 20538, 18543, 13936,
	13949, 13274, 12744, 12107, 12000, 10314, 9646, 8630,
	8307, 8294, 8420, 8709, 8405, 8049, 8080, 7835,
	7431, 8127, 7573, 8868, 9083, 7877, 7010, 8690,
	7815, 7318, 7240, 7209, 6969, 10268, 9183, 8890,
	9103, 10016, 10051, 14934, 15383, 32767, 0, 0
};

static const SHORT auxiliaryCoefficients[] =
{
	256, 256, 512, 512, 512, 256, 2304, 3072,
	3584, 3584, 3584, 3072, 3584, 5120, 5888, 6144,
	5632, 4864, 4096, 5888, 7680, 7936, 6912, 5632,
	3328, 4608, 5376, 5376, 5120, 4096, 2048, 2560,
	2816, 2816, 2560, 2048, 256, 256, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 256, 256, 256, 1280, 1792, 2816,
	3072, 2816, 2560, 2816, 4096, 5120, 5376, 5376,
	4608, 3584, 5120, 7424, 7936, 6912, 5632, 3328,
	4608, 5632, 5632, 4864, 4352, 1536, 2304, 2304,
	2560, 2304, 2048, 0, 0, 0, 0, 0
};

struct a3d_hrtf_definition_s
{
	DWORD	dwSampleRate;
	DWORD	nCoefficients;
	DWORD	dwCoefficientStride;
	DWORD	dwSelector;
	const SHORT* pTables[2];
	DWORD	dwReservedBeforeGrid[2];
	int		nAzimuthDivisions;
	int		nElevationDivisions;
	int		nFirstElevation;
	int		nLastElevation;
	DWORD	dwReservedAfterGrid[2];
};

static const a3d_hrtf_definition_s defaultDefinition =
{
	22050, 1, 1, 0,
	{ filterCoefficients, auxiliaryCoefficients },
	{ 0, 0 },
	12, 10, 2, 7,
	{ 0, 0 }
};

static const a3d_hrtf_definition_s* const filterDefinitions[] =
{
	&defaultDefinition
};

CHrtfMgr::CHrtfMgr()
{
	m_bInitialized = FALSE;
	m_bInterpolationReady = FALSE;
	m_dwReserved = 1;
	m_pDefinition = NULL;
}

CHrtfMgr::~CHrtfMgr()
{
}

HRESULT CHrtfMgr::Initialize( DWORD dwSampleRate, DWORD nCoefficients, DWORD dwSelector )
{
	const a3d_hrtf_definition_s* pExact = NULL;
	const a3d_hrtf_definition_s* pFallback = NULL;
	for ( size_t i = 0; i < sizeof( filterDefinitions ) / sizeof( filterDefinitions[0] ); i++ )
	{
		const a3d_hrtf_definition_s* pDefinition = filterDefinitions[i];
		if ( dwSampleRate != pDefinition->dwSampleRate || dwSelector != pDefinition->dwSelector )
			continue;

		if ( nCoefficients == pDefinition->nCoefficients )
		{
			pExact = pDefinition;
			break;
		}

		if ( nCoefficients > pDefinition->nCoefficients &&
			( !pFallback || pDefinition->nCoefficients > pFallback->nCoefficients ) )
			pFallback = pDefinition;
	}

	const a3d_hrtf_definition_s* pDefinition = pExact ? pExact : pFallback;
	if ( !pDefinition )
		return E_FAIL;

	m_pDefinition = pDefinition;
	m_nCoefficients = nCoefficients;
	m_bInterpolationReady = TRUE;
	m_bInitialized = TRUE;
	return S_OK;
}

HRESULT CHrtfMgr::GetCoeffs( const int indices[4], const float weights[4], DWORD,
		LONG* pCoefficients )
{
	if ( !m_bInitialized || !m_bInterpolationReady )
		return E_ACCESSDENIED;

	for ( int i = 0; i < 4; i++ )
	{
		if ( (DWORD)indices[i] * m_pDefinition->dwCoefficientStride + m_pDefinition->nCoefficients >
			sizeof( filterCoefficients ) / sizeof( filterCoefficients[0] ) )
			return E_ACCESSDENIED;
	}

	const SHORT* samples[4];
	float factors[4];
	for ( int i = 0; i < 4; i++ )
	{
		DWORD offset = (DWORD)indices[i] * m_pDefinition->dwCoefficientStride;
		samples[i] = &m_pDefinition->pTables[0][offset];
		factors[i] = weights[i];
	}

	DWORD nCoefficients = m_pDefinition->nCoefficients;
	for ( DWORD i = 0; i < nCoefficients; i++ )
	{
		float firstPair = (float)( (double)samples[1][i] * factors[1] +
			(double)samples[0][i] * factors[0] );
		float firstThree = (float)( (double)samples[2][i] * factors[2] + firstPair );
		pCoefficients[i] = (LONG)(LONGLONG)( (double)samples[3][i] * factors[3] + firstThree );
	}

	for ( DWORD i = nCoefficients; i < m_nCoefficients; i++ )
		pCoefficients[i] = 0;

	return S_OK;
}

HRESULT CHrtfMgr::GetIDXsAndWts( float azimuth, float elevation, int nEar,
		int indices[4], float weights[4] )
{
	if ( !m_bInitialized )
		return E_ACCESSDENIED;

	int nAzimuths = m_pDefinition->nAzimuthDivisions;
	int nElevations = m_pDefinition->nElevationDivisions;
	int nFirstElevation = m_pDefinition->nFirstElevation;
	int nLastElevation = m_pDefinition->nLastElevation;
	int nRows = nLastElevation - nFirstElevation + 1;
	int nRingSamples = nRows * nAzimuths;
	double azimuthStep = (double)HRTF_FULL_TURN / nAzimuths;
	double elevationStep = (double)HRTF_PI / nElevations;
	double azimuthOffset = (double)azimuth + HRTF_PI;
	int nTurns = (int)(LONGLONG)( azimuthOffset * HRTF_INV_FULL_TURN );
	float wrappedAzimuth = (float)( azimuthOffset - (double)nTurns * HRTF_FULL_TURN );
	int nAzimuth = (int)(LONGLONG)( wrappedAzimuth / azimuthStep ) % nAzimuths;
	double elevationFromTop = (double)HRTF_HALF_PI - elevation;
	int nElevation = (int)(LONGLONG)( elevationFromTop / elevationStep ) % nElevations;
	double upperWeight;

	if ( nElevation < nFirstElevation )
	{
		indices[0] = nRingSamples;
		indices[1] = nAzimuth * nRows;
		indices[2] = nRingSamples;
		indices[3] = ( nAzimuth + 1 ) % nAzimuths * nRows;
		upperWeight = 1.0 - elevationFromTop / ( nFirstElevation * elevationStep );
	}
	else if ( nElevation >= nLastElevation )
	{
		indices[0] = nAzimuth * nRows + nRows - 1;
		indices[1] = nRingSamples + 1;
		indices[2] = ( nAzimuth + 1 ) % nAzimuths * nRows + nRows - 1;
		indices[3] = nRingSamples + 1;
		upperWeight = 1.0 - ( elevationFromTop - nLastElevation * elevationStep ) /
			( ( nElevations - nLastElevation ) * elevationStep );
	}
	else
	{
		int nRow = nElevation - nFirstElevation;
		indices[0] = nAzimuth * nRows + nRow;
		indices[1] = indices[0] + 1;
		indices[2] = ( indices[0] + nRows ) % nRingSamples;
		indices[3] = ( indices[0] + nRows + 1 ) % nRingSamples;
		upperWeight = 1.0 - ( elevationFromTop - ( nFirstElevation + nRow ) * elevationStep ) /
			elevationStep;
	}

	float lowerWeight = (float)( 1.0 - upperWeight );
	double leftWeight = 1.0 - ( wrappedAzimuth - nAzimuth * azimuthStep ) / azimuthStep;
	double rightWeight = 1.0 - leftWeight;
	float roundedRightWeight = (float)rightWeight;
	weights[0] = (float)( upperWeight * leftWeight );
	weights[1] = (float)( leftWeight * lowerWeight );
	weights[2] = (float)( upperWeight * roundedRightWeight );
	weights[3] = (float)( rightWeight * lowerWeight );

	if ( nEar == 1 )
	{
		for ( int i = 0; i < 4; i++ )
			indices[i] += nRingSamples + HRTF_EAR_EXTRA_SAMPLES;
	}

	return S_OK;
}
