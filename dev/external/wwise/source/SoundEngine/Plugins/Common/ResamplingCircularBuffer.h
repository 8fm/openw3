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

#ifndef _RESAMPLINGCIRCULARBUFFER_H_
#define _RESAMPLINGCIRCULARBUFFER_H_

#include "CircularBuffer.h"

#ifdef AK_XBOX360
#define AKCIRCULARBUFFER_USESIMD
#endif

namespace DSP
{
	class CAkResamplingCircularBuffer : public DSP::CAkCircularBuffer
	{
	public:

	// Fixed point interpolation index
	#define FPBITS 16
	#define FPMUL (1<<FPBITS)
	#define FPMASK (FPMUL-1)
	#define SINGLEFRAMEDISTANCE (FPMUL)

#ifndef __SPU__
	void ForceFullBuffer( )
	{
		Reset();
		m_uFramesReady = m_uSize; // The zeros in the input buffer are considered valid data and will be output
	}

	void Reset();

	// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read.
	// Actual number of frames pushed returned
	// Resamples data befor pushing to circular buffer
	AkUInt32 PushFrames( 
		AkReal32 * in_pfBuffer, 
		AkUInt32 in_NumFrames,
		AkReal32 in_fResamplingFactor );

	protected:
#endif // #ifndef __SPU__

		// Low-level version that take a pointer to its own data (possibly local storage address on SPU

		// Tries to push all frames from a given buffer into circular buffer without overwriting data not yet read.
		// Actual number of frames consumed returned
		// Resamples data befor pushing to circular buffer
		SPUInline AkUInt32 PushFrames( 
			AkReal32 * in_pfBuffer, 
			AkUInt32 in_NumFrames,
			AkReal32 * io_pfData,
			AkReal32 in_fResamplingFactor );
	protected:

		AkReal32		m_fPastVal;
#ifndef AKCIRCULARBUFFER_USESIMD
		AkReal32		m_fInterpLoc;
#else
		AkUInt32		m_uFloatIndex;
#endif
	
	};

} // namespace DSP


#endif // _RESAMPLINGCIRCULARBUFFER_H_
