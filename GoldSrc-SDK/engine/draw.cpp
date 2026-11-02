#include "draw.h"
#include "model.h"
#include "vid.h"
#include "render.h"
#include "d_local.h"
#include "r_local.h"
#include "engine_color.h"
#include "common.h"
#include "conprint.h"
#include "cvar.h"
#include "decals.h"
#include "mem.h"
#include "sys.h"
#include "sound.h"
#include "engine_apiproxy.h"
#include "vgui_intwrap2.h"
#include "buildnum.h"
#include "protocol.h"

#include <cmath>
#include <cstdio>

static int firsttime = 1;
static wrect_t scissor_rect;
static int scissor_x, scissor_y, scissor_x2, scissor_y2;
static qboolean giScissorTest;

qpic_t* draw_disc;
qpic_t* draw_backtile;
cachewad_t custom_wad;
cachepic_t menu_cachepics[16];
int menu_numcachepics;

extern byte		texgammatable[256];
extern qboolean	gfCustomBuild;
extern char		szCustName[10];
extern cvar_t	con_color;
extern cvar_t	console;

void Draw_MiptexTexture( cachewad_t* wad, byte* data )
{
	if ( wad->cacheExtra != sizeof( texture_t ) - sizeof( miptex_t ) )
		Sys_Error( "Draw_MiptexTexture: Bad cached wad %s\n", wad->name );

	miptex_t mip;
	Q_memcpy( &mip, data + wad->cacheExtra, sizeof( mip ) );
	texture_t* texture = (texture_t*)data;
	Q_memcpy( texture->name, mip.name, sizeof( texture->name ) );
	texture->width = LittleLong( mip.width );
	texture->height = LittleLong( mip.height );
	texture->anim_total = 0;
	texture->anim_min = 0;
	texture->anim_max = 0;
	texture->anim_next = NULL;
	texture->alternate_anims = NULL;
	for ( int i = 0; i < MIPLEVELS; ++i )
		texture->offsets[i] = wad->cacheExtra + LittleLong( mip.offsets[i] );

	int pixels = texture->width * texture->height;
	int mipPixels = pixels + ( pixels >> 4 ) + ( pixels >> 2 ) + ( pixels >> 6 );
	texture->paloffset = mipPixels + texture->offsets[0] + sizeof( word );
	int colors = *(word*)( data + mipPixels + sizeof( texture_t ) );
	byte* palette = data + texture->paloffset;
	if ( gfCustomBuild )
	{
		Q_strncpy( texture->name, szCustName, sizeof( texture->name ) - 1 );
		texture->name[sizeof( texture->name ) - 1] = 0;
	}
	texture->name[0] = palette[765] == 0 && palette[766] == 0 && palette[767] == 255 ? '{' : '}';
	for ( int i = 0; i < colors; ++i )
	{
		palette[i * 3] = texgammatable[palette[i * 3]];
		palette[i * 3 + 1] = texgammatable[palette[i * 3 + 1]];
		palette[i * 3 + 2] = texgammatable[palette[i * 3 + 2]];
	}
}

short hlRGB( word* palette, int index )
{
	PackedColorVec* color = &( (PackedColorVec*)palette )[index];
	if ( is15bit )
		return (word)( ( (short)color->r >> 3 ) | ( ( color->g << 2 ) & COLOR_GREENMASK_555 )
			| ( ( color->b << 7 ) & COLOR_REDMASK_555 ) );
	return (word)( ( (short)color->r >> 3 ) | ( ( color->g << 3 ) & COLOR_GREENMASK_565 )
		| ( ( color->b << 8 ) & COLOR_REDMASK_565 ) );
}

short PackedRGB( byte* palette, int index )
{
	return RGBToPacked( palette + index * 3 );
}

short PutRGB( colorVec* color )
{
	if ( is15bit )
		return (word)( ( (short)color->b >> 3 ) | ( ( color->g << 2 ) & COLOR_GREENMASK_555 )
			| ( ( color->r << 7 ) & COLOR_REDMASK_555 ) );
	return (word)( ( (short)color->b >> 3 ) | ( ( color->g << 3 ) & COLOR_GREENMASK_565 )
		| ( ( color->r << 8 ) & COLOR_REDMASK_565 ) );
}

short RGBToPacked( byte* color )
{
	if ( is15bit )
		return (word)( ( color[2] >> 3 ) | ( ( color[1] << 2 ) & COLOR_GREENMASK_555 )
			| ( ( color[0] << 7 ) & COLOR_REDMASK_555 ) );
	return (word)( ( color[2] >> 3 ) | ( ( color[1] << 3 ) & COLOR_GREENMASK_565 )
		| ( ( color[0] << 8 ) & COLOR_REDMASK_565 ) );
}

void GetRGB( short pixel, colorVec* color )
{
	color->b = (byte)( pixel << 3 );
	if ( is15bit )
	{
		color->r = ( pixel & COLOR_REDMASK_555 ) >> 7;
		color->g = ( pixel & COLOR_GREENMASK_555 ) >> 2;
	}
	else
	{
		color->r = ( pixel & COLOR_REDMASK_565 ) >> 8;
		color->g = ( pixel & COLOR_GREENMASK_565 ) >> 3;
	}
}

qpic_t* Draw_PicFromWad( char* name )
{
	return name ? (qpic_t*)W_GetLumpName( 0, name ) : NULL;
}

qpic_t* Draw_CachePic( char* path )
{
	int index = Draw_CacheIndex( menu_wad, path );
	return (qpic_t*)Draw_CacheGet( menu_wad, index );
}

/*
===============
Draw_Init
===============
*/
void Draw_Init()
{
	Draw_Shutdown();
	VGUI2_Draw_Init();
	m_bDrawInitialized = TRUE;
	firsttime = 1;
	menu_wad = (cachewad_t*)Mem_Malloc( sizeof( *menu_wad ) );
	Q_memset( menu_wad, 0, sizeof( *menu_wad ) );
	Draw_CacheWadInit( "cached.wad", 16, menu_wad );
	Draw_CacheWadHandler( &custom_wad, Draw_MiptexTexture, sizeof( texture_t ) - sizeof( miptex_t ) );
	Q_memset( decal_names, 0, sizeof( decal_names ) );
	draw_disc = (qpic_t*)W_GetLumpName( 0, "lambda" );
	int r, g, b;
	if ( sscanf( con_color.string, "%i %i %i", &r, &g, &b ) == 3 )
		VGUI2_Draw_SetTextColor( r, g, b );
}

void Draw_ResetTextColor()
{
	int r, g, b;
	if ( sscanf( con_color.string, "%i %i %i", &r, &g, &b ) == 3 )
		VGUI2_Draw_SetTextColor( r, g, b );
}

void Draw_SetTextColor( float r, float g, float b )
{
	g_engdstAddrs.pfnDrawSetTextColor( &r, &g, &b );
	VGUI2_Draw_SetTextColor( (int)( r * 255.0f ), (int)( g * 255.0f ), (int)( b * 255.0f ) );
}

void Draw_GetDefaultColor()
{
	int r, g, b;
	if ( sscanf( con_color.string, "%i %i %i", &r, &g, &b ) == 3 )
		VGUI2_Draw_SetTextColor( r, g, b );
}

int Draw_Character( int x, int y, int character, unsigned int font )
{
	return VGUI2_Draw_Character( x, y, character, font );
}

int Draw_String( int x, int y, char* text )
{
	int width = VGUI2_DrawString( x, y, text, VGUI2_GetConsoleFont() );
	int r, g, b;
	if ( sscanf( con_color.string, "%i %i %i", &r, &g, &b ) == 3 )
		VGUI2_Draw_SetTextColor( r, g, b );
	return x + width;
}

void Draw_DebugChar( char )
{
}

int Draw_StringLen( const char* text, unsigned int font )
{
	return VGUI2_Draw_StringLen( text, font );
}

int Draw_MessageFontInfo( short*, unsigned int font )
{
	return VGUI2_GetFontTall( font );
}

int Draw_MessageCharacterAdd( int x, int y, int character, int r, int g, int b, unsigned int font )
{
	return VGUI2_Draw_CharacterAdd( x, y, character, r, g, b, font );
}

/*
=============
Draw_Pic
=============
*/
void Draw_Pic( int x, int y, qpic_t* pic )
{
	if ( !pic )
		return;
	if ( x < 0 || x + pic->width > (int)vid.width || y < 0 || y + pic->height > (int)vid.height )
	{
		Sys_Warning( "Draw_Pic: bad coordinates" );
		return;
	}
	byte* palette = pic->data + pic->width * pic->height + sizeof( word );
	byte* source = pic->data;
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	for ( int row = 0; row < pic->height; ++row )
	{
		for ( int col = 0; col < pic->width; ++col )
			dest[col] = PackedRGB( palette, source[col] );
		source += pic->width;
		dest += vid.rowbytes >> 1;
	}
}

void Draw_AlphaSubPic( int xDest, int yDest, int xSrc, int ySrc, int width, int height,
	qpic_t* pic, colorVec* color, int alpha )
{
	if ( !pic )
		return;
	if ( xDest < 0 || xDest + width > (int)vid.width || yDest < 0 || yDest + height > (int)vid.height )
	{
		Sys_Warning( "Draw_AlphaPic: bad coordinates" );
		return;
	}
	int right = width + xSrc < pic->width ? width + xSrc : pic->width;
	int bottom = height + ySrc < pic->height ? height + ySrc : pic->height;
	word* dest = (word*)vid.buffer + xDest + yDest * ( vid.rowbytes >> 1 );
	byte* source = pic->data + pic->width * ySrc;
	byte* palette = pic->data + pic->width * pic->height + sizeof( word );
	for ( int row = ySrc; row < bottom; ++row )
	{
		for ( int col = xSrc; col < right; ++col )
		{
			int index = source[col];
			if ( color ? index == 0 : index == 255 )
				continue;
			colorVec pixel;
			GetRGB( dest[col - xSrc], &pixel );
			if ( color )
			{
				int coverage = ( alpha * index ) >> 8;
				pixel.r = ( ( 255 - coverage ) * pixel.r + coverage * color->r ) >> 8;
				pixel.g = ( ( 255 - coverage ) * pixel.g + coverage * color->g ) >> 8;
				pixel.b = ( ( 255 - coverage ) * pixel.b + coverage * color->b ) >> 8;
			}
			else
			{
				pixel.r = ( ( 255 - alpha ) * pixel.r + alpha * palette[index * 3] ) >> 8;
				pixel.g = ( ( 255 - alpha ) * pixel.g + alpha * palette[index * 3 + 1] ) >> 8;
				pixel.b = ( ( 255 - alpha ) * pixel.b + alpha * palette[index * 3 + 2] ) >> 8;
			}
			dest[col - xSrc] = PutRGB( &pixel );
		}
		dest += vid.rowbytes >> 1;
		source += pic->width;
	}
}

void Draw_AlphaPic( int x, int y, qpic_t* pic, colorVec* color, int alpha )
{
	if ( pic )
		Draw_AlphaSubPic( x, y, 0, 0, pic->width, pic->height, pic, color, alpha );
}

void Draw_AlphaAddPic( int x, int y, qpic_t* pic, colorVec*, int alpha )
{
	if ( !pic )
		return;
	if ( x < 0 || x + pic->width > (int)vid.width || y < 0 || y + pic->height > (int)vid.height )
	{
		Sys_Warning( "Draw_AlphaPic: bad coordinates" );
		return;
	}
	byte* source = pic->data;
	byte* palette = pic->data + pic->width * pic->height + sizeof( word );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	for ( int row = 0; row < pic->height; ++row )
	{
		for ( int col = 0; col < pic->width; ++col )
		{
			int index = source[col];
			if ( index == 255 )
				continue;
			colorVec pixel;
			GetRGB( dest[col], &pixel );
			pixel.r += ( alpha * palette[index * 3] ) >> 8;
			pixel.g += ( alpha * palette[index * 3 + 1] ) >> 8;
			pixel.b += ( alpha * palette[index * 3 + 2] ) >> 8;
			if ( pixel.r > 255 )
				pixel.r = 255;
			if ( pixel.g > 255 )
				pixel.g = 255;
			if ( pixel.b > 255 )
				pixel.b = 255;
			dest[col] = PutRGB( &pixel );
		}
		dest += vid.rowbytes >> 1;
		source += pic->width;
	}
}

/*
=============
Draw_TransPic
=============
*/
void Draw_TransPic( int x, int y, qpic_t* pic )
{
	if ( x < 0 || (unsigned int)( x + pic->width ) > vid.width
		|| y < 0 || (unsigned int)( y + pic->height ) > vid.height )
		Sys_Error( "Draw_TransPic: bad coordinates" );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	byte* source = pic->data;
	byte* palette = pic->data + pic->width * pic->height + sizeof( word );
	for ( int row = 0; row < pic->height; ++row )
	{
		for ( int col = 0; col < pic->width; ++col )
		{
			if ( source[col] != 255 )
				dest[col] = PackedRGB( palette, source[col] );
		}
		dest += vid.rowbytes >> 1;
		source += pic->width;
	}
}

void EnableScissorTest( int x, int y, int width, int height )
{
	scissor_x = x > (int)vid.width ? vid.width : ( x < 0 ? 0 : x );
	scissor_y = y > (int)vid.height ? vid.height : ( y < 0 ? 0 : y );
	scissor_x2 = x + width > (int)vid.width ? vid.width : ( x + width < 0 ? 0 : x + width );
	scissor_y2 = y + height > (int)vid.height ? vid.height : ( y + height < 0 ? 0 : y + height );
	scissor_rect.left = scissor_x;
	scissor_rect.top = scissor_y;
	scissor_rect.right = scissor_x2;
	scissor_rect.bottom = scissor_y2;
	giScissorTest = TRUE;
}

void DisableScissorTest()
{
	giScissorTest = FALSE;
	scissor_x = scissor_y = scissor_x2 = scissor_y2 = 0;
	Q_memset( &scissor_rect, 0, sizeof( scissor_rect ) );
}

int ValidateWRect( const wrect_t* rect )
{
	return rect && rect->left < rect->right && rect->top < rect->bottom;
}

int IntersectWRect( const wrect_t* first, const wrect_t* second, wrect_t* out )
{
	wrect_t rect;
	if ( !out )
		out = &rect;
	out->left = first->left > second->left ? first->left : second->left;
	out->right = first->right < second->right ? first->right : second->right;
	if ( out->left >= out->right )
		return 0;
	out->top = first->top > second->top ? first->top : second->top;
	out->bottom = first->bottom < second->bottom ? first->bottom : second->bottom;
	return out->top < out->bottom;
}

int AdjustSubRect( mspriteframe_t* frame, int* width, int* height, const wrect_t* rect )
{
	if ( !ValidateWRect( rect ) )
		return 0;
	int left = rect->left < 0 ? 0 : rect->left;
	int right = rect->right < *width ? rect->right : *width;
	if ( left >= right )
		return 0;
	int top = rect->top < 0 ? 0 : rect->top;
	int bottom = rect->bottom < *height ? rect->bottom : *height;
	if ( top >= bottom )
		return 0;
	*width = right - left;
	*height = bottom - top;
	return left + top * frame->width;
}

int SpriteFrameClip( mspriteframe_t* frame, int* x, int* y, int* width, int* height, const wrect_t* rect )
{
	if ( !frame )
		return 0;
	*width = frame->width;
	*height = frame->height;
	int offset = 0;
	if ( rect )
	{
		offset = AdjustSubRect( frame, width, height, rect );
	}
	else if ( giScissorTest )
	{
		if ( *x < scissor_x )
		{
			int clipped = scissor_x - *x;
			if ( clipped > *width )
				clipped = *width;
			*width -= clipped;
			offset = scissor_x - *x;
			*x = scissor_x;
		}
		if ( *y < scissor_y )
		{
			int clipped = scissor_y - *y;
			if ( clipped > *height )
				clipped = *height;
			*height -= clipped;
			offset += frame->width * ( scissor_y - *y );
			*y = scissor_y;
		}
		if ( *y + *height > scissor_y2 )
			*height = scissor_y2 - *y < 0 ? 0 : scissor_y2 - *y;
		if ( *x + *width > scissor_x2 )
			*width = scissor_x2 - *x < 0 ? 0 : scissor_x2 - *x;
	}
	if ( *x < 0 )
	{
		*width += *x;
		offset = -*x;
		*x = 0;
	}
	if ( *y < 0 )
	{
		*height += *y;
		offset -= frame->width * *y;
		*y = 0;
	}
	if ( *y + *height > (int)vid.height )
		*height = vid.height - *y;
	if ( *x + *width > (int)vid.width )
		*width = vid.width - *x;
	if ( *height < 0 )
		*height = 0;
	return offset;
}

void Draw_SpriteFrame( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect )
{
	int width, height;
	byte* source = frame->pixels + SpriteFrameClip( frame, &x, &y, &width, &height, rect );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	while ( height-- )
	{
		for ( int col = 0; col < width; ++col )
			dest[col] = palette[source[col]];
		dest += vid.rowbytes >> 1;
		source += frame->width;
	}
}

void Draw_SpriteFrameHoles( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect )
{
	int width, height;
	byte* source = frame->pixels + SpriteFrameClip( frame, &x, &y, &width, &height, rect );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	while ( height-- )
	{
		for ( int col = 0; col < width; ++col )
		{
			if ( source[col] != 255 )
				dest[col] = palette[source[col]];
		}
		dest += vid.rowbytes >> 1;
		source += frame->width;
	}
}

void Draw_SpriteFrameAdditive( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect )
{
	int width, height;
	byte* source = frame->pixels + SpriteFrameClip( frame, &x, &y, &width, &height, rect );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	while ( height-- )
	{
		for ( int col = 0; col < width; ++col )
		{
			word color = palette[source[col]];
			if ( !color )
				continue;
			if ( is15bit )
			{
				unsigned int sum = ( ( (unsigned int)dest[col] << 16 | dest[col] ) & COLOR_PACKEDMASK_555 )
					+ ( ( ( color & COLOR_REDBLUEMASK_555 ) << 16 ) | ( color & COLOR_GREENMASK_555 ) );
				if ( sum & 0x80200400 )
					sum |= 0x80200400 - ( ( sum & 0x80200400 ) >> 5 );
				dest[col] = (word)( ( ( sum >> 16 ) & COLOR_REDBLUEMASK_555 ) | ( sum & COLOR_GREENMASK_555 ) );
			}
			else
			{
				unsigned int background = ( (unsigned int)dest[col] << 16 | dest[col] ) & COLOR_PACKEDMASK_565;
				unsigned int sum = background
						+ ( ( (unsigned int)( color & COLOR_REDBLUEMASK_565 ) << 16 ) | ( color & COLOR_GREENMASK_565 ) );
				unsigned int overflow = sum & 0x00200800;
				if ( background > sum )
					overflow |= 1;
				if ( overflow )
				{
					overflow = ( overflow >> 1 ) | ( overflow << 31 );
					sum |= 2 * ( ( overflow | 0x00100400 ) - ( overflow >> 5 ) );
				}
				dest[col] = (word)( ( ( sum >> 16 ) & COLOR_REDBLUEMASK_565 ) | ( sum & COLOR_GREENMASK_565 ) );
			}
		}
		dest += vid.rowbytes >> 1;
		source += frame->width;
	}
}

void Draw_SpriteFrameGeneric( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t*,
	int, int, int screenWidth, int screenHeight )
{
	float rows = ceilf( (float)( (double)screenHeight / frame->height ) );
	float cols = ceilf( (float)( (double)screenWidth / frame->width ) );
	int numRows = (int)rows;
	int numCols = (int)cols;
	int clipHeight = screenHeight % frame->height;
	int clipWidth = screenWidth % frame->width;
	int destY = y;
	wrect_t subRect;
	subRect.left = 0;
	subRect.top = 0;
	for ( int row = 0; row < numRows; ++row )
	{
		int destX = x;
		for ( int col = 0; col < numCols; ++col )
		{
			subRect.right = col == numCols - 1 ? clipWidth : frame->width;
			subRect.bottom = row == numRows - 1 ? clipHeight : frame->height;
			int width, height;
			byte* source = frame->pixels + SpriteFrameClip( frame, &x, &y, &width, &height, &subRect );
			word* dest = (word*)vid.buffer + destX + destY * ( vid.rowbytes >> 1 );
			while ( height-- )
			{
				for ( int pixel = 0; pixel < width; ++pixel )
				{
					word color = palette[source[pixel]];
					if ( !color )
						continue;
					if ( is15bit )
					{
						unsigned int sum = ( ( (unsigned int)dest[pixel] << 16 | dest[pixel] ) & COLOR_PACKEDMASK_555 )
							+ ( ( ( color & COLOR_REDBLUEMASK_555 ) << 16 ) | ( color & COLOR_GREENMASK_555 ) );
						if ( sum & 0x80200400 )
							sum |= 0x80200400 - ( ( sum & 0x80200400 ) >> 5 );
						dest[pixel] = (word)( ( ( sum >> 16 ) & COLOR_REDBLUEMASK_555 )
								| ( sum & COLOR_GREENMASK_555 ) );
					}
					else
					{
						unsigned int background = ( (unsigned int)dest[pixel] << 16 | dest[pixel] ) & COLOR_PACKEDMASK_565;
						unsigned int sum = background
							+ ( ( (unsigned int)( color & COLOR_REDBLUEMASK_565 ) << 16 ) | ( color & COLOR_GREENMASK_565 ) );
						unsigned int overflow = sum & 0x00200800;
						if ( background > sum )
							overflow |= 1;
						if ( overflow )
						{
							overflow = ( overflow >> 1 ) | ( overflow << 31 );
							sum |= 2 * ( ( overflow | 0x00100400 ) - ( overflow >> 5 ) );
						}
						dest[pixel] = (word)( ( ( sum >> 16 ) & COLOR_REDBLUEMASK_565 )
								| ( sum & COLOR_GREENMASK_565 ) );
					}
				}
				dest += vid.rowbytes >> 1;
				source += frame->width;
			}
			destX += frame->width;
		}
		destY += frame->height;
	}
}

void Draw_FillRGBA( int x, int y, int width, int height, int r, int g, int b, int alpha )
{
	g_engdstAddrs.pfnFillRGBA( &x, &y, &width, &height, &r, &g, &b, &alpha );
	int left = x < 0 ? 0 : x;
	int right = x + width > (int)vid.width ? vid.width : x + width;
	int top = y < 0 ? 0 : y;
	int bottom = y + height > (int)vid.height ? vid.height : y + height;
	if ( left >= right || top >= bottom )
		return;
	if ( giScissorTest )
	{
		if ( left < scissor_rect.left )
			left = scissor_rect.left;
		if ( right > scissor_rect.right )
			right = scissor_rect.right;
		if ( left >= right )
			return;
		if ( top < scissor_rect.top )
			top = scissor_rect.top;
		if ( bottom > scissor_rect.bottom )
			bottom = scissor_rect.bottom;
		if ( top >= bottom )
			return;
	}
	int tableOffset = ( COLOR_LIGHTSCALE_BYTE * alpha ) & COLOR_LIGHTMASK;
	word color = red_64klut[tableOffset + r] | green_64klut[tableOffset + g] | blue_64klut[tableOffset + b];
	word* dest = (word*)vid.buffer + left + top * ( vid.rowbytes >> 1 );
	int redMask = is15bit ? COLOR_REDMASK_555 : COLOR_REDMASK_565;
	int greenMask = is15bit ? COLOR_GREENMASK_555 : COLOR_GREENMASK_565;
	for ( int row = top; row < bottom; ++row )
	{
		for ( int col = 0; col < right - left; ++col )
		{
			r = ( color & redMask ) + ( dest[col] & redMask );
			g = ( color & greenMask ) + ( dest[col] & greenMask );
			b = ( color & COLOR_BLUEMASK ) + ( dest[col] & COLOR_BLUEMASK );
			if ( r > redMask )
				r = redMask;
			if ( g > greenMask )
				g = greenMask;
			if ( b > 31 )
				b = 31;
			dest[col] = (word)( r | g | b );
		}
		dest += vid.rowbytes >> 1;
	}
}

void Draw_FillRGBABlend( int x, int y, int width, int height, int r, int g, int b, int alpha )
{
	int left = x < 0 ? 0 : x;
	int right = x + width > (int)vid.width ? vid.width : x + width;
	int top = y < 0 ? 0 : y;
	int bottom = y + height > (int)vid.height ? vid.height : y + height;
	if ( left >= right || top >= bottom )
		return;
	if ( giScissorTest )
	{
		if ( left < scissor_rect.left )
			left = scissor_rect.left;
		if ( right > scissor_rect.right )
			right = scissor_rect.right;
		if ( left >= right )
			return;
		if ( top < scissor_rect.top )
			top = scissor_rect.top;
		if ( bottom > scissor_rect.bottom )
			bottom = scissor_rect.bottom;
		if ( top >= bottom )
			return;
	}
	if ( !alpha )
		return;
	int tableOffset = ( COLOR_LIGHTSCALE_BYTE * alpha ) & COLOR_LIGHTMASK;
	word color = red_64klut[tableOffset + r] | green_64klut[tableOffset + g] | blue_64klut[tableOffset + b];
	int inverse = 255 - alpha;
	int redMask = is15bit ? COLOR_REDMASK_555 : COLOR_REDMASK_565;
	int greenMask = is15bit ? COLOR_GREENMASK_555 : COLOR_GREENMASK_565;
	word* dest = (word*)vid.buffer + left + top * ( vid.rowbytes >> 1 );
	for ( int row = top; row < bottom; ++row )
	{
		for ( int col = 0; col < right - left; ++col )
		{
			int background = ( ( ( inverse * ( dest[col] & redMask ) ) >> 8 ) & redMask )
				| ( ( ( inverse * ( dest[col] & greenMask ) ) >> 8 ) & greenMask )
				| ( ( ( inverse * ( dest[col] & COLOR_BLUEMASK ) ) >> 8 ) & COLOR_BLUEMASK );
			dest[col] = (word)( color + background );
		}
		dest += vid.rowbytes >> 1;
	}
}

void Draw_FillRGB( int x, int y, int width, int height, int r, int g, int b )
{
	int left = x < 0 ? 0 : x;
	int right = x + width > (int)vid.width ? vid.width : x + width;
	int top = y < 0 ? 0 : y;
	int bottom = y + height > (int)vid.height ? vid.height : y + height;
	if ( left >= right || top >= bottom )
		return;
	if ( giScissorTest )
	{
		if ( left < scissor_rect.left )
			left = scissor_rect.left;
		if ( right > scissor_rect.right )
			right = scissor_rect.right;
		if ( left >= right )
			return;
		if ( top < scissor_rect.top )
			top = scissor_rect.top;
		if ( bottom > scissor_rect.bottom )
			bottom = scissor_rect.bottom;
		if ( top >= bottom )
			return;
	}
	colorVec color;
	color.r = r;
	color.g = g;
	color.b = b;
	word* dest = (word*)vid.buffer + left + top * ( vid.rowbytes >> 1 );
	for ( int row = top; row < bottom; ++row )
	{
		for ( int col = 0; col < right - left; ++col )
			dest[col] = PutRGB( &color );
		dest += vid.rowbytes >> 1;
	}
}

void Draw_LineRGB( int x1, int y1, int x2, int y2, int r, int g, int b )
{
	int xinc = x2 >= x1 ? 1 : -1;
	int yinc = (int)( vid.rowbytes >> 1 );
	if ( y2 < y1 )
		yinc = -yinc;
	int left = x1 < x2 ? x1 : x2;
	int right = ( x1 > x2 ? x1 : x2 ) + 1;
	int top = y1 < y2 ? y1 : y2;
	int bottom = ( y1 > y2 ? y1 : y2 ) + 1;
	if ( left < 0 )
		left = 0;
	if ( right > (int)vid.width )
		right = vid.width;
	if ( top < 0 )
		top = 0;
	if ( bottom > (int)vid.height )
		bottom = vid.height;
	if ( left >= right || top >= bottom )
		return;
	if ( giScissorTest )
	{
		if ( left < scissor_rect.left )
			left = scissor_rect.left;
		if ( right > scissor_rect.right )
			right = scissor_rect.right;
		if ( left >= right )
			return;
		if ( top < scissor_rect.top )
			top = scissor_rect.top;
		if ( bottom > scissor_rect.bottom )
			bottom = scissor_rect.bottom;
		if ( top >= bottom )
			return;
	}
	int dx = abs( x2 - x1 );
	int dy = abs( y2 - y1 );
	colorVec color;
	color.r = r;
	color.g = g;
	color.b = b;
	word* dest = (word*)vid.buffer + x1 + y1 * ( vid.rowbytes >> 1 );
	if ( dx < dy )
	{
		int error = 2 * dx - dy;
		for ( int i = 0; i <= dy; ++i )
		{
			*dest = PutRGB( &color );
			dest += yinc;
			if ( error > 0 )
			{
				dest += xinc;
				error += 2 * dx - 2 * dy;
			}
			else
			{
				error += 2 * dx;
			}
		}
	}
	else
	{
		int error = 2 * dy - dx;
		for ( int i = 0; i <= dx; ++i )
		{
			*dest = PutRGB( &color );
			dest += xinc;
			if ( error > 0 )
			{
				dest += yinc;
				error += 2 * dy - 2 * dx;
			}
			else
			{
				error += 2 * dy;
			}
		}
	}
}

void Draw_ScaledRGBAImage( int x, int y, int width, int height, int srcx, int srcy,
	int srcw, int srch, int texw, int, byte* rgba )
{
	if ( !rgba || x > (int)vid.width || y > (int)vid.height || !width || !height )
		return;
	int uClip = 0;
	if ( x + width > (int)vid.width )
	{
		width = vid.width - x;
	}
	else if ( x < 0 )
	{
		if ( x + width < 0 )
			return;
		uClip = -x;
	}
	int vClip = 0;
	if ( y + height > (int)vid.height )
	{
		height = vid.height - y;
	}
	else if ( y < 0 )
	{
		if ( y + height < 0 )
			return;
		vClip = -y;
	}
	float uScale = (float)srcw / (float)width;
	float vScale = (float)srch / (float)height;
	int uOffset = (int)( 1.0f / uScale * (float)uClip );
	int vOffset = (int)( 1.0f / vScale * (float)vClip );
	word* dest = (word*)vid.buffer + x + uClip + ( y + vClip ) * ( vid.rowbytes >> 1 );
	int uStep = (int)( uScale * 256.0 );
	for ( int row = vOffset; row < height; ++row )
	{
		int u = uStep * uOffset;
		for ( int col = 0; col < width - uOffset; ++col )
		{
			float v = (float)row * vScale;
			byte* source = rgba + 4 * ( srcx + texw * srcy + ( u >> 8 ) + texw * (int)v );
			if ( *(unsigned int*)source && source[3] )
				dest[col] = RGBToPacked( source );
			u += uStep;
		}
		dest += vid.rowbytes >> 1;
	}
}

void Draw_RGBAImage( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int,
	byte* rgba, int istext, byte red, byte green, byte blue, byte alpha )
{
	if ( x < 0 || !alpha || !rgba || x + srcw > (int)vid.width || y < 0 || y + srch > (int)vid.height )
		return;
	int offset = 0;
	if ( giScissorTest )
	{
		if ( x < scissor_x )
		{
			int clipped = scissor_x - x;
			if ( clipped >= srcw )
				clipped = srcw;
			srcw -= clipped;
			offset = 4 * ( scissor_x - x );
			x = scissor_x;
		}
		if ( y < scissor_y )
		{
			int clipped = scissor_y - y;
			if ( clipped >= srch )
				clipped = srch;
			srch -= clipped;
			offset += 4 * texw * ( scissor_y - y );
			y = scissor_y;
		}
		if ( y + srch > scissor_y2 )
			srch = scissor_y2 - y < 0 ? 0 : scissor_y2 - y;
		if ( x + srcw > scissor_x2 )
			srcw = scissor_x2 - x < 0 ? 0 : scissor_x2 - x;
	}
	if ( !srcw || !srch )
		return;
	byte* source = rgba + 4 * ( srcx + srcy * texw ) + offset;
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	if ( alpha == 255 )
	{
		if ( istext )
		{
			colorVec color;
			color.r = red;
			color.g = green;
			color.b = blue;
			word packed = PutRGB( &color );
			for ( int row = 0; row < srch; ++row )
			{
				for ( int col = 0; col < srcw; ++col )
				{
					if ( *(unsigned int*)( source + col * 4 ) )
						dest[col] = packed;
				}
				dest += vid.rowbytes >> 1;
				source += texw * 4;
			}
		}
		else
		{
			for ( int row = 0; row < srch; ++row )
			{
				for ( int col = 0; col < srcw; ++col )
				{
					byte* pixel = source + col * 4;
					if ( *(unsigned int*)pixel && pixel[3] )
						dest[col] = RGBToPacked( pixel );
				}
				dest += vid.rowbytes >> 1;
				source += texw * 4;
			}
		}
		return;
	}
	for ( int row = 0; row < srch; ++row )
	{
		for ( int col = 0; col < srcw; ++col )
		{
			byte* pixel = source + col * 4;
			if ( !*(unsigned int*)pixel )
				continue;
			int coverage = ( alpha * pixel[3] ) >> 8;
			int inverse = 256 - coverage;
			int r = inverse * ( dest[col] >> 11 );
			int g = inverse * ( ( dest[col] >> 5 ) & 0x3F );
			int b = inverse * ( dest[col] & 0x1F );
			if ( istext )
			{
				r = ( r + coverage * ( red >> 3 ) ) >> 8;
				g = ( g + coverage * ( green >> 2 ) ) >> 8;
				b = ( b + coverage * ( blue >> 3 ) ) >> 8;
			}
			else
			{
				r = ( r + coverage * ( pixel[0] >> 3 ) ) >> 8;
				g = ( g + coverage * ( pixel[1] >> 2 ) ) >> 8;
				b = ( b + coverage * ( pixel[2] >> 3 ) ) >> 8;
			}
			dest[col] = (word)( b | ( g << 5 ) | ( r << 11 ) );
		}
		dest += vid.rowbytes >> 1;
		source += texw * 4;
	}
}

void Draw_RGBAImageAdd15( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int,
	byte* rgba, int, byte red, byte green, byte blue, byte alpha )
{
	if ( !alpha || !rgba || x < 0 || x + srcw > (int)vid.width || y < 0 || y + srch > (int)vid.height )
		return;
	byte* source = rgba + 4 * ( srcx + texw * srcy );
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	while ( srch-- )
	{
		for ( int col = 0; col < srcw; ++col )
		{
			byte* pixel = source + col * 4;
			if ( !pixel[3] )
				continue;
			int tableOffset = ( COLOR_LIGHTSCALE_BYTE * pixel[3] ) & COLOR_LIGHTMASK;
			word color = red_64klut[tableOffset + ( ( red * pixel[0] ) >> 8 )]
				| green_64klut[tableOffset + ( ( green * pixel[1] ) >> 8 )]
				| blue_64klut[tableOffset + ( ( blue * pixel[2] ) >> 8 )];
			if ( !color )
				continue;
			int r = ( color & COLOR_REDMASK_555 ) + ( dest[col] & COLOR_REDMASK_555 );
			int g = ( color & COLOR_GREENMASK_555 ) + ( dest[col] & COLOR_GREENMASK_555 );
			int b = ( color & COLOR_BLUEMASK ) + ( dest[col] & COLOR_BLUEMASK );
			if ( r > COLOR_REDMASK_555 )
				r = COLOR_REDMASK_555;
			if ( g > COLOR_GREENMASK_555 )
				g = COLOR_GREENMASK_555;
			if ( b > 31 )
				b = 31;
			dest[col] = (word)( r | g | b );
		}
		dest += vid.rowbytes >> 1;
		source += 4 * texw;
	}
}

void Draw_RGBAImageAdd16( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
	byte* rgba, int, byte red, byte green, byte blue, byte alpha )
{
	if ( x > (int)vid.width || y > (int)vid.height || !texw || !texh )
		return;
	int uOffset = 0;
	int vOffset = 0;
	if ( x < 0 )
	{
		if ( x + srcw < 0 )
			return;
		uOffset = -x;
	}
	if ( y < 0 )
	{
		if ( y + srch < 0 )
			return;
		vOffset = -y;
	}
	int maxWidth = x + srcw > (int)vid.width ? vid.width - x : srcw;
	int maxHeight = y + srch > (int)vid.height ? vid.height - y : srch;
	if ( !rgba || !alpha )
		return;
	byte* source = rgba + 4 * ( uOffset + srcx + texw * ( vOffset + srcy ) );
	word* dest = (word*)vid.buffer + x + uOffset + ( y + vOffset ) * ( vid.rowbytes >> 1 );
	int remaining = maxHeight - 1;
	if ( maxHeight == 0 || vOffset >= remaining )
		return;
	do
	{
		for ( int col = 0; col < maxWidth - uOffset; ++col )
		{
			byte* pixel = source + col * 4;
			if ( !pixel[3] )
				continue;
			int tableOffset = ( COLOR_LIGHTSCALE_BYTE * pixel[3] ) & COLOR_LIGHTMASK;
			word color = red_64klut[tableOffset + ( ( red * pixel[0] ) >> 8 )]
				| green_64klut[tableOffset + ( ( green * pixel[1] ) >> 8 )]
				| blue_64klut[tableOffset + ( ( blue * pixel[2] ) >> 8 )];
			if ( !color )
				continue;
			int r = ( color & COLOR_REDMASK_565 ) + ( dest[col] & COLOR_REDMASK_565 );
			int g = ( color & COLOR_GREENMASK_565 ) + ( dest[col] & COLOR_GREENMASK_565 );
			int b = ( color & COLOR_BLUEMASK ) + ( dest[col] & COLOR_BLUEMASK );
			if ( r > COLOR_REDMASK_565 )
				r = COLOR_REDMASK_565;
			if ( g > COLOR_GREENMASK_565 )
				g = COLOR_GREENMASK_565;
			if ( b > 31 )
				b = 31;
			dest[col] = (word)( r | g | b );
		}
		--remaining;
		dest += vid.rowbytes >> 1;
		source += texw * 4;
	}
	while ( vOffset < remaining && remaining != -1 );
}

void Draw_RGBAImageAdditive( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
	byte* rgba, int istext, byte red, byte green, byte blue, byte alpha )
{
	if ( is15bit )
		Draw_RGBAImageAdd15( x, y, srcx, srcy, srcw, srch, texw, texh, rgba, istext, red, green, blue, alpha );
	else
		Draw_RGBAImageAdd16( x, y, srcx, srcy, srcw, srch, texw, texh, rgba, istext, red, green, blue, alpha );
}

/*
==============
R_DrawRect16
==============
*/
void R_DrawRect16( vrect_t* rect, int rowbytes, byte* source, byte* palette, int transparent )
{
	word* dest = (word*)vid.buffer + rect->x + rect->y * ( vid.rowbytes >> 1 );
	for ( int row = 0; row < rect->height; ++row )
	{
		for ( int col = 0; col < rect->width; ++col )
		{
			if ( !transparent || source[col] != 255 )
				dest[col] = PackedRGB( palette, source[col] );
		}
		source += rowbytes;
		dest += vid.rowbytes >> 1;
	}
}

void Draw_TileClear( int, int, int, int )
{
}

void Draw_Fill( int x, int y, int width, int height, int color )
{
	word* dest = (word*)vid.buffer + x + y * ( vid.rowbytes >> 1 );
	for ( int row = 0; row < height; ++row )
	{
		for ( int col = 0; col < width; ++col )
			dest[col] = (word)color;
		dest += vid.rowbytes >> 1;
	}
}

/*
================
Draw_FadeScreen

================
*/
void Draw_FadeScreen()
{
	S_ExtraUpdate();
	for ( int y = 0; y < (int)vid.height; ++y )
	{
		word* dest = (word*)( vid.buffer + y * vid.rowbytes );
		for ( int x = 0; x < (int)vid.width; ++x )
			dest[x] = ( dest[x] & ( is15bit ? 0xEADE : 0xF7DE ) ) >> 1;
	}
	S_ExtraUpdate();
}

void Draw_BeginDisc()
{
}

void Draw_EndDisc()
{
}
