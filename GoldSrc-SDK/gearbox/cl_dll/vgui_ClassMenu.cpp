//=========== (C) Copyright 1996-2002 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: TFC Class Menu 
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
#include "vgui_int.h"
#include "vgui_CTFViewport.h"
#include "vgui_StatsMenu.h"

#include "VGUI_Font.h"
#include "VGUI_TextImage.h"

// Class Menu Dimensions
#define CLASSMENU_TITLE_X			XRES( 40 )
#define CLASSMENU_TITLE_Y			YRES( 32 )
#define CLASSMENU_BUTTON_X			XRES( 40 )
#define CLASSMENU_BUTTON_Y			YRES( 80 )
#define CLASSMENU_BUTTON_WIDE		XRES( 124 )
#define CLASSMENU_BUTTON_TALL		YRES( 24 )
#define CLASSMENU_BUTTON_SPACING		YRES( 8 )
#define CLASSMENU_WINDOW_X			XRES( 176 )
#define CLASSMENU_WINDOW_Y			YRES( 80 )
#define CLASSMENU_WINDOW_WIDE		XRES( 424 )
#define CLASSMENU_WINDOW_TALL		YRES( 312 )
#define CLASSMENU_TEXT_X				XRES( 150 )
#define CLASSMENU_TEXT_Y				YRES( 80 )

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
CClassMenuPanel::CClassMenuPanel( int iTrans, int iRemoveMe, int x, int y, int wide, int tall )
	: CMenuPanel( iTrans, iRemoveMe, x, y, wide, tall )
{
	bool showGraphic = ScreenWidth >= 640;
	memset( m_pClassImages, 0, sizeof( m_pClassImages ) );

	// Get the scheme used for the Titles
	CSchemeManager* pSchemes = gViewPort->GetSchemeManager();
	SchemeHandle_t titleScheme = pSchemes->getSchemeHandle( "Title Font" );
	SchemeHandle_t textScheme = pSchemes->getSchemeHandle( "Briefing Text" );
	int r, g, b, a;

	Label* pTitle = new Label( "", CLASSMENU_TITLE_X, CLASSMENU_TITLE_Y );
	pTitle->setParent( this );
	pTitle->setFont( pSchemes->getFont( titleScheme ) );
	pSchemes->getFgColor( titleScheme, r, g, b, a );
	pTitle->setFgColor( r, g, b, a );
	pSchemes->getBgColor( titleScheme, r, g, b, a );
	pTitle->setBgColor( r, g, b, a );
	pTitle->setContentAlignment( Label::a_west );
	pTitle->setText( CHudTextMessage::BufferedLocaliseTextString( "#CTFTitle_SelectYourCharacter" ) );

	m_pScrollPanel = new CTFScrollPanel( CLASSMENU_WINDOW_X, CLASSMENU_WINDOW_Y,
		CLASSMENU_WINDOW_WIDE, CLASSMENU_WINDOW_TALL );
	m_pScrollPanel->setParent( this );
	//force the scrollbars on, so after the validate clientClip will be smaller
	m_pScrollPanel->setScrollBarAutoVisible( false, false );
	m_pScrollPanel->setScrollBarVisible( true, true );
	m_pScrollPanel->setBorder( new LineBorder( Color( 0, 112, 0, 0 ) ) );
	m_pScrollPanel->validate();
	int clientWide = m_pScrollPanel->getClient()->getWide();

	//turn scrollpanel back into auto show scrollbar mode and validate
	m_pScrollPanel->setScrollBarAutoVisible( false, true );
	m_pScrollPanel->setScrollBarVisible( false, false );
	m_pScrollPanel->validate();

	for ( int team = 0; team < 2; team++ )
	{
		for ( int character = 0; character < 7; character++ )
		{
			char text[256];
			sprintf( text, "selectchar %d", character + 1 );
			ActionSignal* pSelect = new CMenuHandler_StringCommandClassSelect( text, true );
			strcpy( text, CHudTextMessage::BufferedLocaliseTextString( sLocalisedClasses[team][character] ) );
			int buttonY = CLASSMENU_BUTTON_Y + ( CLASSMENU_BUTTON_TALL + CLASSMENU_BUTTON_SPACING ) * character;
			ClassButton* pButton = new ClassButton( character, text, CLASSMENU_BUTTON_X, buttonY,
				CLASSMENU_BUTTON_WIDE, CLASSMENU_BUTTON_TALL, true );
			m_pButtons[team][character] = pButton;
			sprintf( text, "%d", character + 1 );
			pButton->setBoundKey( text[0] );
			pButton->setContentAlignment( Label::a_west );
			pButton->addActionSignal( pSelect );
			pButton->addInputSignal( new CHandler_MenuButtonOver( this, character ) );
			pButton->setParent( this );

			CTransparentPanel* pInfo = new CTransparentPanel( 255, 0, 0, clientWide, CLASSMENU_WINDOW_TALL );
			m_pClassInfoPanel[team][character] = pInfo;
			pInfo->setParent( m_pScrollPanel->getClient() );
			int textX = showGraphic ? CLASSMENU_TEXT_X : XRES( 8 );
			sprintf( text, "#CTFTitle_%s", sCTFClassSelection[team][character] );
			char* pName = CHudTextMessage::BufferedLocaliseTextString( text );
			Label* pNameLabel = new Label( "", textX, YRES( 8 ) );
			pNameLabel->setFont( pSchemes->getFont( titleScheme ) );
			pNameLabel->setParent( pInfo );
			pSchemes->getFgColor( titleScheme, r, g, b, a );
			pNameLabel->setFgColor( r, g, b, a );
			pSchemes->getBgColor( titleScheme, r, g, b, a );
			pNameLabel->setBgColor( r, g, b, a );
			pNameLabel->setContentAlignment( Label::a_west );
			pNameLabel->setText( pName );

			if ( showGraphic )
			{
				sprintf( text, sCTFClassSelection[team][character] );
				CImageLabel* pImage = new CImageLabel( text, 0, 0, CLASSMENU_TEXT_X, CLASSMENU_TEXT_Y );
				m_pClassImages[team][character] = pImage;
				pImage->setParent( pInfo );
				pImage->setVisible( false );
				int nameWide, nameTall;
				pNameLabel->getTextSize( nameWide, nameTall );
				pImage->setPos( ( CLASSMENU_TEXT_X - pImage->getWide() ) / 2, nameTall / 2 );
			}

			sprintf( text, "classes/short_%s.txt", sCTFClassSelection[team][character] );
			char* pFile = (char*)gEngfuncs.COM_LoadFile( text, 5, NULL );
			const char* pDescription = pFile ? pFile : "Class Description not available.";
			TextPanel* pText = new TextPanel( pDescription, textX, CLASSMENU_TEXT_Y,
				CLASSMENU_WINDOW_WIDE - textX - 5, CLASSMENU_WINDOW_TALL - CLASSMENU_TEXT_Y );
			pText->setParent( pInfo );
			pText->setFont( pSchemes->getFont( textScheme ) );
			pSchemes->getFgColor( textScheme, r, g, b, a );
			pText->setFgColor( r, g, b, a );
			pSchemes->getBgColor( textScheme, r, g, b, a );
			pText->setBgColor( r, g, b, a );
			int textWide, textTall;
			pText->getTextImage()->getTextSizeWrapped( textWide, textTall );
			pText->setSize( textWide, textTall );
			int textLeft, textTop;
			pText->getPos( textLeft, textTop );
			int infoWide = textLeft + textWide;
			int infoTall = textTop + textTall;
			if ( m_pClassImages[0][character] )
			{
				m_pClassImages[0][character]->getPos( textLeft, textTop );
				if ( textTop + m_pClassImages[0][character]->getTall() > infoTall )
					infoTall = textTop + m_pClassImages[0][character]->getTall();
			}
			pInfo->setSize( infoWide, infoTall );
		}
	}

	m_pCancelButton = new CommandButton( CHudTextMessage::BufferedLocaliseTextString( "#CTFMenu_Cancel" ),
		CLASSMENU_BUTTON_X, 0, CLASSMENU_BUTTON_WIDE, CLASSMENU_BUTTON_TALL );
	m_pCancelButton->setParent( this );
	m_pCancelButton->addActionSignal( new CMenuHandler_TextWindow( HIDE_TEXTWINDOW ) );
	m_pCancelButton->addActionSignal( new CMenuHandler_StringCommandWatch( "cancelmenu", true ) );
	m_iCurrentInfo = 0;
}

void CClassMenuPanel::Update()
{
	if ( (unsigned int)gViewPort->m_iCTFTeamNumber - 1 >= sizeof( m_pButtons ) / sizeof( m_pButtons[0] ) )
		return;

	int buttonY = CLASSMENU_BUTTON_Y;
	for ( int team = 0; team < 2; team++ )
	{
		for ( int character = 0; character < 7; character++ )
		{
			if ( team == gViewPort->m_iCTFTeamNumber - 1 )
			{
				m_pButtons[team][character]->setVisible( true );
				m_pButtons[team][character]->setPos( CLASSMENU_BUTTON_X, buttonY );
				buttonY += CLASSMENU_BUTTON_TALL + CLASSMENU_BUTTON_SPACING;

				// Start with the first option up
				if ( !m_iCurrentInfo )
					SetActiveInfo( character );
			}
			else
			{
				m_pButtons[team][character]->setVisible( false );
			}
			CImageLabel* pImage = m_pClassImages[gViewPort->m_iCTFTeamNumber - 1][character];
			if ( pImage )
				pImage->setVisible( true );
		}
	}
	m_pCancelButton->setPos( CLASSMENU_BUTTON_X, buttonY );
	m_pCancelButton->setVisible( true );
}

bool CClassMenuPanel::SlotInput( int iSlot )
{
	if ( iSlot < 1 || iSlot > 7 )
		return false;
	if ( (unsigned int)gViewPort->m_iCTFTeamNumber - 1 >= sizeof( m_pButtons ) / sizeof( m_pButtons[0] ) )
		return false;
	ClassButton* pButton = m_pButtons[gViewPort->m_iCTFTeamNumber - 1][iSlot - 1];
	if ( !pButton->isVisible() )
		return false;
	pButton->fireActionSignal();
	return true;
}

//======================================
// Update the Class menu before opening it
void CClassMenuPanel::Open()
{
	Update();
	CMenuPanel::Open();
}

//-----------------------------------------------------------------------------
// Purpose: Called each time a new level is started.
//-----------------------------------------------------------------------------
void CClassMenuPanel::Initialize()
{
	setVisible( false );
	m_pScrollPanel->setScrollValue( 0, 0 );
}

void CClassMenuPanel::SetActiveInfo( int iInput )
{
	for ( int team = 0; team < 2; team++ )
	{
		for ( int character = 0; character < 7; character++ )
		{
			m_pButtons[team][character]->setArmed( false );
			m_pClassInfoPanel[team][character]->setVisible( false );
		}
	}
	if ( (unsigned int)iInput >= sizeof( m_pButtons[0] ) / sizeof( m_pButtons[0][0] ) )
		iInput = 0;
	int team = gViewPort->m_iCTFTeamNumber;
	if ( (unsigned int)team - 1 >= sizeof( m_pButtons ) / sizeof( m_pButtons[0] ) )
		return;
	team--;
	m_pButtons[team][iInput]->setArmed( true );
	m_pClassInfoPanel[team][iInput]->setVisible( true );
	if ( m_iCurrentInfo != iInput )
		gViewPort->m_pStatsMenu->SetPlayerImage( sCTFClassSelection[team][iInput] );
	m_iCurrentInfo = iInput;
	m_pScrollPanel->setScrollValue( 0, 0 );
	m_pScrollPanel->validate();
}
