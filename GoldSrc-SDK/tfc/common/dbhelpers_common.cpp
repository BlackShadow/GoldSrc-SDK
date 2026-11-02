#include "dbhelpers_common.h"

#include <stdio.h>

bool GetUniqueFilename( const char* pTitle, const char* pExt, char* pFilename )
{
	for ( int i = 0; i < 32000; ++i )
	{
		sprintf( pFilename, "%s%d.%s", pTitle, i, pExt );
		FILE* pFile = fopen( pFilename, "rb" );
		if ( !pFile )
			return true;
		fclose( pFile );
	}
	return false;
}
