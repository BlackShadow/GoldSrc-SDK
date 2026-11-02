#ifndef IPROXY_H
#define IPROXY_H

#include "common/ISystemModule.h"

class BitBuffer;
class NetAddress;
class IWorld;
class IServer;
class IDirector;
class INetSocket;
class IObjectContainer;
struct resource_s;

enum
{
	GROUP_CLIENT = 1,
	GROUP_PROXY = 2,
	GROUP_DEMO = 4,
	GROUP_UNUSED = 8,
	GROUP_VOICE = 16,
	GROUP_CHAT = 32
};

class IProxy : public virtual ISystemModule
{
public:
	virtual ~IProxy() override
	{
	}

	virtual void Reset() = 0;
	virtual void Broadcast( unsigned char* data, int length, int group, bool reliable ) = 0;
	virtual void IncreaseCheering( int amount ) = 0;

	virtual void ParseStatusMsg( BitBuffer* stream ) = 0;
	virtual void ParseStatusReport( NetAddress* from, BitBuffer* stream ) = 0;
	virtual bool ProcessConnectionlessMessage( NetAddress* from, BitBuffer* stream ) = 0;

	virtual void ChatCommentator( char* name, char* text ) = 0;
	virtual void ChatSpectator( char* name, char* text ) = 0;
	virtual void CountLocalClients( int& proxies, int& spectators ) = 0;
	virtual resource_s* AddResource( char* filename, int type, char* name ) = 0;

	virtual bool IsLanOnly() = 0;
	virtual bool IsMaster() = 0;
	virtual bool IsActive() = 0;
	virtual bool IsPublicGame() = 0;
	virtual bool IsPasswordProtected() = 0;
	virtual bool IsStressed() = 0;

	virtual void SetDelay( float delay ) = 0;
	virtual void SetClientTime( double time, bool relative ) = 0;
	virtual void SetClientTimeScale( float scale ) = 0;
	virtual void SetMaxRate( int rate ) = 0;
	virtual void SetMaxLoss( float loss ) = 0;
	virtual void SetMaxUpdateRate( int rate ) = 0;
	virtual bool SetMaxClients( int clients ) = 0;
	virtual void SetRegion( unsigned char region ) = 0;

	virtual float GetDelay() = 0;
	virtual double GetSpectatorTime() = 0;
	virtual double GetProxyTime() = 0;
	virtual int GetMaxClients() = 0;

	virtual IWorld* GetWorld() = 0;
	virtual IServer* GetServer() = 0;
	virtual IDirector* GetDirector() = 0;
	virtual INetSocket* GetSocket() = 0;

	virtual int GetChatMode() = 0;
	virtual void GetStatistics( int& proxies, int& slots, int& spectators ) = 0;
	virtual int GetMaxRate() = 0;
	virtual int GetMaxUpdateRate() = 0;
	virtual resource_s* GetResource( char* filename ) = 0;
	virtual int GetDispatchMode() = 0;
	virtual unsigned char GetRegion() = 0;
	virtual IObjectContainer* GetClients() = 0;

	virtual bool WriteSignonData( int type, BitBuffer* stream ) = 0;
};

#endif
