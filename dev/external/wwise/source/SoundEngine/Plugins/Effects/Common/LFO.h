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

#ifndef _AKLFO_H_
#define _AKLFO_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "OnePoleFilter.h"

// ---------------------------------------------------------------------
// LFO Definitions.
// Defines parameters, common structures and helper functions needed 
// by LFO and multichannel LFO modules.
// ---------------------------------------------------------------------

namespace DSP
{
	namespace LFO
	{
		static const AkReal32 TWO_PI = 2.f * 3.14159265358979323846f;
		static const AkReal32 ONE_OVER_TWO_PI = 0.5f * 0.318309886183790671538f;

		// Waveform type.
		enum Waveform
		{
			WAVEFORM_SINE = 0,
			WAVEFORM_TRIANGLE,
			WAVEFORM_SQUARE,
			WAVEFORM_SAW_UP,
			WAVEFORM_SAW_DOWN
		};

		// (Single-channel) LFO parameters. 
		// They should be kept by the owner of the LFO module.
		// Note: Gain/amplitude is NOT a parameter of an LFO.
		struct Params
		{
			Waveform 	eWaveform;		// Waveform type.
			AkReal32 	fFrequency;		// LFO frequency (Hz).
			AkReal32	fSmooth;		// Waveform smoothing [0,1].
			AkReal32	fPWM;			// Pulse width modulation (applies to square wave only).
		};


		// ---------------------------------------------------------------------
		// LFO State.
		// Defines the state of a single channel oscillator.
		// ---------------------------------------------------------------------
		class State
		{
		public:
			State()
				: fPhase( 0.f )
				, fPhaseDelta( 0.f )
				, eWaveform( WAVEFORM_SINE )
			{}

			DSP::OnePoleFilter	filter;		// Filter for smoothing.
			AkReal32			fPhase;		// Current phase.
			AkReal32			fPhaseDelta;// Phase increment. Depends on LFO frequency (and also wave type).
			Waveform 			eWaveform;	// Waveform type.
		};

		// ---------------------------------------------------------------------
		// Multichannel specific definitions.
		// ---------------------------------------------------------------------
		namespace MultiChannel
		{
			// Phase spread mode.
			enum PhaseMode
			{
				PHASE_MODE_LEFT_RIGHT = 0,
				PHASE_MODE_FRONT_REAR,
				PHASE_MODE_CIRCULAR,
				PHASE_MODE_RANDOM
			};

			// Multichannel LFO parameters. 
			// These are only definitions. They should be kept by the owner of the LFO module.
			struct PhaseParams
			{
				AkReal32		fPhaseOffset;	// Phase offset (all channels) (degrees [0,360]).
				AkReal32		fPhaseSpread;	// Phase spread (degrees [0,360]).
				PhaseMode		ePhaseMode;		// Phase spread mode.
			};

			// This structure holds all multichannel LFO parameters.
			// Typically, an effect that has a multichannel LFO section should hold this.
			struct AllParams
			{
				Params			lfoParams;		// LFO parameters (common to all LFO channels).
				PhaseParams		phaseParams;	// Phase parameters.
			};

#ifndef __SPU__

			// Helper: Compute initial phase for each channel based on the channel mask and phase parameters.
			void ComputeInitialPhase( 
				AkChannelMask		in_uChannelMask, 
				const PhaseParams & in_phaseParams,
				AkReal32			out_uChannelPhase[]	// Returned array of channel phase offsets. Caller needs to allocate in_uNumChannels*sizeof(AkReal32)
				);

#endif
		}
	}

} // namespace DSP

#endif // _AKLFO_H_

