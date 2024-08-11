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

//////////////////////////////////////////////////////////////////////
//
// AkMeterFX.h
//
// Meter processing FX implementation.
//
// Copyright 2010 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AK_METERFX_H_
#define _AK_METERFX_H_

#include "AkMeterFXParams.h"

//-----------------------------------------------------------------------------
// Name: class CAkMeterFX
//-----------------------------------------------------------------------------
class CAkMeterFX : public AK::IAkInPlaceEffectPlugin
{
public:
	CAkMeterFX * pNextItem; // for CAkMeterManager::Meters

    // Constructor/destructor
    CAkMeterFX();
    ~CAkMeterFX();

	// Allocate memory needed by effect and other initializations
    AKRESULT Init(	AK::IAkPluginMemAlloc *	in_pAllocator,		// Memory allocator interface.
					AK::IAkEffectPluginContext * in_pFXCtx,		// FX Context
				    AK::IAkPluginParam * in_pParams,			// Effect parameters.
                    AkAudioFormat &	in_rFormat					// Required audio input format.
				);
    
	// Free memory used by effect and effect termination
	AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset or seek to start (looping).
	AKRESULT Reset( );

    // Effect type query.
    AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

    // Execute effect processing.
    void Execute( 
		AkAudioBuffer * io_pBuffer
#ifdef AK_PS3
		, AK::MultiCoreServices::DspProcess*& out_pDspProcess
#endif
		);

	// Skips execution of some frames, when the voice is virtual.  Nothing special to do for this effect.
	AKRESULT TimeSkip(AkUInt32 in_uFrames) {return AK_DataReady;}	

private:
	friend class CAkMeterManager;

    CAkMeterFXParams * m_pParams;
	AK::IAkEffectPluginContext * m_pCtx;
	AK::IAkPluginMemAlloc * m_pAllocator;
	class CAkMeterManager * m_pMeterManager;

	AkMeterState m_state;

	AkReal32 m_fMin;
	AkUniqueID m_uGameParamID;
	bool m_bTerminated;

#ifdef AK_PS3
	AK::MultiCoreServices::DspProcess m_DspProcess;
#endif
};

#endif // _AK_METERFX_H_