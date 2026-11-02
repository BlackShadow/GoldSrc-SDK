#include "controls.h"

namespace vgui2
{
ILocalize* g_pLocalizeInterface;

ILocalize* localize()
{
	return g_pLocalizeInterface;
}
}
