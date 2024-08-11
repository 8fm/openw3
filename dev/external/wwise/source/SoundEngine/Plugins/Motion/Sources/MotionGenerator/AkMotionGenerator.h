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
// AkMotionGenerator.h
//
// Silence source, pretty straight forward.
// Note: Target output format is currently determined by the source itself.
// Out format currently used is : 48 kHz, Float, Mono
//
// Copyright (c) 2006-2007 Audiokinetic Inc. / All Rights Reserved
//
//////////////////////////////////////////////////////////////////////

#ifndef _AKFXSRC_FEEDBACKGEN_H_
#define _AKFXSRC_FEEDBACKGEN_H_

#include <AK/SoundEngine/Common/IAkPlugin.h>
#include <AK/Plugin/AkMotionGeneratorFactory.h>

#include "..\..\..\..\AkAudioLib\Common\AkConversionTable.h"

class AkCurve : public CAkConversionTable<AkRTPCGraphPoint, AkReal32>
{
};

// Parameters IDs. To be used by game for RTPC.
//const AkPluginParamID AK_SRCSILENCE_FXPARAM_DUR_ID			= 0;
//...

const AkPluginParamID AK_Period_Param			= 0;
const AkPluginParamID AK_PeriodMultiplier_Param	= 1;
const AkPluginParamID AK_DurationType_Param		= 2;
const AkPluginParamID AK_Duration_Param			= 3;
const AkPluginParamID AK_AttackTime_Param		= 12;
const AkPluginParamID AK_DecayTime_Param		= 13;
const AkPluginParamID AK_SustainTime_Param		= 14;
const AkPluginParamID AK_SustainLevel_Param		= 15;
const AkPluginParamID AK_ReleaseTime_Param		= 16;
const AkPluginParamID AK_WiiPitch_Param			= 1000;		//This is necessary to support the pitch feature on the Wii.
const AkPluginParamID AK_NUM_FEEDBACKGENERATOR_FXPARAM	= 10;

#if defined AK_WII_FAMILY
const AkPluginParamID AK_NUM_CURVES				= 1; 
#elif defined AK_XBOXONE || defined AK_WIN	//AK_WIN must have the maximum curves to be able to test in the Authoring.
const AkPluginParamID AK_NUM_CURVES				= 4; 
#else
const AkPluginParamID AK_NUM_CURVES				= 2; 
#endif

// Parameters structure for this effect.
// The effect implementor has the task of defining its parameter's structure.
struct AkMotionGeneratorParams
{
	~AkMotionGeneratorParams()
	{
		for(AkUInt16 i = 0; i < AK_NUM_CURVES; i++)
			m_Curves[i].Set(NULL, 0, AkCurveScaling_None);
	};

	// This Function return the fixed size of the structure, it doesn't contain dynamic memory. (no pointer)
	AkUInt32 GetFixedStructureSize()
	{
		return 	sizeof(m_fPeriod) +
				sizeof(m_fPeriodMultiplier) +
				sizeof(m_fDuration) +
				sizeof(m_fAttackTime) +
				sizeof(m_fDecayTime) +
				sizeof(m_fSustainTime) +
				sizeof(m_fReleaseTime) +
				sizeof(m_fSustainLevel) +
				sizeof(m_eDurationType);
	}

	//Keep this in order!  There is a memcpy that copies this block.
	AkReal32	m_fPeriod;
	AkReal32	m_fPeriodMultiplier;
	AkReal32	m_fDuration;
	AkReal32	m_fAttackTime;
	AkReal32	m_fDecayTime;
	AkReal32	m_fSustainTime;
	AkReal32	m_fReleaseTime;
	AkReal32	m_fSustainLevel;
	AkUInt16	m_eDurationType;

	//Not in the banks.
#if defined AK_WII_FAMILY
	AkReal32	m_fWiiPitch;
#endif

	AkCurve		m_Curves[AK_NUM_CURVES];
	AkUInt16	m_uCurves;
};

//-----------------------------------------------------------------------------
// Name: class CAkMotionGeneratorParams
// Desc: Implementation the silence source shared parameters.
//-----------------------------------------------------------------------------
class CAkMotionGeneratorParams : public AK::IAkPluginParam
{
public:

    // Plugin mechanism. Implement Create function and register its address to the FX manager.
	static AK::IAkPluginParam * CreateEffectParam( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor.
    CAkMotionGeneratorParams(AK::IAkPluginMemAlloc * in_pAllocator);
    CAkMotionGeneratorParams( const CAkMotionGeneratorParams &Copy );
    ~CAkMotionGeneratorParams();

    // Create duplicate.
	virtual AK::IAkPluginParam * Clone( AK::IAkPluginMemAlloc * in_pAllocator );

    // Init/Term.
	virtual AKRESULT Init( AK::IAkPluginMemAlloc *	in_pAllocator,						   
                           const void *				in_pvParamsBlock, 
                           AkUInt32					in_ulBlockSize 
                         );
	virtual AKRESULT Term( AK::IAkPluginMemAlloc * in_pAllocator );

    // Set all parameters at once.
    virtual AKRESULT SetParamsBlock( const void *	in_pvParamsBlock, 
                                     AkUInt32		in_ulBlockSize
                                   );

    // Update one parameter.
	virtual AKRESULT SetParam( AkPluginParamID in_ParamID,
                               const void * in_pvValue, 
                               AkUInt32 in_ulParamSize
                             );

	inline AkMotionGeneratorParams& GetParams() {return m_Params;}
private:
	AKRESULT ReadCurve(AkUInt16 in_iIndex, const void** io_ppData);
	AKRESULT ReadAllCurves(const void** io_ppData);

    // Parameter structure.
    AkMotionGeneratorParams m_Params;
	AK::IAkPluginMemAlloc * m_pAllocator;
};

//-----------------------------------------------------------------------------
// Name: class CAkMotionGenerator
// Desc: Implementation of a generator for rumble pads
//-----------------------------------------------------------------------------
class CAkMotionGenerator : public AK::IAkSourcePlugin
{
public:

	// Plugin mechanism. Implement Create function and register its address to the FX manager.
	static IAkPlugin* CreateEffect( AK::IAkPluginMemAlloc * in_pAllocator );

    // Constructor/destructor
    CAkMotionGenerator();
    virtual ~CAkMotionGenerator(); 

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

    // Execute effect processing.
	virtual void Execute( 
				AkAudioBuffer * io_pBufferOut // Output buffer interface
#ifdef AK_PS3
				, AK::MultiCoreServices::DspProcess*&	out_pDspProcess	///< Asynchronous DSP process utilities on PS3
#endif 
				);

	virtual AkReal32 GetDuration() const;
	virtual AKRESULT StopLooping();

	// Returns a random float value between in_fMin and in_fMax
	inline AkReal32 RandRange( AkReal32 in_fMin, AkReal32 in_fMax );
private:

	AkReal32 ComputeDuration();

    // Internal state variables.
    AkUInt32			m_uSampleRate;			
	AkUInt32			m_uBytesPerSample;
	AkReal32			m_fTime;				//Current time in the graph
	AkUInt32			m_uDuration;			//Duration of the source (without loops)
	AkUInt32			m_uSamplesProduced;		//Samples produced up to now (always less than m_uDuration)
	AkUInt32			m_uLoops;				//Loops to complete
	AkReal32			m_fVol;					//Current volume (for ADSR)
	AkUInt32			m_iCurvePoint[2];		//Current point in the curve.

	//ADSR progress variables
	struct ADSRState
	{
		AkInt32			m_iNextSection;			//Sample index where we must go to the next section of ADSR
		AkReal32		m_fStep;				//Volume step we must add at each sample
		AkReal32		m_fStartVol;			//Volume at the begining of this section
	};
	ADSRState			m_ADSRState[4];			//The 4 sections of ADSR
	AkUInt8				m_uSection;				//The current section

    // Shared parameters structure
    CAkMotionGeneratorParams * m_pSharedParams;

	// Source FX context interface
	AK::IAkSourcePluginContext * m_pSourceFXContext;
};

#endif  //_AKFXSRC_FEEDBACKGEN_H_
