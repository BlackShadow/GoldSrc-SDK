#ifndef DODTEAMMENU_H
#define DODTEAMMENU_H

#include "game_controls/teammenu.h"

class CDoDTeamMenu : public CTeamMenu
{
public:
	typedef CTeamMenu BaseClass;
	CDoDTeamMenu( vgui2::Panel* parent );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void Paint() override;
	void OnCommand( const char* command ) override;
	void OnTick() override;
	vgui2::Panel* CreateControlByName( const char* controlName ) override;
	vgui2::PanelMap_t* GetPanelMap() override;
	void Update( const char* mapName, bool allowSpectators, const char** teamNames, int numTeams );
	void Update();
	void SetVisibleButton( const char* name, bool visible );
	void OnMouseOver( int button );
	void UpdateNumOnTeamLabel();
private:
	static vgui2::MessageMapItem_t m_MessageMap[1];
	static vgui2::PanelMap_t m_PanelMap;
	int m_iActiveTeam;
	vgui2::Label* m_pNumOnTeamLabel;
};

#endif
