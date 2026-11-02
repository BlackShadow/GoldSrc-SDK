//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose:
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "stdafx.h"
#include "worldcraft.h"
#include "OptionProperties.h"
#include "OPTTextures.h"
#include "Options.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// COPTTextures property page

IMPLEMENT_DYNCREATE( COPTTextures, CPropertyPage )

COPTTextures::COPTTextures() : CPropertyPage( COPTTextures::IDD )
{
	//{{AFX_DATA_INIT(COPTTextures)
	// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	m_bDeleted = FALSE;
}

COPTTextures::~COPTTextures()
{
}

void COPTTextures::DoDataExchange( CDataExchange* pDX )
{
	CPropertyPage::DoDataExchange( pDX );
	//{{AFX_DATA_MAP(COPTTextures)
	DDX_Control( pDX, IDC_TEXTUREFILES, m_TextureFiles );
	DDX_Control( pDX, IDC_BRIGHTNESS, m_cBrightness );
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP( COPTTextures, CPropertyPage )
//{{AFX_MSG_MAP(COPTTextures)
ON_BN_CLICKED( IDC_EXTRACT, OnExtract )
ON_BN_CLICKED( IDC_ADDTEXFILE, OnAddtexfile )
ON_BN_CLICKED( IDC_REMOVETEXFILE, OnRemovetexfile )
ON_WM_HSCROLL()

//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COPTTextures message handlers

BOOL COPTTextures::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	// load texture file list with options
	int i;
	for ( i = 0; i < Options.textures.nTextureFiles; i++ )
		m_TextureFiles.AddString( Options.textures.TextureFiles[i] );

	// set brightness control & values
	m_cBrightness.SetRange( 1, 50 ); // 10 is default
	m_cBrightness.SetPos( int( Options.textures.fBrightness * 10 ) );

	return TRUE;
}

void COPTTextures::OnExtract()
{
	// redo listbox content
	m_TextureFiles.ResetContent();
	for ( int i = 0; i < Options.textures.nTextureFiles; i++ )
		m_TextureFiles.AddString( Options.textures.TextureFiles[i] );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
//-----------------------------------------------------------------------------
void COPTTextures::OnAddtexfile( void )
{
	static char szInitialDir[MAX_PATH] = "\0";

	CFileDialog dlg( TRUE, "wad", NULL,
		OFN_ALLOWMULTISELECT | OFN_HIDEREADONLY | OFN_NOCHANGEDIR | OFN_FILEMUSTEXIST | OFN_ENABLESIZING,
		"Texture files (*.wad;*.pak)|*.wad; *.pak||", NULL, 0, FALSE );
	dlg.m_ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;

	if ( szInitialDir[0] == '\0' )
	{
		POSITION pos;
		APP()->GetFirstSearchDir( SEARCHDIR_WADS, szInitialDir, pos );
	}

	dlg.m_ofn.lpstrInitialDir = szInitialDir;

	if ( dlg.DoModal() != IDOK )
		return;

	//
	// Get all the filenames from the open file dialog.
	//
	POSITION pos = dlg.GetStartPosition();
	CString	 str;
	while ( pos != NULL )
	{
		str = dlg.GetNextPathName( pos );
		str.MakeLower();
		m_TextureFiles.AddString( str );
		SetModified();
	}

	//
	// Use this directory as the default directory for the next time.
	//
	int nBackslash = str.ReverseFind( '\\' );
	if ( nBackslash != -1 )
		lstrcpyn( szInitialDir, str, nBackslash + 1 );
}

void COPTTextures::OnRemovetexfile()
{
	int i = m_TextureFiles.GetCount();

	for ( i--; i >= 0; i-- )
		if ( m_TextureFiles.GetSel( i ) )
			m_TextureFiles.DeleteString( i );

	m_bDeleted = TRUE;
	SetModified();
}

void COPTTextures::OnHScroll( UINT nSBCode, UINT nPos, CScrollBar* pScrollBar )
{
	if ( pScrollBar == (CScrollBar*)&m_cBrightness )
		SetModified();

	CPropertyPage::OnHScroll( nSBCode, nPos, pScrollBar );
}

BOOL COPTTextures::OnApply()
{
	Options.textures.fBrightness = (float)m_cBrightness.GetPos() / 10.0f;

	int		iSize = m_TextureFiles.GetCount();
	CString str;
	Options.textures.nTextureFiles = iSize;
	Options.textures.TextureFiles.RemoveAll();
	for ( int i = 0; i < iSize; i++ )
	{
		m_TextureFiles.GetText( i, str );
		Options.textures.TextureFiles.Add( str );
	}

	if ( m_bDeleted )
	{
		// inform them that deleted files will only be reapplied after
		// they reload Worldcraft
		MessageBox( "You have removed some texture files from the list.\n"
					"These texture files will continue to be used during this\n"
					"session, but will not be loaded the next time you run\n"
					"Worldcraft.",
					"A Quick Note" );
	}

	Options.PerformChanges( COptions::secTextures );

	return CPropertyPage::OnApply();
}

void GetDirectory( char* pDest, const char* pLongName )
{
	strcpy( pDest, pLongName );
	int i = strlen( pDest );
	while ( pLongName[i] != '\\' && pLongName[i] != '/' && i > 0 )
		i--;

	if ( i <= 0 )
		i = 0;

	pDest[i] = 0;

	return;
}

