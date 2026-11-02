#include "HLTV/Core/Server.h"
#include "HLTV/Core/NetSocket.h"
#include "HLTV/Core/delta.h"
#include "HLTV/common/INetwork.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/DirectorCmd.h"
#include "HLTV/common/IDirector.h"
#include "HLTV/common/IProxy.h"
#include "HLTV/common/common.h"
#include "HLTV/common/munge.h"
#include "HLTV/common/random.h"
#include "common/TokenLine.h"
#include "common/event_args.h"
#include "HLTV/common/mem.h"
#include "common/IBaseSystem.h"
#include "HLTV/Core/DownloadURL.h"
#include "filesystem.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ENTITY_STATE_SIZE		340
#define DATAGRAM_SIZE			0xFAAu
#define MAX_CHALLENGE_RETRIES	3
#define RETRY_DELAY				4.0

#define SND_VOLUME				1
#define SND_ATTENUATION			2
#define SND_LARGE_INDEX			4
#define SND_PITCH				8

#define RES_WASMISSING			2
#define RES_CUSTOM				4

extern "C" char* MD5_GetCDKeyHash( const char* key );

static const int TE_LENGTH[128] =
{
	24, 20, 6, 11, 6, 10, 12, 17, 16, 6, 6, 6, 8, -1, 9, 19,
	-2, 10, 16, 24, 24, 24, 10, 11, 16, 19, -2, 12, 16, -1, 19, 17,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2,
	-2, -2, -2,
	2, 10, 14, 12, 14, 9, 5, 17, 13, 24, 9, 17, 7, 10, 19, 19,
	12, 7, 7, 9, 16, 18, 6, 10, 13, 7, 1, 18, 15
};

Server::Server()
{
}

Server::~Server()
{
}

void Server::ExecuteCommand( int commandID, char* )
{
	m_System->Printf( "ERROR! Server::ExecuteCommand: unknown command ID %i.\n", commandID );
}

bool Server::IsVoiceBlocking()
{
	return isVoiceBlocking;
}

void Server::SetVoiceBlocking( bool blocking )
{
	isVoiceBlocking = blocking;
}

void Server::ParseBad()
{
	m_System->Printf( "WARNING! Server::ParseBad: illegal server message.\n" );
	instream->sizeError = true;
}

void Server::ParseDeltaDescription()
{
	m_World->ParseDeltaDescription( instream );
}

void Server::ParseBaseline()
{
	m_World->ParseBaseline( instream );
}

void Server::ParseChoke()
{
}

InfoString* Server::GetServerInfoString()
{
	return &serverinfo;
}

int Server::GetRate()
{
	return m_Rate;
}

int Server::GetUpdateRate()
{
	return m_UpdateRate;
}

char* Server::GetHostName()
{
	return m_HostName;
}

int Server::GetProtocol()
{
	return m_Protocol;
}

bool Server::SetProtocol( int version )
{
	if ( (unsigned int)( version - 46 ) <= 2 )
	{
		m_Protocol = version;
		return true;
	}

	return false;
}

char* Server::GetType()
{
	return "server001";
}

IWorld* Server::GetWorld()
{
	return m_World;
}

bool Server::GetAutoRetry()
{
	return m_AutoRetry;
}

void Server::SetProxy( IProxy* proxy )
{
	m_Proxy = proxy;
	m_Director = proxy->GetDirector();
}

void Server::SetDirector( IDirector* director )
{
	m_Director = director;
}

bool Server::IsConnected()
{
	return state > SERVER_CONNECTING;
}

bool Server::IsGameServer()
{
	return isGameServer;
}

bool Server::IsRelayProxy()
{
	return !isGameServer;
}

float Server::GetTime()
{
	return m_Time;
}

void Server::StopRetry()
{
	m_NextAutoRetry = 0.0;
}

void Server::SetAutoRetry( bool retry )
{
	m_AutoRetry = retry;
	if ( !retry )
		m_NextAutoRetry = 0.0;
}

void Server::SetDelayReconnect( bool delay )
{
	m_DelayReconnect = delay;
}

void Server::ParsePrint()
{
	char* text;

	text = instream->ReadString();
	m_System->Printf( ">%s\n", &text[*text <= 31] );
}

void Server::ParseSendCvarValue()
{
	instream->ReadString();
}

void Server::ParseCDTrack()
{
	int track;
	int loop;

	track = instream->ReadByte();
	loop = instream->ReadByte();
	m_World->SetCDInfo( track, loop );
}

void Server::ParseSendExtraInfo()
{
	char* fallback;
	int cheats;

	fallback = instream->ReadString();
	cheats = instream->ReadByte();
	m_World->SetExtraInfo( fallback, cheats );
}

void Server::ParseSoundFade()
{
	int percent;
	int hold;
	int fadeOut;
	int fadeIn;

	reliableData.WriteByte( svc_soundfade );
	percent = instream->ReadByte();
	reliableData.WriteByte( percent );
	hold = instream->ReadByte();
	reliableData.WriteByte( hold );
	fadeOut = instream->ReadByte();
	reliableData.WriteByte( fadeOut );
	fadeIn = instream->ReadByte();
	reliableData.WriteByte( fadeIn );
}

void Server::ParseVoiceInit()
{
	unsigned char* start;
	char* codec;
	int quality;
	int length;

	start = instream->currentByte;
	codec = instream->ReadString();
	quality = instream->ReadByte();
	length = (int)strlen( codec ) + 2;

	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_voiceinit, start, length );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseVoiceInit: unexpected server state.\n" );
	}
	else
	{
		reliableData.WriteByte( svc_voiceinit );
		reliableData.WriteString( codec );
		reliableData.WriteByte( quality );
	}

	m_World->SetVoiceEnabled( *codec != 0 );
}

void Server::ParseDecalName()
{
	int index;
	char* name;

	reliableData.WriteByte( svc_decalname );
	index = instream->ReadByte();
	reliableData.WriteByte( index );
	name = instream->ReadString();
	reliableData.WriteString( name );
}

void Server::ParseFinale()
{
	char* text;

	reliableData.WriteByte( svc_finale );
	text = instream->ReadString();
	reliableData.WriteString( text );
}

void Server::ParseCutscene()
{
	char* text;

	reliableData.WriteByte( svc_cutscene );
	text = instream->ReadString();
	reliableData.WriteString( text );
}

void Server::ParseCenterPrint()
{
	char* text;

	text = instream->ReadString();
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_centerprint, (unsigned char*)text, (int)strlen( text ) + 1 );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseStuffText: unexpected state.\n" );
	}
	else
	{
		unreliableData.WriteByte( svc_centerprint );
		unreliableData.WriteString( text );
	}

	if ( *text != 0 )
		m_System->DPrintf( ">>%s\n", text );
}

void Server::ParseDeltaPacketEntities()
{
	int count;
	int from;

	count = instream->ReadShort();
	if ( count > MAX_PACKET_ENTITIES )
		m_System->Errorf( "Server::ParsePacketEntities: entnum>MAX_PACKET_ENTITIES.\n" );

	frame.delta = 0;
	frame.entitynum = count;
	frame.entitiesSize = ENTITY_STATE_SIZE * count;
	frame.entities = entityBuffer;
	memset( entityBuffer, 0, (size_t)frame.entitiesSize );

	from = instream->ReadByte();
	if ( !m_World->UncompressEntitiesFromStream( &frame, instream, SeqNrMap[from] ) )
		instream->sizeError = true;
}

void Server::ParseStopSound()
{
	int entity;

	entity = instream->ReadShort();
	unreliableData.WriteByte( svc_stopsound );
	unreliableData.WriteShort( entity );
}

void Server::ParseRoomType()
{
	int room;

	reliableData.WriteByte( svc_roomtype );
	room = instream->ReadShort();
	reliableData.WriteShort( room );
}

void Server::ParseVoiceData()
{
	int sender;
	int size;

	sender = instream->ReadByte();
	size = instream->ReadShort();
	if ( !isVoiceBlocking && state > SERVER_CONNECTED )
	{
		voiceData.WriteByte( svc_voicedata );
		voiceData.WriteByte( sender );
		voiceData.WriteShort( size );
		voiceData.WriteBuf( instream->currentByte, size );
	}

	instream->SkipBytes( size );
}

void Server::ParseWeaponAnim()
{
	instream->SkipBytes( 2 );
}

void Server::ParseSetAngle()
{
	unsigned char* start;

	start = instream->currentByte;
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_setangle, start, 6 );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseSetAngle: unexpected server state.\n" );
	}
	else
	{
		unreliableData.WriteByte( svc_setangle );
		unreliableData.WriteBuf( start, 6 );
	}

	instream->SkipBytes( 6 );
}

void Server::ParseAddAngle()
{
	unsigned char* start;

	start = instream->currentByte;
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_addangle, start, 2 );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseAddAngle: unexpected server state.\n" );
	}
	else
	{
		unreliableData.WriteByte( svc_addangle );
		unreliableData.WriteBuf( start, 2 );
	}

	instream->SkipBytes( 2 );
}

void Server::ParseSpawnStaticSound()
{
	unsigned char* start;

	start = instream->currentByte;
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_spawnstaticsound, start, 14 );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseSpawnStaticSound: unexpected server state.\n" );
	}
	else
	{
		unreliableData.WriteByte( svc_spawnstaticsound );
		unreliableData.WriteBuf( start, 14 );
	}

	instream->SkipBytes( 14 );
}

void Server::ParseParticle()
{
	unsigned char* start;

	start = instream->currentByte;
	unreliableData.WriteByte( svc_particle );
	unreliableData.WriteBuf( start, 11 );
	instream->SkipBytes( 11 );
}

void Server::ParseVersion()
{
	unsigned int protocol;

	protocol = instream->ReadLong();
	if ( protocol != (unsigned int)m_Protocol )
		m_System->Printf( "Server::ParseVersion: Server is protocol %i instead of %i\n", protocol, PROTOCOL_VERSION );
}

void Server::ParseSendCvarValue2()
{
	instream->ReadLong();
	instream->ReadString();
}

void Server::ParseNewUserMsg()
{
	int number;
	int size;
	char name[17];

	number = instream->ReadByte();
	size = instream->ReadByte();
	if ( size == 255 )
		size = -1;

	instream->ReadBuf( 16, name );
	name[16] = 0;
	m_System->DPrintf( "Adding user message:%s(%i).\n", name, number );
	m_World->AddUserMessage( number, size, name );
}

void Server::ParseUpdateUserInfo()
{
	unsigned char* start;
	int slot;
	unsigned int userId;
	char* info;
	char hashedcdkey[16];
	int consumed;

	start = instream->currentByte;
	slot = instream->ReadByte();
	userId = instream->ReadLong();
	info = instream->ReadString();
	instream->ReadBuf( 16, hashedcdkey );
	m_World->UpdatePlayer( slot, (int)userId, info, hashedcdkey );

	if ( state == SERVER_CONNECTED )
		return;

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseUpdateUserInfo: unexpected state.\n" );
		return;
	}

	consumed = (int)( instream->currentByte - start );
	reliableData.WriteByte( svc_updateuserinfo );
	reliableData.WriteBuf( start, consumed );
}

void Server::ParseRestore()
{
	int count;
	int i;

	instream->SkipString();
	count = instream->ReadByte();
	for ( i = 0; i < count; i++ )
		instream->SkipString();
}

void Server::ParseTime()
{
	float time;

	time = instream->ReadFloat();
	m_Time = time;
	frame.time = time;
}

void Server::ParseCrosshairAngle()
{
	int pitch;
	int yaw;

	reliableData.WriteByte( svc_crosshairangle );
	pitch = instream->ReadChar();
	reliableData.WriteChar( pitch );
	yaw = instream->ReadChar();
	reliableData.WriteChar( yaw );
}

char* Server::GetPlayerName()
{
	return userinfo.ValueForKey( "name" );
}

void Server::ParsePings()
{
	unsigned char* start;
	int consumed;

	instream->StartBitMode();
	start = instream->currentByte;
	while ( instream->ReadBit() != 0 )
		instream->ReadBits( 24 );

	instream->EndBitMode();
	consumed = (int)( instream->currentByte - start );
	unreliableData.WriteByte( svc_pings );
	unreliableData.WriteBuf( start, consumed );
}

void Server::ParseTempEntity()
{
	unsigned char* start;
	int type;
	int length;

	start = instream->currentByte;
	type = instream->ReadByte();
	if ( type > 127 )
	{
		m_System->Errorf( "Server::ParseTempEntity:type >= TE_MAX.\n" );
		return;
	}

	length = TE_LENGTH[type];
	if ( length == -2 )
	{
		m_System->Errorf( "Server::ParseTempEntity:invalid type %i.\n", type );
		return;
	}

	if ( length == -1 )
	{
		if ( type == 13 )
		{
			length = 10;
			instream->SkipBytes( 8 );
			if ( instream->ReadWord() != 0 )
			{
				length = 12;
				instream->SkipBytes( 2 );
			}
		}
		else if ( type == 29 )
		{
			int extra;

			extra = 20;
			instream->SkipBytes( 5 );
			if ( instream->ReadByte() == 2 )
			{
				extra = 22;
				instream->SkipBytes( 2 );
			}

			instream->SkipBytes( 14 );
			length = instream->SkipString() + extra;
		}
		else
		{
			m_System->Printf( "ERROR! Server::ParseTempEntity: unknown type with dynamic length.\n" );
		}
	}
	else
	{
		instream->SkipBytes( TE_LENGTH[type] );
	}

	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_temp_entity, start, length + 1 );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
		m_System->Errorf( "Server::ParseTempEntity: unexpected server state.\n" );
	else
	{
		unreliableData.WriteByte( svc_temp_entity );
		unreliableData.WriteBuf( start, length + 1 );
	}
}

void Server::ParseCustomization()
{
	unsigned int slot;
	resource_t* resource;
	int flags;
	char* name;

	slot = instream->ReadByte();
	if ( slot > 31 )
	{
		m_System->DPrintf( "Bogus player index (%i) during customization parsing.\n", slot );
		instream->sizeError = true;
		return;
	}

	resource = (resource_t*)Mem_ZeroMalloc( sizeof( resource_t ) );
	resource->type = (resourcetype_t)instream->ReadByte();
	name = instream->ReadString();
	strncpy( resource->szFileName, name, 0x103u );
	resource->szFileName[259] = 0;
	resource->nIndex = instream->ReadShort();
	resource->nDownloadSize = (int)instream->ReadLong();
	flags = instream->ReadByte();
	resource->pNext = NULL;
	resource->ucFlags = (unsigned char)( flags & ~RES_WASMISSING );
	if ( ( flags & RES_CUSTOM ) != 0 )
		instream->ReadBuf( 16, resource->rgucMD5_hash );

	resource->playernum = (unsigned char)slot;
	m_System->DPrintf( "Ignoring player customization (%s).\n", resource->szFileName );
	free( resource );
}

void Server::ParseSound()
{
	unsigned char* start;
	int flags;
	float pos[3];
	int consumed;

	instream->StartBitMode();
	start = instream->currentByte;
	flags = (int)instream->ReadBits( 9 );
	if ( ( flags & SND_VOLUME ) != 0 )
		instream->ReadBits( 8 );
	if ( ( flags & SND_ATTENUATION ) != 0 )
		instream->ReadBits( 8 );

	instream->ReadBits( 22 );
	if ( ( flags & SND_LARGE_INDEX ) != 0 )
		instream->ReadBits( 8 );

	instream->ReadBitVec3Coord( pos );
	if ( ( flags & SND_PITCH ) != 0 )
		instream->ReadBits( 8 );

	instream->EndBitMode();
	consumed = (int)( instream->currentByte - start );
	unreliableData.WriteByte( svc_sound );
	unreliableData.WriteBuf( start, consumed );
}

void Server::ParseEvent()
{
	unsigned int count;
	unsigned int i;

	instream->StartBitMode();
	frame.events = instream->currentByte;
	count = instream->ReadBits( 5 );
	frame.eventnum = count;
	for ( i = 0; i < count; i++ )
	{
		instream->SkipBits( 10 );
		if ( instream->ReadBit() != 0 )
			instream->SkipBits( 11 );
		if ( instream->ReadBit() != 0 )
			m_World->ParseEvent( instream );
		if ( instream->ReadBit() != 0 )
			instream->SkipBits( 16 );
	}

	instream->EndBitMode();
	frame.eventsSize = (unsigned int)( instream->currentByte - frame.events );
}

void Server::ParseEventReliable()
{
	unsigned char* start;
	int consumed;

	instream->StartBitMode();
	start = instream->currentByte;
	instream->SkipBits( 10 );
	m_World->ParseEvent( instream );
	if ( instream->ReadBit() != 0 )
		instream->SkipBits( 16 );

	instream->EndBitMode();
	consumed = (int)( instream->currentByte - start );
	reliableData.WriteByte( svc_event_reliable );
	reliableData.WriteBuf( start, consumed );
}

bool Server::IsDemoFile()
{
	return demoFile.IsPlaying();
}

void Server::ParseResourceLocation()
{
	char* url;

	url = instream->ReadString();
	if ( url != NULL )
	{
		strncpy( g_DownloadURL, url, 0x80u );
		g_DownloadURL[127] = 0;
	}
}

void Server::ParseMoveVars()
{
	movevars_t movevars;

	movevars.gravity = instream->ReadFloat();
	movevars.stopspeed = instream->ReadFloat();
	movevars.maxspeed = instream->ReadFloat();
	movevars.spectatormaxspeed = instream->ReadFloat();
	movevars.accelerate = instream->ReadFloat();
	movevars.airaccelerate = instream->ReadFloat();
	movevars.wateraccelerate = instream->ReadFloat();
	movevars.friction = instream->ReadFloat();
	movevars.edgefriction = instream->ReadFloat();
	movevars.waterfriction = instream->ReadFloat();
	movevars.entgravity = instream->ReadFloat();
	movevars.bounce = instream->ReadFloat();
	movevars.stepsize = instream->ReadFloat();
	movevars.maxvelocity = instream->ReadFloat();
	movevars.zmax = instream->ReadFloat();
	movevars.waveHeight = instream->ReadFloat();
	movevars.footsteps = instream->ReadByte();
	movevars.rollangle = instream->ReadFloat();
	movevars.rollspeed = instream->ReadFloat();
	movevars.skycolor_r = instream->ReadFloat();
	movevars.skycolor_g = instream->ReadFloat();
	movevars.skycolor_b = instream->ReadFloat();
	movevars.skyvec_x = instream->ReadFloat();
	movevars.skyvec_y = instream->ReadFloat();
	movevars.skyvec_z = instream->ReadFloat();
	strncpy( movevars.skyName, instream->ReadString(), 31 );
	movevars.skyName[31] = 0;
	m_World->SetMoveVars( &movevars );
}

void Server::ParseTimeScale()
{
	float timescale;

	timescale = instream->ReadFloat();
	if ( m_Director != NULL )
		return;

	if ( state == SERVER_CONNECTED )
	{
		m_System->Printf( "Server::ParseTimeScale: invalid during signon.\n" );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseTimeScale: unexpected server state.\n" );
		return;
	}

	reliableData.WriteByte( svc_timescale );
	reliableData.WriteFloat( timescale );
}

void Server::ParseClientData()
{
	unsigned int delta;

	if ( m_IsHLTV )
		return;

	instream->StartBitMode();
	delta = 0;
	if ( instream->ReadBit() != 0 )
		delta = SeqNrMap[instream->ReadByte()];

	m_World->ParseClientData( instream, delta, &clientData, &m_ClientDataStruct );
	instream->EndBitMode();
}

void Server::Retry()
{
	if ( serverAddress.IsValid() && serverSocket != NULL && m_World != NULL )
	{
		Connect( m_World, &serverAddress, serverSocket );
		FireSignal( 7, NULL );
	}
	else
	{
		m_System->DPrintf( "HLTV proxy wasn't connected before.\n" );
	}
}

void Server::ParseResourceRequest()
{
	int spawnCount;
	int index;

	if ( state != SERVER_CONNECTED )
	{
		m_System->Printf( "WARNING! Server::ParseResourceRequest: custom resource request not valid - "
			"not connected\n" );
		return;
	}

	spawnCount = (int)instream->ReadLong();
	if ( spawnCount != m_ServerCount )
	{
		m_System->Printf( "WARNING! Server::ParseResourceRequest: from old level\n" );
		return;
	}

	index = (int)instream->ReadLong();
	if ( index != 0 )
	{
		m_System->Printf( "WARNING! Server::ParseResourceRequest: custom resource list request out of range\n" );
		return;
	}

	serverChannel.reliableStream.WriteByte( clc_resourcelist );
	serverChannel.reliableStream.WriteShort( 0 );
}

void Server::ParseHLTV()
{
	int command;

	command = instream->ReadByte();
	if ( command == 1 )
	{
		if ( m_Proxy != NULL )
			m_Proxy->ParseStatusMsg( instream );
		else
			instream->SkipBytes( 10 );
	}
	else if ( command == 2 )
	{
		m_System->Printf( "WARNING! Server::ParseHLTV: unexpected \"listen\" command.\n" );
	}
	else if ( command != 0 )
	{
		m_System->Printf( "WARNING! Server::ParseHLTV: unknown director command.\n" );
		instream->sizeError = true;
	}
	else
	{
		m_IsHLTV = true;
		m_ForceHLTV = false;
	}
}

void Server::ParseSetView()
{
	int entity;
	unsigned char* start;

	if ( state == SERVER_CONNECTED )
	{
		entity = instream->ReadShort();
		m_World->SetViewEntity( entity );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseAddAngle: unexpected server state.\n" );
		return;
	}

	start = instream->currentByte;
	reliableData.WriteByte( svc_setview );
	reliableData.WriteBuf( start, 2 );
	instream->SkipBytes( 2 );
}

void Server::ParsePacketEntities()
{
	int count;

	count = instream->ReadShort();
	if ( count > MAX_PACKET_ENTITIES )
	{
		m_System->Printf( "ERROR! Server::ParsePacketEntities: entnum>MAX_PACKET_ENTITIES.\n" );
		instream->sizeError = true;
		return;
	}

	frame.delta = 0;
	frame.entitynum = count;
	frame.entitiesSize = ENTITY_STATE_SIZE * count;
	frame.entities = entityBuffer;
	memset( entityBuffer, 0, (size_t)frame.entitiesSize );
	m_World->UncompressEntitiesFromStream( &frame, instream );
}

void Server::ParseLightStyle()
{
	int index;
	char* style;

	index = instream->ReadByte();
	style = instream->ReadString();
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddLightStyle( index, style );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseLightStyle: unexpected server state.\n" );
		return;
	}

	reliableData.WriteByte( svc_lightstyle );
	reliableData.WriteByte( index );
	reliableData.WriteString( style );
}

void Server::ParseFileTransferFailed()
{
	char* name;

	name = instream->ReadString();
	if ( name != NULL && *name != 0 )
		m_System->Printf( "WARNING! Downloading \"%s\" failed.\n", name );
	else
		m_System->Printf( "WARNING! Server::ParseFileTransferFailed: empty filename.\n" );
}

void Server::ParseNop()
{
}

NetAddress* Server::GetAddress()
{
	if ( IsDemoFile() )
		return NULL;

	return &serverAddress;
}

char* Server::GetDemoFileName()
{
	if ( IsDemoFile() )
		return demoFile.GetFileName();

	return NULL;
}

void Server::ParseIntermission()
{
	if ( state != SERVER_INTERMISSION )
	{
		if ( state == SERVER_RUNNING )
			state = SERVER_INTERMISSION;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", state, SERVER_INTERMISSION );
	}

	if ( !IsDemoFile() )
		reliableData.WriteByte( svc_intermission );
}

void Server::ClearFrame( bool completely )
{
	if ( completely )
	{
		userMessages.FastClear();
		reliableData.FastClear();
		frame.reliableDataSize = 0;
		frame.userMessagesSize = 0;
	}

	unreliableData.FastClear();
	voiceData.FastClear();
	clientData.FastClear();
	demoData.FastClear();
	frame.entities = NULL;
	frame.entitynum = 0;
	frame.eventnum = 0;
	frame.eventsSize = 0;
	frame.entitiesSize = 0;
	frame.time = 0.0f;
	frame.events = NULL;
	frame.demoInfo = NULL;
	memset( &demoInfo, 0, sizeof( demoInfo ) );
	frame.unreliableDataSize = 0;
	frame.voiceDataSize = 0;
	frame.clientDataSize = 0;
	frame.demoDataSize = 0;
}

void Server::CheckAutoRetry()
{
	if ( m_NextAutoRetry > 0.0 && m_SystemTime > m_NextAutoRetry )
	{
		m_NextAutoRetry = 0.0;
		Retry();
		m_System->Printf( "Automatic connection retry...\n" );
	}
}

void Server::ScheduleAutoRetry()
{
	if ( m_AutoRetry )
		m_NextAutoRetry = m_SystemTime + RETRY_DELAY;
	else
		m_NextAutoRetry = 0.0;
}

char* Server::GetCmdName( int cmd )
{
	static char description[64];
	UserMsg* msg;

	if ( cmd > svc_sendcvarvalue2 && m_World != NULL )
	{
		msg = m_World->GetUserMsg( cmd );
		if ( msg != NULL )
			snprintf( description, sizeof( description ), "UserMsg:%s", msg->szName );
		else
			strcpy( description, "Invalid UserMsg" );
	}
	else
	{
		snprintf( description, sizeof( description ), "EngMsg:%s", PARSEFUNCS[cmd].pszname );
	}

	return description;
}

void Server::ProcessEntityUpdate()
{
	entity_state_t* entities;
	unsigned int count;
	unsigned int i;

	entities = (entity_state_t*)frame.entities;
	if ( entities == NULL )
		return;

	count = frame.entitynum;
	for ( i = 0; i < count; i++ )
	{
		if ( entities[i].number - 1 == m_ClientPlayerNum )
		{
			entities[i].origin[0] = m_ClientDataStruct.origin[0];
			entities[i].origin[1] = m_ClientDataStruct.origin[1];
			entities[i].origin[2] = m_ClientDataStruct.origin[2];
		}
	}
}

void Server::ParseSetPause()
{
	BitBuffer buf( 0x80u );
	int paused;

	paused = instream->ReadByte();
	isPaused = paused != 0;
	if ( paused == 0 )
		validsequence = 0;

	m_World->SetPaused( paused != 0 );
}

char* Server::GetStatusLine()
{
	static char string[256];
	NetAddress* address;
	float in;
	float out;

	switch ( state )
	{
	case SERVER_INITILAZING:
		strcpy( string, "Initialzing.\n" );
		break;
	case SERVER_DISCONNECTED:
		strcpy( string, "Disconnected.\n" );
		break;
	case SERVER_CHALLENGING:
		address = serverChannel.GetTargetAddress();
		snprintf( string, sizeof( string ), "challenging %s.\n", address->ToString() );
		break;
	case SERVER_AUTHENTICATING:
		strcpy( string, "Authenticating.\n" );
		break;
	case SERVER_CONNECTING:
		address = serverChannel.GetTargetAddress();
		snprintf( string, sizeof( string ), "Connecting to %s.\n", address->ToString() );
		break;
	case SERVER_CONNECTED:
	case SERVER_RUNNING:
		serverChannel.GetFlowStats( &in, &out );
		address = serverChannel.GetTargetAddress();
		snprintf( string, sizeof( string ), "Connected to %s, Time %.0f, In %.2f, Out %.2f.\n",
			address->ToString(),
			m_SystemTime - serverChannel.connect_time,
			in,
			out );
		break;
	case SERVER_INTERMISSION:
		address = serverChannel.GetTargetAddress();
		snprintf( string, sizeof( string ), "Intermission (%s).\n", address->ToString() );
		break;
	default:
		m_System->Errorf( "Server::GetStatusLine: not valid state.\n" );
		break;
	}

	return string;
}

void Server::ParseInfo( BitBuffer* stream, bool detailed )
{
	serverinfo_t info;

	strncpy( info.address, serverChannel.GetTargetAddress()->ToBaseString(), sizeof( info.address ) - 1 );
	info.address[sizeof( info.address ) - 1] = 0;
	strncpy( info.name, stream->ReadString(), sizeof( info.name ) - 1 );
	info.name[sizeof( info.name ) - 1] = 0;
	strncpy( info.map, stream->ReadString(), sizeof( info.map ) - 1 );
	info.map[sizeof( info.map ) - 1] = 0;
	strncpy( info.gamedir, stream->ReadString(), sizeof( info.gamedir ) - 1 );
	info.gamedir[sizeof( info.gamedir ) - 1] = 0;
	strncpy( info.description, stream->ReadString(), sizeof( info.description ) - 1 );
	info.description[sizeof( info.description ) - 1] = 0;
	info.activePlayers = stream->ReadByte();
	info.maxPlayers = stream->ReadByte();
	info.protocol = (unsigned char)stream->ReadByte();

	if ( detailed )
	{
		info.type = (char)stream->ReadByte();
		info.os = (char)stream->ReadByte();
		info.pw = (char)stream->ReadByte();
		info.mod = stream->ReadByte() != 0;
		if ( info.mod )
		{
			strncpy( info.url_info, stream->ReadString(), sizeof( info.url_info ) - 1 );
			info.url_info[sizeof( info.url_info ) - 1] = 0;
			strncpy( info.url_dl, stream->ReadString(), sizeof( info.url_dl ) - 1 );
			info.url_dl[sizeof( info.url_dl ) - 1] = 0;
			strncpy( info.hlversion, stream->ReadString(), sizeof( info.hlversion ) - 1 );
			info.hlversion[sizeof( info.hlversion ) - 1] = 0;
			info.ver = stream->ReadLong();
			info.size = stream->ReadLong();
			info.svonly = stream->ReadByte() != 0;
			info.cldll = stream->ReadByte() != 0;
		}
		else
		{
			info.url_info[0] = 0;
			info.url_dl[0] = 0;
			info.ver = 0;
			info.size = 0;
		}

		stream->ReadByte();
	}
	else
	{
		info.type = '?';
		info.os = '?';
		info.pw = '?';
		info.mod = false;
	}

	if ( info.activePlayers == 255 || info.maxPlayers == 255 )
	{
		info.activePlayers = stream->ReadLong();
		info.maxPlayers = stream->ReadLong();
	}

	if ( m_World != NULL )
		m_World->SetServerInfo( &info );
}

void Server::Reset()
{
	serverChannel.ClearFragments();
	serverChannel.Close();
	ClearFrame( true );
	memset( SeqNrMap, 0, sizeof( SeqNrMap ) );
	validsequence = 0;
	currentRetry = 0;
	m_IsHLTV = true;
	m_ForceHLTV = false;
	isPaused = false;
	isGameServer = false;
	m_Time = 0.0f;
	demoFile.Reset();
	SetState( SERVER_DISCONNECTED );
}

void Server::AcceptBadPassword()
{
	if ( state == SERVER_CONNECTING )
	{
		m_System->Printf( "Bad server password.\n" );
		SetState( SERVER_DISCONNECTED );
	}
	else
	{
		m_System->Printf( "WARNING! Server::AcceptBadPassword: ignoring unwanted bad password return.\n" );
	}
}

void Server::AcceptRejection( char* reason )
{
	if ( state != SERVER_CONNECTING )
	{
		m_System->Printf( "WARNING! Server::AcceptRejection: ignoring unwanted rejection message.\n" );
		return;
	}

	m_System->Printf( "Connection rejected: %s\n", reason );
	SetState( SERVER_DISCONNECTED );
	ScheduleAutoRetry();
}

void Server::AcceptRedirect( char* toAddress )
{
	INetwork* network;

	if ( state != SERVER_CONNECTING )
	{
		m_System->Printf( "WARNING! Server::AcceptRedirect: ignoring unwanted redirect message.\n" );
		return;
	}

	network = serverSocket->GetNetwork();
	network->ResolveAddress( toAddress, &serverAddress );
	m_System->Printf( "Redirected to \n", serverAddress.ToString() );
	SetState( SERVER_DISCONNECTED );
	FireSignal( 3, &serverAddress );
	Retry();
}

void Server::Challenge()
{
	static float nextRetry;

	if ( currentRetry == 0 )
	{
		currentRetry = 1;
		nextRetry = 0.0f;
	}

	if ( !( m_SystemTime > nextRetry ) )
		return;

	if ( currentRetry > MAX_CHALLENGE_RETRIES )
	{
		m_System->Printf( "WARNING! Server::Challenge: Timeout after %i retries\n", MAX_CHALLENGE_RETRIES );
		SetState( SERVER_DISCONNECTED );
		ScheduleAutoRetry();
		return;
	}

	serverChannel.OutOfBandPrintf( "%s\n", "getchallenge" );
	m_System->Printf( "Challenging %s (%i/%i).\n", serverChannel.GetTargetAddress()->ToString(),
		currentRetry, MAX_CHALLENGE_RETRIES );
	nextRetry = (float)( m_SystemTime + RETRY_DELAY );
	currentRetry++;
}

bool Server::Init( IBaseSystem* system, int serial, char* name )
{
	char* rate;
	char* updaterate;

	if ( state != SERVER_INITILAZING )
		state = SERVER_INITILAZING;

	BaseSystemModule::Init( system, serial, name );
	if ( name != NULL )
	{
		strncpy( m_Name, name, 0xFEu );
		m_Name[254] = 0;
	}
	else
	{
		strcpy( m_Name, "server001" );
	}

	m_FileSystem = m_System->GetFileSystem();
	m_Rate = 10000;
	m_DelayReconnect = true;
	m_UpdateRate = 20;
	m_NextAutoRetry = 0.0;
	m_Protocol = PROTOCOL_VERSION;
	userinfo.SetMaxSize( 256 );
	userinfo.SetValueForKey( "name", "HLTV Proxy" );
	userinfo.SetValueForKey( "cl_lw", "1" );
	userinfo.SetValueForKey( "cl_lc", "1" );
	userinfo.SetValueForKey( "*hltv", "1" );
	rate = COM_VarArgs( "%i", m_Rate );
	userinfo.SetValueForKey( "rate", rate );
	updaterate = COM_VarArgs( "%i", m_UpdateRate );
	userinfo.SetValueForKey( "cl_updaterate", updaterate );
	serverSocket = NULL;
	isGameServer = false;
	isVoiceBlocking = false;
	serverChannel.Create( system, NULL, NULL );
	serverinfo.SetMaxSize( 512 );
	SetState( SERVER_DISCONNECTED );
	reliableData.Resize( DATAGRAM_SIZE );
	unreliableData.Resize( DATAGRAM_SIZE );
	voiceData.Resize( DATAGRAM_SIZE );
	userMessages.Resize( DATAGRAM_SIZE );
	clientData.Resize( DATAGRAM_SIZE );
	demoData.Resize( DATAGRAM_SIZE );
	memset( &frame, 0, sizeof( frame ) );
	frame.reliableData = reliableData.data;
	frame.unreliableData = unreliableData.data;
	frame.voiceData = voiceData.data;
	frame.userMessages = userMessages.data;
	frame.clientData = clientData.data;
	frame.demoData = demoData.data;
	unreliableData.Clear();
	reliableData.Clear();
	voiceData.Clear();
	userMessages.Clear();
	clientData.Clear();
	demoData.Clear();
	strncpy( cdkey, "2123437429222", 31 );
	cdkey[31] = 0;
	strcpy( m_HostName, "Unkown Host" );
	memset( SeqNrMap, 0, sizeof( SeqNrMap ) );
	validsequence = 0;
	m_AutoRetry = true;
	m_IsHLTV = true;
	m_ForceHLTV = false;
	isPaused = false;
	m_World = NULL;
	m_Proxy = NULL;
	m_Director = NULL;
	demoFile.Reset();
	m_State = 3;
	m_System->Printf( "Server module initialized.\n" );
	return true;
}

void Server::SendConnectPacket()
{
	static float nextRetry;
	InfoString protinfo( 1024 );
	char buffer[1024];
	char data[2048];
	char* userString;
	char* protString;
	int length;

	if ( currentRetry == 0 )
	{
		currentRetry = 1;
		nextRetry = 0.0f;
	}

	if ( !( m_SystemTime > nextRetry ) )
		return;

	if ( currentRetry > MAX_CHALLENGE_RETRIES )
	{
		m_System->Printf( "WARNING! Server::SendConnectPacket: Timeout \n" );
		SetState( SERVER_DISCONNECTED );
		ScheduleAutoRetry();
		return;
	}

	if ( m_AuthProtocol == 2 )
	{
		if ( cdkey[0] == 0 )
		{
			m_System->Printf( "Invalid CD Key\n" );
			SetState( SERVER_DISCONNECTED );
			return;
		}

		strncpy( buffer, MD5_GetCDKeyHash( cdkey ), 1023 );
		buffer[1023] = 0;
	}

	protinfo.SetValueForKey( "prot", COM_VarArgs( "%i", m_AuthProtocol ) );
	protinfo.SetValueForKey( "unique", COM_VarArgs( "%i", -1 ) );
	protinfo.SetValueForKey( "raw", buffer );
	if ( m_AuthProtocol != 2 )
		protinfo.SetValueForKey( "cdkey", MD5_GetCDKeyHash( cdkey ) );

	userString = userinfo.GetString();
	protString = protinfo.GetString();
	snprintf( data, sizeof( data ), "%c%c%c%cconnect %i %i \"%s\" \"%s\"\n",
		255, 255, 255, 255, m_Protocol, m_ChallengeNumber, protString, userString );
	length = (int)strlen( data );
	serverSocket->SendPacket( serverChannel.GetTargetAddress(), data, length );
	m_System->Printf( "Connecting to  %s (%i/%i).\n", serverChannel.GetTargetAddress()->ToString(),
		currentRetry, MAX_CHALLENGE_RETRIES );
	nextRetry = (float)( m_SystemTime + RETRY_DELAY );
	currentRetry++;
}

void Server::AcceptChallenge( char* cmdLine )
{
	TokenLine params( cmdLine );
	char* challenge;

	if ( state != SERVER_CHALLENGING )
	{
		m_System->Printf( "WARNING! Server::AcceptChallenge: ignoring unwanted challenge return.\n" );
		return;
	}

	if ( params.CountToken() <= 2 )
	{
		m_System->Printf( "WARNING! Server::AcceptChallenge: invalid challenge format.\n" );
		return;
	}

	challenge = params.GetToken( 1 );
	m_ChallengeNumber = atoi( challenge );
	m_AuthProtocol = 2;
	m_System->Printf( "Get challenge (HASHEDCDKEY)\n" );
	SetState( SERVER_CONNECTING );
	currentRetry = 0;
}

bool Server::Connect( IWorld* world, NetAddress* address, INetSocket* socket )
{
	INetwork* network;
	NetAddress* local;

	if ( socket == NULL || address == NULL || world == NULL )
	{
		m_System->Printf( "ERROR! Server::Connect: invalid parameters.\n" );
		return false;
	}

	network = socket->GetNetwork();
	local = network->GetLocalAddress();
	if ( address->Equal( local ) )
	{
		m_System->Printf( "ERROR! Server::Connect: can't connect to myself.\n" );
		return false;
	}

	Reset();
	serverAddress.FromNetAddress( address );
	serverSocket = socket;
	serverChannel.Create( m_System, socket, &serverAddress );
	m_World = world;
	SetState( SERVER_CHALLENGING );
	return true;
}

void Server::SetState( int newState )
{
	int oldState;
	bool invalid;

	oldState = state;
	if ( oldState == newState )
		return;

	switch ( newState )
	{
	case SERVER_INITILAZING:
		state = newState;
		break;
	case SERVER_DISCONNECTED:
		if ( oldState != SERVER_INITILAZING )
			serverChannel.Close();
		state = newState;
		break;
	case SERVER_CHALLENGING:
		invalid = oldState != SERVER_DISCONNECTED;
		serverChannel.SetUpdateRate( 1 );
		if ( !invalid )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", oldState, newState );
		break;
	case SERVER_AUTHENTICATING:
		if ( oldState == SERVER_CHALLENGING )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", oldState, newState );
		break;
	case SERVER_CONNECTING:
		if ( (unsigned int)( oldState - SERVER_CHALLENGING ) <= 1u )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", oldState, newState );
		break;
	case SERVER_CONNECTED:
		invalid = false;
		if ( (unsigned int)( oldState - SERVER_RUNNING ) > 1u && oldState != SERVER_CONNECTING )
		{
			if ( !IsDemoFile() )
				invalid = true;
		}

		serverChannel.SetTimeOut( 60.0 );
		serverChannel.SetUpdateRate( 30 );
		serverChannel.SetKeepAlive( true );
		if ( !invalid )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", state, newState );
		break;
	case SERVER_RUNNING:
		invalid = ( ( oldState - SERVER_CONNECTED ) & ~2 ) != 0;
		serverChannel.SetKeepAlive( false );
		serverChannel.SetTimeOut( 30.0 );
		if ( !invalid )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", state, newState );
		break;
	case SERVER_INTERMISSION:
		if ( oldState == SERVER_RUNNING )
			state = newState;
		else
			m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", oldState, newState );
		break;
	default:
		m_System->Errorf( "Server::SetState: not valid state (%i -> %i).\n", oldState, newState );
		break;
	}
}

bool Server::LoadDemo( IWorld* world, char* filename, bool forceHLTV, bool continuous )
{
	bool loaded;

	Reset();
	serverSocket = NULL;
	m_World = world;
	serverChannel.Create( m_System, NULL, NULL );
	demoFile.Init( m_World, this, &serverChannel );
	loaded = demoFile.LoadDemo( filename );
	if ( !loaded )
		return false;

	demoFile.SetContinuous( continuous );
	m_IsHLTV = false;
	m_ForceHLTV = forceHLTV;
	SetState( SERVER_CONNECTED );
	serverChannel.SetConnected( true );
	return true;
}

void Server::SendServerCommands()
{
	if ( validsequence != 0 && state > SERVER_CONNECTED )
	{
		serverChannel.unreliableStream.WriteByte( clc_delta );
		serverChannel.unreliableStream.WriteByte( (unsigned char)validsequence );
	}

	serverChannel.TransmitOutgoing();
}

float Server::GetPacketLoss()
{
	return serverChannel.GetLoss();
}

void Server::ParseDirector()
{
	unsigned char* start;
	int length;
	DirectorCmd cmd;

	start = instream->currentByte;
	length = instream->ReadByte();
	if ( m_Director != NULL )
	{
		cmd.ReadFromStream( instream );
		cmd.SetTime( m_Time );
		m_Director->AddCommand( &cmd );
		return;
	}

	instream->SkipBytes( length );
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_director, start, length + 1 );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseDirector: unexpected server state.\n" );
		return;
	}

	reliableData.WriteByte( svc_director );
	reliableData.WriteBuf( start, length + 1 );
}

void Server::SendStringCommand( char* command )
{
	if ( serverChannel.IsFakeChannel() )
		return;

	serverChannel.reliableStream.WriteByte( clc_stringcmd );
	serverChannel.reliableStream.WriteString( command );
}

void Server::SendHLTVCommand( BitBuffer* msg )
{
	if ( serverChannel.IsFakeChannel() )
		return;

	serverChannel.reliableStream.WriteByte( clc_hltv );
	serverChannel.reliableStream.WriteBuf( msg->data, msg->CurrentSize() );
}

void Server::SendUserVar( char* key, char* value )
{
	char command[1024];

	if ( key == NULL || !IsConnected() || *key == 0 || value == NULL || *value == 0 )
		return;

	snprintf( command, sizeof( command ), "setinfo \"%s\" \"%s\"\n", key, value );
	SendStringCommand( command );
}

void Server::SetUserInfo( char* key, char* value )
{
	if ( key == NULL || value == NULL )
		return;

	userinfo.SetValueForKey( key, value );
	if ( IsConnected() && *key != 0 && *value != 0 )
		SendUserVar( key, value );
}

void Server::SetPlayerName( char* newName )
{
	COM_RemoveEvilChars( newName );
	SetUserInfo( "name", newName );
}

void Server::SetRate( int rate )
{
	char* text;

	if ( rate > 20000 )
		rate = 20000;
	else if ( rate <= 999 )
		rate = 1000;

	text = COM_VarArgs( "%i", rate );
	SetUserInfo( "rate", text );
	m_Rate = rate;
}

void Server::SetUpdateRate( int updaterate )
{
	char* text;

	if ( updaterate > 100 )
		updaterate = 100;
	else if ( updaterate <= 0 )
		updaterate = 1;

	text = COM_VarArgs( "%i", updaterate );
	SetUserInfo( "cl_updaterate", text );
	m_UpdateRate = updaterate;
}

void Server::Disconnect()
{
	int i;

	if ( state == SERVER_DISCONNECTED )
		return;

	FireSignal( 6, NULL );
	serverChannel.ClearFragments();
	if ( IsDemoFile() )
	{
		demoFile.StopPlayBack();
		m_System->Printf( "Demo playback stopped.\n" );
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			SendStringCommand( "dropclient\n" );
			serverChannel.TransmitOutgoing();
		}

		m_System->Printf( "Disconnected.\n" );
	}

	m_World->FinishGame();
	Reset();
	ScheduleAutoRetry();
	serverChannel.Close();
}

void Server::ShutDown()
{
	if ( m_State == 4 )
		return;

	Disconnect();
	serverChannel.Close();
	reliableData.Free();
	unreliableData.Free();
	voiceData.Free();
	userMessages.Free();
	clientData.Free();
	demoData.Free();
	m_Listener.Clear( false );
	m_System->Printf( "Server module shutdown.\n" );
	BaseSystemModule::ShutDown();
}

void Server::ParseDisconnect()
{
	char* reason;

	reason = instream->ReadString();
	if ( reason != NULL && *reason != 0 )
		m_System->Printf( "Dropped from %s (%s).\n", serverChannel.GetTargetAddress()->ToString(), reason );
	else
		m_System->Printf( "Dropped from %s.\n", serverChannel.GetTargetAddress()->ToString() );

	if ( IsConnected() )
		ScheduleAutoRetry();

	Disconnect();
}

void Server::Reconnect()
{
	if ( state <= SERVER_CONNECTING )
		return;

	m_System->DPrintf( "Reconnecting...\n" );
	SetState( SERVER_CONNECTED );
	serverChannel.Clear();
	validsequence = 0;
	FireSignal( 2, NULL );
	SendStringCommand( "new" );
}

void Server::AcceptConnection()
{
	if ( state != SERVER_CONNECTING )
	{
		m_System->Printf( "WARNING! Server::AcceptConnection: ignoring unwanted connection return.\n" );
		return;
	}

	SetState( SERVER_CONNECTED );
	serverChannel.SetConnected( true );
	m_System->DPrintf( "Connection accepted by %s\n", serverAddress.ToString() );
	SendStringCommand( "new" );
	FireSignal( 1, &serverAddress );
}

bool Server::ProcessConnectionlessMessage( NetAddress* from, BitBuffer* stream )
{
	TokenLine params;
	char* text;
	char* command;
	char marker;

	if ( !params.SetLine( stream->ReadString() ) )
	{
		m_System->Printf( "WARNING! Server::ProcessConnectionlessMessage: message too long.\n" );
		return true;
	}

	if ( params.CountToken() == 0 )
	{
		m_System->DPrintf( "WARNING! Invalid packet from %s.\n", from->ToString() );
		return true;
	}

	command = params.GetToken( 0 );
	marker = command[0];
	if ( marker == 0 )
		return true;

	switch ( marker )
	{
	case '8':
		AcceptBadPassword();
		return true;
	case '9':
		text = params.GetLine();
		AcceptRejection( text + 1 );
		return true;
	case 'A':
	case 'G':
		AcceptChallenge( params.GetLine() );
		return true;
	case 'B':
		AcceptConnection();
		return true;
	case 'C':
		ParseInfo( stream, false );
		return true;
	case 'L':
		text = params.GetLine();
		AcceptRedirect( text + 1 );
		return true;
	case 'l':
		text = params.GetLine();
		m_System->Printf( ">%s\n", text + 1 );
		return true;
	case 'm':
		ParseInfo( stream, true );
		return true;
	default:
		m_System->DPrintf( "Server::ProcessConnectionlessServerMessage: unknown command \"%c\"\n", marker );
		return false;
	}
}

void Server::ParseStuffText()
{
	TokenLine cmdLine;
	char* command;
	char* argument;
	NetAddress to;
	INetwork* network;
	InfoString* info;

	if ( !cmdLine.SetLine( instream->ReadString() ) )
	{
		m_System->Printf( "WARNING! Server::ParseStuffText: command too long.\n" );
		return;
	}

	command = cmdLine.GetToken( 0 );
	if ( command == NULL )
	{
		m_System->Printf( "WARNING! Server::ParseStuffText: command is NULL.\n" );
		return;
	}

	if ( strcasecmp( command, "fullserverinfo" ) == 0 )
	{
		argument = cmdLine.GetToken( 1 );
		serverinfo.SetString( argument );
		m_World->SetServerInfoString( argument );
		if ( IsDemoFile() )
		{
			isGameServer = false;
		}
		else
		{
			network = serverSocket->GetNetwork();
			network->ResolveAddress( serverinfo.ValueForKey( "proxy" ), &to );
			if ( to.IsValid() )
			{
				isGameServer = false;
				m_World->SetGameServerAddress( &to );
			}
			else
			{
				isGameServer = true;
				m_World->SetGameServerAddress( &serverAddress );
				info = m_World->GetServerInfoString();
				info->SetValueForKey( "proxy", serverAddress.ToString() );
			}
		}

		FireSignal( 4, NULL );
		return;
	}

	if ( strcasecmp( command, "reconnect" ) == 0 )
	{
		if ( IsDemoFile() )
			Disconnect();
		else if ( m_DelayReconnect )
		{
			FireSignal( 5, NULL );
			m_System->DPrintf( "Delaying reconnect to broadcast complete game.\n" );
		}
		else
		{
			Reconnect();
		}

		isPaused = false;
		return;
	}

	if ( strcasecmp( command, "connect" ) == 0 )
	{
		if ( serverSocket != NULL && m_World != NULL )
		{
			network = serverSocket->GetNetwork();
			network->ResolveAddress( cmdLine.GetToken( 1 ), &to );
			Connect( m_World, &to, serverSocket );
			FireSignal( 3, NULL );
		}
		else
		{
			m_System->Printf( "ERROR! Server::ParseStuffText: received redirect while offline.\n" );
			Disconnect();
		}

		return;
	}

	if ( strcasecmp( command, "rate" ) == 0
		|| strcasecmp( command, "cl_updaterate" ) == 0
		|| strcasecmp( command, "ex_interp" ) == 0
		|| strcasecmp( command, "cl_cmdrate" ) == 0
		|| strcasecmp( command, "cl_cmdbackup" ) == 0 )
	{
		return;
	}

	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( svc_stufftext, (unsigned char*)cmdLine.m_fullLine,
			(int)strlen( cmdLine.m_fullLine ) + 1 );
		return;
	}

	if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Errorf( "Server::ParseStuffText: unexpected state.\n" );
		return;
	}

	unreliableData.WriteByte( svc_stufftext );
	unreliableData.WriteString( cmdLine.m_fullLine );
}

void Server::ParseSignonNum()
{
	int number;
	char bans[80];
	int i;

	number = instream->ReadByte();
	m_System->DPrintf( "Received signon (%i).\n", number );
	if ( IsDemoFile() )
	{
		m_World->SetServerInfo( &demoFile.serverInfo );
	}
	else
	{
		serverChannel.OutOfBandPrintf( "details" );
		m_World->UpdateServerInfo();
		SendStringCommand( "sendents" );
		SendStringCommand( "spectate" );
		SendStringCommand( COM_VarArgs( "VModEnable %d", !isVoiceBlocking ) );
		strcpy( bans, "vban" );
		for ( i = 0; i < 32; i++ )
			strcat( bans, " 0" );

		SendStringCommand( bans );
	}

	SetState( SERVER_RUNNING );
}

void Server::ParseResourceList()
{
	int total;
	int i;
	resource_t resource;
	char* name;
	int munge;
	char* command;

	instream->StartBitMode();
	total = (int)instream->ReadBits( 12 );
	for ( i = 0; i < total; i++ )
	{
		memset( &resource, 0, sizeof( resource ) );
		resource.type = (resourcetype_t)instream->ReadBits( 4 );
		name = instream->ReadBitString();
		strncpy( resource.szFileName, name, 0x103u );
		resource.szFileName[259] = 0;
		resource.nIndex = (int)instream->ReadBits( 12 );
		resource.nDownloadSize = (int)instream->ReadBits( 24 );
		resource.ucFlags = (unsigned char)instream->ReadBits( 3 );
		if ( ( resource.ucFlags & RES_CUSTOM ) != 0 )
			instream->ReadBitData( resource.rgucMD5_hash, 16 );
		if ( instream->ReadBit() != 0 )
			instream->ReadBitData( resource.rguc_reserved, 32 );
		if ( strncmp( "gfx/temp/", resource.szFileName, 9 ) != 0 )
			m_World->AddResource( &resource );
	}

	m_System->Printf( "Added %i resources.\n", total );
	if ( instream->ReadBit() != 0 )
	{
		if ( !IsDemoFile() )
			m_System->Printf( "WARNING: HLTV proxy doesn't support file consistency check.\n"
				"WARNING: Disable mp_consistency on server." );

		while ( instream->ReadBit() != 0 )
		{
			if ( instream->ReadBit() != 0 )
				instream->ReadBits( 5 );
			else
				instream->ReadBits( 10 );
		}
	}

	instream->EndBitMode();
	munge = (int)m_ServerCRC;
	COM_Munge2( (unsigned char*)&munge, 4, (unsigned char)( -1 - m_ServerCount ) );
	command = COM_VarArgs( "spawn %i %i", m_ServerCount, munge );
	SendStringCommand( command );
}

bool Server::ParseUserMessage( int cmd )
{
	UserMsg* msg;
	int size;
	int stored;
	unsigned char* start;
	char command[32];
	char bans[80];
	int i;

	msg = m_World->GetUserMsg( cmd );
	if ( msg == NULL )
	{
		m_System->DPrintf( "WARNING! Server::ProcessMessage: unknown user message (%i).\n", cmd );
		return false;
	}

	size = msg->iSize;
	stored = size;
	start = instream->currentByte;
	if ( size == -1 )
	{
		size = instream->ReadByte();
		stored = size + 1;
	}

	if ( size > 192 )
	{
		m_System->DPrintf( "WARNING! Server::ParseUserMessage:  User Msg %d sent too much data (%i bytes)\n",
			cmd, size );
		return false;
	}

	instream->SkipBytes( size );
	if ( state == SERVER_CONNECTED )
	{
		m_World->AddSignonData( (unsigned char)cmd, start, stored );
	}
	else if ( state < SERVER_CONNECTED || state > SERVER_INTERMISSION )
	{
		m_System->Printf( "WARNING! Server::ParseUserMessage: unexpected server state.\n" );
	}
	else
	{
		userMessages.WriteByte( cmd );
		userMessages.WriteBuf( start, stored );
	}

	if ( strcmp( msg->szName, "SayText" ) == 0 )
	{
		m_System->Printf( "%s\n", start + 2 );
		return true;
	}

	if ( strcmp( msg->szName, "TextMsg" ) == 0 )
	{
		m_System->DPrintf( "%s\n", start + 2 );
		return true;
	}

	if ( strcmp( msg->szName, "ReqState" ) == 0 )
	{
		snprintf( command, sizeof( command ), "VModEnable %d", !isVoiceBlocking );
		SendStringCommand( command );
		strcpy( bans, "vban" );
		for ( i = 0; i < 32; i++ )
			strcat( bans, " 0" );

		SendStringCommand( bans );
	}

	return true;
}

void Server::ProcessMessage( unsigned int seqNr )
{
	int cmd;
	ParseFunction handler;

	if ( !IsConnected() )
		return;

	frame.seqnr = seqNr;
	while ( state != SERVER_DISCONNECTED )
	{
		if ( instream->sizeError )
		{
			m_System->Printf( "WARNING! Server::ProcessMessage: packet read overflow.\n" );
			break;
		}

		cmd = instream->ReadByte();
		if ( cmd == -1 )
			break;

		if ( cmd > svc_sendcvarvalue2 )
		{
			if ( !ParseUserMessage( cmd ) )
				break;
			continue;
		}

		handler = PARSEFUNCS[cmd].func;
		if ( handler == NULL )
		{
			m_System->Printf( "TODO! Server::ProcessMessage: missing parsing routine for %s.\n",
				PARSEFUNCS[cmd].pszname );
			break;
		}

		( this->*handler )();
		if ( state == SERVER_DISCONNECTED )
			break;
	}

	if ( frame.entitynum == 0 )
	{
		if ( m_World->IsActive() )
			ClearFrame( false );
		else
			ClearFrame( true );
		return;
	}

	if ( !reliableData.sizeError )
		frame.reliableDataSize = reliableData.CurrentSize();
	if ( !unreliableData.sizeError )
		frame.unreliableDataSize = unreliableData.CurrentSize();
	if ( !voiceData.sizeError )
		frame.voiceDataSize = voiceData.CurrentSize();
	if ( !clientData.sizeError )
		frame.clientDataSize = clientData.CurrentSize();
	if ( !userMessages.sizeError )
		frame.userMessagesSize = userMessages.CurrentSize();

	if ( !demoData.sizeError && !m_ForceHLTV )
		frame.demoDataSize = demoData.CurrentSize();

	if ( m_ForceHLTV )
		ProcessEntityUpdate();
	else if ( !m_IsHLTV )
		frame.demoInfo = &demoInfo;

	SeqNrMap[(unsigned char)seqNr] = (unsigned int)m_World->AddFrame( &frame );
	validsequence = (int)seqNr;
	ClearFrame( true );
}

void Server::ParseServerinfo()
{
	unsigned int protocol;
	unsigned char clientdllmd5[16];
	int maxclients;
	int playernum;
	int gametype;
#ifdef _WIN32
	char gamedir[260];
	char levelname[260];
#else
	char gamedir[4096];
	char levelname[4096];
#endif
	int extra;
	int skip;

	if ( IsDemoFile() && state == SERVER_RUNNING )
	{
		Disconnect();
		return;
	}

	m_System->DPrintf( "Serverinfo packet received.\n" );
	protocol = instream->ReadLong();
	if ( protocol != (unsigned int)m_Protocol )
	{
		m_System->Printf( "WARNING! Server returned protocol version %i, not %i\n", protocol, m_Protocol );
		Disconnect();
		return;
	}

	m_ServerCount = (int)instream->ReadLong();
	m_World->NewGame( m_ServerCount );
	m_ServerCRC = instream->ReadLong();
	instream->ReadBuf( 16, clientdllmd5 );
	maxclients = instream->ReadByte();
	playernum = instream->ReadByte();
	m_ClientPlayerNum = playernum;
	COM_UnMunge3( (unsigned char*)&m_ServerCRC, 4, (unsigned char)~playernum );
	if ( m_ForceHLTV )
	{
		if ( maxclients == 1 )
		{
			playernum = 1;
			maxclients = 2;
		}
		else
		{
			playernum = maxclients - 1;
		}
	}

	gametype = instream->ReadByte();
	strncpy( gamedir, instream->ReadString(), sizeof( gamedir ) - 1 );
	gamedir[sizeof( gamedir ) - 1] = 0;
	strncpy( m_HostName, instream->ReadString(), sizeof( m_HostName ) - 1 );
	m_HostName[sizeof( m_HostName ) - 1] = 0;
	strncpy( levelname, instream->ReadString(), sizeof( levelname ) - 1 );
	levelname[sizeof( levelname ) - 1] = 0;
	instream->SkipString();
	extra = instream->ReadByte();
	if ( extra != 0 )
	{
		skip = instream->ReadByte();
		instream->SkipBytes( skip );
		instream->SkipBytes( 16 );
	}

	m_World->SetServerInfo( (int)protocol, m_ServerCRC, clientdllmd5, maxclients, playernum,
		gametype, gamedir, m_HostName, levelname );
	if ( !IsDemoFile() )
		SetGameDirectory( "valve", gamedir );

	m_World->SetHLTV( m_IsHLTV || m_ForceHLTV );
	SetState( SERVER_CONNECTED );
	SendStringCommand( "sendres" );
}

void Server::CheckConnection()
{
	const char* reason;

	if ( state == SERVER_DISCONNECTED )
		return;

	if ( !serverChannel.IsTimedOut() && !serverChannel.IsCrashed() )
		return;

	if ( serverChannel.IsCrashed() )
		reason = "data mismatch";
	else
		reason = "time out";

	m_System->Printf( "Server connection lost (%s).\n", reason );
	Disconnect();
	ScheduleAutoRetry();
}

void Server::RunFrame( double time )
{
	int remaining;
	NetPacket* packet;

	BaseSystemModule::RunFrame( time );
	CheckConnection();
	if ( IsDemoFile() )
		demoFile.ReadDemoPacket( &demoData, &demoInfo );

	remaining = 33;
	packet = serverChannel.GetPacket();
	while ( packet != NULL )
	{
		if ( packet->connectionless )
		{
			if ( !ProcessConnectionlessMessage( &packet->address, &packet->data ) && m_Proxy != NULL )
			{
				packet->data.Reset();
				m_Proxy->ProcessConnectionlessMessage( &packet->address, &packet->data );
			}
		}
		else
		{
			instream = &packet->data;
			ProcessMessage( packet->seqnr );
		}

		serverChannel.FreePacket( packet );
		remaining--;
		if ( remaining == 0 )
			break;

		if ( IsDemoFile() )
			demoFile.ReadDemoPacket( &demoData, &demoInfo );

		packet = serverChannel.GetPacket();
	}

	switch ( state )
	{
	case SERVER_DISCONNECTED:
		CheckAutoRetry();
		break;
	case SERVER_CHALLENGING:
		Challenge();
		break;
	case SERVER_CONNECTING:
		SendConnectPacket();
		break;
	case SERVER_CONNECTED:
		return;
	case SERVER_RUNNING:
	case SERVER_INTERMISSION:
		if ( serverChannel.IsReadyToSend() )
			SendServerCommands();
		break;
	default:
		m_System->Errorf( "Server::RunFrame: not valid state.\n" );
		break;
	}
}

void Server::SetGameDirectory( const char* defaultDir, const char* gameDir )
{
#ifdef _WIN32
	char temp[MAX_PATH];
#else
	char temp[4096];
#endif
	const char* base;

	m_FileSystem->RemoveAllSearchPaths();
	if ( gameDir != NULL && strcasecmp( gameDir, defaultDir ) != 0 )
	{
		base = m_System->COM_GetBaseDir();
		sprintf( temp, "%s/%s", base, gameDir );
		m_FileSystem->AddSearchPath( temp, "GAME" );
	}

	base = m_System->COM_GetBaseDir();
	sprintf( temp, "%s/%s", base, defaultDir );
	m_FileSystem->AddSearchPath( temp, "DEFAULTGAME" );
	base = m_System->COM_GetBaseDir();
	m_FileSystem->AddSearchPath( base, "ROOT" );
}

const Server::svc_func_t Server::PARSEFUNCS[60] =
{
	{ svc_bad, (char*)"svc_bad", &Server::ParseBad },
	{ svc_nop, (char*)"svc_nop", &Server::ParseNop },
	{ svc_disconnect, (char*)"svc_disconnect", &Server::ParseDisconnect },
	{ svc_event, (char*)"svc_event", &Server::ParseEvent },
	{ svc_version, (char*)"svc_version", &Server::ParseVersion },
	{ svc_setview, (char*)"svc_setview", &Server::ParseSetView },
	{ svc_sound, (char*)"svc_sound", &Server::ParseSound },
	{ svc_time, (char*)"svc_time", &Server::ParseTime },
	{ svc_print, (char*)"svc_print", &Server::ParsePrint },
	{ svc_stufftext, (char*)"svc_stufftext", &Server::ParseStuffText },
	{ svc_setangle, (char*)"svc_setangle", &Server::ParseSetAngle },
	{ svc_serverinfo, (char*)"svc_serverinfo", &Server::ParseServerinfo },
	{ svc_lightstyle, (char*)"svc_lightstyle", &Server::ParseLightStyle },
	{ svc_updateuserinfo, (char*)"svc_updateuserinfo", &Server::ParseUpdateUserInfo },
	{ svc_deltadescription, (char*)"svc_deltadescription", &Server::ParseDeltaDescription },
	{ svc_clientdata, (char*)"svc_clientdata", &Server::ParseClientData },
	{ svc_stopsound, (char*)"svc_stopsound", &Server::ParseStopSound },
	{ svc_pings, (char*)"svc_pings", &Server::ParsePings },
	{ svc_particle, (char*)"svc_particle", &Server::ParseParticle },
	{ svc_damage, (char*)"svc_damage", &Server::ParseNop },
	{ svc_spawnstatic, (char*)"svc_spawnstatic", &Server::ParseNop },
	{ svc_event_reliable, (char*)"svc_event_reliable", &Server::ParseEventReliable },
	{ svc_spawnbaseline, (char*)"svc_spawnbaseline", &Server::ParseBaseline },
	{ svc_temp_entity, (char*)"svc_temp_entity", &Server::ParseTempEntity },
	{ svc_setpause, (char*)"svc_setpause", &Server::ParseSetPause },
	{ svc_signonnum, (char*)"svc_signonnum", &Server::ParseSignonNum },
	{ svc_centerprint, (char*)"svc_centerprint", &Server::ParseCenterPrint },
	{ svc_killedmonster, (char*)"svc_killedmonster", &Server::ParseNop },
	{ svc_foundsecret, (char*)"svc_foundsecret", &Server::ParseNop },
	{ svc_spawnstaticsound, (char*)"svc_spawnstaticsound", &Server::ParseSpawnStaticSound },
	{ svc_intermission, (char*)"svc_intermission", &Server::ParseIntermission },
	{ svc_finale, (char*)"svc_finale", &Server::ParseFinale },
	{ svc_cdtrack, (char*)"svc_cdtrack", &Server::ParseCDTrack },
	{ svc_restore, (char*)"svc_restore", &Server::ParseRestore },
	{ svc_cutscene, (char*)"svc_cutscene", &Server::ParseCutscene },
	{ svc_weaponanim, (char*)"svc_weaponanim", &Server::ParseWeaponAnim },
	{ svc_decalname, (char*)"svc_decalname", &Server::ParseDecalName },
	{ svc_roomtype, (char*)"svc_roomtype", &Server::ParseRoomType },
	{ svc_addangle, (char*)"svc_addangle", &Server::ParseAddAngle },
	{ svc_newusermsg, (char*)"svc_newusermsg", &Server::ParseNewUserMsg },
	{ svc_packetentities, (char*)"svc_packetentities", &Server::ParsePacketEntities },
	{ svc_deltapacketentities, (char*)"svc_deltapacketentities", &Server::ParseDeltaPacketEntities },
	{ svc_choke, (char*)"svc_choke", &Server::ParseChoke },
	{ svc_resourcelist, (char*)"svc_resourcelist", &Server::ParseResourceList },
	{ svc_newmovevars, (char*)"svc_newmovevars", &Server::ParseMoveVars },
	{ svc_resourcerequest, (char*)"svc_resourcerequest", &Server::ParseResourceRequest },
	{ svc_customization, (char*)"svc_customization", &Server::ParseCustomization },
	{ svc_crosshairangle, (char*)"svc_crosshairangle", &Server::ParseCrosshairAngle },
	{ svc_soundfade, (char*)"svc_soundfade", &Server::ParseSoundFade },
	{ svc_filetxferfailed, (char*)"svc_filetxferfailed", &Server::ParseFileTransferFailed },
	{ svc_hltv, (char*)"svc_hltv", &Server::ParseHLTV },
	{ svc_director, (char*)"svc_director", &Server::ParseDirector },
	{ svc_voiceinit, (char*)"svc_voiceinit", &Server::ParseVoiceInit },
	{ svc_voicedata, (char*)"svc_voicedata", &Server::ParseVoiceData },
	{ svc_sendextrainfo, (char*)"svc_sendextrainfo", &Server::ParseSendExtraInfo },
	{ svc_timescale, (char*)"svc_timescale", &Server::ParseTimeScale },
	{ svc_resourcelocation, (char*)"svc_resourcelocation", &Server::ParseResourceLocation },
	{ svc_sendcvarvalue, (char*)"svc_sendcvarvalue", &Server::ParseSendCvarValue },
	{ svc_sendcvarvalue2, (char*)"svc_sendcvarvalue2", &Server::ParseSendCvarValue2 },
	{ 255, (char*)"End of List", NULL }
};
