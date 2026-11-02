// protocol.h -- communications protocols
#pragma once

#define PROTOCOL_VERSION 48

//
// server to client
//
#define svc_bad 0
#define svc_nop 1
#define svc_disconnect 2
#define svc_event 3 // 
#define svc_version 4
// #define	svc_version			4	// [long] server version
#define svc_setview 5 // [short] entity number
#define svc_sound 6 // <see code>
#define svc_time 7 // [float] server time
#define svc_print 8 // [string] null terminated string
#define svc_stufftext 9 // [string] stuffed into client's console buffer
// the string should be \n terminated
#define svc_setangle 10 // [angle3] set the view angle to this absolute value
#define svc_serverinfo 11 // [long] version
#define svc_lightstyle 12 // [byte] [string]
#define svc_updateuserinfo 13 // [byte] slot [long] uid [string] userinfo
#define svc_deltadescription 14
#define svc_clientdata 15 // <shortbits + data>
#define svc_stopsound 16 // <see code>
#define svc_pings 17
#define svc_particle 18 // custom
#define svc_damage 19
#define svc_spawnstatic 20
#define svc_event_reliable 21
#define svc_spawnbaseline 22
#define svc_temp_entity 23 // variable
#define svc_setpause 24
#define svc_signonnum 25
#define svc_centerprint 26
#define svc_killedmonster 27
#define svc_foundsecret 28
#define svc_spawnstaticsound 29
#define svc_intermission 30
#define svc_finale 31
#define svc_cdtrack 32
#define svc_restore 33
#define svc_cutscene 34
#define svc_weaponanim 35
#define svc_decalname 36
#define svc_roomtype 37
#define svc_addangle 38
#define svc_newusermsg 39 // Server wants client to upload a logo file
#define svc_packetentities 40
#define svc_deltapacketentities 41
#define svc_choke 42
#define svc_resourcelist 43
#define svc_newmovevars 44
#define svc_resourcerequest 45
#define svc_customization 46 // A debug text overlay to be displayed
#define svc_crosshairangle 47
#define svc_soundfade 48
#define svc_filetxferfailed 49
#define svc_hltv 50 // A grid overlay to be displayed
#define svc_director 51
#define svc_voiceinit 52
#define svc_voicedata 53
#define svc_sendextrainfo 54
#define svc_timescale 55
#define svc_resourcelocation 56
#define svc_sendcvarvalue 57
#define svc_sendcvarvalue2 58
#define svc_lastmsg 59

//
// client to server
//
#define clc_bad 0
#define clc_nop 1
#define clc_move 2
#define clc_stringcmd 3
#define clc_delta 4
#define clc_resourcelist 5
#define clc_tmove 6
#define clc_fileconsistency 7
#define clc_voicedata 8
#define clc_hltv 9
#define clc_cvarvalue 10
#define clc_cvarvalue2 11
#define clc_endoflist 255

#define SND_SENTENCE ( 1 << 4 )
#define SND_STOP ( 1 << 5 ) // Disconnect if we can't get this file.
#define SND_CHANGE_VOL ( 1 << 6 )
#define SND_CHANGE_PITCH ( 1 << 7 )

#define MSG_FL_PVS 2
#define MSG_FL_PAS 4
