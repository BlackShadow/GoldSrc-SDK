#pragma once

#ifndef _WIN32

#include "TaskFrame.h"

namespace vgui2
{
class Button;
class TextEntry;
}

class CCDKeyEntryDialog : public CTaskFrame
{
	DECLARE_CLASS_SIMPLE( CCDKeyEntryDialog, CTaskFrame );

public:
	CCDKeyEntryDialog( vgui2::Panel* parent, bool inConnect );
	~CCDKeyEntryDialog() override;
	void Activate() override;

private:
	enum
	{
		MAX_CDKEY_ERRORS = 4
	};

	void OnCommand( const char* command ) override;
	void OnClose() override;
	void OnThink() override;
	MESSAGE_FUNC_PTR( OnTextChanged, "TextChanged", panel );
	bool IsEnteredKeyValid();

	vgui2::Button*		m_pOK;
	vgui2::Button*		m_pQuitGame;
	vgui2::TextEntry*	m_pEntry1;
	vgui2::TextEntry*	m_pEntry2;
	vgui2::TextEntry*	m_pEntry3;
	vgui2::TextEntry*	m_pEntry4;
	vgui2::TextEntry*	m_pEntry5;
	bool				m_bIsValveCDKey;
	bool				m_bInConnect;
	int					m_iErrCount;
};

#endif
