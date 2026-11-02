#pragma once

#include "Button.h"

class CBitmapButton : public vgui2::Button
{
public:
	CBitmapButton( vgui2::Panel* parent, const char* panelName, const char* text,
		const char* normalImage, const char* armedImage, const char* clickedImage, const char* disabledImage );
	~CBitmapButton() override;

	void Flash();

protected:
	void PaintBackground() override;
	void OnThink() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void DrawFocusBorder( int x0, int y0, int x1, int y1 ) override;

private:
	struct bimage_t
	{
		int	imageID;
		int	width;
		int	height;
	};

	bimage_t	m_ImageID[4];
	float		m_interval;
	float		m_lastTransition;
	int			m_transitionsLeft;
	bool		m_shouldFlash;
	bool		m_isFlashColor;
	int			m_totalTransitions;
};
