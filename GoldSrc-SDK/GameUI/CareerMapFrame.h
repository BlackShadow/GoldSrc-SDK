#pragma once

#include "CareerBaseFrame.h"

class CCareerMap;
class CMapButton;

namespace vgui2
{
class CBitmapImagePanel;
class RichText;
class ScrollBar;
}

class CCareerMapFrame : public CCareerBaseFrame
{
public:
	CCareerMapFrame( vgui2::Panel* parent );
	~CCareerMapFrame() override;
	void Activate() override;
	virtual void OnSliderMoved( int position );
	vgui2::Button* GetNextButton();
	void ShowMapDescription( CCareerMap* map );
	void ChangeActiveMap( int right, int down );
	void OnCommand( const char* command ) override;

protected:
	vgui2::PanelMap_t* GetPanelMap() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnKeyCodeTyped( vgui2::KeyCode code ) override;
	void OnThink() override;

private:
	static vgui2::MessageMapItem_t m_MessageMap[1];
	static vgui2::PanelMap_t m_PanelMap;

	CMapButton*					m_pMapButton[3][3];
	vgui2::Button*				m_pNextButton;
	vgui2::Button*				m_pPlayButton;
	vgui2::Button*				m_pCancelButton;
	vgui2::RichText*				m_pMapDesc;
	vgui2::Label*				m_pMapName;
	vgui2::Label*				m_pTripletBorder[3];
	vgui2::Label*				m_pTripletStatus[3];
	vgui2::ScrollBar*			m_pMapScroll;
	vgui2::Label*				m_pMapSelectionBackground;
	vgui2::Label*				m_pMapDescriptionBackground;
	vgui2::CBitmapImagePanel*	m_pMapImage;
	vgui2::CBitmapImagePanel*	m_pMedalImage[4];
	int							m_lastTripletActive;
	int							m_numTriplets;
	int							m_activeMapOffset;
	bool						m_doPostActivate;
	Color						m_mapDescriptionColor;
	Color						m_friendlyFireColor;
};

void ShowCareerMapDescription( vgui2::RichText* text, CCareerMap* map, Color normalColor, Color friendlyFireColor );
