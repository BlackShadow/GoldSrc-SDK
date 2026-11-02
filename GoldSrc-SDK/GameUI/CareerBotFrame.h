#pragma once

#include "CareerBaseFrame.h"
#include "utlvector.h"

class CBitmapButton;
class CBotButton;
class CCareerCharacter;

namespace vgui2
{
class Button;
class Label;
class PanelHorizListPanel;
}

class CCareerBotFrame : public CCareerBaseFrame
{
public:
	CCareerBotFrame( vgui2::Panel* parent );
	~CCareerBotFrame() override;

	void Activate() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void UpdatePointsRemaining();
	void OnKeyCodeTyped( vgui2::KeyCode code ) override;

private:
	void OnCommand( const char* command ) override;
	int Hire( const char* name );
	void Fire( const char* name );
	void SortHires();
	void AutoRoster();
	CBotButton* GetHiredButton( const char* name );
	CBotButton* GetPoolButton( const char* name );
	void OnThink() override;
	CUtlVector<const CCareerCharacter*>* GetPoolCharacters( int points );

	enum
	{
		MAX_HIRED = 5
	};

	CBotButton*					m_pHiredButton[MAX_HIRED];
	vgui2::Label*				m_pPointsLabel;
	vgui2::PanelHorizListPanel*	m_pListPanel;
	vgui2::Label*				m_pPoolBackground;
	vgui2::Label*				m_pHiredBackground;
	vgui2::Button*				m_pAutoRosterButton;
	CBitmapButton*				m_pPoolPgUpButton;
	CBitmapButton*				m_pPoolPgDownButton;
	vgui2::Button*				m_pStartButton;
	vgui2::Button*				m_pPrevButton;
	vgui2::Label*				m_pPointsLeftLabel;
	CUtlVector<CBitmapButton*>	m_pJumpButtons;
	bool						m_initialLayoutDone;
};
