//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Team Menu
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"
#include "vgui_int.h"
#include "vgui_CTFViewport.h"

#include "VGUI_Font.h"
#include "VGUI_TextImage.h"

// Team Menu Dimensions
#define TEAMMENU_TITLE_X			XRES( 40 )
#define TEAMMENU_TITLE_Y			YRES( 32 )
#define TEAMMENU_BUTTON_X			XRES( 40 )
#define TEAMMENU_BUTTON_Y			YRES( 80 )
#define TEAMMENU_BUTTON_WIDE		XRES( 124 )
#define TEAMMENU_BUTTON_TALL		YRES( 24 )
#define TEAMMENU_BUTTON_SPACING	YRES( 8 )
#define TEAMMENU_WINDOW_X			XRES( 176 )
#define TEAMMENU_WINDOW_Y			YRES( 80 )
#define TEAMMENU_WINDOW_WIDE		XRES( 424 )
#define TEAMMENU_WINDOW_TALL		YRES( 312 )
#define TEAMMENU_TEXT_X			XRES( 16 )
#define TEAMMENU_TEXT_Y			YRES( 48 )
#define TEAMMENU_TEXT_TALL		YRES( 178 )
#define TEAMMENU_INFO_Y			YRES( 234 )

CTeamMenuPanel::CTeamMenuPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	// Get the scheme used for the Titles
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t titleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t textScheme = pSchemes->getSchemeHandle( "Briefing Text" );
	SchemeHandle_t infoScheme = pSchemes->getSchemeHandle( "Team Info Text" );
	Font* pTitleFont = pSchemes->getFont( titleScheme );
	int r, g, b, a;

	Label* pTitle = new Label( "", TEAMMENU_TITLE_X, TEAMMENU_TITLE_Y );
	pTitle->setParent( this );
	pTitle->setFont( pTitleFont );
	pSchemes->getFgColor( titleScheme, r, g, b, a );
	pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( titleScheme, r, g, b, a );
	pTitle->setBgColor( r, g, b, a );
	pTitle->setContentAlignment( Label::a_west );
	pTitle->setText( CHudTextMessage::BufferedLocaliseTextString( "#CTFTitle_SelectYourTeam" ) );

	m_pTeamWindow = new CTransparentPanel( 255, TEAMMENU_WINDOW_X, TEAMMENU_WINDOW_Y,
		TEAMMENU_WINDOW_WIDE, TEAMMENU_WINDOW_TALL );
	m_pTeamWindow->setParent( this );
	m_pTeamWindow->setBorder( new LineBorder( Color( 0, 112, 0, 0 ) ) );
	m_pMapTitle = new Label( "", XRES( 16 ), YRES( 16 ) );
	m_pMapTitle->setFont( pTitleFont );
	m_pMapTitle->setParent( m_pTeamWindow );
	pSchemes->getFgColor( titleScheme, r, g, b, a );
	m_pMapTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( titleScheme, r, g, b, a );
	m_pMapTitle->setBgColor( r, g, b, a );
	m_pMapTitle->setContentAlignment( Label::a_west );

	m_pScrollPanel = new CTFScrollPanel( TEAMMENU_TEXT_X, TEAMMENU_TEXT_Y,
		TEAMMENU_WINDOW_WIDE - TEAMMENU_TEXT_X * 2, TEAMMENU_TEXT_TALL );
	m_pScrollPanel->setParent( m_pTeamWindow );
	m_pScrollPanel->setScrollBarVisible( false, false );

	// Create the Map Briefing panel
	m_pBriefing = new TextPanel( "", 0, 0, TEAMMENU_WINDOW_WIDE - TEAMMENU_TEXT_X, TEAMMENU_TEXT_TALL );
	m_pBriefing->setParent( m_pScrollPanel->getClient() );
	m_pBriefing->setFont( pSchemes->getFont( textScheme ) );
	pSchemes->getFgColor( textScheme, r, g, b, a );
	m_pBriefing->setFgColor( r, g, b, a );
	pSchemes->getBgColor( textScheme, r, g, b, a );
	m_pBriefing->setBgColor( r, g, b, a );
	m_pBriefing->setText( "Map Description not available." );

	for ( int team = 1; team <= 3; team++ )
	{
		int buttonY = TEAMMENU_BUTTON_Y + ( TEAMMENU_BUTTON_TALL + TEAMMENU_BUTTON_SPACING ) * team;
		CommandButton* pButton = new CommandButton( "", TEAMMENU_BUTTON_X, buttonY,
			TEAMMENU_BUTTON_WIDE, TEAMMENU_BUTTON_TALL, true );
		m_pButtons[team] = pButton;
		pButton->setParent( this );
		pButton->setContentAlignment( Label::a_west );
		pButton->setVisible( false );
		if ( team == 3 )
		{
			pButton->setBoundKey( '3' );
			pButton->setText( CHudTextMessage::BufferedLocaliseTextString( "#CTFTeam_AutoAssign" ) );
			pButton->setVisible( true );
		}
		char command[256];
		sprintf( command, "jointeam %d", team );
		pButton->addActionSignal( new CMenuHandler_StringCommandWatch( command, true ) );
		pButton->addInputSignal( new CHandler_MenuButtonOver( this, team ) );
		TextPanel* pInfo = new TextPanel( "", TEAMMENU_TEXT_X, TEAMMENU_INFO_Y,
			TEAMMENU_WINDOW_WIDE - TEAMMENU_TEXT_X, TEAMMENU_WINDOW_WIDE - TEAMMENU_INFO_Y );
		m_pTeamInfoPanel[team] = pInfo;
		pInfo->setParent( m_pTeamWindow );
		pInfo->setFont( pSchemes->getFont( infoScheme ) );
		pInfo->setFgColor( iTeamColors[team][0], iTeamColors[team][1], iTeamColors[team][2], 0 );
		pInfo->setBgColor( 0, 0, 0, 255 );
	}

	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#CTFMenu_Cancel" ),
		TEAMMENU_BUTTON_X, 0, TEAMMENU_BUTTON_WIDE, TEAMMENU_BUTTON_TALL );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );
	m_pCancelButton->addActionSignal( new CMenuHandler_StringCommandWatch( "cancelmenu", true ) );
	m_pSpectateButton = new SpectateButton( CHudTextMessage::BufferedLocaliseTextString( "#CTFMenu_Spectate" ),
		TEAMMENU_BUTTON_X, 0, TEAMMENU_BUTTON_WIDE, TEAMMENU_BUTTON_TALL, true );
	m_pSpectateButton->setParent( this );
	m_pSpectateButton->addActionSignal( new CMenuHandler_StringCommand( "spectate", true ) );
	m_pSpectateButton->setBoundKey( '4' );
	m_pSpectateButton->addInputSignal( new CHandler_MenuButtonOver( this, 4 ) );

	m_pTeamFull = new TextPanel( "Team balancing enabled.  Selected team is full.", XRES( 215 ), YRES( 42 ),
		TEAMMENU_WINDOW_WIDE - TEAMMENU_TEXT_X, TEAMMENU_TEXT_TALL );
	m_pTeamFull->setParent( this );
	m_pTeamFull->setFont( pSchemes->getFont( textScheme ) );
	pSchemes->getFgColor( textScheme, r, g, b, a );
	m_pTeamFull->setFgColor( 255, 255, 0, a );
	pSchemes->getBgColor( textScheme, r, g, b, a );
	m_pTeamFull->setBgColor( r, g, b, a );
	int textWide, textTall;
	m_pTeamFull->getTextImage()->getTextSize( textWide, textTall );
	m_pTeamFull->setSize( textWide, textTall );
	m_pTeamFull->setVisible( false );
	Initialize();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CTeamMenuPanel::Initialize()
{
	m_iCurrentInfo = 0;
	m_bUpdatedMapName = false;
	m_pScrollPanel->setScrollValue( 0, 0 );
}

void CTeamMenuPanel::Update()
{
	int buttonY = TEAMMENU_BUTTON_Y;
	for ( int team = 1; team <= 2; team++ )
	{
		if ( !m_pButtons[team] )
			continue;
		if ( team > gViewPort->GetNumberOfTeams() )
		{
			m_pButtons[team]->setVisible( false );
			continue;
		}
		m_pButtons[team]->setText( gViewPort->GetTeamName( team ) );
		char key[256];
		sprintf( key, "%d", team );
		m_pButtons[team]->setBoundKey( key[0] );
		m_pButtons[team]->setVisible( true );
		m_pButtons[team]->setPos( TEAMMENU_BUTTON_X, buttonY );
		buttonY += TEAMMENU_BUTTON_TALL + TEAMMENU_BUTTON_SPACING;

		// Start with the first option up
		if ( !m_iCurrentInfo )
			SetActiveInfo( team );
	}
	m_pButtons[3]->setPos( TEAMMENU_BUTTON_X, buttonY );
	buttonY += TEAMMENU_BUTTON_TALL + TEAMMENU_BUTTON_SPACING;

	// Spectate button
	if ( m_pSpectateButton->IsNotValid() )
	{
		m_pSpectateButton->setVisible( false );
	}
	else
	{
		m_pSpectateButton->setPos( TEAMMENU_BUTTON_X, buttonY );
		m_pSpectateButton->setVisible( true );
		buttonY += TEAMMENU_BUTTON_TALL + TEAMMENU_BUTTON_SPACING;
	}
	if ( gViewPort->m_iCTFTeamNumber )
	{
		m_pCancelButton->setPos( TEAMMENU_BUTTON_X, buttonY );
		m_pCancelButton->setVisible( true );
	}
	else
	{
		m_pCancelButton->setVisible( false );
	}

	// Set the Map Title
	if ( !m_bUpdatedMapName )
	{
		const char* pLevel = gEngfuncs.pfnGetLevelName();
		if ( pLevel && *pLevel )
		{
			char filename[256];
			char title[256];
			strcpy( filename, pLevel );
			char* pSlash = strchr( filename, '/' );
			if ( !pSlash )
				pSlash = strchr( filename, '\\' );
			strcpy( title, pSlash + 1 );
			char* pExtension = strchr( title, '.' );
			*pExtension = 0;
			m_pMapTitle->setText( title );
			*pExtension = '.';
			strcpy( filename, pLevel );
			*strchr( filename, '.' ) = 0;
			strcat( filename, ".txt" );
			char* pFile = (char*)gEngfuncs.COM_LoadFile( filename, 5, NULL );
			if ( pFile )
			{
				m_pBriefing->setText( pFile );
				int textWide, textTall;
				m_pBriefing->getTextImage()->getTextSize( textWide, textTall );
				m_pBriefing->setSize( textWide, textTall );
			}
			m_bUpdatedMapName = true;
		}
	}
	m_pScrollPanel->validate();
}

//=====================================
// Key inputs
bool CTeamMenuPanel::SlotInput( int iSlot )
{
	// Check for AutoAssign
	if ( iSlot == 3 )
	{
		m_pButtons[3]->fireActionSignal();
		return true;
	}

	// Spectate
	if ( iSlot == 4 )
	{
		m_pSpectateButton->fireActionSignal();
		return true;
	}
	if ( iSlot < 1 || iSlot > 2 || !m_pButtons[iSlot] || !m_pButtons[iSlot]->isVisible() )
		return false;
	m_pButtons[iSlot]->fireActionSignal();
	return true;
}

//======================================
// Update the Team menu before opening it
void CTeamMenuPanel::Open()
{
	Update();
	CMenuPanel::Open();
}

void CTeamMenuPanel::paintBackground()
{
	// make sure we get the map briefing up
	if ( !m_bUpdatedMapName )
		Update();
	if ( m_flTeamFullReset != 0 && gHUD.m_flTime >= m_flTeamFullReset )
	{
		m_pTeamFull->setVisible( false );
		m_flTeamFullReset = 0;
	}
	CMenuPanel::paintBackground();
}

//======================================
// Mouse is over a team button, bring up the class info
void CTeamMenuPanel::SetActiveInfo( int iInput )
{
	// Remove all the Info panels and bring up the specified one
	m_pSpectateButton->setArmed( false );
	for ( int team = 1; team <= 3; team++ )
	{
		m_pButtons[team]->setArmed( false );
		m_pTeamInfoPanel[team]->setVisible( false );
	}

	// 6 is Spectate
	if ( iInput == 4 )
	{
		m_pSpectateButton->setArmed( true );
	}
	else
	{
		m_pButtons[iInput]->setArmed( true );
		m_pTeamInfoPanel[iInput]->setVisible( true );
	}
	m_iCurrentInfo = iInput;
	m_pScrollPanel->validate();
}

int CTeamMenuPanel::MsgFunc_TeamFull( const char*, int iSize, void* pbuf )
{
	BEGIN_READ( pbuf, iSize );
	m_pTeamFull->setVisible( READ_BYTE() != 0 );
	m_flTeamFullReset = gHUD.m_flTime + 2.0;
	return 1;
}

void CTeamMenuPanel::Reset()
{
	CMenuPanel::Reset();
	m_iCurrentInfo = 0;
	if ( gHUD.m_flTime >= m_flTeamFullReset )
	{
		m_pTeamFull->setVisible( false );
		m_flTeamFullReset = 0;
	}
}
