#pragma once

#include "vgui/KeyCode.h"
#include "vgui/MouseCode.h"

namespace vgui2
{
void InitVGUIToVKTranslationTable();
}

extern "C" vgui2::KeyCode	KeyCode_VirtualKeyToVGUI( int keyCode );
int							KeyCode_VGUIToVirtualKey( vgui2::KeyCode keyCode );
bool						IsKeyCode( int vk_code );

bool						IsMouseCode( int vk_code );
vgui2::MouseCode			MouseCode_VirtualKeyToVGUI( int vk_code );
int							MouseCode_VGUIToVirtualKey( vgui2::MouseCode keyCode );
