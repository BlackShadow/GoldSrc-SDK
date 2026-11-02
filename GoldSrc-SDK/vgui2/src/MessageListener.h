#ifndef MESSAGELISTENER_H
#define MESSAGELISTENER_H

#ifdef _WIN32
#pragma once
#endif

#include "IMessageListener.h"

namespace vgui2
{

class CMessageListener : public IMessageListener
{
public:
	void Message( VPanel* pSender, VPanel* pReceiver, KeyValues* pKeyValues, MessageSendType_t type ) override;
};

IMessageListener*	MessageListener();

}

#endif
