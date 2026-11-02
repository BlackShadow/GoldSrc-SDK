#include "ImageButton.h"

#include "TextImage.h"
#include "controls.h"
#include "vgui/ISurface.h"

#include <string.h>

CImageButton::CImageButton( vgui2::Panel* parent, const char* panelName, const char* text )
	: vgui2::Button( parent, panelName, text )
{
	m_szTexture[0] = 0;
	m_bUploaded = false;
	m_nTextureId = -1;
	m_padFill = 1;
	m_padImage = 1;
}

CImageButton::~CImageButton()
{
}

Color CImageButton::GetImageColor()
{
	return GetBgColor();
}

void CImageButton::ForceReload()
{
	if ( m_bUploaded )
		vgui2::surface()->DrawSetTextureFile( m_nTextureId, m_szTexture, 0, true );
}

void CImageButton::ForceUpload()
{
	if ( m_szTexture[0] )
	{
		m_bUploaded = true;
		m_nTextureId = vgui2::surface()->CreateNewTextureID( false );
		vgui2::surface()->DrawSetTextureFile( m_nTextureId, m_szTexture, 0, true );
	}
}

void CImageButton::SetTexture( const char* filename )
{
	strcpy( m_szTexture, filename );
	if ( m_bUploaded )
		ForceReload();
	else
		ForceUpload();
}

void CImageButton::PaintBackground()
{
	int wide, tall;
	GetSize( wide, tall );
	vgui2::surface()->DrawSetColor( GetBgColor() );
	vgui2::surface()->DrawFilledRect( m_padFill, m_padFill, wide - m_padFill, tall - m_padFill );
	if ( !m_szTexture[0] )
		return;

	if ( !m_bUploaded )
		ForceUpload();
	vgui2::surface()->DrawSetColor( GetImageColor() );
	vgui2::surface()->DrawSetTexture( m_nTextureId );
	vgui2::surface()->DrawTexturedRect( m_padImage, m_padImage, wide - m_padImage, tall - m_padImage );

	vgui2::TextImage* textImage = GetTextImage();
	int x, y;
	textImage->GetPos( x, y );
	textImage->SetPos( x + 1, y + 1 );
	textImage->SetColor( Color( 0, 0, 0, 192 ) );
	textImage->Paint();
	textImage->SetPos( x, y );
}

void CImageButton::SetImagePadding( int pad )
{
	m_padImage = pad;
	InvalidateLayout( false, false );
}

void CImageButton::SetFillPadding( int pad )
{
	m_padFill = pad;
	InvalidateLayout( false, false );
}
