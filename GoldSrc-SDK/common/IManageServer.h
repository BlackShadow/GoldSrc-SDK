#pragma once

#include "../public/interface.h"

class IManageServer : public IBaseInterface
{
public:
	virtual void ShowPage() = 0;
	virtual void SetAsRemoteServer( bool remote ) = 0;
	virtual void AddToConsole( const char* message ) = 0;
};

#define IMANAGESERVER_INTERFACE_VERSION "IManageServer002"
