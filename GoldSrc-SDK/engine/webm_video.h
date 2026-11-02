#pragma once

struct SDL_Window;
struct vpx_codec_ctx;
struct VorbisDecoder;

namespace mkvparser
{
class IMkvReader;
class Segment;
class Cluster;
class Block;
class BlockEntry;
class VideoTrack;
class AudioTrack;
}

#if defined( _WIN32 )
#pragma pack( push, 8 )
#else
#pragma pack( push, 4 )
#endif
class WebMFrame
{
public:
	WebMFrame();
	~WebMFrame();
	WebMFrame& operator=( const WebMFrame& other );

	int m_uBufferSize;
	int m_uBufferCapacity;
	unsigned char* m_pBuffer;
	double m_flTime;
	bool m_bKey;
};
#pragma pack( pop )

class WebMDemuxer
{
public:
	enum VIDEO_CODEC
	{
		NO_VIDEO = 0,
		VIDEO_VP8 = 1,
		VIDEO_VP9 = 2
	};

	enum AUDIO_CODEC
	{
		NO_AUDIO = 0,
		AUDIO_VORBIS = 1,
		AUDIO_OPUS = 2
	};

	WebMDemuxer( mkvparser::IMkvReader* reader, int videoTrack, int audioTrack );
	~WebMDemuxer();

	double GetLength() const;

	VIDEO_CODEC GetVideoCodec() const;
	int GetWidth() const;
	int GetHeight() const;

	AUDIO_CODEC GetAudioCodec() const;
	const unsigned char* GetAudioExtradata( unsigned int& size ) const;
	double GetSampleRate() const;
	int GetChannels() const;
	int GetAudioDepth() const;

	bool ReadFrame( WebMFrame* videoFrame, WebMFrame* audioFrame );

	mkvparser::IMkvReader* m_pReader;
	mkvparser::Segment* m_pSegment;
	const mkvparser::Cluster* m_pCluster;
	const mkvparser::Block* m_pBlock;
	const mkvparser::BlockEntry* m_pBlockEntry;
	int m_nBlockFrameIndex;

	const mkvparser::VideoTrack* m_pVideoTrack;
	VIDEO_CODEC m_eVideoCodec;

	const mkvparser::AudioTrack* m_pAudioTrack;
	AUDIO_CODEC m_eAudioCodec;

	bool m_bIsOpen;
	bool m_bEOS;
};

class VPXDecoder
{
public:
	enum IMAGE_ERROR
	{
		VPX_UNSUPPORTED_FRAME = -1,
		VPX_NO_ERROR = 0,
		VPX_NO_FRAME = 1
	};

	struct Image
	{
		int GetWidth( int plane ) const;
		int GetHeight( int plane ) const;

		int w;
		int h;
		int cs;
		int chromaShiftW;
		int chromaShiftH;
		unsigned char* planes[3];
		int linesize[3];
	};

	VPXDecoder( const WebMDemuxer& demuxer, unsigned int uThreads );
	~VPXDecoder();

	bool Decode( const WebMFrame& frame );
	IMAGE_ERROR GetImage( Image& image );

	vpx_codec_ctx* m_pCtx;
	const void* m_pIter;
	int m_nDelay;
	int m_nLastSpace;
};

class AudioVorbisDecoder
{
public:
	explicit AudioVorbisDecoder( const WebMDemuxer& demuxer );
	~AudioVorbisDecoder();

	bool IsOpen() const;
	bool OpenVorbis( const WebMDemuxer& demuxer );
	void Close();

	bool GetPCMS16( WebMFrame& frame, short* buffer, int& numOutSamples );

	VorbisDecoder* m_pVorbis;
	int m_nSamples;
	int m_nChannels;
};

class WebMPlayer
{
public:
	void PlayVideo( const char* szPath, SDL_Window* pSDLWindow );
};
