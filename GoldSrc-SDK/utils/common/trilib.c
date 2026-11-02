/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
****/

//
// trilib.c: library for loading triangles from an Alias triangle file
//

#include <stdio.h>
#include "cmdlib.h"
#include "mathlib.h"
#include "trilib.h"

// on disk representation of a face


#define	FLOAT_START	99999.0
#define	FLOAT_END	-FLOAT_START
#define MAGIC       123322

//#define NOISY 1

typedef struct {
	float v[3];
} vector;

typedef struct
{
	vector n;    /* normal */
	vector p;    /* point */
	vector c;    /* color */
	float  u;    /* u */
	float  v;    /* v */
} aliaspoint_t;

typedef struct {
	aliaspoint_t	pt[3];
} tf_triangle;


void ByteSwapTri (tf_triangle *tri)
{
	int		i;
	
	for (i=0 ; i<sizeof(tf_triangle)/4 ; i++)
	{
		((int *)tri)[i] = BigLong (((int *)tri)[i]);
	}
}

void LoadTriangleList (char *filename, triangle_t **pptri, int *numtriangles)
{
	FILE* input;
	float start;
	char name[256], tex[256];
	int i, count, magic;
	tf_triangle tri;
	triangle_t* triangles = NULL;
	triangle_t* ptri;
	int iLevel = 0;
	int exitpattern;
	float end = -FLOAT_START;
	size_t bytes;
	char* reason = "Malformed Alias triangle file";

	if ( !filename || !pptri || !numtriangles )
		Error( "LoadTriangleList: invalid parameters" );
	*pptri = NULL;
	*numtriangles = 0;
	memcpy( &exitpattern, &end, sizeof( exitpattern ) );
	exitpattern = BigLong( exitpattern );
	input = fopen( filename, "rb" );
	if ( !input )
		Error( "reader: could not open file '%s'", filename );
	if ( fread( &magic, sizeof( magic ), 1, input ) != 1 || BigLong( magic ) != MAGIC )
		goto BadFile;

	triangles = malloc( MAXTRIANGLES * sizeof( triangle_t ) );
	if ( !triangles )
	{
		reason = "LoadTriangleList: out of memory";
		goto BadFile;
	}
	ptri = triangles;

	for ( ;; )
	{
		bytes = fread( &start, 1, sizeof( start ), input );
		if ( !bytes && feof( input ) && !iLevel )
			break;
		if ( bytes != sizeof( start ) )
			goto BadFile;
		*(int*)&start = BigLong( *(int*)&start );
		if ( *(int*)&start == exitpattern )
			break;
		if ( start != FLOAT_START && start != FLOAT_END )
			goto BadFile;

		for ( i = 0; i < (int)sizeof( name ); ++i )
		{
			int ch = fgetc( input );
			if ( ch == EOF )
				goto BadFile;
			name[i] = (char)ch;
			if ( !ch )
				break;
		}
		if ( i == sizeof( name ) )
			goto BadFile;
		if ( start == FLOAT_END )
		{
			if ( !iLevel )
				goto BadFile;
			--iLevel;
			continue;
		}

		if ( fread( &count, sizeof( count ), 1, input ) != 1 )
			goto BadFile;
		count = BigLong( count );
		if ( count < 0 || count >= MAXTRIANGLES - ( ptri - triangles ) )
			goto BadFile;
		++iLevel;
		if ( count )
		{
			for ( i = 0; i < (int)sizeof( tex ); ++i )
			{
				int ch = fgetc( input );
				if ( ch == EOF )
					goto BadFile;
				tex[i] = (char)ch;
				if ( !ch )
					break;
			}
			if ( i == sizeof( tex ) )
				goto BadFile;
		}

//
// read the triangles
//		
		for ( i = 0; i < count; ++i )
		{
			int j, k;
			if ( fread( &tri, sizeof( tri ), 1, input ) != 1 )
				goto BadFile;
			ByteSwapTri( &tri );
			for ( j = 0; j < 3; ++j )
			{
				for ( k = 0; k < 3; ++k )
					ptri->verts[j][k] = tri.pt[j].p.v[k];
			}
			++ptri;
		}
	}
	*numtriangles = ptri - triangles;
	*pptri = triangles;
	fclose( input );
	return;

BadFile:
	free( triangles );
	fclose( input );
	Error( "%s: %s", reason, filename );
}

