#pragma once

#include "Panel.h"
#include "utlsymbol.h"
#include "utlvector.h"

namespace vgui = vgui2;

namespace vgui2
{

class TestScriptController : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( TestScriptController, vgui::Panel );

public:
	TestScriptController();
	~TestScriptController() override;

	bool ExecuteScript( const char* scriptFile );

protected:
	MESSAGE_FUNC_FLOAT( OnDelay, "Delay", time );
	MESSAGE_FUNC_FLOAT( OnSetScriptRunRate, "SetScriptRunRate", time );
	void OnTick() override;

private:
	enum ScriptCommandType_e
	{
		SCRIPTCOMMAND_MESSAGE = 0,
		SCRIPTCOMMAND_WAITUNTILVISIBLE,
		SCRIPTCOMMAND_WAITUNTILENABLED,
		SCRIPTCOMMAND_REGISTRYSET,
		SCRIPTCOMMAND_EXEC
	};

	struct ScriptCommand_t
	{
		ScriptCommandType_e	cmd;
		UtlSymId_t			target;
		KeyValues*			msg;
		char*				originalLine;
	};

	struct ScriptFunction_t
	{
		UtlSymId_t					name;
		CUtlVector<ScriptCommand_t>	scriptCommands;
	};

	struct ScriptProgramCounter_t
	{
		int	currentScriptFunctionIndex;
		int	currentScriptCommandIndex;
	};

	VPANEL FindMessageTarget( const char* targetName );
	VPANEL FindMessageTargetInSubPanel( VPANEL parent, const char* targetName );
	int PrintMatchingPanels( VPANEL startPanel, const char* targetName );
	void OnPanelNotFoundError( const char* targetName );
	void ExecuteNextScriptCommand();
	void CalculateFullNameForPanel( VPANEL panel, char* name, int nameBufSize );
	void ClearScriptCommandBuffer();
	void MakeValueString( const char* value, char* output, int outputBufferSize );
	void AbortOnAnyErrorLogsPresent();
	void CreateNewUniqueTestString();
	const char* ParseFunction( ScriptFunction_t& function, const char* memory );
	bool ParseScriptFile( const char* memory, int length );
	bool LoadScriptFile( const char* scriptFile );

	CUtlSymbolTable					m_TargetSymbols;
	CUtlVector<ScriptFunction_t>		m_ScriptFunctions;
	CUtlVector<ScriptProgramCounter_t>	m_CurrentScriptCommandIndex;
	float							m_flNextScriptCommandExecTime;
	float							m_flTimeBetweenRunningEachScriptLine;
	char							m_szUniqueTestString[256];
	bool							m_bParsingErrorOccured;
};

}
