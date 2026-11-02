#ifndef CZEROVIEWPORT_H
#define CZEROVIEWPORT_H

#include "hl_base/vgui_TeamFortressViewport.h"
#include "iczeroviewport.h"

class CWorldMap;

class CZEROViewPort : public TeamFortressViewport, public ICzeroViewPort
{
public:
	CZEROViewPort();
	~CZEROViewPort() override;
	void Initialize( CreateInterfaceFn* factories, int count ) override;
	void Start() override;
	void SetParent( vgui2::VPANEL parent ) override;
	int KeyInput( int down, int key, const char* binding ) override;
	void UpdateSpectatorPanel() override;
	void OnLevelChange() override;
	void ActivateClientUI() override;
	void ShowWorldMap() override;
	void HideWorldMap() override;
	bool IsWorldMapVisible() override;
	void UnlockMission( const char* missionName ) override;
	void LockMission( const char* missionName ) override;
	void CurrentMission( const char* missionName ) override;

private:
	CWorldMap* m_pWorldMap;
};

#endif
