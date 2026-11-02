#pragma once

#include "Button.h"

class CImageButton : public vgui2::Button
{
public:
	CImageButton( vgui2::Panel* parent, const char* panelName, const char* text );
	~CImageButton() override;

	virtual void SetTexture( const char* filename );
	void SetImagePadding( int pad );
	void SetFillPadding( int pad );

	virtual void ForceReload();
	virtual Color GetImageColor();
	virtual void ForceUpload();

protected:
	void PaintBackground() override;

private:
	bool	m_bUploaded;
	int		m_nTextureId;
	char	m_szTexture[128];
	int		m_padImage;
	int		m_padFill;
};
