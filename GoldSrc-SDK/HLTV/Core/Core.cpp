#include "HLTV/Core/Server.h"
#include "public/tier1/interface.h"

IServer* CreateServer()
{
	return new Server();
}

static InterfaceReg __g_CreateIServer_reg( (InstantiateInterfaceFn)CreateServer, "server001" );
