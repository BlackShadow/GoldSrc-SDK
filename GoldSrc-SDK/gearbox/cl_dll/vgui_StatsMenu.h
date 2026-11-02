#pragma once

#include "vgui_CTFViewport.h"

class CStatsMenuPanel : public CMenuPanel
{
public:
	CStatsMenuPanel( int transparency, int remove, int x, int y, int wide, int tall );
	void Open() override;
	bool SlotInput( int slot ) override;
	virtual void SetActiveInfo( int info );
	virtual void Update();
	virtual void Initialize();
	void Reset() override;
	void SetPlayerImage( const char* image );
	void SaveStats();
	int MsgFunc_StatsInfo( const char* name, int size, void* buffer );
	int MsgFunc_StatsPlayer( const char* name, int size, void* buffer );

private:
	friend class CTFViewport;
	CTransparentPanel* m_pClassInfoPanel[4];
	CommandButton* m_pButtons[4];
	CommandButton* m_pCancelButton;
	CommandButton* m_pScoreBoardButton;
	ScrollPanel* m_pScrollPanel;
	TextPanel* m_pStatsWindow[4];
	CImageLabel* m_pClassImages[4];
	int m_iCurrentInfo;
	CommandButton* m_pSaveButton;
};
