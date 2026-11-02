#pragma once

#include "ILocalize.h"

class IKeyValues;

IKeyValues* keyvalues();

namespace vgui2
{
extern ILocalize* g_pLocalizeInterface;

ILocalize* localize();
} // namespace vgui
