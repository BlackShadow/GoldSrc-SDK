#include "engine_apiproxy.h"
#include "blob.h"
#include "cmd.h"
#include "common.h"
#include "conprint.h"
#include "interface.h"
#include "server_static.h"

#if defined( _WIN32 )
#include <Windows.h>
#else
#include <dlfcn.h>
#endif

#include <cstdlib>

#ifdef GetModuleHandle
#undef GetModuleHandle
#endif

module_t g_module;
modfuncs_t g_modfuncs;
void* g_pvModuleSpecial;
BlobFootprint_t g_blobfootprintModule;

#if !defined( _WIN32 )
struct client_s;
void SV_DropClient( client_s* client, qboolean crash, const char* format, ... );
void SV_KickPlayer( int nPlayerSlot, int nReason );

modshelpers_t g_modshelpers = { SV_KickPlayer };
#endif

void NullDst()
{
}

cl_enginefunc_dst_t g_engdstAddrs = k_engdstNull;
cldll_func_dst_t g_cldstAddrs = k_cldstNull;

void CloseSecurityModule()
{
	if ( !g_module.fLoaded )
		return;

#if defined( _WIN32 )
	cldll_func_dst_t cldstNull = k_cldstNull;
#endif
	cl_enginefunc_dst_t engdstNull = k_engdstNull;

	if ( g_modfuncs.m_pfnCloseMod )
		g_modfuncs.m_pfnCloseMod();

	FreeBlob( &g_blobfootprintModule );

	if ( g_pvModuleSpecial )
	{
		free( g_pvModuleSpecial );
		g_pvModuleSpecial = NULL;
	}

	Q_memset( &g_modfuncs, 0, sizeof( g_modfuncs ) );
	g_engdstAddrs = engdstNull;
#if defined( _WIN32 )
	g_cldstAddrs = cldstNull;
#endif
	Q_memset( &g_module, 0, sizeof( g_module ) );
}

#if !defined( _WIN32 )
void* GetModuleHandle( char* name )
{
	if ( !name )
		return NULL;

	void* handle = dlopen( name, RTLD_NOW );
	if ( !handle )
		return NULL;

	dlclose( handle );
	return handle;
}

void CommonLoadSecurityModule( void* nAddrModule )
{
	engdata_t engdata;

	if ( g_modfuncs.m_nVersion != k_nModuleVersionCur )
	{
		CloseSecurityModule();
		return;
	}

	if ( g_modfuncs.m_pfnSetEngineVersion )
		g_modfuncs.m_pfnSetEngineVersion( k_nEngineVersion16Validated );
	g_modfuncs.m_pfnLoadMod( (char*)nAddrModule );
	g_modfuncs.m_pfnGetEngDstAddrs( &g_engdstAddrs );
	if ( g_modfuncs.m_pfnSetModSHelpers )
		g_modfuncs.m_pfnSetModSHelpers( &g_modshelpers );

	Q_memset( &engdata, 0, sizeof( engdata ) );
	engdata.pg_engdstAddrs = &g_engdstAddrs;
	engdata.pg_modfuncs = &g_modfuncs;
	engdata.pcmd_functions = &cmd_functions;
	engdata.pfnConPrintf = (void ( * )( char*, ... ))Con_Printf;
	engdata.pcvar_vars = &cvar_vars;
	engdata.pfnSZ_GetSpace = SZ_GetSpace;
	engdata.pmodfuncs = &g_modfuncs;
	engdata.pfnGetProcAddress = (void*)dlsym;
	engdata.pfnGetModuleHandle = (void*)GetModuleHandle;
	engdata.psvs = (server_static_s*)&svs;
	engdata.pfnSV_DropClient = (void ( * )( client_s*, qboolean, char*, ... ))SV_DropClient;
	engdata.pfnNetchan_Transmit = Netchan_Transmit;
	engdata.pfnNET_SendPacket = (void ( * )( netsrc_s, int, void*, netadr_t ))NET_SendPacket;
	engdata.pfnCvarFindVar = Cvar_FindVar;

	g_modfuncs.m_pfnSetEngData( &engdata );
	g_modfuncs.m_pfnModuleLoaded();
	g_module.fLoaded = 1;
}

void LoadSecurityModuleFromDisk( char* pszDllName )
{
	void* special;
	void ( *pfnEntry )( void* );

	g_modfuncs.m_nVersion = k_nModuleVersionCur;
	NLoadBlobFile( pszDllName, &g_blobfootprintModule, &g_modfuncs, 1 );
	if ( !g_blobfootprintModule.m_hDll )
		return;

	special = malloc( 64 * 1024 );
	g_pvModuleSpecial = special;

	pfnEntry = (void ( * )( void* ))Sys_GetProcAddress( g_blobfootprintModule.m_hDll, "P" );
	pfnEntry( special );
	CommonLoadSecurityModule( special );
}
#endif
