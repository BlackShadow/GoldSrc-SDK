#include "creditsmenu.h"
#include "hud.h"
#include "GameUI_Interfaces.h"
#include "GameUI/ICareerUI.h"
#include "GameUI/IMusicManager.h"
#include "IBaseUI.h"
#include "Label.h"
#include "Button.h"
#include "FileSystem.h"
#include "game_controls/iviewport.h"
#include "vgui_controls/Controls.h"
#include "vgui/ISurface.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

class CreditsLabel : public vgui2::Label
{
public:
	CreditsLabel( vgui2::Panel* parent, const char* panelName, const char* text );
	~CreditsLabel() override;
	void SetStyle( const char* style );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;

private:
	char m_style[64];
};

CreditsLabel::CreditsLabel( vgui2::Panel* parent, const char* panelName, const char* text )
	: vgui2::Label( parent, panelName, text )
{
	strcpy( m_style, "Normal" );
}

CreditsLabel::~CreditsLabel()
{
}

void CreditsLabel::SetStyle( const char* style )
{
	_snprintf( m_style, sizeof( m_style ), "%s", style );
}

void CreditsLabel::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Label::ApplySchemeSettings( scheme );
	char name[128];
	_snprintf( name, sizeof( name ), "Credits%sFG", m_style );
	SetFgColor( scheme->GetColor( name, Color( 255, 255, 255, 255 ) ) );
	_snprintf( name, sizeof( name ), "Credits%sBG", m_style );
	SetBgColor( scheme->GetColor( name, Color( 0, 0, 0, 0 ) ) );
	SetBorder( NULL );
	_snprintf( name, sizeof( name ), "Credits%s", m_style );
	SetFont( scheme->GetFont( name, true ) );
	SetContentAlignment( vgui2::Label::a_center );
}

bool CCSCreditsMenu::m_shouldDisconnect;

void ShowCreditsMenu( bool disconnectAfter )
{
	if ( gViewPortInterface )
	{
		CCSCreditsMenu::SetDisconnect( disconnectAfter );
		gViewPortInterface->ShowVGUIMenu( 37 );
	}
}

CCSCreditsMenu::CCSCreditsMenu( vgui2::Panel* parent )
	: vgui2::Frame( parent, "Credits", false, false )
{
	if ( g_pCareerUI && g_pCareerUI->IsPlayingMatch() )
		SetScheme( "CareerClientScheme" );
	else
		SetScheme( "ClientScheme" );
	SetProportional( true );
	int wide, tall;
	vgui2::surface()->GetScreenSize( wide, tall );
	SetSize( wide, tall );
	m_panel = new vgui2::Panel( this );
	SetMenuButtonVisible( false );
	SetMinimizeButtonVisible( false );
	SetCloseButtonVisible( false );
	SetTitle( "", false );
	vgui2::Button* button = new vgui2::Button( this, "Button", "" );
	button->SetBounds( 0, 0, wide, tall );
	button->SetPaintBackgroundEnabled( false );
	button->SetPaintBorderEnabled( false );
	button->MoveToFront();
	button->SetCommand( "Close" );
	button->AddActionSignalTarget( this );
	ReadCreditsFile();
}

CCSCreditsMenu::~CCSCreditsMenu()
{
}

void CCSCreditsMenu::ReadCreditsFile()
{
	FileHandle_t file = vgui2::filesystem()->Open( "credits.txt", "r", NULL );
	if ( !file )
		return;

	int lineNumber = 1;
	char labelName[32];
	char style[64];
	char line[256];
	style[0] = 0;
	while ( vgui2::filesystem()->ReadLine( line, 255, file ) )
	{
		sprintf( labelName, "CreditsLine%d", lineNumber );
		char* end = line + strlen( line ) - 1;
		while ( end > line && end )
		{
			if ( isprint( *end ) )
				break;
			*end-- = 0;
		}
		if ( !strnicmp( line, "@STYLE ", 6 ) && strlen( line ) > 7 )
		{
			_snprintf( style, sizeof( style ), "%s", line + 7 );
			style[sizeof( style ) - 1] = 0;
		}
		else
		{
			if ( line[0] == '\n' || line[0] == '\r' )
				line[0] = 0;
			CreditsLabel* label = new CreditsLabel( m_panel, labelName, line );
			label->SetStyle( style );
			label->SetParent( m_panel );
			m_labelVector.push_back( label );
			++lineNumber;
		}
	}
	vgui2::filesystem()->Close( file );
}

void CCSCreditsMenu::PerformLayout()
{
	int tall = 0;
	int screenWide, screenTall;
	vgui2::surface()->GetScreenSize( screenWide, screenTall );
	for ( std::vector<vgui2::Label*>::iterator i = m_labelVector.begin(); i != m_labelVector.end(); ++i )
	{
		vgui2::Label* label = *i;
		label->SizeToContents();
		int wide, labelTall;
		label->GetSize( wide, labelTall );
		tall += 2;
		label->SetPos( screenWide / 2 - wide / 2, tall );
		tall += labelTall;
	}
	m_panel->SetSize( screenWide, tall );
	m_panel->SetVisible( gEngfuncs.GetAbsoluteTime() >= m_startTime );
}

void CCSCreditsMenu::OnThink()
{
	int screenWide, screenTall, left, top, wide, tall;
	vgui2::surface()->GetScreenSize( screenWide, screenTall );
	float elapsed = (float)( gEngfuncs.GetAbsoluteTime() - m_startTime );
	float elapsedPercent = elapsed * m_panelSpeed;
	m_panel->GetBounds( left, top, wide, tall );
	elapsedPercent = screenTall * elapsedPercent;
	top = screenTall - (int)elapsedPercent;
	float hideAt = m_panelOffscreenStart + m_panelHideTime;
	if ( m_panelOffscreenStart && gEngfuncs.GetAbsoluteTime() > hideAt )
	{
		g_pMusicManager->FadeOutTrack();
		if ( m_shouldDisconnect )
		{
			gEngfuncs.pfnClientCmd( "disconnect\n" );
			g_pCareerUI->OnMatchEndMenuClose( false );
		}
		else
			gEngfuncs.pfnClientCmd( "unpause\n" );
		Close();
	}
	if ( tall + top < 0 && m_panelOffscreenStart == 0 )
		m_panelOffscreenStart = (float)gEngfuncs.GetAbsoluteTime();
	m_panel->SetPos( left, top );
	InvalidateLayout( false, false );
}

void CCSCreditsMenu::ApplySchemeSettings( vgui2::IScheme* scheme )
{
	vgui2::Frame::ApplySchemeSettings( scheme );
	SetBgColor( Color( 0, 0, 0, 255 ) );
}

void CCSCreditsMenu::Activate()
{
	vgui2::Frame::Activate();
	if ( g_pMusicManager->IsPlayingTrack( TRACKTYPE_CREDITS ) )
		return;
	if ( g_pBaseUI )
		g_pBaseUI->HideGameUI();
	int wide, tall;
	vgui2::surface()->GetScreenSize( wide, tall );
	m_panelSpeed = 0.1f;
	m_panelOffscreenStart = 0;
	m_startTime = (float)( gEngfuncs.GetAbsoluteTime() + 1 );
	m_panelHideTime = 1;
	g_pMusicManager->StartTrack( TRACKTYPE_CREDITS, true );
	gEngfuncs.pfnClientCmd( "setpause\n" );
	gViewPortInterface->CloseTutorTextWindow();
}

void CCSCreditsMenu::SetDisconnect( bool disconnectAfter )
{
	m_shouldDisconnect = disconnectAfter;
}

void CCSCreditsMenu::OnCommand( const char* command )
{
	float elapsed = (float)( gEngfuncs.GetAbsoluteTime() - m_startTime );
	if ( !( elapsed >= 3 ) )
		return;
	g_pMusicManager->FadeOutTrack();
	if ( !stricmp( command, "close" ) )
	{
		if ( m_shouldDisconnect )
		{
			gEngfuncs.pfnClientCmd( "disconnect\n" );
			g_pCareerUI->OnMatchEndMenuClose( false );
		}
		else
			gEngfuncs.pfnClientCmd( "unpause\n" );
		Close();
		OnCommand( "CreditsDone" );
	}
	else
		vgui2::Frame::OnCommand( command );
}
