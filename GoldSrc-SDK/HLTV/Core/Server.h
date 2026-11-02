#ifndef SERVER_H
#define SERVER_H

#include "HLTV/common/IServer.h"
#include "common/BaseSystemModule.h"
#include "HLTV/common/NetChannel.h"
#include "HLTV/common/NetAddress.h"
#include "HLTV/common/InfoString.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/DemoFile.h"

class IFileSystem;

class Server : public IServer, public BaseSystemModule
{
public:
	enum ServerState
	{
		SERVER_UNDEFINED			= 0,
		SERVER_INITILAZING		= 1,
		SERVER_DISCONNECTED		= 2,
		SERVER_CHALLENGING		= 3,
		SERVER_AUTHENTICATING	= 4,
		SERVER_CONNECTING		= 5,
		SERVER_CONNECTED		= 6,
		SERVER_RUNNING			= 7,
		SERVER_INTERMISSION		= 8
	};

	typedef void ( Server::*ParseFunction )();

	typedef struct svc_func_s
	{
		unsigned char	opcode;
		char*			pszname;
		ParseFunction	func;
	} svc_func_t;

	Server();
	virtual ~Server() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual void ReceiveSignal( ISystemModule* module, unsigned int signal, void* data ) override
	{
		BaseSystemModule::ReceiveSignal( module, signal, data );
	}
	virtual void ExecuteCommand( int commandID, char* commandLine ) override;
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

	virtual bool Connect( IWorld* world, NetAddress* address, INetSocket* socket ) override;
	virtual bool LoadDemo( IWorld* world, char* filename, bool forceHLTV, bool continuous ) override;
	virtual void Reconnect() override;
	virtual void Disconnect() override;
	virtual void Retry() override;
	virtual void StopRetry() override;
	virtual void SendStringCommand( char* command ) override;
	virtual void SendHLTVCommand( BitBuffer* msg ) override;

	virtual bool IsConnected() override;
	virtual bool IsDemoFile() override;
	virtual bool IsGameServer() override;
	virtual bool IsRelayProxy() override;
	virtual bool IsVoiceBlocking() override;

	virtual void SetProxy( IProxy* proxy ) override;
	virtual void SetDirector( IDirector* director ) override;
	virtual void SetPlayerName( char* newName ) override;
	virtual void SetDelayReconnect( bool state ) override;
	virtual void SetAutoRetry( bool state ) override;
	virtual void SetVoiceBlocking( bool state ) override;
	virtual void SetRate( int rate ) override;
	virtual void SetUpdateRate( int updaterate ) override;
	virtual void SetUserInfo( char* key, char* value ) override;
	virtual bool SetProtocol( int version ) override;
	virtual void SetGameDirectory( const char* defaultDir, const char* gameDir ) override;

	virtual int GetRate() override;
	virtual int GetUpdateRate() override;
	virtual InfoString* GetServerInfoString() override;
	virtual char* GetPlayerName() override;
	virtual float GetTime() override;
	virtual IWorld* GetWorld() override;
	virtual char* GetDemoFileName() override;
	virtual NetAddress* GetAddress() override;
	virtual char* GetHostName() override;
	virtual bool GetAutoRetry() override;
	virtual float GetPacketLoss() override;
	virtual int GetProtocol() override;

	void ParseBad();
	void ParseDeltaDescription();
	void ParseBaseline();
	void ParseChoke();
	void ParsePrint();
	void ParseSendCvarValue();
	void ParseCDTrack();
	void ParseSendExtraInfo();
	void ParseSoundFade();
	void ParseVoiceInit();
	void ParseDecalName();
	void ParseFinale();
	void ParseCutscene();
	void ParseCenterPrint();
	void ParseDeltaPacketEntities();
	void ParseStopSound();
	void ParseRoomType();
	void ParseVoiceData();
	void ParseWeaponAnim();
	void ParseSetAngle();
	void ParseAddAngle();
	void ParseSpawnStaticSound();
	void ParseParticle();
	void ParseVersion();
	void ParseSendCvarValue2();
	void ParseNewUserMsg();
	void ParseUpdateUserInfo();
	void ParseRestore();
	void ParseTime();
	void ParseCrosshairAngle();
	void ParsePings();
	void ParseTempEntity();
	void ParseCustomization();
	void ParseSound();
	void ParseEvent();
	void ParseEventReliable();
	void ParseResourceLocation();
	void ParseMoveVars();
	void ParseTimeScale();
	void ParseClientData();
	void ParseResourceRequest();
	void ParseHLTV();
	void ParseSetView();
	void ParsePacketEntities();
	void ParseLightStyle();
	void ParseFileTransferFailed();
	void ParseNop();
	void ParseIntermission();

	void ClearFrame( bool completely );
	void CheckAutoRetry();
	void ScheduleAutoRetry();
	char* GetCmdName( int cmd );
	void ProcessEntityUpdate();
	void ParseSetPause();
	void ParseInfo( BitBuffer* stream, bool detailed );
	void Reset();

	void AcceptBadPassword();
	void AcceptRejection( char* reason );
	void AcceptRedirect( char* toAddress );
	void Challenge();
	void SendConnectPacket();
	void AcceptChallenge( char* cmdLine );
	void SetState( int newState );
	void SendServerCommands();
	void ParseDirector();
	void SendUserVar( char* key, char* value );
	void ParseDisconnect();
	void AcceptConnection();
	bool ProcessConnectionlessMessage( NetAddress* from, BitBuffer* stream );
	void ParseStuffText();
	void ParseSignonNum();
	void ParseResourceList();
	bool ParseUserMessage( int cmd );
	void ProcessMessage( unsigned int seqNr );
	void ParseServerinfo();
	void CheckConnection();

	IWorld*				m_World = NULL;
	IProxy*				m_Proxy = NULL;
	IDirector*			m_Director = NULL;
	IFileSystem*		m_FileSystem = NULL;
	int					m_ChallengeNumber = 0;
	INetSocket*			serverSocket = NULL;
	NetChannel			serverChannel;
	NetAddress			serverAddress;
	int					state = 0;
#ifdef _WIN32
	char				m_HostName[260]{};
#else
	char				m_HostName[4096]{};
#endif
	int					currentRetry = 0;
	InfoString			userinfo;
	char				cdkey[32]{};
	int					m_AuthProtocol = 0;

	frame_t				frame{};
	unsigned char		entityBuffer[348160]{};
	int					m_Rate = 0;
	int					m_UpdateRate = 0;
	float				m_Time = 0.0f;
	BitBuffer			reliableData;
	BitBuffer			unreliableData;
	BitBuffer			voiceData;
	BitBuffer			userMessages;
	BitBuffer			clientData;
	BitBuffer*			instream = NULL;
	int					validsequence = 0;

	bool				m_AutoRetry = false;
	double				m_NextAutoRetry = 0.0;
	bool				m_IsHLTV = false;
	bool				m_ForceHLTV = false;
	bool				isGameServer = false;
	bool				isPaused = false;
	bool				isVoiceBlocking = false;

	DemoFile			demoFile;
	demo_info_t			demoInfo{};
	BitBuffer			demoData;
	CRC32_t				m_ServerCRC{};
	int					m_ServerCount = 0;
	int					m_ClientPlayerNum = 0;
	int					m_Protocol = 0;
	clientdata_t		m_ClientDataStruct{};
	InfoString			serverinfo;
	bool				m_DelayReconnect = false;
	unsigned int		SeqNrMap[256]{};

	static const svc_func_t PARSEFUNCS[60];
};

#endif // SERVER_H
