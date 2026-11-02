#ifndef IDODVIEWPORT_H
#define IDODVIEWPORT_H

struct VGuiLibraryTeamInfo_t;

struct ExtraScoreBoardInfo
{
	int score;
};

class IDoDViewPort
{
public:
	virtual int GetForceCamera() = 0;
	virtual void setWidescreenVisible( bool visible ) = 0;
	virtual bool isWidescreenVisible() = 0;
	virtual void setWidescreenSubtitle( int subtitle ) = 0;
	virtual void updateTeamMenu() = 0;
	virtual void ShowHintMessage( const char* message ) = 0;
	virtual void ShowSpectatorGUI() = 0;
	virtual void DeactivateSpectatorGUI() = 0;
	virtual void PaintBackgroundPanel() = 0;
	virtual int GetTeamScore( int team ) = 0;
	virtual void ShowMortarPanel() = 0;
	virtual void HideMortarPanel() = 0;
	virtual void PostMortarValue( float value ) = 0;
	virtual ExtraScoreBoardInfo GetExtraScoreBoardInfo( int team ) = 0;
	virtual VGuiLibraryTeamInfo_t GetPlayerTeamInfo( int team ) = 0;
};

extern IDoDViewPort* gDoDViewPortInterface;

#endif
