#ifndef COMMON_H
#define COMMON_H

#include "common/port.h"

#define PROTOCOL_VERSION		48
#define MAX_PACKET_ENTITIES	1024

#define svc_bad					0
#define svc_nop					1
#define svc_disconnect			2
#define svc_event				3
#define svc_version				4
#define svc_setview				5
#define svc_sound				6
#define svc_time				7
#define svc_print				8
#define svc_stufftext			9
#define svc_setangle			10
#define svc_serverinfo			11
#define svc_lightstyle			12
#define svc_updateuserinfo		13
#define svc_deltadescription	14
#define svc_clientdata			15
#define svc_stopsound			16
#define svc_pings				17
#define svc_particle			18
#define svc_damage				19
#define svc_spawnstatic			20
#define svc_event_reliable		21
#define svc_spawnbaseline		22
#define svc_temp_entity			23
#define svc_setpause			24
#define svc_signonnum			25
#define svc_centerprint			26
#define svc_killedmonster		27
#define svc_foundsecret			28
#define svc_spawnstaticsound	29
#define svc_intermission		30
#define svc_finale				31
#define svc_cdtrack				32
#define svc_restore				33
#define svc_cutscene			34
#define svc_weaponanim			35
#define svc_decalname			36
#define svc_roomtype			37
#define svc_addangle			38
#define svc_newusermsg			39
#define svc_packetentities		40
#define svc_deltapacketentities	41
#define svc_choke				42
#define svc_resourcelist		43
#define svc_newmovevars			44
#define svc_resourcerequest		45
#define svc_customization		46
#define svc_crosshairangle		47
#define svc_soundfade			48
#define svc_filetxferfailed		49
#define svc_hltv				50
#define svc_director			51
#define svc_voiceinit			52
#define svc_voicedata			53
#define svc_sendextrainfo		54
#define svc_timescale			55
#define svc_resourcelocation	56
#define svc_sendcvarvalue		57
#define svc_sendcvarvalue2		58

#define clc_bad					0
#define clc_nop					1
#define clc_move					2
#define clc_stringcmd			3
#define clc_delta				4
#define clc_resourcelist		5
#define clc_tmove				6
#define clc_fileconsistency	7
#define clc_voicedata			8
#define clc_hltv				9
#define clc_cvarvalue			10
#define clc_cvarvalue2		11

extern "C"
{
	extern char	com_token[1024];
	extern bool	s_com_token_unget;
}

int				COM_BuildNumber();
char*			COM_TimeString();
char*			COM_SkipPath( char* pathname );
void			COM_FileBase( char* in, char* out );
void			COM_UngetToken();
char*			COM_Parse( char* data );
char*			COM_VarArgs( char* format, ... );
//============================================================================

unsigned char	COM_Nibble( char c );
void			COM_HexConvert( const char* pszInput, int nInputLength, unsigned char* pOutput );
char*			COM_BinPrintf( unsigned char* buf, int nLen );
char*			COM_FormatTime( float seconds );
void			COM_RemoveEvilChars( char* string );
int				COM_FileNameCmp( const char* file1, const char* file2 );
bool			COM_IsWhiteSpace( char space );
void			COM_TrimSpace( const char* source, char* dest );
void			COM_UnpackRGB( int& r, int& g, int& b, unsigned int ulRGB );
unsigned int	COM_PackRGB( int r, int g, int b );
char*			COM_FileExtension( char* in );
char*			COM_LastFileExtension( char* in );
void			COM_DefaultExtension( char* path, char* extension );

#endif // COMMON_H
