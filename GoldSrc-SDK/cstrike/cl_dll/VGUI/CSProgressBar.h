#pragma once

#include "vgui/IScheme.h"
#include "Frame.h"

namespace vgui2
{
class CBitmapImagePanel;
class Label;
}

class RoundedCornerPanel : public vgui2::Panel
{
public:
	RoundedCornerPanel( vgui2::Panel* parent );
	~RoundedCornerPanel() override;
	void PerformLayout() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void Draw();

private:
	vgui2::CBitmapImagePanel*	m_pTopLeftPanel;
	vgui2::CBitmapImagePanel*	m_pTopRightPanel;
	vgui2::CBitmapImagePanel*	m_pBottomLeftPanel;
	vgui2::CBitmapImagePanel*	m_pBottomRightPanel;
	vgui2::CBitmapImagePanel*	m_pTopLeftBorderPanel;
	vgui2::CBitmapImagePanel*	m_pTopRightBorderPanel;
	vgui2::CBitmapImagePanel*	m_pBottomLeftBorderPanel;
	vgui2::CBitmapImagePanel*	m_pBottomRightBorderPanel;
	Color						m_bgColor;
	Color						m_borderColor;
};

class CCSProgressBar : public vgui2::Frame
{
public:
	CCSProgressBar( vgui2::Panel* parent );
	~CCSProgressBar() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void PerformLayout() override;
	void OnThink() override;
	void PaintBackground() override;
	void Init( const char* title, int numTicks, int startTick, bool isTimeBased );
	void Update( const char* statusText, int tick );

protected:
	bool					m_isTimeBased;
	float					m_startTime;
	int						m_numTicks;
	int						m_startTick;
	int						m_currentTick;
	vgui2::Label*			m_pTitle;
	vgui2::Label*			m_pText;
	vgui2::Panel*			m_pProgressBarBorder;
	vgui2::Panel*			m_pProgressBar;
	vgui2::Panel*			m_pProgressBarSizer;
	RoundedCornerPanel*		m_pBackgroundPanel;
};
