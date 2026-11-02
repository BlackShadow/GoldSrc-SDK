#include "HLTV/common/DirectorCmd.h"

#include "HLTV/common/common.h"
#include "HLTV/common/mathlib.h"

#include <stdio.h>
#include <string.h>

extern "C" const char* DRC_CMD_NAME[16] =
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
	"WAYPOINTS"
};

bool DirectorCmd::GetEventData( int& entity1, int& entity2, int& flags )
{
	if ( m_Type != DRC_CMD_EVENT )
	{
		return false;
	}

	m_Data.Reset();
	entity1 = m_Data.ReadWord();
	entity2 = m_Data.ReadWord();
	flags = m_Data.ReadLong();
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

bool DirectorCmd::GetCameraData( float* position, float* angles, float& fov, int& entity )
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

bool DirectorCmd::GetCamPathData( float* position, float* angles, float& fov, int& flags )
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

bool DirectorCmd::GetMessageData( int& effect, int& color, float* position, float& fadein, float& fadeout,
		float& holdtime, float& fxtime, char* text )
{
	if ( m_Type != DRC_CMD_MESSAGE )
	{
		return false;
	}

	m_Data.Reset();
	effect = m_Data.ReadByte();
	color = m_Data.ReadLong();
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
	slots = m_Data.ReadLong();
	spectators = m_Data.ReadLong();
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
	m_Data.Resize( 8 );
	m_Size = 8;
	m_Data.WriteWord( entity1 );
	m_Data.WriteWord( entity2 );
	m_Data.WriteLong( flags );
}

void DirectorCmd::SetChaseData( int entity1, int entity2, float distance, int flags )
{
	m_Type = DRC_CMD_CHASE;
	m_Data.Resize( 9 );
	m_Size = 9;
	m_Data.WriteWord( entity1 );
	m_Data.WriteWord( entity2 );
	m_Data.WriteFloat( distance );
	m_Data.WriteByte( flags );
}

void DirectorCmd::SetInEyeData( int entity )
{
	m_Type = DRC_CMD_INEYE;
	m_Data.Resize( 2 );
	m_Size = 2;
	m_Data.WriteWord( entity );
}

void DirectorCmd::SetMapData( int entity, float angle, float distance )
{
	m_Type = DRC_CMD_MAP;
	m_Data.Resize( 10 );
	m_Size = 10;
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
	m_Data.Resize( 1 );
	m_Size = 1;
	m_Data.WriteByte( mode );
}

void DirectorCmd::SetCameraData( float* position, float* angles, float fov, int entity )
{
	m_Type = DRC_CMD_CAMERA;
	m_Data.Resize( 15 );
	m_Size = 15;
	m_Data.WriteCoord( position[0] );
	m_Data.WriteCoord( position[1] );
	m_Data.WriteCoord( position[2] );
	m_Data.WriteCoord( angles[PITCH] );
	m_Data.WriteCoord( angles[YAW] );
	m_Data.WriteCoord( angles[ROLL] );
	m_Data.WriteByte( (int)fov );
	m_Data.WriteWord( entity );
}

void DirectorCmd::SetCamPathData( float* position, float* angles, float fov, int flags )
{
	m_Type = DRC_CMD_CAMPATH;
	m_Data.Resize( 14 );
	m_Size = 14;
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
	int size = strlen( name ) + 5;

	m_Type = DRC_CMD_SOUND;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( name );
	m_Data.WriteFloat( volume );
}

void DirectorCmd::SetTimeScaleData( float factor )
{
	m_Type = DRC_CMD_TIMESCALE;
	m_Data.Resize( 4 );
	m_Size = 4;
	m_Data.WriteFloat( factor );
}

void DirectorCmd::SetTime( float time )
{
	m_Time = time;
}

void DirectorCmd::SetMessageData( int effect, unsigned int color, float* position, float fadein, float fadeout,
		float holdtime, float fxtime, char* text )
{
	int size = strlen( text ) + 30;

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
	m_Data.Resize( 10 );
	m_Size = 10;
	m_Data.WriteLong( slots );
	m_Data.WriteLong( spectators );
	m_Data.WriteWord( proxies );
}

void DirectorCmd::SetBannerData( char* filename )
{
	int size = strlen( filename ) + 1;

	m_Type = DRC_CMD_BANNER;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( filename );
}

void DirectorCmd::SetStuffTextData( char* commands )
{
	int size = strlen( commands ) + 1;

	m_Type = DRC_CMD_STUFFTEXT;
	m_Data.Resize( size );
	m_Size = size;
	m_Data.WriteString( commands );
}

void DirectorCmd::SetWayPoints( int number )
{
	m_Type = DRC_CMD_WAYPOINTS;
	m_Data.Resize( 1 );
	m_Size = 1;
	m_Data.WriteByte( number );
}

bool DirectorCmd::ReadFromStream( BitBuffer* stream )
{
	if ( stream == NULL )
	{
		return false;
	}

	m_Type = DRC_CMD_NONE;
	m_Time = 0.0f;
	m_Data.Free();

	int type = stream->ReadByte();
	m_Type = type;

	switch ( type )
	{
	case DRC_CMD_START:
		m_Data.Resize( 0 );
		m_Size = 0;
		return true;

	case DRC_CMD_EVENT:
		m_Data.Resize( 8 );
		m_Size = 8;
		m_Data.WriteBuf( stream, 8 );
		return true;

	case DRC_CMD_MODE:
	case DRC_CMD_INEYE:
		m_Data.Resize( 1 );
		m_Size = 1;
		m_Data.WriteBuf( stream, 1 );
		return true;

	case DRC_CMD_CAMERA:
		m_Data.Resize( 15 );
		m_Size = 15;
		m_Data.WriteBuf( stream, 15 );
		return true;

	case DRC_CMD_TIMESCALE:
		m_Data.Resize( 4 );
		m_Size = 4;
		m_Data.WriteBuf( stream, 4 );
		return true;

	case DRC_CMD_MESSAGE:
		{
			unsigned char* start = stream->currentByte;
			stream->SkipBytes( 29 );
			char* text = stream->ReadString();
			int textLength = strlen( text ) + 1;
			int size = textLength + 29;

			m_Data.Resize( size );
			m_Size = size;
			m_Data.WriteBuf( start, 29 );
			m_Data.WriteBuf( text, textLength );
			return true;
		}

	case DRC_CMD_SOUND:
		{
			char* name = stream->ReadString();
			int nameLength = strlen( name ) + 1;
			int size = nameLength + 4;

			m_Data.Resize( size );
			m_Size = size;
			m_Data.WriteBuf( name, nameLength );
			m_Data.WriteFloat( stream->ReadFloat() );
			return true;
		}

	case DRC_CMD_STATUS:
		m_Data.Resize( 10 );
		m_Size = 10;
		m_Data.WriteBuf( stream, 10 );
		return true;

	case DRC_CMD_BANNER:
	case DRC_CMD_STUFFTEXT:
		{
			char* text = stream->ReadString();
			int size = strlen( text ) + 1;

			m_Data.Resize( size );
			m_Size = size;
			m_Data.WriteBuf( text, size );
			return true;
		}

	case DRC_CMD_CHASE:
		m_Data.Resize( 7 );
		m_Size = 7;
		m_Data.WriteBuf( stream, 7 );
		return true;

	case DRC_CMD_MAP:
		m_Data.Resize( 10 );
		m_Size = 10;
		m_Data.WriteBuf( stream, 15 );
		return true;

	case DRC_CMD_CAMPATH:
		m_Data.Resize( 14 );
		m_Size = 14;
		m_Data.WriteBuf( stream, 14 );
		return true;

	case DRC_CMD_WAYPOINTS:
		{
			m_Data.Resize( 1 );
			m_Size = 1;

			int number = stream->ReadByte();
			m_Data.WriteByte( number );
			stream->SkipBytes( number * 14 );
			return true;
		}

	default:
		return false;
	}
}

void DirectorCmd::WriteToStream( BitBuffer* stream )
{
	if ( stream == NULL )
	{
		return;
	}

	if ( (unsigned int)( m_Type - DRC_CMD_START ) > (unsigned int)( DRC_CMD_WAYPOINTS - DRC_CMD_START ) )
	{
		return;
	}

	if ( m_Size > 254 )
	{
		return;
	}

	stream->WriteByte( svc_director );
	stream->WriteByte( m_Size + 1 );
	stream->WriteByte( m_Type );
	stream->WriteBuf( m_Data.data, m_Size );
}

char* DirectorCmd::ToString()
{
	static char s[1024];
	char text[1024];

	if ( (unsigned int)( m_Type - DRC_CMD_START ) > (unsigned int)( DRC_CMD_WAYPOINTS - DRC_CMD_START ) )
	{
		return NULL;
	}

	const char* name = DRC_CMD_NAME[m_Type];
	memset( s, 0, sizeof( s ) );

	switch ( m_Type )
	{
	case DRC_CMD_START:
		sprintf( s, "%s", name );
		return s;

	case DRC_CMD_EVENT:
		{
			m_Data.Reset();
			int entity1 = m_Data.ReadWord();
			int entity2 = m_Data.ReadWord();
			int flags = m_Data.ReadLong();
			sprintf( s, "%s %i %i %i", name, entity1, entity2, flags );
			return s;
		}

	case DRC_CMD_MODE:
	case DRC_CMD_INEYE:
	case DRC_CMD_WAYPOINTS:
		{
			m_Data.Reset();
			int value = m_Data.ReadByte();
			sprintf( s, "%s %i", name, value );
			return s;
		}

	case DRC_CMD_CAMERA:
		{
			m_Data.Reset();
			float px = m_Data.ReadCoord();
			float py = m_Data.ReadCoord();
			float pz = m_Data.ReadCoord();
			float ax = m_Data.ReadCoord();
			float ay = m_Data.ReadCoord();
			float az = m_Data.ReadCoord();
			int fov = m_Data.ReadByte();
			int entity = m_Data.ReadWord();
			sprintf( s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i", name, px, py, pz, ax, ay, az,
					(float)fov, entity );
			return s;
		}

	case DRC_CMD_TIMESCALE:
		{
			m_Data.Reset();
			float factor = m_Data.ReadFloat();
			sprintf( s, "%s %.2f", name, factor );
			return s;
		}

	case DRC_CMD_MESSAGE:
		{
			m_Data.Reset();
			int effect = m_Data.ReadByte();
			unsigned int color = m_Data.ReadLong();
			float x = m_Data.ReadFloat();
			float y = m_Data.ReadFloat();
			float fadein = m_Data.ReadFloat();
			float fadeout = m_Data.ReadFloat();
			float holdtime = m_Data.ReadFloat();
			float fxtime = m_Data.ReadFloat();
			strcpy( text, m_Data.ReadString() );
			sprintf( s, "%s \"%s\" %i %x (%.2f %.2f) %.1f, %.1f %.1f %.1f", name, text, effect, color, x, y,
					fadein, fadeout, holdtime, fxtime );
			return s;
		}

	case DRC_CMD_SOUND:
		{
			m_Data.Reset();
			strcpy( text, m_Data.ReadString() );
			float volume = m_Data.ReadFloat();
			sprintf( s, "%s \"%s\" %.2f", name, text, volume );
			return s;
		}

	case DRC_CMD_STATUS:
		{
			m_Data.Reset();
			int slots = m_Data.ReadLong();
			int spectators = m_Data.ReadLong();
			int proxies = m_Data.ReadWord();
			sprintf( s, "%s %i %i %i", name, slots, spectators, proxies );
			return s;
		}

	case DRC_CMD_BANNER:
	case DRC_CMD_STUFFTEXT:
		{
			m_Data.Reset();
			strcpy( text, m_Data.ReadString() );
			sprintf( s, "%s \"%s\"", name, text );
			return s;
		}

	case DRC_CMD_CHASE:
		{
			m_Data.Reset();
			int entity1 = m_Data.ReadByte();
			int entity2 = m_Data.ReadByte();
			float distance = m_Data.ReadFloat();
			int flags = m_Data.ReadByte();
			sprintf( s, "%s %i %i %.1f %i", name, entity1, entity2, distance, flags );
			return s;
		}

	case DRC_CMD_MAP:
		{
			m_Data.Reset();
			int entity = m_Data.ReadByte();
			float angle = m_Data.ReadFloat();
			float distance = m_Data.ReadFloat();
			sprintf( s, "%s %i %.1f %.1f", name, entity, angle, distance );
			return s;
		}

	case DRC_CMD_CAMPATH:
		{
			m_Data.Reset();
			float px = m_Data.ReadCoord();
			float py = m_Data.ReadCoord();
			float pz = m_Data.ReadCoord();
			float ax = m_Data.ReadCoord();
			float ay = m_Data.ReadCoord();
			float az = m_Data.ReadCoord();
			int fov = m_Data.ReadByte();
			int flags = m_Data.ReadByte();
			sprintf( s, "%s (%.1f %.1f %.1f) (%.1f %.1f %.1f) %.1f %i", name, px, py, pz, ax, ay, az,
					(float)fov, flags );
			return s;
		}

	default:
		return s;
	}
}

void DirectorCmd::FromString( char* string )
{
	(void)string;
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
