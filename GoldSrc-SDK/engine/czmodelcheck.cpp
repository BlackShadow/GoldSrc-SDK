#include "crc.h"
#include <cstring>

qboolean IsCZPlayerModel( CRC32_t crc, const char* name )
{
	if ( !name )
		return 0;

	const char* model;
	switch ( crc )
	{
	case 0x02B95E5F:
	case 0x72DB74E4:
		model = "models/player/vip/vip.mdl";
		break;
	case 0x068168DB:
	case 0xC7F0DBF3:
		model = "models/player/urban/urban.mdl";
		break;
	case 0x0DA8922A:
	case 0x56DD2D02:
		model = "models/player/gsg9/gsg9.mdl";
		break;
	case 0x1AAA3360:
	case 0x35AC6FED:
		model = "models/player/gign/gign.mdl";
		break;
	case 0x1B6C4115:
	case 0x1F3CD80B:
		model = "models/player/guerilla/guerilla.mdl";
		break;
	case 0x270FB2D7:
	case 0xEC43F76D:
		model = "models/player/terror/terror.mdl";
		break;
	case 0x27FB4D2F:
		model = "models/player/spetsnaz/spetsnaz.mdl";
		break;
	case 0x29FE156C:
	case 0xC37369F6:
		model = "models/player/leet/leet.mdl";
		break;
	case 0x3BCAA016:
		model = "models/player/militia/militia.mdl";
		break;
	case 0x4986827B:
	case 0xA37D8680:
		model = "models/player/arctic/arctic.mdl";
		break;
	case 0x43E67FF3:
	case 0xF141AE3F:
		model = "models/player/sas/sas.mdl";
		break;
	default:
		return 0;
	}
	return _stricmp( name, model ) == 0;
}
