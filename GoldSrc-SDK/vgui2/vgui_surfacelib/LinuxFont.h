#pragma once

#include "utlrbtree.h"

struct FT_FaceRec_;
typedef FT_FaceRec_* FT_Face;

struct __GLXcontextRec;
typedef __GLXcontextRec* GLXContext;

#if defined( _WIN32 )
struct HDC__;
#endif

class CLinuxFont
{
public:
	struct abc_t
	{
#if defined( _WIN32 )
		signed char a;
		signed char b;
		signed char c;
		char reserved;
#else
		short b;
		char a;
		char c;
#endif
	};

	struct abc_cache_t
	{
#if defined( _WIN32 )
		unsigned short wch;
#else
		int wch;
#endif
		abc_t abc;
	};

	struct font_name_entry
	{
		char* m_pchFile;
		char* m_pchFriendlyName;
	};

	CLinuxFont();
	~CLinuxFont();

#if !defined( _WIN32 )
	bool FindFontDataFile( const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags,
		char* dataFile, int dataFileChars );
#endif

	bool Create( const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags );
	void GetCharRGBA( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );
	bool IsEqualTo( const char* windowsFontName, int tall, int weight, int blur, int scanlines, int flags );
	bool IsValid();

	int GetHeight();
	int GetAscent();
	int GetMaxCharWidth();
	int GetFlags();

	int GetBlur() { return m_iBlur; }
	bool GetUnderlined() { return m_bUnderlined; }
	bool IsAdditive() { return m_bAdditive; }
	const char* GetName() { return m_szName; }
	int GetWeight() { return m_iWeight; }

#if defined( _WIN32 )
	void SetAsActiveFont( HDC__* hdc );
#else
	void* SetAsActiveFont( GLXContext cglContext );
#endif
#if !defined( _WIN32 )
	bool HasChar( wchar_t wch );
#endif
	void GetCharABCWidths( int ch, int& a, int& b, int& c );

private:
	static bool ExtendedABCWidthsCacheLessFunc( const abc_cache_t& lhs, const abc_cache_t& rhs );
	static bool FontEntryLess( const font_name_entry& a, const font_name_entry& b );

#if defined( _WIN32 )
	bool CreateWindowsFont( const char* name, int tall, int weight, int flags );
	void RenderWindowsGlyph( int ch, int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );
#endif

	void ApplyRotaryEffectToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );
	void ApplyScanlineEffectToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );
	void ApplyDropShadowToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall,
		unsigned char* rgba );
#if !defined( _WIN32 )
	void ApplyOutlineToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, int charWide, int charTall,
		unsigned char* rgba );
#endif
	void ApplyGaussianBlurToTexture( int rgbaX, int rgbaY, int rgbaWide, int rgbaTall, unsigned char* rgba );

	char m_szName[32];
	int m_iTall;
	int m_iWeight;
	int m_iFlags;

	bool m_bAntiAliased;
	bool m_bRotary;
#if !defined( _WIN32 )
	bool m_bAdditive;
#endif
	int m_iDropShadowOffset;
	bool m_bUnderlined;
#if !defined( _WIN32 )
	int m_iOutlineSize;
#endif

	int m_iHeight;
	int m_iMaxCharWidth;
	int m_iAscent;

#if defined( _WIN32 )
	static constexpr int ASCII_CHAR_COUNT = 128;
	abc_t m_ABCWidths[ASCII_CHAR_COUNT];
#endif
	CUtlRBTree<abc_cache_t, unsigned short> m_ExtendedABCWidthsCache;

#if defined( _WIN32 )
	void* m_hFont;
	void* m_hDC;
	void* m_hDIB;
#endif
	int m_rgiBitmapSize[2];
	unsigned char* m_pBuf;
#if defined( _WIN32 )
	bool m_bAdditive;
#endif

	int m_iScanLines;
	int m_iBlur;
	float* m_pGaussianDistribution;

#if !defined( _WIN32 )
	FT_Face face;
	int m_iFxpHeight;
#endif

	static CUtlRBTree<font_name_entry> m_FriendlyNameCache;
	static bool ms_bSetFriendlyNameCacheLessFunc;
};
