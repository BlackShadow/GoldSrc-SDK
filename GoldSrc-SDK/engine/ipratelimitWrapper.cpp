#include "ipratelimit.h"

static CIPRateLimit rateChecker;

extern "C" int CheckIP( netadr_t adr )
{
	return rateChecker.CheckIP( adr );
}
