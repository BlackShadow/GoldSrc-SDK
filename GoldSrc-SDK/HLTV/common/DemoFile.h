#pragma once

#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/IServer.h"
#include "HLTV/common/IWorld.h"
#include "HLTV/common/NetChannel.h"
#include "engine/cl_demo.h"

typedef void* FileHandle_t;

class IBaseSystem;
class IFileSystem;

class DemoFile
{
public:
	DemoFile();
	virtual ~DemoFile();

	void Reset();
	bool IsRecording();
	void CloseFile();

	void WriteDemoInfo( demo_info_t* demoInfo );
	void WriteSequenceInfo();
	void WriteDemoStartup( BitBuffer* buffer );
	void WriteDemoMessage( BitBuffer* unreliableData, BitBuffer* reliableData );
	void WriteUpdateClientData( client_data_t* cdata );
	float GetDemoTime();
	void WriteSignonData();
	bool StartRecording( char* newName );

	bool IsPlaying();
	bool LoadDemo( char* demoname );
	void StopPlayBack();
	void ReadDemoPacket( BitBuffer* demoData, demo_info_t* demoInfo );
	void ReadDemoInfo( demo_info_t* demoInfo );
	void ReadSequenceInfo();
	char* GetFileName();

	bool IsContinuous();
	void SetContinuous( bool enabled );
	void Init( IWorld* world, IServer* server, NetChannel* channel );

	serverinfo_t	serverInfo;
#ifdef _WIN32
	char			fileName[260];
#else
	char			fileName[4096];
#endif
	int				state;
	unsigned int	frameCount;
	FileHandle_t	m_FileHandle;

	demoheader_t	demoheader;
	demoentry_t		loadEntry;
	demoentry_t		gameEntry;
	demo_info_t		zeroDemoInfo;

	float			startTime;
	float			nextReadTime;
	NetChannel*		demoChannel;
	int				startPos;
	int				entryNumber;
	int				currentEntry;
	demoentry_t*	entries;
	bool			continuous;

	IBaseSystem*	m_System;
	IWorld*			m_World;
	IServer*		m_Server;
	IFileSystem*	m_FileSystem;
};
