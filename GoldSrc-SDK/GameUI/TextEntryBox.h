#pragma once

#include "QueryBox.h"

class CCvarTextEntry;

namespace vgui2
{
class TextEntry;
}

class CTextEntryBox : public vgui2::QueryBox
{
	typedef vgui2::QueryBox BaseClass;

public:
	CTextEntryBox( const char* title, const char* queryText, const char* entryText, bool isCvar, vgui2::Panel* parent );
	~CTextEntryBox() override;
	void PerformLayout() override;
	void ShowWindow( vgui2::Frame* frameOver ) override;

protected:
	void OnKeyCodeTyped( vgui2::KeyCode code ) override;
	void OnCommand( const char* command ) override;

	CCvarTextEntry*		m_pCvarEntry;
	vgui2::TextEntry*	m_pEntry;
};
