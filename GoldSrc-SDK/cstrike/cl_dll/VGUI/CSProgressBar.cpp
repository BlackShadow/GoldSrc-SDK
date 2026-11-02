#include "CSProgressBar.h"
#include "hud.h"
#include "GameUI_Interfaces.h"
#include "GameUI/ICareerUI.h"
#include "BitmapImagePanel.h"
#include "Label.h"
#include "vgui_controls/Controls.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

RoundedCornerPanel::RoundedCornerPanel( vgui2::Panel* parent )
	: vgui2::Panel( parent, "RoundedCornerBackground" )
{
	m_pTopLeftPanel = new vgui2::CBitmapImagePanel( this, "TopLeftPanel", "gfx/vgui/round_corner_nw" );
	m_pTopRightPanel = new vgui2::CBitmapImagePanel( this, "TopRightPanel", "gfx/vgui/round_corner_ne" );
	m_pBottomLeftPanel = new vgui2::CBitmapImagePanel( this, "BottomLeftPanel", "gfx/vgui/round_corner_sw" );
	m_pBottomRightPanel = new vgui2::CBitmapImagePanel( this, "BottomRightPanel", "gfx/vgui/round_corner_se" );
	m_pTopLeftBorderPanel = new vgui2::CBitmapImagePanel( this, "TopLeftBorderPanel", "gfx/vgui/round_corner_small_nw" );
	m_pTopRightBorderPanel = new vgui2::CBitmapImagePanel( this, "TopRightBorderPanel", "gfx/vgui/round_corner_small_ne" );
	m_pBottomLeftBorderPanel = new vgui2::CBitmapImagePanel( this, "BottomLeftBorderPanel", "gfx/vgui/round_corner_small_sw" );
	m_pBottomRightBorderPanel = new vgui2::CBitmapImagePanel( this, "BottomRightBorderPanel", "gfx/vgui/round_corner_small_se" );
	m_borderColor = Color( 0, 0, 0, 0 );
	m_bgColor = m_borderColor;
}

RoundedCornerPanel::~RoundedCornerPanel()
{
}

void RoundedCornerPanel::PerformLayout()
{
	int wide = IEngineStudio.IsHardware() == 1 ? 10 : 0;
	bool visible = IEngineStudio.IsHardware && IEngineStudio.IsHardware();
	m_pTopLeftPanel->SetVisible( visible );
	m_pTopRightPanel->SetVisible( visible );
	m_pBottomLeftPanel->SetVisible( visible );
	m_pBottomRightPanel->SetVisible( visible );
	m_pTopLeftBorderPanel->SetVisible( visible );
	m_pTopRightBorderPanel->SetVisible( visible );
	m_pBottomLeftBorderPanel->SetVisible( visible );
	m_pBottomRightBorderPanel->SetVisible( visible );
	int x = GetWide() - wide - 1;
	int y = GetTall() - wide - 1;
	m_pTopLeftPanel->SetBounds( 0, 0, wide, wide );
	m_pTopRightPanel->SetBounds( x, 0, wide, wide );
	m_pBottomLeftPanel->SetBounds( 0, y, wide, wide );
	m_pBottomRightPanel->SetBounds( x, y, wide, wide );
	m_pTopLeftBorderPanel->SetBounds( 0, 0, wide, wide );
	m_pTopRightBorderPanel->SetBounds( x, 0, wide, wide );
	m_pBottomLeftBorderPanel->SetBounds( 0, y, wide, wide );
	m_pBottomRightBorderPanel->SetBounds( x, y, wide, wide );
	m_pTopLeftBorderPanel->MoveToFront();
	m_pTopRightBorderPanel->MoveToFront();
	m_pBottomLeftBorderPanel->MoveToFront();
	m_pBottomRightBorderPanel->MoveToFront();
}

void RoundedCornerPanel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Panel::ApplySchemeSettings( scheme );
	m_bgColor = GetSchemeColor( "BuddyListBgColor", GetBgColor(), scheme );
	m_borderColor = scheme->GetColor( "BorderBright", Color( 0, 0, 0, 0 ) );
	m_pTopLeftPanel->m_bgColor = Color( 255, 255, 255, m_bgColor[3] );
	m_pTopRightPanel->m_bgColor = Color( 255, 255, 255, m_bgColor[3] );
	m_pBottomLeftPanel->m_bgColor = Color( 255, 255, 255, m_bgColor[3] );
	m_pBottomRightPanel->m_bgColor = Color( 255, 255, 255, m_bgColor[3] );
	m_pTopLeftBorderPanel->m_bgColor = m_borderColor;
	m_pTopRightBorderPanel->m_bgColor = m_borderColor;
	m_pBottomLeftBorderPanel->m_bgColor = m_borderColor;
	m_pBottomRightBorderPanel->m_bgColor = m_borderColor;
}

void RoundedCornerPanel::Draw()
{
	int x1, x2, y1, y2;
	vgui2::surface()->DrawSetColor( m_bgColor );
	if ( IEngineStudio.IsHardware() == 1 )
	{
		m_pTopLeftPanel->GetPos( x1, y1 );
		m_pBottomLeftPanel->GetPos( x2, y2 );
		y1 += m_pTopLeftPanel->GetTall();
		x2 += m_pBottomLeftPanel->GetWide();
		vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );
		m_pTopLeftPanel->GetPos( x1, y1 );
		m_pBottomRightPanel->GetPos( x2, y2 );
		x1 += m_pTopLeftPanel->GetWide();
		y2 += m_pBottomRightPanel->GetTall();
		vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );
		m_pTopRightPanel->GetPos( x1, y1 );
		m_pBottomRightPanel->GetPos( x2, y2 );
		y1 += m_pTopRightPanel->GetTall();
		x2 += m_pBottomRightPanel->GetWide();
		vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );
		vgui2::surface()->DrawSetColor( m_borderColor );
		vgui2::surface()->DrawSetTextColor( m_borderColor );
		m_pTopLeftPanel->GetPos( x1, y1 );
		m_pBottomLeftPanel->GetPos( x2, y2 );
		y1 += m_pTopLeftPanel->GetTall();
		vgui2::surface()->DrawFilledRect( x1, y1, x2 + 1, y2 + 1 );
		m_pTopLeftPanel->GetPos( x1, y1 );
		m_pTopRightPanel->GetPos( x2, y2 );
		x1 += m_pTopLeftPanel->GetWide();
		vgui2::surface()->DrawFilledRect( x1, y1, x2 + 1, y2 + 1 );
		m_pBottomLeftPanel->GetPos( x1, y1 );
		m_pBottomRightPanel->GetPos( x2, y2 );
		x1 += m_pBottomLeftPanel->GetWide();
		y1 += m_pBottomLeftPanel->GetTall();
		y2 += m_pBottomRightPanel->GetTall();
		vgui2::surface()->DrawFilledRect( x1, y1, x2 + 1, y2 + 1 );
		m_pTopRightPanel->GetPos( x1, y1 );
		m_pBottomRightPanel->GetPos( x2, y2 );
		y1 += m_pTopRightPanel->GetTall();
		x1 += m_pTopRightPanel->GetWide();
		x2 += m_pBottomRightPanel->GetWide();
		vgui2::surface()->DrawFilledRect( x1, y1, x2 + 1, y2 + 1 );
	}
	else
	{
		m_pTopLeftPanel->GetPos( x1, y1 );
		m_pBottomRightPanel->GetPos( x2, y2 );
		x2 += m_pBottomRightPanel->GetWide();
		y2 += m_pBottomRightPanel->GetTall();
		vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );
		vgui2::surface()->DrawSetColor( m_borderColor );
		vgui2::surface()->DrawOutlinedRect( x1, y1, x2, y2 );
	}
}

CCSProgressBar::CCSProgressBar( vgui2::Panel* parent )
	: vgui2::Frame( parent, "CSProgressBar", true, false )
{
	m_isTimeBased = false;
	m_startTime = 0;
	m_numTicks = 0;
	m_startTick = 0;
	m_currentTick = 0;
	if ( g_pCareerUI && g_pCareerUI->IsPlayingMatch() )
		SetScheme( "CareerClientScheme" );
	else
		SetScheme( "ClientScheme" );
	SetProportional( true );
	SetMoveable( false );
	SetSizeable( false );
	m_pTitle = new vgui2::Label( this, "TitleLabel", "" );
	m_pText = new vgui2::Label( this, "TextLabel", "" );
	m_pProgressBarBorder = new vgui2::Panel( this, "ProgressBarBorder" );
	m_pProgressBar = new vgui2::Panel( this, "ProgressBar" );
	m_pProgressBarSizer = new vgui2::Panel( this, "ProgressBarSizer" );
	m_pBackgroundPanel = new RoundedCornerPanel( this );
	LoadControlSettings( "Resource/UI/CSProgressBar.res", NULL );
}

CCSProgressBar::~CCSProgressBar()
{
}

void CCSProgressBar::Init( const char* title, int numTicks, int startTick, bool isTimeBased )
{
	m_pTitle->SetText( title );
	m_pText->SetText( "" );
	m_numTicks = numTicks;
	m_isTimeBased = isTimeBased;
	m_startTick = startTick;
	m_currentTick = startTick;
	InvalidateLayout( false, false );
}

void CCSProgressBar::Update( const char* statusText, int tick )
{
	m_pText->SetText( statusText );
	m_currentTick = tick;
	InvalidateLayout( false, false );
}

void CCSProgressBar::OnThink()
{
}

void CCSProgressBar::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Frame::ApplySchemeSettings( scheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	SetBorder( NULL );
	m_pProgressBarSizer->SetVisible( false );
	m_pBackgroundPanel->SetVisible( true );
	m_pProgressBarBorder->SetBorder( scheme->GetBorder( "ButtonDepressedBorder" ) );
	m_pProgressBarBorder->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pProgressBar->SetBorder( scheme->GetBorder( "ButtonBorder" ) );
	m_pProgressBar->SetBgColor( scheme->GetColor( "SelectionBG", Color( 0, 0, 0, 0 ) ) );
}

void CCSProgressBar::PerformLayout()
{
	vgui2::Frame::PerformLayout();
	int x, y, wide, tall, screenWide, screenTall;
	GetBounds( x, y, wide, tall );
	vgui2::surface()->GetScreenSize( screenWide, screenTall );
	if ( wide / 2 + x != screenWide / 2 )
	{
		SetPos( screenWide / 2 - wide / 2, screenTall / 2 - tall / 2 );
		GetBounds( x, y, wide, tall );
	}
	if ( m_numTicks )
		m_pProgressBar->SetWide( m_currentTick * m_pProgressBarSizer->GetWide() / m_numTicks );
	m_pBackgroundPanel->InvalidateLayout( false, false );
}

void CCSProgressBar::PaintBackground()
{
	m_pBackgroundPanel->Draw();
}
