// This header defines the interface convention used in the valve engine.
// To make an interface and expose it:
//    1. Derive from IBaseInterface.
//    2. The interface must be ALL pure virtuals, and have no data members.
//    3. Define a name for it.
//    4. In its implementation file, use EXPOSE_INTERFACE or EXPOSE_SINGLE_INTERFACE.

// Versioning
// There are two versioning cases that are handled by this:
// 1. You add functions to the end of an interface, so it is binary compatible with the previous interface. In this case, 
//    you need two EXPOSE_INTERFACEs: one to expose your class as the old interface and one to expose it as the new interface.
// 2. You update an interface so it's not compatible anymore (but you still want to be able to expose the old interface 
//    for legacy code). In this case, you need to make a new version name for your new interface, and make a wrapper interface and 
//    expose it for the old interface.

// JoshA: Everything uses this now.
#ifndef INTERFACE_H
#define INTERFACE_H

#ifdef _WIN32
#pragma once
#endif

#include "interface_registry.h"

#define CREATEINTERFACE_PROCNAME	"CreateInterface"


// Use this to expose an interface that can have multiple instances.
// e.g.:
// EXPOSE_INTERFACE( CInterfaceImp, IInterface, "MyInterface001" )
// This will expose a class called CInterfaceImp that implements IInterface (a pure class)
// clients can receive a pointer to this class by calling CreateInterface( "MyInterface001" )
//
// In practice, the shared header file defines the interface (IInterface) and version name ("MyInterface001")
// so that each component can use these names/vtables to communicate
//
// A single class can support multiple interfaces through multiple inheritance
//
// Use this if you want to write the factory function.
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


// load/unload components
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

#endif
