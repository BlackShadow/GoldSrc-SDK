#ifndef BUILDSLAVE_ASSERT_H
#define BUILDSLAVE_ASSERT_H
#pragma once

#include <assert.h>

#if defined( _WIN32 )

#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable : 28251 )
#endif
#ifdef __cplusplus
extern "C"
#endif
_ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned line );
#ifdef _MSC_VER
#pragma warning( pop )
#endif

#define BUILDSLAVE_PATH( relative )	L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\" relative

#define ASSERT_AT( expression, line ) \
	(void)( ( !!( expression ) ) || ( _wassert( _CRT_WIDE( #expression ), ASSERT_FILE, (unsigned)( line ) ), 0 ) )

#else

#define BUILDSLAVE_PATH( relative )	relative
#define ASSERT_AT( expression, line )	assert( expression )

#endif

#endif
