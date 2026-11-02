#include "HLTV/common/DirectorCmd.h"
#include "HLTV/common/mathlib.h"

#include <stdio.h>
#include <string.h>

#if defined( _MSC_VER )
#pragma warning( disable : 4996 )
#endif

#define svc_director	51

#define DRC_COMMAND_TYPE_BYTES			1
#define DRC_STRING_TERMINATOR_BYTES		1
#define DRC_EVENT_BYTES					8
#define DRC_MODE_BYTES					1
#define DRC_CAMERA_BYTES				15
#define DRC_TIMESCALE_BYTES				4
#define DRC_MESSAGE_HEADER_BYTES		29
#define DRC_SOUND_VOLUME_BYTES			4
#define DRC_STATUS_BYTES				10
#define DRC_CHASE_WRITE_BYTES			9
#define DRC_CHASE_READ_BYTES			7
#define DRC_INEYE_WRITE_BYTES			2
#define DRC_MAP_BYTES					10
#define DRC_MAP_STREAM_BYTES			15
#define DRC_CAMPATH_BYTES				14
#define DRC_WAYPOINT_COUNT_BYTES		1
#define DRC_WAYPOINT_BYTES				14
#define DRC_MAX_COMMAND_PAYLOAD_BYTES	( 255 - DRC_COMMAND_TYPE_BYTES )
#define DRC_COMMAND_STRING_BYTES		1024

extern "C"
{
	const char* DRC_CMD_NAME[] =
	{
		"NONE",
		"START",
		"EVENT",
		"MODE",
		"CAMERA",
		"TIMESCALE",
		"MESSAGE",
		"SOUND",
		"STATUS",
		"BANNER",
		"STUFFTEXT",
		"CHASE",
		"INEYE",
		"MAP",
		"CAMPATH",
		"WAYPOINTS",
	};
}

bool DirectorCmd::GetEventData( int& entity1, int& entity2, int& flags )
{
	if ( m_Type != DRC_CMD_EVENT )
	{
		return false;
	}

	m_Data.Reset();
	entity1 = m_Data.ReadWord();
	entity2 = m_Data.ReadWord();
	flags = (int)m_Data.ReadLong();
	return true;
}

bool DirectorCmd::GetModeData( int& mode )
{
	if ( m_Type != DRC_CMD_MODE )
	{
		return false;
	}

	m_Data.Reset();
	mode = m_Data.ReadByte();
	return true;
}

bool DirectorCmd::GetChaseData( int& entity1, int& entity2, float& distance, int& flags )
{
	if ( m_Type != DRC_CMD_CHASE )
	{
		return false;
	}

	m_Data.Reset();
	entity1 = m_Data.ReadByte();
	entity2 = m_Data.ReadByte();
	distance = m_Data.ReadFloat();
	flags = m_Data.ReadByte();
	return true;
}

bool DirectorCmd::GetInEyeData( int& player )
{
	if ( m_Type != DRC_CMD_INEYE )
	{
		return false;
	}

	m_Data.Reset();
	player = m_Data.ReadByte();
	return true;
}

bool DirectorCmd::GetMapData( int& entity, float& angle, float& distance )
{
	if ( m_Type != DRC_CMD_MAP )
	{
		return false;
	}

	m_Data.Reset();
	entity = m_Data.ReadByte();
	angle = m_Data.ReadFloat();
	distance = m_Data.ReadFloat();
	return true;
}

bool DirectorCmd::GetCameraData( vec_t* position, vec_t* angles, float& fov, int& entity )
{
	if ( m_Type != DRC_CMD_CAMERA )
	{
		return false;
	}

	m_Data.Reset();
	position[0] = m_Data.ReadCoord();
	position[1] = m_Data.ReadCoord();
	position[2] = m_Data.ReadCoord();
	angles[PITCH] = m_Data.ReadCoord();
	angles[YAW] = m_Data.ReadCoord();
	angles[ROLL] = m_Data.ReadCoord();
	fov = (float)m_Data.ReadByte();
	entity = m_Data.ReadWord();
	return true;
}

bool DirectorCmd::GetCamPathData( vec_t* position, vec_t* angles, float& fov, int& flags )
{
	if ( m_Type != DRC_CMD_CAMPATH )
	{
		return false;
	}

	m_Data.Reset();
	position[0] = m_Data.ReadCoord();
	position[1] = m_Data.ReadCoord();
	position[2] = m_Data.ReadCoord();
	angles[PITCH] = m_Data.ReadCoord();
	angles[YAW] = m_Data.ReadCoord();
	angles[ROLL] = m_Data.ReadCoord();
	fov = (float)m_Data.ReadByte();
	flags = m_Data.ReadByte();
	return true;
}

bool DirectorCmd::GetSoundData( char* name, float& volume )
{
	if ( m_Type != DRC_CMD_SOUND )
	{
		return false;
	}

	m_Data.Reset();
	strcpy( name, m_Data.ReadString() );
	volume = m_Data.ReadFloat();
	return true;
}

float DirectorCmd::GetTime()
{
	return m_Time;
}

int DirectorCmd::GetType()
{
	return m_Type;
}

char* DirectorCmd::GetName()
{
	return (char*)DRC_CMD_NAME[m_Type];
}

bool DirectorCmd::GetTimeScaleData( float& factor )
{
	if ( m_Type != DRC_CMD_TIMESCALE )
	{
		return false;
	}

	m_Data.Reset();
	factor = m_Data.ReadFloat();
	return true;
}

bool DirectorCmd::GetWayPointsData( int& number )
{
	if ( m_Type != DRC_CMD_WAYPOINTS )
	{
		return false;
	}

	m_Data.Reset();
	number = m_Data.ReadByte();
	return true;
}

bool DirectorCmd::GetMessageData( int& effect, int& color, vec_t* position, float& fadein, float& fadeout,
		float& holdtime, float& fxtime, char* text )
{
	if ( m_Type != DRC_CMD_MESSAGE )
	{
		return false;
	}

	m_Data.Reset();
	effect = m_Data.ReadByte();
	color = (int)m_Data.ReadLong();
	position[0] = m_Data.ReadFloat();
	position[1] = m_Data.ReadFloat();
	fadein = m_Data.ReadFloat();
	fadeout = m_Data.ReadFloat();
	holdtime = m_Data.ReadFloat();
	fxtime = m_Data.ReadFloat();
	strcpy( text, m_Data.ReadString() );
	return true;
}

bool DirectorCmd::GetStatusData( int& slots, int& spectators, int& proxies )
{
	if ( m_Type != DRC_CMD_STATUS )
	{
		return false;
	}

	m_Data.Reset();
	slots = (int)m_Data.ReadLong();
	spectators = (int)m_Data.ReadLong();
	proxies = m_Data.ReadWord();
	return true;
}

bool DirectorCmd::GetBannerData( char* filename )
{
	if ( m_Type != DRC_CMD_BANNER )
	{
		return false;
	}

	m_Data.Reset();
	strcpy( filename, m_Data.ReadString() );
	return true;
}

bool DirectorCmd::GetStuffTextData( char* commands )
{
	if ( m_Type != DRC_CMD_STUFFTEXT )
	{
		return false;
	}

	m_Data.Reset();
	strcpy( commands, m_Data.ReadString() );
	return true;
}

void DirectorCmd::SetEventData( int entity1, int entity2, int flags )
{
	m_Type = DRC_CMD_EVENT;
	m_Data.Resize( DRC_EVENT_BYTES );
	m_Size = DRC_EVENT_BYTES;
	m_Data.WriteWord( entity1 );
	m_Data.WriteWord( entity2 );
	m_Data.WriteLong( flags );
}

void DirectorCmd::SetChaseData( int entity1, int entity2, float distance, int flags )
{
	m_Type = DRC_CMD_CHASE;
	m_Data.Resize( DRC_CHASE_WRITE_BYTES );
	m_Size = DRC_CHASE_WRITE_BYTES;
	m_Data.WriteWord( entity1 );
	m_Data.WriteWord( entity2 );
	m_Data.WriteFloat( distance );
	m_Data.WriteByte( flags );
}

void DirectorCmd::SetInEyeData( int entity )
{
	m_Type = DRC_CMD_INEYE;
	m_Data.Resize( DRC_INEYE_WRITE_BYTES );
	m_Size = DRC_INEYE_WRITE_BYTES;
	m_Data.WriteWord( entity );
}

void DirectorCmd::SetMapData( int entity, float angle, float distance )
{
	m_Type = DRC_CMD_MAP;
	m_Data.Resize( DRC_MAP_BYTES );
	m_Size = DRC_MAP_BYTES;
	m_Data.WriteWord( entity );
	m_Data.WriteFloat( angle );
	m_Data.WriteFloat( distance );
}

void DirectorCmd::SetStartData()
{
	m_Type = DRC_CMD_START;
	m_Data.Resize( 0 );
	m_Size = 0;
}

void DirectorCmd::SetModeData( int mode )
{
	m_Type = DRC_CMD_MODE;
	m_Data.Resize( DRC_MODE_BYTES );
	m_Size = DRC_MODE_BYTES;
	m_Data.WriteByte( mode );
}

void DirectorCmd::SetCameraData( vec_t* position, vec_t* angles, float fov, int entity )
{
	m_Type = DRC_CMD_CAMERA;
	m_Data.Resize( DRC_CAMERA_BYTES );
	m_Size = DRC_CAMERA_BYTES;
	m_Data.WriteCoord( position[0] );
	m_Data.WriteCoord( position[1] );
	m_Data.WriteCoord( position[2] );
	m_Data.WriteCoord( angles[PITCH] );
	m_Data.WriteCoord( angles[YAW] );
	m_Data.WriteCoord( angles[ROLL] );
	m_Data.WriteByte( (int)fov );
	m_Data.WriteWord( entity );
}

void DirectorCmd::SetCamPathData( vec_t* position, vec_t* angles, float fov, int flags )
{
	m_Type = DRC_CMD_CAMPATH;
	m_Data.Resize( DRC_CAMPATH_BYTES );
	m_Size = DRC_CAMPATH_BYTES;
	m_Data.WriteCoord( position[0] );
	m_Data.WriteCoord( position[1] );
	m_Data.WriteCoord( position[2] );
	m_Data.WriteCoord( angles[PITCH] );
	m_Data.WriteCoord( angles[YAW] );
	m_Data.WriteCoord( angles[ROLL] );
	m_Data.WriteByte( (int)fov );
	m_Data.WriteByte( flags );
}

void DirectorCmd::SetSoundData( char* name, float volume )
{
	int size;

	size = (int)( strlen( name ) + DRC_STRING_TERMINATOR_BYTES + DRC_SOUND_VOLUME_BYTES );
	m_Type = DRC_CMD_SOUND;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( name );
	m_Data.WriteFloat( volume );
}

void DirectorCmd::SetTimeScaleData( float factor )
{
	m_Type = DRC_CMD_TIMESCALE;
	m_Data.Resize( DRC_TIMESCALE_BYTES );
	m_Size = DRC_TIMESCALE_BYTES;
	m_Data.WriteFloat( factor );
}

void DirectorCmd::SetTime( float time )
{
	m_Time = time;
}

void DirectorCmd::SetMessageData( int effect, unsigned int color, vec_t* position, float fadein, float fadeout,
		float holdtime, float fxtime, char* text )
{
	int size;

	size = (int)( strlen( text ) + DRC_MESSAGE_HEADER_BYTES + DRC_STRING_TERMINATOR_BYTES );
	m_Type = DRC_CMD_MESSAGE;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteByte( effect );
	m_Data.WriteLong( color );
	m_Data.WriteFloat( position[0] );
	m_Data.WriteFloat( position[1] );
	m_Data.WriteFloat( fadein );
	m_Data.WriteFloat( fadeout );
	m_Data.WriteFloat( holdtime );
	m_Data.WriteFloat( fxtime );
	m_Data.WriteString( text );
}

void DirectorCmd::Copy( DirectorCmd* cmd )
{
	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();
	m_Time = cmd->m_Time;
	m_Type = cmd->m_Type;
	m_Size = cmd->m_Size;
	m_Index = cmd->m_Index;
	m_Data.Resize( m_Size );
	m_Data.WriteBuf( cmd->m_Data.data, m_Size );
}

void DirectorCmd::SetStatusData( int slots, int spectators, int proxies )
{
	m_Type = DRC_CMD_STATUS;
	m_Data.Resize( DRC_STATUS_BYTES );
	m_Size = DRC_STATUS_BYTES;
	m_Data.WriteLong( slots );
	m_Data.WriteLong( spectators );
	m_Data.WriteWord( proxies );
}

void DirectorCmd::SetBannerData( char* filename )
{
	int size;

	size = (int)( strlen( filename ) + DRC_STRING_TERMINATOR_BYTES );
	m_Type = DRC_CMD_BANNER;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( filename );
}

void DirectorCmd::SetStuffTextData( char* commands )
{
	int size;

	size = (int)( strlen( commands ) + DRC_STRING_TERMINATOR_BYTES );
	m_Type = DRC_CMD_STUFFTEXT;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( commands );
}

void DirectorCmd::SetWayPoints( int number )
{
	m_Type = DRC_CMD_WAYPOINTS;
	m_Data.Resize( DRC_WAYPOINT_COUNT_BYTES );
	m_Size = DRC_WAYPOINT_COUNT_BYTES;
	m_Data.WriteByte( number );
}

bool DirectorCmd::ReadFromStream( BitBuffer* stream )
{
	int type;

	if ( stream == NULL )
	{
		return false;
	}

	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();

	type = stream->ReadByte();
	m_Type = type;

	switch ( type )
	{
	case DRC_CMD_START:
		m_Data.Resize( 0 );
		m_Size = 0;
		return true;

	case DRC_CMD_EVENT:
		m_Data.Resize( DRC_EVENT_BYTES );
		m_Size = DRC_EVENT_BYTES;
		m_Data.WriteBuf( stream, DRC_EVENT_BYTES );
		return true;

	case DRC_CMD_MODE:
	case DRC_CMD_INEYE:
		m_Data.Resize( DRC_MODE_BYTES );
		m_Size = DRC_MODE_BYTES;
		m_Data.WriteBuf( stream, DRC_MODE_BYTES );
		return true;

	case DRC_CMD_CAMERA:
		m_Data.Resize( DRC_CAMERA_BYTES );
		m_Size = DRC_CAMERA_BYTES;
		m_Data.WriteBuf( stream, DRC_CAMERA_BYTES );
		return true;

	case DRC_CMD_TIMESCALE:
		m_Data.Resize( DRC_TIMESCALE_BYTES );
		m_Size = DRC_TIMESCALE_BYTES;
		m_Data.WriteBuf( stream, DRC_TIMESCALE_BYTES );
		return true;

	case DRC_CMD_MESSAGE:
	{
		unsigned char* start;
		char* text;
		int textLength;
		int size;

		start = stream->currentByte;
		stream->SkipBytes( DRC_MESSAGE_HEADER_BYTES );
		text = stream->ReadString();
		textLength = (int)( strlen( text ) + DRC_STRING_TERMINATOR_BYTES );
		size = textLength + DRC_MESSAGE_HEADER_BYTES;
		m_Data.Resize( size );
		m_Size = size;
		m_Data.WriteBuf( start, DRC_MESSAGE_HEADER_BYTES );
		m_Data.WriteBuf( text, textLength );
		return true;
	}

	case DRC_CMD_SOUND:
	{
		char* name;
		int nameLength;
		int size;
		float volume;

		name = stream->ReadString();
		nameLength = (int)( strlen( name ) + DRC_STRING_TERMINATOR_BYTES );
		size = nameLength + DRC_SOUND_VOLUME_BYTES;
		m_Data.Resize( size );
		m_Size = size;
		m_Data.WriteBuf( name, nameLength );
		volume = stream->ReadFloat();
		m_Data.WriteFloat( volume );
		return true;
	}

	case DRC_CMD_STATUS:
		m_Data.Resize( DRC_STATUS_BYTES );
		m_Size = DRC_STATUS_BYTES;
		m_Data.WriteBuf( stream, DRC_STATUS_BYTES );
		return true;

	case DRC_CMD_BANNER:
	case DRC_CMD_STUFFTEXT:
	{
		char* text;
		int size;

		text = stream->ReadString();
		size = (int)( strlen( text ) + DRC_STRING_TERMINATOR_BYTES );
		m_Data.Resize( size );
		m_Size = size;
		m_Data.WriteBuf( text, size );
		return true;
	}

	case DRC_CMD_CHASE:
		m_Data.Resize( DRC_CHASE_READ_BYTES );
		m_Size = DRC_CHASE_READ_BYTES;
		m_Data.WriteBuf( stream, DRC_CHASE_READ_BYTES );
		return true;

	case DRC_CMD_MAP:
		m_Data.Resize( DRC_MAP_BYTES );
		m_Size = DRC_MAP_BYTES;
		m_Data.WriteBuf( stream, DRC_MAP_STREAM_BYTES );
		return true;

	case DRC_CMD_CAMPATH:
		m_Data.Resize( DRC_CAMPATH_BYTES );
		m_Size = DRC_CAMPATH_BYTES;
		m_Data.WriteBuf( stream, DRC_CAMPATH_BYTES );
		return true;

	case DRC_CMD_WAYPOINTS:
	{
		int number;

		m_Data.Resize( DRC_WAYPOINT_COUNT_BYTES );
		m_Size = DRC_WAYPOINT_COUNT_BYTES;
		number = stream->ReadByte();
		m_Data.WriteByte( number );
		stream->SkipBytes( DRC_WAYPOINT_BYTES * number );
		return true;
	}

	default:
		return false;
	}
}

void DirectorCmd::WriteToStream( BitBuffer* stream )
{
	if ( stream != NULL && m_Type >= DRC_CMD_START && m_Type <= DRC_CMD_WAYPOINTS
		&& m_Size <= DRC_MAX_COMMAND_PAYLOAD_BYTES )
	{
		stream->WriteByte( svc_director );
		stream->WriteByte( m_Size + DRC_COMMAND_TYPE_BYTES );
		stream->WriteByte( m_Type );
		stream->WriteBuf( m_Data.data, m_Size );
	}
}

char* DirectorCmd::ToString()
{
	static char s[DRC_COMMAND_STRING_BYTES];
	char text[DRC_COMMAND_STRING_BYTES];
	const char* name;

	if ( m_Type < DRC_CMD_START || m_Type > DRC_CMD_WAYPOINTS )
	{
		return NULL;
	}

	name = DRC_CMD_NAME[m_Type];
	memset( s, 0, sizeof( s ) );

	switch ( m_Type )
	{
	case DRC_CMD_START:
		sprintf( s, "%s", name );
		break;

	case DRC_CMD_EVENT:
	{
		int entity1;
		int entity2;
		int flags;

		m_Data.Reset();
		entity1 = m_Data.ReadWord();
		entity2 = m_Data.ReadWord();
		flags = (int)m_Data.ReadLong();
		sprintf( s, "%s %i %i %i", name, entity1, entity2, flags );
		break;
	}

	case DRC_CMD_MODE:
	case DRC_CMD_INEYE:
	case DRC_CMD_WAYPOINTS:
	{
		int value;

		m_Data.Reset();
		value = m_Data.ReadByte();
		sprintf( s, "%s %i", name, value );
		break;
	}

	case DRC_CMD_CAMERA:
	{
		float x;
		float y;
		float z;
		float ax;
		float ay;
		float az;
		int fov;
		int entity;

		m_Data.Reset();
		x = m_Data.ReadCoord();
		y = m_Data.ReadCoord();
		z = m_Data.ReadCoord();
		ax = m_Data.ReadCoord();
		ay = m_Data.ReadCoord();
		az = m_Data.ReadCoord();
		fov = m_Data.ReadByte();
		entity = m_Data.ReadWord();
		sprintf( s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i",
				name, x, y, z, ax, ay, az, (float)fov, entity );
		break;
	}

	case DRC_CMD_TIMESCALE:
	{
		float factor;

		m_Data.Reset();
		factor = m_Data.ReadFloat();
		sprintf( s, "%s %.2f", name, factor );
		break;
	}

	case DRC_CMD_MESSAGE:
	{
		int effect;
		unsigned int color;
		float x;
		float y;
		float fadein;
		float fadeout;
		float holdtime;
		float fxtime;

		m_Data.Reset();
		effect = m_Data.ReadByte();
		color = m_Data.ReadLong();
		x = m_Data.ReadFloat();
		y = m_Data.ReadFloat();
		fadein = m_Data.ReadFloat();
		fadeout = m_Data.ReadFloat();
		holdtime = m_Data.ReadFloat();
		fxtime = m_Data.ReadFloat();
		strcpy( text, m_Data.ReadString() );
		sprintf( s, "%s \"%s\" %i %x (%.2f %.2f) %.1f, %.1f %.1f %.1f", name, text, effect, color, x, y,
				fadein, fadeout, holdtime, fxtime );
		break;
	}

	case DRC_CMD_SOUND:
	{
		float volume;

		m_Data.Reset();
		strcpy( text, m_Data.ReadString() );
		volume = m_Data.ReadFloat();
		sprintf( s, "%s \"%s\" %.2f", name, text, volume );
		break;
	}

	case DRC_CMD_STATUS:
	{
		int slots;
		int spectators;
		int proxies;

		m_Data.Reset();
		slots = (int)m_Data.ReadLong();
		spectators = (int)m_Data.ReadLong();
		proxies = m_Data.ReadWord();
		sprintf( s, "%s %i %i %i", name, slots, spectators, proxies );
		break;
	}

	case DRC_CMD_BANNER:
	case DRC_CMD_STUFFTEXT:
		m_Data.Reset();
		strcpy( text, m_Data.ReadString() );
		sprintf( s, "%s \"%s\"", name, text );
		break;

	case DRC_CMD_CHASE:
	{
		int entity1;
		int entity2;
		float distance;
		int flags;

		m_Data.Reset();
		entity1 = m_Data.ReadByte();
		entity2 = m_Data.ReadByte();
		distance = m_Data.ReadFloat();
		flags = m_Data.ReadByte();
		sprintf( s, "%s %i %i %.1f %i", name, entity1, entity2, distance, flags );
		break;
	}

	case DRC_CMD_MAP:
	{
		int entity;
		float angle;
		float distance;

		m_Data.Reset();
		entity = m_Data.ReadByte();
		angle = m_Data.ReadFloat();
		distance = m_Data.ReadFloat();
		sprintf( s, "%s %i %.1f %.1f", name, entity, angle, distance );
		break;
	}

	case DRC_CMD_CAMPATH:
	{
		float x;
		float y;
		float z;
		float ax;
		float ay;
		float az;
		int fov;
		int flags;

		m_Data.Reset();
		x = m_Data.ReadCoord();
		y = m_Data.ReadCoord();
		z = m_Data.ReadCoord();
		ax = m_Data.ReadCoord();
		ay = m_Data.ReadCoord();
		az = m_Data.ReadCoord();
		fov = m_Data.ReadByte();
		flags = m_Data.ReadByte();
		sprintf( s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i",
				name, x, y, z, ax, ay, az, (float)fov, flags );
		break;
	}

	default:
		break;
	}

	return s;
}

void DirectorCmd::FromString( char* )
{
}

void DirectorCmd::Clear()
{
	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();
}

void DirectorCmd::Resize( int size )
{
	m_Data.Resize( size );
	m_Size = size;
}

#if defined( _MSC_VER )
#pragma warning( suppress : 26495 )
#endif
DirectorCmd::DirectorCmd()
{
	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();
}

DirectorCmd::~DirectorCmd()
{
	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();
}
