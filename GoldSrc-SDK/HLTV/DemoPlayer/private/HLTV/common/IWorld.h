#ifndef IWORLD_H
#define IWORLD_H

class IBaseSystem;
class ISystemModule;
class IBSPModel;
class IDirector;
class NetAddress;
class InfoString;
class BitBuffer;
struct _UserMsg;
struct serverinfo_s;
struct resource_s;
struct movevars_s;
struct clientdata_s;
struct entity_state_s;

struct frame_s
{
	float			time;
	unsigned int	seqnr;
	unsigned char*	data;

	void*			entities;
	unsigned int	entitiesSize;
	unsigned int	entitynum;

	void*			clientData;
	unsigned int	clientDataSize;

	unsigned char*	events;
	unsigned int	eventsSize;
	unsigned int	eventnum;

	unsigned char*	reliableData;
	unsigned int	reliableDataSize;
	unsigned char*	unreliableData;
	unsigned int	unreliableDataSize;

	unsigned char*	userMessages;
	unsigned int	userMessagesSize;
	unsigned char*	voiceData;
	unsigned int	voiceDataSize;

	unsigned char*	demoData;
	unsigned int	demoDataSize;
	void*			demoInfo;
	unsigned int	delta;
};

typedef frame_s frame_t;

class IWorld
{
public:
	virtual ~IWorld() = default;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) = 0;
	virtual void RunFrame( double time ) = 0;

	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) = 0;
	virtual void ExecuteCommand( int commandID, char* commandLine ) = 0;
	virtual void RegisterListener( ISystemModule* module ) = 0;
	virtual void RemoveListener( ISystemModule* module ) = 0;

	virtual IBaseSystem* GetSystem() = 0;
	virtual int GetSerial() = 0;
	virtual char* GetStatusLine() = 0;
	virtual char* GetType() = 0;
	virtual char* GetName() = 0;
	virtual int GetState() = 0;
	virtual int GetVersion() = 0;

	virtual void ShutDown() = 0;

	virtual double GetTime() = 0;
	virtual NetAddress* GetGameServerAddress() = 0;
	virtual char* GetLevelName() = 0;
	virtual char* GetGameDir() = 0;

	virtual frame_t* GetFrameByTime( double time ) = 0;
	virtual frame_t* GetFrameBySeqNr( unsigned int sequence ) = 0;
	virtual frame_t* GetLastFrame() = 0;
	virtual frame_t* GetFirstFrame() = 0;

	virtual int GetServerCount() = 0;
	virtual int GetSlotNumber() = 0;
	virtual int GetMaxClients() = 0;
	virtual int GetNumPlayers() = 0;
	virtual IBSPModel* GetWorldModel() = 0;
	virtual InfoString* GetServerInfoString() = 0;
	virtual bool GetPlayerInfoString( int player, InfoString* info ) = 0;
	virtual _UserMsg* GetUserMsg( int message ) = 0;
	virtual char* GetHostName() = 0;
	virtual serverinfo_s* GetServerInfo() = 0;

	virtual bool IsPlayerIndex( int index ) = 0;
	virtual bool IsVoiceEnabled() = 0;
	virtual bool IsActive() = 0;
	virtual bool IsPaused() = 0;
	virtual bool IsComplete() = 0;
	virtual bool IsHLTV() = 0;

	virtual void Reset() = 0;
	virtual void SetServerInfo( int protocol, unsigned int crc, unsigned char* hash, int maxClients, int slot,
		int gameType, char* gameDirectory, char* hostName, char* levelName ) = 0;
	virtual void SetServerInfoString( char* info ) = 0;
	virtual void SetServerInfo( serverinfo_s* info ) = 0;
	virtual void UpdateServerInfo() = 0;

	virtual void SetPaused( bool paused ) = 0;
	virtual void SetTime( double time ) = 0;
	virtual void SetBufferSize( float seconds ) = 0;
	virtual void SetVoiceEnabled( bool enabled ) = 0;
	virtual void SetMoveVars( movevars_s* movevars ) = 0;
	virtual void SetCDInfo( int track, int loopTrack ) = 0;
	virtual void SetHLTV( bool enabled ) = 0;
	virtual void SetExtraInfo( char* clientFallback, int allowCheats ) = 0;
	virtual void SetViewEntity( int entity ) = 0;
	virtual void SetGameServerAddress( NetAddress* address ) = 0;
	virtual void SetHostName( char* name ) = 0;

	virtual void NewGame( int serverCount ) = 0;
	virtual void FinishGame() = 0;
	virtual bool SaveAsDemo( char* filename, IDirector* director ) = 0;
	virtual void StopGame() = 0;

	virtual int FindUserMsgByName( char* name ) = 0;
	virtual void ParseDeltaDescription( BitBuffer* stream ) = 0;
	virtual void ParseBaseline( BitBuffer* stream ) = 0;
	virtual void ParseEvent( BitBuffer* stream ) = 0;
	virtual void ParseClientData( BitBuffer* stream, unsigned int delta, BitBuffer* output, clientdata_s* data ) = 0;
	virtual bool GetUncompressedFrame( unsigned int sequence, frame_t* frame ) = 0;
	virtual bool UncompressEntitiesFromStream( frame_t* frame, BitBuffer* stream ) = 0;
	virtual bool UncompressEntitiesFromStream( frame_t* frame, BitBuffer* stream, unsigned int delta ) = 0;
	virtual bool GetClientData( unsigned int sequence, clientdata_s* data ) = 0;
	virtual bool GetClientData( frame_t* frame, clientdata_s* data ) = 0;

	virtual int AddFrame( frame_t* frame ) = 0;
	virtual bool AddResource( resource_s* resource ) = 0;
	virtual void AddLightStyle( int style, char* value ) = 0;
	virtual bool AddSignonData( unsigned char type, unsigned char* data, int size ) = 0;
	virtual bool AddUserMessage( int type, int size, char* name ) = 0;
	virtual void AddBaselineEntity( int index, entity_state_s* entity ) = 0;
	virtual void AddInstancedBaselineEntity( int index, entity_state_s* entity ) = 0;
	virtual void UpdatePlayer( int player, int userID, char* info, char* hash ) = 0;

	virtual void WriteFrame( frame_t* frame, unsigned int lastSequence, BitBuffer* reliable, BitBuffer* unreliable,
		unsigned int deltaSequence, unsigned int outgoingSequence, bool addVoice ) = 0;
	virtual void WriteNewData( BitBuffer* stream ) = 0;
	virtual void WriteClientUpdate( BitBuffer* stream, int client ) = 0;
	virtual void WriteMovevars( BitBuffer* stream ) = 0;
	virtual void WriteSigonData( BitBuffer* stream ) = 0;
	virtual void WriteLightStyles( BitBuffer* stream ) = 0;

	virtual int RemoveFrames( unsigned int start, unsigned int end ) = 0;
	virtual int DuplicateFrames( unsigned int start, unsigned int end ) = 0;
	virtual int MoveFrames( unsigned int start, unsigned int end, double time ) = 0;
	virtual int RevertFrames( unsigned int start, unsigned int end ) = 0;
};

#endif
