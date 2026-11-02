#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "../../../public/interface_registry.h"

#define CREATEINTERFACE_PROCNAME	"CreateInterface"

#define EXPOSE_INTERFACE_FN( functionName, interfaceName, versionName ) \
	static InterfaceReg __g_Create##interfaceName##_reg( functionName, versionName );

#define EXPOSE_INTERFACE( className, interfaceName, versionName ) \
	static void* __Create##className##_interface() \
	{ \
		return (interfaceName*)new className; \
	} \
	static InterfaceReg __g_Create##className##_reg( __Create##className##_interface, versionName );

// Use this to expose a singleton interface with a global variable you've created.
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

class CSysModule;

#ifdef _WIN32
#define DLL_EXPORT	__declspec( dllexport )
#else
#define DLL_EXPORT	__attribute__( ( visibility( "default" ) ) )
#endif

extern "C" DLL_EXPORT void*	CreateInterface( const char* pName, int* pReturnCode );

void*					Sys_GetProcAddress( void* pModuleHandle, const char* pName );
extern CreateInterfaceFn	Sys_GetFactoryThis();
//-----------------------------------------------------------------------------
// UNDONE: This is obsolete, use the module load/unload/get instead!!!
//-----------------------------------------------------------------------------
extern CreateInterfaceFn	Sys_GetFactory( const char* pModuleName );
//-----------------------------------------------------------------------------
// Load & Unload should be called in exactly one place for each module
// The factory for that module should be passed on to dependent components for
// proper versioning.
//-----------------------------------------------------------------------------
extern CSysModule*		Sys_LoadModule( const char* pModuleName );
extern void				Sys_UnloadModule( CSysModule* pModule );
extern CreateInterfaceFn	Sys_GetFactory( CSysModule* pModule );

#endif // MSVC 6.0
