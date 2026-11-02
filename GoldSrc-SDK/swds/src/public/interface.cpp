// interface.cpp -- named interface factories and dynamic module loading

#include "tier1/interface.h"

#if defined( _WIN32 )
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>

InterfaceReg* InterfaceReg::s_pInterfaceRegs = NULL;

#if !defined( _WIN32 )
InterfaceReg::InterfaceReg( InstantiateInterfaceFn fn, const char* pName )
	: m_CreateFn( fn ), m_pName( pName ), m_pNext( s_pInterfaceRegs )
{
	s_pInterfaceRegs = this;
}
#endif

extern "C" DLL_EXPORT void* CreateInterface( const char* pName, int* pReturnCode )
{
	for ( InterfaceReg* pCur = InterfaceReg::s_pInterfaceRegs; pCur; pCur = pCur->m_pNext )
	{
		if ( strcmp( pCur->m_pName, pName ) == 0 )
		{
			if ( pReturnCode )
				*pReturnCode = IFACE_OK;
			return pCur->m_CreateFn();
		}
	}

	if ( pReturnCode )
		*pReturnCode = IFACE_FAILED;
	return NULL;
}

#if !defined( _WIN32 )
void* GetModuleHandle( const char* name )
{
	if ( !name )
		return NULL;

	void* handle = dlopen( name, RTLD_NOW );
	if ( !handle )
		return NULL;

	dlclose( handle );
	return handle;
}
#endif

void* Sys_GetProcAddress( void* pModuleHandle, const char* pName )
{
#if defined( _WIN32 )
	return (void*)GetProcAddress( (HMODULE)pModuleHandle, pName );
#else
	return dlsym( pModuleHandle, pName );
#endif
}

CSysModule* Sys_LoadModule( const char* pModuleName )
{
#if defined( _WIN32 )
	HMODULE hDLL = LoadLibraryA( pModuleName );
	if ( !hDLL )
	{
		char str[512];

		_snprintf( str, sizeof( str ), "%s.dll", pModuleName );
		hDLL = LoadLibraryA( str );
	}
#else
	void* hDLL;
	char szAbsoluteModuleName[1024];
	szAbsoluteModuleName[0] = '\0';

	if ( pModuleName[0] != '/' )
	{
		char szCwd[1024];
		char szPath[1024];

		getcwd( szCwd, sizeof( szCwd ) );
		if ( szCwd[strlen( szCwd ) - 1] == '/' )
			szCwd[strlen( szCwd ) - 1] = '\0';

		snprintf( szPath, sizeof( szPath ), "%s/%s", szCwd, pModuleName );
		hDLL = dlopen( szPath, RTLD_NOW );
	}
	else
	{
		snprintf( szAbsoluteModuleName, sizeof( szAbsoluteModuleName ), "%s", pModuleName );
		hDLL = dlopen( pModuleName, RTLD_NOW );
	}

	if ( !hDLL )
	{
		char str[512];

		printf( "Error:%s\n", dlerror() );
		// relative paths leave the retry prefix empty.
		snprintf( str, sizeof( str ), "%s.so", szAbsoluteModuleName );
		hDLL = dlopen( str, RTLD_NOW );
	}
#endif

	return (CSysModule*)hDLL;
}

void Sys_UnloadModule( CSysModule* pModule )
{
	if ( !pModule )
		return;

#if defined( _WIN32 )
	FreeLibrary( (HMODULE)pModule );
#else
	dlclose( pModule );
#endif
}

CreateInterfaceFn Sys_GetFactory( CSysModule* pModule )
{
	if ( !pModule )
		return NULL;

#if defined( _WIN32 )
	return (CreateInterfaceFn)GetProcAddress( (HMODULE)pModule, CREATEINTERFACE_PROCNAME );
#else
	return (CreateInterfaceFn)dlsym( pModule, CREATEINTERFACE_PROCNAME );
#endif
}

CreateInterfaceFn Sys_GetFactoryThis()
{
	return CreateInterface;
}

CreateInterfaceFn Sys_GetFactory( const char* pModuleName )
{
#if defined( _WIN32 )
	HMODULE module = GetModuleHandleA( pModuleName );
	if ( !module )
		return NULL;

	return (CreateInterfaceFn)GetProcAddress( module, CREATEINTERFACE_PROCNAME );
#else
	void* handle = GetModuleHandle( pModuleName );
	return (CreateInterfaceFn)dlsym( handle, CREATEINTERFACE_PROCNAME );
#endif
}
