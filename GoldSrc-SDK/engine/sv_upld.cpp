#include "server.h"
#include "conprint.h"
#include "mem.h"
#include "hashpak.h"
#include "sys.h"
#include "protocol.h"
#include "eiface.h"

#include <cstdio>

extern cvar_t sv_allow_upload;
extern cvar_t sv_max_upload;
extern cvar_t sv_max_upload_count;

qboolean SV_UploadComplete( client_t* client );
void SV_PropagateCustomizations();

static void SV_RemoveFromResourceList( resource_t* pResource )
{
	pResource->pPrev->pNext = pResource->pNext;
	pResource->pNext->pPrev = pResource->pPrev;
	pResource->pPrev = NULL;
	pResource->pNext = NULL;
}

static void SV_AddToResourceList( resource_t* pResource, resource_t* pList )
{
	if ( pResource->pPrev || pResource->pNext )
	{
		Con_Printf( "Resource already linked\n" );
		return;
	}

	pResource->pPrev = pList->pPrev;
	pList->pPrev->pNext = pResource;
	pList->pPrev = pResource;
	pResource->pNext = pList;
}

void SV_ClearResourceList( resource_t* pList )
{
	resource_t *p, *n;

	for ( p = pList->pNext; p && p != pList; p = n )
	{
		n = p->pNext;

		SV_RemoveFromResourceList( p );
		Mem_Free( p );
	}

	pList->pPrev = pList;
	pList->pNext = pList;
}

void SV_MoveToOnHandList( resource_t* pResource )
{
	if ( !pResource )
	{
		Con_DPrintf( "Null resource passed to SV_MoveToOnHandList\n" );
		return;
	}

	SV_RemoveFromResourceList( pResource );
	SV_AddToResourceList( pResource, &host_client->resourcesonhand );
}

void SV_ClearResourceLists( client_t* client )
{
	if ( !client )
		Sys_Error( "SV_ClearResourceLists with NULL client!" );

	SV_ClearResourceList( &client->resourcesneeded );
	SV_ClearResourceList( &client->resourcesonhand );
}

static qboolean SV_CheckFile( sizebuf_t* msg, char* filename )
{
	resource_t p;

	Q_memset( &p, 0, sizeof( p ) );

	if ( Q_strlen( filename ) == 36 && !Q_strnicmp( filename, "!MD5", 4 ) )
	{
		COM_HexConvert( filename + 4, 32, p.rgucMD5_hash );
		if ( HPAK_GetDataPointer( "custom.hpk", &p, NULL, NULL ) )
			return TRUE;
	}

	if ( !sv_allow_upload.value )
		return TRUE;

	MSG_WriteByte( msg, svc_stufftext );
	MSG_WriteString( msg, va( "upload \"!MD5%s\"\n", MD5_Print( p.rgucMD5_hash ) ) );
	return FALSE;
}

void SV_BatchUploadRequest( client_t* client )
{
	resource_t *p, *n;
	char filename[260];

	for ( p = client->resourcesneeded.pNext; p != &client->resourcesneeded; p = n )
	{
		n = p->pNext;

		if ( !( p->ucFlags & RES_WASMISSING ) )
		{
			SV_MoveToOnHandList( p );
			continue;
		}

		if ( p->type != t_decal )
			continue;

		if ( p->ucFlags & RES_CUSTOM )
		{
			snprintf( filename, sizeof( filename ), "!MD5%s", MD5_Print( p->rgucMD5_hash ) );
			if ( SV_CheckFile( &client->netchan.message, filename ) )
				SV_MoveToOnHandList( p );
		}
		else
		{
			Con_Printf( "Non customization in upload queue!\n" );
			SV_MoveToOnHandList( p );
		}
	}
}

void SV_ParseResourceList( client_t* )
{
	int i, total;
	int requested;
	int maxUpload;
	resource_t* resource;
	resourceinfo_t ri;

	total = MSG_ReadShort();
	if ( sv_max_upload_count.value > 0 )
	{
		int maxCount = (int)sv_max_upload_count.value;
		if ( total > maxCount )
			total = maxCount;
		else if ( total < 0 )
			total = 0;
	}

	SV_ClearResourceList( &host_client->resourcesneeded );
	SV_ClearResourceList( &host_client->resourcesonhand );

	if ( sv_max_upload.value > 0 )
		maxUpload = (int)( sv_max_upload.value * 1048576.0f );
	else
		maxUpload = 1073741824;

	for ( i = 0; i < total; i++ )
	{
		resource = (resource_t*)Mem_ZeroMalloc( sizeof( resource_t ) );

		Q_strncpy( resource->szFileName, MSG_ReadString(), sizeof( resource->szFileName ) - 1 );
		resource->szFileName[sizeof( resource->szFileName ) - 1] = 0;
		resource->type = (resourcetype_t)MSG_ReadByte();
		resource->nIndex = MSG_ReadShort();
		resource->nDownloadSize = MSG_ReadLong();
		resource->ucFlags = (byte)( MSG_ReadByte() & ~RES_WASMISSING );
		resource->pNext = resource->pPrev = NULL;

		if ( resource->ucFlags & RES_CUSTOM )
			MSG_ReadBuf( sizeof( resource->rgucMD5_hash ), resource->rgucMD5_hash );

		SV_AddToResourceList( resource, &host_client->resourcesneeded );

		if ( msg_badread || resource->type > t_world || resource->nDownloadSize < 0 || resource->nDownloadSize > maxUpload )
		{
			SV_ClearResourceList( &host_client->resourcesneeded );
			SV_ClearResourceList( &host_client->resourcesonhand );
			return;
		}
	}

	if ( sv_allow_upload.value )
		Con_DPrintf( "Verifying and uploading resources...\n" );

	total = COM_SizeofResourceList( &host_client->resourcesneeded, &ri );
	if ( total )
	{
		Con_DPrintf( "Custom resources total %.2fK\n", (float)total / 1024.0 );

		if ( ri.info[t_model].size )
			Con_DPrintf( "  Models:  %.2fK\n", (float)ri.info[t_model].size / 1024.0 );
		if ( ri.info[t_sound].size )
			Con_DPrintf( "  Sounds:  %.2fK\n", (float)ri.info[t_sound].size / 1024.0 );
		if ( ri.info[t_decal].size )
			Con_DPrintf( "  Decals:  %.2fK\n", (float)ri.info[t_decal].size / 1024.0 );
		if ( ri.info[t_skin].size )
			Con_DPrintf( "  Skins :  %.2fK\n", (float)ri.info[t_skin].size / 1024.0 );
		if ( ri.info[t_generic].size )
			Con_DPrintf( "  Generic :  %.2fK\n", (float)ri.info[t_generic].size / 1024.0 );
		if ( ri.info[t_eventscript].size )
			Con_DPrintf( "  Events  :  %.2fK\n", (float)ri.info[t_eventscript].size / 1024.0 );

		Con_DPrintf( "----------------------\n" );

		requested = 0;
		for ( resource = host_client->resourcesneeded.pNext; resource != &host_client->resourcesneeded; resource = resource->pNext )
		{
			if ( resource->type == t_decal && !HPAK_ResourceForHash( "custom.hpk", resource->rgucMD5_hash, NULL )
				 && resource->nDownloadSize )
			{
				resource->ucFlags |= RES_WASMISSING;
				requested += resource->nDownloadSize;
			}
		}

		if ( requested > maxUpload )
		{
			SV_ClearResourceList( &host_client->resourcesneeded );
			SV_ClearResourceList( &host_client->resourcesonhand );
			return;
		}

		if ( requested > 1024 )
			Con_DPrintf( "Resources to request: %.2fK\n", (float)requested / 1024.0 );
		else
			Con_DPrintf( "Resources to request: %i bytes\n", requested );
	}

	host_client->uploading = TRUE;
	host_client->uploaddoneregistering = FALSE;

	SV_BatchUploadRequest( host_client );
}

static void SV_CreateCustomizationList( client_t* pHost )
{
	resource_t* pResource;
	customization_t* pList;
	customization_t* pCust;
	int nLumps;

	pHost->customdata.pNext = NULL;

	for ( pResource = pHost->resourcesonhand.pNext; pResource != &pHost->resourcesonhand; pResource = pResource->pNext )
	{
		for ( pList = pHost->customdata.pNext; pList; pList = pList->pNext )
		{
			if ( !Q_memcmp( pList->resource.rgucMD5_hash, pResource->rgucMD5_hash, sizeof( pResource->rgucMD5_hash ) ) )
				break;
		}

		if ( pList )
		{
			Con_DPrintf( "SV_CreateCustomization list, ignoring dup. resource for player %s\n", pHost->name );
			continue;
		}

		nLumps = 0;
		if ( COM_CreateCustomization( &pHost->customdata, pResource, -1, FCUST_FROMHPAK | FCUST_WIPEDATA, &pCust, &nLumps ) )
		{
			pCust->nUserData2 = nLumps;
			gEntityInterface.pfnPlayerCustomization( pHost->edict, pCust );
		}
		else if ( sv_allow_upload.value )
		{
			Con_Printf( "Ignoring invalid custom decal from %s\n", pHost->name );
		}
		else
		{
			Con_Printf( "Ignoring custom decal from %s\n", pHost->name );
		}
	}
}

static void SV_Customization( client_t* pPlayer, resource_t* pResource, qboolean bSkipPlayer )
{
	int i;
	int nPlayerNumber;

	nPlayerNumber = -1;
	for ( i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++ )
	{
		if ( host_client == pPlayer )
		{
			nPlayerNumber = i;
			break;
		}
	}

	if ( nPlayerNumber == -1 )
		Sys_Error( "Couldn't find player index for customization." );

	for ( i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++ )
	{
		if ( !host_client->active && !host_client->spawned )
			continue;
		if ( host_client->fakeclient || ( bSkipPlayer && host_client == pPlayer ) )
			continue;

		MSG_WriteByte( &host_client->netchan.message, svc_customization );
		MSG_WriteByte( &host_client->netchan.message, nPlayerNumber );
		MSG_WriteByte( &host_client->netchan.message, pResource->type );
		MSG_WriteString( &host_client->netchan.message, pResource->szFileName );
		MSG_WriteShort( &host_client->netchan.message, pResource->nIndex );
		MSG_WriteLong( &host_client->netchan.message, pResource->nDownloadSize );
		MSG_WriteByte( &host_client->netchan.message, pResource->ucFlags );
		if ( pResource->ucFlags & RES_CUSTOM )
			SZ_Write( &host_client->netchan.message, pResource->rgucMD5_hash, sizeof( pResource->rgucMD5_hash ) );
	}
}

void SV_RegisterResources()
{
	resource_t* pResource;
	client_t* pHost;

	pHost = host_client;
	pHost->uploading = FALSE;

	for ( pResource = pHost->resourcesonhand.pNext; pResource != &pHost->resourcesonhand; pResource = pResource->pNext )
	{
		SV_CreateCustomizationList( pHost );
		SV_Customization( pHost, pResource, TRUE );
	}

	host_client = pHost;
}

void SV_RequestMissingResourcesFromClients()
{
	int i;

	for ( i = 0, host_client = svs.clients; i < svs.maxclients; i++, host_client++ )
	{
		if ( !host_client->active && !host_client->spawned )
			continue;

		if ( host_client->uploading && !host_client->uploaddoneregistering )
			SV_UploadComplete( host_client );
	}
}

int SV_EstimateNeededResources()
{
	resource_t* p;
	int missing;

	missing = 0;

	for ( p = host_client->resourcesneeded.pNext; p != &host_client->resourcesneeded; p = p->pNext )
	{
		if ( p->type != t_decal )
			continue;

		if ( !HPAK_ResourceForHash( "custom.hpk", p->rgucMD5_hash, NULL ) && p->nDownloadSize )
		{
			p->ucFlags |= RES_WASMISSING;
			missing += p->nDownloadSize;
		}
	}

	return missing;
}

qboolean SV_UploadComplete( client_t* client )
{
	if ( client->resourcesneeded.pNext != &client->resourcesneeded )
		return FALSE;

	SV_RegisterResources();
	SV_PropagateCustomizations();

	if ( sv_allow_upload.value )
		Con_DPrintf( "Custom resource propagation complete.\n" );

	client->uploaddoneregistering = TRUE;
	return TRUE;
}

qboolean SV_RequestMissingResources()
{
	if ( host_client->uploading && !host_client->uploaddoneregistering )
		SV_UploadComplete( host_client );

	return FALSE;
}
