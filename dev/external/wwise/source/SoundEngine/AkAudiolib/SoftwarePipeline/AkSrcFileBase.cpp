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

#include "stdafx.h"
#include "AkSrcFileBase.h"
#include "AkLEngine.h"
#include "AudiolibDefs.h"
#include "AkMonitor.h"
#include "AkProfile.h"
#include "AkPlayingMgr.h"

CAkSrcFileBase::CAkSrcFileBase( CAkPBI * in_pCtx )
	: CAkSrcBaseEx( in_pCtx )
	, m_pStream( NULL )
    , m_pNextAddress( NULL )
    , m_ulSizeLeft( 0 )
    , m_ulFileOffset( 0 )
	, m_uiCorrection( 0 )
#ifdef AK_VITA_HW
	, m_uNextFileOffset( 0 )
#endif
	, m_ulLoopStart( 0 )
	, m_ulLoopEnd( 0 )
	, m_uStreamLoopCntAhead( 0 )
    , m_bIsLastStmBuffer( false )
	, m_bSkipBufferRelease( false )
	, m_bFormatReady( false )
	, m_bIsReadingPrefecth( false )
	, m_bAnalysisAllocated( false )
	, m_bUsingPrefetch( false )
{
}

CAkSrcFileBase::~CAkSrcFileBase()
{
	if ( m_bAnalysisAllocated && m_pAnalysisData )
	{
		AkFree( g_LEngineDefaultPoolId, m_pAnalysisData );
	}

    if ( m_pStream != NULL )
    {
		AKASSERT( !"This should not happen" );
		StopStream();
    }
}

AKRESULT CAkSrcFileBase::StartStream()
{
	if ( m_bFormatReady )
	{
		// Check streaming status if header has already been parsed.
		return IsInitialPrebufferingReady();
	}
	else if ( m_pStream )
	{
		// Try process first buffer if stream is already created and running
        AKRESULT eResult = ProcessFirstBuffer();
		if ( eResult == AK_Success )
			eResult = IsInitialPrebufferingReady();
		return eResult;
	}

	// Specify default buffer constraints. By default, no constraint is necessary. But it is obviously harder
	// for any stream manager implementation to increase the minimum buffer size at run time than to decrease
	// it, so we set it to a "worst case" value.
	AkAutoStmBufSettings bufSettings;
	bufSettings.uBufferSize		= 0;		// No constraint.
	bufSettings.uMinBufferSize	= AK_WORST_CASE_MIN_STREAM_BUFFER_SIZE;
	bufSettings.uBlockSize		= 0;
	AKRESULT eResult = CreateStream( bufSettings, 0 );
	if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
		return eResult;

	bool bUsePrefetchedData;
	eResult = HandlePrefetch( bUsePrefetchedData );
	if ( AK_Success == eResult )
	{
		// Start IO.
        eResult = m_pStream->Start();
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;

		// If the header was not parsed from bank data, attempt to fetch and process the first stream buffer now.
		if ( !bUsePrefetchedData )
		{
			eResult = ProcessFirstBuffer();
			if ( eResult == AK_Success )
				eResult = IsInitialPrebufferingReady();
		}
	}

	return eResult;
}

bool CAkSrcFileBase::SupportMediaRelocation() const
{
	return true;
}

AKRESULT CAkSrcFileBase::RelocateMedia( AkUInt8* in_pNewMedia, AkUInt8* in_pOldMedia )
{
	if( m_bIsReadingPrefecth )
		m_pNextAddress = m_pNextAddress + (AkUIntPtr)in_pNewMedia - (AkUIntPtr)in_pOldMedia;
	return AK_Success;
}

bool CAkSrcFileBase::MustRelocateAnalysisDataOnMediaRelocation()
{
	return m_pAnalysisData != NULL && !m_bAnalysisAllocated;
}

// Create stream from stream manager.
AKRESULT CAkSrcFileBase::CreateStream(
	AkAutoStmBufSettings &	in_bufSettings,		// Stream buffering constraints.
	AkUInt8					in_uMinNumBuffers	// Typical number of streaming buffers that the source requires.
	)
{
	// Get audio context data for stream settings.

    // File name.
	AkSrcTypeInfo * pSrcType = m_pCtx->GetSrcTypeInfo();
    if ( pSrcType->GetFilename() == NULL &&
         pSrcType->GetFileID() == AK_INVALID_FILE_ID )
    {
        //AKASSERT( !"Invalid source descriptor" );
        return AK_Fail;
    }
    // Note. pSrcType->GetFilename() is a null-terminated string that holds the file name.

    // Stream heuristics.
    AkAutoStmHeuristics heuristics;
    heuristics.uMinNumBuffers = in_uMinNumBuffers;
    // Average throughput: updated in ParseHeader() with real format. 
    heuristics.fThroughput = 1;
    // Looping: we do not know anything about looping points just yet.
    heuristics.uLoopStart = 0;
    heuristics.uLoopEnd = 0;
    // Priority.
    heuristics.priority = m_pCtx->GetPriority();

    // Generation of complete file name from bank encoded name and global audio source path settings
    // is done at the file system level. Just fill up custom FS parameters to give it a clue.
    AkFileSystemFlags fileSystemFlags( 
		AKCOMPANYID_AUDIOKINETIC, // Company ID. 
		CODECID_FROM_PLUGINID( pSrcType->dwID ), // File/Codec type ID (defined in AkTypes.h).
		0,					// User parameter size.
        0,                  // User parameter.
        ((bool)pSrcType->mediaInfo.bIsLanguageSpecific), // True when file location depends on current language.
		((bool)pSrcType->mediaInfo.bIsFromRSX), // True if the source indicates that the streamed file should be in the RSX local storage (PS3).
		pSrcType->GetCacheID()
		);

	if (pSrcType->mediaInfo.bExternallySupplied)
	{
		//This stream was started through the external source mechanism.
		//Tell the low level IO that we are only the middleman; we don't know this file, only the game knows about it.	
		//Also, need to disable cache because a different file may be used each time with same source.
		fileSystemFlags.uCompanyID = AKCOMPANYID_AUDIOKINETIC_EXTERNAL;
	}

    // Create stream.
    AKRESULT eResult;
    // Use string overload if pszFilename is set
    // Otherwise use ID overload.
    if ( !pSrcType->UseFilenameString() )
    {
        AKASSERT( pSrcType->GetFileID() != AK_INVALID_FILE_ID );
        eResult = AK::IAkStreamMgr::Get()->CreateAuto( 
                                    pSrcType->GetFileID(),   // Application defined ID.
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Auto stream heuristics.
                                    &in_bufSettings,	// Stream buffer constraints.
                                    m_pStream,
									AK_PERF_OFFLINE_RENDERING);
    }
    else
    {
        eResult = AK::IAkStreamMgr::Get()->CreateAuto( 
                                    pSrcType->GetFilename(),	// Application defined string (title only, or full path, or code...).
                                    &fileSystemFlags,   // File system special parameters.
                                    heuristics,         // Auto stream heuristics.
                                    &in_bufSettings,	// Stream buffer constraints.
                                    m_pStream,
									AK_PERF_OFFLINE_RENDERING);
    }
    
    if ( eResult != AK_Success )
    {
        AKASSERT( m_pStream == NULL );
        //AKASSERT( !"Could not open stream" ); Monitored at a lower level.
        return eResult;
    }

    AKASSERT( m_pStream != NULL );

    // In profiling mode, name the stream.
    // Profiling: create a string out of FileID.
#ifndef AK_OPTIMIZED
    if ( pSrcType->GetFilename() != NULL )
	{
#if defined AK_WIN
		// Truncate first characters. GetFilename() is the whole file path pushed by the WAL, and it may be longer
		// than AK_MONITOR_STREAMNAME_MAXLENGTH.
		size_t len = AKPLATFORM::OsStrLen( pSrcType->GetFilename() );
		if ( len < AK_MONITOR_STREAMNAME_MAXLENGTH )
			m_pStream->SetStreamName( pSrcType->GetFilename() );
		else
		{
			AkOSChar szName[AK_MONITOR_STREAMNAME_MAXLENGTH];
			const AkOSChar * pSrcStr = pSrcType->GetFilename() + len - AK_MONITOR_STREAMNAME_MAXLENGTH + 1;
			AKPLATFORM::SafeStrCpy( szName, pSrcStr, AK_MONITOR_STREAMNAME_MAXLENGTH );
			szName[0] = '.';
			szName[1] = '.';
			szName[2] = '.';
			m_pStream->SetStreamName( szName );
		}
#else
		{
			m_pStream->SetStreamName( (const AkOSChar*)pSrcType->GetFilename() );
		}
#endif
	}
	else
	{
		const unsigned long MAX_NUMBER_STR_SIZE = 11;
		AkOSChar szName[MAX_NUMBER_STR_SIZE];
		AK_OSPRINTF( szName, MAX_NUMBER_STR_SIZE, AKTEXT("%u"), pSrcType->GetFileID() );
		m_pStream->SetStreamName( szName );
	}
#endif

	return eResult;
}

// Check if bank data can be used. If it can, header is parsed and consumed.
AKRESULT CAkSrcFileBase::HandlePrefetch(
	bool & out_bUsePrefetchedData		// Return value: true if data from bank was used. m_ulSizeLeft is set accordingly.
	)
{
	// Get data from bank if stream sound is prefetched.
    // If it is, open stream paused, set position to end of prefetched data, then resume.
    out_bUsePrefetchedData = false;
	AkUInt8 * pPrefetchBuffer = NULL;
	if ( m_pCtx->IsPrefetched() && !m_pCtx->RequiresSourceSeek() )
	{
		m_pCtx->GetDataPtr( pPrefetchBuffer, m_ulSizeLeft );
		out_bUsePrefetchedData = ( pPrefetchBuffer != NULL && m_ulSizeLeft != 0 );
		m_bSkipBufferRelease = out_bUsePrefetchedData;
	}

    if ( out_bUsePrefetchedData )
    {
		m_bUsingPrefetch = true;

		// Parse header.
        AKRESULT eResult = ParseHeader( pPrefetchBuffer );
        if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
            return eResult;

		AKASSERT( m_uTotalSamples && m_uDataSize );

		// Process prefetch buffer: handle looping and update stream-side variables.
		eResult = ProcessStreamBuffer( pPrefetchBuffer, true );
        if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;

		// If stream position was not changed because the loop end was found within the prefetch buffer,
		// change the stream position to the end of the prefetched data. 
		if ( !Stream_IsLoopPending() )
		{
			eResult = SetStreamPosition( m_ulSizeLeft );
			if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
				return eResult;
		}

		// "Consume" header.
		AKASSERT( m_ulSizeLeft >= m_uDataOffset || !"Header must be entirely contained within prefetch buffer" );
		ConsumeData( m_uDataOffset );
    }

	return AK_Success;
}

void CAkSrcFileBase::StopStream()
{
    // Close stream.
    if ( m_pStream != NULL )
    {
        m_pStream->Destroy();
        m_pStream = NULL;
    }

    CAkSrcBaseEx::StopStream();
}

AKRESULT CAkSrcFileBase::StopLooping()
{
	// Override source base StopLooping:
	// *Stream-side* loop count is released. Source-side loop count follows.
	SetLoopCnt( 1 + m_uStreamLoopCntAhead );

	if ( m_pStream )
	{
		// Set stream's heuristics to non-looping.
		AkAutoStmHeuristics heuristics;
		m_pStream->GetHeuristics( heuristics );
		heuristics.uLoopEnd = 0;
		m_pStream->SetHeuristics( heuristics );
	}
	return AK_Success;
}

// Fetch a new stream buffer from the Stream Manager. Stream buffer management members are updated.
// Returns AK_NoDataReady if starving, AK_DataReady if new streaming data was obtained, AK_Fail in case of error.
AKRESULT CAkSrcFileBase::FetchStreamBuffer()
{
	// Stream management members must be clear.
	AKASSERT( m_ulSizeLeft == 0 && !HasNoMoreStreamData() );
	m_pNextAddress = NULL;	// Note. Derived implementations are loose regarding this one.

	// Update priority heuristic.
	AkAutoStmHeuristics heuristics;
    m_pStream->GetHeuristics( heuristics );
    heuristics.priority = m_pCtx->GetPriority( );
    m_pStream->SetHeuristics( heuristics );
    
    // Get stream buffer.
	AkUInt8 * pBuffer;
    AKRESULT eResult = m_pStream->GetBuffer( 
		(void*&)pBuffer,   // Address of granted data space.
        m_ulSizeLeft,       // Size of granted data space.
        AK_PERF_OFFLINE_RENDERING ); // Block until data is ready.

	if ( eResult == AK_DataReady || eResult == AK_NoMoreData )
	{
		if ( m_ulSizeLeft != 0 )
		{
			// Got data: update stream buffer management members. Returns AK_Success or AK_Fail.
			eResult = ProcessStreamBuffer( pBuffer );
			if ( eResult == AK_Success )
				eResult = AK_DataReady;
		}
		else
		{
			AKASSERT( !"Unexpected end of streamed audio file" );
			eResult = AK_Fail;
		}
	}

	AKASSERT( eResult == AK_NoDataReady || eResult == AK_DataReady || eResult == AK_Fail );
	return eResult;
}

// Process newly acquired buffer from stream: 
// Update offset in file, 
// Reset pointers for client GetBuffer(),
// Deal with looping: update count, file position, SetStreamPosition() to loop start.
// Sets bLastStmBuffer flag.
AKRESULT CAkSrcFileBase::ProcessStreamBuffer( 
	AkUInt8 *	in_pBuffer,					// Stream buffer to interpret
	bool		in_bIsReadingPrefecth
	)
{
    AKASSERT( m_pStream );

    // Update offset in file.
    //AKASSERT( m_ulFileOffset == m_pStream->GetPosition( NULL ) ); Not true at first call with prefetched data.

	m_ulFileOffset += m_ulSizeLeft;

    // Set next pointer.
    AKASSERT( in_pBuffer != NULL );
    m_pNextAddress = in_pBuffer + m_uiCorrection;
	m_bIsReadingPrefecth = in_bIsReadingPrefecth;

    // Update size left.
    m_ulSizeLeft -= m_uiCorrection;

    // Will we hit the loopback boundary?
	AkUInt32 ulEndLimit = Stream_Doloop() ? m_ulLoopEnd : m_uDataOffset + m_uDataSize;
    if ( m_ulFileOffset >= ulEndLimit )
    {
		// Yes. Correct size left.
		AkUInt32 ulCorrectionAmount = m_ulFileOffset - ulEndLimit;
		AKASSERT( m_ulSizeLeft >= ulCorrectionAmount ||
			!"Missed the position change at last stream read" );
		m_ulSizeLeft -= ulCorrectionAmount;

		if ( Stream_Doloop() )
		{
			// Change stream position for next read.
			if ( AK_EXPECT_FALSE( SetStreamPosition( m_ulLoopStart ) != AK_Success ) )
				return AK_Fail;
			
			++m_uStreamLoopCntAhead;

            // Update heuristics to end of file if last loop.
            if ( !Stream_Doloop() ) 
            {
                // Set stream's heuristics to non-looping.
                AkAutoStmHeuristics heuristics;
                m_pStream->GetHeuristics( heuristics );
                heuristics.uLoopEnd = 0;
                m_pStream->SetHeuristics( heuristics );
            }
        }
        else
        {
            // Hit the loop release (end of file) boundary: will be NoMoreData.
            // Don't care about correction.
            // Set this flag to notify output.
            m_bIsLastStmBuffer = true;
        }
    }
    else
    {
        // If it will not loop inside that buffer, reset the offset.
        m_uiCorrection = 0;
    }
    return AK_Success;
}

// Process first buffer from stream: 
// If not ready, returns AK_FormatNotReady.
// Otherwise, 
// - calls parse header;
// - sets m_bIsReady flag.
AKRESULT CAkSrcFileBase::ProcessFirstBuffer()
{
	EnterPreBufferingState();

	AkUInt8 * pBuffer;
    AKRESULT eResult = m_pStream->GetBuffer(
                            (void*&)pBuffer,	// Address of granted data space.
                            m_ulSizeLeft,		// Size of granted data space.
                            AK_PERF_OFFLINE_RENDERING );
    
    if ( eResult == AK_NoDataReady )
    {
        // Not ready. Leave.
        return AK_FormatNotReady;
    }
    else if ( eResult != AK_DataReady &&
              eResult != AK_NoMoreData )
    {
        // IO error.
        return AK_Fail;
    }

	AKASSERT( pBuffer );

	// Parse header. 
	eResult = ParseHeader( pBuffer );
    if ( eResult != AK_Success )
        return eResult;

	AKASSERT( m_uTotalSamples && m_uDataSize );

	if ( m_pCtx->RequiresSourceSeek() )
	{
		eResult = SeekToSourceOffset();

		// Flush streamed input.
		if ( m_ulSizeLeft != 0 )
		{
			ReleaseStreamBuffer();
			m_ulSizeLeft = 0;
		}
	}
	else
	{
		// Store the file offset of the beginning of the stream buffer before processing it. 
		// It will be needed to properly skip the header.
		// Usually, the file offset is 0 (beginning of file), unless the implementation of 
		// ParseHeader() performs file seeking.
		AkUInt32 uRealStreamBufferFileOffset = (AkUInt32)m_pStream->GetPosition( NULL ) + m_uiCorrection;
		AKASSERT( uRealStreamBufferFileOffset <= m_uDataOffset || !"Stream buffer must be exactly at, or before, the beginning of the data section" );

		// Process stream buffer: handle looping and update stream-side variables.
		eResult = ProcessStreamBuffer( pBuffer );

		// "Consume" header.
		AkUInt32 uSkipSize = m_uDataOffset - uRealStreamBufferFileOffset;
		AKASSERT( m_ulSizeLeft >= uSkipSize || !"Header must be entirely contained within first stream buffer" );
		ConsumeData( uSkipSize );
	}

	m_bFormatReady = true;
	return eResult;
}

void CAkSrcFileBase::VirtualOn( AkVirtualQueueBehavior eBehavior )
{
	// Note: Stopping the stream will likely let the stream manager flush all the buffers
	// that we don't currently own.
	m_pStream->Stop();

	// In elapsed and beginning mode, release our streaming buffer.
	// In resume mode, don't touch anything.
	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime 
		|| eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		if ( m_ulSizeLeft != 0 )
		{
			ReleaseStreamBuffer();
			m_pNextAddress = NULL;
			m_ulSizeLeft = 0;
		}

		// In elapsed mode, m_ulFileOffset is NOT USED. It is reset in VirtualOff.
		// m_ulCurSample is our virtual pointer (new in 2010.1).
		// In beginning mode, m_ulCurSample is set to 0 in VirtualOff.
	}
}

AKRESULT CAkSrcFileBase::VirtualOff( AkVirtualQueueBehavior eBehavior, bool in_bUseSourceOffset )
{
	if ( eBehavior == AkVirtualQueueBehavior_FromElapsedTime )  // Do not release/seek when last buffer.
	{
		// Set stream position now for next read.
		AKRESULT eResult;
		if ( !in_bUseSourceOffset )
		{
			AKASSERT ( m_uCurSample < m_uTotalSamples );
			// Use virtual pointer.
			// Note: Seeking error is not dealt with: we always snap to codec's seeking granularity, whatever it is.
			eResult = SeekStream( m_uCurSample, m_uCurSample );
			if( eResult != AK_Success )
			{
				// We failed to seek, most chances are we failed because we had no seek table
				// Default seeking at the beginning.
				eResult = SeekStream( 0, m_uCurSample );
			}
		}
		else
		{
			eResult = SeekToSourceOffset();
		}

		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;
	}
	else if ( eBehavior == AkVirtualQueueBehavior_FromBeginning )
	{
		m_uCurSample = 0;
		
		// Reset loop count.
		// Important: must be done before calling SeekStream() to set proper heuristics.
		ResetLoopCnt();

		// Set stream position now for next read.
		AKRESULT eResult = SeekStream( 0, m_uCurSample );
		if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
			return eResult;
		AKASSERT( m_uCurSample == 0 || !"All codecs should be able to seek at the beginning" );	
	}
	else if ( eBehavior == AkVirtualQueueBehavior_Resume )
		EnterPreBufferingState();

	return m_pStream->Start();
}

// Random seeking to position stored in PBI.
AKRESULT CAkSrcFileBase::SeekToSourceOffset()
{
	AKASSERT( m_pCtx->RequiresSourceSeek() );

	AkUInt32 uSourceOffset = GetSourceOffset();

	if ( AK_EXPECT_FALSE( SeekStream( uSourceOffset, m_uCurSample ) != AK_Success ) )
		return AK_Fail;

	AKASSERT( uSourceOffset >= m_uCurSample );

	// Push difference between desired and actual sample position to PBI. The pitch node will consume it.
	m_pCtx->SetSourceOffsetRemainder( uSourceOffset - m_uCurSample );

	return AK_Success;
}

// Random seeking in stream. Sets the stream position to the closest seekable sample in file,
// and returns the actual position (format-dependent). 
// IMPORTANT: Ensure that you fix the source-side loop count before calling this function. 
// Stream-side loop count and streaming heuristics are updated herein.
AKRESULT CAkSrcFileBase::SeekStream( 
	AkUInt32 in_uDesiredSample,		// Desired sample position in file.
	AkUInt32 & out_uSeekedSample	// Returned sample where file position was set.
	)
{
	// Check desired sample against total number of samples before calling codec.
	// We want to avoid backtracking to a valid value within the file if seeking occurs past the end of file.
	if ( in_uDesiredSample >= m_uTotalSamples )
	{
		MONITOR_SOURCE_ERROR( AK::Monitor::ErrorCode_SeekAfterEof, m_pCtx );
		return AK_Fail;
	}

	AkUInt32 uFileOffset;
	if ( AK_EXPECT_FALSE( FindClosestFileOffset( 
			in_uDesiredSample,
			out_uSeekedSample,
			uFileOffset ) != AK_Success ) )
	{
		// Note: In case of failure, the codec is expected to post its own reason to monitoring.
		return AK_Fail;
	}

	// Reset stream-side loop count.
	m_uStreamLoopCntAhead = 0;

	// Change stream position.
	if ( AK_EXPECT_FALSE( SetStreamPosition( uFileOffset ) != AK_Success ) )
		return AK_Fail;

#ifdef AK_VITA_HW
#if AK_VOICE_BUFFER_POSITION_TRACKING
	char msg[256];
	sprintf( msg, "-- -- -- -- [%p]SetStreamPosition %i \n", this, m_ulFileOffset );
	AKPLATFORM::OutputDebugMsg( msg );
#endif

	m_uNextFileOffset = m_ulFileOffset;
#endif
	
	ResetStreamingAfterSeek();
	
	return AK_Success;
}

// Changes position of stream for next read and updates internal file pointer and correction value
// to handle streaming block size.
AKRESULT CAkSrcFileBase::SetStreamPosition( 
	AkUInt32 in_uPosition	// Position in file, in bytes.
	)
{
	AkInt64 lRealOffset;
	
	// Set stream position now for next read.
	AKRESULT eResult = m_pStream->SetPosition( in_uPosition, AK_MoveBegin, &lRealOffset );
	if ( AK_EXPECT_FALSE( eResult != AK_Success ) )
		return AK_Fail;

	// Keep track of offset caused by unbuffered IO constraints.
	// Set file offset to true value.
	m_uiCorrection = in_uPosition - (AkUInt32)lRealOffset;
	m_ulFileOffset = (AkUInt32)lRealOffset;

	return AK_Success;
}

// Helper: Reset stream-side loop count, heuristics and prebuffering status after a random seek.
// IMPORTANT: Ensure that you fix the source-side loop count before calling this function. 
// Streaming heuristics are updated herein.
void CAkSrcFileBase::ResetStreamingAfterSeek()
{
	AKASSERT( !Stream_IsLoopPending() );

	// Update looping heuristics on stream.
	AkAutoStmHeuristics heuristics;
	m_pStream->GetHeuristics( heuristics );
	GetStreamLoopHeuristic( DoLoop(), heuristics );
	m_pStream->SetHeuristics( heuristics );

	m_bIsLastStmBuffer = false;

	// Reset prebuffering status.
	EnterPreBufferingState();
}

// User seek.
AKRESULT CAkSrcFileBase::ChangeSourcePosition()
{
	if ( SeekToSourceOffset() == AK_Success )
	{
		// Flush streamed data.
		ReleaseStreamBuffer();		
		m_ulSizeLeft = 0;
		m_pNextAddress = NULL;
		
		return AK_Success;
	}
	return AK_Fail;
}

// Sets loop values of AkAutoStmHeuristics according to in_sNumLoop. Does not change the other fields.
void CAkSrcFileBase::GetStreamLoopHeuristic( bool in_bIsLooping, AkAutoStmHeuristics & io_heuristics )
{
	if ( in_bIsLooping )
	{
		io_heuristics.uLoopStart = m_ulLoopStart;
        io_heuristics.uLoopEnd = m_ulLoopEnd;
	}
	else
	{
		io_heuristics.uLoopStart = 0;
		io_heuristics.uLoopEnd = 0;
	}
}

// Store analysis data (for streamed files, or anything that doesn't have access to header after parsing). 
// Performs deep copy. 
AKRESULT CAkSrcFileBase::StoreAnalysisData( AkFileParser::AnalysisDataChunk & in_analysisDataChunk )
{
	AKASSERT( in_analysisDataChunk.uDataSize > 0 );

	if ( !m_bUsingPrefetch )
	{	
		m_pAnalysisData = (AkFileParser::AnalysisData*)AkAlloc( g_LEngineDefaultPoolId, in_analysisDataChunk.uDataSize );
		if ( !m_pAnalysisData )
			return AK_InsufficientMemory;

		memcpy( m_pAnalysisData, in_analysisDataChunk.pData, in_analysisDataChunk.uDataSize );
		m_bAnalysisAllocated = true;
	}
	else
	{
		// Assign directly.
		m_pAnalysisData = in_analysisDataChunk.pData;
	}
	return AK_Success;
}
