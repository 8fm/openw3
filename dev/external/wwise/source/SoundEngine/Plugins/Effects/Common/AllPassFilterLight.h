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

// Length of delay line is aligned on 4 frames boundary (i.e. may not be suited for reverberation for example)
// Processing frames also must always be aligned on 4 frames boundary
// Vector processing (SIMD)

#ifndef _AKALLPASSFILTERLIGHT_
#define _AKALLPASSFILTERLIGHT_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/Tools/Common/AkAssert.h>
#include "DelayLineLight.h"
#include "SPUInline.h"

namespace DSP
{
	class CAllPassFilterLight : public CDelayLight
	{
	public:
#ifndef __SPU__
		CAllPassFilterLight( ) :
			m_fG( 0.707f ) {}

		 void SetGain( AkReal32 in_fG )
		  {
			  m_fG = in_fG;
		  }

		// Delay line will be rounded to 4 frame boundary
		AKRESULT Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength );
		void Reset( );
		
		// Requirement: in_uNumFrames % 4 == 0
		void ProcessBuffer( AkReal32 * io_pfIOBuf, AkUInt32 in_uNumFrames );
		void ProcessBuffer( AkReal32 * in_pfInBuf, AkReal32 * out_pfOutBuf, AkUInt32 in_uNumFrames );

#else // SPU functions

		// Requirement: in_uNumFrames % 4 == 0
		SPUInline void ProcessBuffer( AkReal32 * io_pfIOBuf, AkUInt32 in_uNumFrames, AkReal32 * io_pfScratchMem, AkUInt32 in_uDMATag );
		SPUInline void ProcessBuffer( AkReal32 * in_pfInBuf, AkReal32 * out_pfOutBuf, AkUInt32 in_uNumFrames, AkReal32 * io_pfScratchMem, AkUInt32 in_uDMATag );

#endif

	protected:
		AkReal32 m_fG;
	};

} // namespace DSP



#endif // _AKALLPASSFILTERLIGHT_
