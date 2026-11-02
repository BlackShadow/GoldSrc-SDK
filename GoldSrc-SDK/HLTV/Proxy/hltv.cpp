#include "../../public/tier1/interface.h"
#include "Proxy.h"

#ifdef _WIN32
EXPOSE_SINGLE_INTERFACE( Proxy, ISystemModule, "proxy001" );
#else
EXPOSE_SINGLE_INTERFACE( Proxy, IProxy, "proxy001" );
#endif
