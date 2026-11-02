#ifndef VGUI_WORLDMAP_MISSIONSELECT_H
#define VGUI_WORLDMAP_MISSIONSELECT_H

#include "Frame.h"

class CHighlightButton;
class CMissionRadioButton;
namespace vgui2
{
class ImagePanel;
class Label;
}

class CWorldMapMissionSelect : public vgui2::Frame
{
	DECLARE_CLASS_SIMPLE( CWorldMapMissionSelect, Frame );

public:
	CWorldMapMissionSelect( vgui2::Panel* parent );
	~CWorldMapMissionSelect() override;
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void Open( const char* missionName, const char* mapName );
	void OnCommand( const char* command ) override;
	void PaintBackground() override;

private:
	class DataPanel : public vgui2::EditablePanel
	{
		DECLARE_CLASS_SIMPLE( DataPanel, EditablePanel );

	public:
		DataPanel( vgui2::Panel* parent, const char* name );
		void ApplySchemeSettings( vgui2::IScheme* scheme ) override;

	private:
		const char* GetClassPage( const char* className );
		vgui2::Label* m_pMissionTitle;
		vgui2::Label* m_pMissionTime;
		vgui2::Label* m_pMissionLocation;
		vgui2::Label* m_pMissionOrganization;
		vgui2::Label* m_pMissionOverview;
		vgui2::Label* m_pMissionObjective;
	};

	const char* GetClassPage( const char* className );
	enum
	{
		BACKGROUND_ROWS = 2,
		BACKGROUND_COLUMNS = 3
	};
	struct bimage_t
	{
		int imageID;
		int width;
		int height;
	};
	char m_szMapName[64];
	DataPanel* m_pInfoPanel;
	vgui2::ImagePanel* m_pWorldMapImage00;
	vgui2::ImagePanel* m_pWorldMapImage10;
	vgui2::ImagePanel* m_pWorldMapImage20;
	vgui2::ImagePanel* m_pWorldMapImage01;
	vgui2::ImagePanel* m_pWorldMapImage11;
	vgui2::ImagePanel* m_pWorldMapImage21;
	CMissionRadioButton* m_pEasyButton;
	CMissionRadioButton* m_pNormalButton;
	CMissionRadioButton* m_pHardButton;
	CHighlightButton* m_pOkayButton;
	CHighlightButton* m_pCancelButton;
	vgui2::Label* m_pMissionLabel;
	vgui2::Label* m_pLocaleLabel;
	vgui2::Label* m_pTimeLabel;
	vgui2::Label* m_pOrganizationLabel;
	vgui2::Label* m_pOverviewLabel;
	vgui2::Label* m_pObjectiveLabel;
	bimage_t m_ImageID[BACKGROUND_ROWS][BACKGROUND_COLUMNS];
};

#endif
