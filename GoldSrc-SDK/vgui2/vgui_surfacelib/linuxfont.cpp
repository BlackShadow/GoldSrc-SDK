#if defined( _WIN32 )
#define UTLRBTREE_SOURCE_PATH L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\public\\UtlRBTree.h"
#endif

#include "LinuxFont.h"

#include "FontManager.h"

#if !defined( _WIN32 )
#include <ft2build.h>
#include FT_FREETYPE_H
#include <fontconfig/fontconfig.h>
#endif

#include <cassert>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>

#if defined( _WIN32 )
#include <windows.h>
#include <malloc.h>
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef GetCharABCWidths
#undef GetCharABCWidths
#endif
#define strcasecmp _stricmp
#endif

#if defined( _WIN32 )
extern "C" _ACRTIMP void __cdecl _wassert( const wchar_t* message, const wchar_t* file, unsigned int line );

#define WIN32FONT_SOURCE_PATH L"c:\\buildslave\\goldsrc_win32\\build\\GoldSrc\\vgui2\\vgui_surfacelib\\Win32Font.cpp"

namespace
{
bool g_bWindowsFontVersionInitialized;
bool g_bWindowsFontSupportsUnicode;
bool g_bWindowsFontFamilyFound;
}
#endif

#define FONTFLAG_ITALIC 0x001
#define FONTFLAG_UNDERLINE 0x002
#define FONTFLAG_STRIKEOUT 0x004
#define FONTFLAG_SYMBOL 0x008
#define FONTFLAG_ANTIALIAS 0x010
#define FONTFLAG_ROTARY 0x040
#define FONTFLAG_DROPSHADOW 0x080
#define FONTFLAG_ADDITIVE 0x100

struct font_scale_t
{
	const char* font;
	float scale;
};

#if !defined( _WIN32 )
static font_scale_t g_FontScales[3] = { { "DejaVu Sans", 0.875f }, { "Liberation Sans", 0.9375f }, { "Fira Sans", 0.875f } };

CUtlRBTree<CLinuxFont::font_name_entry> CLinuxFont::m_FriendlyNameCache( 0, 0, &CLinuxFont::FontEntryLess );
bool CLinuxFont::ms_bSetFriendlyNameCacheLessFunc = false;
#endif

#if !defined( _WIN32 )
static int Fixed26Dot6ToInt( int value )
{
	int rounded = value >> 6;
	if ( value & 0x20 )
		rounded += ( value >> 31 ) | 1;
	return rounded;
}

static int RoundHalfBBox( int xMax )
{
	int adjusted = xMax + (int)( (unsigned int)xMax >> 31 );
	int rounded = adjusted >> 7;
	if ( ( adjusted >> 1 ) & 0x20 )
		rounded += ( adjusted >> 31 ) | 1;
	return rounded;
}

static char LowByteDifference( int rounded, unsigned char first, unsigned char second )
{
	return (char)( (char)rounded - first - second );
}

static char LowByteDifference( int rounded, unsigned char first, unsigned char second, unsigned char third )
{
	return (char)( (char)rounded - first - second - third );
}

static FcPattern* FontMatch( const char* object, FcType type, const void* value, ... )
{
	va_list arguments;
	FcPattern* pattern;
	FcPattern* match;
	FcValue fcValue;
	FcResult result;

	va_start( arguments, value );
	pattern = FcPatternCreate();
	for ( ;; )
	{
		fcValue.type = type;
		fcValue.u.s = NULL;
		if ( type == FcTypeInteger )
			fcValue.u.i = (int)(intptr_t)value;
		else if ( type == FcTypeString )
			fcValue.u.s = (const FcChar8*)value;
		FcPatternAdd( pattern, object, fcValue, 0 );

		object = va_arg( arguments, const char* );
		if ( !object )
			break;
		type = (FcType)va_arg( arguments, int );
		value = va_arg( arguments, const void* );
	}
	va_end( arguments );

	FcConfigSubstitute( NULL, pattern, FcMatchPattern );
	FcDefaultSubstitute( pattern );
	result = FcResultMatch;
	match = FcFontMatch( NULL, pattern, &result );
	FcPatternDestroy( pattern );
	return match;
}
#endif

bool CLinuxFont::ExtendedABCWidthsCacheLessFunc( const abc_cache_t& lhs, const abc_cache_t& rhs )
{
	return lhs.wch < rhs.wch;
}

#if !defined( _WIN32 )
bool CLinuxFont::FontEntryLess( const font_name_entry& a, const font_name_entry& b )
{
	return strcasecmp( b.m_pchFriendlyName, a.m_pchFriendlyName ) > 0;
}
#endif

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CLinuxFont::CLinuxFont() : m_ExtendedABCWidthsCache( 256, 0, &CLinuxFont::ExtendedABCWidthsCacheLessFunc )
{
	m_szName[0] = 0;
	m_iTall = 0;
	m_iWeight = 0;
	m_iFlags = 0;
	m_iMaxCharWidth = 0;
	m_bAntiAliased = false;
	m_bUnderlined = false;
	m_iBlur = 0;
	m_pGaussianDistribution = NULL;
	m_iScanLines = 0;
	m_bRotary = false;
	m_bAdditive = false;
#if defined( _WIN32 )
	m_hFont = NULL;
	m_hDC = NULL;
	m_hDIB = NULL;
	if ( !g_bWindowsFontVersionInitialized )
	{
		g_bWindowsFontVersionInitialized = true;
		OSVERSIONINFOA version = {};
		version.dwOSVersionInfoSize = sizeof( version );
#if defined( _MSC_VER )
#pragma warning( suppress : 4996 28159 )
		GetVersionExA( &version );
#else
		GetVersionExA( &version );
#endif
		g_bWindowsFontSupportsUnicode = version.dwMajorVersion >= 5;
	}
#else
	m_pBuf = NULL;
	face = NULL;
	ms_bSetFriendlyNameCacheLessFunc = true;
#endif
}

CLinuxFont::~CLinuxFont()
{
#if defined( _WIN32 )
	if ( m_hFont )
		DeleteObject( static_cast<HGDIOBJ>( m_hFont ) );
	if ( m_hDC )
		DeleteDC( static_cast<HDC>( m_hDC ) );
	if ( m_hDIB )
		DeleteObject( static_cast<HGDIOBJ>( m_hDIB ) );
#else
	if ( face )
		FT_Done_Face( face );
	delete[] m_pGaussianDistribution;
#endif
}

#if defined( _WIN32 )
static int CALLBACK FontFamilyFound( const LOGFONTA*, const TEXTMETRICA*, DWORD, LPARAM )
{
	g_bWindowsFontFamilyFound = true;
	return 0;
}

bool CLinuxFont::CreateWindowsFont( const char* name, int tall, int weight, int flags )
{
	static int runningUnderWine = -1;
	if ( runningUnderWine == -1 )
	{
		runningUnderWine = 0;
		HMODULE module = GetModuleHandleA( "ntdll.dll" );
		if ( module != NULL && GetProcAddress( module, "wine_get_version" ) != NULL )
			runningUnderWine = 1;
	}
	m_bAntiAliased = runningUnderWine != 0 || ( flags & FONTFLAG_ANTIALIAS ) != 0;

	int charset = ( flags & FONTFLAG_SYMBOL ) ? SYMBOL_CHARSET : ANSI_CHARSET;
	m_bUnderlined = ( flags & FONTFLAG_UNDERLINE ) != 0;
	m_iDropShadowOffset = ( flags & FONTFLAG_DROPSHADOW ) != 0;
	m_bRotary = ( flags & FONTFLAG_ROTARY ) != 0;
	m_bAdditive = ( flags & FONTFLAG_ADDITIVE ) != 0;
	if ( !strcasecmp( name, "win98japanese" ) )
	{
		charset = SHIFTJIS_CHARSET;
		strncpy( m_szName, "Tahoma", sizeof( m_szName ) );
	}

	m_hDC = CreateCompatibleDC( NULL );
	HDC dc = static_cast<HDC>( m_hDC );
	LOGFONTA requested = {};
	requested.lfCharSet = DEFAULT_CHARSET;
	requested.lfPitchAndFamily = 0;
	strcpy( requested.lfFaceName, m_szName );
	g_bWindowsFontFamilyFound = false;
	EnumFontFamiliesExA( dc, &requested, FontFamilyFound, 0, 0 );
	if ( !g_bWindowsFontFamilyFound )
		return false;

	m_hFont = CreateFontA( tall, 0, 0, 0, weight, flags & FONTFLAG_ITALIC, flags & FONTFLAG_UNDERLINE,
		flags & FONTFLAG_STRIKEOUT, charset, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
		m_bAntiAliased ? ANTIALIASED_QUALITY : NONANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name );
	if ( !m_hFont )
		return false;

	SetMapMode( dc, MM_TEXT );
	SelectObject( dc, static_cast<HFONT>( m_hFont ) );
	SetTextAlign( dc, TA_LEFT | TA_TOP | TA_UPDATECP );

	TEXTMETRICA tm = {};
	GetTextMetricsA( dc, &tm );
	m_iHeight = tm.tmHeight + m_iDropShadowOffset;
	m_iAscent = tm.tmAscent;
	m_iMaxCharWidth = tm.tmMaxCharWidth;
	m_rgiBitmapSize[0] = tm.tmMaxCharWidth;
	m_rgiBitmapSize[1] = m_iHeight;

	BITMAPINFO bitmapInfo = {};
	bitmapInfo.bmiHeader.biSize = sizeof( bitmapInfo.bmiHeader );
	bitmapInfo.bmiHeader.biWidth = m_rgiBitmapSize[0];
	bitmapInfo.bmiHeader.biHeight = -m_rgiBitmapSize[1];
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;
	m_hDIB = CreateDIBSection( dc, &bitmapInfo, DIB_RGB_COLORS, (void**)&m_pBuf, NULL, 0 );
	SelectObject( dc, static_cast<HBITMAP>( m_hDIB ) );

	memset( m_ABCWidths, 0, sizeof( m_ABCWidths ) );
	ABC widths[ASCII_CHAR_COUNT];
	if ( GetCharABCWidthsW( dc, 0, ASCII_CHAR_COUNT - 1, widths )
		|| GetCharABCWidthsA( dc, 0, ASCII_CHAR_COUNT - 1, widths ) )
	{
		for ( int ch = 0; ch < ASCII_CHAR_COUNT; ++ch )
		{
			m_ABCWidths[ch].a = static_cast<signed char>( widths[ch].abcA - m_iBlur );
			m_ABCWidths[ch].b = static_cast<signed char>( widths[ch].abcB + 2 * m_iBlur + m_iDropShadowOffset );
			m_ABCWidths[ch].c = static_cast<signed char>( widths[ch].abcC - m_iBlur - m_iDropShadowOffset );
		}
	}
	else
	{
		for ( int ch = 0; ch < ASCII_CHAR_COUNT; ++ch )
			m_ABCWidths[ch].b = static_cast<signed char>( tm.tmAveCharWidth );
	}
	return true;
}
#endif

#if !defined( _WIN32 )
bool CLinuxFont::FindFontDataFile(
	const char* windowsFontName, int, int weight, int, int, int flags, char* dataFile, int dataFileChars )
{
	const char* family = windowsFontName;
	FcChar8* filename;
	FcPattern* pattern;
	bool found = false;

	if ( FcInit() )
	{
		int fontWeight = FC_WEIGHT_REGULAR;
		if ( weight > 699 )
			fontWeight = FC_WEIGHT_BOLD;

		if ( !strcasecmp( windowsFontName, "Tahoma" ) )
			family = "Bitstream Vera Sans";

		int slant = FC_SLANT_ROMAN;
		if ( flags & FONTFLAG_ITALIC )
			slant = FC_SLANT_ITALIC;

		pattern = FontMatch( "family", FcTypeString, family, "weight", FcTypeInteger, (const void*)(intptr_t)fontWeight,
			"slant", FcTypeInteger, (const void*)(intptr_t)slant, nullptr );
		if ( pattern )
		{
			filename = NULL;
			if ( FcPatternGetString( pattern, "file", 0, &filename ) == FcResultMatch )
			{
				found = true;
				strncpy( dataFile, (const char*)filename, (size_t)dataFileChars );
			}
			FcPatternDestroy( pattern );
		}
	}

	return found;
}
#endif

bool CLinuxFont::Create( const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags )
{
	int i;

#if defined( _WIN32 )
	strncpy( m_szName, windowsFontName, sizeof( m_szName ) - 1 );
	m_iWeight = weight;
	m_szName[sizeof( m_szName ) - 1] = 0;
	m_iTall = tall;
	m_iFlags = flags;
	m_iBlur = blur;
	m_iScanLines = scanlines;

	if ( !CreateWindowsFont( windowsFontName, tall, weight, flags ) )
	{
		m_szName[0] = 0;
		return false;
	}
#else
	float scale = 1.0f;

	for ( i = 0; i < 3; i++ )
	{
		if ( !strcasecmp( g_FontScales[i].font, windowsFontName ) )
			scale = g_FontScales[i].scale;
	}

	strncpy( m_szName, windowsFontName, sizeof( m_szName ) - 1 );
	m_szName[sizeof( m_szName ) - 1] = 0;
	m_iFlags = flags;
	m_iOutlineSize = 0;
	m_iTall = tall;
	m_iWeight = weight;
	m_bAntiAliased = ( flags & FONTFLAG_ANTIALIAS ) != 0;
	m_bUnderlined = ( flags & FONTFLAG_UNDERLINE ) != 0;
	m_iDropShadowOffset = ( flags & FONTFLAG_DROPSHADOW ) != 0;
	m_iFxpHeight = 0;
	m_iBlur = blur;
	m_iScanLines = scanlines;
	m_bRotary = ( flags & FONTFLAG_ROTARY ) != 0;
	m_iHeight = (int)( (float)tall * scale );
	m_bAdditive = ( flags & FONTFLAG_ADDITIVE ) != 0;

	if ( !FcInit() )
	{
		printf( "Unable to find font named %s\n", windowsFontName );
		m_szName[0] = 0;
		return false;
	}

	int fontWeight = FC_WEIGHT_REGULAR;
	if ( weight >= 700 )
		fontWeight = FC_WEIGHT_BOLD;

	const char* family = windowsFontName;
	if ( !strcasecmp( windowsFontName, "Tahoma" ) )
		family = "Bitstream Vera Sans";

	int slant = FC_SLANT_ROMAN;
	if ( flags & FONTFLAG_ITALIC )
		slant = FC_SLANT_ITALIC;

	FcPattern* pattern = FontMatch( "family", FcTypeString, family, "weight", FcTypeInteger,
		(const void*)(intptr_t)fontWeight, "slant", FcTypeInteger, (const void*)(intptr_t)slant, nullptr );
	if ( !pattern )
	{
		printf( "Unable to find font named %s\n", windowsFontName );
		m_szName[0] = 0;
		return false;
	}

	FcChar8* filename = NULL;
	if ( FcPatternGetString( pattern, "file", 0, &filename ) != FcResultMatch )
	{
		FcPatternDestroy( pattern );
		printf( "Unable to find font named %s\n", windowsFontName );
		m_szName[0] = 0;
		return false;
	}

	char fontFile[4096];
	strncpy( fontFile, (const char*)filename, sizeof( fontFile ) );
	FcPatternDestroy( pattern );

	if ( FT_New_Face( FontManager().m_Library, fontFile, 0, &face ) )
		return false;

	FT_Set_Pixel_Sizes( face, 0, (FT_UInt)( m_iHeight - 1 ) );

	const FT_Size_Metrics& metrics = face->size->metrics;
	m_iAscent = Fixed26Dot6ToInt( (int)metrics.ascender );
	m_iHeight = 2 * m_iOutlineSize + m_iDropShadowOffset + Fixed26Dot6ToInt( (int)metrics.height );
	m_iMaxCharWidth = Fixed26Dot6ToInt( (int)metrics.max_advance );
	m_iFxpHeight = (int)metrics.height + ( ( m_iDropShadowOffset + 2 * m_iOutlineSize ) << 6 );
#endif

	if ( m_iBlur > 1 )
	{
		m_pGaussianDistribution = new float[m_iBlur * 2 + 1];
		double sigma = (double)m_iBlur * 0.683;
		double scaleFactor = sqrt( 6.28 * sigma * sigma );
#if defined( _WIN32 )
		float normalization = static_cast<float>( 1.0 / scaleFactor );
#endif
		for ( i = 0; i <= m_iBlur * 2; i++ )
		{
			int x = i - m_iBlur;
			double exponent = -(double)( x * x ) / ( ( sigma + sigma ) * sigma );
#if defined( _WIN32 )
			m_pGaussianDistribution[i] = (float)( pow( 2.7, exponent ) * normalization );
#else
			m_pGaussianDistribution[i] = (float)( pow( 2.7, exponent ) * ( 1.0 / scaleFactor ) );
#endif
		}
	}

	return true;
}

void CLinuxFont::ApplyRotaryEffectToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba )
{
	int x;
	int y;
	unsigned char* dst;

	if ( !m_bRotary )
		return;

	y = (int)( (double)rgbaTall * 0.5 );
	dst = &rgba[4 * rgbaX + 4 * rgbaWide * ( rgbaY + y )];
	for ( x = 0; x < rgbaWide; x++, dst += 4 )
	{
		dst[0] = 127;
		dst[1] = 127;
		dst[2] = 127;
		dst[3] = 255;
	}
}

void CLinuxFont::ApplyScanlineEffectToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba )
{
	int x;
	int y;
	unsigned char* dst;

	if ( m_iScanLines <= 1 )
		return;

	for ( y = 1; y < rgbaTall; y++ )
	{
		if ( !( y % m_iScanLines ) )
			continue;

		dst = &rgba[4 * ( rgbaX + rgbaWide * ( rgbaY + y ) )];
		for ( x = 0; x < rgbaWide; x++, dst += 4 )
		{
#if defined( _WIN32 )
			dst[0] = static_cast<unsigned char>( dst[0] * 0.7 );
			dst[1] = static_cast<unsigned char>( dst[1] * 0.7 );
			dst[2] = static_cast<unsigned char>( dst[2] * 0.7 );
#else
			dst[0] = (unsigned char)( dst[0] * 0.7f );
			dst[1] = (unsigned char)( dst[1] * 0.7f );
			dst[2] = (unsigned char)( dst[2] * 0.7f );
#endif
		}
	}
}

void CLinuxFont::ApplyDropShadowToTexture(
	int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int, int, unsigned char* rgba )
{
	int x;
	int y;
	unsigned char* dst;
	const unsigned char* src;
	int offset = m_iDropShadowOffset;

	if ( !offset || offset > rgbaTall - 1 )
		return;

	for ( y = rgbaTall - 1; y >= offset; y-- )
	{
		for ( x = rgbaWide - 1; x >= offset; x-- )
		{
			dst = &rgba[4 * ( rgbaX + x + rgbaWide * ( rgbaY + y ) )];
			if ( dst[3] )
				continue;

			src = &rgba[4 * ( rgbaX + x - offset + rgbaWide * ( rgbaY + y - offset ) )];
			dst[0] = 0;
			dst[1] = 0;
			dst[2] = 0;
			dst[3] = src[3];
		}
	}
}

#if !defined( _WIN32 )
void CLinuxFont::ApplyOutlineToTexture(
	int rgbaX, int rgbaY, int rgbaWide, int, int charWide, int charTall, unsigned char* rgba )
{
	int x, y;
	int shadowX, shadowY;
	int testX, testY;
	unsigned char* src;
	unsigned char* test;

	if ( !m_iOutlineSize )
		return;

	for ( y = 0; y < charTall; y++ )
	{
		src = &rgba[4 * rgbaX + 4 * rgbaWide * ( rgbaY + y )];
		for ( x = 0; x < charWide; x++, src += 4 )
		{
			if ( src[3] )
				continue;

			for ( shadowX = -m_iOutlineSize; shadowX <= m_iOutlineSize; shadowX++ )
			{
				testX = x + shadowX;
				if ( testX < 0 || testX >= charWide )
					continue;

				for ( shadowY = -m_iOutlineSize; shadowY <= m_iOutlineSize; shadowY++ )
				{
					if ( !shadowX && !shadowY )
						continue;

					testY = y + shadowY;
					if ( testY < 0 || testY >= charTall )
						continue;

					test = &rgba[4 * ( rgbaX + testX + rgbaWide * ( rgbaY + testY ) )];
					if ( test[0] && test[1] && test[2] && test[3] )
					{
						src[0] = 0;
						src[1] = 0;
						src[2] = 0;
						src[3] = 255;
					}
				}
			}
		}
	}
}

#endif

void CLinuxFont::ApplyGaussianBlurToTexture( int, int, int rgbaWide, int rgbaTall, unsigned char* rgba )
{
	int x, y;
	int minX, maxX, minY, maxY;
	int srcX, srcY;
	int alpha;
	int size;
#if defined( _WIN32 )
	float accum;
#else
	double accum;
#endif
	float weightX, weightY;
	unsigned char* src;
	unsigned char* dst;

	if ( !m_pGaussianDistribution )
		return;

	size = 4 * rgbaTall * rgbaWide;
#if defined( _WIN32 )
	src = static_cast<unsigned char*>( _alloca( size ) );
#else
	src = new unsigned char[size];
#endif
	memcpy( src, rgba, size );

	dst = rgba;
	for ( y = 0; y < rgbaTall; y++ )
	{
		for ( x = 0; x < rgbaWide; x++ )
		{
			maxX = x + m_iBlur;
			if ( maxX > rgbaWide - 1 )
				maxX = rgbaWide - 1;
			minX = x - m_iBlur;
			if ( minX < 0 )
				minX = 0;
			minY = y - m_iBlur;
			if ( minY < 0 )
				minY = 0;
			maxY = y + m_iBlur;
			if ( maxY > rgbaTall - 1 )
				maxY = rgbaTall - 1;

			accum = 0.0;
			for ( srcX = minX; srcX <= maxX; srcX++ )
			{
				weightX = m_pGaussianDistribution[srcX - x + m_iBlur];
				for ( srcY = minY; srcY <= maxY; srcY++ )
				{
					weightY = m_pGaussianDistribution[srcY - y + m_iBlur];
#if defined( _WIN32 )
					accum += ( weightX * weightY ) * static_cast<float>( src[4 * ( srcX + srcY * rgbaWide )] );
#else
					accum += (double)src[4 * ( srcX + srcY * rgbaWide )] * (double)weightX * (double)weightY;
#endif
				}
			}

			alpha = (int)accum;
			if ( alpha > 255 )
				alpha = 255;

			dst[0] = 255;
			dst[1] = 255;
			dst[2] = 255;
			dst[3] = (unsigned char)alpha;
			dst += 4;
		}
	}

#if !defined( _WIN32 )
	delete[] src;
#endif
}

#if defined( _WIN32 )
void CLinuxFont::RenderWindowsGlyph( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba )
{
	HDC dc = static_cast<HDC>( m_hDC );

	int a, b, c;
	GetCharABCWidths( ch, a, b, c );
	SelectObject( dc, static_cast<HFONT>( m_hFont ) );
	const int charWide = m_bUnderlined ? a + b + c : b;

	MAT2 transform = {};
	transform.eM11.value = 1;
	transform.eM22.value = 1;
	GLYPHMETRICS metrics = {};
	int glyphBytes = 0;
	if ( m_bAntiAliased && ch <= 255 )
	{
		SelectObject( dc, static_cast<HFONT>( m_hFont ) );
		glyphBytes = static_cast<int>( GetGlyphOutlineA( dc, ch, GGO_GRAY8_BITMAP, &metrics, 0, NULL, &transform ) );
	}

	if ( glyphBytes > 0 )
	{
		unsigned char* bitmap = static_cast<unsigned char*>( _alloca( glyphBytes ) );
		GetGlyphOutlineA( dc, ch, GGO_GRAY8_BITMAP, &metrics, glyphBytes, bitmap, &transform );

		const int blackBoxWide = static_cast<int>( metrics.gmBlackBoxX );
		const int stride = ( blackBoxWide + 3 ) & ~3;
		const int startX = blackBoxWide >= b + 2 ? ( blackBoxWide - b ) / 2 : 0;
		const int top = m_iAscent - metrics.gmptGlyphOrigin.y;
		for ( int row = 0; row < static_cast<int>( metrics.gmBlackBoxY ); ++row )
		{
			const int y = rgbaY + row + top;
			for ( int col = startX; col < blackBoxWide; ++col )
			{
				const int x = rgbaX + col - startX + m_iBlur;
				if ( x >= rgbaWide || y >= rgbaTall )
					continue;
				const unsigned char coverage = bitmap[row * stride + col];
				const unsigned char color = coverage && ch != '\t' ? 255 : 0;
				unsigned char* pixel = &rgba[4 * ( x + y * rgbaWide )];
				pixel[0] = pixel[1] = pixel[2] = color;
				pixel[3] = static_cast<unsigned char>( std::min( 1.0f, coverage / 64.0f ) * 255.0f );
			}
		}
		return;
	}

	SetBkColor( dc, RGB( 0, 0, 0 ) );
	SetTextColor( dc, RGB( 255, 255, 255 ) );
	SetBkMode( dc, OPAQUE );
	MoveToEx( dc, m_bUnderlined ? 0 : -a, 0, NULL );
	const wchar_t wide = static_cast<wchar_t>( ch );
	if ( g_bWindowsFontSupportsUnicode )
	{
		ExtTextOutW( dc, 0, 0, 0, NULL, &wide, 1, NULL );
	}
	else
	{
		RECT rect = { 0, 0, charWide, m_iHeight };
		ExtTextOutA( dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL );
		char text[6] = {};
		WideCharToMultiByte( CP_ACP, 0, &wide, 1, text, sizeof( text ), NULL, NULL );
		ExtTextOutA( dc, 0, 0, 0, NULL, text, static_cast<UINT>( strlen( text ) ), NULL );
	}
	SetBkMode( dc, TRANSPARENT );

	const int width = std::min( charWide, m_rgiBitmapSize[0] );
	const int height = std::min( m_iHeight, m_rgiBitmapSize[1] );
	for ( int row = 0; row < height; ++row )
	{
		const int y = rgbaY + row;
		for ( int col = 0; col < width - m_iDropShadowOffset; ++col )
		{
			const int x = rgbaX + col;
			if ( x >= rgbaWide || y >= rgbaTall )
				continue;
			const unsigned char* source = &m_pBuf[4 * ( col + row * m_rgiBitmapSize[0] )];
			float red = source[0] / 255.0f;
			float green = source[1] / 255.0f;
			float blue = source[2] / 255.0f;
			if ( ch == '\t' )
				red = green = blue = 0.0f;
			unsigned char* pixel = &rgba[4 * ( x + y * rgbaWide )];
			pixel[0] = static_cast<unsigned char>( red * 255.0f );
			pixel[1] = static_cast<unsigned char>( green * 255.0f );
			pixel[2] = static_cast<unsigned char>( blue * 255.0f );
			pixel[3] = static_cast<unsigned char>( ( green * 0.55f + red * 0.34f + blue * 0.11f ) * 255.0f );
		}
	}

	if ( m_iDropShadowOffset )
	{
		unsigned char* pixel = &rgba[4 * ( rgbaX + ( m_iHeight - 1 ) * rgbaWide )];
		for ( int col = 0; col < width; ++col, pixel += 4 )
			memset( pixel, 0, 4 );
	}
}
#endif

void CLinuxFont::GetCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba )
{
#if defined( _WIN32 )
	RenderWindowsGlyph( ch, rgbaX, rgbaY, rgbaWide, rgbaTall, rgba );
#else
	FT_Error error = FT_Load_Glyph( face, FT_Get_Char_Index( face, (FT_ULong)ch ), FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT );
	if ( error )
	{
		fprintf( stderr, "Error in FL_Load_Glyph: %x\n", (unsigned int)error );
		return;
	}

	FT_GlyphSlot slot = face->glyph;
	int rows = (int)slot->bitmap.rows;
	int width = (int)slot->bitmap.width;
	unsigned char* buffer = slot->bitmap.buffer;
	int top = m_iAscent - slot->bitmap_top;
	int srcY = 0;
	if ( top < 0 )
		srcY = -top;
	int maxY = rows;
	if ( rows + top > rgbaTall )
		maxY = rgbaTall - top;

	unsigned char* dstRow = &rgba[4 * rgbaWide * ( srcY + top )];
	int srcIndex = width * srcY;
	for ( ; srcY < maxY; srcY++ )
	{
		int left = slot->bitmap_left;
		int startX = 0;
		int endX = width;
		if ( left < 0 )
			startX = -left;
		if ( rgbaWide < width + left )
			endX = rgbaWide - left;

		unsigned char* dst = dstRow + 4 * ( startX + left );
		for ( int x = startX; x < endX; x++, dst += 4 )
		{
			dst[0] = 255;
			dst[1] = 255;
			dst[2] = 255;
			dst[3] = buffer[srcIndex + x];
		}

		srcIndex += width;
		dstRow += 4 * rgbaWide;
	}
#endif

	ApplyDropShadowToTexture( rgbaX, rgbaY, rgbaWide, rgbaTall, 0, 0, rgba );
	ApplyGaussianBlurToTexture( rgbaX, rgbaY, rgbaWide, rgbaTall, rgba );
	ApplyScanlineEffectToTexture( rgbaX, rgbaY, rgbaWide, rgbaTall, rgba );
	ApplyRotaryEffectToTexture( rgbaX, rgbaY, rgbaWide, rgbaTall, rgba );
}

bool CLinuxFont::IsEqualTo( const char* windowsFontName, int tall, int weight, int blur, int, int flags )
{
	if ( strcasecmp( windowsFontName, m_szName ) )
		return false;

	return m_iTall == tall && m_iWeight == weight && m_iBlur == blur && m_iFlags == flags;
}

bool CLinuxFont::IsValid()
{
	return m_szName[0] != 0;
}

int CLinuxFont::GetHeight()
{
#if defined( _WIN32 )
	if ( !IsValid() )
		_wassert( L"IsValid()", WIN32FONT_SOURCE_PATH, 702 );
#endif
	return m_iHeight;
}

int CLinuxFont::GetAscent()
{
#if defined( _WIN32 )
	if ( !IsValid() )
		_wassert( L"IsValid()", WIN32FONT_SOURCE_PATH, 711 );
#endif
	return m_iAscent;
}

int CLinuxFont::GetMaxCharWidth()
{
#if defined( _WIN32 )
	if ( !IsValid() )
		_wassert( L"IsValid()", WIN32FONT_SOURCE_PATH, 720 );
#endif
	return m_iMaxCharWidth;
}

int CLinuxFont::GetFlags()
{
	return m_iFlags;
}

#if defined( _WIN32 )
void CLinuxFont::SetAsActiveFont( HDC hdc )
{
	if ( !IsValid() )
		_wassert( L"IsValid()", WIN32FONT_SOURCE_PATH, 630 );
	SelectObject( hdc, static_cast<HFONT>( m_hFont ) );
}
#else
void* CLinuxFont::SetAsActiveFont( GLXContext )
{
	return NULL;
}
#endif

#if !defined( _WIN32 )
bool CLinuxFont::HasChar( wchar_t wch )
{
	return FT_Get_Char_Index( face, (FT_ULong)wch ) != 0;
}
#endif

#if defined( _WIN32 )
void CLinuxFont::GetCharABCWidths( int ch, int& a, int& b, int& c )
{
	if ( !IsValid() )
		_wassert( L"IsValid()", WIN32FONT_SOURCE_PATH, 639 );

	if ( ch < ASCII_CHAR_COUNT )
	{
		a = m_ABCWidths[ch].a;
		b = m_ABCWidths[ch].b;
		c = m_ABCWidths[ch].c;
		return;
	}

	abc_cache_t entry = {};
	entry.wch = static_cast<unsigned short>( ch );
	unsigned short index = m_ExtendedABCWidthsCache.Find( entry );
	if ( m_ExtendedABCWidthsCache.IsValidIndex( index ) )
	{
		const abc_t& cached = m_ExtendedABCWidthsCache[index].abc;
		a = cached.a;
		b = cached.b;
		c = cached.c;
		return;
	}

	ABC measured = {};
	if ( GetCharABCWidthsW( static_cast<HDC>( m_hDC ), ch, ch, &measured ) )
	{
		a = measured.abcA;
		b = static_cast<int>( measured.abcB );
		c = measured.abcC;
	}
	else
	{
		wchar_t wide = static_cast<wchar_t>( ch );
		char text[6] = {};
		WideCharToMultiByte( CP_ACP, 0, &wide, 1, text, sizeof( text ), nullptr, nullptr );
		SIZE size = {};
		a = c = 0;
		b = GetTextExtentPoint32A( static_cast<HDC>( m_hDC ), text, static_cast<int>( strlen( text ) ), &size )
			? size.cx : m_iMaxCharWidth;
	}

	entry.abc.a = static_cast<signed char>( a );
	entry.abc.b = static_cast<signed char>( b );
	entry.abc.c = static_cast<signed char>( c );
	m_ExtendedABCWidthsCache.Insert( entry );
}
#else
void CLinuxFont::GetCharABCWidths( int ch, int& a, int& b, int& c )
{
	abc_cache_t finder;
	unsigned short i;

	finder.wch = ch;
	finder.abc.a = 0;
	finder.abc.b = 0;
	finder.abc.c = 0;

	i = m_ExtendedABCWidthsCache.Find( finder );
	if ( m_ExtendedABCWidthsCache.IsValidIndex( i ) )
	{
		a = m_ExtendedABCWidthsCache.Element( i ).abc.a;
		b = m_ExtendedABCWidthsCache.Element( i ).abc.b;
		c = m_ExtendedABCWidthsCache.Element( i ).abc.c;
		return;
	}

	a = 0;
	b = 0;
	c = 0;

	if ( FT_Load_Char( face, (FT_ULong)ch, 0 ) )
	{
		finder.abc.a = 0;
		finder.abc.c = 0;
		finder.abc.b = (short)( RoundHalfBBox( (int)face->bbox.xMax ) + m_iDropShadowOffset
			+ 2 * ( m_iOutlineSize + m_iBlur ) );
	}
	else
	{
		const FT_Glyph_Metrics& metrics = face->glyph->metrics;
		unsigned char blur = (unsigned char)m_iBlur;
		unsigned char outline = (unsigned char)m_iOutlineSize;
		unsigned char shadow = (unsigned char)m_iDropShadowOffset;
		char left = LowByteDifference( Fixed26Dot6ToInt( (int)metrics.horiBearingX ), blur, outline );
		int rightRaw = ( (int)metrics.horiAdvance - (int)metrics.horiBearingX ) - (int)metrics.width;
		char right = LowByteDifference( Fixed26Dot6ToInt( rightRaw ), blur, shadow, outline );
		int mid = Fixed26Dot6ToInt( (int)metrics.width ) + m_iDropShadowOffset + 2 * ( m_iOutlineSize + m_iBlur );
		finder.abc.a = 0;
		finder.abc.c = 0;
		finder.abc.b = (short)( (int)(unsigned short)mid + (short)left + (short)right );
	}

	m_ExtendedABCWidthsCache.Insert( finder );
	a = finder.abc.a;
	b = finder.abc.b;
	c = finder.abc.c;
}
#endif
