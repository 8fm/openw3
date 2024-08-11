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

#include "OLACircularBuffer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace DSP
{
	bool CAkOLACircularBuffer::IsDoneTail( )
	{
		if ( m_uFramesReady )
			return false;

		// Write offset stopped on last write to where
		AkUInt32 uTailEndPos = (m_uWriteOffset + m_uWindowSize) % m_uSize;
		AkUInt32 uFramesToFlush;
		if ( uTailEndPos >= m_uReadOffset )
			uFramesToFlush = uTailEndPos - m_uReadOffset;
		else
			uFramesToFlush = m_uSize - (uTailEndPos - m_uReadOffset);

		return uFramesToFlush == 0;
	}

#ifndef __SPU__

	AKRESULT CAkOLACircularBuffer::Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,
		AkUInt32 in_uSize,
		AkUInt32 in_uWindowSize )
	{
		m_uWindowSize = in_uWindowSize;
		return CAkCircularBuffer::Init( in_pAllocator, in_uSize );
	}

	// Tries to overlap all frames from a given buffer with current circular buffer contents without overwriting data not yet read
	// Does not do anything if there is insufficient storage.
	// If enough storage does the overlap-add and increment write pointer by hop size provided.
	bool CAkOLACircularBuffer::PushOverlappedWindow( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_uHopSize )
	{
		return PushOverlappedWindow(
			in_pfBuffer, 
			in_uHopSize,
			m_pfData );
	}

	// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
	// Actual number of frames filled returned.
	// Also zeroes OLA buffer so that new frames can be pushed without carrying previous history
	AkUInt32 CAkOLACircularBuffer::PopFrames( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_RequestedFrames,
		bool in_bNoMoredata )
	{
		return PopFrames( 
			out_pfBuffer, 
			in_RequestedFrames,
			in_bNoMoredata,
			m_pfData );
	}

#endif // #ifndef __SPU__

	// Tries to overlap all frames from a given buffer with current circular buffer contents without overwriting data not yet read
	// Does not do anything if there is insufficient storage.
	// If enough storage does the overlap-add and increment write pointer by hop size provided.
	bool CAkOLACircularBuffer::PushOverlappedWindow( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_uHopSize,
		AkReal32 * io_pfData )
	{
		AkUInt32 uMaxWriteSize = m_uSize - m_uFramesReady;
		if ( !in_pfBuffer || m_uWindowSize > uMaxWriteSize )
			return false;
		
		AkUInt32 uFramesBeforeWrap = AkMin( m_uWindowSize, m_uSize-m_uWriteOffset );
		{
			AkReal32 * AK_RESTRICT pfOLAData = (AkReal32 * AK_RESTRICT) &io_pfData[m_uWriteOffset];
			AkReal32 * AK_RESTRICT pfInData = (AkReal32 * AK_RESTRICT) in_pfBuffer;
			for ( AkUInt32 i = 0; i < uFramesBeforeWrap; i++ )
				*pfOLAData++ += *pfInData++;
		}
		AkUInt32 uFramesLeftToWrite = m_uWindowSize-uFramesBeforeWrap;
		if ( uFramesLeftToWrite )
		{
			AkReal32 * AK_RESTRICT pfOLAData = (AkReal32 * AK_RESTRICT) &io_pfData[0];
			AkReal32 * AK_RESTRICT pfInData = (AkReal32 * AK_RESTRICT) in_pfBuffer+uFramesBeforeWrap;
			for ( AkUInt32 i = 0; i < uFramesLeftToWrite; i++ )
				*pfOLAData++ += *pfInData++;
		}
		m_uWriteOffset = (m_uWriteOffset + in_uHopSize) % m_uSize;
		m_uFramesReady += in_uHopSize;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPushed += in_uHopSize;
#endif
		return true;
	}

	// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
	// Actual number of frames filled returned.
	// Also zeroes OLA buffer so that new frames can be pushed without carrying previous history
	AkUInt32 CAkOLACircularBuffer::PopFrames( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_RequestedFrames,
		bool in_bNoMoredata,
		AkReal32 * io_pfData )
	{
		if ( !out_pfBuffer || in_RequestedFrames == 0 )
			return 0;

		AkUInt32 uFramesToRead;
		if ( !in_bNoMoredata || m_uFramesReady > 0 )
		{
			uFramesToRead = AkMin( m_uFramesReady, in_RequestedFrames );
			m_uFramesReady -= uFramesToRead;
		}
		else
		{
			// There will be no more input data, so no need to wait for data for overlap add that will never come
			// Finish reading everything that was written to the buffer
			// Write offset stopped on last write to where
			AkUInt32 uTailEndPos = (m_uWriteOffset + m_uWindowSize) % m_uSize;
			AkUInt32 uFramesToFlush;
			if ( uTailEndPos > m_uReadOffset )
				uFramesToFlush = uTailEndPos - m_uReadOffset;
			else
				uFramesToFlush = m_uSize - (uTailEndPos - m_uReadOffset);
			uFramesToRead = AkMin( uFramesToFlush, in_RequestedFrames );
		}

		AkUInt32 uFramesBeforeWrap = AkMin( uFramesToRead, m_uSize-m_uReadOffset );
		AKPLATFORM::AkMemCpy( out_pfBuffer, &io_pfData[m_uReadOffset], uFramesBeforeWrap*sizeof(AkReal32) );
		AKPLATFORM::AkMemSet( &io_pfData[m_uReadOffset], 0, uFramesBeforeWrap*sizeof(AkReal32) );
		AkUInt32 uFramesLeftToRead = uFramesToRead-uFramesBeforeWrap;
		if ( uFramesLeftToRead )
		{
			AKPLATFORM::AkMemCpy( out_pfBuffer+uFramesBeforeWrap, &io_pfData[0], uFramesLeftToRead*sizeof(AkReal32) );
			AKPLATFORM::AkMemSet( &io_pfData[0], 0, uFramesLeftToRead*sizeof(AkReal32) );
		}
		m_uReadOffset = (m_uReadOffset + uFramesToRead) % m_uSize;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPopped += uFramesToRead;
#endif
		return uFramesToRead;
	}
	
} // namespace DSP
