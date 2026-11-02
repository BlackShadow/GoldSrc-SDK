#pragma once

#if defined( SW_RENDERER )
#include "model.h"
#include "render.h"
#include "r_shared.h"

#include "wad.h"
#include "filesystem.h"

#include "../cl_dll/wrect.h"

#define MIP_EXTRASIZE	24

extern qpic_t*		draw_disc; // also used on sbar
extern qpic_t*		draw_backtile;
extern cachewad_t	custom_wad;
extern cachepic_t	menu_cachepics[16];
extern int			menu_numcachepics;

void		Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad );
void		Draw_CacheWadInitFromFile( FileHandle_t file, int length, char* name, int cacheMax, cachewad_t* wad );
void		Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize );
void		Draw_AllocateCacheSpace( cachewad_t* wad, int cacheMax, int tempWad );
void		Draw_FreeWad( cachewad_t* wad );
int			Draw_CacheIndex( cachewad_t* wad, char* name );
int			Draw_CacheFindIndex( cachewad_t* wad, char* name );
int			Draw_CacheByIndex( cachewad_t* wad, int index, int playernum );
void*		Draw_CacheGet( cachewad_t* wad, int index );
qboolean	Draw_CacheReload( cachewad_t* wad, int index, lumpinfo_t* lump, cachepic_t* pic, char* clean, char* path );
qboolean	Draw_CustomCacheWadInit( int cacheMax, cachewad_t* wad, void* data, int size );
void*		Draw_CustomCacheGet( cachewad_t* wad, void* data, int size, int index );
qboolean	Draw_ValidateCustomLogo( cachewad_t* wad, byte* data, lumpinfo_t* lump );
qboolean	CustomDecal_Init( cachewad_t* wad, void* data, int size, int playernum );
qboolean	CustomDecal_Validate( void* data, int size );

void		Draw_MiptexTexture( cachewad_t* wad, byte* data );
qpic_t*		Draw_PicFromWad( char* name );
qpic_t*		Draw_CachePic( char* path );
void		Draw_Init();
void		Draw_ResetTextColor();
void		Draw_SetTextColor( float red, float green, float blue );
void		Draw_GetDefaultColor();
int			Draw_Character( int x, int y, int character, unsigned int font );
int			Draw_String( int x, int y, char* text );
void		Draw_DebugChar( char character );
int			Draw_StringLen( const char* text, unsigned int font );
int			Draw_MessageFontInfo( short* widths, unsigned int font );
int			Draw_MessageCharacterAdd( int x, int y, int character, int red, int green, int blue, unsigned int font );
void		Draw_Pic( int x, int y, qpic_t* pic );
void		Draw_AlphaSubPic( int xDest, int yDest, int xSrc, int ySrc, int width, int height,
				qpic_t* pic, colorVec* color, int alpha );
void		Draw_AlphaPic( int x, int y, qpic_t* pic, colorVec* color, int alpha );
void		Draw_AlphaAddPic( int x, int y, qpic_t* pic, colorVec* color, int alpha );
void		Draw_FillRGBA( int x, int y, int width, int height, int red, int green, int blue, int alpha );
void		Draw_FillRGBABlend( int x, int y, int width, int height, int red, int green, int blue, int alpha );
void		Draw_FillRGB( int x, int y, int width, int height, int red, int green, int blue );
void		Draw_LineRGB( int x1, int y1, int x2, int y2, int red, int green, int blue );
void		Draw_TransPic( int x, int y, qpic_t* pic );
void		EnableScissorTest( int x, int y, int width, int height );
void		DisableScissorTest();
int			ValidateWRect( const wrect_t* rect );
int			IntersectWRect( const wrect_t* first, const wrect_t* second, wrect_t* result );
int			AdjustSubRect( mspriteframe_t* frame, int* width, int* height, const wrect_t* rect );
int			SpriteFrameClip( mspriteframe_t* frame, int* x, int* y, int* width, int* height, const wrect_t* rect );
void		Draw_SpriteFrame( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect );
void		Draw_SpriteFrameHoles( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect );
void		Draw_SpriteFrameAdditive( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect );
void		Draw_SpriteFrameGeneric( mspriteframe_t* frame, word* palette, int x, int y, const wrect_t* rect,
				int src, int dest, int width, int height );
void		Draw_ScaledRGBAImage( int x, int y, int width, int height, int srcx, int srcy, int srcw, int srch,
				int texw, int texh, byte* rgba );
void		Draw_RGBAImage( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
				byte* rgba, int istext, byte red, byte green, byte blue, byte alpha );
void		Draw_RGBAImageAdd15( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
				byte* rgba, int istext, byte red, byte green, byte blue, byte alpha );
void		Draw_RGBAImageAdd16( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
				byte* rgba, int istext, byte red, byte green, byte blue, byte alpha );
void		Draw_RGBAImageAdditive( int x, int y, int srcx, int srcy, int srcw, int srch, int texw, int texh,
				byte* rgba, int istext, byte red, byte green, byte blue, byte alpha );
qpic_t*		Draw_SetupConback( int screenWidth );
void		Draw_ConsoleBackground( int lines );
void		R_DrawRect16( vrect_t* rect, int rowbytes, byte* source, byte* palette, int transparent );
void		Draw_TileClear( int x, int y, int width, int height );
void		Draw_Fill( int x, int y, int width, int height, int color );
void		Draw_FadeScreen();
void		Draw_BeginDisc();
void		Draw_EndDisc();
#else

#include "wad.h"
#include "quakedef.h"
#include "filesystem.h"

#define CACHE_WAD_NAME_BYTES 64

#define MIP_EXTRASIZE 32

typedef struct
{
	char name[CACHE_WAD_NAME_BYTES];
	cache_user_t cache;
} cacheentry_t;

typedef struct cachewad_s cachewad_t;

typedef void ( *PFNCACHE )( cachewad_t* wad, byte* data );

struct cachewad_s
{
	char* name;
	cacheentry_t* cache;
	int cacheCount;
	int cacheMax;
	lumpinfo_t* lumps;
	int lumpCount;
	int cacheExtra;
	PFNCACHE pfnCacheBuild;
	int numpaths;
	char** basedirs;
	int* lumppathindices;
	int tempWad;
};

void			Draw_CacheWadInit( char* name, int cacheMax, cachewad_t* wad );
void			Draw_CacheWadInitFromFile( FileHandle_t file, int length, char* name, int cacheMax, cachewad_t* wad );
void			Draw_CacheWadHandler( cachewad_t* wad, PFNCACHE fn, int extraDataSize );
void			Draw_AllocateCacheSpace( cachewad_t* wad, int cacheMax, int tempWad );
void			Draw_FreeWad( cachewad_t* wad );
int				Draw_CacheIndex( cachewad_t* wad, char* name );
int				Draw_CacheByIndex( cachewad_t* wad, int index, int playernum );
void*			Draw_CacheGet( cachewad_t* wad, int index );
qboolean		Draw_CacheReload( cachewad_t* wad, int index, lumpinfo_t* lump, cacheentry_t* pic, char* clean,
					char* path );
void			Draw_MiptexTexture( cachewad_t* wad, byte* data );

qboolean		Draw_CustomCacheWadInit( int cacheMax, cachewad_t* wad, void* data, int size );
void*			Draw_CustomCacheGet( cachewad_t* wad, void* data, int size, int index );
qboolean		Draw_ValidateCustomLogo( cachewad_t* wad, byte* data, lumpinfo_t* lump );
qboolean		CustomDecal_Init( cachewad_t* wad, void* data, int size, int playernum );
qboolean		CustomDecal_Validate( void* data, int size );

#endif
