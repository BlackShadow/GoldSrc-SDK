#include "ivoicerecord.h"
#include "filesystem.h"

#include <windows.h>
#include <mmsystem.h>
#include <dsound.h>

#include <cassert>
#include <string.h>
#include <new>

typedef HRESULT( WINAPI* DirectSoundCaptureCreateFn )( LPCGUID, LPDIRECTSOUNDCAPTURE*, LPUNKNOWN );

// ------------------------------------------------------------------------------
// Static helpers
// ------------------------------------------------------------------------------



// ------------------------------------------------------------------------------
// VoiceRecord_DSound
// ------------------------------------------------------------------------------

class VoiceRecord_DSound : public IVoiceRecord
{
public:
	VoiceRecord_DSound();
	~VoiceRecord_DSound() override;
	void Release() override;
	bool RecordStart() override;
	void RecordStop() override;
	void Idle() override;
	int GetRecordedData( short* pOut, int nSamples ) override;
	virtual bool Init( int sampleRate );

private:
	void Term(); // Delete members.

	HMODULE m_hInstDS;
	IDirectSoundCapture* m_pCapture;
	IDirectSoundCaptureBuffer* m_pCaptureBuffer;
	// How many bytes our capture buffer has.
	DWORD m_nCaptureBufferBytes;
	// We need to know when the capture buffer loops, so we install an event and 
	// update this in the event.
	DWORD m_WrapOffset;
	HANDLE m_hWrapEvent;
	DWORD m_LastReadPos;
};

#ifdef _MSC_VER
#pragma warning( suppress: 26495 )
#endif
VoiceRecord_DSound::VoiceRecord_DSound()
{
	m_pCapture = NULL;
	m_pCaptureBuffer = NULL;
	m_WrapOffset = 0;
	m_LastReadPos = 0;
	m_hWrapEvent = NULL;
	m_hInstDS = NULL;
}

VoiceRecord_DSound::~VoiceRecord_DSound()
{
	Term();
}

void VoiceRecord_DSound::Term()
{
	if ( m_pCaptureBuffer )
		m_pCaptureBuffer->Release();

	if ( m_pCapture )
		m_pCapture->Release();

	if ( m_hWrapEvent )
		DeleteObject( (HGDIOBJ)m_hWrapEvent );

	if ( m_hInstDS )
		FreeLibrary( m_hInstDS );

	m_pCapture = NULL;
	m_pCaptureBuffer = NULL;
	m_WrapOffset = 0;
	m_LastReadPos = 0;
	m_hWrapEvent = NULL;
	m_hInstDS = NULL;
}

void VoiceRecord_DSound::Release()
{
	delete this;
}

bool VoiceRecord_DSound::RecordStart()
{
	return true;
}

void VoiceRecord_DSound::RecordStop()
{
}

void VoiceRecord_DSound::Idle()
{
	if ( m_pCaptureBuffer && WaitForSingleObject( m_hWrapEvent, 0 ) == WAIT_OBJECT_0 )
		m_WrapOffset += m_nCaptureBufferBytes;
}

int VoiceRecord_DSound::GetRecordedData( short* pOut, int nSamples )
{
	if ( !m_pCaptureBuffer )
		_wassert( L"false", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\voice_record_dsound.cpp", 250 );

	DWORD dwStatus;
	if ( FAILED( m_pCaptureBuffer->GetStatus( &dwStatus ) ) || !( dwStatus & DSCBSTATUS_CAPTURING ) )
		return 0;

	Idle(); // Update wrapping..

	DWORD dwCapturePos, dwReadPos;
	if ( FAILED( m_pCaptureBuffer->GetCurrentPosition( &dwCapturePos, &dwReadPos ) ) )
		return 0;

	dwCapturePos += m_WrapOffset;
	dwReadPos += m_WrapOffset;

	DWORD nMaxBytes = nSamples;
	DWORD dwStartPos = nMaxBytes < dwReadPos ? dwReadPos - nMaxBytes : 0;
	if ( dwStartPos < m_LastReadPos )
		dwStartPos = m_LastReadPos;

	void *pData1, *pData2;
	DWORD dwData1Bytes, dwData2Bytes;
	if ( FAILED( m_pCaptureBuffer->Lock( dwStartPos % m_nCaptureBufferBytes, dwReadPos - dwStartPos, &pData1,
			 &dwData1Bytes, &pData2, &dwData2Bytes, 0 ) ) )
		return 0;

	if ( dwData1Bytes + dwData2Bytes > nMaxBytes )
		_wassert( L"false", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\voice_record_dsound.cpp", 301 );

	memcpy( pOut, pData1, dwData1Bytes );
	memcpy( pOut + ( dwData1Bytes >> 1 ), pData2, dwData2Bytes );

	m_pCaptureBuffer->Unlock( pData1, dwData1Bytes, pData2, dwData2Bytes );
	m_LastReadPos = dwReadPos;

	return ( dwData1Bytes + dwData2Bytes ) >> 1;
}

bool VoiceRecord_DSound::Init( int sampleRate )
{
	DSCBUFFERDESC dscDesc = {};
	DSCBCAPS caps = {};
	DSBPOSITIONNOTIFY dsbNotify;
	IDirectSoundNotify* pNotify;
	DirectSoundCaptureCreateFn createFn;
	HRESULT hr;

	Term();

	WAVEFORMATEX recordFormat;
	recordFormat.wFormatTag = WAVE_FORMAT_PCM;
	recordFormat.nChannels = 1;
	recordFormat.nSamplesPerSec = sampleRate;
	recordFormat.nAvgBytesPerSec = (DWORD)sampleRate * 2;
	recordFormat.nBlockAlign = 2;
	recordFormat.wBitsPerSample = 16;
	recordFormat.cbSize = sizeof( WAVEFORMATEX );

	// Load the DSound DLL.
	m_hInstDS = (HMODULE)FS_LoadLibrary( "dsound.dll" );
	if ( !m_hInstDS )
		goto HandleError;

	createFn = (DirectSoundCaptureCreateFn)GetProcAddress( m_hInstDS, "DirectSoundCaptureCreate" );
	if ( !createFn )
		goto HandleError;

	if ( FAILED( createFn( NULL, &m_pCapture, NULL ) ) )
		goto HandleError;

	// Create the capture buffer.
	dscDesc.dwSize = sizeof( dscDesc );
	dscDesc.dwBufferBytes = recordFormat.nAvgBytesPerSec;
	dscDesc.lpwfxFormat = &recordFormat;
	if ( FAILED( m_pCapture->CreateCaptureBuffer( &dscDesc, &m_pCaptureBuffer, NULL ) ) )
		goto HandleError;

	// Figure out how many bytes we got in our capture buffer.
	caps.dwSize = sizeof( caps );
	if ( FAILED( m_pCaptureBuffer->GetCaps( &caps ) ) )
		goto HandleError;

	m_nCaptureBufferBytes = caps.dwBufferBytes;

	// Set it up so we get notification when the buffer wraps.
	m_hWrapEvent = CreateEventA( NULL, FALSE, FALSE, NULL );
	if ( !m_hWrapEvent )
		goto HandleError;

	dsbNotify.dwOffset = dscDesc.dwBufferBytes - 1;
	dsbNotify.hEventNotify = m_hWrapEvent;

	if ( FAILED( m_pCaptureBuffer->QueryInterface( IID_IDirectSoundNotify, (void**)&pNotify ) ) )
		goto HandleError;

	hr = pNotify->SetNotificationPositions( 1, &dsbNotify );
	pNotify->Release();
	if ( FAILED( hr ) )
		goto HandleError;

	// Start capturing.
	return SUCCEEDED( m_pCaptureBuffer->Start( DSCBSTART_LOOPING ) );

HandleError:
	Term();
	return false;
}

IVoiceRecord* CreateVoiceRecord_DSound( int sampleRate )
{
	VoiceRecord_DSound* pRecord = new VoiceRecord_DSound;
	if ( !pRecord )
		return NULL;

	if ( pRecord->Init( sampleRate ) )
		return pRecord;

	pRecord->Release();
	return NULL;
}
