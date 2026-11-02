#pragma once

#include "GameUI/IMusicManager.h"

#include <stdlib.h>
#ifndef _WIN32
#include <limits.h>
#endif

class MusicManager : public IMusicManager
{
public:
	MusicManager();
	~MusicManager() override;

	void StartTrack( TrackType track, bool looping ) override;
	void StopTrack() override;
	void FadeOutTrack() override;
	bool IsPlayingTrack() override;
	bool IsPlayingTrack( TrackType track ) override;

	void Reset();
	void Init();

private:
	struct TrackInfo
	{
#ifdef _WIN32
		char	t_path[_MAX_PATH];
#else
		char	t_path[PATH_MAX];
#endif
		bool	looping;
	};

	TrackType	m_currentTrack;
	TrackInfo	m_tracks[TRACKTYPE_NUM];
	bool		m_useLoopingInfo;
};

extern MusicManager* TheMusicManager;

void	CreateMusicManager();
void	ShutdownMusicManager();
