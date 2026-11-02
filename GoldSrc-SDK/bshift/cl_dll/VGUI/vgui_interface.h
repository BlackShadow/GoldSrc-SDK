#pragma once

#include "IViewPort.h"
#include "IViewPortMsgs.h"

class CCreateCommandComboMenu;

CCreateCommandComboMenu*	CreateCommandCombo();
int						GetMaxTeams();

extern "C" DLL_EXPORT void HUD_ChatInputPosition( int* x, int* y );
