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

#ifndef _AK_RESAMPLINGPHASEVOCODER_H_
#define _AK_RESAMPLINGPHASEVOCODER_H_

#include "AkPhaseVocoder.h"
#include "ResamplingCircularBuffer.h"

namespace DSP
{

namespace BUTTERFLYSET_NAMESPACE
{

	class CAkResamplingPhaseVocoder : public CAkPhaseVocoder
	{
	public:
#ifndef __SPU__
	void ResetInputFill( );
	AKRESULT Init(	
		AK::IAkPluginMemAlloc *	in_pAllocator,	
		AkUInt32				in_uNumChannels,
		AkUInt32				in_uSampleRate,
		AkUInt32				in_uFFTSize,
		bool					in_bUseInputBuffer = false 
		);
	void Term( AK::IAkPluginMemAlloc * in_pAllocator );
	void Reset( );
#endif // #ifndef __SPU__

#ifndef __PPU__
	AKRESULT ProcessPitchChannel( 
		AkReal32 *		in_pfInBuf, 
		AkUInt32		in_uNumFrames,
		bool			in_bNoMoreData,
		AkUInt32		in_uChannelIndex,
		AkReal32 *		io_pfOutBuf,
		AkReal32		in_fResamplingFactor,
		AkReal32 *		in_pfTempStorage
#ifdef __SPU__
		, AkUInt8 *		in_pScratchMem
		, AkUInt32		in_uDMATag
#endif		
		);
#endif // #ifndef __PPU__

#ifdef AK_PS3
	AkUInt32 GetScratchMemRequirement( );
#endif

protected:

		DSP::CAkResamplingCircularBuffer		m_ResamplingInputAccumBuf[AK_VOICE_MAX_NUM_CHANNELS];
		
	};

} // namespace BUTTERFLYSET_NAMESPACE

} // namespace DSP

#endif // _AK_RESAMPLINGPHASEVOCODER_H_
