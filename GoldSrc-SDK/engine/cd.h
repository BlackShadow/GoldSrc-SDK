#pragma once

#define CD_REMAP_TRACKS 100

class ICDAudio
{
public:
	virtual int Init() = 0;
	virtual void Shutdown() = 0;

	virtual void Play( int track, bool looping ) = 0;
	virtual void PlayFile( const char* filename, bool looping ) = 0;
	virtual void Pause() = 0;
	virtual void Resume() = 0;

	virtual void Frame() = 0;

	virtual void MP3_Resume_Audio() = 0;
	virtual void MP3_Suspend_Audio() = 0;
};

class CCDAudio : public ICDAudio
{
public:
	static const float TRACK_EXTRA_TIME;

	struct _MP3_AUDIO_STATE
	{
		bool inuse;
		bool suspended;
		bool playing;
		char trackname[512];
		int tracknum;
		bool looping;
		float volume;
	};

	CCDAudio();
	virtual ~CCDAudio();

	virtual int Init();
	virtual void Shutdown();
	virtual void Play( int track, bool looping );
	virtual void PlayFile( const char* filename, bool looping );
	virtual void Pause();
	virtual void Resume();
	virtual void Frame();
	virtual void MP3_Resume_Audio();
	virtual void MP3_Suspend_Audio();

	void ResetCDTimes();
	void Reset();
	void Stop();
	void Eject();
	void CloseDoor();
	void GetAudioDiskInfo();
	void PrimeTrack( char* filename, int looping );
	void SwitchToLauncher();
	void SwitchToEngine();
	void FadeOut();
	void CD_f();

	bool MP3_Init();
	void MP3_Shutdown();
	void MP3_ReleaseDriver();
	float MP3_SetVolume( float newVolume );
	bool MP3_InitStream( int trackNum, bool looping );
	bool MP3_InitStream( const char* filename, bool looping );
	void MP3_PlayTrackFinalize( int trackNum, bool looping );
	bool MP3_PlayTrack( const char* filename, bool looping );
	bool MP3_PlayTrack( int trackNum, bool looping );
	void MP3_StartStream();
	void MP3_StopStream();
	void MP3_Loop();
	void MP3_SetPause( bool onOff );

	void _Init( int, int );
	void _Play( int track, int looping );
	void _Pause( int, int );
	void _Resume( int, int );
	void _Stop( int, int );
	void _Eject( int, int );
	void _CloseDoor( int, int );
	void _GetAudioDiskInfo( int, int );
	void _SwitchToLauncher( int, int );
	void _SwitchToEngine( int, int );
	void _CDReset( int, int );
	void _CDUpdate( int, int );
	void _PrimeTrack( int track, int looping );

	void FreeAllocatedTracks();

	float m_flPlayTime;
	double m_dStartTime;
	double m_dPauseTime;
	double m_dFadeOutTime;

	bool m_bIsCDValid;
	bool m_bIsPlaying;
	bool m_bWasPlaying;
	bool m_bInitialized;
	bool m_bEnabled;
	bool m_bIsLooping;
	bool m_bIsPrimed;
	bool m_bIsInMiddleOfPriming;

	float m_flVolume;
	float m_flMP3Volume;

	int m_nPlayTrack;
	char m_szPendingPlayFilename[512];
	int m_nMaxCDTrack;
	bool m_bResumeOnSwitch;
	unsigned int m_uiDeviceID;
	int m_rgRemapCD[CD_REMAP_TRACKS];

	_MP3_AUDIO_STATE m_MP3;
};

extern ICDAudio* cdaudio;

CCDAudio*	GetInteralCDAudio();

void		CDAudio_Init();
void		CDAudio_Shutdown();
void		CDAudio_Play( int track, int looping );
void		CDAudio_Pause();
void		CDAudio_Resume();
void		MP3_Suspend_Audio();
void		PrimeMusicStream( char* filename, int looping );
