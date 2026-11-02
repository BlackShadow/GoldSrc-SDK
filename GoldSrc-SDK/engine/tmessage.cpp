#include "common.h"
#include "tmessage.h"
#include "engine_apiproxy.h"
#include "conprint.h"
#include "mem.h"
#include "sys.h"
#include "vgui/ISurface.h"
#include "vgui_intwrap2.h"

constexpr int MAX_MESSAGES = 1000;
constexpr int NAME_HEAP_SIZE = 16384;
constexpr int TEXT_MESSAGE_LINE_BYTES = 512;
constexpr int MSGFILE_NAME = 0;
constexpr int MSGFILE_TEXT = 1;
constexpr byte NETWORK_MESSAGE_WHITE = 255;
constexpr float NETWORK_MESSAGE_CENTERED = -1.0f;

char gNetworkTextMessageBuffer[MAX_NETWORK_TEXT_MESSAGES][NETWORK_TEXT_MESSAGE_BYTES];
const char* gNetworkMessageNames[MAX_NETWORK_TEXT_MESSAGES] = {
	"__NETMESSAGE__1", "__NETMESSAGE__2", "__NETMESSAGE__3", "__NETMESSAGE__4" };

client_textmessage_t gNetworkTextMessage[MAX_NETWORK_TEXT_MESSAGES] = { { 0, NETWORK_MESSAGE_WHITE,
	NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_WHITE,
	NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_WHITE, NETWORK_MESSAGE_CENTERED, NETWORK_MESSAGE_CENTERED, 0.0f, 0.0f, 0.0f,
	0.0f, "__NETMESSAGE__1", gNetworkTextMessageBuffer[0] } };

client_textmessage_t gMessageParms;
client_textmessage_t* gMessageTable = nullptr;
int gMessageTableCount = 0;

extern client_textmessage_t tm_demomessage;

byte* COM_LoadTempFile( char* path, int* length );
int Draw_MessageCharacterAdd( int x, int y, int number, int red, int green, int blue, unsigned int font );

void SetDemoMessage( const char* pszMessage, float fFadeInTime, float fFadeOutTime, float fHoldTime )
{
	if ( !pszMessage || !pszMessage[0] )
		return;

	Q_strcpy( const_cast<char*>( tm_demomessage.pMessage ), pszMessage );
	tm_demomessage.fadein = fFadeInTime;
	tm_demomessage.fadeout = fFadeOutTime;
	tm_demomessage.holdtime = fHoldTime;
}

void TrimSpace( const char* source, char* destination )
{
	int first = 0;
	int length = Q_strlen( source );
	while ( source[first] )
	{
		char character = source[first];
		if ( character != ' ' && character != '\t' && character != '\r' && character != '\n' )
			break;
		++first;
	}
	int last = length - 1;
	while ( last > 0 )
	{
		char character = source[last];
		if ( character != ' ' && character != '\t' && character != '\r' && character != '\n' )
			break;
		--last;
	}
	int copyLength = last - first + 1;
	if ( copyLength <= 0 )
		*destination = 0;
	else
	{
		Q_strncpy( destination, source + first, copyLength );
		destination[copyLength] = 0;
	}
}

client_textmessage_t* TextMessageGet( const char* name )
{
	g_engdstAddrs.pfnTextMessageGet( &name );
	if ( Q_strcasecmp( name, "__DEMOMESSAGE__" ) == 0 )
		return &tm_demomessage;
	if ( Q_strcasecmp( name, "__NETMESSAGE__1" ) == 0 )
		return &gNetworkTextMessage[0];
	if ( Q_strcasecmp( name, "__NETMESSAGE__2" ) == 0 )
		return &gNetworkTextMessage[1];
	if ( Q_strcasecmp( name, "__NETMESSAGE__3" ) == 0 )
		return &gNetworkTextMessage[2];
	if ( Q_strcasecmp( name, "__NETMESSAGE__4" ) == 0 )
		return &gNetworkTextMessage[3];

	if ( !gMessageTable || gMessageTableCount <= 0 )
		return nullptr;
	for ( int index = 0; index < gMessageTableCount; ++index )
	{
		if ( Q_strcasecmp( name, gMessageTable[index].pName ) == 0 )
			return &gMessageTable[index];
	}
	return nullptr;
}

int TextMessageDrawCharacter( int x, int y, int number, int red, int green, int blue )
{
	g_engdstAddrs.pfnDrawCharacter( &x, &y, &number, &red, &green, &blue );
	if ( red == 0 && green == 0 && blue == 0 )
		return 0;

	vgui2::HFont font = VGUI2_GetCreditsFont();
	return Draw_MessageCharacterAdd( x, y, number, red, green, blue, font );
}

char* memfgets( byte* pMemFile, int fileSize, int* pFilePos, char* pBuffer, int bufferSize )
{
	int filePos = *pFilePos;
	if ( !pBuffer || !pMemFile )
		return nullptr;
	if ( filePos >= fileSize )
		return nullptr;

	int limit = fileSize;
	if ( fileSize - filePos >= bufferSize )
	{
		limit = filePos + bufferSize - 1;
		if ( filePos >= limit )
			return nullptr;
	}

	int end = filePos;
	byte character;
	do
	{
		character = pMemFile[end++];
	} while ( end < limit && character != '\n' );

	if ( filePos == end )
		return nullptr;

	int size = end - filePos;
	Q_memcpy( pBuffer, pMemFile + filePos, size );
	if ( size < bufferSize )
		pBuffer[size] = 0;
	*pFilePos = end;
	return pBuffer;
}

// The string "pText" is assumed to have all whitespace from both ends cut out
int IsComment( char* pText )
{
	if ( pText )
	{
		int length = Q_strlen( pText );
		if ( length >= 2 && pText[0] == '/' && pText[1] == '/' )
			return 1;
		// No text?
		if ( length > 0 )
			return 0;
	}
	// No text is a comment too
	return 1;
}

int IsStartOfText( char* pText )
{
	return pText != nullptr && pText[0] == '{';
}

int IsEndOfText( char* pText )
{
	return pText != nullptr && pText[0] == '}';
}

int IsWhiteSpace( char space )
{
	if ( space == ' ' || space == '\t' || space == '\r' || space == '\n' )
		return 1;
	return 0;
}

const char* SkipSpace( const char* pText )
{
	int pos;

	if ( !pText )
		return nullptr;

	pos = 0;
	while ( pText[pos] && IsWhiteSpace( pText[pos] ) )
		++pos;
	return pText + pos;
}

const char* SkipText( const char* pText )
{
	int pos;

	if ( !pText )
		return nullptr;

	pos = 0;
	while ( pText[pos] && !IsWhiteSpace( pText[pos] ) )
		++pos;
	return pText + pos;
}

int ParseFloats( const char* pText, float* pFloat, int count )
{
	const char* pTemp = pText;
	int index = 0;

	while ( pTemp && count > 0 )
	{
		// Skip current token / float
		pTemp = SkipText( pTemp );
		// Skip any whitespace in between
		pTemp = SkipSpace( pTemp );
		if ( pTemp )
		{
			pFloat[index] = Q_atof( pTemp );
			--count;
			++index;
		}
	}

	return count == 0;
}

int IsToken( const char* pText, const char* pTokenName )
{
	int length;

	if ( !pText || !pTokenName )
		return 0;

	length = Q_strlen( pTokenName );
	return Q_strnicmp( pText + 1, pTokenName, length ) == 0;
}

int ParseDirective( const char* pText )
{
	float tempFloat[8];

	if ( !pText || pText[0] != '$' )
		return 0;

	if ( IsToken( pText, "position" ) )
	{
		if ( ParseFloats( pText, tempFloat, 2 ) )
		{
			gMessageParms.x = tempFloat[0];
			gMessageParms.y = tempFloat[1];
		}
	}
	else if ( IsToken( pText, "effect" ) )
	{
		if ( ParseFloats( pText, tempFloat, 1 ) )
			gMessageParms.effect = static_cast<int>( tempFloat[0] );
	}
	else if ( IsToken( pText, "fxtime" ) )
	{
		if ( ParseFloats( pText, tempFloat, 1 ) )
			gMessageParms.fxtime = tempFloat[0];
	}
	else if ( IsToken( pText, "color2" ) )
	{
		if ( ParseFloats( pText, tempFloat, 3 ) )
		{
			gMessageParms.r2 = static_cast<byte>( static_cast<int>( tempFloat[0] ) );
			gMessageParms.g2 = static_cast<byte>( static_cast<int>( tempFloat[1] ) );
			gMessageParms.b2 = static_cast<byte>( static_cast<int>( tempFloat[2] ) );
		}
	}
	else if ( IsToken( pText, "color" ) )
	{
		if ( ParseFloats( pText, tempFloat, 3 ) )
		{
			gMessageParms.r1 = static_cast<byte>( static_cast<int>( tempFloat[0] ) );
			gMessageParms.g1 = static_cast<byte>( static_cast<int>( tempFloat[1] ) );
			gMessageParms.b1 = static_cast<byte>( static_cast<int>( tempFloat[2] ) );
		}
	}
	else if ( IsToken( pText, "fadein" ) )
	{
		if ( ParseFloats( pText, tempFloat, 1 ) )
			gMessageParms.fadein = tempFloat[0];
	}
	else if ( IsToken( pText, "fadeout" ) )
	{
		if ( ParseFloats( pText, tempFloat, 3 ) )
			gMessageParms.fadeout = tempFloat[0];
	}
	else if ( IsToken( pText, "holdtime" ) )
	{
		if ( ParseFloats( pText, tempFloat, 3 ) )
			gMessageParms.holdtime = tempFloat[0];
	}
	else
	{
		Con_DPrintf( "Unknown token: %s\n", pText );
	}

	return 1;
}

#ifdef _MSC_VER
#pragma warning( suppress: 6262 )
#endif
void TextMessageParse( byte* pMemFile, int fileSize )
{
	char buf[TEXT_MESSAGE_LINE_BYTES];
	char trim[TEXT_MESSAGE_LINE_BYTES];
	char currentName[TEXT_MESSAGE_LINE_BYTES];
	char nameHeap[NAME_HEAP_SIZE];
	char* pCurrentText = nullptr;
	int mode = MSGFILE_NAME; // Searching for a message name	
	int lineNumber = 0;
	int filePos = 0;
	int lastLinePos = 0;
	int messageCount = 0;
	int nameHeapSize = 0;
	client_textmessage_t textMessages[MAX_MESSAGES];

	if ( fileSize > 0 && pMemFile )
	{
		while ( memfgets( pMemFile, fileSize, &filePos, buf, TEXT_MESSAGE_LINE_BYTES ) )
		{
			if ( messageCount > 999 )
				Sys_Error( "tmessage::TextMessageParse : messageCount>=MAX_MESSAGES" );

			TrimSpace( buf, trim );
			if ( mode == MSGFILE_NAME )
			{
				if ( !IsComment( trim ) && !ParseDirective( trim ) )
				{
					if ( IsStartOfText( trim ) )
					{
						mode = MSGFILE_TEXT;
						pCurrentText = reinterpret_cast<char*>( pMemFile + filePos );
					}
					else if ( IsEndOfText( trim ) )
					{
						Con_DPrintf( "Unexpected '}' found, line %d\n", lineNumber );
						return;
					}
					else
					{
						Q_strncpy( currentName, trim, TEXT_MESSAGE_LINE_BYTES - 1 );
						currentName[TEXT_MESSAGE_LINE_BYTES - 1] = 0;
					}
				}
			}
			else if ( IsEndOfText( trim ) )
			{
				int length = Q_strlen( currentName );
				if ( nameHeapSize + length > NAME_HEAP_SIZE )
				{
					Con_DPrintf( "Error parsing file!  length > %i bytes\n", NAME_HEAP_SIZE );
					return;
				}

				Q_strcpy( nameHeap + nameHeapSize, currentName );
				// Terminate text in-place in the memory file (it's temporary memory that will be deleted)
				pMemFile[lastLinePos - 1] = 0;
				// Save name/text on heap
				textMessages[messageCount] = gMessageParms;
				textMessages[messageCount].pName = nameHeap + nameHeapSize;
				textMessages[messageCount].pMessage = pCurrentText;
				nameHeapSize += length + 1;
				++messageCount;
				// Reset parser to search for names
				mode = MSGFILE_NAME;
			}
			else if ( IsStartOfText( trim ) )
			{
				Con_DPrintf( "Unexpected '{' found, line %d\n", lineNumber );
				return;
			}

			++lineNumber;
			lastLinePos = filePos;
		}
	}

	Con_DPrintf( "Parsed %d text messages\n", messageCount );
	int textHeapSize = 0;
	for ( int index = 0; index < messageCount; ++index )
		textHeapSize += Q_strlen( textMessages[index].pMessage ) + 1;

	int messageSize = messageCount * static_cast<int>( sizeof( client_textmessage_t ) );
	gMessageTable = static_cast<client_textmessage_t*>(
		Mem_Malloc( static_cast<std::size_t>( textHeapSize + nameHeapSize + messageSize ) ) );
	Q_memcpy( gMessageTable, textMessages, messageSize );

	char* pNameHeap = reinterpret_cast<char*>( gMessageTable ) + messageSize;
	Q_memcpy( pNameHeap, nameHeap, nameHeapSize );
	char* pTextOut = pNameHeap + nameHeapSize;
	const auto nameOffset = pNameHeap - gMessageTable[0].pName;
	for ( int index = 0; index < messageCount; ++index )
	{
		gMessageTable[index].pName += nameOffset; // Adjust name pointer (parallel buffer)
		Q_strcpy( pTextOut, gMessageTable[index].pMessage ); // Copy text over
		gMessageTable[index].pMessage = pTextOut;
		pTextOut += Q_strlen( pTextOut ) + 1;
	}
	gMessageTableCount = messageCount;
}

void TextMessageShutdown()
{
	// Clear out any old data that's sitting around.
	if ( gMessageTable )
	{
		Mem_Free( gMessageTable );
		gMessageTable = nullptr;
	}
}

void TextMessageInit()
{
	int fileSize;
	byte* loaded;

	// Clear out any old data that's sitting around.
	if ( gMessageTable )
	{
		Mem_Free( gMessageTable );
		gMessageTable = nullptr;
	}

	loaded = COM_LoadTempFile( const_cast<char*>( "titles.txt" ), &fileSize );
	if ( loaded )
		TextMessageParse( loaded, fileSize );
}
