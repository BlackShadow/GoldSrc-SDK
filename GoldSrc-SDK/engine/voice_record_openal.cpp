#include "ivoicerecord.h"
#include <stddef.h>

struct ALCdevice;

extern "C"
{
	ALCdevice* alcCaptureOpenDevice( const char* deviceName, unsigned int frequency, int format, int bufferSize );
	int alcCaptureCloseDevice( ALCdevice* device );
	void alcCaptureStart( ALCdevice* device );
	void alcCaptureStop( ALCdevice* device );
	void alcCaptureSamples( ALCdevice* device, void* buffer, int samples );
	void alcGetIntegerv( ALCdevice* device, int param, int size, int* values );
	int alGetError();
}

#define AL_NO_ERROR 0
#define AL_FORMAT_MONO16 0x1101
#define ALC_CAPTURE_SAMPLES 0x312

class VoiceRecord_OpenAL : public IVoiceRecord
{
public:
	VoiceRecord_OpenAL();
	~VoiceRecord_OpenAL() override;
	void Release() override;
	bool RecordStart() override;
	void RecordStop() override;
	void Idle() override;
	int GetRecordedData( short* pOut, int nSamples ) override;
	virtual bool Init( int sampleRate );

private:
	bool InitalizeInterfaces();
	void ReleaseInterfaces();
	void ClearInterfaces();

	ALCdevice* m_Device;
	int m_nSampleRate;
};

VoiceRecord_OpenAL::VoiceRecord_OpenAL()
{
	m_Device = NULL;
	m_nSampleRate = 0;
}

VoiceRecord_OpenAL::~VoiceRecord_OpenAL()
{
	alcCaptureCloseDevice( m_Device );
}

void VoiceRecord_OpenAL::Release()
{
	delete this;
}

bool VoiceRecord_OpenAL::RecordStart()
{
	if ( !m_Device )
	{
		m_Device = alcCaptureOpenDevice( NULL, m_nSampleRate, AL_FORMAT_MONO16, m_nSampleRate * 20 );
		if ( !m_Device )
			return false;

		alGetError();
	}

	alcCaptureStart( m_Device );
	return alGetError() == AL_NO_ERROR;
}

void VoiceRecord_OpenAL::RecordStop()
{
	if ( m_Device )
		alcCaptureStop( m_Device );

	ReleaseInterfaces();
}

void VoiceRecord_OpenAL::Idle()
{
}

int VoiceRecord_OpenAL::GetRecordedData( short* pOut, int nSamples )
{
	int nAvailable = 0;
	alcGetIntegerv( m_Device, ALC_CAPTURE_SAMPLES, 1, &nAvailable );
	if ( nAvailable <= 0 )
		return 0;

	if ( nAvailable > nSamples )
		nAvailable = nSamples;

	alcCaptureSamples( m_Device, pOut, nAvailable );
	if ( alGetError() != AL_NO_ERROR )
		return 0;

	return nAvailable;
}

bool VoiceRecord_OpenAL::Init( int sampleRate )
{
	m_nSampleRate = sampleRate;
	ReleaseInterfaces();
	return true;
}

bool VoiceRecord_OpenAL::InitalizeInterfaces()
{
	m_Device = alcCaptureOpenDevice( NULL, m_nSampleRate, AL_FORMAT_MONO16, m_nSampleRate * 20 );
	return m_Device != NULL && alGetError() == AL_NO_ERROR;
}

void VoiceRecord_OpenAL::ReleaseInterfaces()
{
	alcCaptureCloseDevice( m_Device );
	ClearInterfaces();
}

void VoiceRecord_OpenAL::ClearInterfaces()
{
	m_Device = NULL;
}

IVoiceRecord* CreateVoiceRecord( int sampleRate )
{
	VoiceRecord_OpenAL* pRecord = new VoiceRecord_OpenAL;
	if ( !pRecord )
		return NULL;

	pRecord->Init( sampleRate );
	return pRecord;
}
