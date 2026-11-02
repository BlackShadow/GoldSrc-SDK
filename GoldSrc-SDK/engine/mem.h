#pragma once

#include <cstddef>

void*	Mem_Malloc( size_t size );
void*	Mem_ZeroMalloc( size_t size );
void*	Mem_Calloc( int count, size_t size );
void*	Mem_Realloc( void* block, size_t size );
char*	Mem_Strdup( const char* text );
void	Mem_Free( void* block );
