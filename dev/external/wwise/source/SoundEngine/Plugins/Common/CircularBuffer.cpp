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

#include "CircularBuffer.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>

namespace DSP
{
#ifndef __SPU__

	AKRESULT CAkCircularBuffer::Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,
		AkUInt32 in_uSize )
	{
		m_uSize = in_uSize;
		m_uFramesReady = 0;
		m_uReadOffset = 0;
		m_uWriteOffset = 0;

		m_pfData = (AkReal32 *)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA(m_uSize*sizeof(AkReal32)) );
		if ( m_pfData == NULL )
			return AK_InsufficientMemory;

#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPushed = 0;
		m_uTotalFramesPopped = 0;
#endif

		return AK_Success;
	}

	void CAkCircularBuffer::Term( AK::IAkPluginMemAlloc * in_pAllocator )
	{
		if ( m_pfData )
		{	
			AK_PLUGIN_FREE( in_pAllocator, m_pfData );
			m_pfData = NULL;
		}
		m_uFramesReady = 0;
	}

	void CAkCircularBuffer::Reset( )
	{
		if ( m_pfData )
		{	
			AkZeroMemLarge( m_pfData, AK_ALIGN_SIZE_FOR_DMA(m_uSize*sizeof(AkReal32)) );
		}
		m_uFramesReady = 0;
		m_uReadOffset = 0;
		m_uWriteOffset = 0;
	}

	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read. 
	// Actual number of frames pushed returned
	AkUInt32 CAkCircularBuffer::PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames )
	{
		return PushFrames( in_pfBuffer, in_NumFrames, m_pfData );
	}

	// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
	// Actual number of frames filled returned.
	// Frees up space for more frames to be pushed later by advancing the read position in the circular buffer
	AkUInt32 CAkCircularBuffer::PopFrames( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_RequestedFrames )
	{
		return PopFrames( out_pfBuffer, in_RequestedFrames, m_pfData );
	}

	// Fills the provided buffer if enough valid frames are available
	// Does not advance the read position in case this location is required to be read again
	bool CAkCircularBuffer::ReadFrameBlock( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_uBlockSize,
		bool in_bNoMoreInputData )
	{
		return ReadFrameBlock( out_pfBuffer, in_uBlockSize, in_bNoMoreInputData, m_pfData );
	}

#endif // #ifndef __SPU__

	// Invalidates frames that won't need to be read anymore
	// Frees up space for more frames to be pushed later by advancing the read position in the circular buffer
	// Actual number of frames skipped returned
	AkUInt32 CAkCircularBuffer::AdvanceFrames( AkUInt32 in_uNumFrames )
	{
		AkUInt32 uFramesToSkip = AkMin( in_uNumFrames, m_uFramesReady );
		m_uReadOffset = (m_uReadOffset + uFramesToSkip) % m_uSize;
		m_uFramesReady -= uFramesToSkip;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPopped += uFramesToSkip;
#endif
		return uFramesToSkip;
	}

	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read.
	// Actual number of frames pushed returned
	AkUInt32 CAkCircularBuffer::PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames,
		AkReal32 * io_pfData )
	{
		AkUInt32 uFramesToWrite = AkMin( FramesEmpty(), in_NumFrames );
		if ( !in_pfBuffer || uFramesToWrite == 0 )
			return 0;

		AkUInt32 uFramesBeforeWrap = AkMin( uFramesToWrite, m_uSize-m_uWriteOffset );
		if ( uFramesBeforeWrap )
			AKPLATFORM::AkMemCpy( &io_pfData[m_uWriteOffset], in_pfBuffer, uFramesBeforeWrap*sizeof(AkReal32) );
		AkUInt32 uFramesLeftToWrite = uFramesToWrite-uFramesBeforeWrap;
		if ( uFramesLeftToWrite )
			AKPLATFORM::AkMemCpy( &io_pfData[0], in_pfBuffer+uFramesBeforeWrap, uFramesLeftToWrite*sizeof(AkReal32) );
		m_uWriteOffset = (m_uWriteOffset + uFramesToWrite) % m_uSize;
		m_uFramesReady += uFramesToWrite;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPushed += uFramesToWrite;
#endif
		return uFramesToWrite;
	}

	// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
	// Actual number of frames filled returned.
	// Frees up space for more frames to be pushed later by advancing the read position in the circular buffer
	AkUInt32 CAkCircularBuffer::PopFrames( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_RequestedFrames,
		AkReal32 * io_pfData )
	{
		AkUInt32 uFramesToRead = AkMin( m_uFramesReady, in_RequestedFrames );
		if ( !out_pfBuffer || uFramesToRead == 0 )
			return 0;

		AkUInt32 uFramesBeforeWrap = AkMin( uFramesToRead, m_uSize-m_uReadOffset );
		AKPLATFORM::AkMemCpy( out_pfBuffer, &io_pfData[m_uReadOffset], uFramesBeforeWrap*sizeof(AkReal32) );
		AkUInt32 uFramesLeftToRead = uFramesToRead-uFramesBeforeWrap;
		if ( uFramesLeftToRead )
			AKPLATFORM::AkMemCpy( out_pfBuffer+uFramesBeforeWrap, &io_pfData[0], uFramesLeftToRead*sizeof(AkReal32) );
		m_uReadOffset = (m_uReadOffset + uFramesToRead) % m_uSize;
		m_uFramesReady -= uFramesToRead;
#ifdef CIRCULARBUFFER_DEBUGINFO
		m_uTotalFramesPopped += uFramesToRead;
#endif
		return uFramesToRead;
	}

	// Fills the provided buffer if enough valid frames are available
	// Does not advance the read position in case this location is required to be read again
	bool CAkCircularBuffer::ReadFrameBlock( 
		AkReal32 * out_pfBuffer, 
		AkUInt32 in_uBlockSize,
		bool in_bNoMoreInputData,
		AkReal32 * io_pfData )
	{
		if ( !out_pfBuffer || in_uBlockSize == 0 )
			return false;

		AkUInt32 uFramesToRead;
		if ( !in_bNoMoreInputData )
		{
			if ( FramesReady() < in_uBlockSize )
				return false;
			uFramesToRead = in_uBlockSize;
		}
		else
		{
			if ( FramesReady() > 0 )
			{
				uFramesToRead = AkMin(m_uFramesReady,in_uBlockSize);
				// memset incomplete (last) buffer
				AkUInt32 uFramesToZeroPad = in_uBlockSize-uFramesToRead;
				if ( uFramesToZeroPad )
					AkZeroMemLarge( out_pfBuffer+uFramesToRead, uFramesToZeroPad*sizeof(AkReal32) );
			}
			else
				return false;
		}

		AkUInt32 uFramesBeforeWrap = AkMin( in_uBlockSize, m_uSize-m_uReadOffset );
		AKPLATFORM::AkMemCpy( out_pfBuffer, &io_pfData[m_uReadOffset], uFramesBeforeWrap*sizeof(AkReal32) );
		AkUInt32 uFramesLeftToRead = in_uBlockSize-uFramesBeforeWrap;
		if ( uFramesLeftToRead )
			AKPLATFORM::AkMemCpy( out_pfBuffer+uFramesBeforeWrap, &io_pfData[0], uFramesLeftToRead*sizeof(AkReal32) );	

		return true;
	}
	
} // namespace DSP
