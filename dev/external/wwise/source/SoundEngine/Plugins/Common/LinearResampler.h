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

#ifndef _AKLINEARRESAMPLER_H_
#define _AKLINEARRESAMPLER_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "SPUInline.h"

namespace DSP
{
	class CAkLinearResampler
	{
	public:

		static const AkUInt32 MAXCHANNELS = AK_VOICE_MAX_NUM_CHANNELS;

#ifndef __SPU__

		CAkLinearResampler() : m_fInterpLoc( 0.f ) 
		{
			AKPLATFORM::AkMemSet( m_fPastVals, 0, MAXCHANNELS*sizeof(AkReal32) );
		}

		void Reset()
		{
			m_fInterpLoc = 0.f;
			AKPLATFORM::AkMemSet( m_fPastVals, 0, MAXCHANNELS*sizeof(AkReal32) );
		}

#endif // #ifndef __SPU__

		SPUInline void Execute(	AkAudioBuffer * io_pInBuffer, 
						AkUInt32		in_uInOffset,
						AkAudioBuffer * io_pOutBuffer,
						AkReal32		in_fResamplingFactor);

	protected:
	
		AkReal32					m_fInterpLoc;
		AkReal32					m_fPastVals[MAXCHANNELS];
	};

} // namespace DSP


#endif // _AKLINEARRESAMPLER_H_
