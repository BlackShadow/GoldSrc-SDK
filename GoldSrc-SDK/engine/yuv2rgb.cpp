#define YUV_OVERFLOW_BITS 0x40080100
#define YUV_CARRY_BITS 0x00400801

struct yuv_row_t
{
	unsigned int pairs;
	bool hasLastPixel;
	short remainingRows;
};

static yuv_row_t YUV_CountRow( int width, int remainingRows )
{
	unsigned int count = (unsigned int)remainingRows - ( (unsigned int)width << 16 ) + 0x10000;
	unsigned int pairs = 0;

	if ( (int)count < 0 )
	{
		pairs = ( ~count >> 17 ) + 1;
		count += pairs << 17;
	}

	return { pairs, ( count & 0xFFFF0000 ) == 0, (short)count };
}

static unsigned int PackYuvPixel( unsigned int value )
{
	unsigned int overflow = value & YUV_OVERFLOW_BITS;

	if ( overflow )
	{
		value |= overflow - ( overflow >> 8 );
		value += ( (int)~( value >> 1 ) >> 8 ) & YUV_CARRY_BITS;
	}

	return ( value & 0xFF ) | ( ( value >> 14 ) & 0xFF00 ) | ( ( value & 0x0007F800 ) << 5 );
}

static void YUV_WritePackedPixel( unsigned char* dst, unsigned int pixel )
{
	dst[0] = (unsigned char)pixel;
	dst[1] = (unsigned char)( pixel >> 8 );
	dst[2] = (unsigned char)( pixel >> 16 );
	dst[3] = 0;
}

static short YUV_Write444Row( unsigned char*& dst, const unsigned char*& y, const unsigned char*& u,
	const unsigned char*& v, int width, int remainingRows, const unsigned int* tables )
{
	const yuv_row_t row = YUV_CountRow( width, remainingRows );
	unsigned int pair;

	for ( pair = 0; pair < row.pairs; pair++ )
	{
		YUV_WritePackedPixel( dst, PackYuvPixel( tables[*y++] + tables[256 + *u++] + tables[512 + *v++] ) );
		YUV_WritePackedPixel( dst + 4, PackYuvPixel( tables[*y++] + tables[256 + *u++] + tables[512 + *v++] ) );
		dst += 8;
	}

	if ( row.hasLastPixel )
	{
		YUV_WritePackedPixel( dst, PackYuvPixel( tables[*y++] + tables[256 + *u++] + tables[512 + *v++] ) );
		dst += 4;
	}

	return row.remainingRows;
}

void yuv444_2_rgb8888( unsigned char* dst, const unsigned char* y, const unsigned char* u, const unsigned char* v,
	int width, int height, int ySpan, int uvSpan, int dstSpan, const unsigned int* tables, int )
{
	int remainingRows = (int)( (unsigned int)height - 1 );
	int dstPad, yPad, uvPad;

	if ( remainingRows <= 0 )
		return;

	dstPad = (int)( (unsigned int)dstSpan - (unsigned int)width * 4 );
	yPad = (int)( (unsigned int)ySpan - (unsigned int)width );
	uvPad = (int)( (unsigned int)uvSpan - (unsigned int)width );

	do
	{
		remainingRows = YUV_Write444Row( dst, y, u, v, width, remainingRows, tables );
		dst += dstPad;
		y += yPad;
		u += uvPad;
		v += uvPad;
		if ( remainingRows == 1 )
			break;

		remainingRows = YUV_Write444Row( dst, y, u, v, width, remainingRows - 1, tables );
		dst += dstPad;
		y += yPad;
		u += uvPad;
		v += uvPad;
		remainingRows--;
	} while ( remainingRows > 0 );
}

void yuv420_2_rgb8888( unsigned char* dst, const unsigned char* y, const unsigned char* u, const unsigned char* v,
	int width, int height, int ySpan, int uvSpan, int dstSpan, const unsigned int* tables, int )
{
	const unsigned int* uTable = tables + 256;
	const unsigned int* vTable = tables + 512;
	unsigned int* out = (unsigned int*)dst;
	const int outSpan = dstSpan >> 2;
	int remainingRows = (int)( (unsigned int)height - 1 );
	unsigned int pair;
	unsigned int chroma;

	if ( remainingRows > 0 )
	{
		do
		{
			const yuv_row_t row = YUV_CountRow( width, remainingRows );

			for ( pair = 0; pair < row.pairs; pair++ )
			{
				chroma = uTable[*u++] + vTable[*v++];
				out[outSpan] = PackYuvPixel( chroma + tables[y[ySpan]] );
				out[0] = PackYuvPixel( chroma + tables[y[0]] );
				out[outSpan + 1] = PackYuvPixel( chroma + tables[y[ySpan + 1]] );
				out[1] = PackYuvPixel( chroma + tables[y[1]] );
				y += 2;
				out += 2;
			}

			if ( row.hasLastPixel )
			{
				unsigned int lower, upper;

				chroma = uTable[*u] + vTable[*v];
				lower = PackYuvPixel( chroma + tables[y[ySpan]] );
				upper = PackYuvPixel( chroma + tables[y[0]] );
				y++;

				out[outSpan] = upper;
				out[0] = lower;
				out++;
			}

			out += 2 * outSpan - width;
			y += 2 * ySpan - width;
			u += uvSpan - ( width >> 1 );
			v += uvSpan - ( width >> 1 );
			remainingRows = row.remainingRows - 2;
		} while ( remainingRows > 0 );
	}

	if ( remainingRows == 0 )
	{
		const yuv_row_t row = YUV_CountRow( width, remainingRows );

		for ( pair = 0; pair < row.pairs; pair++ )
		{
			chroma = uTable[*u++] + vTable[*v++];
			out[0] = PackYuvPixel( chroma + tables[y[0]] );
			out[1] = PackYuvPixel( chroma + tables[y[1]] );
			y += 2;
			out += 2;
		}

		if ( row.hasLastPixel )
		{
			chroma = uTable[*u] + vTable[*v];
			*out = PackYuvPixel( chroma + tables[*y] );
		}
	}
}

static short YUV_Write422Row( unsigned char*& dst, const unsigned char*& y, const unsigned char*& u,
	const unsigned char*& v, int width, int remainingRows, const unsigned int* tables )
{
	const yuv_row_t row = YUV_CountRow( width, remainingRows );
	unsigned int pair;
	unsigned int chroma;

	for ( pair = 0; pair < row.pairs; pair++ )
	{
		chroma = tables[256 + *u++] + tables[512 + *v++];
		YUV_WritePackedPixel( dst, PackYuvPixel( chroma + tables[y[0]] ) );
		YUV_WritePackedPixel( dst + 4, PackYuvPixel( chroma + tables[y[1]] ) );
		y += 2;
		dst += 8;
	}

	if ( row.hasLastPixel )
	{
		chroma = tables[256 + *u] + tables[512 + *v];
		YUV_WritePackedPixel( dst, PackYuvPixel( chroma + tables[*y++] ) );
		dst += 4;
	}

	return row.remainingRows;
}

void yuv422_2_rgb8888( unsigned char* dst, const unsigned char* y, const unsigned char* u, const unsigned char* v,
	int width, int height, int ySpan, int uvSpan, int dstSpan, const unsigned int* tables, int )
{
	int remainingRows = (int)( (unsigned int)height - 1 );
	int dstPad, yPad, uvPad;

	if ( remainingRows <= 0 )
		return;

	dstPad = (int)( (unsigned int)dstSpan - (unsigned int)width * 4 );
	yPad = (int)( (unsigned int)ySpan - (unsigned int)width );
	uvPad = (int)( (unsigned int)uvSpan - (unsigned int)( width >> 1 ) );

	do
	{
		remainingRows = YUV_Write422Row( dst, y, u, v, width, remainingRows, tables );
		dst += dstPad;
		y += yPad;
		u += uvPad;
		v += uvPad;
		if ( remainingRows == 1 )
			break;

		remainingRows = YUV_Write422Row( dst, y, u, v, width, remainingRows - 1, tables );
		dst += dstPad;
		y += yPad;
		u += uvPad;
		v += uvPad;
		remainingRows--;
	} while ( remainingRows > 0 );
}
