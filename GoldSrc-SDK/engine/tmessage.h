#pragma once

#include "const.h"
#include "../common/Sequence.h"

#define MAX_NETWORK_TEXT_MESSAGES 4
#define NETWORK_TEXT_MESSAGE_BYTES 512

extern client_textmessage_t gNetworkTextMessage[MAX_NETWORK_TEXT_MESSAGES];
extern char gNetworkTextMessageBuffer[MAX_NETWORK_TEXT_MESSAGES][NETWORK_TEXT_MESSAGE_BYTES];
extern const char* gNetworkMessageNames[MAX_NETWORK_TEXT_MESSAGES];

client_textmessage_t*	TextMessageGet( const char* name );
int						TextMessageDrawCharacter( int x, int y, int number, int red, int green, int blue );
char*					memfgets( byte* file, int fileSize, int* position, char* buffer, int bufferSize );
void					TrimSpace( const char* source, char* destination );
