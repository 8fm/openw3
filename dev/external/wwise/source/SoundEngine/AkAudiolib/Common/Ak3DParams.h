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
// Ak3DParams.h
//
//////////////////////////////////////////////////////////////////////
#ifndef _3D_PARAMETERS_H_
#define _3D_PARAMETERS_H_

#include "AkLEngineDefs.h"		
#include "AudiolibLimitations.h"
#include "AkMath.h"
#include <AK/Tools/Common/AkArray.h>

// Forward definition.
class AkSoundPositionRef;
class CAkParameterNode;

struct ConeParams
{
	AkReal32		fInsideAngle;					// radians
	AkReal32		fOutsideAngle;					// radians
	AkReal32		fOutsideVolume;
	AkLPFType		LoPass;
};

// Extends emitter-listener pair with flags permitting partial fill up.
class AkEmitterListenerPairEx : public AkEmitterListenerPair
{
public:
	AkEmitterListenerPairEx() 
		: m_bIsEmitterAngleSet( false )
		, m_bArePositionAnglesSet( false )
		, m_uListenerMask( 0 )
	{}

	// Extended services.
	// Get distance project on the horizontal plane.
	AkForceInline AkReal32 ProjectedDistanceOnPlane() const { return r * cosf( phi ); }

	// Partial features management.
	AkForceInline bool ArePositionAnglesSet() const { return m_bArePositionAnglesSet; }
	AkForceInline bool IsEmitterAngleSet() const { return m_bIsEmitterAngleSet; }
	
	// Setters.
	AkForceInline void SetDistance( AkReal32 in_fDistance ) { r = in_fDistance; }
	AkForceInline void SetAngles( AkReal32 in_fAzimuth, AkReal32 in_fElevation ) { theta = in_fAzimuth; phi = in_fElevation; m_bArePositionAnglesSet = true; }
	AkForceInline void SetEmitterAngle( AkReal32 in_fAngle ) { fEmitterAngle = in_fAngle; m_bIsEmitterAngleSet = true; }

	void Copy( const AkEmitterListenerPairEx & in_emitterListenerPair )
	{
		r = in_emitterListenerPair.r;
		theta = in_emitterListenerPair.theta;
		phi = in_emitterListenerPair.phi;
		fEmitterAngle = in_emitterListenerPair.fEmitterAngle;
		m_bIsEmitterAngleSet = in_emitterListenerPair.m_bIsEmitterAngleSet;
		m_bArePositionAnglesSet = in_emitterListenerPair.m_bArePositionAnglesSet;
		m_uListenerMask = in_emitterListenerPair.m_uListenerMask;
	}
	
	void CopyTo( AkEmitterListenerPair & out_emitterListenerPair )
	{
		out_emitterListenerPair.r = r;
		out_emitterListenerPair.theta = theta;
		out_emitterListenerPair.phi = phi;
		out_emitterListenerPair.fEmitterAngle = fEmitterAngle;
	}

protected:
	AkUInt8	m_bIsEmitterAngleSet :1;
	AkUInt8	m_bArePositionAnglesSet :1;
	AkUInt8 m_uListenerMask;	// Listener mask (in the 3D case, only one bit should be set).
};

typedef AkArray<AkEmitterListenerPairEx, const AkEmitterListenerPairEx &, ArrayPoolDefault> AkEmitterListenerPairArray;

// Volume data associated to a ray (emitter-listener pair).
class AkRayVolumeData : public AkEmitterListenerPairEx
{
public:
	AkRayVolumeData()
		: fDryMixGain( 1.f )
		, fGameDefAuxMixGain( 1.f )
		, fUserDefAuxMixGain( 1.f )
		, fConeInterp( 1.f )
	{}

	AkForceInline AkUInt32 ListenerMask() const { return m_uListenerMask; }
	// Get listener index of one and only listener in mask (should not be used if there are more than 1).
	inline AkUInt8 ListenerIdx() const 
	{ 
		/// TODO PERF optimized ffs
		AKASSERT( AK::GetNumChannels( m_uListenerMask ) == 1 ); 
		AkUInt8 uListenerIdx = 0;
		AkUInt8 uListenerMask = m_uListenerMask;
		while ( !( uListenerMask & 0x01 ) )
		{
			uListenerMask >>= 1;
			++uListenerIdx;
		}
		return uListenerIdx; 
	}
	AkForceInline void SetListenerMask( AkUInt8 in_uListenerMask ) { m_uListenerMask = in_uListenerMask; }

public:
	AkReal32	fDryMixGain;
	AkReal32	fGameDefAuxMixGain;
	AkReal32	fUserDefAuxMixGain;
	AkReal32	fConeInterp;	// 0-1
};

typedef AkArray<AkRayVolumeData, const AkRayVolumeData &, ArrayPoolDefault> AkVolumeDataArray;

// NOTE: Definition of the following structures here in this file is questionable.

// HDR
struct AkHDRParams
{
	AkVolumeValue		fActiveRange;		// dB
	AkUInt8				bHdrServiced		:1;
	AkUInt8 			bEnableEnvelope		:1;

	void Clear()
	{
		fActiveRange		= AK_DEFAULT_LEVEL_DB;
		bHdrServiced		= false;
		bEnableEnvelope		= false;
	}
};

struct AkNormalizationParams
{
	AkVolumeValue		fMakeUpGain;		// dB
	AkUInt8 /*bool*/	bNormalizeLoudness	:1;
	AkUInt8				bServiced	:1;		// Autonormalization is exclusive/overridable.

	void Clear()
	{
		fMakeUpGain			= AK_DEFAULT_LEVEL_DB;
		bNormalizeLoudness	= false;
		bServiced			= false;	
	}
};

struct AkSoundParams
{
	AkVolumeValue		Volume;			// dBs, no limit
	AkReal32			fFadeRatio;		// [0,1]
	AkPitchValue		Pitch;
	AkLPFType			LPF;
	AkVolumeValue		BusVolume;	// dBs, no limit
	AkNormalizationParams	normalization;
	AkHDRParams			hdr;

	void Clear()
	{
		Volume		= AK_DEFAULT_LEVEL_DB;
		fFadeRatio	= AK_UNMUTED_RATIO;
		Pitch		= AK_DEFAULT_PITCH;
		LPF			= AK_DEFAULT_LOPASS_VALUE;
		BusVolume	= AK_DEFAULT_LEVEL_DB;
		normalization.Clear();
		hdr.Clear();
	}
};

struct AkSoundParamsEx : public AkSoundParams
{
	// Bus Out
	AkReal32 fOutputBusVolume;
    AkReal32 fOutputBusLPF;

	// User Defined
	AkReal32 aUserAuxSendVolume[AK_NUM_AUX_SEND_PER_OBJ];
	AkUniqueID aAuxSend[AK_NUM_AUX_SEND_PER_OBJ];

	// Game Defined
	AkReal32 fGameAuxSendVolume;
	bool bGameDefinedAuxEnabled;

	// Only used in callstack
	bool bGameDefinedServiced;
	bool bUserDefinedServiced;

	void ClearEx()
	{
		Clear();

		fOutputBusVolume = 0;
		fOutputBusLPF = 0;

		for( int i = 0; i < AK_NUM_AUX_SEND_PER_OBJ; ++i )
		{
			aUserAuxSendVolume[i] = 0.0f;
			aAuxSend[i] = AK_INVALID_UNIQUE_ID;
		}

		fGameAuxSendVolume = 0;
		bGameDefinedAuxEnabled = false;

		bGameDefinedServiced = false;
		bUserDefinedServiced = false;
	}
};

#endif //_3D_PARAMETERS_H_
