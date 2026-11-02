#include "engine_apiproxy.h"
#include "common.h"
#include "conprint.h"
#include "enginecallbacks.h"
#include "mem.h"
#include "sys.h"
#include "../common/Sequence.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#if !defined( _WIN32 )
#define _snprintf snprintf
#define _vsnprintf vsnprintf
#endif

constexpr int SEQUENCE_FILE_PATH_BYTES = 256;
constexpr int SEQUENCE_COMMAND_MAPPING_COUNT = 23;
constexpr int SEQUENCE_NUMBER_TOKEN_BYTES = 256;

static_assert( sizeof( sequenceCommandLine_s ) == 92 );
static_assert( sizeof( sequenceEntry_s ) == 20 );

byte* COM_LoadTempFile( char* path, int* length );
void Sequence_ParseBuffer( byte* buffer, int length );
void Sequence_ResetDefaults( sequenceCommandLine_s* destination, sequenceCommandLine_s* source );
qboolean Sequence_IsNameValueChar( char character );
void Sequence_GetToken( char* token, int tokenLength );
void Sequence_GetLine( char* line, int lineMaxLen );
char Sequence_GetSymbol();
char Sequence_ParseEntry();
char Sequence_ParseLine( char startToken, sequenceEntry_s* entry );
char Sequence_ParseModifierLine( sequenceEntry_s* entry, int modifierType );
char Sequence_ParseGlobalDataBlock();
void Sequence_ExpandAllGosubs();
void Sequence_StripComments( char* buffer, int* bufferSize );
void Sequence_FlattenAllEntries();
qboolean Sequence_IsEntrySafe( sequenceEntry_s* entry );
void Sequence_CreateDefaultsCommand( sequenceEntry_s* entry );
sentenceGroupEntry_s* Sequence_AddSentenceGroup( char* groupName );
sequenceEntry_s* Sequence_GetEntryForName( const char* entryName );
sequenceCommandLine_s* Sequence_CopyCommand( sequenceCommandLine_s* original );
void Sequence_FreeEntry( sequenceEntry_s* entry );
void Sequence_FreeSentenceGroup( sentenceGroupEntry_s* group );
void Sequence_FreeSentenceGroupEntries( sentenceGroupEntry_s* group, qboolean purgeGlobals );

static sequenceEntry_s* g_sequenceList;
static sentenceGroupEntry_s* g_sentenceGroupList;

sequenceEntry_s* SequenceGet( const char* fileName, const char* entryName )
{
	g_engdstAddrs.pfnSequenceGet( &fileName, &entryName );
	return Sequence_GetEntryForName( entryName );
}

sentenceEntry_s* SequencePickSentence( const char* groupName, int pickMethod, int* picked )
{
	sentenceGroupEntry_s* group = g_sentenceGroupList;
	while ( group && _stricmp( group->groupName, groupName ) != 0 )
		group = group->nextEntry;

	g_engdstAddrs.pfnSequencePickSentence( &groupName, &pickMethod, &picked );
	int choice = 0;
	sentenceEntry_s* sentence = nullptr;
	if ( group )
	{
		choice = RandomLong( 0, static_cast<int>( group->numSentences - 1 ) );
		sentence = group->firstSentence;
		for ( int remaining = choice; remaining != 0; --remaining )
			sentence = sentence->nextEntry;
	}

	if ( picked )
		*picked = choice;
	return sentence;
}

static char* g_scan;
static char* g_lineScan;
static int g_lineNum;
static sequenceCommandLine_s g_fileScopeDefaults;
static sequenceCommandLine_s g_blockScopeDefaults;

static unsigned int g_nonGlobalSentences;

void Sequence_PurgeEntries( qboolean purgeGlobals )
{
	sequenceEntry_s* previous = nullptr;
	sequenceEntry_s* entry = g_sequenceList;
	while ( entry )
	{
		if ( entry->isGlobal && !purgeGlobals )
		{
			previous = entry;
			entry = entry->nextEntry;
			continue;
		}

		sequenceEntry_s* removed = entry;
		entry = removed->nextEntry;
		if ( previous )
			previous->nextEntry = entry;
		else
			g_sequenceList = entry;
		Sequence_FreeEntry( removed );
	}

	sentenceGroupEntry_s* previousGroup = nullptr;
	sentenceGroupEntry_s* group = g_sentenceGroupList;
	while ( group )
	{
		Sequence_FreeSentenceGroupEntries( group, purgeGlobals );
		if ( group->numSentences )
		{
			previousGroup = group;
			group = group->nextEntry;
			continue;
		}

		sentenceGroupEntry_s* removedGroup = group;
		group = removedGroup->nextEntry;
		if ( previousGroup )
			previousGroup->nextEntry = group;
		else
			g_sentenceGroupList = group;
		Sequence_FreeSentenceGroup( removedGroup );
	}
}

static char g_sequenceParseFileName[SEQUENCE_FILE_PATH_BYTES];
static qboolean g_sequenceParseFileIsGlobal;

void Sequence_ParseFile( const char* filename, qboolean isGlobal )
{
	Q_strncpy( g_sequenceParseFileName, filename, sizeof( g_sequenceParseFileName ) - 1 );
	g_sequenceParseFileIsGlobal = isGlobal;
	g_sequenceParseFileName[sizeof( g_sequenceParseFileName ) - 1] = 0;
	char path[SEQUENCE_FILE_PATH_BYTES];
	_snprintf( path, sizeof( path ), "sequences/%s.seq", filename );
	int length;
	byte* buffer = COM_LoadTempFile( path, &length );
	if ( buffer )
		Sequence_ParseBuffer( buffer, length );
	else
		Con_DPrintf( "WARNING: failed to locate sequence file %s\n", filename );
}

static void Sequence_BakeDefaults( sequenceCommandLine_s* destination, sequenceCommandLine_s* source )
{
	destination->clientMessage.effect = source->clientMessage.effect;
	destination->clientMessage.r1 = source->clientMessage.r1;
	destination->clientMessage.g1 = source->clientMessage.g1;
	destination->clientMessage.b1 = source->clientMessage.b1;
	destination->clientMessage.a1 = source->clientMessage.a1;
	destination->clientMessage.r2 = source->clientMessage.r2;
	destination->clientMessage.g2 = source->clientMessage.g2;
	destination->clientMessage.b2 = source->clientMessage.b2;
	destination->clientMessage.a2 = source->clientMessage.a2;
	destination->clientMessage.x = source->clientMessage.x;
	destination->clientMessage.y = source->clientMessage.y;
	destination->clientMessage.fadein = source->clientMessage.fadein;
	destination->clientMessage.fadeout = source->clientMessage.fadeout;
	destination->clientMessage.holdtime = source->clientMessage.holdtime;
	destination->clientMessage.fxtime = source->clientMessage.fxtime;
	destination->textChannel = source->textChannel;
	if ( destination->speakerName )
		Mem_Free( destination->speakerName );
	destination->speakerName = Mem_Strdup( source->speakerName );
	if ( destination->listenerName )
		Mem_Free( destination->listenerName );
	destination->listenerName = Mem_Strdup( source->listenerName );
}

void Sequence_ParseBuffer( byte* buffer, int bufferSize )
{
	char* input = reinterpret_cast<char*>( buffer );
	g_lineNum = 1;
	g_scan = input;
	g_lineScan = input;
	Sequence_StripComments( input, &bufferSize );
	Sequence_ResetDefaults( &g_fileScopeDefaults, nullptr );
	char token = Sequence_GetSymbol();
	while ( token )
	{
		switch ( token )
		{
		case '!':
			token = Sequence_ParseGlobalDataBlock();
			break;
		case '$':
			token = Sequence_ParseModifierLine( nullptr, 0 );
			break;
		case '%':
			token = Sequence_ParseEntry();
			break;
		default:
			Sys_Error(
				"Parsing error on line %d of %s.seq: At file scope, lines must begin with '$' (modifier) or '%' (entry block)"
				" or '!' (sentence / global data block); found '%c'\n",
				g_lineNum, g_sequenceParseFileName, token );
		}
	}

	Sequence_ExpandAllGosubs();
	Sequence_FlattenAllEntries();
}

void Sequence_ResetDefaults( sequenceCommandLine_s* destination, sequenceCommandLine_s* source )
{
	client_textmessage_t* message = &destination->clientMessage;
	if ( source )
	{
		message->effect = source->clientMessage.effect;
		message->r1 = source->clientMessage.r1;
		message->g1 = source->clientMessage.g1;
		message->b1 = source->clientMessage.b1;
		message->a1 = source->clientMessage.a1;
		message->r2 = source->clientMessage.r2;
		message->g2 = source->clientMessage.g2;
		message->b2 = source->clientMessage.b2;
		message->a2 = source->clientMessage.a2;
		message->x = source->clientMessage.x;
		message->y = source->clientMessage.y;
		message->fadein = source->clientMessage.fadein;
		message->fadeout = source->clientMessage.fadeout;
		message->holdtime = source->clientMessage.holdtime;
		message->fxtime = source->clientMessage.fxtime;
		message->pName = nullptr;
		message->pMessage = nullptr;
		destination->textChannel = source->textChannel;
		destination->delay = source->delay;
		destination->repeatCount = source->repeatCount;
		destination->nextCommandLine = nullptr;
		destination->soundFileName = nullptr;
		if ( destination->speakerName )
			Mem_Free( destination->speakerName );
		destination->speakerName = Mem_Strdup( source->speakerName );
		if ( destination->listenerName )
			Mem_Free( destination->listenerName );
		destination->listenerName = Mem_Strdup( source->listenerName );
	}
	else
	{
		constexpr byte DEFAULT_COLOR = 255;
		message->effect = 0;
		message->pName = nullptr;
		message->pMessage = nullptr;
		destination->textChannel = 0;
		destination->delay = 0.0f;
		destination->repeatCount = 0;
		destination->nextCommandLine = nullptr;
		destination->soundFileName = nullptr;
		destination->speakerName = nullptr;
		message->r1 = DEFAULT_COLOR;
		message->g1 = DEFAULT_COLOR;
		message->b1 = DEFAULT_COLOR;
		message->a1 = DEFAULT_COLOR;
		message->r2 = DEFAULT_COLOR;
		message->g2 = DEFAULT_COLOR;
		message->b2 = DEFAULT_COLOR;
		message->a2 = DEFAULT_COLOR;
		message->x = 0.5f;
		message->y = 0.5f;
		message->fadein = 0.2f;
		message->fadeout = 0.2f;
		message->holdtime = 1.6f;
		message->fxtime = 1.0f;
		destination->listenerName = nullptr;
	}
}

void Sequence_WriteDefaults( sequenceCommandLine_s* source, sequenceCommandLine_s* destination )
{
	if ( !destination )
		Sys_Error( "Attempt to bake defaults into a non-existant command." );
	if ( !source )
		Sys_Error( "Attempt to bake defaults from a non-existant command." );

	if ( source->modifierBitField & SEQUENCE_MODIFIER_EFFECT_BIT )
		destination->clientMessage.effect = source->clientMessage.effect;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_COLOR_BIT )
	{
		destination->clientMessage.r1 = source->clientMessage.r1;
		destination->clientMessage.g1 = source->clientMessage.g1;
		destination->clientMessage.b1 = source->clientMessage.b1;
		destination->clientMessage.a1 = source->clientMessage.a1;
	}
	if ( source->modifierBitField & SEQUENCE_MODIFIER_COLOR2_BIT )
	{
		destination->clientMessage.r2 = source->clientMessage.r2;
		destination->clientMessage.g2 = source->clientMessage.g2;
		destination->clientMessage.b2 = source->clientMessage.b2;
		destination->clientMessage.a2 = source->clientMessage.a2;
	}
	if ( source->modifierBitField & SEQUENCE_MODIFIER_POSITION_BIT )
	{
		destination->clientMessage.x = source->clientMessage.x;
		destination->clientMessage.y = source->clientMessage.y;
	}
	if ( source->modifierBitField & SEQUENCE_MODIFIER_FADEIN_BIT )
		destination->clientMessage.fadein = source->clientMessage.fadein;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_FADEOUT_BIT )
		destination->clientMessage.fadeout = source->clientMessage.fadeout;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_HOLDTIME_BIT )
		destination->clientMessage.holdtime = source->clientMessage.holdtime;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_FXTIME_BIT )
		destination->clientMessage.fxtime = source->clientMessage.fxtime;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_TEXTCHANNEL_BIT )
		destination->textChannel = source->textChannel;
	if ( source->modifierBitField & SEQUENCE_MODIFIER_SPEAKER_BIT )
	{
		if ( destination->speakerName )
			Mem_Free( destination->speakerName );
		destination->speakerName = Mem_Strdup( source->speakerName );
	}
	if ( source->modifierBitField & SEQUENCE_MODIFIER_LISTENER_BIT )
	{
		if ( destination->listenerName )
			Mem_Free( destination->listenerName );
		destination->listenerName = Mem_Strdup( source->listenerName );
	}
}

static sequenceCommandMapping_s g_sequenceCommandMappingTable[SEQUENCE_COMMAND_MAPPING_COUNT] = {
	{ SEQUENCE_COMMAND_PAUSE, "pause", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_TEXT, "text", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SOUND, "sound", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_FIRETARGETS, "firetargets", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_KILLTARGETS, "killtargets", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_GOSUB, "gosub", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SENTENCE, "sentence", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_REPEAT, "repeat", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_SETDEFAULTS, "setdefaults", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_MODIFIER, "modifier", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_POSTMODIFIER, "postmodifier", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_COMMAND_NOOP, "noop", SEQUENCE_TYPE_COMMAND },
	{ SEQUENCE_MODIFIER_EFFECT, "effect", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_POSITION, "position", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_COLOR, "color", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_COLOR2, "color2", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FADEIN, "fadein", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FADEOUT, "fadeout", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_HOLDTIME, "holdtime", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_FXTIME, "fxtime", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_SPEAKER, "speaker", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_LISTENER, "listener", SEQUENCE_TYPE_MODIFIER },
	{ SEQUENCE_MODIFIER_TEXTCHANNEL, "channel", SEQUENCE_TYPE_MODIFIER } };

sequenceCommandEnum_e Sequence_GetCommandEnumForName( const char* name )
{
	for ( const sequenceCommandMapping_s& mapping : g_sequenceCommandMappingTable )
	{
		if ( !Q_strcasecmp( name, mapping.commandName ) )
			return mapping.commandEnum;
	}
	return SEQUENCE_COMMAND_ERROR;
}

qboolean Sequence_IsCommandAModifier( sequenceCommandEnum_e command )
{
	for ( const sequenceCommandMapping_s& mapping : g_sequenceCommandMappingTable )
	{
		if ( mapping.commandEnum == command )
			return mapping.commandType == SEQUENCE_TYPE_MODIFIER;
	}
	Sys_Error( "Internal error caused by line %d of %s.seq: unknown command enum = %d\n", g_lineNum,
		g_sequenceParseFileName, command );
}

qboolean Sequence_SkipWhitespace()
{
	qboolean newline = false;
	for ( ; std::isspace( *g_scan ) != 0; ++g_scan )
	{
		if ( *g_scan == '\n' )
		{
			++g_lineNum;
			g_lineScan = g_scan + 1;
			newline = true;
		}
	}
	return newline;
}

void Sequence_GetNameValueString( char* token, int tokenLen )
{
	if ( tokenLen <= 0 )
		return;
	Sequence_SkipWhitespace();
	if ( !Sequence_IsNameValueChar( *g_scan ) )
	{
		int character = *g_scan;
		if ( character != '#' && character != '$' )
		{
			Sys_Error( "Parsing error on line %d of %s.seq: expected name/value, found illegal character '%c'\n",
				g_lineNum, g_sequenceParseFileName, character );
		}
		Sys_Error(
			"Parsing error on line %d of %s.seq: cannot have more than one '%c' per line; '%c' must be at the beginning of the line ONLY\n",
			g_lineNum, g_sequenceParseFileName, character, character );
	}
	int length = 0;
	for ( ; Sequence_IsNameValueChar( *g_scan ) && length < tokenLen - 1; ++length )
		token[length] = *g_scan++;
	token[length] = '\0';
}

float Sequence_ReadFloat()
{
	char token[SEQUENCE_NUMBER_TOKEN_BYTES];
	Sequence_SkipWhitespace();
	Sequence_GetNameValueString( token, sizeof( token ) );
	return static_cast<float>( std::atof( token ) );
}

int Sequence_ReadInt()
{
	char token[SEQUENCE_NUMBER_TOKEN_BYTES];
	Sequence_SkipWhitespace();
	Sequence_GetNameValueString( token, sizeof( token ) );
	return std::atoi( token );
}

void Sequence_ReadQuotedString( char* string, int length )
{
	if ( length <= 0 )
		return;

	Sequence_SkipWhitespace();
	Sequence_SkipWhitespace();
	const char opening = *g_scan;
	if ( opening )
		++g_scan;
	if ( opening != '"' )
		Sys_Error( "Parsing error on or before line %d of %s.seq: expected quote (\"), found '%c' instead\n", g_lineNum,
			g_sequenceParseFileName, opening );

	int written = 0;
	while ( *g_scan && *g_scan != '"' && written < length - 1 )
	{
		if ( *g_scan == '\n' )
			++g_lineNum;
		++written;
		*string++ = *g_scan;
		++g_scan;
	}
	*string = '\0';
	++g_scan;
}

void Sequence_ReadCommandData( sequenceCommandEnum_e command, sequenceCommandLine_s* line )
{
	constexpr int COMMAND_VALUE_BYTES = 1024;
	constexpr byte COMMAND_COLOR_ALPHA = 255;
	char value[COMMAND_VALUE_BYTES];
	switch ( command )
	{
	case SEQUENCE_COMMAND_PAUSE:
		line->delay = Sequence_ReadFloat();
		break;
	case SEQUENCE_COMMAND_FIRETARGETS:
		Sequence_ReadQuotedString( value, sizeof( value ) );
		line->fireTargetNames = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_KILLTARGETS:
		Sequence_ReadQuotedString( value, sizeof( value ) );
		line->killTargetNames = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_TEXT:
		Sequence_ReadQuotedString( value, sizeof( value ) );
		line->clientMessage.pMessage = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_SOUND:
		Sequence_SkipWhitespace();
		Sequence_GetNameValueString( value, sizeof( value ) );
		line->soundFileName = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_GOSUB:
		Sequence_SkipWhitespace();
		Sequence_GetNameValueString( value, sizeof( value ) );
		line->clientMessage.pName = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_SENTENCE:
		Sequence_SkipWhitespace();
		Sequence_GetNameValueString( value, sizeof( value ) );
		line->sentenceName = Mem_Strdup( value );
		break;
	case SEQUENCE_COMMAND_REPEAT:
		line->repeatCount = Sequence_ReadInt();
		break;
	case SEQUENCE_MODIFIER_EFFECT:
	{
		int effect = Sequence_ReadInt();
		line->modifierBitField |= SEQUENCE_MODIFIER_EFFECT_BIT;
		line->clientMessage.effect = effect;
		break;
	}
	case SEQUENCE_MODIFIER_POSITION:
	{
		line->clientMessage.x = Sequence_ReadFloat();
		float y = Sequence_ReadFloat();
		line->modifierBitField |= SEQUENCE_MODIFIER_POSITION_BIT;
		line->clientMessage.y = y;
		break;
	}
	case SEQUENCE_MODIFIER_COLOR:
	{
		line->clientMessage.r1 = static_cast<byte>( Sequence_ReadInt() );
		line->clientMessage.g1 = static_cast<byte>( Sequence_ReadInt() );
		byte blue = static_cast<byte>( Sequence_ReadInt() );
		line->modifierBitField |= SEQUENCE_MODIFIER_COLOR_BIT;
		line->clientMessage.b1 = blue;
		line->clientMessage.a1 = COMMAND_COLOR_ALPHA;
		break;
	}
	case SEQUENCE_MODIFIER_COLOR2:
	{
		line->clientMessage.r2 = static_cast<byte>( Sequence_ReadInt() );
		line->clientMessage.g2 = static_cast<byte>( Sequence_ReadInt() );
		byte blue = static_cast<byte>( Sequence_ReadInt() );
		line->modifierBitField |= SEQUENCE_MODIFIER_COLOR2_BIT;
		line->clientMessage.b2 = blue;
		line->clientMessage.a2 = COMMAND_COLOR_ALPHA;
		break;
	}
	case SEQUENCE_MODIFIER_FADEIN:
	{
		float fadein = Sequence_ReadFloat();
		line->modifierBitField |= SEQUENCE_MODIFIER_FADEIN_BIT;
		line->clientMessage.fadein = fadein;
		break;
	}
	case SEQUENCE_MODIFIER_FADEOUT:
	{
		float fadeout = Sequence_ReadFloat();
		line->modifierBitField |= SEQUENCE_MODIFIER_FADEOUT_BIT;
		line->clientMessage.fadeout = fadeout;
		break;
	}
	case SEQUENCE_MODIFIER_HOLDTIME:
	{
		float holdtime = Sequence_ReadFloat();
		line->modifierBitField |= SEQUENCE_MODIFIER_HOLDTIME_BIT;
		line->clientMessage.holdtime = holdtime;
		break;
	}
	case SEQUENCE_MODIFIER_FXTIME:
	{
		float fxtime = Sequence_ReadFloat();
		line->modifierBitField |= SEQUENCE_MODIFIER_FXTIME_BIT;
		line->clientMessage.fxtime = fxtime;
		break;
	}
	case SEQUENCE_MODIFIER_SPEAKER:
		Sequence_SkipWhitespace();
		Sequence_GetNameValueString( value, sizeof( value ) );
		line->speakerName = Mem_Strdup( value );
		line->modifierBitField |= SEQUENCE_MODIFIER_SPEAKER_BIT;
		break;
	case SEQUENCE_MODIFIER_LISTENER:
		Sequence_SkipWhitespace();
		Sequence_GetNameValueString( value, sizeof( value ) );
		line->listenerName = Mem_Strdup( value );
		line->modifierBitField |= SEQUENCE_MODIFIER_LISTENER_BIT;
		break;
	case SEQUENCE_MODIFIER_TEXTCHANNEL:
	{
		int channel = Sequence_ReadInt();
		line->modifierBitField |= SEQUENCE_MODIFIER_TEXTCHANNEL_BIT;
		line->textChannel = channel;
		break;
	}
	default:
		Sys_Error( "Internal error caused by line %d of %s.seq: unknown command enum = %d\n", g_lineNum,
			g_sequenceParseFileName, command );
	}
}

char Sequence_ParseModifier( sequenceCommandLine_s* defaults )
{
	constexpr int MODIFIER_NAME_BYTES = 256;
	char name[MODIFIER_NAME_BYTES];
	Sequence_GetNameValueString( name, sizeof( name ) );
	sequenceCommandEnum_e command = Sequence_GetCommandEnumForName( name );
	if ( command == SEQUENCE_COMMAND_ERROR )
	{
		Sys_Error(
			"Parsing error on line %d of %s.seq: unknown modifier \"%s\"\n", g_lineNum, g_sequenceParseFileName, name );
	}
	if ( !Sequence_IsCommandAModifier( command ) )
	{
		Sys_Error( "Parsing error on line %d of %s.seq: \"%s\" is a #command, not a $modifier\n", g_lineNum,
			g_sequenceParseFileName, name );
	}
	Sequence_SkipWhitespace();
	char separator = *g_scan;
	if ( separator )
		++g_scan;
	if ( separator != '=' )
	{
		Sys_Error( "Parsing error on or after line %d of %s.seq: after modifier \"%s\", expected '=', found '%c'\n",
			g_lineNum, g_sequenceParseFileName, name, separator );
	}
	Sequence_ReadCommandData( command, defaults );
	if ( !Sequence_SkipWhitespace() && *g_scan != ',' )
	{
		Sys_Error(
			"Parsing error on line %d of %s.seq: after value(s) for modifier \"%s\", expected ',' or End-Of-Line; found '%c'\n",
			g_lineNum, g_sequenceParseFileName, name, *g_scan );
	}
	Sequence_SkipWhitespace();
	char delimiter = *g_scan;
	if ( delimiter )
		++g_scan;
	return delimiter;
}

char Sequence_ParseCommand( sequenceCommandLine_s* command )
{
	char name[256];
	Sequence_GetNameValueString( name, sizeof( name ) );
	sequenceCommandEnum_e commandType = Sequence_GetCommandEnumForName( name );
	if ( commandType == SEQUENCE_COMMAND_ERROR )
		Sys_Error(
			"Parsing error on line %d of %s.seq: unknown command \"%s\"\n", g_lineNum, g_sequenceParseFileName, name );
	sequenceCommandLine_s* destination = command;
	if ( Sequence_IsCommandAModifier( commandType ) )
	{
		sequenceCommandLine_s* modifier =
			static_cast<sequenceCommandLine_s*>( Mem_ZeroMalloc( sizeof( sequenceCommandLine_s ) ) );
		modifier->commandType = SEQUENCE_COMMAND_POSTMODIFIER;
		while ( destination->nextCommandLine )
			destination = destination->nextCommandLine;
		destination->nextCommandLine = modifier;
		destination = modifier;
	}
	Sequence_SkipWhitespace();
	char delimiter = *g_scan;
	if ( delimiter )
		++g_scan;
	if ( delimiter != '=' )
		Sys_Error( "Parsing error on or before line %d of %s.seq: after command \"%s\", expected '=', found '%c'\n",
			g_lineNum, g_sequenceParseFileName, name, delimiter );
	Sequence_ReadCommandData( commandType, destination );
	Sequence_SkipWhitespace();
	char nextToken = *g_scan;
	if ( nextToken )
		++g_scan;
	return nextToken;
}

char Sequence_ParseEntry()
{
	constexpr int ENTRY_NAME_BYTES = 256;
	char name[ENTRY_NAME_BYTES];
	Sequence_GetNameValueString( name, sizeof( name ) );
	char delimiter = Sequence_GetSymbol();
	if ( delimiter != '{' )
		Sys_Error( "Parsing error on line %d of %s.seq: expected '{' to start a\n new entry block; found '%c' instead!",
			g_lineNum, g_sequenceParseFileName, delimiter );

	sequenceEntry_s* entry = static_cast<sequenceEntry_s*>( Mem_Malloc( sizeof( sequenceEntry_s ) ) );
	Sequence_ResetDefaults( &g_blockScopeDefaults, &g_fileScopeDefaults );
	entry->entryName = Mem_Strdup( name );
	entry->fileName = Mem_Strdup( g_sequenceParseFileName );
	entry->isGlobal = g_sequenceParseFileIsGlobal;
	entry->firstCommand = nullptr;
	Sequence_CreateDefaultsCommand( entry );
	delimiter = Sequence_GetSymbol();
	while ( delimiter != '}' )
		delimiter = Sequence_ParseLine( delimiter, entry );

	if ( !Sequence_IsEntrySafe( entry ) )
		Sys_Error(
			"Logic error in file %s.seq before line %d: execution of entry \"%%%s\" would cause an infinite loop!",
			g_sequenceParseFileName, g_lineNum, entry->entryName );
	entry->nextEntry = g_sequenceList;
	g_sequenceList = entry;
	return Sequence_GetSymbol();
}

qboolean Sequence_ParseSentenceLine()
{
	constexpr int SENTENCE_GROUP_NAME_BYTES = 64;
	constexpr int SENTENCE_LINE_BYTES = 1024;
	char sentenceName[SENTENCE_GROUP_NAME_BYTES];
	char groupName[SENTENCE_GROUP_NAME_BYTES];
	char data[SENTENCE_LINE_BYTES];
	Sequence_GetToken( sentenceName, sizeof( sentenceName ) );
	if ( sentenceName[0] == '}' )
		return true;
	char* suffix = sentenceName + std::strlen( sentenceName ) - 1;
	while ( std::isalpha( *suffix ) == 0 && *suffix != '_' )
		--suffix;
	if ( suffix[1] )
		suffix[1] = '\0';
	std::strcpy( groupName, sentenceName );
	Sequence_GetLine( data, sizeof( data ) );
	std::size_t last = std::strlen( data ) - 1;
	if ( data[last] == '\n' || data[last] == '\r' )
		data[last] = '\0';
	sentenceGroupEntry_s* group = g_sentenceGroupList;
	while ( group && _stricmp( group->groupName, groupName ) != 0 )
		group = group->nextEntry;
	if ( !group )
		group = Sequence_AddSentenceGroup( groupName );
	sentenceEntry_s* sentence = static_cast<sentenceEntry_s*>( Mem_Malloc( sizeof( *sentence ) ) );
	sentence->data = Mem_Strdup( data );
	sentence->isGlobal = g_sequenceParseFileIsGlobal;
	sentence->index = g_nonGlobalSentences;
	sentence->nextEntry = nullptr;
	++g_nonGlobalSentences;
	++group->numSentences;
	if ( group->firstSentence )
	{
		sentenceEntry_s* tail = group->firstSentence;
		while ( tail->nextEntry )
			tail = tail->nextEntry;
		tail->nextEntry = sentence;
	}
	else
		group->firstSentence = sentence;
	return false;
}

qboolean Sequence_ExpandGosubsForEntry( sequenceEntry_s* entry )
{
	qboolean complete = true;
	sequenceCommandLine_s* command = entry->firstCommand;
	while ( command )
	{
		sequenceCommandLine_s* tail = command;
		if ( command->clientMessage.pName )
		{
			if ( _stricmp( command->clientMessage.pName, entry->entryName ) == 0 )
				Sys_Error( "Error in %s.seq: entry \"%s\" gosubs itself!\n", entry->fileName, entry->entryName );
			sequenceEntry_s* target = Sequence_GetEntryForName( command->clientMessage.pName );
			if ( !target )
				Sys_Error( "Error in %s.seq: Gosub in entry \"%s\" specified unknown entry \"%s\"\n", entry->fileName,
					entry->entryName, command->clientMessage.pName );
			sequenceCommandLine_s* firstCopy = nullptr;
			sequenceCommandLine_s* lastCopy = nullptr;
			for ( sequenceCommandLine_s* source = target->firstCommand; source; source = source->nextCommandLine )
			{
				if ( source->commandType == SEQUENCE_COMMAND_SETDEFAULTS )
					continue;
				sequenceCommandLine_s* copy = Sequence_CopyCommand( source );
				if ( lastCopy )
					lastCopy->nextCommandLine = copy;
				else
					firstCopy = copy;
				lastCopy = copy;
			}
			if ( firstCopy )
			{
				lastCopy->nextCommandLine = command->nextCommandLine;
				command->nextCommandLine = firstCopy;
				tail = lastCopy;
			}
			Mem_Free( const_cast<char*>( command->clientMessage.pName ) );
			command->clientMessage.pName = nullptr;
			complete = false;
		}
		command = tail->nextCommandLine;
	}
	return complete;
}

void Sequence_ExpandAllGosubs()
{
	qboolean complete;
	do
	{
		complete = true;
		for ( sequenceEntry_s* entry = g_sequenceList; entry; entry = entry->nextEntry )
		{
			if ( !Sequence_ExpandGosubsForEntry( entry ) )
				complete = false;
		}
	} while ( !complete );
}

void Sequence_OnLevelLoad( const char* mapName )
{
	Sequence_PurgeEntries( false );
	Sequence_ParseFile( mapName, false );
}

void Sequence_Init()
{
	Sequence_ParseFile( "global", true );
}

qboolean Sequence_IsNameValueChar( char character )
{
	if ( std::isalnum( character ) != 0 )
		return true;
	return character == '.' || character == '-' || character == '_' || character == '/' || character == '\\';
}

qboolean Sequence_IsSymbol( char character )
{
	switch ( character )
	{
	case '"':
	case '#':
	case '$':
	case '%':
	case ',':
	case '=':
	case '@':
	case '{':
	case '}':
		return true;
	default:
		return false;
	}
}

char Sequence_GetSymbol()
{
	Sequence_SkipWhitespace();
	const char symbol = *g_scan;
	if ( symbol != '\0' )
		++g_scan;
	return symbol;
}

void Sequence_ValidateNameValueString( char* token )
{
	for ( char* scan = token; *scan != '\0'; ++scan )
	{
		if ( !Sequence_IsNameValueChar( *scan ) )
		{
			Sys_Error( "Parsing error on line %d of %s.seq: name/value string \"%s\" had illegal character '%c'\n",
				g_lineNum, g_sequenceParseFileName, token, *scan );
		}
	}
}

void Sequence_GetToken( char* token, int tokenLen )
{
	Sequence_SkipWhitespace();
	const char character = *g_scan;
	if ( Sequence_IsNameValueChar( character ) )
	{
		if ( tokenLen > 0 )
			Sequence_GetNameValueString( token, tokenLen );
		return;
	}
	if ( !Sequence_IsSymbol( character ) )
	{
		Sys_Error( "Parsing error on line %d of %s.seq: expected token, found '%c' instead\n", g_lineNum,
			g_sequenceParseFileName, character );
	}
	_snprintf( token, tokenLen, "%c", character );
	++g_scan;
}

void Sequence_GetLine( char* line, int lineMaxLen )
{
	Sequence_SkipWhitespace();
	const char* start = g_scan;
	char* end = std::strchr( g_scan, '\n' );
	if ( !end )
	{
		Sys_Error( "Syntax Error on line %d of %s.seq: expected sentence definition or '}', found End-Of-File!\n",
			g_lineNum, g_sequenceParseFileName );
	}
	const int length = static_cast<int>( end - start );
	if ( length >= lineMaxLen )
	{
		Sys_Error( "Syntax Error on line %d of %s.seq: line was too long (was %d chars; max is %d chars)\n", g_lineNum,
			g_sequenceParseFileName, length, lineMaxLen - 1 );
	}
	std::strncpy( line, start, length );
	line[length] = '\0';
	g_scan = end;
}

void Sequence_StripComments( char* buffer, int* pBufSize )
{
	char* write = buffer;
	char* read = buffer;
	char* end = buffer + *pBufSize;
	bool finished = false;
	while ( read < end && *read != '\0' && !finished )
	{
		if ( read[0] == '/' && read[1] == '/' )
		{
			while ( *read != '\0' && *read != '\n' && *read != '\r' )
				++read;
			continue;
		}
		if ( read[0] != '/' || read[1] != '*' )
		{
			*write++ = *read++;
			continue;
		}

		char current = read[2];
		char* look = read + 3;
		read += 2;
		if ( current == '\0' )
			break;

		bool closed = false;
		while ( *look != '\0' )
		{
			if ( *look == '/' && current == '*' )
			{
				read = look + 1;
				closed = true;
				break;
			}
			if ( current == '\n' || current == '\r' )
				*write++ = current;
			++look;
			current = *( look - 1 );
			read = look - 1;
			if ( current == '\0' )
			{
				finished = true;
				break;
			}
		}
		if ( finished || closed )
			continue;
		if ( read >= end )
			break;
	}
	*write = '\0';
}

void Sequence_ReadString( char* string, int stringLen )
{
	Sequence_SkipWhitespace();
	if ( stringLen > 0 )
		Sequence_GetNameValueString( string, stringLen );
}

qboolean Sequence_ConfirmCarriageReturnOrSymbol( char symbol )
{
	const qboolean newline = Sequence_SkipWhitespace();
	return newline || symbol == *g_scan;
}

void Sequence_FreeDataPointer( void* data )
{
	if ( data )
		Mem_Free( data );
}

void Sequence_AddCommandLineToEntry( sequenceCommandLine_s* commandLine, sequenceEntry_s* entry )
{
	sequenceCommandLine_s* tail = entry->firstCommand;
	if ( !tail )
		entry->firstCommand = commandLine;
	else
	{
		while ( tail->nextCommandLine )
			tail = tail->nextCommandLine;
		tail->nextCommandLine = commandLine;
	}
	commandLine->nextCommandLine = nullptr;
}

char Sequence_ParseModifierLine( sequenceEntry_s* entry, int modifierType )
{
	for ( ;; )
	{
		char result;
		if ( modifierType == 0 )
			result = Sequence_ParseModifier( &g_fileScopeDefaults );
		else if ( modifierType == 1 )
		{
			sequenceCommandLine_s* command =
				static_cast<sequenceCommandLine_s*>( Mem_ZeroMalloc( sizeof( *command ) ) );
			command->commandType = SEQUENCE_COMMAND_MODIFIER;
			Sequence_AddCommandLineToEntry( command, entry );
			result = Sequence_ParseModifier( command );
		}
		else
			continue;
		if ( result != ',' )
			return result;
	}
}

char Sequence_ParseCommandLine( sequenceEntry_s* entry )
{
	sequenceCommandLine_s* command = static_cast<sequenceCommandLine_s*>( Mem_ZeroMalloc( sizeof( *command ) ) );
	Sequence_ResetDefaults( command, &g_blockScopeDefaults );
	Sequence_AddCommandLineToEntry( command, entry );
	char result;
	do
	{
		result = Sequence_ParseCommand( command );
	} while ( result == ',' );
	return result;
}

char Sequence_ParseMacro( sequenceEntry_s* entry )
{
	sequenceCommandLine_s* command = static_cast<sequenceCommandLine_s*>( Mem_Malloc( sizeof( *command ) ) );
	Q_memset( command, 0, sizeof( *command ) );
	Sequence_ResetDefaults( command, &g_blockScopeDefaults );
	Sequence_AddCommandLineToEntry( command, entry );
	Sequence_ReadCommandData( SEQUENCE_COMMAND_GOSUB, command );
	char result = Sequence_GetSymbol();
	if ( result == ',' )
	{
		do
		{
			result = Sequence_ParseCommand( command );
		} while ( result == ',' );
	}
	return result;
}

char Sequence_ParseLine( char startToken, sequenceEntry_s* entry )
{
	switch ( startToken )
	{
	case '#':
		return Sequence_ParseCommandLine( entry );
	case '$':
		return Sequence_ParseModifierLine( entry, 1 );
	case '@':
		return Sequence_ParseMacro( entry );
	default:
		Sys_Error(
			"Parsing error on line %d of %s.seq: line must begin with either '#' (command) or '$' (modifier); found '%c'\n",
			g_lineNum, g_sequenceParseFileName, startToken );
	}
}

float Sequence_CalcEntryDuration( sequenceEntry_s* entry )
{
	float duration = 0.0f;
	for ( sequenceCommandLine_s* command = entry->firstCommand; command; command = command->nextCommandLine )
		duration += command->delay;
	return duration;
}

qboolean Sequence_DoesEntryContainInfiniteLoop( sequenceEntry_s* entry )
{
	for ( sequenceCommandLine_s* command = entry->firstCommand; command; command = command->nextCommandLine )
	{
		if ( command->repeatCount < 0 )
			return true;
	}
	return false;
}

qboolean Sequence_IsEntrySafe( sequenceEntry_s* entry )
{
	float elapsed = 0.0f;
	for ( sequenceCommandLine_s* command = entry->firstCommand; command; command = command->nextCommandLine )
	{
		elapsed += command->delay;
		if ( command->repeatCount < 0 && elapsed <= 0.0f )
			return false;
	}
	return true;
}

void Sequence_CreateDefaultsCommand( sequenceEntry_s* entry )
{
	constexpr int INITIAL_MODIFIER_MASK = 255;
	sequenceCommandLine_s* command = static_cast<sequenceCommandLine_s*>( Mem_ZeroMalloc( sizeof( *command ) ) );
	Sequence_ResetDefaults( command, &g_fileScopeDefaults );
	command->commandType = SEQUENCE_COMMAND_SETDEFAULTS;
	command->modifierBitField = INITIAL_MODIFIER_MASK;
	Sequence_AddCommandLineToEntry( command, entry );
}

sentenceGroupEntry_s* Sequence_FindSentenceGroup( const char* groupName )
{
	for ( sentenceGroupEntry_s* group = g_sentenceGroupList; group; group = group->nextEntry )
	{
		if ( Q_strcasecmp( group->groupName, groupName ) == 0 )
			return group;
	}
	return nullptr;
}

sentenceEntry_s* SequenceGetSentenceByIndex( unsigned int index )
{
	sentenceGroupEntry_s* group = g_sentenceGroupList;
	if ( !group )
		return nullptr;

	unsigned int base = group->numSentences;
	if ( index >= base )
	{
		for ( ;; )
		{
			group = group->nextEntry;
			if ( !group )
				return nullptr;
			if ( base + group->numSentences > index )
				break;
			base += group->numSentences;
		}
	}
	else
		base = 0;

	sentenceEntry_s* sentence = group->firstSentence;
	while ( index > base )
	{
		++base;
		sentence = sentence->nextEntry;
	}
	return sentence;
}

sentenceGroupEntry_s* Sequence_AddSentenceGroup( char* groupName )
{
	sentenceGroupEntry_s* group = static_cast<sentenceGroupEntry_s*>( Mem_Malloc( sizeof( *group ) ) );
	group->groupName = Mem_Strdup( groupName );
	group->numSentences = 0;
	group->firstSentence = nullptr;
	group->nextEntry = nullptr;
	if ( !g_sentenceGroupList )
		g_sentenceGroupList = group;
	else
	{
		sentenceGroupEntry_s* tail = g_sentenceGroupList;
		while ( tail->nextEntry )
			tail = tail->nextEntry;
		tail->nextEntry = group;
	}
	return group;
}

void Sequence_AddSentenceToGroup( char* groupName, char* data )
{
	sentenceGroupEntry_s* group = Sequence_FindSentenceGroup( groupName );
	if ( !group )
	{
		group = Sequence_AddSentenceGroup( groupName );
		if ( !group )
		{
			Sys_Error( "Unable to allocate sentence group %s at line %d in file %s.seq", groupName, g_lineNum,
				g_sequenceParseFileName );
		}
	}

	sentenceEntry_s* sentence = static_cast<sentenceEntry_s*>( Mem_Malloc( sizeof( *sentence ) ) );
	sentence->data = Mem_Strdup( data );
	sentence->nextEntry = nullptr;
	sentence->isGlobal = g_sequenceParseFileIsGlobal;
	sentence->index = g_nonGlobalSentences;
	++group->numSentences;
	++g_nonGlobalSentences;
	if ( group->firstSentence )
	{
		sentenceEntry_s* tail = group->firstSentence;
		while ( tail->nextEntry )
			tail = tail->nextEntry;
		tail->nextEntry = sentence;
	}
	else
		group->firstSentence = sentence;
}

char Sequence_ParseSentenceBlock()
{
	const char opening = Sequence_GetSymbol();
	if ( opening != '{' )
	{
		Sys_Error(
			"Parsing error on line %d of %s.seq: expected '{' to start a\n new sentence block; found '%c' instead!",
			g_lineNum, g_sequenceParseFileName, opening );
	}
	while ( Sequence_ParseSentenceLine() == false )
	{
	}
	return Sequence_GetSymbol();
}

char Sequence_ParseGlobalDataBlock()
{
	constexpr int DATA_TYPE_BYTES = 256;
	char dataType[DATA_TYPE_BYTES];
	Sequence_GetNameValueString( dataType, sizeof( dataType ) );
	if ( Q_strcasecmp( dataType, "Sentences" ) != 0 )
	{
		Sys_Error(
			"Syntax error in file %s.seq on line %d: found global data block symbol '!' with unknown data type \"%s\"",
			g_sequenceParseFileName, g_lineNum, dataType );
	}
	return Sequence_ParseSentenceBlock();
}

sequenceEntry_s* Sequence_GetEntryForName( const char* entryName )
{
	for ( sequenceEntry_s* entry = g_sequenceList; entry; entry = entry->nextEntry )
	{
		if ( Q_strcasecmp( entryName, entry->entryName ) == 0 )
			return entry;
	}
	return nullptr;
}

sequenceCommandLine_s* Sequence_CopyCommand( sequenceCommandLine_s* commandOrig )
{
	sequenceCommandLine_s* copy = static_cast<sequenceCommandLine_s*>( Mem_ZeroMalloc( sizeof( *copy ) ) );
	copy->nextCommandLine = nullptr;
	copy->commandType = commandOrig->commandType;
	copy->modifierBitField = commandOrig->modifierBitField;
	copy->repeatCount = commandOrig->repeatCount;
	copy->delay = commandOrig->delay;
	copy->textChannel = commandOrig->textChannel;
	copy->fireTargetNames = Mem_Strdup( commandOrig->fireTargetNames );
	copy->killTargetNames = Mem_Strdup( commandOrig->killTargetNames );
	copy->listenerName = Mem_Strdup( commandOrig->listenerName );
	copy->sentenceName = Mem_Strdup( commandOrig->sentenceName );
	copy->soundFileName = Mem_Strdup( commandOrig->soundFileName );
	copy->speakerName = Mem_Strdup( commandOrig->speakerName );
	copy->clientMessage.effect = commandOrig->clientMessage.effect;
	copy->clientMessage.a1 = commandOrig->clientMessage.a1;
	copy->clientMessage.b1 = commandOrig->clientMessage.b1;
	copy->clientMessage.a2 = commandOrig->clientMessage.a2;
	copy->clientMessage.b2 = commandOrig->clientMessage.b2;
	copy->clientMessage.fadein = commandOrig->clientMessage.fadein;
	copy->clientMessage.fadeout = commandOrig->clientMessage.fadeout;
	copy->clientMessage.fxtime = commandOrig->clientMessage.fxtime;
	copy->clientMessage.g1 = commandOrig->clientMessage.g1;
	copy->clientMessage.g2 = commandOrig->clientMessage.g2;
	copy->clientMessage.holdtime = commandOrig->clientMessage.holdtime;
	copy->clientMessage.r1 = commandOrig->clientMessage.r1;
	copy->clientMessage.r2 = commandOrig->clientMessage.r2;
	copy->clientMessage.x = commandOrig->clientMessage.x;
	copy->clientMessage.y = commandOrig->clientMessage.y;
	copy->clientMessage.pMessage = Mem_Strdup( commandOrig->clientMessage.pMessage );
	copy->clientMessage.pName = Mem_Strdup( commandOrig->clientMessage.pName );
	return copy;
}

sequenceCommandLine_s* Sequence_CopyCommandList( sequenceCommandLine_s* list )
{
	sequenceCommandLine_s* head = nullptr;
	sequenceCommandLine_s* tail = nullptr;
	for ( ; list; list = list->nextCommandLine )
	{
		if ( list->commandType == SEQUENCE_COMMAND_SETDEFAULTS )
			continue;
		sequenceCommandLine_s* copy = Sequence_CopyCommand( list );
		if ( tail )
			tail->nextCommandLine = copy;
		else
			head = copy;
		tail = copy;
	}
	return head;
}

void Sequence_FlattenEntry( sequenceEntry_s* entry )
{
	sequenceCommandLine_s* previous = nullptr;
	for ( sequenceCommandLine_s* command = entry->firstCommand; command; command = command->nextCommandLine )
	{
		if ( command->commandType == SEQUENCE_COMMAND_MODIFIER )
			Sequence_WriteDefaults( command, &g_blockScopeDefaults );
		else if ( command->commandType == SEQUENCE_COMMAND_POSTMODIFIER )
			Sequence_WriteDefaults( command, previous );
		else if ( command->commandType == SEQUENCE_COMMAND_SETDEFAULTS )
		{
			Sequence_WriteDefaults( command, &g_blockScopeDefaults );
			command->commandType = SEQUENCE_COMMAND_NOOP;
		}
		else
		{
			Sequence_BakeDefaults( command, &g_blockScopeDefaults );
			previous = command;
		}
	}
}

void Sequence_FlattenAllEntries()
{
	sequenceCommandLine_s* previous = nullptr;
	for ( sequenceEntry_s* entry = g_sequenceList; entry; entry = entry->nextEntry )
	{
		for ( sequenceCommandLine_s* command = entry->firstCommand; command; command = command->nextCommandLine )
		{
			if ( command->commandType == SEQUENCE_COMMAND_MODIFIER )
				Sequence_WriteDefaults( command, &g_blockScopeDefaults );
			else if ( command->commandType == SEQUENCE_COMMAND_POSTMODIFIER )
				Sequence_WriteDefaults( command, previous );
			else if ( command->commandType == SEQUENCE_COMMAND_SETDEFAULTS )
			{
				Sequence_WriteDefaults( command, &g_blockScopeDefaults );
				command->commandType = SEQUENCE_COMMAND_NOOP;
			}
			else
			{
				Sequence_BakeDefaults( command, &g_blockScopeDefaults );
				previous = command;
			}
		}
	}
}

void Sequence_FreeCommand( sequenceCommandLine_s* kill )
{
	if ( kill->fireTargetNames )
		Mem_Free( kill->fireTargetNames );
	if ( kill->speakerName )
		Mem_Free( kill->speakerName );
	if ( kill->listenerName )
		Mem_Free( kill->listenerName );
	if ( kill->soundFileName )
		Mem_Free( kill->soundFileName );
	if ( kill->sentenceName )
		Mem_Free( kill->sentenceName );
	if ( kill->clientMessage.pName )
		Mem_Free( const_cast<char*>( kill->clientMessage.pName ) );
	if ( kill->clientMessage.pMessage )
		Mem_Free( const_cast<char*>( kill->clientMessage.pMessage ) );
}

void Sequence_FreeEntry( sequenceEntry_s* kill )
{
	if ( kill->entryName )
		Mem_Free( kill->entryName );
	if ( kill->fileName )
		Mem_Free( kill->fileName );
	while ( kill->firstCommand )
	{
		sequenceCommandLine_s* command = kill->firstCommand;
		kill->firstCommand = command->nextCommandLine;
		Sequence_FreeCommand( command );
	}
	if ( kill )
		Mem_Free( kill );
}

void Sequence_FreeSentence( sentenceEntry_s* sentenceEntry )
{
	if ( sentenceEntry->data )
		Mem_Free( sentenceEntry->data );
	if ( sentenceEntry )
		Mem_Free( sentenceEntry );
}

void Sequence_FreeSentenceGroup( sentenceGroupEntry_s* groupEntry )
{
	if ( groupEntry->groupName )
		Mem_Free( groupEntry->groupName );
	if ( groupEntry )
		Mem_Free( groupEntry );
}

void Sequence_FreeSentenceGroupEntries( sentenceGroupEntry_s* groupEntry, qboolean purgeGlobals )
{
	sentenceEntry_s* sentence = groupEntry->firstSentence;
	sentenceEntry_s* previous = nullptr;
	while ( sentence )
	{
		if ( sentence->isGlobal && !purgeGlobals )
		{
			previous = sentence;
			sentence = sentence->nextEntry;
			continue;
		}

		sentenceEntry_s* removed = sentence;
		sentence = removed->nextEntry;
		if ( previous )
			previous->nextEntry = sentence;
		else
			groupEntry->firstSentence = sentence;
		--groupEntry->numSentences;
		--g_nonGlobalSentences;
		Sequence_FreeSentence( removed );
	}
}
