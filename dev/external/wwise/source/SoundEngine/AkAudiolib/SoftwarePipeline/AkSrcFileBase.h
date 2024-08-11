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

#ifndef _AK_SRC_FILEBASE_H_
#define _AK_SRC_FILEBASE_H_

#include "AkSrcBase.h"
#include "AkSrcFileServices.h"
#include <AK/SoundEngine/Common/IAkStreamMgr.h>
#include "AkProfile.h"

#define AK_WORST_CASE_MIN_STREAM_BUFFER_SIZE	(2048)	// Totally arbitrary. Most sources (that use the default implementation of StartStream()) should not need more.

class CAkSrcFileBase : public CAkSrcBaseEx
{
public:

	//Constructor and destructor
    CAkSrcFileBase( CAkPBI * in_pCtx );
	virtual ~CAkSrcFileBase();

	// Data access.
	virtual AKRESULT StartStream();
	virtual void	 StopStream();

	virtual AKRESULT StopLooping();

	// Virtual voices.
	virtual void     VirtualOn( AkVirtualQueueBehavior eBehavior );
	virtual AKRESULT VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset );

	// User seek.
	virtual AKRESULT ChangeSourcePosition();

	virtual bool SupportMediaRelocation() const;
	virtual AKRESULT RelocateMedia( AkUInt8* in_pNewMedia,  AkUInt8* in_pOldMedia );
	virtual bool MustRelocateAnalysisDataOnMediaRelocation();

#ifndef AK_VITA_HW // Vita HW needs to access this publicly
protected:
#endif

	// Fetch a new stream buffer from the Stream Manager. Stream buffer management members are updated.
	// Returns AK_NoDataReady if starving, AK_DataReady if new streaming data was obtained, AK_Fail in case of error.
	// NOTE: This class cannot provide the file position of the streaming data that you are currently 
	// looking at (using GetStreamData()). Derived implementations may keep track of it by storing the 
	// value returned by GetNextStreamBufferFilePosition() just before calling FetchStreamBuffer().
	AKRESULT FetchStreamBuffer();
	// Release a stream buffer to the Stream Manager. Handle management of prefetch buffer.
	// Sources that use 2 streaming buffers must manage the second buffer on their side.
	inline void ReleaseStreamBuffer()
	{
		if ( !m_bSkipBufferRelease )
			m_pStream->ReleaseBuffer();
		else
			m_bSkipBufferRelease = false;
	}
	
	// Consume streaming data.
	inline void ConsumeData( AkUInt32 uSizeConsumed )
	{
		AKASSERT( uSizeConsumed <= m_ulSizeLeft );
		m_pNextAddress += uSizeConsumed;
		m_ulSizeLeft -= uSizeConsumed;
	}

	// Returns true if stream layer will not be feeding new data. 
	inline bool HasNoMoreStreamData() { return m_bIsLastStmBuffer; }
	inline bool IsDataLeftInBuffer() const { return m_ulSizeLeft > 0; }

protected:

	// Format-specific methods to override.
	// -------------------------------------------
	virtual AKRESULT ParseHeader( 
		AkUInt8 * in_pBuffer	// Buffer to parse
		) = 0;

	virtual AkReal32 GetThroughput( const AkAudioFormat & in_rFormat ) = 0;	// Returns format-specific throughput.

	// Finds the closest offset in file that corresponds to the desired sample position.
	// Returns the file offset (in bytes, relative to the beginning of the file) and its associated sample position.
	// Returns AK_Fail if the codec is unable to seek.
	virtual AKRESULT FindClosestFileOffset( 
		AkUInt32 in_uDesiredSample,		// Desired sample position in file.
		AkUInt32 & out_uSeekedSample,	// Returned sample where file position was set.
		AkUInt32 & out_uFileOffset		// Returned file offset where file position was set.
		) = 0;

	// Overriden from CAkSrcBaseEx.
	// -------------------------------------------
	// Override OnLoopComplete() handler from CAkSrcBaseEx: resolve streaming loop look-ahead.
	virtual AKRESULT OnLoopComplete(
		bool in_bEndOfFile		// True if this was the end of file, false otherwise.
		) 
	{
		if ( !in_bEndOfFile )
		{
			AKASSERT( Stream_IsLoopPending() ); 
			--m_uStreamLoopCntAhead;
		}
		return CAkSrcBaseEx::OnLoopComplete( in_bEndOfFile );
	}
		
	// Streaming services.
	// -------------------------------------------
	
	// Returns position in file of next stream buffer you will acquire with FetchStreamBuffer().
	inline AkUInt32	Stream_GetNextBufferFilePosition() { return m_ulFileOffset + m_uiCorrection; }	

	// Create stream from stream manager.
	AKRESULT CreateStream(
		AkAutoStmBufSettings &	in_bufSettings,		// Stream buffering constraints.
		AkUInt8					in_uMinNumBuffers	// Typical number of streaming buffers that the source requires.
		);
	
	// Check if bank data can be used. If it can, header is parsed and consumed.
	AKRESULT HandlePrefetch(
		bool & out_bUsePrefetchedData	// Return value: true if data from bank was used. m_uSizeLeft is set accordingly.
		);



	// Update the source after fetching a streaming buffer.
	AKRESULT ProcessStreamBuffer( 
		AkUInt8 *	in_pBuffer,					// Stream buffer to interpret
		bool		in_bIsReadingPrefecth = false
		);

	inline AkUInt32 GetDataOffsetFromHeader(){ return m_ulFileOffset - m_uDataOffset; }	

protected:

	// Returns true if stream-side has looped. Source is expected to check its sample position and
	// resolve source-side loop count accordingly.
	inline bool Stream_IsLoopPending() { return ( m_uStreamLoopCntAhead > 0 ); }
	
	// Stream-side counterpart of DoLoop().
	inline bool Stream_Doloop() 
	{ 
		if ( GetLoopCnt() == LOOPING_INFINITE )
			return true;
		// Otherwise, deduce streaming loop count from source loop count and streaming loop ahead.
		AKASSERT( GetLoopCnt() >= m_uStreamLoopCntAhead && ( GetLoopCnt() - m_uStreamLoopCntAhead ) >= 1 );
		return ( ( GetLoopCnt() - m_uStreamLoopCntAhead ) != LOOPING_ONE_SHOT );
	}

	// Returns AK_NoDataReady if we need to wait for sufficient buffering,
	// AK_DataReady if we are ready to go, AK_Fail if there was an error with IO.
	inline AKRESULT IsPrebufferingReady()
	{
		if ( AK_EXPECT_FALSE( IsPreBuffering() && !AK_PERF_OFFLINE_RENDERING ) )
		{
			AKRESULT eResult = AK::SrcFileServices::IsPrebufferingReady( m_pStream, m_ulSizeLeft );
			if ( eResult == AK_DataReady )
				LeavePreBufferingState();
			return eResult;
		}
		else if ( AK_PERF_OFFLINE_RENDERING )
			LeavePreBufferingState();

		return AK_DataReady;
	}

	// Similar to IsPrebufferingReady(). Converts return codes needed in StartStream().
	// Does not leave prebuffering state. This must only be done during pipeline processing, not voice management.
	inline AKRESULT IsInitialPrebufferingReady()
	{
		if ( IsPreBuffering() && !AK_PERF_OFFLINE_RENDERING )
		{
			AKRESULT eResult = AK::SrcFileServices::IsPrebufferingReady( m_pStream, m_ulSizeLeft );
			if ( eResult == AK_NoDataReady )
				eResult = AK_FormatNotReady;
			else if ( eResult == AK_DataReady )
				eResult = AK_Success;
			return eResult;
		}
		return AK_Success;
	}

	// Sets loop values of AkAutoStmHeuristics according to in_bIsLooping. Does not change the other fields.
	void GetStreamLoopHeuristic( bool in_bIsLooping, AkAutoStmHeuristics & io_heuristics );

	// Random seeking to position stored in PBI (wraps SeekStream() below).
	AKRESULT SeekToSourceOffset();

	// Random seeking in stream. Sets the stream position to the closest seekable sample in file,
	// and returns the actual position (format-dependent). 
	// IMPORTANT: Ensure that you fix the source-side loop count before calling this function. 
	// Stream-side loop count and streaming heuristics are updated herein.
	AKRESULT SeekStream( 
		AkUInt32 in_uDesiredSample,		// Desired sample position in file.
		AkUInt32 & out_uSeekedSample	// Returned sample where file position was set.
		);

	//
	// Stream specific helpers.
	//

	// Fetch the first stream buffer and parse the header.
	AKRESULT ProcessFirstBuffer();

	// Changes position of stream for next read and updates internal file pointer and correction value
	// to handle streaming block size.
	AKRESULT SetStreamPosition( 
		AkUInt32 in_uPosition	// Position in file, in bytes.
		);

	// Reset stream-side loop count, heuristics and prebuffering status after a random seek.
	// IMPORTANT: Ensure that you fix the source-side loop count before calling this function. 
	// Streaming heuristics are updated herein.
	void ResetStreamingAfterSeek();

	// Store analysis data (for streamed files, or anything that doesn't have access to header after parsing). 
	// Performs deep copy of envelope table. 
	AKRESULT StoreAnalysisData( AkFileParser::AnalysisDataChunk & in_analysisDataChunk );

protected:

	AK::IAkAutoStream * m_pStream;          // Stream handle.

	// Stream buffer management.
//private:
    AkUInt8 *           m_pNextAddress;		// Address of _current_ stream buffer. NULL if none.
    AkUInt32            m_ulSizeLeft;		// Size left in stream buffer.
    AkUInt32            m_ulFileOffset;		// Offset relative to beginning of file of _next_ stream buffer.
	AkUInt32			m_uiCorrection;		// Correction amount (when loop start not on sector boundary).

#ifdef AK_VITA_HW
	AkUInt32            m_uNextFileOffset;
#endif

protected:
    // Looping data.
    AkUInt32			m_ulLoopStart;      // Loop start position: Byte offset from beginning of stream.
    AkUInt32			m_ulLoopEnd;        // Loop back boundary from beginning of stream.
	AkUInt16			m_uStreamLoopCntAhead;	// Stream-side loop count ahead (of source-side loop count).

	// State machine.
	AkUInt8				m_bIsLastStmBuffer		:1;	// True when last stream buffer.
	AkUInt8				m_bSkipBufferRelease	:1;	// Skip streaming buffer release (due to prefetch).
	AkUInt8				m_bFormatReady			:1;	// True when all headers have been parsed.
	AkUInt8				m_bIsReadingPrefecth	:1; // true when m_pNextAddress contains a pointer to an in memory prefetch.
	AkUInt8				m_bAnalysisAllocated	:1; // true when analysis chunk is allocated by this class.
	AkUInt8				m_bUsingPrefetch		:1; // true when header has been read from prefetch region.
};

class CAkSrcFileBaseHWDecoder : public CAkSrcFileBase
{
public:
	CAkSrcFileBaseHWDecoder( CAkPBI * in_pCtx )
	: CAkSrcFileBase( in_pCtx )
	{
	}

	virtual bool SupportMediaRelocation() const
	{
		//Hardware decoders currently do not support HotSwapping on prefetch, but they can still swap if the source is not currently processing the prefetch.
		if( !m_bIsReadingPrefecth )
			return true;
		return false;
	}
};

#endif // _AK_SRC_FILEBASE_H_
