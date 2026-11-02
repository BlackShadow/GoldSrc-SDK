#pragma once

#include "Frame.h"
#include "../HLTV/DemoPlayer/private/HLTV/common/DirectorCmd.h"

#define MAX_DEMO_DATA_FIELDS	8

class CLabeledCommandComboBox;
class IDemoPlayer;
class IEngineWrapper;

namespace vgui2
{
class Button;
class Label;
class TextEntry;
}

class CDemoEditDialog : public vgui2::Frame
{
	DECLARE_CLASS_SIMPLE_NAMED( CDemoEditDialog, vgui2::Frame, "vgui::Frame" );

public:
	CDemoEditDialog( vgui2::Panel* parent, const char* name, IEngineWrapper* engine, IDemoPlayer* demoPlayer );
	~CDemoEditDialog() override;
	void SetCommand( DirectorCmd* command );

protected:
	void OnCommand( const char* command ) override;
	void OnOK();
	void OnGetView();
	void OnGetTime();
	void OnUpdate();
	MESSAGE_FUNC( OnTypeChanged, "ControlModified" );
	void UpdateDrcCmd();
	void ClearDataFields();
	void SetDataField( int index, char* label, char* text );
	void SetDataFieldf( int index, char* label, float value );
	void SetDataFieldi( int index, char* label, int value );
	char* GetDataFields( int index );
	int GetDataFieldx( int index );
	int GetDataFieldi( int index );
	float GetDataFieldf( int index );
	void GetDataField3f( int index, float* first, float* second, float* third );
	void GetDataField2f( int index, float* first, float* second );

private:
	IEngineWrapper*		m_Engine;
	IDemoPlayer*		m_DemoPlayer;
	DirectorCmd			m_DrcCmd;
	bool				m_ChangingData;
	CLabeledCommandComboBox*	m_TypeBox;
	vgui2::TextEntry*	m_TimeStamp;
	vgui2::Button*		m_GetViewButton;
	vgui2::TextEntry*	m_DataEntries[MAX_DEMO_DATA_FIELDS];
	vgui2::Label*		m_DataLabels[MAX_DEMO_DATA_FIELDS];
};
