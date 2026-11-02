#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

int CHudScope::Init()
{
	m_iFlags |= HUD_ACTIVE;
	m_fOn = 0;
	m_iAlpha = 110;
	gHUD.AddHudElem( this );
	return 1;
}

void CHudScope::Reset()
{
	m_fOn = 0;
	m_iAlpha = 110;
}

int CHudScope::MsgFunc_NVGToggle( const char*, int size, void* buffer )
{
	BEGIN_READ( buffer, size );
	m_fOn = READ_BYTE();
	return 1;
}

void CHudScope::UserCmd_NVGAdjustUp()
{
	m_iAlpha += 10;
	if ( m_iAlpha > 220 )
		m_iAlpha = 220;
}

void CHudScope::UserCmd_NVGAdjustDown()
{
	m_iAlpha -= 10;
	if ( m_iAlpha < 30 )
		m_iAlpha = 30;
}

CHudScope::~CHudScope()
{
	delete m_hTL;
	delete m_hT;
	delete m_hTR;
	delete m_hL;
	delete m_hR;
	delete m_hBL;
	delete m_hB;
	delete m_hBR;
	delete m_hTL2;
	delete m_hT2;
	delete m_hTR2;
	delete m_hL2;
	delete m_hR2;
	delete m_hBL2;
	delete m_hB2;
	delete m_hBR2;
	delete m_hTL3;
	delete m_hT3;
	delete m_hTR3;
	delete m_hL3;
	delete m_hR3;
	delete m_hBL3;
	delete m_hB3;
	delete m_hBR3;
	delete m_hHor;
	delete m_hVer;
}

int CHudScope::VidInit()
{
	wrect_t** rects[3][8] =
	{
		{ &m_hTL, &m_hT, &m_hTR, &m_hL, &m_hR, &m_hBL, &m_hB, &m_hBR },
		{ &m_hTL2, &m_hT2, &m_hTR2, &m_hL2, &m_hR2, &m_hBL2, &m_hB2, &m_hBR2 },
		{ &m_hTL3, &m_hT3, &m_hTR3, &m_hL3, &m_hR3, &m_hBL3, &m_hB3, &m_hBR3 },
	};
	HSPRITE* sprites[3][8] =
	{
		{ &m_hTopLeft, &m_hTop, &m_hTopRight, &m_hLeft, &m_hRight, &m_hBottomLeft, &m_hBottom, &m_hBottomRight },
		{ &m_hTopLeft2, &m_hTop2, &m_hTopRight2, &m_hLeft2, &m_hRight2, &m_hBottomLeft2, &m_hBottom2, &m_hBottomRight2 },
		{ &m_hTopLeft3, &m_hTop3, &m_hTopRight3, &m_hLeft3, &m_hRight3, &m_hBottomLeft3, &m_hBottom3, &m_hBottomRight3 },
	};
	const char* names[3][8] =
	{
		{ "top_left", "top", "top_right", "left", "right", "bottom_left", "bottom", "bottom_right" },
		{ "top_left2", "top2", "top_right2", "left2", "right2", "bottom_left2", "bottom2", "bottom_right2" },
		{ "top_left3", "top3", "top_right3", "left3", "right3", "bottom_left3", "bottom3", "bottom_right3" },
	};
	const char* paths[3][8] =
	{
		{ "sprites/top_left.spr", "sprites/top.spr", "sprites/top_right.spr", "sprites/left.spr",
			"sprites/right.spr", "sprites/bottom_left.spr", "sprites/bottom.spr", "sprites/bottom_right.spr" },
		{ "sprites/top_left2.spr", "sprites/top2.spr", "sprites/top_right2.spr", "sprites/left2.spr",
			"sprites/right2.spr", "sprites/bottom_left2.spr", "sprites/bottom2.spr", "sprites/bottom_right2.spr" },
		{ "sprites/top_left3.spr", "sprites/top3.spr", "sprites/top_right3.spr", "sprites/left3.spr",
			"sprites/right3.spr", "sprites/bottom_left3.spr", "sprites/bottom3.spr", "sprites/bottom_right3.spr" },
	};
	for ( int group = 0; group < 3; ++group )
	{
		for ( int i = 0; i < 8; ++i )
			*rects[group][i] = new wrect_t;
	}
	m_hHor = new wrect_t;
	m_hVer = new wrect_t;
	m_hTopLeftSmall = SPR_Load( "sprites/top_left_small.spr" );
	m_hTopRightSmall = SPR_Load( "sprites/top_right_small.spr" );
	m_hBottomLeftSmall = SPR_Load( "sprites/bottom_left_small.spr" );
	m_hBottomRightSmall = SPR_Load( "sprites/bottom_right_small.spr" );
	for ( int group = 0; group < 3; ++group )
	{
		for ( int i = 0; i < 8; ++i )
			gHUD.GetSpriteIndex( names[group][i] );
		for ( int i = 0; i < 8; ++i )
		{
			wrect_t& rect = **rects[group][i];
			rect.left = 0;
			rect.top = 0;
			rect.right = group == 0 && i != 1 && i != 6 ? 192 : 256;
			rect.bottom = group == 2 || i == 3 || i == 4 ? 256 : group == 0 ? 112 : 172;
		}
		for ( int i = 0; i < 8; ++i )
			*sprites[group][i] = SPR_Load( paths[group][i] );
	}
	gHUD.GetSpriteIndex( "horizontal" );
	gHUD.GetSpriteIndex( "vertical" );
	m_hHor->left = 0;
	m_hHor->top = 0;
	m_hHor->right = 256;
	m_hHor->bottom = 32;
	m_hVer->left = 0;
	m_hVer->top = 0;
	m_hVer->right = 40;
	m_hVer->bottom = 256;
	m_hHorizontal = SPR_Load( "sprites/horizontal.spr" );
	m_hVertical = SPR_Load( "sprites/vertical.spr" );
	return 1;
}

int CHudScope::Draw( float )
{
	if ( !IEngineStudio.IsHardware() || ( g_iUser1 & ~4 ) || gHUD.m_iFOV > 44 )
		return 1;
	int centerX = ScreenWidth / 2;
	int centerY = ScreenHeight / 2;
	if ( ScreenWidth < 640 )
	{
		HSPRITE sprites[] = { m_hTopLeftSmall, m_hTopRightSmall, m_hBottomLeftSmall, m_hBottomRightSmall };
		const char* names[] = { "top_left", "top_right", "bottom_left", "bottom_right" };
		for ( int i = 0; i < 4; ++i )
		{
			int x = i & 1 ? centerX : ScreenWidth <= 511 ? -48 : 0;
			int y = i >= 2 ? centerY - ( ScreenWidth <= 511 ? 32 : 0 ) : 0;
			SPR_Set( sprites[i], 255, 255, 255 );
			SPR_DrawHoles( 0, x, y, &gHUD.GetSpriteRect( gHUD.GetSpriteIndex( names[i] ) ) );
		}
	}
	else
	{
		HSPRITE sprites[3][8] =
		{
			{ m_hTopLeft, m_hTop, m_hTopRight, m_hLeft, m_hRight, m_hBottomLeft, m_hBottom, m_hBottomRight },
			{ m_hTopLeft2, m_hTop2, m_hTopRight2, m_hLeft2, m_hRight2, m_hBottomLeft2, m_hBottom2, m_hBottomRight2 },
			{ m_hTopLeft3, m_hTop3, m_hTopRight3, m_hLeft3, m_hRight3, m_hBottomLeft3, m_hBottom3, m_hBottomRight3 },
		};
		wrect_t* rects[3][8] =
		{
			{ m_hTL, m_hT, m_hTR, m_hL, m_hR, m_hBL, m_hB, m_hBR },
			{ m_hTL2, m_hT2, m_hTR2, m_hL2, m_hR2, m_hBL2, m_hB2, m_hBR2 },
			{ m_hTL3, m_hT3, m_hTR3, m_hL3, m_hR3, m_hBL3, m_hB3, m_hBR3 },
		};
		int group = ScreenWidth > 1023 ? 2 : ScreenWidth > 799 ? 1 : 0;
		int left = group == 0 ? -320 : -384;
		int top = group == 0 ? -240 : group == 1 ? -300 : -384;
		const int columns[] = { 0, 1, 2, 0, 2, 0, 1, 2 };
		const int rows[] = { 0, 0, 0, 1, 1, 2, 2, 2 };
		int x[] = { centerX + left, centerX - 128, centerX + 128 };
		int y[] = { centerY + top, centerY - 128, centerY + 128 };
		for ( int i = 0; i < 8; ++i )
		{
			SPR_Set( sprites[group][i], 255, 255, 255 );
			SPR_DrawHoles( 0, x[columns[i]], y[rows[i]], rects[group][i] );
		}
	}
	if ( ScreenWidth <= 640 )
		return 1;
	int leftMask = centerX - 320;
	int rightMask = centerX + 320;
	if ( ScreenWidth > 799 )
	{
		leftMask = centerX - 384;
		rightMask = centerX + 384;
	}
	if ( ScreenWidth > 1024 )
	{
		int columns = ScreenWidth / 256 + 1;
		for ( int y = centerY - 416; y > 0; y -= 32 )
		{
			for ( int i = 0; i < columns; ++i )
			{
				SPR_Set( m_hHorizontal, 255, 255, 255 );
				SPR_Draw( 0, i * 256, y, m_hHor );
			}
		}
		for ( int i = 0; i < columns; ++i )
		{
			SPR_Set( m_hHorizontal, 255, 255, 255 );
			SPR_Draw( 0, i * 256, 0, m_hHor );
		}
		for ( int y = centerY + 384; y < ScreenHeight; y += 32 )
		{
			for ( int i = 0; i < columns; ++i )
			{
				SPR_Set( m_hHorizontal, 255, 255, 255 );
				SPR_Draw( 0, i * 256, y, m_hHor );
			}
		}
	}
	int rows = ScreenHeight / 256 + 1;
	for ( int x = leftMask - 40; x > 0; x -= 40 )
	{
		for ( int i = 0; i < rows; ++i )
		{
			SPR_Set( m_hVertical, 255, 255, 255 );
			SPR_Draw( 0, x, i * 256, m_hVer );
		}
	}
	for ( int i = 0; i < rows; ++i )
	{
		SPR_Set( m_hVertical, 255, 255, 255 );
		SPR_Draw( 0, 0, i * 256, m_hVer );
	}
	for ( int x = rightMask; x < ScreenWidth; x += 40 )
	{
		for ( int i = 0; i < rows; ++i )
		{
			SPR_Set( m_hVertical, 255, 255, 255 );
			SPR_Draw( 0, x, i * 256, m_hVer );
		}
	}
	return 1;
}
