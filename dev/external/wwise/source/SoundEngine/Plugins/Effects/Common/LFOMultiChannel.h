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

#ifndef _AKLFO_MULTICHANNEL_H_
#define _AKLFO_MULTICHANNEL_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>

#include "LFOMono.h"

// ---------------------------------------------------------------------
// MultiChannelLFO.
// Holds N LFO channels. This class handles computation of individual
// initial phase offsets for each channel based on the phase parameters.
// Hosts a polarity policy.
//
// Usage:
// - Instantiate a MultiChannelLFO with desired polarity 
//		typedef MultiChannelLFO<Unipolar>	UnipolarMultiChannelLFO;
//		or
//		typedef MultiChannelLFO<Bipolar>	BipolarMultiChannelLFO;
// - Init(), then call SetParams() whenever parameters change.
// - Produce samples for each channel: 
//		Access each channel (GetChannel()), then 
//		either call single-sample production methods 
//		explicitly inside a for() loop (see example below), or allocate 
//		a buffer and call ProduceBuffer().
// ---------------------------------------------------------------------

namespace DSP
{
	template < class PolarityPolicy, class OutputPolicy >
	class MultiChannelLFO
	{
	public:
		
#ifndef __SPU__

		MultiChannelLFO() 
			: m_uNumChannels( 0 )
			{}
		
		// Init multichannel LFO. Instantiates and initialize all internal single channel LFOs, with
		// proper individual phase offsets based on the phase parameters.
		void Setup(	
			AkChannelMask		in_uChannelMask,// Channel configuration.
			AkUInt32 			in_uSampleRate,	// Output sample rate (Hz).
			const LFO::MultiChannel::AllParams & in_params		// LFO parameters.
			)
		{
			m_uNumChannels = AK::GetNumChannels( in_uChannelMask );
			if ( m_uNumChannels > 0 )
			{
				AkReal32 * arChannelOffsets = (AkReal32*)AkAlloca( m_uNumChannels * sizeof( AkReal32 ) );
				
				LFO::MultiChannel::ComputeInitialPhase( 
					in_uChannelMask, 
					in_params.phaseParams,
					arChannelOffsets
					);

				for ( AkUInt32 iChan=0; iChan<m_uNumChannels; iChan++ )
				{
					m_arLfo[iChan].Setup(
						in_uSampleRate, 
						in_params.lfoParams,
						arChannelOffsets[iChan] );
				}
			}
		}

		// Call this whenever one of the LFO::Params has changed. Dispatches to all LFO channels.
		void SetParams(	
			AkUInt32			in_uSampleRate,	// Sample rate (Hz)
			const LFO::Params &	in_lfoParams	// Single-channel LFO parameters (applied on all channels).
			)
		{
			// Filter coefficients are the same for all LFO channels. Compute once.
			AkReal32 fB0, fA1;
			MonoLFO<PolarityPolicy, OutputPolicy>::ComputeFilterCoefsFromParams( in_uSampleRate, in_lfoParams, fB0, fA1 );
			
			for ( AkUInt32 iChan=0; iChan<m_uNumChannels; iChan++ )
			{
				m_arLfo[iChan].SetParams( in_uSampleRate, in_lfoParams, fB0, fA1 );
			}
		}
		
#endif // __SPU__

		
		// Access each LFO channel individually.
		AkForceInline AkUInt32 GetNumChannels() { return m_uNumChannels; }
		AkForceInline MonoLFO<PolarityPolicy, OutputPolicy> & GetChannel( AkUInt32 in_uChannel )
		{
			return m_arLfo[in_uChannel];
		}

	protected:

		MonoLFO<PolarityPolicy, OutputPolicy>	m_arLfo[AK_VOICE_MAX_NUM_CHANNELS];
		AkUInt32				m_uNumChannels;

	} AK_ALIGN_DMA;

} // namespace DSP

#endif // _AKLFO_MULTICHANNEL_H_

