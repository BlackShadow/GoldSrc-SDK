#include "d_local.h"
#include "engine_color.h"
#include "render.h"
#include "vid.h"

/*
================
D_FillRect
================
*/
void D_FillRect( vrect_t* rect, byte* color )
{
	word colorVal;
	if ( is15bit )
		colorVal = (word)( ( ( color[0] << 7 ) & COLOR_REDMASK_555 )
			| ( ( color[1] << 2 ) & COLOR_GREENMASK_555 ) | ( color[2] >> 3 ) );
	else
		colorVal = (word)( ( ( color[0] << 8 ) & COLOR_REDMASK_565 )
			| ( ( color[1] << 3 ) & COLOR_GREENMASK_565 ) | ( color[2] >> 3 ) );

	int x = rect->x;
	int y = rect->y;
	int width = rect->width;
	int height = rect->height;
	if ( x < 0 )
	{
		width += x;
		x = 0;
	}
	if ( y < 0 )
	{
		height += y;
		y = 0;
	}
	if ( x + width > (int)vid.width )
		width = vid.width - x;
	if ( y + height > (int)vid.height )
		height = vid.height - x;
	if ( width <= 0 || height <= 0 )
		return;

	byte* dest = vid.buffer + y * vid.rowbytes + x * sizeof( colorVal );
	for ( int row = 0; row < height; row++ )
	{
		word* pixels = (word*)dest;
		for ( int column = 0; column < width; column++ )
			pixels[column] = colorVal;
		dest += vid.rowbytes;
	}
}
