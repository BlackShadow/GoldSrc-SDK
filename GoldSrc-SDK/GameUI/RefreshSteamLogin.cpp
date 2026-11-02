#include "RefreshSteamLogin.h"

#ifndef _WIN32

#include "GameUI_Interface.h"
#include "common/ienginevgui.h"

#include "wrect.h"
#include "cl_dll.h"
#include "Button.h"
#include "CheckButton.h"
#include "MessageBox.h"
#include "SteamCommon.h"
#include "TextEntry.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui_controls/Controls.h"

#include <strings.h>

CRefreshSteamLogin::CRefreshSteamLogin( vgui2::Panel* parent, const char* username )
	: BaseClass( parent, "RefreshSteamLogin", true, false )
{
	m_bLoggingIn = false;
	SetSizeable( false );
	SetSize( 360, 224 );
	SetTitle( "#GameUI_RefreshLogin", true );
	m_pUserName = new vgui2::TextEntry( this, "username" );
	m_pPassword = new vgui2::TextEntry( this, "password" );
	m_pSavePassword = new vgui2::CheckButton( this, "SavePasswordCheck", "" );
	m_pSavePassword->SetSelected( true );
	m_pLoginButton = new vgui2::Button( this, "OKButton", "" );
	LoadControlSettings( "Resource/RefreshLogin.res", NULL );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	m_pUserName->SetEnabled( false );
	m_pUserName->SetText( username );
}

CRefreshSteamLogin::~CRefreshSteamLogin()
{
}

void CRefreshSteamLogin::OnClose()
{
	vgui2::input()->ReleaseAppModalSurface();
	vgui2::Frame::OnClose();
	MarkForDeletion();
}

void CRefreshSteamLogin::Activate()
{
	vgui2::Frame::Activate();
	m_pPassword->RequestFocus( 0 );
	m_RestrictedPaintPanel = GetVPanel();
}

void CRefreshSteamLogin::OnCommand( const char* command )
{
	if ( !strcasecmp( command, "Login" ) )
	{
		char password[128];
		m_pPassword->GetText( password, sizeof( password ) );
		bool savePassword = m_pSavePassword->IsSelected();
		if ( !EngineVGUI()->SteamRefreshLogin( password, savePassword ) )
		{
			vgui2::input()->ReleaseAppModalSurface();
			vgui2::MessageBox* dialog = new vgui2::MessageBox( "#GameUI_Login_ErrorTitle",
				"#GameUI_ErrorLoginFailed", this );
			dialog->SetCommand( "errorclose" );
			dialog->DoModal( NULL );
			m_RestrictedPaintPanel = dialog->GetVPanel();
			return;
		}

		m_bLoggingIn = true;
		m_pPassword->SetEnabled( false );
		m_pLoginButton->SetEnabled( false );
	}
	else if ( !strcasecmp( command, "Close" ) )
	{
		Close();
	}
	else if ( !strcasecmp( command, "errorclose" ) )
	{
		m_RestrictedPaintPanel = GetVPanel();
	}
	else
	{
		vgui2::Frame::OnCommand( command );
	}
}

void CRefreshSteamLogin::OnThink()
{
	vgui2::surface()->RestrictPaintToSinglePanel( m_RestrictedPaintPanel );
	if ( !vgui2::input()->GetFocus()
		|| !vgui2::ipanel()->HasParent( vgui2::input()->GetFocus(), GetVPanel() ) )
		Activate();

	if ( !m_bLoggingIn )
	{
		vgui2::Frame::OnThink();
		return;
	}

	bool finished;
	TSteamProgress progress;
	TSteamError steamError;
	if ( !EngineVGUI()->SteamProcessCall( &finished, &progress, &steamError ) )
	{
		m_pPassword->SetEnabled( true );
		m_pLoginButton->SetEnabled( true );
		m_bLoggingIn = false;
		vgui2::input()->ReleaseAppModalSurface();
		vgui2::MessageBox* dialog = new vgui2::MessageBox( "#GameUI_Login_ErrorTitle",
			"#GameUI_ErrorLoginFailed", this );
		dialog->SetCommand( "errorclose" );
		dialog->DoModal( NULL );
		m_RestrictedPaintPanel = dialog->GetVPanel();
		return;
	}

	if ( finished )
	{
		if ( steamError.eSteamError != eSteamErrorNone )
		{
			vgui2::input()->ReleaseAppModalSurface();
			vgui2::MessageBox* dialog = new vgui2::MessageBox( "#GameUI_Login_ErrorTitle",
				"#GameUI_ErrorLoginFailed", this );
			dialog->SetCommand( "errorclose" );
			dialog->DoModal( NULL );
			m_RestrictedPaintPanel = dialog->GetVPanel();
		}
		else
		{
			engine->pfnClientCmd( "retry\n" );
			Close();
		}

		m_bLoggingIn = false;
		m_pLoginButton->SetEnabled( true );
		m_pPassword->SetEnabled( true );
	}

	vgui2::Frame::OnThink();
}

#endif
