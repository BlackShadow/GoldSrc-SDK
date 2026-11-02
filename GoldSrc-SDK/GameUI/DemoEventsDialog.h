#pragma once

#include "Frame.h"

class CDemoEditDialog;
class DirectorCmd;
class IDemoPlayer;
class IEngineWrapper;

namespace vgui2
{
class ListPanel;
}

class CDemoEventsDialog : public vgui2::Frame
{
	DECLARE_CLASS_SIMPLE_NAMED( CDemoEventsDialog, vgui2::Frame, "vgui::Frame" );

public:
	CDemoEventsDialog( vgui2::Panel* parent, const char* name, IEngineWrapper* engine, IDemoPlayer* demoPlayer );
	~CDemoEventsDialog() override;

	MESSAGE_FUNC( OnUpdate, "UpdateCmdList" );
	MESSAGE_FUNC( OnUpdateLastCmd, "UpdateLastCmd" );

	void GetCurrentCmd();
	void OnGoto();
	void OnModify();
	void OnRemove();
	void OnAdd();
	void OnCommand( const char* command ) override;

private:
	vgui2::ListPanel*	m_EventList;
	IEngineWrapper*		m_Engine;
	IDemoPlayer*		m_DemoPlayer;
	DirectorCmd*		m_CurrentCmd;
	CDemoEditDialog*	m_EditDialog;
};
