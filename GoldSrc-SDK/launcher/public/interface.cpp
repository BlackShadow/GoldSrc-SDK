#include "tier1/interface.h"

#include <cstring>

InterfaceReg* InterfaceReg::s_pInterfaceRegs = NULL;

extern "C" void* CreateInterface( const char* name, int* returnCode )
{
	return InterfaceReg::Create( name, returnCode );
}
