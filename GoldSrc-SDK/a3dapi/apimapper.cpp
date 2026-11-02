#include "ChunkPage.h"
#include "apimapper.h"
#include "A3d3.h"

#include <math.h>
#include <new>
#include <string.h>

CA3dCommandBuffer::CA3dCommandBuffer( int cCapacity, int iBufferType )
{
	m_pCommands = NULL;
	m_iCursor = 0;
	m_cCapacity = 0;
	m_cCommands = 0;

	size_t commandBytes = (DWORD)cCapacity * sizeof( a3d_geometry_command_t );
	m_pCommands = (a3d_geometry_command_t*)::operator new( commandBytes, std::nothrow );
	if ( m_pCommands )
	{
		m_cCapacity = cCapacity;
		m_iCursor = 0;
	}

	memset( m_pCommands, 0, commandBytes );
	m_iBufferType = iBufferType;
}

CA3dCommandBuffer::~CA3dCommandBuffer()
{
	if ( m_cCapacity > 0 )
		::operator delete( m_pCommands );
}

int __cdecl CommandBuffer_CheckFit( const CA3dCommandBuffer* pDest, int iDest,
		const CA3dCommandBuffer* pSource )
{
	int cAvailable = (int)( (DWORD)pDest->m_cCapacity - (DWORD)iDest );
	if ( pSource->m_cCapacity > cAvailable )
		return 0;

	return pSource->m_cCapacity;
}

int __cdecl CommandBuffer_Copy( CA3dCommandBuffer* pDest, int iDest,
		const CA3dCommandBuffer* pSource, int iSource, int cCommands )
{
	int cAvailable = (int)( (DWORD)pDest->m_cCapacity - (DWORD)iDest );
	if ( cCommands > cAvailable )
		return 0;

	DWORD commandBytes = (DWORD)cCommands * sizeof( a3d_geometry_command_t );
	memcpy( &pDest->m_pCommands[iDest], &pSource->m_pCommands[iSource], commandBytes );

	return cCommands;
}

void* CA3dCommandBuffer::Next( DWORD* pdwOpcode )
{
	if ( m_iCursor >= m_cCommands )
		return NULL;

	*pdwOpcode = 0;
	a3d_geometry_command_t* pCommands = m_pCommands;
	do
	{
		DWORD dwType = pCommands[m_iCursor].m_dwOpcode & A3D_VERTEX_MASK;
		if ( dwType >= A3D_LINES && dwType <= A3D_QUADS )
		{
			*pdwOpcode = pCommands[m_iCursor].m_dwOpcode;
			int iCommand = m_iCursor;
			m_iCursor = (int)( (DWORD)iCommand + 1 );
			return &m_pCommands[iCommand].m_Payload;
		}

		m_iCursor = (int)( (DWORD)m_iCursor + 1 );
	}
	while ( m_iCursor < m_cCommands );

	return NULL;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	*ppObject = NULL;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( iid ) ) ||
		!memcmp( &iid, &IID_IDirectSoundBuffer, sizeof( iid ) ) )
	{
		*ppObject = (IDirectSoundBuffer*)this;
	}
	else if ( !memcmp( &iid, &IID_IDirectSound3DBuffer, sizeof( iid ) ) )
	{
		*ppObject = (IDirectSound3DBuffer*)this;
	}

	if ( !*ppObject )
		return E_NOINTERFACE;

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dMapperSecBuffer::AddRef()
{
	InterlockedIncrement( &m_cReferences );
	return m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dMapperSecBuffer::Release()
{
	InterlockedDecrement( &m_cReferences );
	ULONG cReferences = m_cReferences;
	if ( !cReferences )
	{
		delete this;
		return 0;
	}

	return cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetCaps( LPDSBCAPS )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetCurrentPosition( LPDWORD pdwReadPosition, LPDWORD pdwWritePosition )
{
	m_pSource->GetWavePosition( pdwReadPosition );
	*pdwWritePosition = 0;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetFormat( LPWAVEFORMATEX, DWORD, LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetVolume( LPLONG plVolume )
{
	*plVolume = m_lVolume;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetPan( LPLONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetFrequency( LPDWORD pdwFrequency )
{
	WAVEFORMATEX format;
	D3DVALUE flPitch;

	m_pSource->GetWaveFormat( &format );
	m_pSource->GetPitch( &flPitch );
	*pdwFrequency = (DWORD)(LONGLONG)( (double)format.nSamplesPerSec * flPitch );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetStatus( LPDWORD pdwStatus )
{
	if ( !pdwStatus )
		return E_INVALIDARG;

	m_pSource->GetStatus( pdwStatus );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Initialize( LPDIRECTSOUND, LPCDSBUFFERDESC )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Lock( DWORD dwOffset, DWORD dwBytes, LPVOID* ppFirst,
		LPDWORD pdwFirstBytes, LPVOID* ppSecond, LPDWORD pdwSecondBytes, DWORD dwFlags )
{
	return m_pSource->Lock( dwOffset, dwBytes, ppFirst, pdwFirstBytes, ppSecond, pdwSecondBytes, dwFlags );
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Play( DWORD, DWORD, DWORD dwFlags )
{
	m_pSource->Play( ( dwFlags & DSBPLAY_LOOPING ) ? 1 : 0 );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetCurrentPosition( DWORD dwPosition )
{
	m_pSource->SetWavePosition( dwPosition );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetFormat( LPCWAVEFORMATEX )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetVolume( LONG lVolume )
{
	m_pSource->SetGain( (D3DVALUE)pow( 10.0, (double)lVolume * 0.0005f ) );
	m_pA3d->Flush();
	m_pA3d->Clear();
	m_lVolume = lVolume;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetPan( LONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetFrequency( DWORD dwFrequency )
{
	WAVEFORMATEX format;

	m_pSource->GetWaveFormat( &format );
	m_pSource->SetPitch( (D3DVALUE)( (double)dwFrequency / (LONG)format.nSamplesPerSec ) );
	m_pA3d->Flush();
	m_pA3d->Clear();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Stop()
{
	m_pSource->Stop();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Unlock( LPVOID pFirst, DWORD dwFirstBytes,
		LPVOID pSecond, DWORD dwSecondBytes )
{
	return m_pSource->Unlock( pFirst, dwFirstBytes, pSecond, dwSecondBytes );
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::Restore()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetAllParameters( LPDS3DBUFFER )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetConeAngles( LPDWORD, LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetConeOrientation( LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetConeOutsideVolume( LPLONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetMaxDistance( D3DVALUE* pflDistance )
{
	D3DVALUE flMinimum;
	D3DVALUE flMaximum;
	DWORD dwMute;

	m_pSource->GetMinMaxDistance( &flMinimum, &flMaximum, &dwMute );
	*pflDistance = flMaximum;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetMinDistance( D3DVALUE* pflDistance )
{
	D3DVALUE flMinimum;
	D3DVALUE flMaximum;
	DWORD dwMute;

	m_pSource->GetMinMaxDistance( &flMinimum, &flMaximum, &dwMute );
	*pflDistance = flMinimum;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetMode( LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetPosition( LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::GetVelocity( LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetAllParameters( LPCDS3DBUFFER, DWORD dwApply )
{
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetConeAngles( DWORD, DWORD, DWORD dwApply )
{
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetConeOrientation( D3DVALUE flX, D3DVALUE flY,
		D3DVALUE flZ, DWORD dwApply )
{
	m_pSource->SetOrientationAngles3f( flX, flY, -flZ );
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetConeOutsideVolume( LONG, DWORD dwApply )
{
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetMaxDistance( D3DVALUE flDistance, DWORD dwApply )
{
	D3DVALUE flMinimum;
	D3DVALUE flMaximum;
	DWORD dwMute;

	m_pSource->GetMinMaxDistance( &flMinimum, &flMaximum, &dwMute );
	m_pSource->SetMinMaxDistance( flMinimum, flDistance, dwMute );
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetMinDistance( D3DVALUE flDistance, DWORD dwApply )
{
	D3DVALUE flMinimum;
	D3DVALUE flMaximum;
	DWORD dwMute;

	m_pSource->GetMinMaxDistance( &flMinimum, &flMaximum, &dwMute );
	m_pSource->SetMinMaxDistance( flDistance, flMaximum, dwMute );
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetMode( DWORD, DWORD dwApply )
{
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetPosition( D3DVALUE flX, D3DVALUE flY,
		D3DVALUE flZ, DWORD dwApply )
{
	m_pSource->SetPosition3f( flX, flY, -flZ );
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperSecBuffer::SetVelocity( D3DVALUE flX, D3DVALUE flY,
		D3DVALUE flZ, DWORD dwApply )
{
	m_pSource->SetVelocity3f( flX, flY, -flZ );
	if ( dwApply == DS3D_IMMEDIATE )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

#define A3D_OUTPUT_GAIN_MIN		0.0f
#define A3D_OUTPUT_GAIN_MAX		1.0f

HRESULT CA3d::DirectSoundGetCaps( LPDSCAPS pCaps )
{
	return m_pDirectSound->GetCaps( pCaps );
}

HRESULT CA3d::DirectSoundCompact()
{
	return m_pDirectSound->Compact();
}

HRESULT CA3d::DirectSoundSetSpeakerConfig( DWORD dwConfig )
{
	return m_pDirectSound->SetSpeakerConfig( dwConfig );
}

HRESULT CA3d::DirectSoundGetSpeakerConfig( LPDWORD pConfig )
{
	return m_pDirectSound->GetSpeakerConfig( pConfig );
}

HRESULT STDMETHODCALLTYPE CA3d::SetOutputMode( DWORD dwFront, DWORD dwRear, DWORD dwMode )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->SetOutputMode( dwFront, dwRear, dwMode );
	if ( m_pA3d != NULL )
		return m_pA3d->SetOutputMode( dwFront, dwRear, dwMode );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::GetOutputMode( LPDWORD pFront, LPDWORD pRear, LPDWORD pMode )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->GetOutputMode( pFront, pRear, pMode );
	if ( m_pA3d != NULL )
		return m_pA3d->GetOutputMode( pFront, pRear, pMode );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::SetResourceManagerMode( DWORD dwMode )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->SetResourceManagerMode( dwMode );
	if ( m_pA3d != NULL )
		return m_pA3d->SetResourceManagerMode( dwMode );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::GetResourceManagerMode( LPDWORD pMode )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->GetResourceManagerMode( pMode );
	if ( m_pA3d != NULL )
		return m_pA3d->GetResourceManagerMode( pMode );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::SetHFAbsorbFactor( FLOAT flFactor )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->SetHFAbsorbFactor( flFactor );
	if ( m_pA3d != NULL )
		return m_pA3d->SetHFAbsorbFactor( flFactor );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::GetHFAbsorbFactor( FLOAT* pFactor )
{
	if ( m_pA3d2 != NULL )
		return m_pA3d2->GetHFAbsorbFactor( pFactor );
	if ( m_pA3d != NULL )
		return m_pA3d->GetHFAbsorbFactor( pFactor );

	return E_FAIL;
}

HRESULT STDMETHODCALLTYPE CA3d::RegisterVersion( DWORD dwVersion )
{
	if ( m_pA3d2 == NULL )
		return E_FAIL;

	return m_pA3d2->RegisterVersion( dwVersion );
}

HRESULT STDMETHODCALLTYPE CA3d::GetSoftwareCaps( LPA3DCAPS_SOFTWARE pCaps )
{
	if ( m_pA3d2 == NULL )
		return E_FAIL;

	return m_pA3d2->GetSoftwareCaps( pCaps );
}

HRESULT STDMETHODCALLTYPE CA3d::GetHardwareCaps( LPA3DCAPS_HARDWARE pCaps )
{
	if ( m_pA3d2 == NULL )
		return E_FAIL;

	return m_pA3d2->GetHardwareCaps( pCaps );
}

HRESULT STDMETHODCALLTYPE CA3d::SetCooperativeLevel( HWND hWnd, DWORD dwLevel )
{
	HRESULT status = m_pDirectSound->SetCooperativeLevel( hWnd, dwLevel );
	if ( FAILED( status ) )
		return A3DERROR_FAILED_SETCOOPERATIVE_LEVEL;

	m_dwCooperativeLevel = dwLevel;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::GetCooperativeLevel( LPDWORD pLevel )
{
	if ( pLevel == NULL )
		return E_INVALIDARG;

	*pLevel = m_dwCooperativeLevel;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::SetMaxReflectionDelayTime( A3DVAL flTime )
{
	m_flMaxReflectionDelayTime = flTime;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::SetOutputGain( A3DVAL flGain )
{
	if ( !( flGain >= A3D_OUTPUT_GAIN_MIN && flGain <= A3D_OUTPUT_GAIN_MAX ) )
		return E_INVALIDARG;

	m_flOutputGain = flGain;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3d::GetOutputGain( LPA3DVAL pGain )
{
	if ( pGain == NULL )
		return E_INVALIDARG;

	memcpy( pGain, &m_flOutputGain, sizeof( *pGain ) );
	return S_OK;
}

CA3dMapper::CA3dMapper( CA3d* pA3d )
{
	m_pA3d = pA3d;
	m_pA3d->AddRef();
	m_pPrimaryBuffer = NULL;

	m_cReferences = 0;
}

CA3dMapper::~CA3dMapper()
{
	m_pA3d->Release();
}

HRESULT STDMETHODCALLTYPE CA3dMapper::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) )
		*ppObject = (IDirectSound*)this;
	else if ( !memcmp( &iid, &IID_IA3d, sizeof( IID ) ) || !memcmp( &iid, &IID_IA3d2, sizeof( IID ) ) )
		*ppObject = (IA3d2*)this;
	else if ( !memcmp( &iid, &IID_IDirectSound, sizeof( IID ) ) )
		*ppObject = (IDirectSound*)this;
	else
	{
		*ppObject = NULL;
		return E_NOINTERFACE;
	}

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dMapper::AddRef()
{
	InterlockedIncrement( &m_cReferences );
	return m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dMapper::Release()
{
	InterlockedDecrement( &m_cReferences );
	ULONG cReferences = m_cReferences;
	if ( !cReferences )
	{
		delete this;
		return 0;
	}

	return cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dMapper::CreateSoundBuffer( LPCDSBUFFERDESC pDesc,
		LPDIRECTSOUNDBUFFER* ppBuffer, IUnknown* )
{
	*ppBuffer = NULL;
	if ( !m_pPrimaryBuffer )
	{
		m_pPrimaryBuffer = new( std::nothrow ) CA3dMapperPrimBuffer( m_pA3d );
		m_pPrimaryBuffer->AddRef();
	}

	if ( pDesc->dwFlags & DSBCAPS_PRIMARYBUFFER )
	{
		m_pPrimaryBuffer->QueryInterface( IID_IDirectSoundBuffer, (void**)ppBuffer );
		return S_OK;
	}

	IA3dSource* pSource;
	HRESULT hr = m_pA3d->NewSource( 0, &pSource );
	if ( hr )
		return hr;

	*ppBuffer = new( std::nothrow ) CA3dMapperSecBuffer( pSource, m_pPrimaryBuffer, m_pA3d );
	( *ppBuffer )->AddRef();

	WAVEFORMATEX format;
	memcpy( &format, pDesc->lpwfxFormat, sizeof( format ) );
	pSource->SetWaveFormat( &format );
	pSource->AllocateWaveData( (INT)pDesc->dwBufferBytes );
	pSource->Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetCaps( LPDSCAPS pCaps )
{
	return m_pA3d->DirectSoundGetCaps( pCaps );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::DuplicateSoundBuffer( LPDIRECTSOUNDBUFFER pBuffer,
		LPDIRECTSOUNDBUFFER* ppDuplicate )
{
	*ppDuplicate = NULL;
	IA3dSource* pSource = ( (CA3dMapperSecBuffer*)pBuffer )->m_pSource;
	if ( !pSource )
		return E_FAIL;

	IA3dSource* pDuplicate;
	HRESULT hr = m_pA3d->DuplicateSource( pSource, &pDuplicate );
	if ( hr )
		return hr;

	*ppDuplicate = new( std::nothrow ) CA3dMapperSecBuffer( pDuplicate, m_pPrimaryBuffer, m_pA3d );
	( *ppDuplicate )->AddRef();
	pDuplicate->Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapper::SetCooperativeLevel( HWND hWindow, DWORD dwLevel )
{
	return m_pA3d->SetCooperativeLevel( hWindow, dwLevel );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::Compact()
{
	return m_pA3d->DirectSoundCompact();
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetSpeakerConfig( LPDWORD pdwConfig )
{
	return m_pA3d->DirectSoundGetSpeakerConfig( pdwConfig );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::SetSpeakerConfig( DWORD dwConfig )
{
	return m_pA3d->DirectSoundSetSpeakerConfig( dwConfig );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::Initialize( LPCGUID pDevice )
{
	return m_pA3d->Init( (LPGUID)pDevice, 0, 0 );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::SetOutputMode( DWORD dwFront, DWORD dwBack, DWORD dwQuad )
{
	return m_pA3d->SetOutputMode( dwFront, dwBack, dwQuad );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetOutputMode( LPDWORD pdwFront, LPDWORD pdwBack, LPDWORD pdwQuad )
{
	return m_pA3d->GetOutputMode( pdwFront, pdwBack, pdwQuad );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::SetResourceManagerMode( DWORD dwMode )
{
	return m_pA3d->SetResourceManagerMode( dwMode );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetResourceManagerMode( LPDWORD pdwMode )
{
	return m_pA3d->GetResourceManagerMode( pdwMode );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::SetHFAbsorbFactor( FLOAT flFactor )
{
	return m_pA3d->SetHFAbsorbFactor( flFactor );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetHFAbsorbFactor( FLOAT* pflFactor )
{
	return m_pA3d->GetHFAbsorbFactor( pflFactor );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::RegisterVersion( DWORD dwVersion )
{
	return m_pA3d->RegisterVersion( dwVersion );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetSoftwareCaps( LPA3DCAPS_SOFTWARE pCaps )
{
	return m_pA3d->GetSoftwareCaps( pCaps );
}

HRESULT STDMETHODCALLTYPE CA3dMapper::GetHardwareCaps( LPA3DCAPS_HARDWARE pCaps )
{
	return m_pA3d->GetHardwareCaps( pCaps );
}

CA3dMapperPrimBuffer::CA3dMapperPrimBuffer( CA3d* pA3d )
{
	m_pA3d = pA3d;
	m_pA3d->AddRef();
	m_flDistanceFactor = 1.0f;
	m_flDopplerFactor = 1.0f;
	m_flRolloffFactor = 1.0f;
	m_pA3d->QueryInterface( IID_IA3dListener, (void**)&m_pListener );
	m_pListener->AddRef();

	m_cReferences = 0;
}

CA3dMapperPrimBuffer::~CA3dMapperPrimBuffer()
{
	m_pListener->Release();
	m_pA3d->Release();
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::QueryInterface( REFIID iid, void** ppObject )
{
	if ( !ppObject )
		return E_INVALIDARG;

	*ppObject = NULL;
	if ( !memcmp( &iid, &IID_IUnknown, sizeof( IID ) ) )
		*ppObject = (IDirectSoundBuffer*)this;
	else if ( !memcmp( &iid, &IID_IDirectSoundBuffer, sizeof( IID ) ) )
		*ppObject = (IDirectSoundBuffer*)this;
	else if ( !memcmp( &iid, &IID_IDirectSound3DListener, sizeof( IID ) ) )
		*ppObject = (IDirectSound3DListener*)this;

	if ( !*ppObject )
		return E_FAIL;

	( (IUnknown*)*ppObject )->AddRef();
	return S_OK;
}

ULONG STDMETHODCALLTYPE CA3dMapperPrimBuffer::AddRef()
{
	InterlockedIncrement( &m_cReferences );
	return m_cReferences;
}

ULONG STDMETHODCALLTYPE CA3dMapperPrimBuffer::Release()
{
	InterlockedDecrement( &m_cReferences );
	ULONG cReferences = m_cReferences;
	if ( !cReferences )
	{
		delete this;
		return 0;
	}

	return cReferences;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetCaps( LPDSBCAPS )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetCurrentPosition( LPDWORD, LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetFormat( LPWAVEFORMATEX, DWORD, LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetVolume( LPLONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetPan( LPLONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetFrequency( LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetStatus( LPDWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Initialize( LPDIRECTSOUND, LPCDSBUFFERDESC )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Lock( DWORD, DWORD, LPVOID*, LPDWORD,
		LPVOID*, LPDWORD, DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Play( DWORD, DWORD, DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetCurrentPosition( DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetFormat( LPCWAVEFORMATEX )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetVolume( LONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetPan( LONG )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetFrequency( DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Stop()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Unlock( LPVOID, DWORD, LPVOID, DWORD )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::Restore()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetAllParameters( LPDS3DLISTENER )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetDistanceFactor( D3DVALUE* pflFactor )
{
	memcpy( pflFactor, &m_flDistanceFactor, sizeof( *pflFactor ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetDopplerFactor( D3DVALUE* pflFactor )
{
	memcpy( pflFactor, &m_flDopplerFactor, sizeof( *pflFactor ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetOrientation( LPD3DVECTOR, LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetPosition( LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetRolloffFactor( D3DVALUE* pflFactor )
{
	memcpy( pflFactor, &m_flRolloffFactor, sizeof( *pflFactor ) );
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::GetVelocity( LPD3DVECTOR )
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetAllParameters( LPCDS3DLISTENER, DWORD dwApply )
{
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetDistanceFactor( D3DVALUE flFactor, DWORD )
{
	m_flDistanceFactor = flFactor;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetDopplerFactor( D3DVALUE flFactor, DWORD dwApply )
{
	m_flDopplerFactor = flFactor;
	m_pA3d->SetSourceDopplerScale( flFactor );
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetOrientation( D3DVALUE flFrontX, D3DVALUE flFrontY,
		D3DVALUE flFrontZ, D3DVALUE flTopX, D3DVALUE flTopY, D3DVALUE flTopZ, DWORD dwApply )
{
	m_pA3d->SetOrientation6f( flFrontX, flFrontY, -flFrontZ, flTopX, flTopY, -flTopZ );
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetPosition( D3DVALUE flX, D3DVALUE flY,
		D3DVALUE flZ, DWORD dwApply )
{
	m_pA3d->SetPosition3f( flX, flY, -flZ );
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetRolloffFactor( D3DVALUE flFactor, DWORD dwApply )
{
	m_flRolloffFactor = flFactor;
	m_pA3d->SetSourceDistanceModelScale( flFactor );
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::SetVelocity( D3DVALUE flX, D3DVALUE flY,
		D3DVALUE flZ, DWORD dwApply )
{
	m_pA3d->SetVelocity3f( flX, flY, -flZ );
	if ( !dwApply )
	{
		m_pA3d->Flush();
		m_pA3d->Clear();
	}

	return S_OK;
}

HRESULT STDMETHODCALLTYPE CA3dMapperPrimBuffer::CommitDeferredSettings()
{
	m_pA3d->Flush();
	m_pA3d->Clear();
	return S_OK;
}

CA3dMapperSecBuffer::CA3dMapperSecBuffer( IA3dSource* pSource,
		CA3dMapperPrimBuffer* pPrimary, CA3d* pA3d )
{
	m_cReferences = 0;
	m_pSource = pSource;
	m_pSource->AddRef();
	m_pA3d = pA3d;
	m_pA3d->AddRef();
	m_pSource->SetDopplerScale( pPrimary->m_flDopplerFactor );
	m_pSource->SetDistanceModelScale( pPrimary->m_flRolloffFactor );

	m_lVolume = 0;
}

CA3dMapperSecBuffer::~CA3dMapperSecBuffer()
{
	m_pSource->Release();
	m_pA3d->Release();
}
