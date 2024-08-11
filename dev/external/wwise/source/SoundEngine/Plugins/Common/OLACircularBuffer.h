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

#ifndef _OLACIRCULARBUFFER_H_
#define _OLACIRCULARBUFFER_H_

#include "CircularBuffer.h"
#include "SPUInline.h"

namespace DSP
{
	// TODO: Template data type at some point...
	class CAkOLACircularBuffer : public DSP::CAkCircularBuffer
	{
	public:

		SPUInline bool IsDoneTail( );
		
#ifndef __SPU__

		CAkOLACircularBuffer() : m_uWindowSize(0) {}

		AKRESULT Init(	
			AK::IAkPluginMemAlloc *	in_pAllocator,
			AkUInt32 in_uSize,
			AkUInt32 in_uWindowSize );

		// Tries to overlap all frames from a given buffer with current circular buffer contents without overwriting data not yet read
		// Does not do anything if there is insufficient storage.
		// If enough storage does the overlap-add and increment write pointer by hop size provided.
		bool PushOverlappedWindow( 
			AkReal32 * in_pfBuffer, 
			AkUInt32 in_uHopSize );
		// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
		// Actual number of frames filled returned.
		// Also zeroes OLA buffer so that new frames can be pushed without carrying previous history
		AkUInt32 PopFrames( 
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_RequestedFrames,
			bool in_bNoMoredata );

	protected:
#endif

		// Low-level version that take a pointer to its own data (possibly local storage address on SPU

		// Tries to overlap all frames from a given buffer with current circular buffer contents without overwriting data not yet read
		// Does not do anything if there is insufficient storage.
		// If enough storage does the overlap-add and increment write pointer by hop size provided.
		SPUInline bool PushOverlappedWindow( 
			AkReal32 * in_pfBuffer, 
			AkUInt32 in_uHopSize,
			AkReal32 * io_pfData );
		// Tries to fill provided buffer with all frames ready in the circular buffer (if any). 
		// Actual number of frames filled returned.
		// Also zeroes OLA buffer so that new frames can be pushed without carrying previous history
		SPUInline AkUInt32 PopFrames( 
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_RequestedFrames,
			bool in_bNoMoredata,
			AkReal32 * io_pfData );	

	protected:

		AkUInt32 m_uWindowSize;
		
	};

} // namespace DSP


#endif // _OLACIRCULARBUFFER_H_
