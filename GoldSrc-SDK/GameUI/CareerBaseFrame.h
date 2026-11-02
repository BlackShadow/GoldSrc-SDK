#pragma once

#include "Frame.h"
#include "Button.h"
#include "PanelListPanel.h"
#include "vgui/Dar.h"

namespace vgui2
{
class Button;
class Label;
class IImage;
class TextImage;
}

class ColorFade
{
public:
	enum FadeDir
	{
		up = 0,
		down,
		left,
		right
	};

	ColorFade();
	void SetFade( Color startColor, Color endColor, float fadePercent, FadeDir direction );
	void DrawFade( int x, int y, int wide, int tall );

private:
	Color m_startColor;
	Color m_endColor;
	float m_fadePercent;
	FadeDir m_direction;
};

class CCareerPanelListPanel : public CPanelListPanel
{
public:
	CCareerPanelListPanel( vgui2::Panel* parent, const char* name, bool inverseButtons )
		: CPanelListPanel( parent, name, inverseButtons )
	{
	}

protected:
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
};

class CCareerButton : public vgui2::Button
{
public:
	CCareerButton( vgui2::Panel* parent, const char* name, const char* text,
		const char* image, bool textFirst );
	~CCareerButton() override;

	virtual void SetArmedBorder( vgui2::IBorder* border );

protected:
	void Paint() override;
	vgui2::IBorder* GetBorder( bool depressed, bool armed, bool selected, bool keyFocus ) override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;

private:
	vgui2::IBorder*		m_armedBorder;
	vgui2::IImage*		m_image;
	vgui2::TextImage*	m_textImage;
	bool				m_textFirst;
	int					m_textPad;
	int					m_imagePad;
	Color				m_textNormalColor;
	Color				m_textDisabledColor;
};

class CCareerBaseFrame : public vgui2::Frame
{
public:
	CCareerBaseFrame( vgui2::Panel* parent, const char* name, bool showTaskbarIcon );
	~CCareerBaseFrame() override;

	void SetImageBackground( bool state );
	void DrawBackgroundImage();
	void Activate() override;
	virtual void ShowAutosaveNotice();

protected:
	void OnThink() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void PaintBackground() override;
	void PerformLayout() override;

protected:
	struct bimage_t
	{
		int imageID;
		int width;
		int height;
	};

	vgui2::Dar<vgui2::Button*> m_buttons;
	ColorFade m_bgFade;
	vgui2::Label* m_pTitleBackground;
	vgui2::Label* m_pPlayerName;
	vgui2::Label* m_pMissionPackName;
	vgui2::Label* m_pAutosaveNotice;
	float m_autosaveHideTime;
	bool m_showImageBackground;
	bimage_t m_ImageID[3][4];
};

int GetAlternateProportionalValueFromNormal( vgui2::Panel* panel, int normalizedValue );
int GetAlternateProportionalValueFromScaled( vgui2::Panel* panel, int scaledValue );
void RepositionControl( vgui2::Panel* panel );
