#ifndef DODCLIENTSCOREBOARD_H
#define DODCLIENTSCOREBOARD_H

#include "game_controls/ClientScoreBoardDialog.h"
#include "idodviewport.h"

class CDoDClientScoreBoardDialog : public CClientScoreBoardDialog
{
public:
	typedef CClientScoreBoardDialog BaseClass;
	CDoDClientScoreBoardDialog( vgui2::Panel* parent );
	void DeathMsg( int killer, int victim ) override;
	void InitScoreboardSections() override;
	void UpdatePlayerInfo() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	bool GetPlayerScoreInfo( int playerIndex, KeyValues* data ) override;
	void AddHeader() override;
	void UpdateTeamInfo() override;
	Color GetTeamColor( int teamNumber ) override;
	virtual ExtraScoreBoardInfo GetExtraScoreBoardInfo( int teamIndex );
	virtual void SetExtraScoreBoardInfo( int teamIndex, int score );
	using BaseClass::AddSection;
	virtual void AddSection( int teamType, int teamNumber );
	virtual int GetTeamScore( int teamIndex );

protected:
	static bool StaticPlayerSortFunc( vgui2::SectionedListPanel* list, int itemID1, int itemID2 );
	ExtraScoreBoardInfo m_TeamExtraInfo[5];
	ExtraScoreBoardInfo m_BlankExtraInfo;
};

#endif
