#include "basecareermenu.h"
#include "GameUI_Interfaces.h"
#include "GameUI/ICareerUI.h"
#include "BuildGroup.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"

CCSCareerBaseMenu::CCSCareerBaseMenu( vgui2::Panel* parent, const char* name, bool showTaskBar )
	: vgui2::Frame( parent, name, showTaskBar, false )
{
	if ( g_pCareerUI && g_pCareerUI->IsPlayingMatch() )
		SetScheme( "CareerClientScheme" );
	else
		SetScheme( "ClientScheme" );
	SetProportional( true );
	vgui2::Frame::SetTitle( "", true );
	vgui2::Frame::SetMoveable( false );
	vgui2::Frame::SetSizeable( false );
	vgui2::Frame::SetTitleBarVisible( false );
	m_offsetY = 0;
	m_offsetX = 0;
}

CCSCareerBaseMenu::~CCSCareerBaseMenu()
{
}

void CCSCareerBaseMenu::UpdateScheme()
{
	if ( g_pCareerUI && g_pCareerUI->IsPlayingMatch() )
		SetScheme( "CareerClientScheme" );
	else
		SetScheme( "ClientScheme" );
	InvalidateLayout( false, true );
}

void CCSCareerBaseMenu::ActivateBuildMode()
{
	vgui2::EditablePanel::ActivateBuildMode();
}

void CCSCareerBaseMenu::Activate()
{
	SetPos( 0, 0 );
	vgui2::Frame::Activate();
	if ( !IsProportional() )
		return;

	int screenWide, screenTall, wide, tall;
	vgui2::surface()->GetScreenSize( screenWide, screenTall );
	GetSize( wide, tall );
	if ( wide == screenWide && tall == screenTall )
		return;

	SetSize( screenWide, screenTall );
	wide = GetAlternateProportionalValueFromScaled( wide );
	tall = GetAlternateProportionalValueFromScaled( tall );
	m_offsetX = ( screenWide - wide ) / 2;
	m_offsetY = ( screenTall - tall ) / 2;
	if ( !GetBuildGroup() || !GetBuildGroup()->GetPanelList() )
		return;

	CUtlVector<vgui2::PHandle>* panels = GetBuildGroup()->GetPanelList();
	CUtlVector<vgui2::Panel*> movedPanels;
	CUtlVector<vgui2::Panel*> resizedPanels;
	for ( int i = 0; i < panels->Count(); ++i )
	{
		vgui2::PHandle handle = ( *panels )[i];
		vgui2::Panel* panel = handle.Get();
		bool seen = false;
		for ( int j = 0; j < resizedPanels.Count(); ++j )
		{
			if ( panel == resizedPanels[j] )
				seen = true;
		}
		if ( panel && !seen )
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
		bool seen = false;
		for ( int j = 0; j < movedPanels.Count(); ++j )
		{
			if ( panel == movedPanels[j] )
				seen = true;
		}
		if ( panel && !seen )
		{
			movedPanels.AddToTail( panel );
			if ( panel != this )
			{
				int x, y;
				panel->GetPos( x, y );
				panel->SetPos( m_offsetX + x, m_offsetY + y );
			}
		}
	}
}
