#include "HLTV/common/NetChannel.h"
#include "HLTV/common/NetPacket.h"
#include "HLTV/common/INetSocket.h"
#include "HLTV/common/mem.h"
#include "HLTV/common/munge.h"
#include "common/IBaseSystem.h"

#include "utils/bzip2/bzlib.h"

#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NETCHAN_RELIABLE_FLAG				( 1u << 31 )
#define NETCHAN_FRAGMENT_FLAG				( 1u << 30 )
#define NETCHAN_SEQUENCE_MASK				( NETCHAN_FRAGMENT_FLAG - 1u )
#define NETCHAN_ACKNOWLEDGEMENT_MASK		( NETCHAN_RELIABLE_FLAG - 1u )
#define NETCHAN_CONNECTIONLESS_SEQUENCE		( ~0u )

static void ZeroFragmentState( NetChannel* channel )
{
	channel->reliable_fragment[0] = 0;
	channel->reliable_fragment[1] = 0;
	channel->reliable_fragid[0] = 0;
	channel->reliable_fragid[1] = 0;
	channel->frag_length[0] = 0;
	channel->frag_length[1] = 0;
	channel->frag_startpos[0] = 0;
	channel->frag_startpos[1] = 0;
}

NetChannel::NetChannel()
{
	waitlist[0] = NULL;
	waitlist[1] = NULL;
	fragbufs[0] = NULL;
	fragbufs[1] = NULL;
	incomingbufs[0] = NULL;
	incomingbufs[1] = NULL;
	frag_startpos[0] = 0;
	frag_startpos[1] = 0;
	frag_length[0] = 0;
	frag_length[1] = 0;
	fragbufcount[0] = 0;
	fragbufcount[1] = 0;
	sock = NULL;
	tempbuffer = NULL;
}

NetChannel::~NetChannel()
{
}

bool NetChannel::Create( IBaseSystem* system, INetSocket* netsocket, NetAddress* adr )
{
	m_System = system;
	incomingPackets.Init();
	blocksize = 1024;

	if ( !reliableStream.Resize( 3990 ) )
	{
		m_System->Errorf( "NetChannel::Create: reliableStream out of memory.\n" );
		return false;
	}

	if ( !unreliableStream.Resize( 3990 ) )
	{
		m_System->Errorf( "NetChannel::Create: unreliableStream out of memory.\n" );
		return false;
	}

	remote_address.FromNetAddress( adr );
	sock = netsocket;

	if ( netsocket != NULL )
	{
		if ( !sock->AddChannel( this ) )
		{
			m_System->Errorf( "NetChannel::Create: could not bound to NetSocket\n" );
			return false;
		}
	}
	else
	{
		m_System->DPrintf( "Creating fake network channel.\n" );
	}

	Clear();
	Reset();
	return true;
}

NetAddress* NetChannel::GetTargetAddress()
{
	return &remote_address;
}

void NetChannel::Close()
{
	if ( sock != NULL )
		sock->RemoveChannel( this );

	Clear();
	reliableStream.Free();
	unreliableStream.Free();
}

void NetChannel::Clear()
{
	NetPacket* packet;

	while ( ( packet = (NetPacket*)incomingPackets.RemoveHead() ) != NULL )
		FreePacket( packet );

	ClearFragments();

	reliable_fragment[0] = 0;
	reliable_fragment[1] = 0;
	reliable_fragid[0] = 0;
	reliable_fragid[1] = 0;
	fragbufcount[0] = 0;
	fragbufcount[1] = 0;
	frag_startpos[0] = 0;
	frag_startpos[1] = 0;
	frag_length[0] = 0;
	frag_length[1] = 0;

	if ( tempbuffer != NULL )
	{
		free( tempbuffer );
		tempbuffer = NULL;
	}

	tempbuffersize = 0;
	reliableOutSize = 0;
	memset( reliableOutBuffer, 0, sizeof( reliableOutBuffer ) );
	memset( flow, 0, sizeof( flow ) );
	reliableStream.Clear();
	unreliableStream.Clear();

	double now = m_System->GetTime();
	loss = 0.0f;
	last_send = now;
	last_received = now;
	cleartime = now;
}

void NetChannel::Reset()
{
	isConnected = false;
	keepAlive = true;
	crashed = false;
	connect_time = m_System->GetTime();
	SetTimeOut( 30.0f );
	SetRate( 10000 );
	SetUpdateRate( 20 );
	incoming_sequence = 0;
	incoming_acknowledged = 0;
	incoming_reliable_acknowledged = 0;
	incoming_reliable_sequence = 0;
	outgoing_sequence = 1;
	reliable_sequence = 0;
	last_reliable_sequence = 0;
}

bool NetChannel::IsConnected()
{
	return isConnected;
}

bool NetChannel::IsReadyToSend()
{
	return m_System->GetTime() > cleartime;
}

bool NetChannel::IsCrashed()
{
	return crashed;
}

bool NetChannel::IsTimedOut()
{
	if ( m_System->GetTime() > timeOut + last_received )
		return sock != NULL;

	return false;
}

bool NetChannel::IsFakeChannel()
{
	return sock == NULL;
}

bool NetChannel::KeepAlive()
{
	return keepAlive;
}

void NetChannel::SetRate( int newRate )
{
	int rate;

	rate = 1000;
	if ( newRate > 999 )
	{
		rate = 20000;
		if ( newRate <= 20000 )
			rate = newRate;
	}

	max_bandwidth_rate = rate;
}

void NetChannel::SetUpdateRate( int newupdaterate )
{
	if ( newupdaterate > 100 )
	{
		updaterate = 100;
		send_interval = 0.01f;
	}
	else if ( newupdaterate < 0 )
	{
		updaterate = 1;
		send_interval = 1.0;
	}
	else
	{
		updaterate = newupdaterate;
		send_interval = 1.0f / newupdaterate;
	}
}

void NetChannel::SetTimeOut( float time )
{
	if ( time > 0.0f )
		timeOut = time;
}

void NetChannel::SetKeepAlive( bool flag )
{
	keepAlive = flag;
}

float NetChannel::GetIdleTime()
{
	return (float)( m_System->GetTime() - last_send );
}

int NetChannel::GetRate()
{
	return max_bandwidth_rate;
}

int NetChannel::GetUpdateRate()
{
	return updaterate;
}

float NetChannel::GetLoss()
{
	return loss;
}

void NetChannel::FakeAcknowledgement()
{
	incoming_sequence = 0;
	incoming_acknowledged = outgoing_sequence - 1;
	incoming_reliable_acknowledged = reliable_sequence;
	reliableOutSize = 0;
	last_received = m_System->GetTime();
}

void NetChannel::SetConnected( bool flag )
{
	isConnected = flag;
}

NetPacket* NetChannel::GetPacket()
{
	return (NetPacket*)incomingPackets.RemoveTail();
}

void NetChannel::FreePacket( NetPacket* packet )
{
	if ( packet != NULL )
	{
		packet->data.Free();
		delete packet;
	}
}

void NetChannel::FlushIncoming( int stream )
{
	NetPacket* packet;
	fragbuf_t* buf;
	fragbuf_t* next;

	while ( ( packet = (NetPacket*)incomingPackets.RemoveTail() ) != NULL )
		FreePacket( packet );

	buf = incomingbufs[stream];
	while ( buf != NULL )
	{
		next = buf->next;
		free( buf );
		buf = next;
	}

	incomingbufs[stream] = NULL;
}

void NetChannel::ClearFragbufs( fragbuf_t** ppbuf )
{
	fragbuf_t* buf;
	fragbuf_t* next;

	if ( ppbuf == NULL )
		return;

	buf = *ppbuf;
	while ( buf != NULL )
	{
		next = buf->next;
		free( buf );
		buf = next;
	}

	*ppbuf = NULL;
}

void NetChannel::ClearFragments()
{
	int stream;

	for ( stream = 0; stream < 2; stream++ )
	{
		fragbufwaiting_t* wait;

		wait = waitlist[stream];
		while ( wait != NULL )
		{
			fragbufwaiting_t* nextWait;

			nextWait = wait->next;
			ClearFragbufs( &wait->fragbufs );
			free( wait );
			wait = nextWait;
		}

		waitlist[stream] = NULL;
		ClearFragbufs( &fragbufs[stream] );
		FlushIncoming( stream );
	}
}

void NetChannel::UnlinkFragment( fragbuf_t* buf, int stream )
{
	fragbuf_t** list;
	fragbuf_t* search;

	list = &fragbufs[stream];
	if ( list == NULL )
	{
		m_System->DPrintf( "Netchan_UnlinkFragment:  Asked to unlink fragment from empty list, ignored\n" );
		return;
	}

	if ( *list == buf )
	{
		*list = buf->next;
		free( buf );
		return;
	}

	search = *list;
	while ( search->next != NULL )
	{
		if ( search->next == buf )
		{
			search->next = buf->next;
			free( buf );
			return;
		}

		search = search->next;
	}

	m_System->DPrintf( "Netchan_UnlinkFragment:  Couldn't find fragment\n" );
}

void NetChannel::AddFragbufToTail( fragbufwaiting_t* wait, fragbuf_t* buf )
{
	fragbuf_t* tail;

	buf->next = NULL;
	wait->fragbufcount++;
	tail = wait->fragbufs;
	if ( tail != NULL )
	{
		while ( tail->next != NULL )
			tail = tail->next;

		tail->next = buf;
	}
	else
	{
		wait->fragbufs = buf;
	}
}

void NetChannel::AddBufferToList( fragbuf_t** pplist, fragbuf_t* pbuf )
{
	fragbuf_t* prev;

	pbuf->next = NULL;
	if ( pplist == NULL )
		return;

	prev = *pplist;
	if ( prev == NULL )
	{
		*pplist = pbuf;
		return;
	}

	for ( ;; )
	{
		fragbuf_t* next;

		next = prev->next;
		if ( next == NULL )
		{
			prev->next = pbuf;
			return;
		}

		if ( ( (unsigned int)pbuf->bufferId >> 16 ) < ( (unsigned int)next->bufferId >> 16 ) )
		{
			pbuf->next = next->next;
			prev->next = pbuf;
			return;
		}

		prev = next;
	}
}

NetChannel::fragbuf_t* NetChannel::FindBufferById( fragbuf_t** pplist, int id, bool allocate )
{
	fragbuf_t* list;
	fragbuf_t* created;
	fragbuf_t* prev;
	unsigned int want;

	list = NULL;
	if ( pplist != NULL )
		list = *pplist;

	if ( list != NULL )
	{
		if ( list->bufferId == id )
			return list;

		while ( list->next != NULL )
		{
			list = list->next;
			if ( list->bufferId == id )
				return list;
		}
	}

	if ( !allocate )
		return NULL;

	created = (fragbuf_t*)Mem_ZeroMalloc( sizeof( fragbuf_t ) );
	created->bufferId = id;
	created->next = NULL;
	if ( pplist == NULL )
		return created;

	prev = *pplist;
	if ( prev == NULL )
	{
		*pplist = created;
		return created;
	}

	want = (unsigned int)id >> 16;
	for ( ;; )
	{
		fragbuf_t* next;

		next = prev->next;
		if ( next == NULL )
		{
			prev->next = created;
			return created;
		}

		if ( ( (unsigned int)next->bufferId >> 16 ) > want )
		{
			created->next = next->next;
			prev->next = created;
			return created;
		}

		prev = next;
	}
}

void NetChannel::FragSend()
{
	if ( fragbufs[0] != NULL || waitlist[0] == NULL )
	{
		if ( fragbufs[1] != NULL )
			return;
	}
	else
	{
		fragbufwaiting_t* wait;

		wait = waitlist[0];
		waitlist[0] = wait->next;
		fragbufs[0] = wait->fragbufs;
		fragbufcount[0] = wait->fragbufcount;
		free( wait );

		if ( fragbufs[1] != NULL )
			return;
	}

	if ( waitlist[1] != NULL )
	{
		fragbufwaiting_t* wait;

		wait = waitlist[1];
		waitlist[1] = wait->next;
		fragbufs[1] = wait->fragbufs;
		fragbufcount[1] = wait->fragbufcount;
		free( wait );
	}
}

void NetChannel::UpdateFlow( int stream )
{
	flow_t* slot;
	int current;
	int bytes;
	int age;
	float accumulated;
	float kbytes;

	slot = &flow[stream];
	if ( slot->nextcompute > m_System->GetTime() )
		return;

	slot->nextcompute = m_System->GetTime() + 0.5;
	current = slot->current;
	bytes = 0;
	for ( age = 15; age >= 1; age-- )
		bytes += slot->stats[( current - age ) & 31].size;

	accumulated = (float)( m_System->GetTime() - slot->stats[( current - 15 ) & 31].time );
	if ( accumulated <= 0.0f )
	{
		slot->kbytespersec = 0.0f;
		kbytes = 0.0f;
	}
	else
	{
		kbytes = (float)( ( (float)bytes / accumulated ) * 0.0009765625 );
		slot->kbytespersec = kbytes;
	}

	slot->avgkbytespersec = kbytes * 0.3333333f + 0.6666667f * slot->avgkbytespersec;
}

void NetChannel::GetFlowStats( float* avgInKBSec, float* avgOutKBSec )
{
	UpdateFlow( 1 );
	UpdateFlow( 0 );
	*avgInKBSec = flow[1].avgkbytespersec;
	*avgOutKBSec = flow[0].avgkbytespersec;
}

bool NetChannel::CreateFragmentsFromFile( char* fileName )
{
	if ( IsFakeChannel() )
	{
		m_System->Printf( "NetChannel::CreateFragmentsFromBuffer: IsFakeChannel()\n" );
		return true;
	}

	m_System->DPrintf( "WARNING! Ignoring file request %s.\n", fileName );
	return false;
}

bool NetChannel::CreateFragmentsFromBuffer( void* buffer, int size, int streamtype, char* filename )
{
	int header;
	fragbufwaiting_t* wait;
	fragbufwaiting_t** tail;
	int bufferId;
	int pos;
	int left;
	bool firstFragment;

	if ( IsFakeChannel() )
	{
		m_System->Printf( "NetChannel::CreateFragmentsFromBuffer: IsFakeChannel()\n" );
		return true;
	}

	if ( size == 0 )
		return true;

	{
		char compressed[65536];
		unsigned int compressedSize;

		header = 4;
		compressedSize = (unsigned int)( size - 4 );
		if ( streamtype == 1 )
			compressedSize = (unsigned int)( size - 8 );

		if ( BZ2_bzBuffToBuffCompress( compressed, &compressedSize, (char*)buffer,
				(unsigned int)size, 9, 0, 30 ) == BZ_OK )
		{
			unsigned char* out;

			m_System->DPrintf( "Compressing split packet (%d -> %d bytes)\n", size, compressedSize );
			out = (unsigned char*)buffer;
			out[0] = 'B';
			out[1] = 'Z';
			out[2] = '2';
			out[3] = 0;
			if ( streamtype == 1 )
			{
				*(int*)( out + 4 ) = size;
				header = 8;
			}

			memcpy( out + header, compressed, compressedSize );
			size = (int)compressedSize + header;
		}
	}

	wait = (fragbufwaiting_t*)Mem_ZeroMalloc( sizeof( fragbufwaiting_t ) );
	bufferId = 1;
	pos = 0;
	left = size;
	firstFragment = true;

	while ( left > 0 )
	{
		int chunk;
		fragbuf_t* frag;
		unsigned int advance;

		chunk = 256;
		if ( left <= 256 )
			chunk = left;

		frag = (fragbuf_t*)Mem_ZeroMalloc( sizeof( fragbuf_t ) );
		if ( frag == NULL )
		{
			m_System->Printf( "NetChannel::CreateFragmentsFromBuffer:Couldn't allocate fragbuf_t\n" );
			free( wait );
			return false;
		}

		advance = (unsigned int)chunk;
		if ( firstFragment && filename != NULL )
		{
			unsigned int nameBytes;

			nameBytes = (unsigned int)strlen( filename ) + 1;
			memcpy( frag->data, filename, nameBytes );
			advance = (unsigned int)chunk - nameBytes;
			memcpy( frag->data + nameBytes, (unsigned char*)buffer + pos, advance );
			firstFragment = false;
		}
		else
		{
			memcpy( frag->data, (unsigned char*)buffer + pos, (size_t)chunk );
		}

		frag->size = chunk;
		left -= (int)advance;
		pos += (int)advance;
		frag->bufferId = bufferId;
		frag->isfile = filename != NULL;
		frag->isbuffer = true;
		AddFragbufToTail( wait, frag );
		bufferId++;
	}

	tail = &waitlist[streamtype];
	while ( *tail != NULL )
		tail = &(*tail)->next;

	*tail = wait;
	return true;
}

void NetChannel::CopyNormalFragments()
{
	fragbuf_t* buf;
	unsigned int total;
	NetPacket* packet;
	unsigned char* payload;

	total = 0;
	buf = incomingbufs[0];
	if ( buf == NULL )
	{
		m_System->DPrintf( "WARNING! NetChannel::CopyNormalFragments: called with no fragments readied.\n" );
		return;
	}

	while ( buf != NULL )
	{
		total += (unsigned int)buf->size;
		buf = buf->next;
	}

	packet = new NetPacket;
	packet->connectionless = false;
	packet->seqnr = (unsigned int)incoming_sequence;
	packet->time = m_System->GetTime();
	packet->address.FromNetAddress( &remote_address );
	packet->data.Resize( total );

	buf = incomingbufs[0];
	while ( buf != NULL )
	{
		fragbuf_t* next;

		next = buf->next;
		packet->data.WriteBuf( buf->data, buf->size );
		free( buf );
		buf = next;
	}

	payload = packet->data.data;
	if ( payload[0] == 'B' && payload[1] == 'Z' && payload[2] == '2' && payload[3] == 0 )
	{
		char uncompressed[65536];
		unsigned int uncompressedSize;

		uncompressedSize = 65536;
		BZ2_bzBuffToBuffDecompress( uncompressed, &uncompressedSize, (char*)payload + 4, total - 4, 1, 0 );
		packet->data.Resize( uncompressedSize );
		packet->data.WriteBuf( uncompressed, (int)uncompressedSize );
	}

	packet->data.Reset();
	incomingPackets.AddHead( packet );
	incomingbufs[0] = NULL;
}

bool NetChannel::CopyFileFragments()
{
	fragbuf_t* buf;
	unsigned int total;
	char filename[4096];

	buf = incomingbufs[1];
	if ( buf == NULL )
	{
		m_System->DPrintf( "WARNING! NetChannel::CopyFileFragments: called with no fragments readied.\n" );
		return false;
	}

	total = 0;
	while ( buf != NULL )
	{
		total += (unsigned int)buf->size;
		buf = buf->next;
	}

	BitBuffer filecontent( total );
	buf = incomingbufs[1];
	while ( buf != NULL )
	{
		fragbuf_t* next;

		next = buf->next;
		filecontent.WriteBuf( buf->data, buf->size );
		free( buf );
		buf = next;
	}

	filecontent.Reset();
	strncpy( filename, filecontent.ReadString(), 4095 );
	filename[4095] = 0;

	if ( filename[0] == 0 )
	{
		m_System->Printf( "File fragment received with no filename\n" );
		FlushIncoming( 1 );
	}
	else if ( strstr( filename, ".." ) != NULL )
	{
		m_System->Printf( "File fragment received with relative path, ignoring\n" );
		FlushIncoming( 1 );
	}

	incomingbufs[1] = NULL;
	return false;
}

bool NetChannel::CheckForCompletion( int stream, int intotalbuffers )
{
	fragbuf_t* buf;
	int count;

	if ( stream > 1 )
	{
		m_System->DPrintf( "ERROR! NetChannel::CheckForCompletion: invalid stream number %i.\n", stream );
		return false;
	}

	buf = incomingbufs[stream];
	if ( buf == NULL )
		return false;

	count = 0;
	while ( buf != NULL )
	{
		count++;
		if ( count != (int)( ( (unsigned int)buf->bufferId >> 16 ) & 0xFFFF ) )
		{
			m_System->DPrintf( "WARNING! NetChannel::CheckForCompletion: lost/dropped fragment "
					"Lost/dropped fragment would cause stall, retrying connection\n" );
			crashed = true;
			return false;
		}

		buf = buf->next;
	}

	if ( count != intotalbuffers )
		return false;

	if ( stream == 0 )
		CopyNormalFragments();
	else if ( stream == 1 )
		m_System->Printf( "TODO! NetChannel::CheckForCompletion: create file from fragments.\n" );

	return true;
}

void NetChannel::OutOfBandPrintf( const char* format, ... )
{
	char string[4037];
	BitBuffer data( string, 4037 );
	va_list argptr;

	if ( sock == NULL )
		return;

	*(int*)string = -1;
	va_start( argptr, format );
#ifdef _WIN32
	_vsnprintf( string + 4, 4033, format, argptr );
#else
	vsnprintf( string + 4, 4033, format, argptr );
#endif
	va_end( argptr );
	data.SkipBytes( (int)strlen( string ) );
	sock->SendPacket( &remote_address, data.data, data.CurrentSize() );
}

void NetChannel::TransmitOutgoing()
{
	unsigned char sendBuf[4037];
	BitBuffer data( sendBuf, 4037 );
	bool sendResending;
	bool sendReliable;
	bool runFragments;
	int sendFromFrag[2];

	if ( IsFakeChannel() )
	{
		double now;

		outgoing_sequence++;
		now = m_System->GetTime();
		last_send = now;
		cleartime = now + send_interval;
		reliableStream.FastClear();
		unreliableStream.FastClear();
		FakeAcknowledgement();
		return;
	}

	if ( reliableStream.sizeError )
	{
		m_System->DPrintf( "NetChannel::Transmit:Outgoing reliableStream overflow (%s)\n", remote_address.ToString() );
		reliableStream.Clear();
		return;
	}

	if ( unreliableStream.sizeError )
	{
		m_System->DPrintf( "NetChannel::Transmit:Outgoing unreliableStream overflow (%s)\n",
				remote_address.ToString() );
		unreliableStream.Clear();
	}

	if ( incoming_acknowledged > last_reliable_sequence )
		sendResending = incoming_reliable_acknowledged != reliable_sequence;
	else
		sendResending = false;

	runFragments = false;
	sendFromFrag[0] = 0;
	sendFromFrag[1] = 0;

	if ( reliableOutSize == 0 )
	{
		int relSize;
		bool has0;
		bool has1;

		FragSend();
		relSize = reliableStream.CurrentSize();
		has0 = fragbufs[0] != NULL;
		has1 = fragbufs[1] != NULL;
		if ( has0 )
			sendFromFrag[0] = 1;
		if ( has1 )
			sendFromFrag[1] = 1;

		if ( relSize != 0 && has0 && reliableStream.CurrentSize() > 1200 )
		{
			CreateFragmentsFromBuffer( reliableStream.data, reliableStream.CurrentSize(), 0, NULL );
			reliableStream.FastClear();
		}

		ZeroFragmentState( this );

		if ( relSize != 0 && !has0 )
		{
			unsigned int bytes;

			reliable_sequence ^= 1u;
			bytes = (unsigned int)reliableStream.CurrentSize();
			memcpy( reliableOutBuffer, reliableStream.data, bytes );
			reliableOutSize = (int)bytes;
			reliableStream.FastClear();
			frag_startpos[0] = (short)reliableOutSize;
			frag_startpos[1] = (short)reliableOutSize;
			sendReliable = true;
		}
		else if ( relSize == 0 && !has0 && !has1 )
		{
			sendReliable = sendResending;
		}
		else
		{
			reliable_sequence ^= 1u;
			sendReliable = true;
		}

		runFragments = true;
	}
	else
	{
		sendReliable = sendResending;
	}

	if ( runFragments )
	{
		int stream;

		for ( stream = 0; stream < 2; stream++ )
		{
			fragbuf_t* frag;

			frag = fragbufs[stream];
			if ( frag != NULL && sendFromFrag[stream] != 0 && reliableOutSize + frag->size <= 1199 )
			{
				int fragSize;

				fragSize = frag->size;
				reliable_fragid[stream] = ( (unsigned int)frag->bufferId << 16 )
						| (unsigned short)fragbufcount[stream];
				if ( frag->isfile && !frag->isbuffer )
					m_System->Printf( "TODO! NetChannel::Transmit: system file support\n" );

				memcpy( reliableOutBuffer + reliableOutSize, frag->data, (size_t)fragSize );
				reliableOutSize += fragSize;
				frag_length[stream] = (short)fragSize;
				UnlinkFragment( frag, stream );
				reliable_fragment[stream] = 1;
				if ( stream == 0 )
					frag_startpos[1] = (short)( frag_startpos[1] + frag_length[0] );
			}
		}
	}

	unsigned int outSeq;
	unsigned int inSeq;
	bool hasFragments;
	int slot;
	int packetSize;
	double bandwidth;
	double now;

	outSeq = (unsigned int)outgoing_sequence | ( (unsigned int)sendReliable << 31 );
	inSeq = (unsigned int)incoming_sequence | ( (unsigned int)incoming_reliable_sequence << 31 );
	hasFragments = ( reliable_fragment[0] != 0 || reliable_fragment[1] != 0 ) && sendReliable;
	if ( hasFragments )
		outSeq |= NETCHAN_FRAGMENT_FLAG;

	outgoing_sequence++;
	data.Clear();
	data.WriteLong( outSeq );
	data.WriteLong( inSeq );

	if ( hasFragments )
	{
		int stream;

		for ( stream = 0; stream < 2; stream++ )
		{
			if ( reliable_fragment[stream] != 0 )
			{
				data.WriteByte( 1 );
				data.WriteLong( reliable_fragid[stream] );
				data.WriteShort( frag_startpos[stream] );
				data.WriteShort( frag_length[stream] );
			}
			else
			{
				data.WriteByte( 0 );
			}
		}
	}

	if ( hasFragments || sendReliable )
	{
		data.WriteBuf( reliableOutBuffer, reliableOutSize );
		last_reliable_sequence = outgoing_sequence - 1;
	}

	int limit;
	int space;

	limit = sendResending ? 1400 : 4037;
	space = limit - data.CurrentSize();
	if ( space < unreliableStream.CurrentSize() )
		m_System->DPrintf( "WARNING! NetChannel::TransmitOutgoing:  Unreliable would overfow, ignoring.\n" );
	else
		data.ConcatBuffer( &unreliableStream );

	unreliableStream.FastClear();

	while ( data.CurrentSize() < 16 )
		data.WriteByte( 1 );

	slot = flow[0].current & 31;
	flow[0].stats[slot].size = data.CurrentSize() + 28;
	flow[0].stats[slot].time = m_System->GetTime();
	flow[0].current++;

	packetSize = data.CurrentSize();
	COM_Munge2( data.data + 8, packetSize - 8, (unsigned char)( outgoing_sequence - 1 ) );
	if ( sock != NULL )
		sock->SendPacket( &remote_address, data.data, data.CurrentSize() );

	last_send = m_System->GetTime();
	now = m_System->GetTime();
	bandwidth = (double)( data.CurrentSize() + 28 ) / (double)max_bandwidth_rate;
	if ( bandwidth <= send_interval )
		bandwidth = send_interval;

	cleartime = bandwidth + now;
}

void NetChannel::ProcessIncoming( unsigned char* data, int size )
{
	BitBuffer message( data, (unsigned int)size );
	unsigned int raw;
	unsigned int ack;
	unsigned int seq;
	bool fragMessage[2];
	unsigned int fragId[2];
	int fragOffset[2];
	int fragLength[2];
	int stream;
	int remaining;
	NetPacket* packet;

	fragMessage[0] = false;
	fragMessage[1] = false;
	fragId[0] = 0;
	fragId[1] = 0;
	fragOffset[0] = 0;
	fragOffset[1] = 0;
	fragLength[0] = 0;
	fragLength[1] = 0;

	raw = message.ReadLong();
	if ( raw == NETCHAN_CONNECTIONLESS_SEQUENCE )
	{
		packet = new NetPacket;
		packet->connectionless = true;
		packet->time = m_System->GetTime();
		packet->seqnr = NETCHAN_CONNECTIONLESS_SEQUENCE;
		packet->address.FromNetAddress( &remote_address );
		packet->data.Resize( (unsigned int)( size - 4 ) );
		packet->data.WriteBuf( data + 4, size - 4 );
		packet->data.Reset();
		incomingPackets.AddHead( packet );
		return;
	}

	if ( !isConnected )
		return;

	ack = message.ReadLong();
	COM_UnMunge2( message.data + 8, size - 8, (unsigned char)raw );
	if ( ( ack & NETCHAN_FRAGMENT_FLAG ) != 0 )
	{
		crashed = true;
		return;
	}

	if ( ( raw & NETCHAN_FRAGMENT_FLAG ) != 0 )
	{
		for ( stream = 0; stream < 2; stream++ )
		{
			if ( message.ReadByte() != 0 )
			{
				fragMessage[stream] = true;
				fragId[stream] = message.ReadLong();
				fragOffset[stream] = message.ReadShort();
				fragLength[stream] = message.ReadShort();
			}
		}
	}

	seq = raw & NETCHAN_SEQUENCE_MASK;
	if ( seq <= (unsigned int)incoming_sequence )
	{
		if ( seq == (unsigned int)incoming_sequence )
			m_System->DPrintf( "NetChannel::ProcessIncoming: duplicate packet %i at %i from %s\n",
					(int)seq, incoming_sequence, remote_address.ToString() );
		else
			m_System->DPrintf( "NetChannel::ProcessIncoming: out of order packet %i at %i from %s\n",
					(int)seq, incoming_sequence, remote_address.ToString() );

		return;
	}

	int dropped;
	float samples;
	float ratio;
	float sample;

	dropped = (int)seq - incoming_sequence - 1;
	if ( dropped < 0 )
		dropped = 0;

	samples = (float)( dropped + 1 );
	ratio = (float)dropped / samples;
	sample = samples * 0.005f;
	if ( sample < 1.0f )
		loss = ( 1.0f - sample ) * loss + sample * ratio;
	else
		loss = 1.0f;

	if ( (int)( ack >> 31 ) == reliable_sequence && incoming_acknowledged + 1 >= last_reliable_sequence )
		reliableOutSize = 0;

	incoming_acknowledged = (int)( ack & NETCHAN_ACKNOWLEDGEMENT_MASK );
	incoming_sequence = (int)seq;
	incoming_reliable_acknowledged = (int)( ack >> 31 );
	if ( ( raw & NETCHAN_RELIABLE_FLAG ) != 0 )
		incoming_reliable_sequence ^= 1u;

	last_received = m_System->GetTime();
	stream = flow[1].current & 31;
	flow[1].stats[stream].size = size + 28;
	flow[1].stats[stream].time = m_System->GetTime();
	flow[1].current++;

	for ( stream = 0; stream < 2; stream++ )
	{
		fragbuf_t* pbuf;
		int cursor;
		unsigned char* start;
		int tail;

		if ( !fragMessage[stream] )
			continue;

		if ( fragId[stream] != 0 )
		{
			pbuf = FindBufferById( &incomingbufs[stream], (int)fragId[stream], true );
			if ( pbuf == NULL )
			{
				m_System->DPrintf( "NetChannel::ProcessIncoming: couldn't allocate or find buffer %i\n",
						(int)( ( fragId[stream] >> 16 ) & 0xFFFF ) );
			}
			else
			{
				cursor = message.CurrentSize();
				memcpy( pbuf->data, message.data + cursor + fragOffset[stream], (size_t)fragLength[stream] );
				pbuf->size = fragLength[stream];
			}

			CheckForCompletion( stream, (int)( fragId[stream] & 0xFFFF ) );
		}

		cursor = message.CurrentSize();
		start = message.data + cursor + fragOffset[stream];
		tail = message.maxSize - cursor - fragOffset[stream] - fragLength[stream];
		if ( tail > 0 )
		{
			unsigned char* src = start + fragLength[stream];
			for ( int i = 0; i < tail; i++ )
				start[i] = src[i];
		}

		message.maxSize -= fragLength[stream];
		if ( stream == 0 )
			fragOffset[1] -= fragLength[stream];
	}

	remaining = message.maxSize - message.CurrentSize();
	if ( remaining <= 0 )
		return;

	packet = new NetPacket;
	packet->connectionless = false;
	packet->hasReliableData = ( raw & NETCHAN_RELIABLE_FLAG ) != 0;
	packet->time = m_System->GetTime();
	packet->seqnr = (unsigned int)incoming_sequence;
	packet->address.FromNetAddress( &remote_address );
	packet->data.Resize( (unsigned int)remaining );
	packet->data.WriteBuf( message.currentByte, remaining );
	packet->data.Reset();
	incomingPackets.AddHead( packet );
}
