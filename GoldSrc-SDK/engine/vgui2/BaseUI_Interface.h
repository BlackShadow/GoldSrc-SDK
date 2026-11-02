#pragma once

#include "../IBaseUI.h"
#include "../engine_apiproxy.h"
#include "../IGameUI.h"
#include "../IGameConsole.h"
#include "../iclientvgui.h"

class BaseUISurface;
class CEnginePanel;
class CStaticPanel;
class ICareerUI;
class IMouseControl;
class SystemWrapper;

namespace vgui2
{
class IInputInternal;
class ISurface;
}

class CBaseUI : public IBaseUI
{
public:
	CBaseUI( const CBaseUI& other );
	CBaseUI();
	~CBaseUI() override;

	void Initialize( CreateInterfaceFn* factories, int count ) override;
	void Start( cl_enginefuncs_s* engineFunctions, int interfaceVersion ) override;
	void Shutdown() override;

	int Key_Event( int down, int keynum, const char* currentBinding ) override;
	void CallEngineSurfaceAppHandler( void* event, void* userData ) override;
	void Paint( int x, int y, int right, int bottom ) override;

	void HideGameUI() override;
	void ActivateGameUI() override;
	void HideConsole() override;
	void ShowConsole() override;

private:
	CreateInterfaceFn	m_FactoryList[6];
	int					m_iNumFactories;

	CSysModule*			m_hVGuiModule;
	CSysModule*			m_hStaticGameUIModule;
	CSysModule*			m_hChromeModule;

	bool				m_bHidingGameUI;
};
