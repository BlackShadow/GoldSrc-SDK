//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:  Implementation of CHTMLFile. see HTML.h for details
//
// $Workfile:     $
// $Date:         $
//
//------------------------------------------------------------------------------------------------------
#include "HTML.h"

#include "Frame.h"
#include "Label.h"
#include "Menu.h"
#include "MessageBox.h"
#include "QueryBox.h"
#include "ScrollBar.h"
#include "TextEntry.h"
#include "controls.h"

#include "color.h"
#include "vgui/IBorder.h"
#include "vgui/IInput.h"
#include "vgui/IPanel.h"
#include "vgui/IScheme.h"
#include "vgui/ISurface.h"
#include "vgui/ISystem.h"
#include "vgui/IVGui.h"
#include "vgui/KeyValues.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

int ConvertMouseCodeToCEFCode( vgui2::MouseCode code )
{
	if ( code == vgui2::MOUSE_RIGHT )
		return 2;
	return code == vgui2::MOUSE_MIDDLE;
}

int GetKeyModifiers()
{
	int modifiers = 0;
	if ( vgui2::input()->IsKeyDown( vgui2::KEY_LALT ) || vgui2::input()->IsKeyDown( vgui2::KEY_RALT ) )
		modifiers |= 1;
	if ( vgui2::input()->IsKeyDown( vgui2::KEY_LCONTROL ) || vgui2::input()->IsKeyDown( vgui2::KEY_RCONTROL ) )
		modifiers |= 2;
	if ( vgui2::input()->IsKeyDown( vgui2::KEY_LSHIFT ) || vgui2::input()->IsKeyDown( vgui2::KEY_RSHIFT ) )
		modifiers |= 4;
	return modifiers;
}

static void AssignString( CUtlString& destination, const char* value )
{
	std::free( destination.m_pchString );
	if ( value != NULL && value[0] != '\0' )
		destination.m_pchString = _strdup( value );
	else
		destination.m_pchString = NULL;
}

static const char* StringOrEmpty( const CUtlString& value )
{
	if ( value.m_pchString != NULL )
		return value.m_pchString;
	return "";
}

static int SchemeInteger( vgui2::IScheme* scheme, const char* name, int defaultValue )
{
	const char* text = scheme->GetResourceString( name );
	if ( text == NULL )
		return defaultValue;
	return (int)std::strtol( text, NULL, 10 );
}

namespace vgui2
{
HTML::CHTMLFindBar::CHTMLFindBar( HTML* parent )
	: EditablePanel( parent, "FindBar" ), m_pFindBar( NULL ), m_pParent( parent ), m_pFindCountLabel( NULL ),
	  m_bHidden( false )
{
	TextEntry* entry = new TextEntry( this, "FindEntry" );
	m_pFindBar = entry;
	entry->AddActionSignalTarget( parent );
	entry->SendNewLine( true );
	Label* count = new Label( this, "FindCount", "" );
	m_pFindCountLabel = count;
	count->SetVisible( false );
	LoadControlSettings( "resource/layout/htmlfindbar.layout", NULL );
}

HTML::CHTMLFindBar::~CHTMLFindBar()
{
}

void HTML::CHTMLFindBar::OnCommand( const char* pchCmd )
{
	if ( _stricmp( pchCmd, "close" ) == 0 )
	{
		m_pParent->HideFindDialog();
	}
	else if ( _stricmp( pchCmd, "previous" ) == 0 )
	{
	}
	else if ( _stricmp( pchCmd, "next" ) == 0 )
	{
		m_pParent->FindNext();
	}
	else
	{
		Panel::OnCommand( pchCmd );
	}
}
}

HTMLInterior::HTMLInterior( vgui2::HTML* parent ) : vgui2::Panel( parent, "HTMLInterior" ), m_pHTML( parent )
{
	SetPaintBackgroundEnabled( false );
	SetKeyBoardInputEnabled( false );
	SetMouseInputEnabled( false );
}

HTMLInterior::~HTMLInterior()
{
}

HTMLComboBoxHost::HTMLComboBoxHost( vgui2::HTML* parent )
	: vgui2::EditablePanel( parent, "ComboBoxHost" ), m_pParent( parent )
{
	MakePopup( false, false );
}

HTMLComboBoxHost::~HTMLComboBoxHost()
{
}

void HTMLComboBoxHost::PerformLayout()
{
}

void HTMLComboBoxHost::PaintBackground()
{
	vgui2::EditablePanel::PaintBackground();
	if ( m_pParent->m_iComboBoxTextureID != 0 )
	{
		vgui2::surface()->DrawSetTexture( m_pParent->m_iComboBoxTextureID );
		Color color;
		color.SetRawColor( -1 );
		vgui2::surface()->DrawSetColor( color );
		vgui2::surface()->DrawTexturedRect(
			0, 0, m_pParent->m_allocedComboBoxWidth, m_pParent->m_allocedComboBoxHeight );
	}
}

void HTMLComboBoxHost::OnKillFocus()
{
	if ( vgui2::input()->GetFocus() != m_pParent->GetVPanel() )
	{
		vgui2::Panel::OnKillFocus();
		if ( m_pParent != NULL )
			m_pParent->m_pComboBoxHost->SetVisible( false );
	}
}

void HTMLComboBoxHost::OnCursorMoved( int x, int y )
{
	if ( vgui2::input()->GetMouseOver() == GetVPanel() )
		m_pParent->m_Serializer->MouseMove( x, y );
}

void HTMLComboBoxHost::OnMousePressed( vgui2::MouseCode code )
{
	m_pParent->OnMousePressed( code );
}

void HTMLComboBoxHost::OnMouseDoublePressed( vgui2::MouseCode code )
{
	m_pParent->OnMouseDoublePressed( code );
}

void HTMLComboBoxHost::OnMouseReleased( vgui2::MouseCode code )
{
	m_pParent->OnMouseReleased( code );
}

void HTMLComboBoxHost::OnMouseWheeled( int delta )
{
	m_pParent->OnMouseWheeled( delta );
}

void HTMLComboBoxHost::OnKeyCodeTyped( vgui2::KeyCode code )
{
	m_pParent->OnKeyCodeTyped( code );
}

void HTMLComboBoxHost::OnKeyTyped( wchar_t unichar )
{
	m_pParent->OnKeyTyped( unichar );
}

void HTMLComboBoxHost::OnKeyCodeReleased( vgui2::KeyCode code )
{
	m_pParent->OnKeyCodeReleased( code );
}

namespace vgui2
{
#ifdef _MSC_VER
#pragma warning( suppress : 26495 )
#endif
HTML::HTML( Panel* parent, const char* name, bool allowJavaScript, bool bPopupWindow )
	: Panel( parent, name ), m_Serializer( NULL ), m_pContextMenu( NULL ), m_pInteriorPanel( NULL ),
	  _hbar( NULL ), _vbar( NULL ), m_pFindBar( NULL ), m_pComboBoxHost( NULL ), m_iMouseX( 0 ),
	  m_iMouseY( 0 ), m_iScrollBorderX( 0 ), m_iScrollBorderY( 0 ), m_iWideLastHTMLSize( 0 ), m_iTalLastHTMLSize( 0 ),
	  m_iCopyLinkMenuItemID( 0 ), m_bScrollBarEnabled( true ), m_bContextMenuEnabled( true ),
	  m_bNewWindowsOnly( false ), m_nViewSourceAllowedIndex( -1 ), m_iDragStartX( 0 ), m_iDragStartY( 0 ),
	  m_iHTMLTextureID( 0 ), m_iComboBoxTextureID( 0 ), m_bInFind( false ), m_bCanGoBack( false ),
	  m_bCanGoForward( false ), m_bRequestingDragURL( false ), m_bRequestingCopyLink( false ), m_flZoom( 100.0f )
{
	( void )allowJavaScript;
	m_LinkAtPos.m_nX = 0;
	m_LinkAtPos.m_nY = 0;
	m_scrollHorizontal.m_bVisible = false;
	m_scrollHorizontal.m_nX = 0;
	m_scrollHorizontal.m_nY = 0;
	m_scrollHorizontal.m_nWide = 0;
	m_scrollHorizontal.m_nTall = 0;
	m_scrollHorizontal.m_nMax = 0;
	m_scrollHorizontal.m_nScroll = 0;
	m_scrollVertical.m_bVisible = false;
	m_scrollVertical.m_nX = 0;
	m_scrollVertical.m_nY = 0;
	m_scrollVertical.m_nWide = 0;
	m_scrollVertical.m_nTall = 0;
	m_scrollVertical.m_nMax = 0;
	m_scrollVertical.m_nScroll = 0;

	m_pInteriorPanel = new HTMLInterior( this );
	SetPostChildPaintEnabled( true );
	if ( surface() != NULL )
	{
		IHTMLChromeController* controller = surface()->AccessChromeHTMLController();
		if ( controller != NULL )
		{
			m_Serializer = controller->CreateSerializer( this );
			controller->CreateBrowser(
				m_Serializer->GetResponseTarget(),
				bPopupWindow, "Valve Half-Life" );
		}
	}

	_hbar = new ScrollBar( this, "HorizScrollBar", false );
	_hbar->SetVisible( false );
	_hbar->AddActionSignalTarget( this );
	_vbar = new ScrollBar( this, "VertScrollBar", true );
	_vbar->SetVisible( false );
	_vbar->AddActionSignalTarget( this );

	m_pFindBar = new CHTMLFindBar( this );
	m_pFindBar->SetZPos( 2 );
	m_pFindBar->SetVisible( false );

	m_pComboBoxHost = new HTMLComboBoxHost( this );
	m_pComboBoxHost->SetPaintBackgroundEnabled( true );
	m_pComboBoxHost->SetVisible( false );

	Menu* menu = new Menu( this, "contextmenu" );
	m_pContextMenu = menu;
	menu->AddMenuItem( "#vgui_HTMLBack", new KeyValues( "Command", "command", "back" ), this, NULL );
	menu->AddMenuItem( "#vgui_HTMLForward", new KeyValues( "Command", "command", "forward" ), this, NULL );
	menu->AddMenuItem( "#vgui_HTMLReload", new KeyValues( "Command", "command", "reload" ), this, NULL );
	menu->AddMenuItem( "#vgui_HTMLStop", new KeyValues( "Command", "command", "stop" ), this, NULL );
	menu->AddMenuItem( "#vgui_HTMLCopyUrl", new KeyValues( "Command", "command", "copyurl" ), this, NULL );
	m_iCopyLinkMenuItemID =
		menu->AddMenuItem( "#vgui_HTMLCopyLink", new KeyValues( "Command", "command", "copylink" ), this, NULL );
	menu->SetItemVisible( m_iCopyLinkMenuItemID, false );
}

HTML::~HTML()
{
	m_pContextMenu->MarkForDeletion();
	if ( surface() != NULL )
	{
		IHTMLChromeController* controller = surface()->AccessChromeHTMLController();
		if ( controller != NULL )
		{
			controller->RemoveBrowser(
				m_Serializer->GetResponseTarget() );
			m_Serializer = NULL;
		}
	}
}

void HTML::OnSetCursorVGUI( int cursor )
{
	SetCursor( (HCursor)cursor );
}

bool HTML::StopLoading()
{
	m_Serializer->StopLoad();
	return true;
}

bool HTML::Refresh()
{
	m_Serializer->Reload();
	return true;
}

void HTML::GoBack()
{
	m_Serializer->GoBack();
}

void HTML::GoForward()
{
	m_Serializer->GoForward();
}

bool HTML::BCanGoBack()
{
	return m_bCanGoBack;
}

bool HTML::BCanGoFoward()
{
	return m_bCanGoForward;
}

void HTML::RunJavascript( const char* pchScript )
{
	m_Serializer->RunJavaScript( pchScript );
}

void HTML::OnMouseDoublePressed( MouseCode code )
{
	m_Serializer->MouseDoubleClick( ConvertMouseCodeToCEFCode( code ) );
}

//-----------------------------------------------------------------------------
// Purpose: passes key presses to the browser (we don't current do this)
//-----------------------------------------------------------------------------
void HTML::OnKeyTyped( wchar_t unichar )
{
	m_Serializer->KeyChar( unichar );
}

void HTML::OnMouseWheeled( int delta )
{
	if ( _vbar != NULL && m_pComboBoxHost != NULL && !m_pComboBoxHost->IsVisible() )
	{
		double value = (double)_vbar->GetValue() - (double)delta * 100.0 / 3.0;
		_vbar->SetValue( (int)value );
	}
	m_Serializer->MouseWheel( delta );
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetScrollbarsEnabled( bool state )
{
	m_bScrollBarEnabled = state;
}

//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
void HTML::SetContextMenuEnabled( bool state )
{
	m_bContextMenuEnabled = state;
}

void HTML::SetViewSourceEnabled( bool state )
{
	m_pContextMenu->SetItemVisible( m_nViewSourceAllowedIndex, state );
}

void HTML::NewWindowsOnly( bool state )
{
	m_bNewWindowsOnly = state;
}

void HTML::OnEditNewLine( Panel* panel )
{
	OnTextChanged( panel );
}

void HTML::BrowserStartRequest( const char* url, const char* target, const char* postdata, bool isredirect )
{
	bool allow = OnStartRequest( url, target, postdata, isredirect );
	m_Serializer->StartRequestResponse( allow );
	UpdateCachedHTMLValues();
}

void HTML::BrowserShowPopup()
{
	m_pComboBoxHost->SetVisible( true );
}

void HTML::BrowserHidePopup()
{
	m_pComboBoxHost->SetVisible( false );
}

void HTML::BrowserPopupHTMLWindow( const char*, int, int, int, int )
{
}

void HTML::BrowserSetCursor( int in_cursor )
{
	if ( (unsigned int)in_cursor > 0xF )
		in_cursor = 2;
	SetCursor( (HCursor)in_cursor );
}

void HTML::BrowserUpdateToolTip( const char* )
{
}

void HTML::BrowserShowToolTip( const char* )
{
}

void HTML::BrowserFileLoadDialog()
{
}

void HTML::BrowserHideToolTip()
{
}

void HTML::BrowserGetZoomResponse( float flZoom )
{
	if ( flZoom == 0.0f )
		m_flZoom = 1.0f;
	else
		m_flZoom = flZoom / 100.0f;
}

void HTML::DismissJSDialog( int bResult )
{
	m_Serializer->JSDialogResponse( bResult );
}

void HTML::BrowserCanGoBackandForward( bool bgoback, bool bgoforward )
{
	m_bCanGoBack = bgoback;
	m_bCanGoForward = bgoforward;
}

void HTML::UpdateCachedHTMLValues()
{
	m_Serializer->RequestBrowserSizes();
}

void HTML::BrowserLoadingResource()
{
	UpdateCachedHTMLValues();
}

void HTML::BrowserNeedsPaint( int textureid, int wide, int tall, const unsigned char* rgba, int updatex, int updatey,
	int updatewide, int updatetall, int combobox_wide, int combobox_tall, const unsigned char* combobox_rgba )
{
	if ( m_iHTMLTextureID == 0 || m_allocedTextureWidth != wide || m_allocedTextureHeight != tall )
	{
		if ( m_iHTMLTextureID != 0 )
			surface()->DeleteTextureByID( m_iHTMLTextureID );
		m_iHTMLTextureID = surface()->CreateNewTextureID( true );
		surface()->DrawSetTextureBGRA( m_iHTMLTextureID, rgba, wide, tall );
		m_allocedTextureWidth = wide;
		m_allocedTextureHeight = tall;
	}
	else
	{
		surface()->DrawUpdateRegionTextureBGRA( m_iHTMLTextureID, updatex, updatey, rgba, updatewide, updatetall );
	}

	if ( m_pComboBoxHost->IsVisible() )
	{
		if ( m_iComboBoxTextureID != 0 && m_allocedComboBoxWidth == combobox_wide
			 && m_allocedComboBoxHeight == combobox_tall )
		{
			surface()->DrawUpdateRegionTextureBGRA(
				m_iComboBoxTextureID, 0, 0, combobox_rgba, combobox_wide, combobox_tall );
		}
		else
		{
			if ( m_iComboBoxTextureID != 0 )
				surface()->DeleteTextureByID( m_iComboBoxTextureID );
			m_iComboBoxTextureID = surface()->CreateNewTextureID( true );
			surface()->DrawSetTextureBGRA( m_iComboBoxTextureID, combobox_rgba, combobox_wide, combobox_tall );
			m_allocedComboBoxWidth = combobox_wide;
			m_allocedComboBoxHeight = combobox_tall;
		}
	}

	Repaint();
	m_Serializer->NeedsPaintResponse( textureid );
}

void HTML::BrowserSetHTMLTitle( const char* title )
{
	PostMessage( GetParent(), new KeyValues( "OnSetHTMLTitle", "title", title ), 0.0f );
	OnSetHTMLTitle( title );
}

void HTML::BrowserStatusText( const char* text )
{
	PostActionSignal( new KeyValues( "OnSetStatusText", "status", text ) );
}

void HTML::OnSetFocus()
{
	Panel::OnSetFocus();
	m_Serializer->SetFocus( true );
}

void HTML::PostChildPaint()
{
}

void HTML::Paint()
{
	Panel::Paint();
	if ( m_iHTMLTextureID != 0 )
	{
		int wide = 0;
		int tall = 0;
		GetSize( wide, tall );
		Color color;
		color.SetRawColor( -1 );
		surface()->DrawSetColor( color );
		surface()->DrawFilledRect( 0, 0, wide, tall );
		surface()->DrawSetTexture( m_iHTMLTextureID );
		surface()->DrawTexturedRect( 0, 0, wide, tall );
	}
	if ( m_iScrollBorderX > 0 || m_iScrollBorderY > 0 )
	{
		int wide = 0;
		int tall = 0;
		GetSize( wide, tall );
		IBorder* border = GetBorder();
		int left = 0;
		int top = 0;
		int right = 0;
		int bottom = 0;
		if ( border != NULL )
			border->GetInset( left, top, right, bottom );
		surface()->DrawSetColor( GetBgColor() );
		if ( m_iScrollBorderX != 0 )
			surface()->DrawFilledRect( wide - m_iScrollBorderX - right, top, wide, tall - bottom );
		if ( m_iScrollBorderY != 0 )
			surface()->DrawFilledRect( left, tall - m_iScrollBorderY - bottom, wide - m_iScrollBorderX - right, tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: causes a repaint when the layout changes
//-----------------------------------------------------------------------------
void HTML::PerformLayout()
{
	Panel::PerformLayout();
	Repaint();
	int scrollWide = 0;
	if ( _vbar->IsVisible() )
		scrollWide = _vbar->GetWide();
	m_pInteriorPanel->SetBounds( 0, 0, GetWide() - scrollWide, GetTall() );

	IScheme* clientScheme = scheme()->GetIScheme( scheme()->GetScheme( "ClientScheme" ) );
	int insetY = SchemeInteger( clientScheme, "HTML.SearchInsetY", 5 );
	int insetX = SchemeInteger( clientScheme, "HTML.SearchInsetX", 5 );
	int searchTall = SchemeInteger( clientScheme, "HTML.SearchTall", 24 );
	int searchWide = SchemeInteger( clientScheme, "HTML.SearchWide", 150 );
	int searchY = insetY;
	if ( m_pFindBar->m_bHidden )
		searchY = -5 - searchTall;
	m_pFindBar->SetBounds( GetWide() - searchWide - insetX - scrollWide, searchY, searchWide, searchTall );
}

void HTML::OnSliderMoved()
{
	if ( _hbar->IsVisible() )
		m_Serializer->SetHorizontalScroll( _hbar->GetValue() );
	if ( _vbar->IsVisible() )
		m_Serializer->SetVerticalScroll( _vbar->GetValue() );
	PostActionSignal( new KeyValues( "HTMLSliderMoved" ) );
}

void HTML::BrowserClose()
{
	PostActionSignal( new KeyValues( "OnCloseWindow" ) );
}

void HTML::CalcScrollBars( int w, int h )
{
	bool wasVisible = _vbar->IsVisible();
	if ( !m_bScrollBarEnabled )
	{
		m_iScrollBorderX = 0;
		m_iScrollBorderY = 0;
		_vbar->SetVisible( false );
		_hbar->SetVisible( false );
	}
	else
	{
		int which = 0;
		for ( ;; )
		{
			int panelWide = 0;
			int panelTall = 0;
			int span = 0;
			int scrollWide = 0;
			ScrollBar* bar = NULL;
			if ( which != 0 )
			{
				int tall = m_scrollVertical.m_nTall;
				span = m_scrollHorizontal.m_bVisible ? tall + 16 : tall - 2;
				scrollWide = m_scrollVertical.m_nWide + 5;
				if ( scrollWide == 0 || !m_scrollVertical.m_bVisible || span == 0 )
				{
					m_iScrollBorderX = 0;
					_vbar->SetVisible( false );
					break;
				}
				GetSize( panelWide, panelTall );
				bar = _vbar;
			}
			else
			{
				int horizontalTall = m_scrollHorizontal.m_nTall;
				span = horizontalTall + 5;
				scrollWide = m_scrollHorizontal.m_nWide + 14;
				if ( scrollWide == 0 || horizontalTall == -5 || !m_scrollHorizontal.m_bVisible )
				{
					m_iScrollBorderY = 0;
					_hbar->SetVisible( false );
					which = 1;
					continue;
				}
				GetSize( panelWide, panelTall );
				bar = _hbar;
			}

			if ( !bar->IsVisible() )
			{
				bar->SetVisible( true );
				PostMessage( this, new KeyValues( "OnSliderMoved" ), 0.02f );
			}

			int rangeWindow = panelTall - scrollWide;
			int range = 0;
			int current = 0;
			if ( which != 0 )
			{
				range = m_scrollVertical.m_nMax + m_scrollVertical.m_nTall;
				current = m_scrollVertical.m_nScroll;
			}
			else
			{
				current = m_scrollHorizontal.m_nScroll;
				rangeWindow = panelWide - span;
				range = m_scrollHorizontal.m_nMax + m_scrollVertical.m_nWide;
			}
			bar->SetEnabled( false );
			bar->SetRangeWindow( rangeWindow );
			bar->SetRange( 0, range );
			bar->SetButtonPressedScrollValue( 5 );
			int value = bar->GetValue();
			if ( current > value + 5 || current < bar->GetValue() - 5 )
				bar->SetValue( current );

			if ( which != 0 )
			{
				bar->SetPos( w - scrollWide, 0 );
				bar->SetTall( span );
				bar->SetWide( scrollWide );
				m_iScrollBorderX = scrollWide;
				break;
			}

			which = 1;
			bar->SetPos( 0, h - span - 1 );
			bar->SetWide( scrollWide );
			bar->SetTall( span );
			m_iScrollBorderY = span;
		}
	}

	if ( wasVisible != _vbar->IsVisible() )
		InvalidateLayout( false, false );
}

void HTML::BrowserResize()
{
	int wide = 0;
	int tall = 0;
	GetSize( wide, tall );
	int htmlWide = wide - m_iScrollBorderX;
	int htmlTall = tall - m_iScrollBorderY;
	if ( m_iWideLastHTMLSize == htmlWide && m_iTalLastHTMLSize == htmlTall )
		return;
	m_iWideLastHTMLSize = htmlWide;
	m_iTalLastHTMLSize = htmlTall;
	if ( htmlTall <= 0 )
	{
		SetTall( 64 );
		htmlTall = 64;
		m_iTalLastHTMLSize = 64;
		htmlWide = m_iWideLastHTMLSize;
	}
	m_Serializer->BrowserSize( htmlWide, htmlTall );
	int vertical = _vbar->GetValue();
	int horizontal = _hbar->GetValue();
	m_Serializer->SetHorizontalScroll( horizontal );
	m_Serializer->SetVerticalScroll( vertical );
}

void HTML::OnFileSelected( const char* )
{
	((Frame*)m_hFileOpenDialog.Get())->Close();
}

void HTML::OnFileSelectionCancelled()
{
	((Frame*)m_hFileOpenDialog.Get())->Close();
}

void HTML::BrowserJSAlert( const char* message )
{
	const char* title = StringOrEmpty( m_sCurrentURL );
	MessageBox* box = new MessageBox( title, message, this );
	box->AddActionSignalTarget( this );
	box->SetCommand( new KeyValues( "DismissJSDialog", "result", 0 ) );
	box->DoModal( NULL );
}

void HTML::BrowserJSConfirm( const char* message )
{
	const char* title = StringOrEmpty( m_sCurrentURL );
	QueryBox* box = new QueryBox( title, message, this );
	box->AddActionSignalTarget( this );
	box->SetOKCommand( new KeyValues( "DismissJSDialog", "result", 1 ) );
	box->SetCancelCommand( new KeyValues( "DismissJSDialog", "result", 0 ) );
	box->DoModal( NULL );
}

void HTML::AddCustomURLHandler( const char* customProtocolName, Panel* target )
{
	int index = m_CustomURLHandlers.AddToTail();
	CustomURLHandler_t& handler = m_CustomURLHandlers[index];
	handler.hPanel.Set( target );
	std::strncpy( handler.url, customProtocolName, sizeof( handler.url ) );
}

void HTML::BrowserFinishedRequest( const char* url, const char* pagetitle )
{
	PostActionSignal( new KeyValues( "OnFinishRequest", "url", url ) );
	if ( pagetitle != NULL )
		PostActionSignal( new KeyValues( "PageTitleChange", "title", pagetitle ) );
	OnFinishRequest( url, pagetitle );
	UpdateCachedHTMLValues();
}

void HTML::BrowserSizePopup( int x, int y, int wide, int tall )
{
	int absX = 0;
	int absY = 0;
	ipanel()->GetAbsPos( GetVPanel(), absX, absY );
	m_pComboBoxHost->SetBounds( x + absX + 1, absY + y, wide, tall );
}

void HTML::OnMove()
{
	Panel::OnMove();
	int absX = 0;
	int absY = 0;
	ipanel()->GetAbsPos( GetVPanel(), absX, absY );
	m_Serializer->BrowserPosition( absX, absY );
	if ( m_pComboBoxHost != NULL && m_pComboBoxHost->IsVisible() )
		m_pComboBoxHost->SetVisible( false );
}

void HTML::OnMousePressed( MouseCode code )
{
	std::free( m_sDragURL.m_pchString );
	m_sDragURL.m_pchString = NULL;
	if ( code == MOUSE_4 )
	{
		PostActionSignal( new KeyValues( "HTMLBackRequested" ) );
		return;
	}
	if ( code == MOUSE_5 )
	{
		PostActionSignal( new KeyValues( "HTMLForwardRequested" ) );
		return;
	}
	if ( code == MOUSE_RIGHT )
	{
		if ( m_bContextMenuEnabled )
		{
			m_Serializer->GetLinkAtPosition( m_iMouseX, m_iMouseY );
			Menu::PlaceContextMenu( this, m_pContextMenu );
		}
		else
		{
			RequestFocus( 0 );
		}
		return;
	}

	RequestFocus( 0 );
	if ( code == MOUSE_MIDDLE )
	{
		m_Serializer->MouseDown( ConvertMouseCodeToCEFCode( code ) );
		return;
	}
	m_Serializer->MouseDown( 0 );
	if ( code != MOUSE_LEFT )
		return;

	input()->GetCursorPos( m_iDragStartX, m_iDragStartY );
	int htmlX = 0;
	int htmlY = 0;
	ipanel()->GetAbsPos( GetVPanel(), htmlX, htmlY );
	m_Serializer->GetLinkAtPosition( m_iDragStartX - htmlX, m_iDragStartY - htmlY );
	m_bRequestingDragURL = true;
	if ( m_sDragURL.m_pchString != NULL )
		input()->SetMouseCapture( GetVPanel() );
}

void HTML::OnKillFocus()
{
	if ( input()->GetFocus() != m_pComboBoxHost->GetVPanel() )
		Panel::OnKillFocus();
	if ( !m_pContextMenu->HasFocus() && !m_pComboBoxHost->HasFocus() )
		m_Serializer->SetFocus( false );
}

void HTML::OnMouseReleased( MouseCode code )
{
	int cefCode = 0;
	if ( code != MOUSE_LEFT )
	{
		cefCode = ConvertMouseCodeToCEFCode( code );
	}
	else
	{
		input()->SetMouseCapture( 0 );
		input()->SetCursorOveride( 0 );
		if ( m_sDragURL.m_pchString != NULL )
		{
			if ( input()->GetMouseOver() != GetVPanel() && input()->GetMouseOver() != 0 )
			{
				KeyValues* drag = new KeyValues( "DragDrop" );
				VPANEL mouseOver = input()->GetMouseOver();
				if ( ipanel()->RequestInfo( mouseOver, drag ) && drag->GetInt( "AcceptPanel", 0 ) != 0 )
				{
					VPANEL accept = (VPANEL)drag->GetInt( "AcceptPanel", 0 );
					const char* url = StringOrEmpty( m_sDragURL );
					ivgui()->PostMessage( accept, new KeyValues( "DragDrop", "text", url ), GetVPanel(), 0.0f );
				}
				drag->deleteThis();
			}
		}
		std::free( m_sDragURL.m_pchString );
		m_sDragURL.m_pchString = NULL;
		cefCode = 0;
	}
	m_Serializer->MouseUp( cefCode );
}

//-----------------------------------------------------------------------------
// Purpose: keeps track of where the cursor is
//-----------------------------------------------------------------------------
void HTML::OnCursorMoved( int x, int y )
{
	if ( input()->GetMouseOver() == GetVPanel() )
	{
		m_iMouseX = x;
		m_iMouseY = y;
		m_Serializer->MouseMove( x, y );
	}
	else if ( m_sDragURL.m_pchString == NULL )
	{
		return;
	}
	else if ( input()->GetMouseOver() == 0 )
	{
		std::free( m_sDragURL.m_pchString );
		m_sDragURL.m_pchString = NULL;
		return;
	}

	if ( m_sDragURL.m_pchString != NULL && input()->GetCursorOveride() == 0 )
	{
		int cursorX = 0;
		int cursorY = 0;
		input()->GetCursorPos( cursorX, cursorY );
		( void )cursorX;
		( void )cursorY;
	}
}

void HTML::OnCommand( const char* pchCommand )
{
	if ( _stricmp( pchCommand, "back" ) == 0 )
	{
		GoBack();
		return;
	}
	if ( _stricmp( pchCommand, "forward" ) == 0 )
	{
		GoForward();
		return;
	}
	if ( _stricmp( pchCommand, "reload" ) == 0 )
	{
		Refresh();
		return;
	}
	if ( _stricmp( pchCommand, "stop" ) == 0 )
	{
		StopLoading();
		return;
	}
	if ( _stricmp( pchCommand, "viewsource" ) == 0 || _stricmp( pchCommand, "copy" ) == 0
		 || _stricmp( pchCommand, "paste" ) == 0 )
		return;
	if ( _stricmp( pchCommand, "copyurl" ) == 0 )
	{
		const char* url = "";
		int length = 0;
		if ( m_sCurrentURL.m_pchString != NULL )
		{
			url = m_sCurrentURL.m_pchString;
			length = (int)std::strlen( m_sCurrentURL.m_pchString );
		}
		system()->SetClipboardText( url, length );
		return;
	}
	if ( _stricmp( pchCommand, "copylink" ) == 0 )
	{
		int menuX = 0;
		int menuY = 0;
		m_pContextMenu->GetPos( menuX, menuY );
		int htmlX = 0;
		int htmlY = 0;
		ipanel()->GetAbsPos( GetVPanel(), htmlX, htmlY );
		m_bRequestingCopyLink = true;
		m_Serializer->GetLinkAtPosition( menuX - htmlX, menuY - htmlY );
		return;
	}
	Panel::OnCommand( pchCommand );
}

void HTML::OnTextChanged( Panel* )
{
	char text[2048];
	m_pFindBar->m_pFindBar->GetText( text, 2048 );
	const char* current = StringOrEmpty( m_sLastSearchString );
	m_bInFind = false;
	if ( _stricmp( current, text ) == 0 )
		m_bInFind = true;
	AssignString( m_sLastSearchString, text );
}

void HTML::BrowserLinkAtPositionResponse( const char* url, int x, int y )
{
	AssignString( m_LinkAtPos.m_sURL, url );
	bool hasLink = m_LinkAtPos.m_sURL.m_pchString != NULL;
	m_LinkAtPos.m_nX = (unsigned int)x;
	m_LinkAtPos.m_nY = (unsigned int)y;
	m_pContextMenu->SetItemVisible( m_iCopyLinkMenuItemID, hasLink );

	if ( m_bRequestingDragURL )
	{
		const char* link = StringOrEmpty( m_LinkAtPos.m_sURL );
		m_bRequestingDragURL = false;
		AssignString( m_sDragURL, link );
		if ( m_sDragURL.m_pchString != NULL )
			input()->SetMouseCapture( GetVPanel() );
	}

	if ( m_bRequestingCopyLink )
	{
		m_bRequestingCopyLink = false;
		if ( m_LinkAtPos.m_sURL.m_pchString != NULL )
		{
			const char* link = m_LinkAtPos.m_sURL.m_pchString;
			system()->SetClipboardText( link, (int)std::strlen( link ) );
		}
		else
		{
			system()->SetClipboardText( "", 1 );
		}
	}

	OnLinkAtPosition( StringOrEmpty( m_LinkAtPos.m_sURL ) );
}

static void StorePendingNavigation( CUtlString& urlSlot, CUtlString& postSlot, const char* url, const char* postData )
{
	AssignString( urlSlot, url );
	AssignString( postSlot, postData );
}

static void SubmitURL( IHTMLSerializer* serializer, const char* url, const char* postData )
{
	if ( serializer->BrowserGetIndex() < 0 )
		return;
	if ( postData != NULL && postData[0] != '\0' )
		serializer->PostURL( url, postData );
	else
		serializer->PostURL( url, NULL );
}

void HTML::OpenURL( const char* URL, const char* postData, bool force )
{
	( void )force;
	if ( m_Serializer->BrowserGetIndex() < 0 )
		StorePendingNavigation( m_sPendingURLLoad, m_sPendingPostData, URL, postData );
	else
		SubmitURL( m_Serializer, URL, postData );
}

void HTML::BrowserReady()
{
	char* url = m_sPendingURLLoad.m_pchString;
	if ( url == NULL )
		return;
	const char* post = "";
	if ( m_sPendingPostData.m_pchString != NULL )
		post = m_sPendingPostData.m_pchString;
	PostURL( url, post, false );
	std::free( m_sPendingURLLoad.m_pchString );
	m_sPendingURLLoad.m_pchString = NULL;
}

void HTML::OnSizeChanged( int wide, int tall )
{
	Panel::OnSizeChanged( wide, tall );
	BrowserResize();
	int currentWide = 0;
	int currentTall = 0;
	GetSize( currentWide, currentTall );
	CalcScrollBars( currentWide, currentTall );
	InvalidateLayout( false, false );
	UpdateCachedHTMLValues();
}

void HTML::ApplySchemeSettings( IScheme* scheme )
{
	Panel::ApplySchemeSettings( scheme );
	BrowserResize();
}

void HTML::BrowserHorizontalScrollBarSizeResponse(
	int x, int y, int wide, int tall, int scroll, int scroll_max, float zoom )
{
	bool wasTall = m_scrollHorizontal.m_nTall > 0;
	bool unchanged = wasTall == m_scrollHorizontal.m_bVisible && x == m_scrollHorizontal.m_nX
					 && y == m_scrollHorizontal.m_nY && m_scrollHorizontal.m_nWide == wide
					 && tall == m_scrollHorizontal.m_nTall && scroll_max == m_scrollHorizontal.m_nMax
					 && scroll == m_scrollHorizontal.m_nScroll;
	m_scrollHorizontal.m_bVisible = wasTall;
	m_scrollHorizontal.m_nX = x;
	m_scrollHorizontal.m_nY = y;
	m_scrollHorizontal.m_nWide = wide;
	m_scrollHorizontal.m_nTall = tall;
	m_scrollHorizontal.m_nMax = scroll_max;
	m_scrollHorizontal.m_nScroll = scroll;
	m_scrollHorizontal.m_flZoom = zoom;
	if ( !unchanged )
		UpdateSizeAndScrollBars();
}

void HTML::BrowserVerticalScrollBarSizeResponse(
	int x, int y, int wide, int tall, int scroll, int scroll_max, float zoom )
{
	bool wasTall = m_scrollVertical.m_nTall > 0;
	bool unchanged = wasTall == m_scrollVertical.m_bVisible && x == m_scrollVertical.m_nX && y == m_scrollVertical.m_nY
					 && m_scrollVertical.m_nWide == wide && tall == m_scrollVertical.m_nTall
					 && scroll_max == m_scrollVertical.m_nMax && scroll == m_scrollVertical.m_nScroll;
	m_scrollVertical.m_bVisible = wasTall;
	m_scrollVertical.m_nX = x;
	m_scrollVertical.m_nY = y;
	m_scrollVertical.m_nWide = wide;
	m_scrollVertical.m_nTall = tall;
	m_scrollVertical.m_nMax = scroll_max;
	m_scrollVertical.m_nScroll = scroll;
	m_scrollVertical.m_flZoom = zoom;
	if ( !unchanged )
		UpdateSizeAndScrollBars();
}

void HTML::PaintComboBox()
{
	Panel::Paint();
	if ( m_iComboBoxTextureID != 0 )
	{
		surface()->DrawSetTexture( m_iComboBoxTextureID );
		Color color;
		color.SetRawColor( -1 );
		surface()->DrawSetColor( color );
		surface()->DrawTexturedRect( 0, 0, m_allocedComboBoxWidth, m_allocedComboBoxHeight );
	}
}

void HTML::PostURL( const char* URL, const char* pchPostData, bool force )
{
	( void )force;
	if ( m_Serializer->BrowserGetIndex() < 0 )
		StorePendingNavigation( m_sPendingURLLoad, m_sPendingPostData, URL, pchPostData );
	else
		SubmitURL( m_Serializer, URL, pchPostData );
}

void HTML::ShowFindDialog()
{
	IScheme* clientScheme = scheme()->GetIScheme( scheme()->GetScheme( "ClientScheme" ) );
	if ( clientScheme == NULL )
		return;
	m_pFindBar->SetVisible( true );
	m_pFindBar->RequestFocus( 0 );
	m_pFindBar->m_pFindBar->SetText( "" );
	m_pFindBar->m_pFindCountLabel->SetVisible( false );
	m_pFindBar->m_bHidden = false;
	int x = 0;
	int y = 0;
	int wide = 0;
	int tall = 0;
	m_pFindBar->GetBounds( x, y, wide, tall );
	m_pFindBar->SetPos( x, -tall );
	const char* insetY = clientScheme->GetResourceString( "HTML.SearchInsetY" );
	if ( insetY != NULL )
		(void)std::strtol( insetY, NULL, 10 );
	const char* animationTime = clientScheme->GetResourceString( "HTML.SearchAnimationTime" );
	if ( animationTime != NULL )
		(void)std::strtod( animationTime, NULL );
}

void HTML::HideFindDialog()
{
	IScheme* clientScheme = scheme()->GetIScheme( scheme()->GetScheme( "ClientScheme" ) );
	if ( clientScheme == NULL )
		return;
	int x = 0;
	int y = 0;
	int wide = 0;
	int tall = 0;
	m_pFindBar->GetBounds( x, y, wide, tall );
	const char* animationTime = clientScheme->GetResourceString( "HTML.SearchAnimationTime" );
	if ( animationTime != NULL )
		(void)std::strtod( animationTime, NULL );
	m_pFindBar->m_bHidden = true;
	m_bInFind = false;
}

bool HTML::FindDialogVisible()
{
	if ( !m_pFindBar->IsVisible() )
		return false;
	return !m_pFindBar->m_bHidden;
}

void HTML::OnKeyCodeReleased( KeyCode code )
{
	int virtualKey = system()->KeyCode_VGUIToVirtualKey( code );
	m_Serializer->KeyUp( virtualKey, GetKeyModifiers() );
}

void HTML::OnKeyCodeTyped( KeyCode code )
{
	if ( code != KEY_ESCAPE )
	{
		if ( code > KEY_ESCAPE )
		{
			switch ( code )
			{
			case KEY_PAGEDOWN:
				_vbar->SetValue( _vbar->GetValue() + 200 );
				break;
			case KEY_F5:
				Refresh();
				break;
			case KEY_PAGEUP:
				_vbar->SetValue( _vbar->GetValue() - 200 );
				break;
			default:
				break;
			}
			m_Serializer->KeyDown( system()->KeyCode_VGUIToVirtualKey( code ), GetKeyModifiers() );
			return;
		}
		if ( code != KEY_F )
		{
			if ( code != KEY_TAB )
			{
				m_Serializer->KeyDown( system()->KeyCode_VGUIToVirtualKey( code ), GetKeyModifiers() );
				return;
			}
		}
		else if ( input()->IsKeyDown( KEY_LCONTROL ) || input()->IsKeyDown( KEY_RCONTROL ) )
		{
			if ( !m_pFindBar->IsVisible() || m_pFindBar->m_bHidden )
				ShowFindDialog();
			else
				HideFindDialog();
			m_Serializer->KeyDown( system()->KeyCode_VGUIToVirtualKey( code ), GetKeyModifiers() );
			return;
		}
	}

	if ( m_pFindBar->IsVisible() && !m_pFindBar->m_bHidden )
	{
		HideFindDialog();
		m_Serializer->KeyDown( system()->KeyCode_VGUIToVirtualKey( code ), GetKeyModifiers() );
		return;
	}
	if ( input()->IsKeyDown( KEY_LCONTROL ) || input()->IsKeyDown( KEY_RCONTROL ) )
	{
		Panel::OnKeyTyped( (wchar_t)code );
		return;
	}
	m_Serializer->KeyDown( system()->KeyCode_VGUIToVirtualKey( code ), GetKeyModifiers() );
}

bool HTML::IsScrolledToBottom()
{
	if ( !_vbar->IsVisible() )
		return true;
	return m_scrollVertical.m_nScroll >= m_scrollVertical.m_nMax;
}

bool HTML::IsScrollbarVisible()
{
	return _vbar->IsVisible();
}

void HTML::AddHeader( const char* pchHeader, const char* pchValue )
{
	m_Serializer->AddHeader( pchHeader, pchValue );
}

void HTML::Find( const char* pchSubStr )
{
	const char* current = StringOrEmpty( m_sLastSearchString );
	m_bInFind = false;
	if ( _stricmp( current, pchSubStr ) == 0 )
		m_bInFind = true;
	AssignString( m_sLastSearchString, pchSubStr );
}

void HTML::FindNext()
{
	const char* current = StringOrEmpty( m_sLastSearchString );
	m_bInFind = false;
	if ( _stricmp( current, current ) == 0 )
		m_bInFind = true;
	AssignString( m_sLastSearchString, current );
}

void HTML::StopFind()
{
	m_bInFind = false;
}

void HTML::GetLinkAtPosition( int x, int y )
{
	m_Serializer->GetLinkAtPosition( x, y );
}

void HTML::UpdateSizeAndScrollBars()
{
	BrowserResize();
	int wide = 0;
	int tall = 0;
	GetSize( wide, tall );
	CalcScrollBars( wide, tall );
	InvalidateLayout( false, false );
}

void HTML::BrowserURLChanged( const char* url, const char* postdata, bool isredirect )
{
	AssignString( m_sCurrentURL, url );
	KeyValues* event = new KeyValues( "OnURLChanged" );
	event->SetString( "url", url );
	event->SetString( "postdata", postdata );
	event->SetInt( "isredirect", isredirect );
	PostActionSignal( event );
	OnURLChanged( StringOrEmpty( m_sCurrentURL ), postdata, isredirect );
}

bool HTML::OnStartRequest( const char* url, const char* target, const char* pchPostData, bool bIsRedirect )
{
	if ( url == NULL || _stricmp( url, "about:blank" ) == 0 )
		return true;
	HideFindDialog();
	bool handled = false;
	for ( int index = 0; index < m_CustomURLHandlers.Count(); ++index )
	{
		CustomURLHandler_t& handler = m_CustomURLHandlers[index];
		if ( _strnicmp( handler.url, url, std::strlen( handler.url ) ) != 0 )
			continue;
		Panel* targetPanel = handler.hPanel.Get();
		if ( targetPanel != NULL )
		{
			KeyValues* message = new KeyValues( "CustomURL", "url", handler.url );
			PostMessage( targetPanel, message, 0.0f );
		}
		handled = true;
	}
	if ( handled )
		return false;
	if ( !m_bNewWindowsOnly || !bIsRedirect )
	{
		if ( target != NULL && target[0] == '\0' )
		{
			AssignString( m_sCurrentURL, url );
			KeyValues* event = new KeyValues( "OnURLChanged" );
			event->SetString( "url", url );
			event->SetString( "postdata", pchPostData );
			event->SetInt( "isredirect", bIsRedirect );
			PostActionSignal( event );
		}
		return true;
	}
	if ( target != NULL && ( _stricmp( target, "_blank" ) == 0 || _stricmp( target, "_new" ) == 0 ) )
		return true;
	return false;
}
}
