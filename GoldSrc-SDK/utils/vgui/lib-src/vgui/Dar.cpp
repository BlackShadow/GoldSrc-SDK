#include "VGUI_Dar.h"

namespace vgui
{

#ifndef _WIN32
template class VGUIAPI Dar<char>;
template class VGUIAPI Dar<char*>;
template class VGUIAPI Dar<int>;
template class VGUIAPI Dar<class Button*>;
template class VGUIAPI Dar<class SurfaceBase*>;
template class VGUIAPI Dar<class InputSignal*>;
template class VGUIAPI Dar<class FocusChangeSignal*>;
template class VGUIAPI Dar<class FrameSignal*>;
template class VGUIAPI Dar<class ActionSignal*>;
template class VGUIAPI Dar<class IntChangeSignal*>;
template class VGUIAPI Dar<class TickSignal*>;
template class VGUIAPI Dar<Dar<char>*>;
template class VGUIAPI Dar<class Frame*>;
template class VGUIAPI Dar<class DesktopIcon*>;
template class VGUIAPI Dar<class ChangeSignal*>;
template class VGUIAPI Dar<class Panel*>;
template class VGUIAPI Dar<class Label*>;
template class VGUIAPI Dar<class RepaintSignal*>;
#endif

}
