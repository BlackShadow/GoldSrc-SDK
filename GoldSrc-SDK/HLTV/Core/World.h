#pragma once

#include "HLTV/common/IWorld.h"
#include "common/BaseSystemModule.h"
#include "common/entity_state.h"
#include "common/ObjectDictionary.h"
#include "pm_shared/pm_movevars.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/InfoString.h"
#include "HLTV/common/NetAddress.h"
#include "HLTV/common/common.h"
#include "HLTV/Core/BSPModel.h"

#define MAX_WORLD_CLIENTS			32
#define MAX_WORLD_ENTITIES			1380
#define MAX_WORLD_LIGHTSTYLES		64
#define WORLD_LIGHTSTYLE_LENGTH		64
#define WORLD_ENTITYBUFFER_BYTES	348160

class World : public IWorld, public BaseSystemModule
{
public:
	enum WorldState
	{
		WORLD_UNDEFINED		= 0,
		WORLD_INITILAZING	= 1,
		WORLD_DISCONNECTED	= 2,
		WORLD_CONNECTING	= 3,
		WORLD_RUNNING		= 4,
		WORLD_COMPLETE		= 5
	};

	struct player_info_s
	{
		int			userid;
		char		userinfo[256];
		char		name[32];
		int			spectator;
		int			ping;
		int			packet_loss;
		char		model[260];
		int			topcolor;
		int			bottomcolor;
		bool		active;
		char		hashedcdkey[16];
		int			trackerID;
	};

	typedef player_info_s player_info_t;

	struct deltaCache_s
	{
		unsigned int	seqNr;
		unsigned int	deltaNr;
		BitBuffer		buffer;
	};

	typedef deltaCache_s deltaCache_t;

	struct frameCache_s
	{
		unsigned int	seqNr;
		entity_state_t	entities[MAX_PACKET_ENTITIES];
	};

	typedef frameCache_s frameCache_t;

	World();
	virtual ~World() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override
	{
		BaseSystemModule::RunFrame( time );
	}
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override
	{
		BaseSystemModule::ReceiveSignal( module, signal, data );
	}
	virtual void ExecuteCommand( int commandID, char* commandLine ) override
	{
		BaseSystemModule::ExecuteCommand( commandID, commandLine );
	}
	virtual void RegisterListener( ISystemModule* module ) override
	{
		BaseSystemModule::RegisterListener( module );
	}
	virtual void RemoveListener( ISystemModule* module ) override
	{
		BaseSystemModule::RemoveListener( module );
	}
	virtual IBaseSystem* GetSystem() override
	{
		return BaseSystemModule::GetSystem();
	}
	virtual int GetSerial() override
	{
		return BaseSystemModule::GetSerial();
	}
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual char* GetName() override
	{
		return BaseSystemModule::GetName();
	}
	virtual int GetState() override
	{
		return BaseSystemModule::GetState();
	}
	virtual int GetVersion() override
	{
		return BaseSystemModule::GetVersion();
	}
	virtual void ShutDown() override;

	virtual double GetTime() override;
	virtual NetAddress* GetGameServerAddress() override;
	virtual char* GetLevelName() override;
	virtual char* GetGameDir() override;

	virtual frame_t* GetFrameByTime( double time ) override;
	virtual frame_t* GetFrameBySeqNr( unsigned int seqnr ) override;
	virtual frame_t* GetLastFrame() override;
	virtual frame_t* GetFirstFrame() override;

	virtual int GetServerCount() override;
	virtual int GetSlotNumber() override;
	virtual int GetMaxClients() override;
	virtual int GetNumPlayers() override;
	virtual IBSPModel* GetWorldModel() override;
	virtual InfoString* GetServerInfoString() override;
	virtual bool GetPlayerInfoString( int playerNum, InfoString* infoString ) override;
	virtual UserMsg* GetUserMsg( int msgNumber ) override;
	virtual char* GetHostName() override;
	virtual serverinfo_t* GetServerInfo() override;

	virtual bool IsPlayerIndex( int index ) override;
	virtual bool IsVoiceEnabled() override;
	virtual bool IsActive() override;
	virtual bool IsPaused() override;
	virtual bool IsComplete() override;
	virtual bool IsHLTV() override;

	virtual void Reset() override;
	virtual void SetServerInfo( int protocol, CRC32_t nserverCRC, unsigned char* nclientdllmd5,
			int nmaxclients, int nplayernum, int ngametype, char* ngamedir, char* nservername,
			char* nlevelname ) override;
	virtual void SetServerInfoString( char* infostring ) override;
	virtual void SetServerInfo( serverinfo_t* info ) override;
	virtual void UpdateServerInfo() override;
	virtual void SetPaused( bool state ) override;
	virtual void SetTime( double newTime ) override;
	virtual void SetBufferSize( float seconds ) override;
	virtual void SetVoiceEnabled( bool enabled ) override;
	virtual void SetMoveVars( movevars_t* nmovevars ) override;
	virtual void SetCDInfo( int ncdtrack, int nlooptrack ) override;
	virtual void SetHLTV( bool enabled ) override;
	virtual void SetExtraInfo( char* nclientfallback, int nallowCheats ) override;
	virtual void SetViewEntity( int nviewentity ) override;
	virtual void SetGameServerAddress( NetAddress* address ) override;
	virtual void SetHostName( char* name ) override;

	virtual void NewGame( int newServerCount ) override;
	virtual void FinishGame() override;
	virtual bool SaveAsDemo( char* filename, IDirector* director ) override;
	virtual void StopGame() override;

	virtual int FindUserMsgByName( char* name ) override;
	virtual void ParseDeltaDescription( BitBuffer* stream ) override;
	virtual void ParseBaseline( BitBuffer* stream ) override;
	virtual void ParseEvent( BitBuffer* stream ) override;
	virtual void ParseClientData( BitBuffer* stream, unsigned int deltaSeqNr, BitBuffer* to,
			clientdata_t* clientData ) override;

	virtual bool GetUncompressedFrame( unsigned int seqNr, frame_t* frame ) override;
	virtual bool UncompressEntitiesFromStream( frame_t* frame, BitBuffer* stream ) override;
	virtual bool UncompressEntitiesFromStream( frame_t* frame, BitBuffer* stream, unsigned int from ) override;
	virtual bool GetClientData( unsigned int SeqNr, clientdata_t* clientData ) override;
	virtual bool GetClientData( frame_t* frame, clientdata_t* clientData ) override;

	virtual int AddFrame( frame_t* newFrame ) override;
	virtual bool AddResource( resource_t* resource ) override;
	virtual void AddLightStyle( int index, char* style ) override;
	virtual bool AddSignonData( unsigned char type, unsigned char* data, int size ) override;
	virtual bool AddUserMessage( int msgNumber, int size, char* name ) override;
	virtual void AddBaselineEntity( int index, entity_state_t* ent ) override;
	virtual void AddInstancedBaselineEntity( int index, entity_state_t* ent ) override;
	virtual void UpdatePlayer( int playerNum, int userId, char* infostring, char* hashedcdkey ) override;

	virtual void WriteFrame( frame_t* frame, unsigned int lastFrameSeqnr, BitBuffer* reliableStream,
			BitBuffer* unreliableStream, unsigned int deltaSeqNr, unsigned int clientDelta, bool addVoice ) override;
	virtual void WriteNewData( BitBuffer* stream ) override;
	virtual void WriteClientUpdate( BitBuffer* stream, int playerIndex ) override;
	virtual void WriteMovevars( BitBuffer* stream ) override;
	virtual void WriteSigonData( BitBuffer* stream ) override;
	virtual void WriteLightStyles( BitBuffer* stream ) override;

	virtual int RemoveFrames( unsigned int startSeqNr, unsigned int endSeqNr ) override;
	virtual int DuplicateFrames( unsigned int startSeqNr, unsigned int endSeqNr ) override;
	virtual int MoveFrames( unsigned int startSeqNr, unsigned int endSeqNr, double destSeqnr ) override;
	virtual int RevertFrames( unsigned int startSeqNr, unsigned int endSeqNr ) override;

	bool SetMaxClients( int max );
	void ClearUserMessages();
	void ClearResources();
	void ClearBaseline();
	void ClearInstancedBaseline();
	void ClearLightStyles();
	void SetState( WorldState newState );
	void ConnectionComplete();
	void ClearPlayers();

	void WriteBaseline( BitBuffer* stream );
	void WriteDeltaDescriptions( BitBuffer* stream );
	void WriteServerinfo( BitBuffer* stream );
	void WriteRegisteredUserMessages( BitBuffer* stream );
	void WriteResources( BitBuffer* stream );

	int ParseDeltaHeader( BitBuffer* stream, bool* remove, bool* custom, int* numbase, bool* newbl,
			int* newblindex, bool full, int* offset );
	int FindBestBaseline( int index, entity_state_t** baseline, entity_state_t* to, int num, bool custom );
	int CompressFrame( frame_t* from, BitBuffer* stream );
	void ClearEntityCache();
	bool GetFrameFromCache( unsigned int seqNr, entity_state_t** entities );
	bool GetDeltaFromCache( unsigned int seqNr, unsigned int deltaNr, BitBuffer** buffer );
	void WritePacketEntities( BitBuffer* stream, frame_t* frame, frame_t* deltaframe );

	void ClearServerInfo();
	void SetName( char* newName );
	void ReorderFrameTimes( float newLastTime );
	bool GetUncompressedFrame( frame_t* deltaFrame, frame_t* frame );
	bool WriteDeltaEntities( BitBuffer* stream, frame_t* fullFrame, unsigned int deltaSeqNr, unsigned int clientDelta );
	void RearrangeFrame( frame_t* frame, int seqNrOffset, float timeOffset );
	float GetBufferedGameTime();
	void CheckFrameBufferSize();
	void ClearFrames();

	bool				m_IsPaused;
	int					state;
	int					m_Protocol;
	int					servercount;
	CRC32_t				serverCRC;
	unsigned char		clientdllmd5[16];
	int					maxclients;
	int					playernum;
	int					gametype;
#ifdef _WIN32
	char				gamedir[260];
#else
	char				gamedir[4096];
#endif
	char				levelname[40];
	char				servername[255];
	int					viewentity;
	serverinfo_t		detailedServerInfo;
	player_info_t		players[MAX_WORLD_CLIENTS];

	unsigned char		entityBuffer[WORLD_ENTITYBUFFER_BYTES];
	entity_state_t		baselines[MAX_WORLD_ENTITIES];
	int					max_baseline;
	entity_state_t		instanced_baselines[64];
	int					max_instanced_baseline;
	char				lightstyles[MAX_WORLD_LIGHTSTYLES][WORLD_LIGHTSTYLE_LENGTH + 1];
	movevars_t			movevars;
	BSPModel			worldModel;
	InfoString			serverinfo;
	UserMsg*			ClientUserMsgs;
	resource_t*			resourcesList;
	int					num_resources;
	BitBuffer			signonData;
	int					cdtrack;
	int					looptrack;

	ObjectDictionary	frames;
	ObjectDictionary	framesByTime;
	ObjectDictionary	camcommands;
	unsigned int		sequenceNr;
	float				maxBufferLength;
	int					maxCacheIndex;
	frameCache_t*		frameCache;
	deltaCache_t*		deltaCache;
	int					cacheHits;
	int					cacheFaults;
	double				worldTime;
	double				startTime;
	bool				voiceEnabled;
	char				clientfallback[260];
	bool				allowCheats;
	bool				isHLTV = false;
	char				hostName[255]{};
	NetAddress			gameServerAddress;
};
