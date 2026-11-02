// mem.c
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#if defined( NDEBUG )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* expression, const wchar_t* filename,
	unsigned int line );
#endif

void* Mem_ZeroMalloc( size_t size )
{
	void* p = malloc( size );
	if ( !p )
		_wassert( L"p", L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\engine\\mem.c", 18 );
#pragma warning( suppress: 6387 )
	memset( p, 0, size );
	return p;
}
