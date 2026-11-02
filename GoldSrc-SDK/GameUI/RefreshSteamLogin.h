#pragma once

#ifndef _WIN32

#include "TaskFrame.h"

namespace vgui2
{
class Button;
class CheckButton;
class TextEntry;
}

class CRefreshSteamLogin : public CTaskFrame
{
	DECLARE_CLASS_SIMPLE( CRefreshSteamLogin, CTaskFrame );

public:
	CRefreshSteamLogin( vgui2::Panel* parent, const char* username );
	~CRefreshSteamLogin() override;
	void Activate() override;

private:
	void OnCommand( const char* command ) override;
	void OnThink() override;
	void OnClose() override;

	vgui2::TextEntry*	m_pUserName;
	vgui2::TextEntry*	m_pPassword;
	vgui2::CheckButton*	m_pSavePassword;
	vgui2::Button*		m_pLoginButton;
	bool				m_bLoggingIn;
	vgui2::VPANEL		m_RestrictedPaintPanel;
};

#endif
