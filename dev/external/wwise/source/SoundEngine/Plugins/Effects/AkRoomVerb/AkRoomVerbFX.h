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

#ifndef _AK_ROOMVERBFX_H_
#define _AK_ROOMVERBFX_H_

// Advanced algorithm tunings, for those who wish to recompile the plug-in 
// without venturing too far in the source code.

#include "AkRoomVerbFXParams.h"
#include "ReverbState.h"
#include "RandNormalizedFloat.h"

//-----------------------------------------------------------------------------
// Name: class CAkRoomVerbFX
//-----------------------------------------------------------------------------
class CAkRoomVerbFX : public AK::IAkInPlaceEffectPlugin
{
public:

    // Constructor/destructor
    CAkRoomVerbFX();
    ~CAkRoomVerbFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
                    AK::IAkPluginParam * in_pParams,			// Effect parameters.
                    AkAudioFormat &	in_rFormat					// Required audio input format.
					);
    
	// Free memory used by effect and effect termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset
	AKRESULT Reset( );

    // Effect type query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
	void Execute(	AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess
#endif
					);

	// Skips execution of some frames, when the voice is virtual.  Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames) {return AK_DataReady;}	

private:

	// Init helpers
	void SetupDCFilters();
	void ComputeTCCoefs1( );
	void ComputeTCCoefs2( );
	void ComputeTCCoefs3( );
	inline AKRESULT SetupToneControlFilters();
	inline AKRESULT SetupERDelay( AK::IAkPluginMemAlloc * in_pAllocator );
	inline AKRESULT SetupReverbDelay( AK::IAkPluginMemAlloc * in_pAllocator );
	inline AKRESULT SetupERFrontBackDelay( AK::IAkPluginMemAlloc * in_pAllocator, AkChannelMask in_uChannelMask );
	inline AKRESULT SetupERUnit( AK::IAkPluginMemAlloc * in_pAllocator );
	inline AKRESULT SetupFDNs( AK::IAkPluginMemAlloc * in_pAllocator );
	inline AKRESULT SetupDiffusionAPF( AK::IAkPluginMemAlloc * in_pAllocator );
	// Term helpers
	void TermToneControlFilters( AK::IAkPluginMemAlloc * in_pAllocator );
	void TermFDNs( AK::IAkPluginMemAlloc * in_pAllocator );
	void TermDiffusionAPF( AK::IAkPluginMemAlloc * in_pAllocator );
	void TermERUnit( AK::IAkPluginMemAlloc * in_pAllocator );
	// Reset helpers
	void ResetFDNs();
	void ResetDiffusionAPF();
	void ResetDCFilters();
	void ResetToneControlFilters();
	void ResetERUnit();
	// Parameter updates
	bool LiveParametersUpdate( AkAudioBuffer* io_pBuffer );
	void RTPCParametersUpdate( );
	inline AkReal32 ComputeDiffusionAPFGain( AkUInt32 in_UnitNum, AkReal32 in_fDiffusion );
	inline void ChangeDiffusion();
	inline void ChangeDecay();

	// DSP processing routines
#ifndef AK_PS3
	void WetPreProcess( AkAudioBuffer * in_pBuffer, AkReal32 * out_pfWetIn, AkUInt32 in_uNumFrames, AkUInt32 in_uFrameOffset );
	void ReverbPreProcess( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames );
	void ReverbPostProcess( AkReal32 ** out_ppfReverb, AkUInt32 in_uNumOutSignals, AkReal32 in_fGain, AkUInt32 in_uNumFrames );
	AKRESULT ProcessSpread1Out( AkAudioBuffer * io_pBuffer );
	AKRESULT ProcessSpread2Out( AkAudioBuffer * io_pBuffer );
	AKRESULT ProcessSpread3Out( AkAudioBuffer * io_pBuffer );
#ifdef AK_71AUDIO
	AKRESULT ProcessSpread4Out( AkAudioBuffer * io_pBuffer );
#endif
#else
	void ComputeScratchMemoryRequired( AkChannelMask in_uChannelMask );
#endif

private:

	ReverbState				m_Reverb;				
	ReverbUnitState *		m_pReverbUnitsState;
	ToneControlsState *		m_pTCFiltersState;
	DSP::ERUnitDual	*		m_pERUnit;

    CAkRoomVerbFXParams *	m_pParams;	
	AK::IAkPluginMemAlloc * m_pAllocator;

	// Cached parameters to avoid computations when values don't change
	RTPCParams				m_PrevRTPCParams;
	InvariantParams			m_PrevInvariantParams;

#ifdef AK_PS3
		AK::MultiCoreServices::DspProcess	m_DspProcess;
		AkUInt32							m_uScratchBytes; // SPU scratch size necessary
#endif

};

#endif // _AK_ROOMVERBFX_H_