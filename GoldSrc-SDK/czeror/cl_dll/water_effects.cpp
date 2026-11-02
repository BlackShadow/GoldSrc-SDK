#include "hud.h"
#include "pm_shared.h"

bool UTIL_CheckForWater(Vector startPosition, Vector endPosition, Vector &waterLevel)
{
	if (gEngfuncs.PM_PointContents(startPosition, NULL) == CONTENTS_WATER)
		return false;

	Vector position = startPosition;
	Vector direction = (endPosition - startPosition).Normalize();
	position = position + direction;
	if (startPosition.z - endPosition.z <= 1.0f)
		return false;

	while (gEngfuncs.PM_PointContents(position, NULL) != CONTENTS_WATER)
	{
		position = position + direction;
		if (position.z - endPosition.z <= 1.0f)
			return false;
	}

	waterLevel = position;
	return true;
}
