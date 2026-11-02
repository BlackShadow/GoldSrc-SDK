#include "mem.h"
#include <cstdlib>
#include <cstring>

#if defined( _WIN32 )
#include <assert.h>

#if defined( NDEBUG )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );
#endif
#endif

void* Mem_Malloc( size_t size )
{
	return malloc( size );
}

void* Mem_Calloc( int count, size_t size )
{
	return calloc( count, size );
}

void Mem_Free( void* block )
{
	free( block );
}

void* Mem_ZeroMalloc( size_t size )
{
#if defined( _WIN32 )
	void* block = malloc( size );
	if ( !block )
		_wassert( L"p", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\mem.c", 18 );

	memset( block, 0, size );
	return block;
#else
	return calloc( size, 1 );
#endif
}

void* Mem_Realloc( void* block, size_t size )
{
	return realloc( block, size );
}

char* Mem_Strdup( const char* text )
{
	if ( !text )
		return NULL;

	return _strdup( text );
}
