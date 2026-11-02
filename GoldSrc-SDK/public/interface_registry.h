#ifndef INTERFACE_REGISTRY_H
#define INTERFACE_REGISTRY_H

#include <cstring>

class IBaseInterface
{
public:
	virtual ~IBaseInterface() {}
};

typedef void* ( *CreateInterfaceFn )( const char* name, int* returnCode );
typedef void* ( *InstantiateInterfaceFn )();

enum
{
	IFACE_OK = 0,
	IFACE_FAILED = 1
};

class InterfaceReg
{
public:
	InterfaceReg( InstantiateInterfaceFn function, const char* name )
		: m_CreateFn( function ), m_pName( name ), m_pNext( s_pInterfaceRegs )
	{
		s_pInterfaceRegs = this;
	}

	static void* Create( const char* name, int* returnCode )
	{
		for ( InterfaceReg* entry = s_pInterfaceRegs; entry; entry = entry->m_pNext )
		{
			if ( std::strcmp( entry->m_pName, name ) == 0 )
			{
				if ( returnCode )
					*returnCode = IFACE_OK;
				return entry->m_CreateFn();
			}
		}

		if ( returnCode )
			*returnCode = IFACE_FAILED;
		return NULL;
	}

	InstantiateInterfaceFn m_CreateFn;
	const char* m_pName;
	InterfaceReg* m_pNext;
	static InterfaceReg* s_pInterfaceRegs;
};

#endif
