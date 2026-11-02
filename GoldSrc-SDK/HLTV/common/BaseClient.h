#ifndef BASECLIENT_H
#define BASECLIENT_H

#include "HLTV/common/IClient.h"
#include "common/BaseSystemModule.h"
#include "HLTV/common/NetChannel.h"
#include "HLTV/common/InfoString.h"
#include "game_shared/voice_common.h"

struct frame_s;
class NetPacket;

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4250 )
#endif

class BaseClient : public IClient, public BaseSystemModule
{
public:
	enum ClientState
	{
		CLIENT_UNDEFINED		= 0,
		CLIENT_INITILAZING	= 1,
		CLIENT_CONNECTING	= 2,
		CLIENT_RUNNING		= 3,
		CLIENT_DISCONNECTED	= 4
	};

	typedef void ( BaseClient::*ParseFunction )( NetPacket* packet );

	struct clc_func_t
	{
		unsigned char	opcode;
		char*			pszname;
		ParseFunction	func;
	};

	BaseClient();
	virtual ~BaseClient() override;

	virtual bool Init( IBaseSystem* system, int serial, char* name ) override;
	virtual void RunFrame( double time ) override;
	virtual char* GetStatusLine() override;
	virtual char* GetType() override;
	virtual void ShutDown() override;

	virtual bool Connect( INetSocket* socket, NetAddress* adr, char* userinfo ) override;
	virtual void Send( unsigned char* data, int length, bool isReliable ) override;
	virtual void Disconnect( char* reason ) override;
	virtual void Reconnect() override;
	virtual void SetWorld( IWorld* world ) override;
	virtual int GetClientType() override;
	virtual char* GetClientName() override;
	virtual InfoString* GetUserInfo() override;
	virtual NetAddress* GetAddress() override;
	virtual bool IsActive() override;
	virtual bool IsHearingVoices() override;
	virtual bool HasChatEnabled() override;

	virtual void DownloadFailed( char* fileName );
	virtual void DownloadFile( char* fileName );
	virtual void UpdateVoiceMask( BitBuffer* stream );
	virtual void QueryVoiceEnabled( BitBuffer* stream );
	virtual void SetName( char* newName );
	virtual void WriteSpawn( BitBuffer* stream );
	virtual void WriteDatagram( double time, frame_s* frame );
	virtual void SendDatagram();
	virtual void Reset();
	virtual void SetState( ClientState newState );
	virtual void ReplyNew();
	virtual void ReplySpawn( int spawncount, int crcMap );
	virtual void ReplyFullUpdate();
	virtual void PrintfToClient( char* fmt, ... );
	virtual void UpdateUserInfo( char* userinfostring );
	virtual void ParseStringCmd( NetPacket* packet );
	virtual void ParseNop( NetPacket* packet );
	virtual void ParseBad( NetPacket* packet );
	virtual void ParseMove( NetPacket* packet );
	virtual void ParseVoiceData( NetPacket* packet );
	virtual void ParseHLTV( NetPacket* packet );
	virtual void ParseDelta( NetPacket* packet );
	virtual void ParseCvarValue( NetPacket* packet );
	virtual void ParseCvarValue2( NetPacket* packet );
	virtual void ProcessMessage( NetPacket* packet );
	virtual bool ProcessStringCmd( char* string );

protected:
	IWorld*			m_World;
	INetSocket*		m_Socket;
	NetChannel		clientChannel;
	int				type;
	int				state;
	char			name[32];
	InfoString		userinfo;
	unsigned int	lastFrameSeqNr;
	unsigned int	deltaFrameSeqNr;
	unsigned int	clientDelta;
	unsigned int	SeqNrMap[256];
	int				crcValue;
	bool			voiceEnabled;
	bool			voiceQuery;
	CPlayerBitVec	sentGameRulesMask;
	CPlayerBitVec	sentBanMask;
	CPlayerBitVec	BanMask;

	static const clc_func_t PARSEFUNCS[];
};

#ifdef _MSC_VER
#pragma warning( pop )
#endif

#endif
