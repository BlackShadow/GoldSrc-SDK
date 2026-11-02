#ifndef DIRECTORCMD_H
#define DIRECTORCMD_H

#include "HLTV/common/Bitbuffer.h"
#include "common/hltv.h"

class DirectorCmd
{
public:
	DirectorCmd();
	virtual ~DirectorCmd();

	bool GetEventData( int& entity1, int& entity2, int& flags );
	bool GetModeData( int& mode );
	bool GetChaseData( int& entity1, int& entity2, float& distance, int& flags );
	bool GetInEyeData( int& player );
	bool GetMapData( int& entity, float& angle, float& distance );
	bool GetCameraData( float* position, float* angles, float& fov, int& entity );
	bool GetCamPathData( float* position, float* angles, float& fov, int& flags );
	bool GetSoundData( char* name, float& volume );

	float GetTime();
	int GetType();
	char* GetName();

	bool GetTimeScaleData( float& factor );
	bool GetWayPointsData( int& number );
	bool GetMessageData( int& effect, int& color, float* position, float& fadein, float& fadeout,
		float& holdtime, float& fxtime, char* text );
	bool GetStatusData( int& slots, int& spectators, int& proxies );
	bool GetBannerData( char* filename );
	bool GetStuffTextData( char* commands );

	void SetEventData( int entity1, int entity2, int flags );
	void SetChaseData( int entity1, int entity2, float distance, int flags );
	void SetInEyeData( int entity );
	void SetMapData( int entity, float angle, float distance );
	void SetStartData();
	void SetModeData( int mode );
	void SetCameraData( float* position, float* angles, float fov, int entity );
	void SetCamPathData( float* position, float* angles, float fov, int flags );
	void SetSoundData( char* name, float volume );
	void SetTimeScaleData( float factor );

	void SetTime( float time );

	void SetMessageData( int effect, unsigned int color, float* position, float fadein, float fadeout,
		float holdtime, float fxtime, char* text );
	void Copy( DirectorCmd* cmd );
	void SetStatusData( int slots, int spectators, int proxies );
	void SetBannerData( char* filename );
	void SetStuffTextData( char* commands );
	void SetWayPoints( int number );

	bool ReadFromStream( BitBuffer* stream );
	void WriteToStream( BitBuffer* stream );

	char* ToString();
	void FromString( char* string );

	void Clear();
	void Resize( int size );

	float		m_Time;
	int			m_Type;
	int			m_Size;
	BitBuffer	m_Data;
	int			m_Index;
};

#endif
