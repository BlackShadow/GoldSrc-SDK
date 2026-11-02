//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
// Extremely low-level platform-specific stuff
//=============================================================================


#ifndef PLATFORM_H
#define PLATFORM_H

#include <stddef.h>

#if defined( _WIN32 )
#include <intrin.h>
#if defined( TIER0_DLL )
#define TIER0_API __declspec( dllexport )
#else
#define TIER0_API __declspec( dllimport )
#endif
#else
#include <signal.h>
#include <x86intrin.h>
#define TIER0_API __attribute__( ( visibility( "default" ) ) )
#endif

typedef long long			int64;
typedef unsigned long long	uint64;
typedef unsigned char		uint8;

struct CPUInformation
{
	int		m_Size; // Size of this structure, for forward compatability.

	bool	m_bRDTSC : 1;
	bool	m_bCMOV : 1;
	bool	m_bFCMOV : 1;
	bool	m_bSSE : 1;
	bool	m_bSSE2 : 1;
	bool	m_b3DNow : 1;
	bool	m_bMMX : 1;
	bool	m_bHT : 1;

	uint8	m_nLogicalProcessors; // Number op logical processors.
	uint8	m_nPhysicalProcessors; // Number of physical processors
	int64	m_Speed; // In cycles per second.
	char*	m_szProcessorID; // Processor vendor Identification.
};

CPUInformation*	GetCPUInformation();

#if !defined( _WIN32 )
int64	CalculateCPUFreq();
#endif

inline void DebuggerBreak()
{
#if defined( _WIN32 )
	__debugbreak();
#else
	raise( SIGTRAP );
#endif
}

extern "C"
{
	TIER0_API double			Plat_FloatTime(); // Returns time in seconds since the module was loaded.
	TIER0_API unsigned int	Plat_MSTime();
	TIER0_API const char*	Plat_GetCommandLine();
	TIER0_API bool			Plat_IsInDebugSession();
	TIER0_API void*			Plat_Alloc( unsigned int size ); // Allocate some memory.
	TIER0_API void*			Plat_Realloc( void* ptr, unsigned int size ); // Rellocate some memory. It may or may not return the same
																					// block of memory.
	TIER0_API void			Plat_Free( void* ptr ); // Free some memory.
	TIER0_API bool			vtune( bool resume );
#if defined( _WIN32 )
	TIER0_API bool			Is64BitWindows();
	TIER0_API bool			Plat_VerifyHardwareKey();
	TIER0_API bool			Plat_FastVerifyHardwareKey();
	TIER0_API bool			Plat_VerifyHardwareKeyDriver();
	TIER0_API bool			Plat_VerifyHardwareKeyPrompt();
#endif
}

#endif /* PLATFORM_H */
