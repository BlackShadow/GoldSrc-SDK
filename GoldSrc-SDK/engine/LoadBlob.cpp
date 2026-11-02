#include "blob.h"
#include "common.h"
#include "filesystem.h"

#if !defined( _WIN32 )
#include <dlfcn.h>
#else
#include <assert.h>

#if defined( NDEBUG )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );
#endif

#define LOADBLOB_SOURCE_PATH L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\LoadBlob.cpp"
#endif

#include <cstdlib>
#include <cstring>

#if defined( _WIN32 )

#define BLOB_XOR_SEED			'W'
#define BLOB_EXPORT_POINT_KEY	0x7A32BC85
#define BLOB_IMAGE_BASE_KEY		0x49C042D1
#define BLOB_IMPORT_TABLE_KEY	0x872C3D47
#define BLOB_ENTRY_POINT_BIAS	12

typedef struct BlobInfo_s
{
	unsigned char m_rgbInfo[68];
} BlobInfo_t;

typedef struct BlobHeader_s
{
	unsigned long m_dwCheckSum;
	int m_nSectionCount;
	unsigned long m_dwExportPoint;
	unsigned long m_dwImageBase;
	unsigned long m_dwEntryPoint;
	unsigned long m_dwImportTable;
} BlobHeader_t;

typedef struct BlobSection_s
{
	unsigned long m_dwVirtualAddress;
	int m_nVirtualSize;
	int m_nDataSize;
	unsigned long m_dwDataAddress;
	int m_fIsDataSection;
} BlobSection_t;

typedef BOOL( WINAPI* BlobDllMain_t )( HINSTANCE, DWORD, LPVOID );
typedef void ( *BlobExportPoint_t )( void* );

static void* LoadBlobFile( unsigned char* pBuffer, BlobFootprint_t*, void* pv, int nSize )
{
	BlobHeader_t* pHeader;
	BlobSection_t* pSection;
	IMAGE_IMPORT_DESCRIPTOR* pImport;
	IMAGE_THUNK_DATA* pThunk;
	HMODULE hImport;
	const char* pszProcName;
	void* pDataSection = NULL;
	unsigned char bXor;
	int i;

	bXor = BLOB_XOR_SEED;
	for ( i = sizeof( BlobInfo_t ); i < nSize; i++ )
	{
		pBuffer[i] ^= bXor;
		bXor += pBuffer[i] + BLOB_XOR_SEED;
	}

	pHeader = (BlobHeader_t*)( pBuffer + sizeof( BlobInfo_t ) );
	pSection = (BlobSection_t*)( pHeader + 1 );

	for ( i = 0; i <= pHeader->m_nSectionCount; i++, pSection++ )
	{
		if ( pSection->m_fIsDataSection )
			pDataSection = (void*)pSection->m_dwVirtualAddress;

		if ( pSection->m_nVirtualSize > pSection->m_nDataSize )
			memset( (char*)pSection->m_dwVirtualAddress + pSection->m_nDataSize, 0, pSection->m_nVirtualSize - pSection->m_nDataSize );

		memcpy( (void*)pSection->m_dwVirtualAddress, pBuffer + pSection->m_dwDataAddress, pSection->m_nDataSize );
	}

	for ( pImport = (IMAGE_IMPORT_DESCRIPTOR*)( pHeader->m_dwImportTable ^ BLOB_IMPORT_TABLE_KEY ); pImport->Name; pImport++ )
	{
		hImport = LoadLibraryA( (const char*)( ( pHeader->m_dwImageBase ^ BLOB_IMAGE_BASE_KEY ) + pImport->Name ) );

		for ( pThunk = (IMAGE_THUNK_DATA*)( ( pHeader->m_dwImageBase ^ BLOB_IMAGE_BASE_KEY ) + pImport->FirstThunk ); pThunk->u1.Function; pThunk++ )
		{
			pszProcName = (const char*)( pThunk->u1.Ordinal & 0x7FFFFFFF );
			if ( (int)pThunk->u1.Ordinal >= 0 )
				pszProcName += ( pHeader->m_dwImageBase ^ BLOB_IMAGE_BASE_KEY ) + 2;

			pThunk->u1.Function = (DWORD)GetProcAddress( hImport, pszProcName );
		}
	}

	( (BlobDllMain_t)( pHeader->m_dwEntryPoint - BLOB_ENTRY_POINT_BIAS ) )( NULL, DLL_PROCESS_ATTACH, NULL );
	( (BlobExportPoint_t)( pHeader->m_dwExportPoint ^ BLOB_EXPORT_POINT_KEY ) )( pv );

	return pDataSection;
}

#endif

void FreeBlob( BlobFootprint_t* pblobfootprint )
{
#if !defined( _WIN32 )
	if ( !pblobfootprint->m_hDll )
		return;

	dlclose( pblobfootprint->m_hDll );
#else
	FreeLibrary( pblobfootprint->m_hDll );
#endif
}

void* NLoadBlobFile( const char* filename, BlobFootprint_t* pblobfootprint, void* pv, BlobLoadMode_t fLoadAsDll )
{
	char path[2048];
	char localPath[2048];
	void ( *pfnEntry )( void* );

	if ( !fLoadAsDll )
	{
#if !defined( _WIN32 )
		return NULL;
#else
		FileHandle_t hFile;
		int nSize;
		unsigned char* pBuffer;
		void* pModule;

		hFile = g_pFileSystem->OpenFromCacheForRead( filename, "rb", NULL );
		if ( !hFile && strcmp( com_gamedir, "cstrike" ) )
			hFile = g_pFileSystem->Open( filename, "rb", NULL );

		g_pFileSystem->Seek( hFile, 0, FILESYSTEM_SEEK_TAIL );
		nSize = g_pFileSystem->Tell( hFile );
		g_pFileSystem->Seek( hFile, 0, FILESYSTEM_SEEK_HEAD );

		pBuffer = (unsigned char*)malloc( nSize );
		g_pFileSystem->Read( pBuffer, nSize, hFile );
		pModule = LoadBlobFile( pBuffer, pblobfootprint, pv, nSize );
		free( pBuffer );

		g_pFileSystem->Close( hFile );
		return pModule;
#endif
	}

#if !defined( _WIN32 )
	if ( strlen( filename ) >= sizeof( path ) )
		abort();
#endif
	strcpy( path, filename );
	g_pFileSystem->GetLocalPath( path, localPath, sizeof( localPath ) );

#if !defined( _WIN32 )
	void* hmoduleT = FS_LoadLibrary( localPath );
#else
	HMODULE hmoduleT = LoadLibraryA( localPath );
#endif
	pblobfootprint->m_hDll = hmoduleT;
#if defined( _WIN32 )
	if ( !hmoduleT )
		_wassert( L"NULL != hmoduleT", LOADBLOB_SOURCE_PATH, 90 );
#else
	if ( !hmoduleT )
		return NULL;
#endif

#if !defined( _WIN32 )
	pfnEntry = (void ( * )( void* ))dlsym( hmoduleT, "F" );
#else
	pfnEntry = (void ( * )( void* ))GetProcAddress( hmoduleT, "F" );
#endif
	if ( !pfnEntry )
	{
#if !defined( _WIN32 )
		FreeBlob( pblobfootprint );
#else
		FreeLibrary( hmoduleT );
#endif
		return NULL;
	}

	pfnEntry( pv );
	return hmoduleT;
}

char FIsBlob( const char* )
{
	return 0;
}
