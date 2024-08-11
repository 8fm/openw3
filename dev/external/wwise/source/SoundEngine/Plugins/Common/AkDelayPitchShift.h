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

#ifndef AK_DELAYPITCHSHIFT_H_
#define AK_DELAYPITCHSHIFT_H_

// Pitch shifting using fractional delay line implementation with crossfaded taps read at a different rate than written
#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/DSP/AkDelayLineMemory.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#include <math.h>

#if defined(AK_XBOX360) || defined(AK_PS3)
	#define AKDELAYPITCHSHIFT_USETWOPASSALGO
#endif

namespace AK
{
	namespace DSP 
	{
		class AkDelayPitchShift 
		{
		public:

			AkDelayPitchShift()
				: m_fReadWriteRateDelta(0.f)
				, m_uNumChannels(0)
				, m_uDelayLength(0)
			{
				for ( AkUInt32 i = 0; i < AK_VOICE_MAX_NUM_CHANNELS; i++ )
					m_fFractDelay[i] = 0.f;
			}
#ifndef __SPU__
			AKRESULT Init( 
				AK::IAkPluginMemAlloc * in_pAllocator, 
				AkReal32 in_MaxDelayTime, 
				AkUInt32 in_uNumChannels, 
				AkUInt32 in_uSampleRate 
				);
			void Term( AK::IAkPluginMemAlloc * in_pAllocator );
			void Reset();
#endif // #ifndef __SPU__

			void SetPitchFactor( AkReal32 in_fPitchFactor );

#ifndef AKDELAYPITCHSHIFT_USETWOPASSALGO
			void ProcessChannel( 
				AkReal32 * in_pfInBuf, 
				AkReal32 * out_pfOutBuf, 
				AkUInt32 in_uNumFrames, 
				AkUInt32 in_uChanIndex	
#ifdef __SPU__
				, AkReal32 * in_pfDelayStorage
				, AkUInt32 in_uDMATag
#endif
				);
#else
			void ProcessChannel( 
				AkReal32 * in_pfInBuf, 
				AkReal32 * out_pfOutBuf, 
				void * in_pTempStorage,
				AkUInt32 in_uNumFrames, 
				AkUInt32 in_uChanIndex	
#ifdef __SPU__
				, AkReal32 * in_pfDelayStorage
				, AkUInt32 in_uDMATag
#endif
				);
#endif // AKDELAYPITCHSHIFT_USETWOPASSALGO

#ifdef AK_PS3
			AkUInt32 GetScratchSize()
			{
				// All channels have same delay length
				return m_DelayLines[0].GetDelayLength()*sizeof(AkReal32);
			}
#endif
	
		protected:

			AK::DSP::CAkDelayLineMemory<AkReal32,1> m_DelayLines[AK_VOICE_MAX_NUM_CHANNELS];	
			AkReal32 m_fFractDelay[AK_VOICE_MAX_NUM_CHANNELS];  // Master delay, other is slave and half delay length away
			AkReal32 m_fReadWriteRateDelta;
			AkUInt32 m_uNumChannels;
			AkUInt32 m_uDelayLength; // Different from allocated length for read chase write
		};
	} // namespace DSP 
} // namespace AK

#endif // AK_DELAYPITCHSHIFT_H_

