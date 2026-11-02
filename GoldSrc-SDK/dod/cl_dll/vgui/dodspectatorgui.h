#ifndef DODSPECTATORGUI_H
#define DODSPECTATORGUI_H

#include "game_controls/SpectatorGUI.h"

class CDoDSpectatorGUI : public CSpectatorGUI
{
public:
	typedef CSpectatorGUI BaseClass;
	CDoDSpectatorGUI( vgui2::Panel* parent );
	bool CanSpectateTeam( int team ) override;
	void GetPlayerLabelText( int player, wchar_t* text, int textLength ) override;
	void GetPlayerLabelColor( int player, Color& color ) override;
	void UpdateSpectatorPlayerList() override;
	void UpdateTimer() override;
	void Update( const char* serverName, int player, int mode, bool spectateOnly,
		int spectatorCount, const char* mapName ) override;
private:
	vgui2::Label* m_pReinforcementsLabel;
	vgui2::Label* m_pTimerLabel;
	vgui2::Label* m_pAlliesScoreLabel;
	vgui2::Label* m_pAxisScoreLabel;
	vgui2::Label* m_pExtraInfo;
};

#endif
