#include "CvarNegateCheckButton.h"
#include "GameUI_Interface.h"

#include "vgui/KeyValues.h"
#include "wrect.h"
#include "cl_dll.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

CCvarNegateCheckButton::CCvarNegateCheckButton( vgui2::Panel* parent, const char* panelName, const char* text,
		const char* cvarName ) : BaseClass( parent, panelName, text )
{
	m_pszCvarName = cvarName ? strdup( cvarName ) : NULL;
	m_bStartState = engine->pfnGetCvarFloat( m_pszCvarName ) < 0.0f;
	SetSelected( m_bStartState );
	AddActionSignalTarget( this );
}

CCvarNegateCheckButton::~CCvarNegateCheckButton()
{
	free( m_pszCvarName );
}

void CCvarNegateCheckButton::Paint()
{
	if ( m_pszCvarName )
	{
		bool state = engine->pfnGetCvarFloat( m_pszCvarName ) < 0.0f;
		if ( state != m_bStartState )
		{
			SetSelected( state );
			m_bStartState = state;
		}
	}

	vgui2::Button::Paint();
}

void CCvarNegateCheckButton::Reset()
{
	m_bStartState = engine->pfnGetCvarFloat( m_pszCvarName ) < 0.0f;
	SetSelected( m_bStartState );
}

bool CCvarNegateCheckButton::HasBeenModified()
{
	return IsSelected() != m_bStartState;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *panel - 
//-----------------------------------------------------------------------------
void CCvarNegateCheckButton::SetSelected( bool state )
{
	vgui2::CheckButton::SetSelected( state );
}

void CCvarNegateCheckButton::ApplyChanges()
{
	if ( !m_pszCvarName || !m_pszCvarName[0] )
		return;

	float value = (float)fabs( (double)engine->pfnGetCvarFloat( m_pszCvarName ) );
	if ( value < 0.00001 )
		value = 0.022f;
	m_bStartState = IsSelected();
	engine->Cvar_SetValue( m_pszCvarName, m_bStartState ? -value : value );
}

void CCvarNegateCheckButton::OnButtonChecked()
{
	if ( IsSelected() != m_bStartState )
		PostActionSignal( new KeyValues( "ControlModified" ) );
}
