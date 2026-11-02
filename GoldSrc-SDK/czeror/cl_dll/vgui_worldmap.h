#ifndef VGUI_WORLDMAP_H
#define VGUI_WORLDMAP_H

#include "Frame.h"
#include "vgui/hl_base/mouseoverpanelbutton.h"

class CHighlightButton;
class CWorldMapMissionSelect;
namespace vgui2
{
class ImageList;
class Label;
}

class CWorldMap : public vgui2::Frame
{
	DECLARE_CLASS_SIMPLE( CWorldMap, Frame );

public:
	CWorldMap( vgui2::Panel* parent );
	~CWorldMap() override;
	void Open();
	vgui2::Panel* CreateControlByName( const char* controlName ) override;
	void UnlockMission( const char* mapName );
	void LockMission( const char* mapName );
	void CurrentMission( const char* mapName );
	enum MissionType_E
	{
		MISSION_UNLOCKED = 0,
		MISSION_CURRENT = 1,
		MISSION_LOCKED = 2
	};
	MissionType_E GetMissionStatus( const char* mapName );
	void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
	void OnCommand( const char* command ) override;
	void PaintBackground() override;

private:
	class ClassHelperPanel : public vgui2::EditablePanel
	{
	public:
		ClassHelperPanel( vgui2::Panel* parent, const char* name );
		void SetMissionStatus( MissionType_E status );
		void PerformLayout() override;

	private:
		void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
		const char* GetClassPage( const char* className );
		vgui2::Label* m_pMissionTitle;
		vgui2::Label* m_pMissionOverview;
		MissionType_E m_Status;
	};

	class CMissionButton : public MouseOverPanelButton
	{
	public:
		CMissionButton( vgui2::Panel* parent, const char* name, vgui2::Panel* helper );
		~CMissionButton() override;
		void PerformLayout() override;
		void ApplySettings( KeyValues* settings ) override;
		void ApplySchemeSettings( vgui2::IScheme* scheme ) override;
		void DrawFocusBorder( int x, int y, int wide, int tall ) override;
		void SetMissionStatus( MissionType_E status );
		MissionType_E GetMissionStatus();
		const char* GetMapName();
		const char* GetMissionName();
		void Paint() override;

	private:
		vgui2::ImageList* m_pMissionImageList;
		ClassHelperPanel* m_pHelperPanel;
		char m_sMapName[32];
		MissionType_E m_Status;
	};

	void MissionStatusHelper( const char* mapName, MissionType_E status );
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
	vgui2::Panel* m_pPanel;
	vgui2::Label* m_pMissionLabel;
	CHighlightButton* m_pResumeButton;
	CWorldMapMissionSelect* m_pMissionSelect;
	bimage_t m_ImageID[BACKGROUND_ROWS][BACKGROUND_COLUMNS];
};

#endif
