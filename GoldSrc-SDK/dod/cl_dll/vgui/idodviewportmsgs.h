#ifndef IDODVIEWPORTMSGS_H
#define IDODVIEWPORTMSGS_H

class IDoDViewPortMsgs
{
public:
	virtual int MsgFunc_ForceCam( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_ScoreInfo( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_ScoreInfoLong( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_TeamScore( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_MapMarker( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_WaveTime( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_WaveStatus( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_WideScreen( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_ScoreShort( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_Frags( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_ObjScore( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_PStatus( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_PTeam( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_PClass( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_CurMarker( const char* name, int size, void* buffer ) = 0;
	virtual int MsgFunc_VGUIMenu( const char* name, int size, void* buffer ) = 0;
};

extern IDoDViewPortMsgs* gDoDViewPortMsgs;

#endif
