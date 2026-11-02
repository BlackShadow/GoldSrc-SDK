#include "MusicManager.h"
#include "GameUI_Interface.h"

#include "vgui/KeyValues.h"
#include "controls.h"
#include "wrect.h"
#include "cl_dll.h"

#include <stdio.h>
#include <string.h>

MusicManager* TheMusicManager;

EXPOSE_SINGLE_INTERFACE( MusicManager, IMusicManager, "MusicManager001" );

MusicManager::MusicManager()
{
	m_currentTrack = TRACKTYPE_INVALID;
	for ( int i = 0; i < TRACKTYPE_NUM; i++ )
	{
		m_tracks[i].t_path[0] = 0;
		m_tracks[i].looping = false;
	}
	m_useLoopingInfo = false;
}

MusicManager::~MusicManager()
{
}

void MusicManager::Reset()
{
	if ( m_currentTrack != TRACKTYPE_INVALID )
	{
		StopTrack();
		m_currentTrack = TRACKTYPE_INVALID;
	}
}

void MusicManager::Init()
{
	Reset();

	KeyValues* playlist = new KeyValues( "Playlist" );
	if ( playlist->LoadFromFile( vgui2::filesystem(), "playlist.txt" ) )
	{
		if ( playlist->GetInt( "Version", 1 ) > 1 )
			m_useLoopingInfo = true;

		strncpy( m_tracks[TRACKTYPE_SPLASH].t_path, playlist->GetString( "Splash" ),
			sizeof( m_tracks[TRACKTYPE_SPLASH].t_path ) );
		m_tracks[TRACKTYPE_SPLASH].looping = playlist->GetInt( "SplashLoop", 1 ) != 0;

		strncpy( m_tracks[TRACKTYPE_ROUNDWIN].t_path, playlist->GetString( "RoundWin" ),
			sizeof( m_tracks[TRACKTYPE_ROUNDWIN].t_path ) );
		m_tracks[TRACKTYPE_ROUNDWIN].looping = playlist->GetInt( "RoundWinLoop", 0 ) != 0;

		strncpy( m_tracks[TRACKTYPE_ROUNDLOSE].t_path, playlist->GetString( "RoundLose" ),
			sizeof( m_tracks[TRACKTYPE_ROUNDLOSE].t_path ) );
		m_tracks[TRACKTYPE_ROUNDLOSE].looping = playlist->GetInt( "RoundLoseLoop", 0 ) != 0;

		strncpy( m_tracks[TRACKTYPE_MATCHWIN].t_path, playlist->GetString( "MatchWin" ),
			sizeof( m_tracks[TRACKTYPE_MATCHWIN].t_path ) );
		m_tracks[TRACKTYPE_MATCHWIN].looping = playlist->GetInt( "MatchWinLoop", 0 ) != 0;

		strncpy( m_tracks[TRACKTYPE_MATCHLOSE].t_path, playlist->GetString( "MatchLose" ),
			sizeof( m_tracks[TRACKTYPE_MATCHLOSE].t_path ) );
		m_tracks[TRACKTYPE_MATCHLOSE].looping = playlist->GetInt( "MatchLoseLoop", 0 ) != 0;

		strncpy( m_tracks[TRACKTYPE_CREDITS].t_path, playlist->GetString( "Credits" ),
			sizeof( m_tracks[TRACKTYPE_CREDITS].t_path ) );
		m_tracks[TRACKTYPE_CREDITS].looping = playlist->GetInt( "CreditsLoop", 1 ) != 0;
	}

	playlist->deleteThis();
}

void MusicManager::StartTrack( TrackType track, bool looping )
{
	if ( m_currentTrack == track || track < TRACKTYPE_SPLASH || track >= TRACKTYPE_NUM )
		return;

	TrackInfo& info = m_tracks[track];
	if ( !info.t_path[0] )
		return;

	if ( m_useLoopingInfo )
		looping = info.looping;

	Career_Printf( "MusicManager::StartTrack - playing track %d %s\n", track, looping ? "as looping" : "" );
	engine->pfnPrimeMusicStream( info.t_path, looping );

	char command[sizeof( info.t_path )];
#ifdef _WIN32
	_snprintf( command, sizeof( command ), "cd %s %s\n", looping ? "loopfile" : "playfile", info.t_path );
#else
	snprintf( command, sizeof( command ), "cd %s %s\n", looping ? "loopfile" : "playfile", info.t_path );
#endif
	engine->pfnClientCmd( command );
	m_currentTrack = track;
}

void MusicManager::StopTrack()
{
	if ( m_currentTrack != TRACKTYPE_INVALID )
	{
		engine->pfnClientCmd( "cd stop" );
		m_currentTrack = TRACKTYPE_INVALID;
	}
}

void MusicManager::FadeOutTrack()
{
	if ( m_currentTrack != TRACKTYPE_INVALID )
	{
		engine->pfnClientCmd( "cd fadeout" );
		m_currentTrack = TRACKTYPE_INVALID;
	}
}

bool MusicManager::IsPlayingTrack()
{
	return m_currentTrack != TRACKTYPE_INVALID;
}

bool MusicManager::IsPlayingTrack( TrackType track )
{
	return m_currentTrack == track;
}

void CreateMusicManager()
{
	TheMusicManager = &__g_MusicManager_singleton;
	TheMusicManager->Init();
}

void ShutdownMusicManager()
{
	TheMusicManager = NULL;
}
