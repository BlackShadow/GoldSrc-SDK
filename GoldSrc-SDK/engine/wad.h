#pragma once

#include "const.h"

#define TYP_QPIC 66

#define WAD_ID_BYTES 4
#define QPIC_DATA_BYTES 4
#define WAD_LUMP_NAME_BYTES 16

typedef struct qpic_s
{
	int width;
	int height;
	byte data[QPIC_DATA_BYTES]; // variably sized
} qpic_t;

typedef struct
{
	char identification[WAD_ID_BYTES]; // should be WAD2 or 2DAW
	int numlumps;
	int infotableofs;
} wadinfo_t;

typedef struct lumpinfo_s
{
	int filepos;
	int disksize;
	int size; // uncompressed
	char type;
	char compression;
	char pad1;
	char pad2;
	char name[WAD_LUMP_NAME_BYTES]; // must be null terminated
} lumpinfo_t;

int				W_LoadWadFile( char* filename );
void			W_Shutdown();
void			W_CleanupName( char* in, char* out );
lumpinfo_t*		W_GetLumpinfo( int wad, char* name, qboolean doerror );
void*			W_GetLumpName( int wad, char* name );
void*			W_GetLumpNum( int wad, int num );
void			SwapPic( qpic_t* pic );
