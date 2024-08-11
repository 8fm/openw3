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
// AkMixerSIMD.cpp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "AkMixer.h"
#include <AK/SoundEngine/Common/AkSimd.h>
#include "AudiolibDefs.h"

// Number of floats in vectors. Implementations that use v2f32 (2 floats) also use this value 
// as they are unrolled twice as much.
static const AkUInt32	ulVectorSize = 4;

AK_ALIGN_SIMD( static ) AkReal32 aVolumes[4];

#define BUILD_VOLUME_VECTOR( VECT, VOLUME, VOLUME_DELTA ) \
	aVolumes[0] = VOLUME; \
	aVolumes[1] = VOLUME + VOLUME_DELTA; \
	aVolumes[2] = VOLUME + 2.f * VOLUME_DELTA; \
	aVolumes[3] = VOLUME + 3.f * VOLUME_DELTA; \
	AKSIMD_V4F32 VECT = AKSIMD_LOAD_V4F32( aVolumes );

// Assumes in_uNumSamples % 8 == 0
void CAkMixer::MixChannelSIMD( 
	AkReal32*	in_pSourceData, 
	AkReal32*	in_pDestData, 
	AkReal32	in_fVolume, 
	AkReal32	in_fVolumeDelta,
	AkUInt32	in_uNumSamples
	)
{
	AkReal32* AK_RESTRICT pSourceData = in_pSourceData;
	AkReal32* AK_RESTRICT pDestData = in_pDestData;
	AkReal32* AK_RESTRICT pSourceEnd = pSourceData + in_uNumSamples;

	if ( in_fVolumeDelta == 0.0f )
	{
		if ( in_fVolume == 0.0f )
		{
			//everything is done over a previous cleared buffer, so if volume=0 we have nothing to do
			return;
		}

#ifdef AKSIMD_V2F32_SUPPORTED

		AKSIMD_BUILD_V2F32( const AKSIMD_V2F32 vVolumes, in_fVolume, in_fVolume );

		do
		{
			AKASSERT( !( in_uNumSamples % 4 ) );
			// get eight samples								 
			AKSIMD_V2F32 vIn1 = AKSIMD_LOAD_V2F32_OFFSET( pSourceData, 0 );
			AKSIMD_V2F32 vIn2 = AKSIMD_LOAD_V2F32_OFFSET( pSourceData, 8 );
			
			pSourceData += ulVectorSize;

			// get the previous ones						
			AKSIMD_V2F32 vOut1 = AKSIMD_LOAD_V2F32_OFFSET( pDestData, 0 );
			AKSIMD_V2F32 vOut2 = AKSIMD_LOAD_V2F32_OFFSET( pDestData, 8 );

			// apply volume and mix
			vOut1 = AKSIMD_MADD_V2F32( vIn1, vVolumes, vOut1);
			vOut2 = AKSIMD_MADD_V2F32( vIn2, vVolumes, vOut2);
			
			// store the result								 
			AKSIMD_STORE_V2F32_OFFSET( pDestData, 0, vOut1 );
			AKSIMD_STORE_V2F32_OFFSET( pDestData, 8, vOut2 );

			pDestData += ulVectorSize;

		}
		while ( pSourceData < pSourceEnd );
#else

		AKASSERT( !( in_uNumSamples % 8 ) );
		BUILD_VOLUME_VECTOR( vVolumes, in_fVolume, 0.0f );

		do
		{
			// get eight samples								 
			AKSIMD_V4F32 vSum1 = AKSIMD_LOAD_V4F32( pSourceData );
			AKSIMD_V4F32 vSum2 = AKSIMD_LOAD_V4F32( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;

			// apply volume
			vSum1 = AKSIMD_MUL_V4F32( vSum1, vVolumes );
			vSum2 = AKSIMD_MUL_V4F32( vSum2, vVolumes );
			
			// get the previous ones						 
			AKSIMD_V4F32 vOutput1 = AKSIMD_LOAD_V4F32( pDestData );
			AKSIMD_V4F32 vOutput2 = AKSIMD_LOAD_V4F32( pDestData  + ulVectorSize );
			
			// add to output sample							 
			vSum1 = AKSIMD_ADD_V4F32( vOutput1, vSum1 );
			vSum2 = AKSIMD_ADD_V4F32( vOutput2, vSum2 );
			
			// store the result								 
			AKSIMD_STORE_V4F32( pDestData, vSum1 );
			AKSIMD_STORE_V4F32( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;
		}
		while ( pSourceData < pSourceEnd );
#endif
	}

	else // has volume delta
	{
#ifdef AKSIMD_V2F32_SUPPORTED
		AKASSERT( !( in_uNumSamples % 4 ) );
		AkReal32 fVolumesDelta = in_fVolumeDelta * 2;
		AKSIMD_V2F32 vVolumesDelta = AKSIMD_SET_V2F32( fVolumesDelta );
		AKSIMD_BUILD_V2F32( AKSIMD_V2F32 vVolumes1, in_fVolume, in_fVolume + in_fVolumeDelta );
		AKSIMD_V2F32 vVolumes2 = AKSIMD_ADD_V2F32( vVolumes1, vVolumesDelta );
		
		// multiply volumes delta by 2 because the loop is unrolled by 2.
		AKSIMD_BUILD_V2F32( const AKSIMD_V2F32 vUnrollFactor, 2.f, 2.f );
		vVolumesDelta = AKSIMD_MUL_V2F32( vVolumesDelta, vUnrollFactor );

		do
		{
			// get eight samples								 
			AKSIMD_V2F32 vIn1 = AKSIMD_LOAD_V2F32_OFFSET( pSourceData, 0 );
			AKSIMD_V2F32 vIn2 = AKSIMD_LOAD_V2F32_OFFSET( pSourceData, 8 );
			
			pSourceData += ulVectorSize;

			// get the previous ones						
			AKSIMD_V2F32 vOut1 = AKSIMD_LOAD_V2F32_OFFSET( pDestData, 0 );
			AKSIMD_V2F32 vOut2 = AKSIMD_LOAD_V2F32_OFFSET( pDestData, 8 );

			// apply volume and mix
			vOut1 = AKSIMD_MADD_V2F32( vIn1, vVolumes1, vOut1);
			vOut2 = AKSIMD_MADD_V2F32( vIn2, vVolumes2, vOut2);
			
			// store the result								 
			AKSIMD_STORE_V2F32_OFFSET( pDestData, 0, vOut1 );
			AKSIMD_STORE_V2F32_OFFSET( pDestData, 8, vOut2 );

			pDestData += ulVectorSize;

			// in_fVolume += in_fVolumeDelta;
			vVolumes1 = AKSIMD_ADD_V2F32( vVolumes1, vVolumesDelta );
			vVolumes2 = AKSIMD_ADD_V2F32( vVolumes2, vVolumesDelta );
		}
		while ( pSourceData < pSourceEnd );

#else
		AKASSERT( !( in_uNumSamples % 8 ) );
		AkReal32 fVolumesDelta = in_fVolumeDelta * 4;
		AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

		BUILD_VOLUME_VECTOR( vVolumes1, in_fVolume, in_fVolumeDelta );
		AKSIMD_V4F32 vVolumes2 = AKSIMD_ADD_V4F32( vVolumes1, vVolumesDelta );

		// multiply volumes delta by 2 because the loop is unrolled by 2.
		vVolumesDelta = AKSIMD_ADD_V4F32( vVolumesDelta, vVolumesDelta );

		do
		{
			// get eight samples								 
			AKSIMD_V4F32 vSum1 = AKSIMD_LOAD_V4F32( pSourceData );
			AKSIMD_V4F32 vSum2 = AKSIMD_LOAD_V4F32( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;

			// apply volume
			vSum1 = AKSIMD_MUL_V4F32( vSum1, vVolumes1 );
			vSum2 = AKSIMD_MUL_V4F32( vSum2, vVolumes2 );
			
			// get the previous ones						 
			AKSIMD_V4F32 vOutput1 = AKSIMD_LOAD_V4F32( pDestData );
			AKSIMD_V4F32 vOutput2 = AKSIMD_LOAD_V4F32( pDestData  + ulVectorSize );
			
			// add to output sample							 
			vSum1 = AKSIMD_ADD_V4F32( vOutput1, vSum1 );
			vSum2 = AKSIMD_ADD_V4F32( vOutput2, vSum2 );
			
			// store the result								 
			AKSIMD_STORE_V4F32( pDestData, vSum1 );
			AKSIMD_STORE_V4F32( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;

			// in_fVolume += in_fVolumeDelta;
			vVolumes1 = AKSIMD_ADD_V4F32( vVolumes1, vVolumesDelta );
			vVolumes2 = AKSIMD_ADD_V4F32( vVolumes2, vVolumesDelta );
		}
		while ( pSourceData < pSourceEnd );

#endif

	}
}

#ifdef AK_ANDROID
inline void CAkMixer::VolumeInterleavedStereo(
		AkAudioBuffer*		in_pSource,
		AkInt16*			in_pDestData,
		AkSpeakerVolumesN	in_Volumes
		)
{

	AKSIMD_V4F32 mMul = AKSIMD_SET_V4F32( AUDIOSAMPLE_SHORT_MAX ); // duplicate multiplier factor 4x
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataL = in_pSource->GetChannel( AK_IDX_SETUP_2_LEFT );
	AkReal32* l_pSourceDataR = in_pSource->GetChannel( AK_IDX_SETUP_2_RIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples
		AKSIMD_V4F32 vVectL = AKSIMD_LOAD_V4F32( l_pSourceDataL );
		l_pSourceDataL += ulVectorSize;

		// apply volume
		vVectL = AKSIMD_MUL_V4F32( vVectL, vVolumes );

		AKSIMD_V4F32 vVectR = AKSIMD_LOAD_V4F32( l_pSourceDataR );
		l_pSourceDataR += ulVectorSize;

		// apply volume
		vVectR = AKSIMD_MUL_V4F32( vVectR, vVolumes );

		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes, vVolumesDelta );

//////////////////////////////////////////////////////////////

		// vVectL = [ L3, L2, L1, L0] * volumes
		// vVectR = [ R3, R2, R1, R0] * volumes
		// we want vTmpFloat1 = [ R1, L1, R0, L0]
		AKSIMD_V4F32 vTmpFloat1 = AKSIMD_UNPACKLO_V4F32( vVectL, vVectR );

		// we want vTmpFloat2 = [ R3, L3, R2, L2]
		AKSIMD_V4F32 vTmpFloat2 = AKSIMD_UNPACKHI_V4F32( vVectL, vVectR );

		// Convert to samples to signed int
		AKSIMD_V4F32 mTmp1 = AKSIMD_MUL_V4F32( vTmpFloat1, mMul );
		AKSIMD_V4F32 mTmp2 = AKSIMD_MUL_V4F32( vTmpFloat2, mMul );

		// Convert from Float32 -> Int32 -> Int16
		AKSIMD_V4I32 mShorts = AKSIMD_PACKS_V4I32(AKSIMD_CONVERT_V4F32_TO_V4I32(mTmp1), AKSIMD_CONVERT_V4F32_TO_V4I32(mTmp2));
		// Store the data
		AKSIMD_STOREU_V4I32((AKSIMD_V4I32*)in_pDestData, mShorts);

		in_pDestData += ulVectorSize * 2;
	}
}
#endif

//====================================================================================================
//====================================================================================================

#ifdef AKSIMD_V2F32_SUPPORTED
inline void CAkMixer::VolumeInterleavedStereo( 
	AkAudioBuffer*		in_pSource, 
	AkReal32*			in_pDestData, 
	AkSpeakerVolumesN	in_Volumes
	)
{
	AKSIMD_BUILD_V2F32( const AKSIMD_V2F32 vUnrollFactor, 2.f, 2.f );

	AkReal32 fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V2F32 vVolumesDelta = AKSIMD_SET_V2F32( fVolumesDelta );
	AKSIMD_BUILD_V2F32( AKSIMD_V2F32 vVolumes1, in_Volumes.fVolume, in_Volumes.fVolume + in_Volumes.fVolumeDelta );
	AKSIMD_V2F32 vVolumes2 = AKSIMD_ADD_V2F32( vVolumes1, vVolumesDelta );	
	vVolumesDelta = AKSIMD_MUL_V2F32( vVolumesDelta, vUnrollFactor );

	AkReal32 * AK_RESTRICT l_pSourceDataL = in_pSource->GetChannel( AK_IDX_SETUP_2_LEFT );
	AkReal32 * AK_RESTRICT l_pSourceDataR = in_pSource->GetChannel( AK_IDX_SETUP_2_RIGHT );
	const AkReal32 * l_pSourceDataLEnd = l_pSourceDataL + m_usMaxFrames;

	while ( l_pSourceDataL < l_pSourceDataLEnd )
	{
		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		AKSIMD_V2F32 vVectL1 = AKSIMD_LOAD_V2F32_OFFSET( l_pSourceDataL, 0 );
		AKSIMD_V2F32 vVectL2 = AKSIMD_LOAD_V2F32_OFFSET( l_pSourceDataL, 8 );
		l_pSourceDataL += ulVectorSize;

		// apply volume									 
		vVectL1 = AKSIMD_MUL_V2F32( vVectL1, vVolumes1 );
		vVectL2 = AKSIMD_MUL_V2F32( vVectL2, vVolumes2 );
		
		AKSIMD_V2F32 vVectR1 = AKSIMD_LOAD_V2F32_OFFSET( l_pSourceDataR, 0 );
		AKSIMD_V2F32 vVectR2 = AKSIMD_LOAD_V2F32_OFFSET( l_pSourceDataR, 8 );
		l_pSourceDataR += ulVectorSize;

		// apply volume									 
		vVectR1 = AKSIMD_MUL_V2F32( vVectR1, vVolumes1 );
		vVectR2 = AKSIMD_MUL_V2F32( vVectR2, vVolumes2 );

		// increment the volumes
		vVolumes1 = AKSIMD_ADD_V2F32( vVolumes1, vVolumesDelta );
		vVolumes2 = AKSIMD_ADD_V2F32( vVolumes2, vVolumesDelta );
		
		// we want 
		// vDest1 = vVectL1[0], vVectR1[0]
		// vDest2 = vVectL1[1], vVectR1[1]
		// vDest3 = vVectL2[0], vVectR2[0]
		// vDest4 = vVectL2[1], vVectR2[1]
		AKSIMD_V2F32 vDest1 = AKSIMD_UNPACKLO_V2F32( vVectL1, vVectR1 );
		AKSIMD_V2F32 vDest2 = AKSIMD_UNPACKHI_V2F32( vVectL1, vVectR1 );
		AKSIMD_V2F32 vDest3 = AKSIMD_UNPACKLO_V2F32( vVectL2, vVectR2 );
		AKSIMD_V2F32 vDest4 = AKSIMD_UNPACKHI_V2F32( vVectL2, vVectR2 );

		// store vDest
		AKSIMD_STORE_V2F32_OFFSET( in_pDestData, 0, vDest1 );
		AKSIMD_STORE_V2F32_OFFSET( in_pDestData, 8, vDest2 );
		AKSIMD_STORE_V2F32_OFFSET( in_pDestData, 16, vDest3 );
		AKSIMD_STORE_V2F32_OFFSET( in_pDestData, 24, vDest4 );
		in_pDestData += (2*ulVectorSize);
	}
}

#else

inline void CAkMixer::VolumeInterleavedStereo( 
	AkAudioBuffer*		in_pSource, 
	AkReal32*			in_pDestData, 
	AkSpeakerVolumesN	in_Volumes
	)
{
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataL = in_pSource->GetChannel( AK_IDX_SETUP_2_LEFT );
	AkReal32* l_pSourceDataR = in_pSource->GetChannel( AK_IDX_SETUP_2_RIGHT );
	               
	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		AKSIMD_V4F32 vVectL = AKSIMD_LOAD_V4F32( l_pSourceDataL );
		l_pSourceDataL += ulVectorSize;

		// apply volume									 
		vVectL = AKSIMD_MUL_V4F32( vVectL, vVolumes );
		
		AKSIMD_V4F32 vVectR = AKSIMD_LOAD_V4F32( l_pSourceDataR );
		l_pSourceDataR += ulVectorSize;

		// apply volume									 
		vVectR = AKSIMD_MUL_V4F32( vVectR, vVolumes );
		
		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes, vVolumesDelta );
		
		// vVectL = [ L3, L2, L1, L0] * volumes
		// vVectR = [ R3, R2, R1, R0] * volumes
		// we want vDest1 = [ R1, L1, R0, L0]
		AKSIMD_V4F32 vDest1 = AKSIMD_UNPACKLO_V4F32( vVectL, vVectR );

		// we want vDest2 = [ R3, L3, R2, L2]
		AKSIMD_V4F32 vDest2 = AKSIMD_UNPACKHI_V4F32( vVectL, vVectR );

		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// store the vDest2 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest2 );
		in_pDestData += ulVectorSize;
	}
}
#endif
//====================================================================================================
//====================================================================================================
#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
inline void CAkMixer::VolumeInterleaved51( 
	AkAudioBuffer*	in_pSource, 
	AkReal32*	in_pDestData, 
	AkSpeakerVolumesN	in_Volumes
	)
{
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT );
	AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_5_CENTER );
	AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_5_LFE );
	AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARLEFT );
	AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARRIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// get four samples								 
		AKSIMD_V4F32 vVectFL = AKSIMD_LOAD_V4F32( l_pSourceDataFL );
		l_pSourceDataFL += ulVectorSize;

		// apply volume									 
		vVectFL = AKSIMD_MUL_V4F32( vVectFL, vVolumes );
		
		AKSIMD_V4F32 vVectFR = AKSIMD_LOAD_V4F32( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;

		// apply volume									 
		vVectFR = AKSIMD_MUL_V4F32( vVectFR, vVolumes );

		AKSIMD_V4F32 vVectC = AKSIMD_LOAD_V4F32( l_pSourceDataC );
		l_pSourceDataC += ulVectorSize;

		// apply volume									 
		vVectC = AKSIMD_MUL_V4F32( vVectC, vVolumes );

		AKSIMD_V4F32 vVectLFE = AKSIMD_LOAD_V4F32( l_pSourceDataLFE );
		l_pSourceDataLFE += ulVectorSize;

		// apply volume									 
		vVectLFE = AKSIMD_MUL_V4F32( vVectLFE, vVolumes );

		AKSIMD_V4F32 vVectRL = AKSIMD_LOAD_V4F32( l_pSourceDataRL );
		l_pSourceDataRL += ulVectorSize;

		// apply volume									 
		vVectRL = AKSIMD_MUL_V4F32( vVectRL, vVolumes );

		AKSIMD_V4F32 vVectRR = AKSIMD_LOAD_V4F32( l_pSourceDataRR );
		l_pSourceDataRR += ulVectorSize;

		// apply volume									 
		vVectRR = AKSIMD_MUL_V4F32( vVectRR, vVolumes );
		
		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes,  vVolumesDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 AKSIMD_SHUFFLE_V4F32()
//   6 AKSIMD_STORE_V4F32()
//----------------------------------------------------------------------------------------------------
		register AKSIMD_V4F32 vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register AKSIMD_V4F32 vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register AKSIMD_V4F32 vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		AKSIMD_V4F32 vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ FR1, FL1, RR0, RL0 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vFrontLeftRight, AKSIMD_SHUFFLE( 3, 1, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ RR1, RL1, LFE1, C1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vCenterLfe, vRearLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );	// FR3, FR2, FL3, FL2
		vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );			// LFE3, LFE2, C3, C2
		vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );		// RR3, RR2, RL3, RL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ FR3, FL3, RR2, RL2 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vFrontLeftRight, AKSIMD_SHUFFLE( 3, 1, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ RR3, RL3, LFE3, C3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vCenterLfe, vRearLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
	}
}

#ifdef AK_71FROMSTEREOMIXER
void CAkMixer::VolumeInterleaved71FromStereo(
	AkAudioBuffer *		in_pSource,
	AkReal32*			in_pDestData,
	AkSpeakerVolumesN	in_Volumes
	)
{
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT );
	//AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_5_CENTER );
	//AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_5_LFE );
	//AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARLEFT );
	//AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARRIGHT );
	//AkReal32* l_pSourceDataEL  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDELEFT );
	//AkReal32* l_pSourceDataER  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDERIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		AKSIMD_V4F32 vVectFL = AKSIMD_LOAD_V4F32( l_pSourceDataFL );
		l_pSourceDataFL += ulVectorSize;
		// apply volume									 
		vVectFL = AKSIMD_MUL_V4F32( vVectFL, vVolumes );
		
		AKSIMD_V4F32 vVectFR = AKSIMD_LOAD_V4F32( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;
		// apply volume									 
		vVectFR = AKSIMD_MUL_V4F32( vVectFR, vVolumes );

		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes,  vVolumesDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 AKSIMD_SHUFFLE_V4F32()
//   6 AKSIMD_STORE_V4F32()
//----------------------------------------------------------------------------------------------------
		register AKSIMD_V4F32 vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register AKSIMD_V4F32 vCenterLfe = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register AKSIMD_V4F32 vRearLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0
		register AKSIMD_V4F32 vExtraLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// ER1, EL1, ER0, EL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		register AKSIMD_V4F32 vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER0, EL0, RR0, RL0  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE1, C1, FR1, FL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER1, EL1, RR1, RL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// FR3, FR2, FL3, FL2
		vCenterLfe = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );							// LFE3, LFE2, C3, C2
		vRearLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );						// RR3, RR2, RL3, RL2
		vExtraLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// ER3, EL2, ER3, EL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER2, EL2, RR2, RL2  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE3, C3, FR3, FL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER3, EL3, RR3, RL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
	}
}
#endif

#ifdef AK_71FROM51MIXER
inline void CAkMixer::VolumeInterleaved71From51(	AkAudioBuffer *				in_pSource,
													AkReal32*					in_pDestData,
													AkSpeakerVolumesN	in_Volumes
	)
{
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_5_FRONTRIGHT );
	AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_5_CENTER );
	AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_5_LFE );
	AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARLEFT );
	AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_5_REARRIGHT );
	//AkReal32* l_pSourceDataEL  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDELEFT );
	//AkReal32* l_pSourceDataER  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDERIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		AKSIMD_V4F32 vVectFL = AKSIMD_LOAD_V4F32( l_pSourceDataFL );
		l_pSourceDataFL += ulVectorSize;
		// apply volume									 
		vVectFL = AKSIMD_MUL_V4F32( vVectFL, vVolumes );
		
		AKSIMD_V4F32 vVectFR = AKSIMD_LOAD_V4F32( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;
		// apply volume									 
		vVectFR = AKSIMD_MUL_V4F32( vVectFR, vVolumes );

		AKSIMD_V4F32 vVectC = AKSIMD_LOAD_V4F32( l_pSourceDataC );
		l_pSourceDataC += ulVectorSize;
		// apply volume									 
		vVectC = AKSIMD_MUL_V4F32( vVectC, vVolumes );

		AKSIMD_V4F32 vVectLFE = AKSIMD_LOAD_V4F32( l_pSourceDataLFE );
		l_pSourceDataLFE += ulVectorSize;
		// apply volume									 
		vVectLFE = AKSIMD_MUL_V4F32( vVectLFE, vVolumes );

		AKSIMD_V4F32 vVectRL = AKSIMD_LOAD_V4F32( l_pSourceDataRL );
		l_pSourceDataRL += ulVectorSize;
		// apply volume									 
		vVectRL = AKSIMD_MUL_V4F32( vVectRL, vVolumes );

		AKSIMD_V4F32 vVectRR = AKSIMD_LOAD_V4F32( l_pSourceDataRR );
		l_pSourceDataRR += ulVectorSize;
		// apply volume									 
		vVectRR = AKSIMD_MUL_V4F32( vVectRR, vVolumes );

		/*AKSIMD_V4F32 vVectEL = AKSIMD_LOAD_V4F32( l_pSourceDataEL );
		l_pSourceDataEL += ulVectorSize;
		// apply volume									 
		vVectEL = AKSIMD_MUL_V4F32( vVectEL, vVolumes );

		AKSIMD_V4F32 vVectER = AKSIMD_LOAD_V4F32( l_pSourceDataER );
		l_pSourceDataER += ulVectorSize;
		// apply volume									 
		vVectER = AKSIMD_MUL_V4F32( vVectER, vVolumes );*/

		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes,  vVolumesDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 AKSIMD_SHUFFLE_V4F32()
//   6 AKSIMD_STORE_V4F32()
//----------------------------------------------------------------------------------------------------
		register AKSIMD_V4F32 vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register AKSIMD_V4F32 vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register AKSIMD_V4F32 vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0
		register AKSIMD_V4F32 vExtraLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// ER1, EL1, ER0, EL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		register AKSIMD_V4F32 vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER0, EL0, RR0, RL0  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE1, C1, FR1, FL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER1, EL1, RR1, RL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// FR3, FR2, FL3, FL2
		vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );							// LFE3, LFE2, C3, C2
		vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );						// RR3, RR2, RL3, RL2
		vExtraLeftRight = AKSIMD_SETZERO_V4F32(); //AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// ER3, EL2, ER3, EL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER2, EL2, RR2, RL2  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE3, C3, FR3, FL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER3, EL3, RR3, RL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
	}
}
#endif
#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
inline void CAkMixer::VolumeInterleaved71(	AkAudioBuffer *				in_pSource,
											AkReal32*					in_pDestData,
											AkSpeakerVolumesN	in_Volumes
	)
{
	BUILD_VOLUME_VECTOR( vVolumes, in_Volumes.fVolume, in_Volumes.fVolumeDelta );

	AkReal32 fVolumesDelta;
	fVolumesDelta = in_Volumes.fVolumeDelta * 2;
	AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );

	AkReal32* l_pSourceDataFL  = in_pSource->GetChannel( AK_IDX_SETUP_7_FRONTLEFT );
	AkReal32* l_pSourceDataFR  = in_pSource->GetChannel( AK_IDX_SETUP_7_FRONTRIGHT );
	AkReal32* l_pSourceDataC   = in_pSource->GetChannel( AK_IDX_SETUP_7_CENTER );
	AkReal32* l_pSourceDataLFE = in_pSource->GetChannel( AK_IDX_SETUP_7_LFE );
	AkReal32* l_pSourceDataRL  = in_pSource->GetChannel( AK_IDX_SETUP_7_REARLEFT );
	AkReal32* l_pSourceDataRR  = in_pSource->GetChannel( AK_IDX_SETUP_7_REARRIGHT );
	AkReal32* l_pSourceDataEL  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDELEFT );
	AkReal32* l_pSourceDataER  = in_pSource->GetChannel( AK_IDX_SETUP_7_SIDERIGHT );

	for( AkUInt32 ulFrames = m_usMaxFrames / ulVectorSize; ulFrames > 0; --ulFrames )
	{
		/////////////////////////////////////////////////////////////////////
		// Apply the volumes to the source
		/////////////////////////////////////////////////////////////////////

		// *in_pDestData += in_fVolume * (*(in_pSourceData)++);
		// get four samples								 
		AKSIMD_V4F32 vVectFL = AKSIMD_LOAD_V4F32( l_pSourceDataFL );
		l_pSourceDataFL += ulVectorSize;
		// apply volume									 
		vVectFL = AKSIMD_MUL_V4F32( vVectFL, vVolumes );
		
		AKSIMD_V4F32 vVectFR = AKSIMD_LOAD_V4F32( l_pSourceDataFR );
		l_pSourceDataFR += ulVectorSize;
		// apply volume									 
		vVectFR = AKSIMD_MUL_V4F32( vVectFR, vVolumes );

		AKSIMD_V4F32 vVectC = AKSIMD_LOAD_V4F32( l_pSourceDataC );
		l_pSourceDataC += ulVectorSize;
		// apply volume									 
		vVectC = AKSIMD_MUL_V4F32( vVectC, vVolumes );

		AKSIMD_V4F32 vVectLFE = AKSIMD_LOAD_V4F32( l_pSourceDataLFE );
		l_pSourceDataLFE += ulVectorSize;
		// apply volume									 
		vVectLFE = AKSIMD_MUL_V4F32( vVectLFE, vVolumes );

		AKSIMD_V4F32 vVectRL = AKSIMD_LOAD_V4F32( l_pSourceDataRL );
		l_pSourceDataRL += ulVectorSize;
		// apply volume									 
		vVectRL = AKSIMD_MUL_V4F32( vVectRL, vVolumes );

		AKSIMD_V4F32 vVectRR = AKSIMD_LOAD_V4F32( l_pSourceDataRR );
		l_pSourceDataRR += ulVectorSize;
		// apply volume									 
		vVectRR = AKSIMD_MUL_V4F32( vVectRR, vVolumes );

		AKSIMD_V4F32 vVectEL = AKSIMD_LOAD_V4F32( l_pSourceDataEL );
		l_pSourceDataEL += ulVectorSize;
		// apply volume									 
		vVectEL = AKSIMD_MUL_V4F32( vVectEL, vVolumes );

		AKSIMD_V4F32 vVectER = AKSIMD_LOAD_V4F32( l_pSourceDataER );
		l_pSourceDataER += ulVectorSize;
		// apply volume									 
		vVectER = AKSIMD_MUL_V4F32( vVectER, vVolumes );

		// increment the volumes
		vVolumes  = AKSIMD_ADD_V4F32( vVolumes,  vVolumesDelta );
//----------------------------------------------------------------------------------------------------
// Interleave the data
//
//  12 AKSIMD_SHUFFLE_V4F32()
//   6 AKSIMD_STORE_V4F32()
//----------------------------------------------------------------------------------------------------
		register AKSIMD_V4F32 vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// FR1, FR0, FL1, FL0
		register AKSIMD_V4F32 vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );			// LFE1, LFE0, C1, C0
		register AKSIMD_V4F32 vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );		// RR1, RR0, RL1, RL0
		register AKSIMD_V4F32 vExtraLeftRight = AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 1, 0, 1, 0 ) );	// ER1, EL1, ER0, EL0

		// we want vDest1 = [ LFE0, C0, FR0, FL0 ]
		register AKSIMD_V4F32 vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER0, EL0, RR0, RL0  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE1, C1, FR1, FL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER1, EL1, RR1, RL1 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		vFrontLeftRight = AKSIMD_SHUFFLE_V4F32( vVectFL, vVectFR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// FR3, FR2, FL3, FL2
		vCenterLfe = AKSIMD_SHUFFLE_V4F32( vVectC, vVectLFE, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );							// LFE3, LFE2, C3, C2
		vRearLeftRight = AKSIMD_SHUFFLE_V4F32( vVectRL, vVectRR, AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );						// RR3, RR2, RL3, RL2
		vExtraLeftRight = AKSIMD_SHUFFLE_V4F32( vVectEL, vVectER,  AKSIMD_SHUFFLE( 3, 2, 3, 2 ) );					// ER3, EL2, ER3, EL2

		// we want vDest1 = [ LFE2, C2, FR2, FL2 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 2, 0, 2, 0 ));
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER2, EL2, RR2, RL2  ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 2, 0, 2, 0 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ LFE3, C3, FR3, FL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vFrontLeftRight, vCenterLfe, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;

		// we want vDest1 = [ ER3, EL3, RR3, RL3 ]
		vDest1 = AKSIMD_SHUFFLE_V4F32( vRearLeftRight, vExtraLeftRight, AKSIMD_SHUFFLE( 3, 1, 3, 1 ) );
		// store the vDest1 result
		AKSIMD_STORE_V4F32( in_pDestData, vDest1 );
		in_pDestData += ulVectorSize;
	}
}
#endif
//====================================================================================================
//====================================================================================================
#if defined AK_APPLE || defined AK_PS4
void CAkMixer::ApplyVolume(	AkReal32* in_pSourceData, AkReal32*	in_pDestData, AkReal32 in_fVolume, AkReal32	in_fVolumeDelta )
{
	AKASSERT( !( m_usMaxFrames % 8 ) );
	
	AkReal32* AK_RESTRICT pSourceData = in_pSourceData;
	AkReal32* AK_RESTRICT pDestData = in_pDestData;
	AkReal32* AK_RESTRICT pSourceEnd = pSourceData + m_usMaxFrames;
	
	if ( in_fVolumeDelta == 0.0f )
	{
		if ( in_fVolume == 0.0f )
		{
			//everything is done over a previous cleared buffer, so if volume=0 we have nothing to do
			return;
		}
		
		BUILD_VOLUME_VECTOR( vVolumes, in_fVolume, 0.0f );
		
		do
		{
			// get eight samples								 
			AKSIMD_V4F32 vSum1 = AKSIMD_LOAD_V4F32( pSourceData );
			AKSIMD_V4F32 vSum2 = AKSIMD_LOAD_V4F32( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;
			
			// apply volume
			vSum1 = AKSIMD_MUL_V4F32( vSum1, vVolumes );
			vSum2 = AKSIMD_MUL_V4F32( vSum2, vVolumes );
			
			// store the result								 
			AKSIMD_STORE_V4F32( pDestData, vSum1 );
			AKSIMD_STORE_V4F32( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;
		}
		while ( pSourceData < pSourceEnd );
	}
	
	else // has volume delta
	{
		AkReal32 fVolumesDelta = in_fVolumeDelta * 4;
		AKSIMD_V4F32 vVolumesDelta = AKSIMD_LOAD1_V4F32( fVolumesDelta );
		
		BUILD_VOLUME_VECTOR( vVolumes1, in_fVolume, in_fVolumeDelta );
		AKSIMD_V4F32 vVolumes2 = AKSIMD_ADD_V4F32( vVolumes1, vVolumesDelta );
		
		// multiply volumes delta by 2 because the loop is unrolled by 2.
		vVolumesDelta = AKSIMD_ADD_V4F32( vVolumesDelta, vVolumesDelta );
		
		do
		{
			// get eight samples								 
			AKSIMD_V4F32 vSum1 = AKSIMD_LOAD_V4F32( pSourceData );
			AKSIMD_V4F32 vSum2 = AKSIMD_LOAD_V4F32( pSourceData + ulVectorSize );
			pSourceData += ulVectorSize * 2;
			
			// apply volume
			vSum1 = AKSIMD_MUL_V4F32( vSum1, vVolumes1 );
			vSum2 = AKSIMD_MUL_V4F32( vSum2, vVolumes2 );
			
			// store the result								 
			AKSIMD_STORE_V4F32( pDestData, vSum1 );
			AKSIMD_STORE_V4F32( pDestData + ulVectorSize, vSum2 );
			pDestData += ulVectorSize * 2;
			
			// in_fVolume += in_fVolumeDelta;
			vVolumes1 = AKSIMD_ADD_V4F32( vVolumes1, vVolumesDelta );
			vVolumes2 = AKSIMD_ADD_V4F32( vVolumes2, vVolumesDelta );
		}
		while ( pSourceData < pSourceEnd );
	}
}
#endif // AK_APPLE
//====================================================================================================
//====================================================================================================
#ifdef AK_APPLE
void CAkMixer::ProcessVolume( AkAudioBufferBus* in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer  )
{
	unsigned int uChannel = 0;
	unsigned int uNumChannels = in_pInputBuffer->NumChannels();

    AkReal32 uVolume	= in_pInputBuffer->m_fPreviousVolume;
    AkReal32 uDelta		= (in_pInputBuffer->m_fNextVolume - uVolume) * m_fOneOverNumFrames;
    
	do
	{
		AkReal32* AK_RESTRICT pInSample = in_pInputBuffer->GetChannel( uChannel );
		
		ApplyVolume( pInSample, in_pOutputBuffer->GetChannel( uChannel ), uVolume, uDelta );
		
	} while(++uChannel < uNumChannels );
	
}
#endif // AK_APPLE

//====================================================================================================
//====================================================================================================
void CAkMixer::MixAndInterleaveStereo( AkAudioBufferBus * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer )
{
	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );
#ifdef AK_ANDROID
	AkInt16* AK_RESTRICT pDryOutSample = (AkInt16*)in_pOutputBuffer->GetInterleavedData();
#else
	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)in_pOutputBuffer->GetInterleavedData();
#endif
	VolumeInterleavedStereo( in_pInputBuffer, pDryOutSample, Volumes );
}
//====================================================================================================
//====================================================================================================
#if defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
void CAkMixer::MixAndInterleave51( AkAudioBufferBus * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer )
{
	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)in_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved51( in_pInputBuffer, pDryOutSample, Volumes );
}

#ifdef AK_71FROMSTEREOMIXER
void CAkMixer::MixAndInterleave71FromStereo( AkAudioBufferBus * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer )
{
	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)in_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved71FromStereo( in_pInputBuffer, pDryOutSample, Volumes );
}
#endif

#ifdef AK_71FROM51MIXER
void CAkMixer::MixAndInterleave71From51( AkAudioBufferBus * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer )
{
	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)in_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved71From51( in_pInputBuffer, pDryOutSample, Volumes );
}
#endif
#endif // defined(AK_REARCHANNELS) && defined(AK_LFECENTER)
//====================================================================================================
//====================================================================================================
#ifdef AK_71AUDIO
void CAkMixer::MixAndInterleave71( AkAudioBufferBus * in_pInputBuffer, AkPipelineBufferBase* in_pOutputBuffer )
{
	AkSpeakerVolumesN Volumes( in_pInputBuffer->m_fPreviousVolume, in_pInputBuffer->m_fNextVolume, m_fOneOverNumFrames );

	AkReal32* AK_RESTRICT pDryOutSample = (AkReal32*)in_pOutputBuffer->GetInterleavedData();
	VolumeInterleaved71( in_pInputBuffer, pDryOutSample, Volumes );
}
#endif
