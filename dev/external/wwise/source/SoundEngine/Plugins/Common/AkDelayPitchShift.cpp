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

#include "AkDelayPitchShift.h"
#include <math.h>
#include <AK/SoundEngine/Common/AkFPUtilities.h>
#include <AK/SoundEngine/Common/AkSimd.h>

#if defined(_DEBUG) && defined(__SPU__)
#include "libsn_spu.h"
#endif

namespace AK 
{
	namespace DSP
	{
#ifndef __SPU__
		AKRESULT AkDelayPitchShift::Init( 
			AK::IAkPluginMemAlloc * in_pAllocator, 
			AkReal32 in_MaxDelayTime, 
			AkUInt32 in_uNumChannels,
			AkUInt32 in_uSampleRate )
		{
			m_uDelayLength = (AkUInt32)floor(in_MaxDelayTime*0.001f*in_uSampleRate);
			m_uDelayLength = AK_ALIGN_TO_NEXT_BOUNDARY( m_uDelayLength, 4 );
			AKASSERT( m_uDelayLength >= 256 );
			m_fReadWriteRateDelta = 0.f;
			AKASSERT( in_uNumChannels <= AK_VOICE_MAX_NUM_CHANNELS );
			m_uNumChannels = in_uNumChannels;
			for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
			{
				m_fFractDelay[i] = 0.f;
				if ( m_uDelayLength > 0 )
				{
					AKRESULT eResult = m_DelayLines[i].Init( in_pAllocator, m_uDelayLength, 1 );
					if ( eResult != AK_Success )
						return eResult;
				}
			}
			return AK_Success;
		}

		void AkDelayPitchShift::Term( AK::IAkPluginMemAlloc * in_pAllocator )
		{
			for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
			{
				m_DelayLines[i].Term( in_pAllocator );
			}
		}

		void AkDelayPitchShift::Reset()
		{
			for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
			{
				m_DelayLines[i].Reset();
			}
		}
#endif // #ifndef __SPU__

		void AkDelayPitchShift::SetPitchFactor( AkReal32 in_fPitchFactor )
		{
			//m_fReadWriteRateDelta = 1.0f - in_fPitchFactor; 
			if ( in_fPitchFactor != 1.0f ) 
			{
				m_fReadWriteRateDelta = 1.0f - in_fPitchFactor; 
			}
			else 
			{
				// Advantage: Avoid artefacts when there is no pitch shift (and return to same state when coming back to no pitch shift)
				 //as opposed to simply sticking with the arbitraty current delay value
				 //Disadvantage, discontinuity when moving pitch factor through zero
				m_fReadWriteRateDelta = 0.f;
				for ( AkUInt32 i = 0; i < m_uNumChannels; i++ )
				{
					m_fFractDelay[i] = 0.f; 
				}
			}
		}

#ifdef AKDELAYPITCHSHIFT_USETWOPASSALGO
		void AkDelayPitchShift::ProcessChannel( 
			AkReal32 * in_pfInBuf, 
			AkReal32 * out_pfOutBuf, 
			void * in_pTempStorage,
			AkUInt32 in_uNumFrames, 
			AkUInt32 in_uChanIndex
#ifdef __SPU__
			, AkReal32 * in_pfDelayStorage
			, AkUInt32 in_uDMATag
#endif
			)
		{
			const int iAllocatedDelayLength = (AkInt32)m_uDelayLength;
			const AkReal32 fReadWriteRateDelta = m_fReadWriteRateDelta;
			const AkReal32 fDelayLength = (AkReal32)m_uDelayLength;
			const AkReal32 fHalfDelayLength = (AkReal32)(m_uDelayLength/2);
			const AkReal32 fHalfLengthNorm = 1.f/fHalfDelayLength;
			AkReal32 fFractDelay1 = m_fFractDelay[in_uChanIndex];
			AkUInt32 uWriteOffset = m_DelayLines[in_uChanIndex].GetCurrentOffset();	
			AkUInt8 * AK_RESTRICT pFrameInfoPtr = (AkUInt8 * AK_RESTRICT) in_pTempStorage;
			// Minimum number of wraps on write head to avoid branch in computation loop
			
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			AkUInt32 uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;		
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess,uFramesBeforeWrap);
				AKASSERT( (uFramesToProcess % 4) == 0 );	// must be a multiple of 4 for SIMD

#if !defined(AK_XBOX360) && !defined(__SPU__)

#define COMPUTE_READ_POS( __io_fFractDelay__, __in_uWriteOffset__, __out_iReadPos__, __out_iReadPosNext__, __out_fInterpLoc__ )		\
				{																													\
					AK_FPSetValGTE( (__io_fFractDelay__), fDelayLength, (__io_fFractDelay__), (__io_fFractDelay__) - fDelayLength );\
					AK_FPSetValLT( __io_fFractDelay__, 0.f, __io_fFractDelay__, __io_fFractDelay__ + fDelayLength );				\
					AkReal32 fWriteOffset = (AkReal32)(__in_uWriteOffset__); /* LHS */												\
					AkReal32 fReadPos = fWriteOffset - (__io_fFractDelay__);														\
					AkReal32 fReadPosFloor = floor(fReadPos); /* LHS */																\
					(__out_iReadPos__) = (AkInt32)fReadPosFloor; /* LHS */															\
					if ( (__out_iReadPos__) >= iAllocatedDelayLength )																\
						(__out_iReadPos__) -= iAllocatedDelayLength;																\
					if ( (__out_iReadPos__) < 0 )																					\
						(__out_iReadPos__) += iAllocatedDelayLength;																\
					(__out_fInterpLoc__) = fReadPos-fReadPosFloor;																	\
					AKASSERT( ((__out_iReadPos__) >= 0) && ((__out_iReadPos__) < iAllocatedDelayLength) );						\
					(__out_iReadPosNext__) = (__out_iReadPos__)+1;																	\
					if ( (__out_iReadPosNext__) >= iAllocatedDelayLength )															\
						(__out_iReadPosNext__) = 0;																					\
					AKASSERT( ((__out_iReadPosNext__) >= 0) && ((__out_iReadPosNext__) < iAllocatedDelayLength) );				\
				}

//#define AKDELAYPITCHSHIFT_UNROLLLOOP
#ifndef AKDELAYPITCHSHIFT_UNROLLLOOP

				// Step 1: Using temporary storage
				for ( AkUInt32 i = 0; i < uFramesToProcess; i++ )
				{
					AkInt32 iReadPos1,iReadPos2,iReadPosNext1,iReadPosNext2;
					AkReal32 fInterpLoc;
					fFractDelay1 += fReadWriteRateDelta;
					AkReal32 fFractDelay2 = fFractDelay1 + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1, uWriteOffset, iReadPos1, iReadPosNext1, fInterpLoc );
					COMPUTE_READ_POS( fFractDelay2, uWriteOffset, iReadPos2, iReadPosNext2, fInterpLoc );
					*(AkReal32*)pFrameInfoPtr = fFractDelay1;					pFrameInfoPtr+= sizeof(AkReal32);
					*(AkReal32*)pFrameInfoPtr = fInterpLoc;						pFrameInfoPtr+= sizeof(AkReal32);
					*(AkInt32*)pFrameInfoPtr = iReadPos1;						pFrameInfoPtr+= sizeof(AkInt32);
					*(AkInt32*)pFrameInfoPtr = iReadPosNext1;					pFrameInfoPtr+= sizeof(AkInt32);
					*(AkInt32*)pFrameInfoPtr = iReadPos2;						pFrameInfoPtr+= sizeof(AkInt32);
					*(AkInt32*)pFrameInfoPtr = iReadPosNext2;					pFrameInfoPtr+= sizeof(AkInt32);
					uWriteOffset++;
				}
#else // #ifndef AKDELAYPITCHSHIFT_UNROLLLOOP
				// Step 2 : Unroll 4x
				AKASSERT( uFramesToProcess > 0 );
				AkReal32 fFractDelay1A = fFractDelay1 + fReadWriteRateDelta;
				AkReal32 fFractDelay1B = fFractDelay1 + 2.f*fReadWriteRateDelta;
				AkReal32 fFractDelay1C = fFractDelay1 + 3.f*fReadWriteRateDelta;
				AkReal32 fFractDelay1D = fFractDelay1 + 4.f*fReadWriteRateDelta;
				for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				{
					AkInt32 iReadPos1A,iReadPos2A,iReadPosNext1A,iReadPosNext2A;
					AkInt32 iReadPos1B,iReadPos2B,iReadPosNext1B,iReadPosNext2B;
					AkInt32 iReadPos1C,iReadPos2C,iReadPosNext1C,iReadPosNext2C;
					AkInt32 iReadPos1D,iReadPos2D,iReadPosNext1D,iReadPosNext2D;
					AkReal32 fInterpLocA,fInterpLocB,fInterpLocC,fInterpLocD;
					AkReal32 fFractDelay2A = fFractDelay1A + fHalfDelayLength;
					AkReal32 fFractDelay2B = fFractDelay1B + fHalfDelayLength;
					AkReal32 fFractDelay2C = fFractDelay1C + fHalfDelayLength;
					AkReal32 fFractDelay2D = fFractDelay1D + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1A, uWriteOffset, iReadPos1A, iReadPosNext1A, fInterpLocA );
					COMPUTE_READ_POS( fFractDelay1B, uWriteOffset+1, iReadPos1B, iReadPosNext1B, fInterpLocB );
					COMPUTE_READ_POS( fFractDelay1C, uWriteOffset+2, iReadPos1C, iReadPosNext1C, fInterpLocC );
					COMPUTE_READ_POS( fFractDelay1D, uWriteOffset+3, iReadPos1D, iReadPosNext1D, fInterpLocD );	
					COMPUTE_READ_POS( fFractDelay2A, uWriteOffset, iReadPos2A, iReadPosNext2A, fInterpLocA );		
					COMPUTE_READ_POS( fFractDelay2B , uWriteOffset+1, iReadPos2B, iReadPosNext2B, fInterpLocB );				
					COMPUTE_READ_POS( fFractDelay2C, uWriteOffset+2, iReadPos2C, iReadPosNext2C, fInterpLocC );
					COMPUTE_READ_POS( fFractDelay2D, uWriteOffset+3, iReadPos2D, iReadPosNext2D, fInterpLocD );
					fFractDelay1A += 4.f*fReadWriteRateDelta;
					fFractDelay1B += 4.f*fReadWriteRateDelta;
					fFractDelay1C += 4.f*fReadWriteRateDelta;
					fFractDelay1D += 4.f*fReadWriteRateDelta;
					uWriteOffset+=4;
					((AkReal32*)pFrameInfoPtr)[0] = fFractDelay1A;				
					((AkReal32*)pFrameInfoPtr)[1] = fFractDelay1B;				
					((AkReal32*)pFrameInfoPtr)[2] = fFractDelay1C;				
					((AkReal32*)pFrameInfoPtr)[3] = fFractDelay1D;				
					pFrameInfoPtr += 4*sizeof(AkReal32);
					((AkReal32*)pFrameInfoPtr)[0] = fInterpLocA;				
					((AkReal32*)pFrameInfoPtr)[1] = fInterpLocB;				
					((AkReal32*)pFrameInfoPtr)[2] = fInterpLocC;				
					((AkReal32*)pFrameInfoPtr)[3] = fInterpLocD;				
					pFrameInfoPtr += 4*sizeof(AkReal32);
					((AkInt32*)pFrameInfoPtr)[0] = iReadPos1A;		
					((AkInt32*)pFrameInfoPtr)[1] = iReadPos1B;		
					((AkInt32*)pFrameInfoPtr)[2] = iReadPos1C;		
					((AkInt32*)pFrameInfoPtr)[3] = iReadPos1D;		
					((AkInt32*)pFrameInfoPtr)[4] = iReadPosNext1A;	
					((AkInt32*)pFrameInfoPtr)[5] = iReadPosNext1B;	
					((AkInt32*)pFrameInfoPtr)[6] = iReadPosNext1C;	
					((AkInt32*)pFrameInfoPtr)[7] = iReadPosNext1D;	
					((AkInt32*)pFrameInfoPtr)[8] = iReadPos2A;		
					((AkInt32*)pFrameInfoPtr)[9] = iReadPos2B;		
					((AkInt32*)pFrameInfoPtr)[10] = iReadPos2C;		
					((AkInt32*)pFrameInfoPtr)[11] = iReadPos2D;		
					((AkInt32*)pFrameInfoPtr)[12] = iReadPosNext2A;	
					((AkInt32*)pFrameInfoPtr)[13] = iReadPosNext2B;	
					((AkInt32*)pFrameInfoPtr)[14] = iReadPosNext2C;	
					((AkInt32*)pFrameInfoPtr)[15] = iReadPosNext2D;	
					pFrameInfoPtr += 16*sizeof(AkInt32);
				}
				fFractDelay1 = fFractDelay1A-fReadWriteRateDelta;
#endif // #ifndef AKDELAYPITCHSHIFT_UNROLLLOOP

#else // #if !defined(AK_XBOX360) && !defined(__SPU__)

				// Step 3 : SIMD
				AK_ALIGN_SIMD( AkReal32 fFractDelay1Ext[4] );
				fFractDelay1Ext[0] = fFractDelay1 + fReadWriteRateDelta;
				fFractDelay1Ext[1] = fFractDelay1 + 2.f*fReadWriteRateDelta;
				fFractDelay1Ext[2] = fFractDelay1 + 3.f*fReadWriteRateDelta;
				fFractDelay1Ext[3] = fFractDelay1 + 4.f*fReadWriteRateDelta;
				AKSIMD_V4F32 vfFractDelay1 = AKSIMD_LOAD_V4F32( fFractDelay1Ext );
				const AkReal32 fReadWriteRateDeltaInc = 4.f*fReadWriteRateDelta;
				const AKSIMD_V4F32 vfReadWriteRateDeltaInc = AKSIMD_LOAD1_V4F32( fReadWriteRateDeltaInc );
				AK_ALIGN_SIMD( AkUInt32 uWriteOffsetExt[4] );
				uWriteOffsetExt[0] = uWriteOffset;
				uWriteOffsetExt[1] = uWriteOffset+1;
				uWriteOffsetExt[2] = uWriteOffset+2;
				uWriteOffsetExt[3] = uWriteOffset+3;
				AKSIMD_V4UI32 vuWriteOffsetExt = AKSIMD_LOAD_V4UI32( uWriteOffsetExt );
				int iAllocatedDelayLengthMinusOne = iAllocatedDelayLength-1;
				unsigned int uWriteOffsetInc = 4;
				const AKSIMD_V4UI32 vuWriteOffsetInc = AKSIMD_LOAD1_V4UI32( uWriteOffsetInc );	
				const AKSIMD_V4I32 viAllocatedDelayLengthMinusOne = AKSIMD_LOAD1_V4I32( (int)iAllocatedDelayLengthMinusOne );
				const AKSIMD_V4I32 viAllocatedDelayLength = AKSIMD_LOAD1_V4I32( iAllocatedDelayLength );
				int iOne = 1;
				const AKSIMD_V4I32 viOne = AKSIMD_LOAD1_V4I32( iOne );
				const AKSIMD_V4F32 vfHalfDelayLength = AKSIMD_LOAD1_V4F32( fHalfDelayLength );
				const AKSIMD_V4F32 vfDelayLength = AKSIMD_LOAD1_V4F32( fDelayLength );				
				for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				{
					AKSIMD_V4F32 vfWriteOffsetExt = AKSIMD_CVT_V4UI32TOV4F32( vuWriteOffsetExt, 0 );
					AKSIMD_V4F32 vfFractDelay2 = AKSIMD_ADD_V4F32( vfFractDelay1, vfHalfDelayLength );

					// Delay 1
					AKSIMD_CMP_CTRLMASK vBranchCtrlMask = AKSIMD_GTEQ_V4F32(vfFractDelay1, vfDelayLength);
					vfFractDelay1 = AKSIMD_VSEL_V4F32(vfFractDelay1, AKSIMD_SUB_V4F32(vfFractDelay1,vfDelayLength), vBranchCtrlMask);
					vBranchCtrlMask = AKSIMD_LT_V4F32(vfFractDelay1, AKSIMD_SETZERO_V4F32() );
					vfFractDelay1 = AKSIMD_VSEL_V4F32(vfFractDelay1, AKSIMD_ADD_V4F32(vfFractDelay1,vfDelayLength), vBranchCtrlMask);	
					AKSIMD_V4F32 vfReadPos1 = AKSIMD_SUB_V4F32( vfWriteOffsetExt, vfFractDelay1 );
					// Floor()
#if defined(AK_XBOX360)
					AKSIMD_V4F32 vfReadPosFloor1 = __vrfim( vfReadPos1 );
#elif defined(__SPU__)
					AKSIMD_CMP_CTRLMASK vSmallerZeroCtrlMask = AKSIMD_LT_V4F32(vfReadPos1, AKSIMD_SETZERO_V4F32());
					AKSIMD_V4F32 vfReadPos1Alt = AKSIMD_CVT_V4I32TOV4F32(AKSIMD_CVT_V4F32TOV4I32( vfReadPos1, 0 ), 0);
					AKSIMD_CMP_CTRLMASK vNotPureInteger = spu_cmpeq( vfReadPos1, vfReadPos1Alt );
					vSmallerZeroCtrlMask = spu_andc( vSmallerZeroCtrlMask, vNotPureInteger );
					AKSIMD_V4F32 vfReadPosFloor1 = AKSIMD_VSEL_V4F32(vfReadPos1Alt, AKSIMD_SUB_V4F32(vfReadPos1Alt,spu_splats(1.f)), vSmallerZeroCtrlMask);
#endif
					AKSIMD_V4I32 viReadPos1 = AKSIMD_CVT_V4F32TOV4I32( vfReadPosFloor1, 0 );
					vBranchCtrlMask = AKSIMD_GT_V4I32( viReadPos1, viAllocatedDelayLengthMinusOne );
					viReadPos1 = AKSIMD_VSEL_V4F32(viReadPos1, AKSIMD_SUB_V4I32(viReadPos1,viAllocatedDelayLength), vBranchCtrlMask);
					vBranchCtrlMask = AKSIMD_GT_V4I32( AKSIMD_SETZERO_V4UI32(), viReadPos1 );
					viReadPos1 = AKSIMD_VSEL_V4F32(viReadPos1, AKSIMD_ADD_V4I32(viReadPos1,viAllocatedDelayLength), vBranchCtrlMask);
					AKSIMD_V4I32 viReadPosNext1 = AKSIMD_ADD_V4I32( viReadPos1, viOne );									
					vBranchCtrlMask = AKSIMD_GT_V4I32( viReadPosNext1, viAllocatedDelayLengthMinusOne );
					viReadPosNext1 = AKSIMD_VSEL_V4F32(viReadPosNext1, AKSIMD_SUB_V4I32(viReadPosNext1,viAllocatedDelayLength), vBranchCtrlMask);

					// Delay 2
					vBranchCtrlMask = AKSIMD_GTEQ_V4F32(vfFractDelay2, vfDelayLength);
					vfFractDelay2 = AKSIMD_VSEL_V4F32(vfFractDelay2, AKSIMD_SUB_V4F32(vfFractDelay2,vfDelayLength), vBranchCtrlMask);
					vBranchCtrlMask = AKSIMD_LT_V4F32(vfFractDelay2, AKSIMD_SETZERO_V4F32() );
					vfFractDelay2 = AKSIMD_VSEL_V4F32(vfFractDelay2, AKSIMD_ADD_V4F32(vfFractDelay2,vfDelayLength), vBranchCtrlMask);
					AKSIMD_V4F32 vfReadPos2 = AKSIMD_SUB_V4F32( vfWriteOffsetExt, vfFractDelay2 );
					// Floor()
#if defined(AK_XBOX360)
					AKSIMD_V4F32 vfReadPosFloor2 = __vrfim( vfReadPos2 );
#elif defined(__SPU__)
					vSmallerZeroCtrlMask = AKSIMD_LT_V4F32(vfReadPos2, AKSIMD_SETZERO_V4F32());
					AKSIMD_V4F32 vfReadPos2Alt = AKSIMD_CVT_V4I32TOV4F32(AKSIMD_CVT_V4F32TOV4I32( vfReadPos2, 0 ), 0);
					vNotPureInteger = spu_cmpeq( vfReadPos2, vfReadPos2Alt );
					vSmallerZeroCtrlMask = spu_andc( vSmallerZeroCtrlMask, vNotPureInteger );
					AKSIMD_V4F32 vfReadPosFloor2 = AKSIMD_VSEL_V4F32(vfReadPos2Alt, AKSIMD_SUB_V4F32(vfReadPos2Alt,spu_splats(1.f)), vSmallerZeroCtrlMask);
#endif
					AKSIMD_V4I32 viReadPos2 = AKSIMD_CVT_V4F32TOV4I32( vfReadPosFloor2, 0 );
					vBranchCtrlMask = AKSIMD_GT_V4I32( viReadPos2, viAllocatedDelayLengthMinusOne );
					viReadPos2 = AKSIMD_VSEL_V4F32(viReadPos2, AKSIMD_SUB_V4I32(viReadPos2,viAllocatedDelayLength), vBranchCtrlMask);
					vBranchCtrlMask = AKSIMD_GT_V4I32( AKSIMD_SETZERO_V4UI32(), viReadPos2 );
					viReadPos2 = AKSIMD_VSEL_V4F32(viReadPos2, AKSIMD_ADD_V4I32(viReadPos2,viAllocatedDelayLength), vBranchCtrlMask);
					AKSIMD_V4I32 viReadPosNext2 = AKSIMD_ADD_V4I32( viReadPos2, viOne );									
					vBranchCtrlMask = AKSIMD_GT_V4I32( viReadPosNext2, viAllocatedDelayLengthMinusOne );
					viReadPosNext2 = AKSIMD_VSEL_V4F32(viReadPosNext2, AKSIMD_SUB_V4I32(viReadPosNext2,viAllocatedDelayLength), vBranchCtrlMask);
					
					AKSIMD_V4F32 vfInterpLoc = AKSIMD_SUB_V4F32( vfReadPos1, vfReadPosFloor1 );
					// Could convert pos to uint16 and merge vectors to reduce memory usage if necessary
					
					// Store results
					AKSIMD_STORE_V4F32( pFrameInfoPtr, vfFractDelay1 );		
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AKSIMD_STORE_V4F32( pFrameInfoPtr, vfInterpLoc );				
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AKSIMD_STORE_V4I32( pFrameInfoPtr, viReadPos1 );	
					pFrameInfoPtr += 4*sizeof(AkInt32);
					AKSIMD_STORE_V4I32( pFrameInfoPtr, viReadPosNext1 );	
					pFrameInfoPtr += 4*sizeof(AkInt32);
					AKSIMD_STORE_V4I32( pFrameInfoPtr, viReadPos2 );	
					pFrameInfoPtr += 4*sizeof(AkInt32);
					AKSIMD_STORE_V4I32( pFrameInfoPtr, viReadPosNext2 );	
					pFrameInfoPtr += 4*sizeof(AkInt32);

					// Advance state
					vfFractDelay1 = AKSIMD_ADD_V4F32( vfFractDelay1, vfReadWriteRateDeltaInc );
					vuWriteOffsetExt = AKSIMD_ADD_V4UI32( vuWriteOffsetExt, vuWriteOffsetInc );
				}
				fFractDelay1 = AKSIMD_GETELEMENT_V4F32( vfFractDelay1, 0 )-fReadWriteRateDelta;
				uWriteOffset += uFramesToProcess;
#endif // #if !defined(AK_XBOX360) && !defined(__SPU__)					

				// Wrap delay line
				if ( uWriteOffset == iAllocatedDelayLength )
					uWriteOffset = 0;
				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;
			}

			m_fFractDelay[in_uChanIndex] = fFractDelay1;

			AkReal32 * AK_RESTRICT pfInChan = (AkReal32 * AK_RESTRICT) in_pfInBuf;
			AkReal32 * AK_RESTRICT pfOutChan = (AkReal32 * AK_RESTRICT) out_pfOutBuf;	
			pFrameInfoPtr = (AkUInt8 * AK_RESTRICT) in_pTempStorage;
			uWriteOffset = m_DelayLines[in_uChanIndex].GetCurrentOffset();	
			// Minimum number of wraps on write head to avoid branch in computation loop
			
#ifndef __SPU__
			AkReal32 * AK_RESTRICT pfDelay = (AkReal32 * AK_RESTRICT) m_DelayLines[in_uChanIndex].GetCurrentPointer(0,0);
#else
			AkReal32 * AK_RESTRICT pfDelay = (AkReal32 * AK_RESTRICT)in_pfDelayStorage;
			m_DelayLines[in_uChanIndex].GetFullDelayChannel( pfDelay, 0, in_uDMATag );
			m_DelayLines[in_uChanIndex].Wait(in_uDMATag);
#endif
			AKASSERT( ((AkUIntPtr)pfDelay & 0xF) == 0 );
			uFramesRemainingToProcess = in_uNumFrames;
			uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;		
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess,uFramesBeforeWrap);
				AKASSERT( (uFramesToProcess % 4) == 0 );	// must be a multiple of 4 for SIMD

#if !defined(AK_XBOX360) && !defined(__SPU__)

#ifndef AKDELAYPITCHSHIFT_UNROLLLOOP
				// Step 1 : Using temporary storage
				for ( AkUInt32 i = 0; i < uFramesToProcess; i++ )
				{
					AkReal32 fFractDelay = *(AkReal32*)pFrameInfoPtr;	pFrameInfoPtr+= sizeof(AkReal32);
					AkReal32 fInterpLoc = *(AkReal32*)pFrameInfoPtr;	pFrameInfoPtr+= sizeof(AkReal32);
					AkReal32 fDelayOut1 = (1.f-fInterpLoc)*pfDelay[((AkInt32*)pFrameInfoPtr)[0]] + fInterpLoc*pfDelay[((AkInt32*)pFrameInfoPtr)[1]];
					AkReal32 fDelayOut2 = (1.f-fInterpLoc)*pfDelay[((AkInt32*)pFrameInfoPtr)[2]] + fInterpLoc*pfDelay[((AkInt32*)pFrameInfoPtr)[3]]; 
					pFrameInfoPtr += 4*sizeof(AkInt32);
					AkReal32 fTriangleEnv = fabs( fFractDelay - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fOut = (1.f-fTriangleEnv)*fDelayOut1 + fTriangleEnv*fDelayOut2;
					pfDelay[uWriteOffset++] = *pfInChan++;
					*pfOutChan++ = fOut;
				}
#else // #ifndef AKDELAYPITCHSHIFT_UNROLLLOOP

				// Step 2 : Unroll 4x
				for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				{
					AkReal32 fFractDelayA = ((AkReal32*)pFrameInfoPtr)[0];	
					AkReal32 fFractDelayB = ((AkReal32*)pFrameInfoPtr)[1];	
					AkReal32 fFractDelayC = ((AkReal32*)pFrameInfoPtr)[2];	
					AkReal32 fFractDelayD = ((AkReal32*)pFrameInfoPtr)[3];	
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AkReal32 fInterpLocA = ((AkReal32*)pFrameInfoPtr)[0];	
					AkReal32 fInterpLocB = ((AkReal32*)pFrameInfoPtr)[1];	
					AkReal32 fInterpLocC = ((AkReal32*)pFrameInfoPtr)[2];	
					AkReal32 fInterpLocD = ((AkReal32*)pFrameInfoPtr)[3];	
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AkReal32 fDelayOut1A = (1.f-fInterpLocA)*pfDelay[((AkInt32*)pFrameInfoPtr)[0]] + fInterpLocA*pfDelay[((AkInt32*)pFrameInfoPtr)[4]];
					AkReal32 fDelayOut1B = (1.f-fInterpLocB)*pfDelay[((AkInt32*)pFrameInfoPtr)[1]] + fInterpLocB*pfDelay[((AkInt32*)pFrameInfoPtr)[5]];
					AkReal32 fDelayOut1C = (1.f-fInterpLocC)*pfDelay[((AkInt32*)pFrameInfoPtr)[2]] + fInterpLocC*pfDelay[((AkInt32*)pFrameInfoPtr)[6]];
					AkReal32 fDelayOut1D = (1.f-fInterpLocD)*pfDelay[((AkInt32*)pFrameInfoPtr)[3]] + fInterpLocD*pfDelay[((AkInt32*)pFrameInfoPtr)[7]];
					AkReal32 fDelayOut2A = (1.f-fInterpLocA)*pfDelay[((AkInt32*)pFrameInfoPtr)[8]] + fInterpLocA*pfDelay[((AkInt32*)pFrameInfoPtr)[12]]; 	
					AkReal32 fDelayOut2B = (1.f-fInterpLocB)*pfDelay[((AkInt32*)pFrameInfoPtr)[9]] + fInterpLocB*pfDelay[((AkInt32*)pFrameInfoPtr)[13]]; 
					AkReal32 fDelayOut2C = (1.f-fInterpLocC)*pfDelay[((AkInt32*)pFrameInfoPtr)[10]] + fInterpLocC*pfDelay[((AkInt32*)pFrameInfoPtr)[14]]; 
					AkReal32 fDelayOut2D = (1.f-fInterpLocD)*pfDelay[((AkInt32*)pFrameInfoPtr)[11]] + fInterpLocD*pfDelay[((AkInt32*)pFrameInfoPtr)[15]]; 
					pFrameInfoPtr += 16*sizeof(AkInt32);
					AkReal32 fTriangleEnvA = fabs( fFractDelayA - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvB = fabs( fFractDelayB - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvC = fabs( fFractDelayC - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvD = fabs( fFractDelayD - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fOutA = (1.f-fTriangleEnvA)*fDelayOut1A + fTriangleEnvA*fDelayOut2A;
					AkReal32 fOutB = (1.f-fTriangleEnvB)*fDelayOut1B + fTriangleEnvB*fDelayOut2B;
					AkReal32 fOutC = (1.f-fTriangleEnvC)*fDelayOut1C + fTriangleEnvC*fDelayOut2C;
					AkReal32 fOutD = (1.f-fTriangleEnvD)*fDelayOut1D + fTriangleEnvD*fDelayOut2D;
					pfDelay[uWriteOffset] = pfInChan[0];
					pfDelay[uWriteOffset+1] = pfInChan[1];
					pfDelay[uWriteOffset+2] = pfInChan[2];
					pfDelay[uWriteOffset+3] = pfInChan[3];
					uWriteOffset += 4;
					pfInChan += 4;
					pfOutChan[0] = fOutA;
					pfOutChan[1] = fOutB;
					pfOutChan[2] = fOutC;
					pfOutChan[3] = fOutD;
					pfOutChan += 4;
				}
#endif // #ifndef AKDELAYPITCHSHIFT_UNROLLLOOP

#else // #if !defined(AK_XBOX360) && !defined(__SPU__)

				// Step 2 : Unroll 4x
				for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				{
					AkReal32 fFractDelayA = ((AkReal32*)pFrameInfoPtr)[0];	
					AkReal32 fFractDelayB = ((AkReal32*)pFrameInfoPtr)[1];	
					AkReal32 fFractDelayC = ((AkReal32*)pFrameInfoPtr)[2];	
					AkReal32 fFractDelayD = ((AkReal32*)pFrameInfoPtr)[3];	
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AkReal32 fInterpLocA = ((AkReal32*)pFrameInfoPtr)[0];	
					AkReal32 fInterpLocB = ((AkReal32*)pFrameInfoPtr)[1];	
					AkReal32 fInterpLocC = ((AkReal32*)pFrameInfoPtr)[2];	
					AkReal32 fInterpLocD = ((AkReal32*)pFrameInfoPtr)[3];	
					pFrameInfoPtr += 4*sizeof(AkReal32);
					AkReal32 fDelayOut1A = (1.f-fInterpLocA)*pfDelay[((AkInt32*)pFrameInfoPtr)[0]] + fInterpLocA*pfDelay[((AkInt32*)pFrameInfoPtr)[4]];
					AkReal32 fDelayOut1B = (1.f-fInterpLocB)*pfDelay[((AkInt32*)pFrameInfoPtr)[1]] + fInterpLocB*pfDelay[((AkInt32*)pFrameInfoPtr)[5]];
					AkReal32 fDelayOut1C = (1.f-fInterpLocC)*pfDelay[((AkInt32*)pFrameInfoPtr)[2]] + fInterpLocC*pfDelay[((AkInt32*)pFrameInfoPtr)[6]];
					AkReal32 fDelayOut1D = (1.f-fInterpLocD)*pfDelay[((AkInt32*)pFrameInfoPtr)[3]] + fInterpLocD*pfDelay[((AkInt32*)pFrameInfoPtr)[7]];
					AkReal32 fDelayOut2A = (1.f-fInterpLocA)*pfDelay[((AkInt32*)pFrameInfoPtr)[8]] + fInterpLocA*pfDelay[((AkInt32*)pFrameInfoPtr)[12]]; 	
					AkReal32 fDelayOut2B = (1.f-fInterpLocB)*pfDelay[((AkInt32*)pFrameInfoPtr)[9]] + fInterpLocB*pfDelay[((AkInt32*)pFrameInfoPtr)[13]]; 
					AkReal32 fDelayOut2C = (1.f-fInterpLocC)*pfDelay[((AkInt32*)pFrameInfoPtr)[10]] + fInterpLocC*pfDelay[((AkInt32*)pFrameInfoPtr)[14]]; 
					AkReal32 fDelayOut2D = (1.f-fInterpLocD)*pfDelay[((AkInt32*)pFrameInfoPtr)[11]] + fInterpLocD*pfDelay[((AkInt32*)pFrameInfoPtr)[15]]; 
					pFrameInfoPtr += 16*sizeof(AkInt32);
					AkReal32 fTriangleEnvA = fabs( fFractDelayA - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvB = fabs( fFractDelayB - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvC = fabs( fFractDelayC - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fTriangleEnvD = fabs( fFractDelayD - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fOutA = (1.f-fTriangleEnvA)*fDelayOut1A + fTriangleEnvA*fDelayOut2A;
					AkReal32 fOutB = (1.f-fTriangleEnvB)*fDelayOut1B + fTriangleEnvB*fDelayOut2B;
					AkReal32 fOutC = (1.f-fTriangleEnvC)*fDelayOut1C + fTriangleEnvC*fDelayOut2C;
					AkReal32 fOutD = (1.f-fTriangleEnvD)*fDelayOut1D + fTriangleEnvD*fDelayOut2D;
					pfDelay[uWriteOffset] = pfInChan[0];
					pfDelay[uWriteOffset+1] = pfInChan[1];
					pfDelay[uWriteOffset+2] = pfInChan[2];
					pfDelay[uWriteOffset+3] = pfInChan[3];
					uWriteOffset += 4;
					pfInChan += 4;
					pfOutChan[0] = fOutA;
					pfOutChan[1] = fOutB;
					pfOutChan[2] = fOutC;
					pfOutChan[3] = fOutD;
					pfOutChan += 4;
				}

				//// Step 3 : SIMD -> No performance gain (360-specific version below)
				//const AKSIMD_V4F32 vfHalfLengthNorm = AKSIMD_LOAD1_V4F32( fHalfLengthNorm );
				//const AKSIMD_V4F32 vfMinusOne = {-1.f,-1.f,-1.f,-1.f};
				//const AKSIMD_V4F32 vfHalfDelayLength = AKSIMD_LOAD1_V4F32( fHalfDelayLength );
				//for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				//{
				//	AKSIMD_V4F32 vfIn = AKSIMD_LOAD_V4F32( pfInChan );
				//	AKSIMD_V4F32 vfFractDelay = AKSIMD_LOAD_V4F32( pFrameInfoPtr );
				//	pFrameInfoPtr += 4*sizeof(AkReal32);
				//	AKSIMD_V4F32 vfInterpLoc = AKSIMD_LOAD_V4F32( pFrameInfoPtr );
				//	pFrameInfoPtr += 4*sizeof(AkReal32);

				//	AKSIMD_V4F32 vfDelayPos1 = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[0]] );
				//	AKSIMD_V4F32 vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[1]] );
				//	vfDelayPos1 =  __vrlimi( vfDelayPos1, vfTmpIn, 4, 3 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[2]] );
				//	vfDelayPos1 =  __vrlimi( vfDelayPos1, vfTmpIn, 2, 2 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[3]] );
				//	vfDelayPos1 =  __vrlimi( vfDelayPos1, vfTmpIn, 1, 1 );

				//	AKSIMD_V4F32 vfDelayPosNext1 = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[4]] );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[5]] );
				//	vfDelayPosNext1 =  __vrlimi( vfDelayPosNext1, vfTmpIn, 4, 3 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[6]] );
				//	vfDelayPosNext1 =  __vrlimi( vfDelayPosNext1, vfTmpIn, 2, 2 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[7]] );
				//	vfDelayPosNext1 =  __vrlimi( vfDelayPosNext1, vfTmpIn, 1, 1 );

				//	AKSIMD_V4F32 vfDelayPos2 = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[8]] );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[9]] );
				//	vfDelayPos2 =  __vrlimi( vfDelayPos2, vfTmpIn, 4, 3 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[10]] );
				//	vfDelayPos2 =  __vrlimi( vfDelayPos2, vfTmpIn, 2, 2 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[11]] );
				//	vfDelayPos2 =  __vrlimi( vfDelayPos2, vfTmpIn, 1, 1 );

				//	AKSIMD_V4F32 vfDelayPosNext2 = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[12]] );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[13]] );
				//	vfDelayPosNext2 =  __vrlimi( vfDelayPosNext2, vfTmpIn, 4, 3 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[14]] );
				//	vfDelayPosNext2 =  __vrlimi( vfDelayPosNext2, vfTmpIn, 2, 2 );
				//	vfTmpIn = AKSIMD_LOAD_SS_V4F32( &pfDelay[((AkInt32*)pFrameInfoPtr)[15]] );
				//	vfDelayPosNext2 =  __vrlimi( vfDelayPosNext2, vfTmpIn, 1, 1 );

				//	pFrameInfoPtr += 16*sizeof(AkInt32);		
				//	AKSIMD_V4F32 vfDelayOut1 = (AKSIMD_vOnes-vfInterpLoc)*vfDelayPos1 + vfInterpLoc*vfDelayPosNext1;
				//	AKSIMD_V4F32 vfDelayOut2 = (AKSIMD_vOnes-vfInterpLoc)*vfDelayPos2 + vfInterpLoc*vfDelayPosNext2;
				//	AKSIMD_V4F32 vfTriangleEnv = vfFractDelay - vfHalfDelayLength;
				//	AKSIMD_CMP_CTRLMASK vBranchCtrlMask = AKSIMD_LT_V4F32(vfTriangleEnv, AKSIMD_SETZERO_V4F32());
				//	vfTriangleEnv = AKSIMD_VSEL_V4F32(AKSIMD_MUL_V4F32(vfTriangleEnv,vfMinusOne), vfTriangleEnv, vBranchCtrlMask); // abs
				//	vfTriangleEnv *= vfHalfLengthNorm;
				//	AKSIMD_V4F32 vfOut = (AKSIMD_vOnes-vfTriangleEnv)*vfDelayOut1 + vfTriangleEnv*vfDelayOut2;
				//	AKSIMD_STORE_V4F32( &pfDelay[uWriteOffset], vfIn );		
				//	uWriteOffset += 4;
				//	pfInChan += 4;
				//	AKSIMD_STORE_V4F32( pfOutChan, vfOut );		
				//	pfOutChan += 4;
				//}
#endif // #if !defined(AK_XBOX360) && !defined(__SPU__)
				
				// Wrap delay line
				if ( uWriteOffset == iAllocatedDelayLength )
					uWriteOffset = 0;
				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;
			}

#ifdef __SPU__
			m_DelayLines[in_uChanIndex].PutChannelWriteTap( pfDelay, in_uNumFrames, 0, in_uDMATag );
			m_DelayLines[in_uChanIndex].Wait( in_uDMATag );
#endif
			m_DelayLines[in_uChanIndex].SetCurrentOffset( uWriteOffset );
		}

#else // #ifdef AKDELAYPITCHSHIFT_USETWOPASSALGO

		void AkDelayPitchShift::ProcessChannel( 
			AkReal32 * in_pfInBuf, 
			AkReal32 * out_pfOutBuf, 
			AkUInt32 in_uNumFrames, 
			AkUInt32 in_uChanIndex
#ifdef __SPU__
			, AkReal32 * in_pfDelayStorage
			, AkUInt32 in_uDMATag
#endif
			)
		{
			const AkInt32 iAllocatedDelayLength = (AkInt32)m_uDelayLength;
			const AkReal32 fReadWriteRateDelta = m_fReadWriteRateDelta;
			const AkReal32 fDelayLength = (AkReal32)m_uDelayLength;
			const AkReal32 fHalfDelayLength = (AkReal32)(m_uDelayLength/2);
			const AkReal32 fHalfLengthNorm = 1.f/fHalfDelayLength;
			AkReal32 fFractDelay1 = m_fFractDelay[in_uChanIndex];
			AkReal32 * AK_RESTRICT pfInChan = (AkReal32 * AK_RESTRICT) in_pfInBuf;
			AkReal32 * AK_RESTRICT pfOutChan = (AkReal32 * AK_RESTRICT) out_pfOutBuf;	
			AkUInt32 uWriteOffset = m_DelayLines[in_uChanIndex].GetCurrentOffset();	
			// Minimum number of wraps on write head to avoid branch in computation loop
			
#ifndef __SPU__
			AkReal32 * AK_RESTRICT pfDelay = (AkReal32 * AK_RESTRICT) m_DelayLines[in_uChanIndex].GetCurrentPointer(0,0);
#else
			AkReal32 * AK_RESTRICT pfDelay = (AkReal32 * AK_RESTRICT)in_pfDelayStorage;
			m_DelayLines[in_uChanIndex].GetFullDelayChannel( pfDelay, 0, in_uDMATag );
			m_DelayLines[in_uChanIndex].Wait(in_uDMATag);
#endif
			AKASSERT( ((AkUIntPtr)pfDelay & 0xF) == 0 );
			AkUInt32 uFramesRemainingToProcess = in_uNumFrames;
			AkUInt32 uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;		
			while ( uFramesRemainingToProcess )
			{
				AkUInt32 uFramesToProcess = AkMin(uFramesRemainingToProcess,uFramesBeforeWrap);
				AKASSERT( (uFramesToProcess % 4) == 0 );	// must be a multiple of 4 for SIMD

//#define AKDELAYPITCHSHIFT_LOOPUNROLL
#ifndef AKDELAYPITCHSHIFT_LOOPUNROLL
				for ( AkUInt32 i = 0; i < uFramesToProcess; i++ )
				{
					fFractDelay1 += fReadWriteRateDelta;
					AkReal32 fFractDelay2 = fFractDelay1 + fHalfDelayLength;
					AK_FPSetValGTE( fFractDelay1, fDelayLength, fFractDelay1, fFractDelay1 - fDelayLength );
					AK_FPSetValGTE( fFractDelay2, fDelayLength, fFractDelay2, fFractDelay2 - fDelayLength );
					AK_FPSetValLT( fFractDelay1, 0.f, fFractDelay1, fFractDelay1 + fDelayLength );
					AK_FPSetValLT( fFractDelay2, 0.f, fFractDelay2, fFractDelay2 + fDelayLength );
					AkReal32 fWriteOffset = (AkReal32)uWriteOffset; // LHS
					AkReal32 fReadPos1 = fWriteOffset - fFractDelay1; 
					AkReal32 fReadPos2 = fWriteOffset - fFractDelay2;
					AkReal32 fReadPosFloor1 = floor(fReadPos1); // LHS
					AkReal32 fReadPosFloor2 = floor(fReadPos2);
					AkInt32 iReadPos1 = (AkInt32)fReadPosFloor1; // LHS
					AkInt32 iReadPos2 = (AkInt32)fReadPosFloor2;
					if ( iReadPos1 >= iAllocatedDelayLength )
						iReadPos1 -= iAllocatedDelayLength;
					if ( iReadPos2 >= iAllocatedDelayLength )
						iReadPos2 -= iAllocatedDelayLength;
					if ( iReadPos1 < 0 )
						iReadPos1 += iAllocatedDelayLength;
					if ( iReadPos2 < 0 )
						iReadPos2 += iAllocatedDelayLength;
					AkReal32 fInterpLoc = fReadPos1-fReadPosFloor1;
					//AKASSERT( (iReadPos1 >= 0) && (iReadPos1 < iAllocatedDelayLength) );
					//AKASSERT( (iReadPos2 >= 0) && (iReadPos2 < iAllocatedDelayLength) );
					AkInt32 iReadPosNext1 = iReadPos1+1;
					AkInt32 iReadPosNext2 = iReadPos2+1;
					if ( iReadPosNext1 >= iAllocatedDelayLength )
						iReadPosNext1 = 0;
					if ( iReadPosNext2 >= iAllocatedDelayLength )
						iReadPosNext2 = 0;
					//AKASSERT( (iReadPosNext1 >= 0) && (iReadPosNext1 < iAllocatedDelayLength) );
					//AKASSERT( (iReadPosNext2 >= 0) && (iReadPosNext2 < iAllocatedDelayLength) );
					AkReal32 fDelayOut1 = (1.f-fInterpLoc)*pfDelay[iReadPos1] + fInterpLoc*pfDelay[iReadPosNext1];
					AkReal32 fDelayOut2 = (1.f-fInterpLoc)*pfDelay[iReadPos2] + fInterpLoc*pfDelay[iReadPosNext2]; 
					AkReal32 fTriangleEnv = fabs( fFractDelay1 - fHalfDelayLength ) * fHalfLengthNorm;
					AkReal32 fOut = (1.f-fTriangleEnv)*fDelayOut1 + fTriangleEnv*fDelayOut2;
					pfDelay[uWriteOffset++] = *pfInChan++;
					*pfOutChan++ = fOut;
				}
#else // #ifndef AKDELAYPITCHSHIFT_LOOPUNROLL

#define COMPUTE_READ_POS( __io_fFractDelay__, __in_uWriteOffset__, __out_iReadPos__, __out_iReadPosNext__, __out_fInterpLoc__ )	\
				{																												\
					AK_FPSetValGTE( __io_fFractDelay__, fDelayLength, __io_fFractDelay__, __io_fFractDelay__ - fDelayLength );	\
					AK_FPSetValLT( __io_fFractDelay__, 0.f, __io_fFractDelay__, __io_fFractDelay__ + fDelayLength );			\
					AkReal32 fWriteOffset = (AkReal32)__in_uWriteOffset__; /* LHS */											\
					AkReal32 fReadPos = fWriteOffset - __io_fFractDelay__;														\
					AkReal32 fReadPosFloor = floor(fReadPos); /* LHS */															\
					__out_iReadPos__ = (AkInt32)fReadPosFloor; /* LHS */														\
					if ( __out_iReadPos__ >= iAllocatedDelayLength )															\
						__out_iReadPos__ -= iAllocatedDelayLength;																\
					if ( __out_iReadPos__ < 0 )																					\
						__out_iReadPos__ += iAllocatedDelayLength;																\
					__out_fInterpLoc__ = fReadPos-fReadPosFloor;																\
					/*AKASSERT( (__out_iReadPos__ >= 0) && (__out_iReadPos__ < iAllocatedDelayLength) );*/						\
					__out_iReadPosNext__ = __out_iReadPos__+1;																	\
					if ( __out_iReadPosNext__ >= iAllocatedDelayLength )														\
						__out_iReadPosNext__ = 0;																				\
					/*AKASSERT( (__out_iReadPosNext__ >= 0) && (__out_iReadPosNext__ < iAllocatedDelayLength) );*/				\
				}

#define COMPUTE_FRACTIONALDELAY( __in_FractDelay__, __in_iReadPos1__, __in_iReadPosNext1__, __in_iReadPos2__, __in_iReadPosNext2_, __in_fInterpLoc__ )	\
				{																																		\
					AkReal32 fDelayOut1 = (1.f-__in_fInterpLoc__)*pfDelay[__in_iReadPos1__] + __in_fInterpLoc__*pfDelay[__in_iReadPosNext1__];\
					AkReal32 fDelayOut2 = (1.f-__in_fInterpLoc__)*pfDelay[__in_iReadPos2__] + __in_fInterpLoc__*pfDelay[__in_iReadPosNext2_]; \
					AkReal32 fTriangleEnv = fabs( __in_FractDelay__ - fHalfDelayLength ) * fHalfLengthNorm;						\
					AkReal32 fOut = (1.f-fTriangleEnv)*fDelayOut1 + fTriangleEnv*fDelayOut2;									\
					pfDelay[uWriteOffset++] = *pfInChan++;																		\
					*pfOutChan++ = fOut;																						\
				}

				for ( AkUInt32 i = 0; i < uFramesToProcess; i+=4 )
				{
					AkInt32 iReadPos1A,iReadPos2A,iReadPosNext1A,iReadPosNext2A;
					AkInt32 iReadPos1B,iReadPos2B,iReadPosNext1B,iReadPosNext2B;
					AkInt32 iReadPos1C,iReadPos2C,iReadPosNext1C,iReadPosNext2C;
					AkInt32 iReadPos1D,iReadPos2D,iReadPosNext1D,iReadPosNext2D;
					AkReal32 fInterpLocA,fInterpLocB,fInterpLocC,fInterpLocD;
					AkReal32 fFractDelay1A,fFractDelay1B,fFractDelay1C,fFractDelay1D;
					AkReal32 fFractDelay2A,fFractDelay2B,fFractDelay2C,fFractDelay2D;

					fFractDelay1A = fFractDelay1 + fReadWriteRateDelta;
					fFractDelay2A = fFractDelay1A + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1A, uWriteOffset, iReadPos1A, iReadPosNext1A, fInterpLocA );
					COMPUTE_READ_POS( fFractDelay2A, uWriteOffset, iReadPos2A, iReadPosNext2A, fInterpLocA );		
					fFractDelay1B = fFractDelay1A + fReadWriteRateDelta;
					fFractDelay2B = fFractDelay1B + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1B, uWriteOffset+1, iReadPos1B, iReadPosNext1B, fInterpLocB );
					COMPUTE_READ_POS( fFractDelay2B , uWriteOffset+1, iReadPos2B, iReadPosNext2B, fInterpLocB );
					fFractDelay1C = fFractDelay1B + fReadWriteRateDelta;
					fFractDelay2C = fFractDelay1C + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1C, uWriteOffset+2, iReadPos1C, iReadPosNext1C, fInterpLocC );
					COMPUTE_READ_POS( fFractDelay2C, uWriteOffset+2, iReadPos2C, iReadPosNext2C, fInterpLocC );
					fFractDelay1D = fFractDelay1C + fReadWriteRateDelta;
					fFractDelay2D = fFractDelay1D + fHalfDelayLength;
					COMPUTE_READ_POS( fFractDelay1D, uWriteOffset+3, iReadPos1D, iReadPosNext1D, fInterpLocD );
					COMPUTE_READ_POS( fFractDelay2D, uWriteOffset+3, iReadPos2D, iReadPosNext2D, fInterpLocD );
					COMPUTE_FRACTIONALDELAY(fFractDelay1A,iReadPos1A,iReadPosNext1A,iReadPos2A,iReadPosNext2A,fInterpLocA);	
					COMPUTE_FRACTIONALDELAY(fFractDelay1B,iReadPos1B,iReadPosNext1B,iReadPos2B,iReadPosNext2B,fInterpLocB);
					COMPUTE_FRACTIONALDELAY(fFractDelay1C,iReadPos1C,iReadPosNext1C,iReadPos2C,iReadPosNext2C,fInterpLocC);
					COMPUTE_FRACTIONALDELAY(fFractDelay1D,iReadPos1D,iReadPosNext1D,iReadPos2D,iReadPosNext2D,fInterpLocD);
					fFractDelay1 = fFractDelay1D;
				}
#endif // LOOP_UNROLL
				
				// Wrap delay line
				if ( uWriteOffset == iAllocatedDelayLength )
					uWriteOffset = 0;
				uFramesRemainingToProcess -= uFramesToProcess;
				uFramesBeforeWrap = (AkUInt32)iAllocatedDelayLength - uWriteOffset;
			}

#ifdef __SPU__
			m_DelayLines[in_uChanIndex].PutChannelWriteTap( pfDelay, in_uNumFrames, 0, in_uDMATag );
			m_DelayLines[in_uChanIndex].Wait( in_uDMATag );
#endif
			m_DelayLines[in_uChanIndex].SetCurrentOffset( uWriteOffset );
			m_fFractDelay[in_uChanIndex] = fFractDelay1;
		}
#endif // AKDELAYPITCHSHIFT_USETWOPASSALGO

	} // namespace DSP
} // AK namespace
