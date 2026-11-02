#include "CDKeyEntryDialog.h"

#ifndef _WIN32

#include "GameUI_Interface.h"

#include "wrect.h"
#include "cl_dll.h"
#include "Button.h"
#include "MessageBox.h"
#include "TextEntry.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui_controls/Controls.h"

#include <string.h>
#include <strings.h>
#include <wctype.h>

class CUpperCaseTextEntry : public vgui2::TextEntry
{
public:
	CUpperCaseTextEntry( vgui2::Panel* parent, const char* panelName )
		: vgui2::TextEntry( parent, panelName )
	{
	}

protected:
	void OnKeyTyped( wchar_t unichar ) override;
};

void CUpperCaseTextEntry::OnKeyTyped( wchar_t unichar )
{
	if ( !iswalnum( unichar ) )
		return;

	if ( unichar >= L'a' && unichar <= L'z' )
		unichar -= L'a' - L'A';
	vgui2::TextEntry::OnKeyTyped( unichar );
}

class CloseMessageBox : public vgui2::MessageBox
{
public:
	CloseMessageBox( const char* title, const char* text, vgui2::Panel* parent )
		: vgui2::MessageBox( title, text, parent )
	{
	}

protected:
	void OnClose() override;
};

void CloseMessageBox::OnClose()
{
	engine->pfnClientCmd( "quit\n" );
}

CCDKeyEntryDialog::CCDKeyEntryDialog( vgui2::Panel* parent, bool inConnect )
	: BaseClass( parent, "CDKeyEntryDialog", true, false )
{
	m_bIsValveCDKey = true;
	m_bInConnect = inConnect;
	m_iErrCount = 0;
	char cdKeyType[64];
	if ( engine->CheckParm( "-forcevalve", NULL )
		|| vgui2::system()->GetRegistryString( "HKEY_CURRENT_USER\\Software\\Valve\\Half-Life\\Settings\\ValveKey",
			cdKeyType, sizeof( cdKeyType ) ) )
		m_bIsValveCDKey = true;

	m_pOK = new vgui2::Button( this, "OKButton", "#GameUI_OK" );
	m_pQuitGame = new vgui2::Button( this, "CancelButton", "#GameUI_Quit" );
	m_pEntry1 = new CUpperCaseTextEntry( this, "Entry1" );
	m_pEntry2 = new CUpperCaseTextEntry( this, "Entry2" );
	m_pEntry3 = new CUpperCaseTextEntry( this, "Entry3" );
	m_pEntry4 = new CUpperCaseTextEntry( this, "Entry4" );
	m_pEntry5 = new CUpperCaseTextEntry( this, "Entry5" );
	if ( m_bIsValveCDKey )
	{
		m_pEntry1->SetMaximumCharCount( 5 );
		m_pEntry2->SetMaximumCharCount( 5 );
		m_pEntry3->SetMaximumCharCount( 5 );
		m_pEntry4->SetMaximumCharCount( 5 );
		m_pEntry5->SetMaximumCharCount( 5 );
	}

	SetSizeable( false );
	SetSize( 360, 224 );
	SetTitle( "#GameUI_CDKey", true );
	LoadControlSettings( m_bIsValveCDKey ? "Resource/ValveCDKeyEntryDialog.res" : "Resource/CDKeyEntryDialog.res", NULL );
	SetMinimizeButtonVisible( false );
	SetMaximizeButtonVisible( false );
	m_pOK->SetEnabled( false );
}

CCDKeyEntryDialog::~CCDKeyEntryDialog()
{
}

void CCDKeyEntryDialog::OnClose()
{
	engine->pfnClientCmd( "quit\n" );
	vgui2::Frame::OnClose();
	MarkForDeletion();
}

void CCDKeyEntryDialog::Activate()
{
	vgui2::Frame::Activate();
	m_pEntry1->RequestFocus( 0 );
}

void CCDKeyEntryDialog::OnCommand( const char* command )
{
	if ( !strcasecmp( command, "OK" ) )
	{
		vgui2::MessageBox* dialog = new vgui2::MessageBox( "#GameUI_CDKey_Invalid_Title",
			"#GameUI_CDKey_Invalid_Text", this );
		dialog->ShowWindow( this );
	}
	else if ( !strcasecmp( command, "Cancel" ) || !strcasecmp( command, "Close" ) )
	{
		vgui2::Frame::OnCommand( "Close" );
	}
	else
	{
		vgui2::Frame::OnCommand( command );
	}

	if ( ++m_iErrCount > MAX_CDKEY_ERRORS )
	{
		CloseMessageBox* dialog = new CloseMessageBox( "#GameUI_CDKey_Invalid_Title",
			"#GameUI_CDKey_TooManyTries", this );
		dialog->ShowWindow( this );
	}
}

void CCDKeyEntryDialog::OnTextChanged( vgui2::Panel* entry )
{
	char cdkey[32];
	bool isValveKey = m_bIsValveCDKey;
	if ( isValveKey )
	{
		m_pEntry1->GetText( cdkey, 6 );
		m_pEntry2->GetText( cdkey + 6, 6 );
		m_pEntry3->GetText( cdkey + 12, 6 );
		m_pEntry4->GetText( cdkey + 18, 6 );
		m_pEntry5->GetText( cdkey + 24, 6 );
		cdkey[5] = '-';
		cdkey[11] = '-';
		cdkey[17] = '-';
		cdkey[23] = '-';
		isValveKey = m_bIsValveCDKey;
		if ( isValveKey && strlen( cdkey ) == 29 )
		{
			m_pOK->SetEnabled( true );
			return;
		}
	}

	memset( cdkey, 0, sizeof( cdkey ) );
	if ( entry == m_pEntry1 )
	{
		m_pEntry1->GetText( cdkey, 6 );
		if ( (int)strlen( cdkey ) >= ( m_bIsValveCDKey ? 5 : 4 ) )
			m_pEntry2->RequestFocus( 0 );
	}
	else if ( entry == m_pEntry2 )
	{
		m_pEntry2->GetText( cdkey, 6 );
		if ( strlen( cdkey ) >= 5 )
			m_pEntry3->RequestFocus( 0 );
	}
	else if ( isValveKey )
	{
		if ( entry == m_pEntry3 )
		{
			m_pEntry3->GetText( cdkey, 6 );
			if ( strlen( cdkey ) >= 5 )
				m_pEntry4->RequestFocus( 0 );
		}
		else if ( entry == m_pEntry4 )
		{
			m_pEntry4->GetText( cdkey, 6 );
			if ( strlen( cdkey ) >= 5 )
				m_pEntry5->RequestFocus( 0 );
		}
	}

	m_pOK->SetEnabled( false );
}

void CCDKeyEntryDialog::OnThink()
{
	vgui2::surface()->RestrictPaintToSinglePanel( GetVPanel() );
	if ( !vgui2::input()->GetFocus()
		|| !vgui2::ipanel()->HasParent( vgui2::input()->GetFocus(), GetVPanel() ) )
		Activate();
	vgui2::Frame::OnThink();
}

bool CCDKeyEntryDialog::IsEnteredKeyValid()
{
	return false;
}

#endif
