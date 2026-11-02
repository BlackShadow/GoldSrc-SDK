/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
/* crc.h */
#pragma once

#include "const.h"
#include "../common/crc.h"

#include <cstddef>
#include <cstdint>

#define CRC32_TABLE_SIZE 256

#define MD5_STATE_WORDS 4
#define MD5_COUNT_WORDS 2
#define MD5_BLOCK_BYTES 64
#define MD5_DIGEST_BYTES 16

typedef uint32_t CRC32_t;

extern const CRC32_t pulCRCTable[CRC32_TABLE_SIZE];

extern "C"
{
	void CRC32_Init( CRC32_t* crc );
	CRC32_t CRC32_Final( CRC32_t crc );
	void CRC32_ProcessByte( CRC32_t* crc, byte value );
	void CRC32_ProcessBuffer( CRC32_t* crc, void* buffer, int length );
	int CRC_File( CRC32_t* crc, char* filename );
}

int			CRC_MapFile( CRC32_t* crc, char* filename );
byte		COM_BlockSequenceCRCByte( byte* base, int length, int sequence );

void		MD5Init( MD5Context_t* context );
void		MD5Update( MD5Context_t* context, const byte* buffer, unsigned int length );
void		MD5Final( byte* digest, MD5Context_t* context );
void		MD5Transform( uint32_t* state, const byte* block );
int			MD5_Hash_File( byte* digest, char* filename, int useFopen, int useSeed, uint32_t* seed );
char*		MD5_Print( byte* digest );
