#pragma once

#include "Frame.h"
#include "vgui/Dar.h"

class CCareerButton;
class CCvarTextEntry;
class CCvarToggleCheckButton;

namespace vgui2
{
class Button;
class CBitmapImagePanel;
class Label;
}

class CCareerBaseBox : public vgui2::Frame
{
public:
	CCareerBaseBox( vgui2::Panel* parent, const char* panelName, bool loadResources );
	~CCareerBaseBox() override;

	virtual void ShowWindow();
	void DoModal() override;
	void SetCancelButtonAsDefault();
	void SetLabelText( const wchar_t* text );
	void AddButton( vgui2::Button* button );

protected:
	void PaintBackground() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void PerformLayout() override;
	void OnKeyCodeTyped( vgui2::KeyCode code ) override;
	void OnCommand( const char* command ) override;

	vgui2::Button*				m_pOkButton;
	vgui2::Button*				m_pCancelButton;
	vgui2::Dar<vgui2::Button*>	m_buttons;
	vgui2::CBitmapImagePanel*	m_pTopLeftPanel;
	vgui2::CBitmapImagePanel*	m_pTopRightPanel;
	vgui2::CBitmapImagePanel*	m_pBottomLeftPanel;
	vgui2::CBitmapImagePanel*	m_pBottomRightPanel;
	Color						m_bgColor;
	vgui2::Label*				m_pTextLabel;
	bool						m_cancelFocus;
};

class CCareerQueryBox : public CCareerBaseBox
{
public:
	CCareerQueryBox( const char* title, const char* labelText, const char* panelName, vgui2::Panel* parent );
	CCareerQueryBox( const wchar_t* title, const wchar_t* labelText, const char* panelName, vgui2::Panel* parent );
	~CCareerQueryBox() override;
};

class CCareerDifficultyBox : public CCareerBaseBox
{
public:
	CCareerDifficultyBox( const char* panelName, vgui2::Panel* parent );
	~CCareerDifficultyBox() override;
	void ShowWindow() override;

protected:
	void OnCommand( const char* command ) override;

private:
	CCareerButton*			m_pFocusButton;
	CCvarToggleCheckButton*	m_pEnableTutorCheck;
};

class CCareerNewProfileBox : public CCareerBaseBox
{
public:
	CCareerNewProfileBox( const char* panelName, vgui2::Panel* parent );
	~CCareerNewProfileBox() override;
	void ShowWindow() override;

protected:
	void OnCommand( const char* command ) override;
	void OnKeyCodeTyped( vgui2::KeyCode code ) override;

private:
	CCvarTextEntry*	m_pEntry;
};
