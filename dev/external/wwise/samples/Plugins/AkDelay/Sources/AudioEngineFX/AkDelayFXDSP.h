//////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DELAYFXDSP_H_
#define _AK_DELAYFXDSP_H_

#include <AK/SoundEngine/Common/AkTypes.h>
#include <AK/Plugin/PluginServices/AkFXTailHandler.h>
#include <AK/SoundEngine/Common/IAkPluginMemAlloc.h>
#include <AK/SoundEngine/Common/AkCommonDefs.h>
#include "AkDelayFXParams.h"
#include <AK/DSP/AkDelayLineMemoryStream.h>

/// Delay FX DSP Processing class
class CAkDelayFXDSP
{
public:
#ifndef __SPU__
	CAkDelayFXDSP();
	~CAkDelayFXDSP();
	void Setup( AkDelayFXParams * pInitialParams, bool in_bIsSendMode, AkUInt32 in_uSampleRate  );
	AKRESULT InitDelay( AK::IAkPluginMemAlloc *	in_pAllocator, AkDelayFXParams * pParams, AkChannelMask in_uChannelMask );
	void ResetDelay( );
	void TermDelay( AK::IAkPluginMemAlloc * in_pAllocator );
	void ComputeTailLength( bool in_bFeedbackEnabled, AkReal32 in_fFeedbackValue );
#endif

#ifdef __PPU__
	AkUInt32 GetPrimeDelayMemory( 
		AkUInt32 in_uMaxFrames, 
		AkReal32 * out_pAddressToDMA[2], 
		AkUInt32 out_uDMASize[2], 
		AkUInt32 & out_uOutputBufferSize );
#endif

#ifdef __SPU__
	void SetupTripleBuffering( AkReal32 * in_pPrimeDataDMABuffer, AkReal32 * in_pOutputBuffer );
#endif

	void Process( 
		AkAudioBuffer * io_pBuffer, 
		AkDelayFXParams * pCurrentParams 
#ifdef __SPU__
		, AkUInt32 in_uDMATag 
#endif
		);

private:

	AK::DSP::CAkDelayLineMemoryStream<AkReal32,AK_VOICE_MAX_NUM_CHANNELS> m_DelayMem;		/// Delay information
	AkFXTailHandler				m_FXTailHandler;			/// FX tail handling services
	AkDelayFXParams				m_PreviousParams;			/// Cache previous parameters for parameter ramps
	AkUInt32					m_uNumProcessedChannels;	/// Number of channels delayed
	AkUInt32					m_uSampleRate;				/// Sample rate
	AkUInt32					m_uTailLength;				/// Cached tail length based on feedback and delay lengths parameters
	bool						m_bSendMode;				/// Effect used in aux send configuration (e.g. environmental effect)
	bool						m_bProcessLFE;				/// Delay or not the LFE channel

} AK_ALIGN_DMA;

#endif // _AK_DELAYFXDSP_H_
