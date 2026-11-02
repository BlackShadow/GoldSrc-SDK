#pragma once

#include "../public/interface.h"

typedef unsigned int ManageServerUIHandle_t;

class IManageServer;

class IAdminServer : public IBaseInterface
{
public:
	virtual ManageServerUIHandle_t OpenManageServerDialog( const char* serverName, const char* gameDir ) = 0;
	virtual ManageServerUIHandle_t OpenManageServerDialog( unsigned int gameIP, unsigned int gamePort,
		const char* password ) = 0;
	virtual void CloseManageServerDialog( ManageServerUIHandle_t gameDialog ) = 0;
	virtual IManageServer* GetManageServerInterface( ManageServerUIHandle_t handle ) = 0;
};

#define ADMINSERVER_INTERFACE_VERSION "AdminServer002"
