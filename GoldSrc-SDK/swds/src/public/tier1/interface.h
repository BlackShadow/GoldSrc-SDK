#if defined( _WIN32 )
#include "../../../../HL25-Engine/public/tier1/interface.h"
#else

#ifndef INTERFACE_H
#define INTERFACE_H

#include <stddef.h>

#define CREATEINTERFACE_PROCNAME	"CreateInterface"

#define EXPORT_FUNCTION	__attribute__(( visibility( "default" ) ))

#ifndef DLL_EXPORT
#define DLL_EXPORT	EXPORT_FUNCTION
#endif

//-----------------------------------------------------------------------------
// Purpose: supplies the common destruction contract for exposed interfaces.
//-----------------------------------------------------------------------------
class IBaseInterface
{
public:
	virtual ~IBaseInterface() {}
};

typedef void* ( *CreateInterfaceFn )( const char* pName, int* pReturnCode );
typedef void* ( *InstantiateInterfaceFn )();

enum
{
	IFACE_OK		= 0,
	IFACE_FAILED	= 1
};

//-----------------------------------------------------------------------------
// Purpose: registers one named factory in the module's interface list.
//-----------------------------------------------------------------------------
class InterfaceReg
{
public:
	InterfaceReg( InstantiateInterfaceFn fn, const char* pName );

	InstantiateInterfaceFn	m_CreateFn;
	const char*				m_pName;
	InterfaceReg*			m_pNext;

	static InterfaceReg* s_pInterfaceRegs;
};

class CSysModule;

#define EXPOSE_INTERFACE_FN( functionName, interfaceName, versionName ) \
	static InterfaceReg __g_Create##interfaceName##_reg( functionName, versionName );

#define EXPOSE_INTERFACE( className, interfaceName, versionName ) \
	static void* __Create##className##_interface() \
	{ \
		return (interfaceName*)new className; \
	} \
	static InterfaceReg __g_Create##className##_reg( __Create##className##_interface, versionName );

#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR( className, interfaceName, versionName, globalVarName ) \
	static void* __Create##className##interfaceName##_interface() \
	{ \
		return (interfaceName*)&globalVarName; \
	} \
	static InterfaceReg __g_Create##className##interfaceName##_reg( \
		__Create##className##interfaceName##_interface, versionName );

#define EXPOSE_SINGLE_INTERFACE( className, interfaceName, versionName ) \
	static className __g_##className##_singleton; \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR( className, interfaceName, versionName, __g_##className##_singleton )

//
// interface.cpp
//
extern "C" DLL_EXPORT void* CreateInterface( const char* pName, int* pReturnCode );

CreateInterfaceFn	Sys_GetFactoryThis();
CreateInterfaceFn	Sys_GetFactory( CSysModule* pModule );
CSysModule*			Sys_LoadModule( const char* pModuleName );
void				Sys_UnloadModule( CSysModule* pModule );

#if !defined( _WIN32 )
void*				GetModuleHandle( const char* name );
void*				Sys_GetProcAddress( void* pModuleHandle, const char* pName );
CreateInterfaceFn	Sys_GetFactory( const char* pModuleName );
#endif

#endif

#endif
