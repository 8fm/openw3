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

#include <ajm/at9_decoder.h>
#include <ajm.h>
#include "AkSrcBase.h"

#include "AkSrcACPHelper.h"

//#define AT9_STREAM_DEBUG 1
//#define AT9_STREAM_DEBUG_OUTPUT 1

template <class T_BASE>
class CAkSrcACPBase : public T_BASE
{
public:
	// -------------------------------------------------------------------------------------
	//Constructor / destructor

	CAkSrcACPBase( CAkPBI * in_pCtx );
	virtual ~CAkSrcACPBase();

	// -------------------------------------------------------------------------------------
	// From CAkSrcBaseEx

	virtual void		StopStream();
	virtual void		ReleaseBuffer();
	virtual AKRESULT 	SeekToSourceOffset();
	virtual AKRESULT 	ChangeSourcePosition();
	virtual bool		SupportMediaRelocation() const;

	// -------------------------------------------------------------------------------------
	// From CAkSrcBaseEx
	
	AkForceInline bool DecodingStarted() const
						{ 
							return m_DecodingStarted;
						}

	virtual void		GetBuffer( AkVPLState & io_state );
	
	
	// -------------------------------------------------------------------------------------
	// Hardware decoder functions
	
	virtual bool 		Register();
	virtual void		Unregister();
	AKRESULT 			CreateHardwareInstance( const SceAjmContextId in_AjmContextId );
	bool				InitIfNeeded(SceAjmBatchInfo* in_Batch);
	AKRESULT 			CreateDecodingJob(SceAjmBatchInfo* in_Batch);
	AKRESULT 			InitialiseDecoderJob( SceAjmBatchInfo* in_Batch );

	// -------------------------------------------------------------------------------------
	// Decoding job helper functions
	
	AkForceInline AkUInt32 FloorSampleToBlockSize( AkUInt32 in_startSample ) const
						{
							return (in_startSample / m_nSamplesPerBlock) * m_nSamplesPerBlock;  
						}

	AkForceInline AkUInt32 FixDecodingLength( AkUInt32 in_numSample ) const
						{
							if ( in_numSample % m_nSamplesPerBlock != 0 )
							{
								// Round to the next block
								return ((in_numSample / m_nSamplesPerBlock) + 1) * m_nSamplesPerBlock;
							}

							return in_numSample;
						}

	AkForceInline AkUInt32 GetNumSamplesToDecode() const
						{
							return AkMin(m_MaxOutputFramesPerJob, FloorSampleToBlockSize( AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_RingbufferUsedSample) );
						}
	//inline bool DoDecoderLoop() const { return m_uDecoderLoopCntAhead != LOOPING_ONE_SHOT; }
	AkForceInline bool Decoder_Doloop() const
	{ 
		if ( T_BASE::GetLoopCnt() == LOOPING_INFINITE )
			return true;
		// Otherwise, deduce streaming loop count from source loop count and streaming loop ahead.
		AKASSERT( T_BASE::GetLoopCnt() >= m_uDecoderLoopCntAhead && ( T_BASE::GetLoopCnt() - m_uDecoderLoopCntAhead ) >= 1 );
		return ( ( T_BASE::GetLoopCnt() - m_uDecoderLoopCntAhead ) != LOOPING_ONE_SHOT );
	}

	bool AkForceInline Decoder_IsLoopPending() const { return ( m_uDecoderLoopCntAhead > 0 ); }
	//	return (m_DecoderSamplePosition + in_numSampleToRead > this->m_uPCMLoopEnd && Decoder_IsLoopPending());
	virtual AkForceInline bool	IsLoopOnThisFrame( AkUInt32 in_numSampleToRead ) const
						{		// Use actual loopEnd and not the gentle file loop point for stream, discarted samples should be included to get all data needed and finish on boundaries
							AkInt32 iLoopEnd = T_BASE::m_uPCMLoopEnd+1;
							AkInt32 fixedEndOfLoop = FloorSampleToBlockSize(iLoopEnd + (m_nSamplesPerBlock-1)); //round up

							return (( Decoder_Doloop() > 0 ) && (m_DecoderSamplePosition + in_numSampleToRead >= fixedEndOfLoop));
						}

	virtual AkForceInline bool DataNeeded() const
					{
						return !m_IsVirtual && m_Ringbuffer.m_RingbufferUsedSample < AKAT9_RINGBUFFER_SAMPLES && 
							(m_DecoderSamplePosition < (int)T_BASE::m_uTotalSamples || Decoder_Doloop()) &&
							(FloorSampleToBlockSize( AKAT9_RINGBUFFER_SAMPLES - m_Ringbuffer.m_RingbufferUsedSample ) > 0);
					}

	AkForceInline bool	FileEndOnThisFrame( AkUInt32 in_numSampleToRead ) const
						{
							return (m_DecoderSamplePosition + in_numSampleToRead >= T_BASE::m_uTotalSamples && !Decoder_Doloop());
						}

	virtual int			SetDecodingInputBuffers(AkInt32 in_uSampleStart, AkInt32 in_uSampleEnd, AkUInt16 in_uBufferIndex, AkUInt32& out_remainingSize){return 0;};
	AkUInt32			SetDecodingOutputBuffers(size_t in_uSampleLength);
	void				SetSkipSamplesForLooping();

	
	virtual void		PrepareRingbuffer();
#ifdef _DEBUG
	AkInt32 			GetCurrentDecodedReadSample() const;
#endif	

	// -------------------------------------------------------------------------------------
	// Compressed / uncompressed size conversion helper functions
	
	/*AkForceInline*/ AkUInt32 	ByteToSample( AkUInt32 in_byte ) const
	{
		AKASSERT( in_byte <= T_BASE::m_uDataSize );
		AKASSERT( (in_byte % m_nBlockSize) == 0 );
		AKASSERT( ((AkInt64)in_byte * m_nSamplesPerBlock / m_nBlockSize) * m_nBlockSize / m_nSamplesPerBlock == in_byte );
		
		// values should be set in children classes, default to 1 (PCM)
		return ((AkInt64)in_byte / m_nBlockSize * m_nSamplesPerBlock);
	}

	AkForceInline AkUInt32 	SampleToByte( AkUInt32 in_sample ) const
	{
		AKASSERT( ((AkInt64)in_sample * m_nBlockSize / m_nSamplesPerBlock) <= T_BASE::m_uDataSize );
		AKASSERT( ((AkInt64)in_sample * m_nBlockSize / m_nSamplesPerBlock) % m_nBlockSize == 0 );

		// values should be set in children classes, default to 1 (PCM)
		return ((AkInt64)in_sample / m_nSamplesPerBlock * m_nBlockSize) ;
	}

	// AT9 has a block to discart at the begining of the file. Hence, byte:0 -> sample:-m_nSamplesPerBlock
	AkForceInline AkInt32 	ByteToSampleRelative( AkUInt32 in_byte ) const
	{
		AKASSERT( in_byte <= T_BASE::m_uDataSize );
		AKASSERT( (in_byte % m_nBlockSize) == 0 );
		AKASSERT( ((AkInt64)in_byte * m_nSamplesPerBlock / m_nBlockSize) * m_nBlockSize / m_nSamplesPerBlock == in_byte );
		
		// values should be set in children classes, default to 1 (PCM)
		return ((in_byte-m_nBlockSize) / m_nBlockSize * m_nSamplesPerBlock );
	}

	// AT9 has a block to discart at the begining of the file. Hence, sample:0 -> byte:+m_nBlockSize
	AkForceInline AkUInt32 	SampleToByteRelative( AkInt32 in_sample ) const
	{
		in_sample += m_nSamplesPerBlock; //
		AKASSERT( ((AkInt64)in_sample * m_nBlockSize / m_nSamplesPerBlock) <= T_BASE::m_uDataSize );
		AKASSERT( ((AkInt64)in_sample * m_nBlockSize / m_nSamplesPerBlock) % m_nBlockSize == 0 );

		// values should be set in children classes, default to 1 (PCM)
		return in_sample / m_nSamplesPerBlock * m_nBlockSize ;
	}

	virtual AKRESULT OnLoopComplete(
		bool in_bEndOfFile		// True if this was the end of file, false otherwise.
		) 
	{
		if ( !in_bEndOfFile )
		{
			AKASSERT( Decoder_IsLoopPending() ); 
			--m_uDecoderLoopCntAhead;
		}
		return T_BASE::OnLoopComplete( in_bEndOfFile );
	}
	
protected:
	
	AkInt32				m_DecoderSamplePosition;
	AkUInt32			m_MaxOutputFramesPerJob;
	AkUInt32			m_uDecoderLoopCntAhead;
	AkUInt32			m_uSampleSkipOnFileSeekStart;

	AkUInt16			m_nSamplesPerBlock;
	AkUInt16			m_nBlockSize;

	AkUInt8				m_DecodingStarted			:1;
	AkUInt8				m_Initialized				:1;
	AkUInt8				m_DecodingFailed			:1;
	AkUInt8				m_IsVirtual					:1;

	SceAjmBuffer		m_InputBufferDesc[3];
	SceAjmBuffer		m_OutputBufferDesc[2];

	sAkDecodeResult		m_DecodeResult;
	SceAjmInitializeResult	m_InitResult;
	SceAjmInstanceId	m_AjmInstanceId;
	SceAjmContextId		m_AjmContextId;

	CAkDecodingRingbuffer	m_Ringbuffer;
	uint8_t 			m_At9ConfigData[SCE_AJM_DEC_AT9_CONFIG_DATA_SIZE];
	SceAjmDecAt9InitializeParameters 	m_InitParam;
};

#include "AkSrcACPBase.inl"