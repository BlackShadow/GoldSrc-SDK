#pragma once

#include "ClientScoreBoardDialog.h"

class TFCCClientScoreBoardDialog : public CClientScoreBoardDialog
{
public:
	TFCCClientScoreBoardDialog( vgui2::Panel* parent ) : CClientScoreBoardDialog( parent ) {}

protected:
	void InitScoreboardSections() override;
	bool GetPlayerScoreInfo( int playerIndex, KeyValues* data ) override;
	void AddHeader() override;
	void AddSection( int teamType, VGuiLibraryTeamInfo_t* teamInfo ) override;
};
