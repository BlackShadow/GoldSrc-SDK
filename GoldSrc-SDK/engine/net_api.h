#pragma once

#include "net.h"
#include "../common/net_api.h"

void	Net_APIInit();
void	Net_APIShutDown();
int		Net_APIProcess();
void	Net_APICheckTimeouts();
