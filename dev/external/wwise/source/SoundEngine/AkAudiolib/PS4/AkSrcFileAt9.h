/***********************************************************************
  The content of this file includes source code for the sound engine
  portion of the AUDIOKINETIC Wwise Technology and constitutes "Level
  Two Source Code" as defined in the Source Code Addendum attached
  with this file.  Any use of the Level Two Source Code shall be
  subject to the terms and conditions outlined in the Source Code
  Addendum and the End User License Agreement for Wwise(R).

  Version: v2013.2.9  Build: 4872
  Copyright (c) 2006-2014 Audiokinetic Inc.
 ***********************************************************************/

#pragma once

#include "AkSrcFileBase.h"
#include "AkSrcACPBase.h"

IAkSoftwareCodec* CreateATRAC9FilePlugin( void* in_pCtx );

class AkACPStreamBuffer
{
public:

	AkACPStreamBuffer():
		m_pBuffer(NULL)
		, m_uDataLeft(0)
		, m_uFileOffset(0)
		, m_uInDecoderBytes(0)
		, m_pStitchBuffer(NULL)
		, m_uStitchBufferFillSize(0)
		, m_bStitchBufferMaxSize(0)
#ifdef AT9_STREAM_DEBUG
		, m_uBufferStartInBytes(0)
		, m_uBufferEndInBytes(0)
		, m_iBufferStartInSample(0)
		, m_iBufferEndInSample(0)
		, m_uInDecoderSamples(0)
#endif
	{
	}

	~AkACPStreamBuffer()
	{
		if (m_pStitchBuffer != NULL)
		{
			AkFree(g_LEngineDefaultPoolId, m_pStitchBuffer);
			m_pStitchBuffer = NULL;
		}
	}
		
	AKRESULT Set(	AkUInt8*	in_pBuffer
				, AkUInt32	in_uDataLeft
				, AkInt32		in_uFileOffset
				, AkUInt16	in_nBlockSize
				, AkUInt16	in_nSamplesPerBlock
				, AkUInt32	in_uStitchBufferFillSize
#ifdef AT9_STREAM_DEBUG
				, AkUInt32	in_uBufferStartInBytes
				, AkUInt32	in_uBufferEndInBytes
				, AkInt32	in_iBufferStartInSample
				, AkInt32	in_iBufferEndInSample
#endif
				)
	{
		m_pBuffer = in_pBuffer;
		m_uDataLeft = in_uDataLeft;
		m_uFileOffset = in_uFileOffset;
#ifdef AT9_STREAM_DEBUG
		char msg[256];
		sprintf( msg, "IsLoopOnThisFrame [%i, %i] %i \n", in_iBufferStartInSample, in_iBufferEndInSample, in_uStitchBufferFillSize );
		AKPLATFORM::OutputDebugMsg( msg );
#endif

		if (in_uStitchBufferFillSize > 0)
		{
			if (m_pStitchBuffer == NULL)
			{
				// Alloc only once, if needed once, will be needed throughout the src's life.
				m_pStitchBuffer = (AkUInt8*) AkAlloc( g_LEngineDefaultPoolId, in_nBlockSize * sizeof(AkUInt8) );

				AKASSERT( m_pStitchBuffer != NULL );
				m_bStitchBufferMaxSize = in_nBlockSize;
				
				if (!m_pStitchBuffer)
				{
					return AK_InsufficientMemory;
				}
			}

			AKASSERT(m_pStitchBuffer != NULL);
			AKASSERT(m_uStitchBufferFillSize == 0);

			AKPLATFORM::AkMemCpy(m_pStitchBuffer, in_pBuffer + in_uDataLeft, in_uStitchBufferFillSize);
			m_uStitchBufferFillSize = in_uStitchBufferFillSize;
		}

#ifdef AT9_STREAM_DEBUG
		m_uBufferStartInBytes = in_uBufferStartInBytes;
		m_uBufferEndInBytes = in_uBufferEndInBytes;
		m_iBufferStartInSample = in_iBufferStartInSample;
		m_iBufferEndInSample = in_iBufferEndInSample;
#endif

#ifdef AT9_STREAM_DEBUG
		AKASSERT(m_uDataLeft == (m_uBufferEndInBytes - m_uBufferStartInBytes));
		AKASSERT(m_iBufferStartInSample < m_iBufferEndInSample);
		AKASSERT(m_uBufferStartInBytes < m_uBufferEndInBytes);
#endif

		return AK_Success;
	}

	void Reset()
	{
		m_pBuffer = NULL;
		m_uDataLeft = 0;
		m_uFileOffset = 0;
		m_uInDecoderBytes = 0;
		m_uStitchBufferFillSize = 0;
#ifdef AT9_STREAM_DEBUG
		m_uBufferStartInBytes = 0;
		m_uBufferEndInBytes = 0;
		m_iBufferStartInSample = 0;
		m_iBufferEndInSample = 0;
		m_uInDecoderSamples = 0;
#endif
	}

	bool IsEmpty() const
	{ 
#ifdef AT9_STREAM_DEBUG
		if(m_uDataLeft == 0)
		{ 
			AKASSERT( m_uBufferStartInBytes == m_uBufferEndInBytes );
		}
#endif
		return m_uDataLeft == 0 && m_uStitchBufferFillSize == 0;
	}

	bool IsEmptyOrAllDataInDecoder() const
	{ 
#ifdef AT9_STREAM_DEBUG
		if(m_uDataLeft == 0)
		{ 
			AKASSERT(m_uBufferStartInBytes == m_uBufferEndInBytes );
		} 
#endif
		return m_uDataLeft == 0 || (m_uDataLeft - m_uInDecoderBytes) == 0;
	}

	inline AkUInt8* GetStitchBuffer() const { return m_pStitchBuffer; }
	inline AkUInt32 GetStitchBufferFillSize() const { return m_uStitchBufferFillSize; }
	inline AkUInt32 NeedStitching() const { AKASSERT(m_uStitchBufferFillSize <= m_bStitchBufferMaxSize); return ( (m_uStitchBufferFillSize == 0) ? 0 : m_bStitchBufferMaxSize - m_uStitchBufferFillSize ); }
	inline bool HasStitchBuffer() const { return m_uStitchBufferFillSize > 0; }
	void Stitch( AkUInt8* in_pBuffer, AkUInt32 in_uStitchBufferFillSize)
	{
		AKASSERT(m_pStitchBuffer != NULL);
		AKASSERT(in_pBuffer != NULL);
		AKASSERT(m_uStitchBufferFillSize != 0);
		AKASSERT(in_uStitchBufferFillSize != 0);
		AKASSERT(in_uStitchBufferFillSize + m_uStitchBufferFillSize == m_bStitchBufferMaxSize);

		AKPLATFORM::AkMemCpy(m_pStitchBuffer+m_uStitchBufferFillSize, in_pBuffer, in_uStitchBufferFillSize);
		m_uStitchBufferFillSize += in_uStitchBufferFillSize;
	}

	AkUInt8* GetAddress() const { return m_pBuffer; }
	AkUInt32 GetSizeLeft() const { return  m_uDataLeft + m_uStitchBufferFillSize; }

	AkUInt32 GetBytesLeftInStreamBuffer() const { return m_uDataLeft; }

#ifdef AT9_STREAM_DEBUG
	AkUInt32 GetBufferStartInBytes(){ return m_uBufferStartInBytes; }
	AkUInt32 GetBufferEndInBytes(){ return m_uBufferEndInBytes + m_uStitchBufferFillSize; }
	AkInt32 GetBufferStartInSample(){ return m_iBufferStartInSample; }
	AkInt32 GetBufferEndInSample(){ return m_iBufferEndInSample; }
	AkUInt32 GetInDecoderSamples(){ return m_uInDecoderSamples; }
#endif

	bool IsBufferDecoding(){ return m_uInDecoderBytes != 0; }
	AkUInt32 GetInDecoderBytes(){ return m_uInDecoderBytes; }
	AkInt32 GetBufferFileOffset(){ return m_uFileOffset; }

	void DecodingCompleted( AkUInt16 in_nBlockSize, AkUInt16 in_nSamplesPerBlock )
	{
#ifdef AT9_STREAM_DEBUG_OUTPUT
//		char msg[512];
//		sprintf( msg, "** ** ** ** [%p] DecodingCompleted %lu, %lu, %lu, {%d -> %d} \n", this, m_uInDecoderBytes, m_uDataLeft, m_uDataLeft - m_uInDecoderBytes, m_pBuffer-originalStart, (m_pBuffer+m_uInDecoderBytes)-originalStart);
//		AKPLATFORM::OutputDebugMsg( msg );
#endif


		if (m_uInDecoderBytes > m_uDataLeft)
		{
			m_uInDecoderBytes -= m_uStitchBufferFillSize;
			m_uStitchBufferFillSize = 0;
		}

		//AKASSERT(m_uInDecoderBytes != 0);
		AKASSERT(m_uInDecoderBytes <= m_uDataLeft);
		m_uDataLeft -= m_uInDecoderBytes;
		m_pBuffer += m_uInDecoderBytes;

#ifdef AT9_STREAM_DEBUG
		m_uBufferStartInBytes += m_uInDecoderBytes;
		m_iBufferStartInSample += m_uInDecoderSamples;
		AKASSERT(m_uDataLeft == (m_uBufferEndInBytes - m_uBufferStartInBytes));

		m_uInDecoderSamples = 0;
#endif
		m_uInDecoderBytes = 0;
		
	}

	void SetInDecoderBytes( AkUInt32 in_length, AkUInt32 in_sample )
	{
#ifdef AT9_STREAM_DEBUG
		AKASSERT(in_length <= m_uDataLeft + m_uStitchBufferFillSize);
		m_uInDecoderSamples += in_sample;
#endif

		m_uInDecoderBytes += in_length;
	}

private:
	AkUInt8*	m_pBuffer;
	AkUInt32	m_uDataLeft;
	AkInt32		m_uFileOffset;
	AkUInt32	m_uInDecoderBytes;

	AkUInt8*	m_pStitchBuffer;
	AkUInt32	m_uStitchBufferFillSize;
	AkUInt32	m_bStitchBufferMaxSize;

#ifdef AT9_STREAM_DEBUG
	AkUInt32	m_uBufferStartInBytes;
	AkUInt32	m_uBufferEndInBytes;
	AkInt32		m_iBufferStartInSample;
	AkInt32		m_iBufferEndInSample;
	AkUInt32	m_uInDecoderSamples;
#endif
};

class CAkSrcFileAt9 : public CAkSrcACPBase<CAkSrcFileBase>
{
public:
	//Constructor and destructor
	CAkSrcFileAt9( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileAt9();
	
	//-------------------------------------------------------------------------
	// File specific functions
	AKRESULT							ParseHeader( AkUInt8 * in_pBuffer );
	//-------------------------------------------------------------------------

	virtual AKRESULT					RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia );

	AkUInt32							DecodingEnded();

	virtual bool						Register();
	virtual void						Unregister();
	virtual void						VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT					VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );

	void								UpdateStreamBuffer();
	AKRESULT							GrabNextStreamBuffer();
	void								FixLoopPoints();

	bool								IsDataReady() const { return !(m_bWaitingOnNextBuffer || m_DecodingFailed); }

	virtual AKRESULT					ChangeSourcePosition();
	virtual void						PrepareRingbuffer();
	void								FixStartPosition();
	
	virtual bool	IsLoopOnThisFrame( AkUInt32 in_numSampleToRead ) const
				{		// Use actual loopEnd and not the gentle file loop point for stream, discarted samples should be included to get all data needed and finish on boundaries
					return (m_DecoderSamplePosition + in_numSampleToRead > this->m_uPCMLoopEnd && Decoder_Doloop());
				}

	CAkSrcFileAt9* 						pNextItem;	// List bare light sibling

protected:
	virtual AkReal32					GetThroughput( const AkAudioFormat & in_rFormat );	// Returns format-specific throughput.

	// Finds the closest offset in file that corresponds to the desired sample position.
	// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
	// Returns AK_Fail if the codec is unable to seek.
	virtual AKRESULT					FindClosestFileOffset( 
											AkUInt32 in_uDesiredSample,		// Desired sample position in file.
											AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
											AkUInt32 & out_m_uFileOffset		// Returned file offset where file position was set.
											);

	virtual int							SetDecodingInputBuffers(
											AkInt32 in_uSampleStart, 
											AkInt32 in_uSampleEnd, 
											AkUInt16 in_uBufferIndex, 
											AkUInt32& out_remainingSize);		

	int									OffsetFromStreamBuffer( AkInt32 in_uDecodingOffset, AkUInt32 in_uLenght, AkUInt16 in_uBufferIndex );

#ifdef AT9_STREAM_DEBUG
	AkUInt32							GetBufferStartInBytesInSample(AkUInt32 in_index){ return ByteToSampleRelative(m_pACPStreamBuffer[in_index].GetBufferStartInBytes()); }
#endif

private:

	AkACPStreamBuffer					m_pACPStreamBuffer[2];
	AkInt8								m_uBufferIndex;
	AkInt32								m_ulFileOffsetPrevious;

	AkUInt8* sptr;
	bool								m_bBufferSwap;

	bool								m_bWaitingOnNextBuffer;
};

/*virtual bool SupportMediaRelocation() const
{
	//Hardware decoders currently do not support HotSwapping on prefetch, but they can still swap if the source is not currently processing the prefetch.
	if( !m_bIsReadingPrefecth )
		return true;
	return false;
}*/