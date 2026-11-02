// mem.h -- engine heap allocation

#ifndef MEM_H
#define MEM_H

#if defined( _WIN32 )
#include "../../../HL25-Engine/engine/mem.h"
#else

#include <stddef.h>

//
// mem.c
//
extern "C"
{
	void*	Mem_ZeroMalloc( size_t size );
	void	Mem_Free( void* block );
}

#endif

#endif
