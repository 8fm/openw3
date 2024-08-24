//////////////////////////////////////////////////////////////////////
//
// AkDelayFX.h
//
// Sample delay FX implementation.
//
// Copyright (c) 2006 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_DELAYFX_H_
#define _AK_DELAYFX_H_

#include "AkDelayFXParams.h"
#include "AkDelayFXDSP.h"

//-----------------------------------------------------------------------------
// Name: class CAkDelayFX
//-----------------------------------------------------------------------------
class CAkDelayFX : public AK::IAkInPlaceEffectPlugin
{
public:

    /// Constructor
    CAkDelayFX();

	 /// Destructor
    ~CAkDelayFX();

	/// Effect plug-in initialization
    AKRESULT Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		/// Memory allocator interface.
					AK::IAkEffectPluginContext *	in_pFXCtx,			/// Sound engine plug-in execution context.
                    AK::IAkPluginParam *			in_pParams,			/// Associated effect parameters node.
                    AkAudioFormat &					in_rFormat			/// Input/output audio format.
                    );
    
	/// Effect plug-in termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	/// Reset effect
	AKRESULT Reset( );

    /// Effect info query
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    /// Effect plug-in DSP processing
	void Execute(	AkAudioBuffer * io_pBuffer								/// Input/Output audio buffer structure.		
#ifdef AK_PS3
					, AK::MultiCoreServices::DspProcess*& out_pDspProcess	/// SPU job setup information.
#endif
		);

	/// Execution processing when the voice is virtual. Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames){ return AK_DataReady; }	

private:

#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess	m_DspProcess;	/// DSPProcess SPU job
#endif

	/// Cached information
	CAkDelayFXDSP			m_FXState;		/// Internal effect state
    CAkDelayFXParams *		m_pParams;		/// Effect parameter node
	AK::IAkPluginMemAlloc * m_pAllocator;	/// Memory allocator interface
};

#endif // _AK_DELAYFX_H_
