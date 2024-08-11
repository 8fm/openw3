#include "build.h"
#include "speechBuffer.h"
#include "soundFileLoader.h"

SpeechBuffer::SpeechBuffer()
	: DataBuffer( TDataBufferAllocator< MC_BufferSpeech >::GetInstance() )
	, m_duration( 0.0f )
	, m_compression( SOUND_COMPRESSION_PCM )
{
}

SpeechBuffer::SpeechBuffer( const SpeechBuffer& other )
	: DataBuffer( other )
	, m_duration( other.m_duration )
	, m_compression( other.m_compression )
{
}

SpeechBuffer& SpeechBuffer::operator=( const SpeechBuffer& other )
{
	DataBuffer::operator=( other );
	m_duration = other.m_duration;
	m_compression = other.m_compression;
	return *this;
}

void SpeechBuffer::Clear()
{
	DataBuffer::Clear();
	m_duration = 0.0f;
	m_compression = SOUND_COMPRESSION_PCM;
}

void SpeechBuffer::Serialize( IFile& file )
{
	// Save basic shit
	DataBuffer::Serialize( file );

	// Grab speech duration
	file << m_duration;
	file << m_compression;
}

Bool SpeechBuffer::LoadFromFile( const String &filename )
{
	// Open the file
	IFile* soundFile = GFileManager->CreateFileReader( filename, FOF_AbsolutePath );
	if ( soundFile )
	{
		// Allocate buffer
		Uint32 fileSize = static_cast< Uint32 >( soundFile->GetSize() );
		Allocate( fileSize );

		// Load the file 
		if ( GetData() && GetSize() )
		{
			soundFile->Serialize( GetData(), GetSize() );
		}

		// Close source file
		delete soundFile;

		// Grab duration
		if ( filename.EndsWith( TXT( "wem" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_PCM;
		}
		else if ( filename.EndsWith( TXT( "wav" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_PCM;
		}
		else if ( filename.EndsWith( TXT( "adpcm" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_ADPCM;
		}
		else if ( filename.EndsWith( TXT( "mp2" ) ) == true )
		{
			m_duration = CalculateMp2Duration();
			m_compression = SOUND_COMPRESSION_MP2;
			if( m_duration < 0.0f )
			{
				ERR_ENGINE( TXT("VO: Mp2 corrupted '%ls'"), filename.AsChar() );
				return false;
			}
		}		
		else if ( filename.EndsWith( TXT( "ogg" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_OGG;
		}
		else if ( filename.EndsWith( TXT( "xma" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_XMA;
		}
		else if ( filename.EndsWith( TXT( "xwma" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_XWMA;
		}
		else if ( filename.EndsWith( TXT( "atr" ) ) == true )
		{
			m_duration = CalculateRIFFDuration();
			m_compression = SOUND_COMPRESSION_ATRAC9;
		}
		else
		{
			ASSERT( ! "Unknown speech sound file" );
			return false;
		}

		// Loaded
		return true;
	}
	
	return false;
}

Float SpeechBuffer::CalculateOggDuration() const
{
	Uint8* oggData = (Uint8*) GetData();
	Uint32 fileTypeCheck = *( (Uint32*) oggData );
	//ASSERT( fileTypeCheck == 'OggS' );

	Uint32 dataSize = static_cast< Uint32 >( GetSize() );
	Uint32 dataOffset = 0;

	Uint32 bitrate = 0;

	

	while ( dataOffset < dataSize )
	{
		Uint8* oggPage = oggData + dataOffset;

		Uint8 numberOfSegments =  *( (Uint8*) oggPage + 26 ); 
		Uint32 pageSize = 27 + numberOfSegments;

		for ( Uint8 segmentIndex = 0; segmentIndex < numberOfSegments; ++segmentIndex)
		{
			Uint8 segmentSize = *( (Uint8*) oggPage + 27 + segmentIndex );
			pageSize += segmentSize;
		}

		Uint8 pageHeaderType = *( (Uint8*) oggPage + 5 ); 
		if ( pageHeaderType & 2 )
		{
			bitrate = *( (Uint32*) ( oggPage + 40 ) );
			ASSERT( bitrate != 0 );
		}
		else if ( pageHeaderType & 4 && bitrate != 0 )
		{
			Uint64 granule = *( (Uint64*) ( oggPage + 6 ) );
			return (Float) granule / bitrate;
		}
		
		dataOffset += pageSize;
	}

	

	return 0.0f;
}

Float SpeechBuffer::CalculateRIFFDuration() const
{
	Uint8* waveData = (Uint8*) GetData();

	Uint8* chunkData = waveData + 12;
	
	// Scan for 'fmt' chunk - comparing with reversed marker to compensate byte swapping
	while ( *( (Uint32*) chunkData ) != ' tmf' )
	{
		chunkData += *( (Uint32*) (chunkData + 4) ) + 8;
	}
	
	Uint32 byteRate = *( (Uint32*) (chunkData + 16) );
	
	// Scan for 'data' chunk - comparing with reversed marker to compensate byte swapping
	while ( *( (Uint32*) chunkData ) != 'atad' )
	{
		chunkData += *( (Uint32*) (chunkData + 4) ) + 8;
	}
	Uint32 dataSize = *( (Uint32*) (chunkData + 4 ) );

	return ( (Float) dataSize / (Float) byteRate );
}

Float SpeechBuffer::CalculateMp2Duration() const
{
	static unsigned short bitrates[] = { 0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 };

	//frame counter, every frame is a 1152 samples, which takes about 38,28125 ms at 44.1kHz
	unsigned int frameCount = 0;
	Uint32 caret = 0;

	Uint16 frequency = 0;
	while( caret < GetSize() - 1 )
	{
		++frameCount;
		Uint16* frame = ( Uint16* )( ( Uint8* )GetData() + ( Uint32 ) caret );
		//check mpg frame synchronizer
		if( ( ( *frame & 0x0000F0FF ) != 0x0000F0FF ) || ( ( *frame & 0x00001E00 ) != 0x00001C00 ) )
		{
			//if doesn't match check one byte after
			//crappy way
			frame = ( Uint16* )( ( Uint8* )GetData() + ( Uint32 ) ++caret );
			if( ( ( *frame & 0x0000F0FF ) != 0x0000F0FF ) || ( ( *frame & 0x00001E00 ) != 0x00001C00 ) )
				return -1;
		}

		Uint8 byte = *( ( ( Uint8*)frame ) + 2 );

		if( !frequency )
		{
			if( ( byte & 0x0C ) == 0x04 )
				frequency = 48000;
			else if( ( byte & 0x0C ) == 0x08 )
				frequency = 32000;
			else frequency = 44100;
		}

		byte >>= 4;

		Float frameSize = ( Float( 144 ) * bitrates[ byte ] * 1000 ) / frequency;
		caret += (Uint32) frameSize;
	}

	//when we have number of frames in file we can count length
	Float duration = ( Float )frameCount * 1152 / frequency;
	return duration;
}


Float SpeechBuffer::CalculateXmaDuration() const
{
	Uint8* waveData = (Uint8*) GetData();

	Uint32 byteRate = *( (Uint32*) ( waveData + 28 ) );
	// Swap bytes
	byteRate = (byteRate>>24) | (byteRate<<24) | ((byteRate&0x00ff0000)>>8) | ((byteRate&0x0000ff00)<<8);

	return ( (Float) ( GetSize() - 44 ) ) / ( (Float) byteRate );
}
