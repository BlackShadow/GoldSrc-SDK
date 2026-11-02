#include "ivoicerecord.h"
#include "CircularBuffer.h"
#include "conprint.h"

#include <Windows.h>
#include <mmsystem.h>
#include <cassert>
#include <string.h>

#define NUM_WAVEIN_BUFFERS 15
#define WAVEIN_BUFFER_SIZE 256

#define WAVE_RECORD_HEADER_DONE 1

static void ReportWaveInputError( const char* pMessage, MMRESULT mmr )
{
	char errorText[MAX_PATH];

	Con_DPrintf( "%s", pMessage );
	waveInGetErrorTextA( mmr, errorText, sizeof( errorText ) );
	Con_Printf( "%s\n", errorText );
}

class VoiceRecord_WaveIn : public IVoiceRecord
{
public:
	VoiceRecord_WaveIn();
	~VoiceRecord_WaveIn() override;
	void Release() override;
	bool RecordStart() override;
	void RecordStop() override;
	void Idle() override;
	int GetRecordedData( short* pOut, int nSamples ) override;
	bool Init( DWORD samplesPerSecond );

private:
	static void CALLBACK WaveInputCallback(
		HWAVEIN hWaveIn, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2 );
	void CloseDevice();

	WAVEHDR m_Headers[NUM_WAVEIN_BUFFERS];
	char m_CaptureData[NUM_WAVEIN_BUFFERS][WAVEIN_BUFFER_SIZE];
	CRITICAL_SECTION m_CriticalSection;
	HWAVEIN m_Device;
	bool m_Recording;
	CSizedCircularBuffer<NUM_WAVEIN_BUFFERS * WAVEIN_BUFFER_SIZE> m_RecordedData;
};

#ifdef _MSC_VER
#pragma warning( suppress: 26495 )
#endif
VoiceRecord_WaveIn::VoiceRecord_WaveIn()
{
	m_Device = NULL;
	InitializeCriticalSection( &m_CriticalSection );
	m_Recording = false;
}

VoiceRecord_WaveIn::~VoiceRecord_WaveIn()
{
	CloseDevice();
	DeleteCriticalSection( &m_CriticalSection );
}

void VoiceRecord_WaveIn::Release()
{
	delete this;
}

void VoiceRecord_WaveIn::CloseDevice()
{
	if ( !m_Device )
		return;

	RecordStop();
	waveInReset( m_Device );
	waveInClose( m_Device );
	m_Device = NULL;
}

bool VoiceRecord_WaveIn::RecordStart()
{
	m_Recording = true;
	Idle();
	return true;
}

void VoiceRecord_WaveIn::RecordStop()
{
	m_Recording = false;
}

void VoiceRecord_WaveIn::Idle()
{
	for ( int i = 0; i < NUM_WAVEIN_BUFFERS; i++ )
	{
		WAVEHDR* pHeader = &m_Headers[i];
		if ( !( pHeader->dwUser & WAVE_RECORD_HEADER_DONE ) || !m_Recording )
			continue;

		memset( m_CaptureData[i], 0, WAVEIN_BUFFER_SIZE );
		pHeader->dwUser = 0;

		MMRESULT mmr = waveInAddBuffer( m_Device, pHeader, sizeof( *pHeader ) );
		if ( mmr != MMSYSERR_NOERROR )
			ReportWaveInputError( "ERROR VoiceRecord_WaveIn::RecordAddBuffer: waveInAddBuffer\n", mmr );
	}

	if ( !m_Recording )
	{
		EnterCriticalSection( &m_CriticalSection );
		m_RecordedData.Flush();
		LeaveCriticalSection( &m_CriticalSection );
	}
}

int VoiceRecord_WaveIn::GetRecordedData( short* pOut, int nSamples )
{
	int nMaxBytes = nSamples * 2;

	EnterCriticalSection( &m_CriticalSection );

	int nAvailable = m_RecordedData.GetReadAvailable();
	if ( nAvailable > nMaxBytes )
		m_RecordedData.Advance( nAvailable - nMaxBytes );

	int nBytesRead = m_RecordedData.Read( pOut, nMaxBytes );

	LeaveCriticalSection( &m_CriticalSection );

	Idle();
	return nBytesRead / 2;
}

bool VoiceRecord_WaveIn::Init( DWORD samplesPerSecond )
{
	WAVEFORMATEX format;
	format.wFormatTag = WAVE_FORMAT_PCM;
	format.nChannels = 1;
	format.nSamplesPerSec = samplesPerSecond;
	format.nAvgBytesPerSec = samplesPerSecond * 2;
	format.nBlockAlign = 2;
	format.wBitsPerSample = 16;
	format.cbSize = sizeof( format );

	MMRESULT mmr = waveInOpen(
		&m_Device, WAVE_MAPPER, &format, (DWORD_PTR)WaveInputCallback, (DWORD_PTR)this, CALLBACK_FUNCTION );
	if ( mmr != MMSYSERR_NOERROR )
	{
		ReportWaveInputError( "ERROR VoiceRecord_WaveIn: waveInOpen\n", mmr );
		CloseDevice();
		return false;
	}

	for ( int i = 0; i < NUM_WAVEIN_BUFFERS; i++ )
	{
		WAVEHDR* pHeader = &m_Headers[i];
		pHeader->dwBytesRecorded = 0;
		pHeader->dwFlags = 0;
		pHeader->dwLoops = 0;
		pHeader->lpNext = NULL;
		pHeader->reserved = 0;
		pHeader->lpData = m_CaptureData[i];
		pHeader->dwBufferLength = WAVEIN_BUFFER_SIZE;
		pHeader->dwUser = WAVE_RECORD_HEADER_DONE;

		mmr = waveInPrepareHeader( m_Device, pHeader, sizeof( *pHeader ) );
		if ( mmr != MMSYSERR_NOERROR )
		{
			ReportWaveInputError( "ERROR VoiceRecord_WaveIn::Init: waveInPrepareHeader\n", mmr );
			CloseDevice();
			return false;
		}
	}

	mmr = waveInStart( m_Device );
	if ( mmr != MMSYSERR_NOERROR )
	{
		ReportWaveInputError( "ERROR Voice_RecordStart: waveInStart\n", mmr );
		CloseDevice();
		return false;
	}

	return true;
}

void CALLBACK VoiceRecord_WaveIn::WaveInputCallback(
	HWAVEIN, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR )
{
	if ( uMsg != WIM_DATA )
		return;

	WAVEHDR* pHeader = (WAVEHDR*)dwParam1;

	if ( !( pHeader->dwFlags & WHDR_DONE ) )
		_wassert( L"pHeader->dwFlags & WHDR_DONE", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\voice_record_wavein.cpp", 313 );
	if ( pHeader->dwFlags & ~( WHDR_DONE | WHDR_PREPARED ) )
		_wassert( L"!(pHeader->dwFlags & ~(WHDR_DONE|WHDR_PREPARED))", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\voice_record_wavein.cpp", 314 );
	if ( pHeader->dwUser & WAVE_RECORD_HEADER_DONE )
		_wassert( L"!(pHeader->dwUser & WAVE_RECORD_HEADER_DONE)", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\voice_record_wavein.cpp", 315 );

	VoiceRecord_WaveIn* pRecord = (VoiceRecord_WaveIn*)dwInstance;
	if ( pRecord->m_Recording && pHeader->dwBytesRecorded )
	{
		EnterCriticalSection( &pRecord->m_CriticalSection );
		pRecord->m_RecordedData.Write( pHeader->lpData, pHeader->dwBytesRecorded );
		LeaveCriticalSection( &pRecord->m_CriticalSection );
	}

	pHeader->dwUser |= WAVE_RECORD_HEADER_DONE;
}

IVoiceRecord* CreateVoiceRecord_WaveIn( int sampleRate )
{
	VoiceRecord_WaveIn* pRecord = new VoiceRecord_WaveIn;
	if ( !pRecord )
		return NULL;

	if ( pRecord->Init( sampleRate ) )
		return pRecord;

	pRecord->Release();
	return NULL;
}
