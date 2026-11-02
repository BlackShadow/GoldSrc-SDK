#include "TextEntryBox.h"
#include "CvarTextEntry.h"

#include "Label.h"
#include "TextEntry.h"

#include <string.h>

#ifndef _WIN32
#include <strings.h>
#define stricmp strcasecmp
#endif

CTextEntryBox::CTextEntryBox( const char* title, const char* queryText, const char* entryText,
	bool isCvar, vgui2::Panel* parent ) : BaseClass( title, queryText, parent )
{
	if ( isCvar )
	{
		m_pCvarEntry = new CCvarTextEntry( this, "TextEntry", entryText );
		m_pEntry = m_pCvarEntry;
	}
	else
	{
		m_pEntry = new vgui2::TextEntry( this, "TextEntry" );
		m_pCvarEntry = NULL;
	}
	m_pEntry->SetTabPosition( 3 );
	m_pEntry->RequestFocus( 0 );
	m_pEntry->GotoTextEnd();
}

CTextEntryBox::~CTextEntryBox()
{
	delete m_pEntry;
}

void CTextEntryBox::ShowWindow( vgui2::Frame* frameOver )
{
	BaseClass::ShowWindow( frameOver );
	m_pEntry->RequestFocus( 0 );
	InvalidateLayout( false, false );
}

void CTextEntryBox::PerformLayout()
{
	BaseClass::PerformLayout();
	int x, y, wide, tall;
	GetClientArea( x, y, wide, tall );
	wide += x;
	tall += y;
	int labelWide, labelTall;
	m_pMessageLabel->GetSize( labelWide, labelTall );
	int entryWide = wide - 30 - labelWide;
	if ( entryWide < 120 )
		entryWide = 120;
	int entryTall = labelTall >= 24 ? labelTall : 24;
	m_pEntry->SetSize( entryWide, entryTall );
	int boxWide, boxTall;
	GetSize( boxWide, boxTall );
	if ( labelWide + entryWide + 30 > boxWide )
		SetSize( labelWide + entryWide + 30, boxTall );

	m_pMessageLabel->GetPos( x, y );
	m_pMessageLabel->SetPos( 10, y - ( entryTall - labelTall ) / 2 );
	m_pEntry->SetPos( m_pMessageLabel->GetWide() + 20, y - ( entryTall - labelTall ) );
}

void CTextEntryBox::OnCommand( const char* command )
{
	if ( stricmp( command, "Ok" ) == 0 && m_pCvarEntry )
		m_pCvarEntry->ApplyChanges( true );

	BaseClass::OnCommand( command );
}

void CTextEntryBox::OnKeyCodeTyped( vgui2::KeyCode code )
{
	if ( code == vgui2::KEY_ENTER )
	{
		OnCommand( "Ok" );
		return;
	}
	if ( code == vgui2::KEY_ESCAPE )
		OnCommand( "Cancel" );

	BaseClass::OnKeyCodeTyped( code );
}
