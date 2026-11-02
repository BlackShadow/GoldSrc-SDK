#include "hud.h"
#include "counterstrikeviewport.h"
#include "BitmapImagePanel.h"
#include "BuildGroup.h"
#include "Label.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"
#include "vgui/IScheme.h"
#include "r_studioint.h"

extern engine_studio_api_t IEngineStudio;

int GetAlternateProportionalValueFromNormal( int normalizedValue )
{
	int wide, tall, proportionalWide, proportionalTall;
	vgui2::surface()->GetScreenSize( wide, tall );
	vgui2::surface()->GetProportionalBase( proportionalWide, proportionalTall );
	double scale = (double)tall / proportionalTall;
	double widthScale = (double)wide / proportionalWide;
	if ( scale > widthScale )
	{
		scale = widthScale;
	}
	return (int)( scale * normalizedValue );
}

int GetAlternateProportionalValueFromScaled( int scaledValue )
{
	int normalizedValue = vgui2::scheme()->GetProportionalNormalizedValue( scaledValue );
	return GetAlternateProportionalValueFromNormal( normalizedValue );
}

void RepositionControl( vgui2::Panel* panel )
{
	int x, y, wide, tall;
	panel->GetBounds( x, y, wide, tall );
	x = GetAlternateProportionalValueFromScaled( x );
	y = GetAlternateProportionalValueFromScaled( y );
	wide = GetAlternateProportionalValueFromScaled( wide );
	tall = GetAlternateProportionalValueFromScaled( tall );
	panel->SetBounds( x, y, wide, tall );
}

#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
CounterStrikeViewport::CCSBackGroundPanel::CCSBackGroundPanel( vgui2::Panel* parent ) :
	TeamFortressViewport::CBackGroundPanel( parent )
{
	m_pTopLeftPanel = new vgui2::CBitmapImagePanel( this, "TopLeftPanel", "gfx/vgui/round_corner_nw" );
	m_pTopRightPanel = new vgui2::CBitmapImagePanel( this, "TopRightPanel", "gfx/vgui/round_corner_ne" );
	m_pBottomLeftPanel = new vgui2::CBitmapImagePanel( this, "BottomLeftPanel", "gfx/vgui/round_corner_sw" );
	m_pBottomRightPanel = new vgui2::CBitmapImagePanel( this, "BottomRightPanel", "gfx/vgui/round_corner_se" );
	m_pGapPanel = new vgui2::Panel( this, "GapPanel" );
	m_pTitleLabel = new vgui2::Label( this, "CaptionLabel", "" );
	m_offsetY = 0;
	m_offsetX = 0;
	LoadControlSettings( "Resource/UI/BackgroundPanel.res", NULL );
}

CounterStrikeViewport::CCSBackGroundPanel::~CCSBackGroundPanel()
{
}

void CounterStrikeViewport::CCSBackGroundPanel::SetTitleText( const char* text )
{
	m_pTitleLabel->SetText( text );
}

void CounterStrikeViewport::CCSBackGroundPanel::PerformLayout()
{
	vgui2::Frame::PerformLayout();
}

void CounterStrikeViewport::CCSBackGroundPanel::Activate()
{
	SetPos( 0, 0 );
	vgui2::Frame::Activate();
	if ( !IsProportional() )
	{
		return;
	}
	int screenWide, screenTall, wide, tall;
	vgui2::surface()->GetScreenSize( screenWide, screenTall );
	GetSize( wide, tall );
	if ( wide == screenWide && tall == screenTall )
	{
		return;
	}
	SetSize( screenWide, screenTall );
	wide = GetAlternateProportionalValueFromScaled( wide );
	tall = GetAlternateProportionalValueFromScaled( tall );
	m_offsetX = ( screenWide - wide ) / 2;
	m_offsetY = ( screenTall - tall ) / 2;
	if ( GetBuildGroup() && GetBuildGroup()->GetPanelList() )
	{
		CUtlVector<vgui2::PHandle>* panels = GetBuildGroup()->GetPanelList();
		CUtlVector<vgui2::Panel*> resizedPanels;
		CUtlVector<vgui2::Panel*> movedPanels;
		for ( int i = 0; i < panels->Count(); ++i )
		{
			vgui2::PHandle handle = ( *panels )[i];
			vgui2::Panel* panel = handle.Get();

			bool found = false;
			for ( int j = 0; j < resizedPanels.Count(); ++j )
			{
				if ( panel == resizedPanels[j] )
					found = true;
			}

			if ( panel && !found )
			{
				resizedPanels.AddToTail( panel );
				if ( panel != this )
					RepositionControl( panel );
			}
		}

		for ( int i = 0; i < panels->Count(); ++i )
		{
			vgui2::PHandle handle = ( *panels )[i];
			vgui2::Panel* panel = handle.Get();

			bool found = false;
			for ( int j = 0; j < movedPanels.Count(); ++j )
			{
				if ( panel == movedPanels[j] )
					found = true;
			}

			if ( panel && !found )
			{
				movedPanels.AddToTail( panel );
				if ( panel != this )
				{
					int x, y;
					panel->GetPos( x, y );
					panel->SetPos( x + m_offsetX, y + m_offsetY );
				}
			}
		}
	}
}

void CounterStrikeViewport::CCSBackGroundPanel::PaintBackground()
{
	vgui2::surface()->DrawSetColor( m_bgColor );
	int x1, y1, x2, y2;
	if ( IEngineStudio.IsHardware() )
	{
		if ( m_pGapPanel->GetTall() > m_pGapPanel->GetWide() )
		{
			m_pTopLeftPanel->GetPos( x1, y1 );
			m_pBottomLeftPanel->GetPos( x2, y2 );
			y1 += m_pTopLeftPanel->GetTall();
			x2 += m_pBottomLeftPanel->GetWide();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			m_pTopLeftPanel->GetPos( x1, y1 );
			m_pGapPanel->GetPos( x2, y2 );
			x1 += m_pTopLeftPanel->GetWide();
			y2 += m_pGapPanel->GetTall();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			m_pGapPanel->GetPos( x1, y1 );
			m_pBottomRightPanel->GetPos( x2, y2 );
			x1 += m_pGapPanel->GetWide();
			y2 += m_pBottomRightPanel->GetTall();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			m_pTopRightPanel->GetPos( x1, y1 );
			m_pBottomRightPanel->GetPos( x2, y2 );
			y1 += m_pTopRightPanel->GetTall();
			x2 += m_pBottomRightPanel->GetWide();
		}
		else
		{
			m_pTopLeftPanel->GetPos( x1, y1 );
			m_pTopRightPanel->GetPos( x2, y2 );
			x1 += m_pTopLeftPanel->GetWide();
			y2 += m_pTopRightPanel->GetTall();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			int x3, y3;
			m_pTopLeftPanel->GetPos( x1, y1 );
			m_pGapPanel->GetPos( x2, y2 );
			m_pTopRightPanel->GetPos( x3, y3 );
			y1 += m_pTopLeftPanel->GetTall();
			x2 = x3 + m_pTopRightPanel->GetWide();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			m_pGapPanel->GetPos( x1, y1 );
			m_pBottomRightPanel->GetPos( x2, y2 );
			y1 += m_pGapPanel->GetTall();
			x2 += m_pBottomRightPanel->GetWide();
			vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );

			m_pBottomLeftPanel->GetPos( x1, y1 );
			m_pBottomRightPanel->GetPos( x2, y2 );
			x1 += m_pBottomLeftPanel->GetWide();
			y2 += m_pBottomRightPanel->GetTall();
		}
		vgui2::surface()->DrawFilledRect( x1, y1, x2, y2 );
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

void CounterStrikeViewport::CCSBackGroundPanel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Frame::ApplySchemeSettings( scheme );
	SetBgColor( scheme->GetColor( "ViewportBG", Color( 0, 0, 0, 0 ) ) );
	m_titleColor = scheme->GetColor( "TitleIcon", Color( 0, 0, 0, 200 ) );
	m_pTitleLabel->SetFgColor( m_titleColor );
	m_bgColor = scheme->GetColor( "ViewportBG", Color( 0, 0, 0, 200 ) );
	m_borderColor = scheme->GetColor( "BorderBright", Color( 0, 0, 0, 0 ) );
	Color cornerColor( 255, 255, 255, m_bgColor.a() );
	m_pTopLeftPanel->m_bgColor = cornerColor;
	m_pTopRightPanel->m_bgColor = cornerColor;
	m_pBottomLeftPanel->m_bgColor = cornerColor;
	m_pBottomRightPanel->m_bgColor = cornerColor;
	if ( IEngineStudio.IsHardware && !IEngineStudio.IsHardware() )
	{
		m_pTopLeftPanel->SetVisible( false );
		m_pTopRightPanel->SetVisible( false );
		m_pBottomLeftPanel->SetVisible( false );
		m_pBottomRightPanel->SetVisible( false );
	}
}
