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

#ifndef _AKFXSRC_SILENCE_H_
#define _AKFXSRC_SILENCE_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Plugin/AkSilenceSourceFactory.h>
#include <AK/Tools/Common/AkAssert.h>

// Parameters struture for this effect.
// The effect implementor has the task of defining its parameter's structure.
struct AkFXSrcSilenceParams
{
    AkReal32     fDuration;				// Duration (secs).
    AkReal32     fRandomizedLengthMinus; // Maximum randomness to subtract to duration (secs) 
	AkReal32     fRandomizedLengthPlus;  // Maximum randomness to add to duration (secs) 
};

// Parameters IDs. To be used by game for RTPC.
const AkPluginParamID AK_SRCSILENCE_FXPARAM_DUR_ID			= 0;
const AkPluginParamID AK_SRCSILENCE_FXPARAM_RANDMINUS_ID	= 1;
const AkPluginParamID AK_SRCSILENCE_FXPARAM_RANDPLUS_ID		= 2;

const AkPluginParamID AK_NUM_SRCSILENCE_FXPARAM				= 3;

#define SILENCE_DURATION_MIN	(0.001f)
#define SILENCE_DURATION_MAX	(3600.f)
#define SILENCE_RANDOMMIN_MIN	(-SILENCE_DURATION_MAX)
#define SILENCE_RANDOMMIN_MAX	(0.f)
#define SILENCE_RANDOMMAX_MIN	(0.f)
#define SILENCE_RANDOMMAX_MAX	(SILENCE_DURATION_MAX)

//-----------------------------------------------------------------------------
// Name: class CAkFXSrcSilenceParams
// Desc: Implementation the silence source shared parameters.
//-----------------------------------------------------------------------------
class CAkFxSrcSilenceParams : public AK::IAkPluginParam
{
public:

    // Plugin mechanism. Implement Create function and register its address to the FX manager.
	static AK::IAkPluginParam * CreateEffectParam( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor.
    CAkFxSrcSilenceParams();
    CAkFxSrcSilenceParams( const CAkFxSrcSilenceParams &Copy );
    virtual ~CAkFxSrcSilenceParams();

    // Create duplicate.
	virtual AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
	virtual AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						   
                           const void *				in_pvParamsBlock, 
                           AkUInt32				in_ulBlockSize 
                         );
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    virtual AKRESULT SetParamsBlock( const void * in_pvParamsBlock, 
                                     AkUInt32 in_ulBlockSize
                                   );

    // Update one parameter.
	virtual AKRESULT SetParam( AkPluginParamID in_ParamID,
                               const void * in_pvValue, 
                               AkUInt32 in_ulParamSize
                             );

    //
    // Silence internal API
    //
    AkReal32 GetDuration( );
    AkReal32 GetRandomMinus( );
	AkReal32 GetRandomPlus( );

private:

    // Parameter structure.
    AkFXSrcSilenceParams m_Params;
};

//-----------------------------------------------------------------------------
// Name: class CAkFXSrcSilence
// Desc: Implementation of a silence source.
//-----------------------------------------------------------------------------
class CAkFXSrcSilence : public AK::IAkSourcePlugin
{
public:

	// Plugin mechanism. Implement Create function and register its address to the FX manager.
	static IAkPlugin* CreateEffect( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor
    CAkFXSrcSilence();
    virtual ~CAkFXSrcSilence(); 

    // Initialize
	virtual AKRESULT Init(	AK::IAkPluginMemAlloc *			in_pAllocator,		// Memory allocator interface.
							AK::IAkSourcePluginContext *	in_pSourceFXContext,// Source FX context
							AK::IAkPluginParam *			in_pParams,			// Effect parameters.
							AkAudioFormat &					io_rFormat			// Supported audio output format.
                         );

    // Terminate
    // NOTE: The effect must DELETE ITSELF herein.
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

	// Reset or seek to start (looping).
	virtual AKRESULT Reset( );

    // Info query:
    // Effect type (source, monadic, mixer, ...)
    // Buffer type: in-place, input(?), output (source), io.
    virtual AKRESULT GetPluginInfo( AkPluginInfo & out_rPluginInfo );

	/// Skip processing of some frames when voice is virtual from elapsed time.
	virtual AKRESULT TimeSkip(AkUInt32 &io_uFrames );

    // Execute effect processing.
	virtual void Execute( AkAudioBuffer *							io_pBufferOut	// Output buffer interface.
#ifdef AK_PS3				
					, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	// the process that needs to run
#endif
					);

	virtual AkReal32 GetDuration() const;
	virtual AKRESULT StopLooping();

	virtual AKRESULT Seek( AkUInt32 in_uPosition );

private:

	// Returns a random float value between in_fMin and in_fMax
	inline AkReal32 RandRange( AkReal32 in_fMin, AkReal32 in_fMax );

    // Internal state variables.
    AkUInt32			m_uSampleRate;
	AkUInt32			m_uBytesPerSample;
	AkUInt32			m_ulOutByteCount;
	AkReal32			m_fDurationModifier;
	AkInt16             m_sNumLoops;
    AkInt16             m_sCurLoopCount;
	AkReal32			m_fInitDuration;

    // Shared parameters structure
    CAkFxSrcSilenceParams * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;

};

// Safely retrieve duration.
inline AkReal32 CAkFxSrcSilenceParams::GetDuration( )
{
    AkReal32 fDuration = m_Params.fDuration;
	AKASSERT( fDuration >= SILENCE_DURATION_MIN && fDuration <= SILENCE_DURATION_MAX );
    return fDuration;
}

// Safely retrieve plus randomness.
inline AkReal32 CAkFxSrcSilenceParams::GetRandomPlus( )
{
    AkReal32 fRandomPlus = m_Params.fRandomizedLengthPlus;
	AKASSERT( fRandomPlus >= SILENCE_RANDOMMAX_MIN && fRandomPlus <= SILENCE_RANDOMMAX_MAX );
    return fRandomPlus;
}

// Safely retrieve minus randomness.
inline AkReal32 CAkFxSrcSilenceParams::GetRandomMinus( )
{
    AkReal32 fRandomMinus = m_Params.fRandomizedLengthMinus;
	AKASSERT( fRandomMinus >= SILENCE_RANDOMMIN_MIN && fRandomMinus <= SILENCE_RANDOMMIN_MAX );
    return fRandomMinus;
}

#endif  //_AKFXSRC_SILENCE_H_
