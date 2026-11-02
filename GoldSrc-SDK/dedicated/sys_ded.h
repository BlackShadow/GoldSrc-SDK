#pragma once

#include "../vgui2/controls/Panel.h"
#include "../vgui2/controls/PHandle.h"
#include "../tracker/common/server.h"

#include <windows.h>

class CCreateMultiplayerGameServerPage;
class IManageServer;

namespace vgui2
{
class ProgressBox;
}

class CMainPanel : public vgui2::Panel
{
public:
	CMainPanel( CreateInterfaceFn* factories, int count );
	~CMainPanel() override;

	virtual void Initialize( CreateInterfaceFn* factories, int count );
	virtual void ShowConfig();
	virtual void StartServer( char* command );
	virtual void Shutdown();

	void Printf( char* text );
	void OnTick() override;
	vgui2::PanelMap_t* GetPanelMap() override;

private:
	void Stop();

	friend int Main( int argc, wchar_t** argv );
	friend int RunServer();

	static vgui2::MessageMapItem_t	m_MessageMap[1];
	static vgui2::PanelMap_t		m_PanelMap;

	IManageServer*					m_pManageServer;
	vgui2::DHANDLE<vgui2::ProgressBox>	m_hProgress;
	CCreateMultiplayerGameServerPage*	m_pConfig;
	HANDLE							m_hEvent;
	bool							m_bStarting;
	bool							m_bStarted;
	bool							m_bQuit;
	bool							m_bWaiting;
	serveritem_t					m_Server;
	int								m_hResourceWait;
	float							m_flProgress;
};

CMainPanel* MainPanel();

void Sys_Printf( char* format, ... );
