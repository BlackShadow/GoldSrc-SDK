#ifndef NETCHANNEL_H
#define NETCHANNEL_H

#include "HLTV/common/INetChannel.h"
#include "HLTV/common/Bitbuffer.h"
#include "HLTV/common/NetAddress.h"
#include "common/ObjectList.h"

#include <stddef.h>

struct flowstats_t
{
	int		size;
	double	time;
};

class NetPacket;

class NetChannel : public INetChannel
{
public:
	enum
	{
#ifdef _WIN32
		MAX_FILENAME = 260
#else
		MAX_FILENAME = 4096
#endif
	};

	struct fragbuf_s
	{
		fragbuf_s*		next;
		int				bufferId;
		unsigned char	data[1400];
		int				size;

		bool			isfile;
		bool			isbuffer;
		char			fileName[MAX_FILENAME];
		int				fOffset;
	};

	typedef fragbuf_s fragbuf_t;

	struct fragbufwaiting_s
	{
		fragbufwaiting_s*	next;
		int					fragbufcount;
		fragbuf_t*			fragbufs;
	};

	typedef fragbufwaiting_s fragbufwaiting_t;

	struct flow_t
	{
		flowstats_t	stats[32];
		int			current;
		double		nextcompute;
		float		kbytespersec;
		float		avgkbytespersec;
	};

	NetChannel();
	virtual ~NetChannel() override;

	virtual bool Create( IBaseSystem* system, INetSocket* netsocket, NetAddress* adr ) override;
	virtual NetAddress* GetTargetAddress() override;
	virtual void Close() override;
	virtual void Clear() override;
	virtual void Reset() override;

	virtual bool IsConnected() override;
	virtual bool IsReadyToSend() override;
	virtual bool IsCrashed() override;
	virtual bool IsTimedOut() override;
	virtual bool IsFakeChannel() override;
	virtual bool KeepAlive() override;

	virtual void SetRate( int newRate ) override;
	virtual void SetUpdateRate( int newupdaterate ) override;
	virtual void SetTimeOut( float time ) override;
	virtual void SetKeepAlive( bool flag ) override;
	virtual float GetIdleTime() override;
	virtual int GetRate() override;
	virtual int GetUpdateRate() override;
	virtual float GetLoss() override;

	virtual void TransmitOutgoing() override;
	virtual void ProcessIncoming( unsigned char* data, int size ) override;
	virtual void OutOfBandPrintf( const char* format, ... ) override;
	virtual void FakeAcknowledgement() override;

	void SetConnected( bool flag );
	void ClearFragments();
	void FreePacket( NetPacket* packet );
	NetPacket* GetPacket();
	void FlushIncoming( int stream );
	void ClearFragbufs( fragbuf_t** ppbuf );
	void UnlinkFragment( fragbuf_t* buf, int stream );
	fragbuf_t* FindBufferById( fragbuf_t** pplist, int id, bool allocate );
	void AddBufferToList( fragbuf_t** pplist, fragbuf_t* pbuf );
	void AddFragbufToTail( fragbufwaiting_t* wait, fragbuf_t* buf );
	void FragSend();
	bool CreateFragmentsFromBuffer( void* buffer, int size, int streamtype, char* filename );
	bool CreateFragmentsFromFile( char* fileName );

	void UpdateFlow( int stream );
	void GetFlowStats( float* avgInKBSec, float* avgOutKBSec );

	void CopyNormalFragments();
	bool CopyFileFragments();
	bool CheckForCompletion( int stream, int intotalbuffers );

	IBaseSystem*		m_System;
	INetSocket*			sock;
	NetAddress			remote_address;

	double				last_received;
	double				last_send;
	double				connect_time;
	float				timeOut;
	int					max_bandwidth_rate;
	double				send_interval;
	int					updaterate;
	double				cleartime;

	bool				keepAlive;
	bool				crashed;
	bool				isConnected;

	int					incoming_sequence;
	int					incoming_acknowledged;
	int					incoming_reliable_acknowledged;
	int					incoming_reliable_sequence;
	int					outgoing_sequence;
	int					reliable_sequence;
	int					last_reliable_sequence;
	void*				connection_status;
	int					blocksize;

	BitBuffer			reliableStream;
	BitBuffer			unreliableStream;
	ObjectList			incomingPackets;
	int					reliableOutSize;
	unsigned char		reliableOutBuffer[4010];

	fragbufwaiting_t*	waitlist[2];
	int					reliable_fragment[2];
	unsigned int		reliable_fragid[2];
	fragbuf_t*			fragbufs[2];
	int					fragbufcount[2];
	short				frag_startpos[2];
	short				frag_length[2];
	fragbuf_t*			incomingbufs[2];
	char				incomingfilename[MAX_FILENAME];
	void*				tempbuffer;
	int					tempbuffersize;

	flow_t				flow[2];
	float				loss;
};

#endif
