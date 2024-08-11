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

// Picks one of White, Pink, Red or Purple noise generators

#ifndef _AK_COLOREDNOISE_H_
#define _AK_COLOREDNOISE_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include "AkWhiteNoise.h"
#include "AkPinkNoise.h"
#include "AkRedNoise.h"
#include "AkPurpleNoise.h"
#include "../Effects/Common/OnePoleFilter.h"
#include "../Effects/Common/OnePoleZeroHPFilter.h"
#include "../Effects/Common/BiquadFilter.h"

namespace DSP
{
	class  CAkColoredNoise : public CAkWhiteNoise
	{
	public:

		enum AkNoiseColor
		{
			NOISECOLOR_WHITE	= 0,
			NOISECOLOR_PINK,
			NOISECOLOR_RED,
			NOISECOLOR_PURPLE,
		};

#ifndef __SPU__
		void Init( AkNoiseColor in_eNoiseColor, AkUInt32 in_uSampleRate );
#endif

		void GenerateBuffer(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames )
		{
			switch ( m_NoiseColor )
			{
			default:
			case NOISECOLOR_WHITE:
				CAkWhiteNoise::GenerateBuffer( out_pfBuffer, in_uNumFrames );
				break;
			case NOISECOLOR_PINK:
				CAkColoredNoise::GenerateBufferPink( out_pfBuffer, in_uNumFrames );
				break;
			case NOISECOLOR_RED:
				CAkColoredNoise::GenerateBufferRed( out_pfBuffer, in_uNumFrames );
				break;
			case NOISECOLOR_PURPLE:
				CAkColoredNoise::GenerateBufferPurple( out_pfBuffer, in_uNumFrames );
				break;
			}
		}

	protected:

		void GenerateBufferPink(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames );
		void GenerateBufferRed(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames );
		void GenerateBufferPurple(	
			AkReal32 * out_pfBuffer, 
			AkUInt32 in_uNumFrames );

		// Pink
		OnePoleZeroHPFilter		m_DCFilter;		// Remove DC offset
		AkUInt32				m_uIndex;		// Current generator index	
		AkReal32				m_fRunningSum;	// Current running sum
		AkReal32				m_RandGenTable[AKPINKNOISE_NUMROWS];	// Table of random generator output

		// Red
		OnePoleFilter			RedFilter;

		// Purple
		BiquadFilterMono		PurpleFilter;

		AkNoiseColor			m_NoiseColor;
	};
} // namespace DSP


#endif // _AK_COLOREDNOISE_H_
