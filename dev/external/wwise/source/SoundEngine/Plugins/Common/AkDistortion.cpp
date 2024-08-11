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

#include "AkDistortion.h"
#include <stdlib.h>
#include <math.h>
#include <AK/SoundEngine/Common/AkFPUtilities.h>
#include <AK/Tools/Common/AkAssert.h>
#include <AkMath.h>

namespace DSP
{

#ifndef __SPU__

CAkDistortion::CAkDistortion():
	m_eDistortionType(DistortionType_None),
	m_fDrive(0.f),
	m_fPrevDrive(0.f),
	m_fDriveGain(0.f),
	m_fPrevDriveGain(0.f),
	m_fTone(0.f),
	m_fPrevTone(0.f)
{

}

void CAkDistortion::SetParameters( DistortionType in_eDistortionType, AkReal32 in_fDrive, AkReal32 in_fTone, bool in_bFirstSet /* = false */ )
{
	AKASSERT( in_fTone >= 0.f && in_fTone <= 100.f );
	AKASSERT( in_fDrive >= 0.f && in_fDrive <= 100.f );
	m_eDistortionType = in_eDistortionType;
	m_fDrive = in_fDrive;
	m_fDriveGain = 4.f*pow(25.f,(in_fDrive/100.f))-3.f; // map (0,100) to (1,97) in non-linear way
	m_fTone = in_fTone;
	if ( in_bFirstSet )	
	{
		// Avoid initial ramping
		m_fPrevDrive = m_fDrive;
		m_fPrevDriveGain = m_fDriveGain; 
		m_fPrevTone = m_fTone;
	}
}

#endif

void CAkDistortion::ProcessBuffer( AkAudioBuffer * io_pBuffer ) 
{
#define APPLY_DISTORTION_PROCESS( __MethodName__ )				\
{																\
	const AkUInt32 uNumChannels = io_pBuffer->NumChannels();	\
	const AkUInt32 uNumFrames = io_pBuffer->uValidFrames;		\
	for ( AkUInt32 i = 0; i < uNumChannels; ++i )				\
	{															\
		AkReal32 * pfChannel = io_pBuffer->GetChannel( i );		\
		(__MethodName__)( pfChannel, uNumFrames );				\
	}															\
}																\

	switch ( m_eDistortionType )
	{
	case DistortionType_Overdrive:
		APPLY_DISTORTION_PROCESS( CAkDistortion::ProcessOverdrive );
		break;
	case DistortionType_Heavy:
		APPLY_DISTORTION_PROCESS( CAkDistortion::ProcessHeavy );
		break;
	case DistortionType_Fuzz:
		APPLY_DISTORTION_PROCESS( CAkDistortion::ProcessFuzz );
		break;
	case DistortionType_Clip:
		APPLY_DISTORTION_PROCESS( CAkDistortion::ProcessClip );
		break;
	default:
		// No distortion
		break;
	}

	// Ramp complete
	m_fPrevDrive = m_fDrive;
	m_fPrevDriveGain = m_fDriveGain; 
	m_fPrevTone = m_fTone;
}

#define INPUTSCALE_DECLARE( __Target__, __Prev__ )							\
	const AkReal32 fSigScaleInc = ((__Target__)-(__Prev__))/in_uNumFrames;	\
	AkReal32 fCurrentSigScale = (__Prev__);									\

#define INPUTSCALE_TICK( __inVar__ )	\
	__inVar__ *= fCurrentSigScale;		\
	fCurrentSigScale += fSigScaleInc;	\

#if defined AKSIMD_FAST_EXP_IMPLEMENTED
#define OUTPUTCOMP_DECLARE()																				\
	const AkReal32 fOutputCompensation = AKSIMD_POW10_SCALAR(-(m_fDrive/(5.f*20.f)));						\
	const AkReal32 fOutputCompensationPrev = AKSIMD_POW10_SCALAR(-(m_fPrevDrive/(5.f*20.f)));				\
	const AkReal32 fOutputCompensationInc = (fOutputCompensation-fOutputCompensationPrev)/in_uNumFrames;	\
	AkReal32 fCurrentOutputCompensation = fOutputCompensationPrev;										
#else
#define OUTPUTCOMP_DECLARE()																				\
	const AkReal32 fOutputCompensation = AkMath::FastPow10(-(m_fDrive/(5.f*20.f)));									\
	const AkReal32 fOutputCompensationPrev = AkMath::FastPow10(-(m_fPrevDrive/(5.f*20.f)));							\
	const AkReal32 fOutputCompensationInc = (fOutputCompensation-fOutputCompensationPrev)/in_uNumFrames;	\
	AkReal32 fCurrentOutputCompensation = fOutputCompensationPrev;										
#endif

#define OUTPUTCOMP_TICK( __outVar__ )						\
	__outVar__ *= fCurrentOutputCompensation;				\
	fCurrentOutputCompensation += fOutputCompensationInc;	\

// Linear in most of the range with symmetrical soft clipping
void CAkDistortion::ProcessOverdrive( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	const AkReal32 fSigScale = m_fDriveGain*(m_fTone/100.f*0.3333333f+0.6666666f);
	const AkReal32 fSigScalePrev = m_fPrevDriveGain*(m_fPrevTone/100.f*0.3333333f+0.6666666f);
	INPUTSCALE_DECLARE( fSigScale, fSigScalePrev );
	OUTPUTCOMP_DECLARE();

	AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
	const AkReal32 * pfEnd = io_pfBuffer + in_uNumFrames;
	while ( pfBuf < pfEnd )
	{	
		AkReal32 fIn = *pfBuf;
		INPUTSCALE_TICK( fIn );
		AkReal32 fAbsIn = fabs(fIn);
		AkReal32 fOut = fIn;

		//// This is probably faster on PC
		//// Note these floating point branch will be very CPU intensive on some platforms...
		//if ( fAbsIn < 0.3333333f )
		//{
		//	// Linear range
		//	fOut = 2.f*fIn;
		//}
		//else if ( fAbsIn < 0.6666666f )
		//{
		//	// Knee range
		//	fOut = (3.f - (2.f-3.f*fAbsIn)*(2.f-3.f*fAbsIn))*0.3333333f;
		//	//if ( fIn <= 0.f )
		//	//	fOut = -fOut;
		//	AK_FPSetValLTE( fIn, 0.f, fOut, -fOut );
		//}
		//else
		//{
		//	fOut = 1.f;
		//	//if ( fIn <= 0.f )
		//	//	fOut = -1.f;
		//	AK_FPSetValLTE( fIn, 0.f, fOut, -1.f );
		//}	

		// Probably faster for platforms other than PC
		AK_FPSetValGT( fAbsIn, 0.6666666f, fOut, 1.f );
		AK_FPSetValLTE( fAbsIn, 0.6666666f, fOut, (3.f - (2.f-3.f*fAbsIn)*(2.f-3.f*fAbsIn))*0.3333333f );
		AK_FPSetValLTE( fAbsIn, 0.3333333f, fOut, 2.f*fAbsIn );
		AK_FPSetValLTE( fIn, 0.f, fOut, -fOut ); // Bring back original signal polarity

		OUTPUTCOMP_TICK( fOut );
		*pfBuf++ = fOut;
	}
}

// Non-linear and symmetrical hard(er) clipping
void CAkDistortion::ProcessHeavy( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	INPUTSCALE_DECLARE( m_fDriveGain, m_fPrevDriveGain );
	OUTPUTCOMP_DECLARE();
	AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
	const AkReal32 * pfEnd = io_pfBuffer + in_uNumFrames;
	while ( pfBuf < pfEnd )
	{	
		AkReal32 fIn = *pfBuf;
		INPUTSCALE_TICK( fIn );
		AkReal32 fNegOnlyIn = -fabs(fIn);
		AkReal32 fOut = 1.f-AkMath::FastExp(fNegOnlyIn);
		/*if ( -fIn <= 0.f )
			fOut = -fOut;*/
		AK_FPSetValLTE( -fIn, 0.f, fOut, -fOut ); // Bring back original signal polarity
		OUTPUTCOMP_TICK( fOut );
		*pfBuf++ = fOut;
	}
}

// Assymetrical (soft+hard) clipping
void CAkDistortion::ProcessFuzz( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	static const AkReal32 fLinearity = -0.2f; // Cannot be == 0
	INPUTSCALE_DECLARE( m_fDriveGain, m_fPrevDriveGain );
	OUTPUTCOMP_DECLARE();
	const AkReal32 fTone = AkMath::FastPow2(m_fTone*0.03f+2.f); // Map (0,100) to (2,5) and then into pow2 to (4,32)	
	const AkReal32 fOffset = fLinearity/(1.f-AkMath::FastExp(fTone*fLinearity));
	const AkReal32 fSingularity = 1.f/fTone + fOffset;
	AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
	const AkReal32 * pfEnd = io_pfBuffer + in_uNumFrames;
	while ( pfBuf < pfEnd )
	{	
		AkReal32 fIn = *pfBuf;
		INPUTSCALE_TICK( fIn );
		// Non-linearity
		AkReal32 fOut;
		if ( fIn != fLinearity )
			fOut = (fIn-fLinearity)/(1.f-AkMath::FastExp(-fTone*(fIn-fLinearity))) + fOffset;
		else
			fOut = fSingularity;
		// Hard clipping
		fOut = AK_FPMin( fOut, 1.f );
		fOut = AK_FPMax( fOut, -1.f );
		OUTPUTCOMP_TICK( fOut );
		*pfBuf++ = fOut;
	}
}

// Symmetrical hard clipping
void CAkDistortion::ProcessClip( AkReal32 * io_pfBuffer, AkUInt32 in_uNumFrames )
{
	INPUTSCALE_DECLARE( m_fDriveGain, m_fPrevDriveGain );
	OUTPUTCOMP_DECLARE();
	AkReal32 * AK_RESTRICT pfBuf = (AkReal32 * AK_RESTRICT) io_pfBuffer;
	const AkReal32 * pfEnd = io_pfBuffer + in_uNumFrames;
	while ( pfBuf < pfEnd )
	{	
		AkReal32 fIn = *pfBuf;
		INPUTSCALE_TICK( fIn );
		AkReal32 fOut = fIn;
		fOut = AK_FPMin( fOut, 1.f );
		fOut = AK_FPMax( fOut, -1.f );
		OUTPUTCOMP_TICK( fOut );
		*pfBuf++ = fOut;
	}
}

} // namespace DSP
