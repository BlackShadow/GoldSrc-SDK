#include "buildnum.h"
#include "common.h"

#include <cstdlib>

#if defined( _WIN32 ) || defined( SWDS_DLL )
//char *date = "Sep 30 2003"; // "Oct 24 1996";
char* date = "Oct  7 2024";
#else
char* date = "Oct  8 2024";
#endif

char* mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

int build_number()
{
	int m = 0;
	int d = 0;
	int y = 0;
	static int b = 0;

	if ( b )
		return b;

	for ( m = 0; m < 11; m++ )
	{
		if ( !Q_strnicmp( &date[0], mon[m], 3 ) )
			break;
		d += mond[m];
	}

	d += atoi( &date[4] ) - 1;

	y = atoi( &date[7] ) - 1900;

	b = d + (int)( ( y - 1 ) * 365.25 );

	if ( ( y % 4 ) == 0 && m > 1 )
		b += 1;

	b -= 34995; // Oct 24 1996

	return b;
}
