#ifndef ICLIENT_H
#define ICLIENT_H

#include "common/ISystemModule.h"

class INetSocket;
class NetAddress;
class IWorld;
class InfoString;

class IClient : public virtual ISystemModule
{
public:
	virtual ~IClient() override
	{
	}

	virtual bool Connect( INetSocket* socket, NetAddress* address, char* userinfo ) = 0;
	virtual void Send( unsigned char* data, int length, bool reliable ) = 0;
	virtual void Disconnect( char* reason ) = 0;
	virtual void Reconnect() = 0;
	virtual void SetWorld( IWorld* world ) = 0;

	virtual int GetClientType() = 0;
	virtual char* GetClientName() = 0;
	virtual InfoString* GetUserInfo() = 0;
	virtual NetAddress* GetAddress() = 0;
	virtual bool IsActive() = 0;
	virtual bool IsHearingVoices() = 0;
	virtual bool HasChatEnabled() = 0;
};

#endif
