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

// Delay that must be given a pointer to it's memory, useful for effects that have a process class. 
// Length is expected to be 4 aligned for it to work 

#include "AllPassFilterLight.h"
#include <AK/Tools/Common/AkPlatformFuncs.h>
#include <AK/SoundEngine/Common/AkSimd.h>
#ifdef __SPU__
#include <AK/Plugin/PluginServices/PS3/SPUServices.h>
#endif

namespace DSP
{		

#ifndef __SPU__
	AKRESULT CAllPassFilterLight::Init( AK::IAkPluginMemAlloc * in_pAllocator, AkUInt32 in_uDelayLineLength )
	{
		m_uDelayLineLength = in_uDelayLineLength/4*4; 
		if ( m_uDelayLineLength )
		{
			m_pfDelay = (AkReal32*)AK_PLUGIN_ALLOC( in_pAllocator, AK_ALIGN_SIZE_FOR_DMA( 2 * sizeof(AkReal32) * m_uDelayLineLength ) );
			if ( m_pfDelay == NULL )
				return AK_InsufficientMemory;
		}

		m_uCurrOffset = 0;
		return AK_Success;
	}

	void CAllPassFilterLight::Reset( )
	{
		if (m_pfDelay)
			AkZeroMemLarge( (void*) m_pfDelay, AK_ALIGN_SIZE_FOR_DMA( 2 * sizeof(AkReal32) * m_uDelayLineLength ) );
		m_uCurrOffset = 0;
	}

	// Requirement: in_uNumFrames % 4 == 0
	void CAllPassFilterLight::ProcessBuffer( AkReal32 * io_pfIOBuf, AkUInt32 in_uNumFrames )
	{
		if ( !m_pfDelay ) // we have no delay line, bail out
			return;

		AKASSERT( (in_uNumFrames % 4) == 0 );	// must be a multiple of 4 and larger than 0
		const AKSIMD_V4F32 vG = AKSIMD_LOAD1_V4F32( m_fG );
		AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) (&m_pfDelay[m_uCurrOffset]);
		AKSIMD_V4F32 * AK_RESTRICT pvIOPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfIOBuf;
		AkUInt32 uFramesBeforeWrap = m_uDelayLineLength - m_uCurrOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			for (AkUInt32 uVectors = 0; uVectors < in_uNumFrames/4 ; uVectors++)
			{
				AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvIOPtr);
				AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
				++pvDelayPtr;
				AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
				AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
				++pvDelayPtr;
				AKSIMD_STORE_V4F32((AkReal32*) pvIOPtr, vYn);
				++pvIOPtr;			
			}
			m_uCurrOffset += in_uNumFrames;
			AKASSERT( m_uCurrOffset < m_uDelayLineLength );
		}
		else
		{
			AkUInt32 uVectorsRemainingToProcess = in_uNumFrames/4;
			AkUInt32 uVectorsBeforeWrap = uFramesBeforeWrap/4;
			while ( uVectorsRemainingToProcess )
			{
				AkUInt32 uVectorsToProcess = AkMin(uVectorsRemainingToProcess,uVectorsBeforeWrap);
				AkUInt32 i = uVectorsToProcess;
				
				while( i > 0 )
				{
					AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvIOPtr);
					AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
					++pvDelayPtr;
					AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
					AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
					++pvDelayPtr;
					AKSIMD_STORE_V4F32((AkReal32*) pvIOPtr, vYn);
					++pvIOPtr;		
					--i;
				}

				m_uCurrOffset += uVectorsToProcess * 4;	

				if ( m_uCurrOffset == m_uDelayLineLength )
				{
					pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) m_pfDelay;
					m_uCurrOffset = 0;
				}
				AKASSERT( m_uCurrOffset < m_uDelayLineLength );

				uVectorsRemainingToProcess -= uVectorsToProcess;
				uVectorsBeforeWrap = (m_uDelayLineLength - m_uCurrOffset)/4;
			}
		}
	}	

	void CAllPassFilterLight::ProcessBuffer( AkReal32 * in_pfInBuf, AkReal32 * out_pfOutBuf, AkUInt32 in_uNumFrames )
	{
		if ( !m_pfDelay ) // we have no delay line, bail out
			return;

		AKASSERT( (in_uNumFrames % 4) == 0 );	// must be a multiple of 4 and larger than 0
		const AKSIMD_V4F32 vG = AKSIMD_LOAD1_V4F32( m_fG );
		AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) (&m_pfDelay[m_uCurrOffset]);
		AKSIMD_V4F32 * AK_RESTRICT pvInPtr = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInBuf;
		AKSIMD_V4F32 * AK_RESTRICT pvOutPtr = (AKSIMD_V4F32 * AK_RESTRICT) out_pfOutBuf;
		AkUInt32 uFramesBeforeWrap = m_uDelayLineLength - m_uCurrOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Not wrapping this time
			for (AkUInt32 uVectors = 0; uVectors < in_uNumFrames/4 ; uVectors++)
			{
				AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvInPtr++);
				AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
				++pvDelayPtr;
				AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
				AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
				++pvDelayPtr;
				AKSIMD_STORE_V4F32((AkReal32*) pvOutPtr, vYn);
				++pvOutPtr;			
			}
			m_uCurrOffset += in_uNumFrames;
			AKASSERT( m_uCurrOffset < m_uDelayLineLength );
		}
		else
		{
			AkUInt32 uVectorsRemainingToProcess = in_uNumFrames/4;
			AkUInt32 uVectorsBeforeWrap = uFramesBeforeWrap/4;
			while ( uVectorsRemainingToProcess )
			{
				AkUInt32 uVectorsToProcess = AkMin(uVectorsRemainingToProcess,uVectorsBeforeWrap);
				AkUInt32 i = uVectorsToProcess;
				
				while( i > 0 )
				{
					AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvInPtr++);
					AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
					++pvDelayPtr;
					AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
					AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
					++pvDelayPtr;
					AKSIMD_STORE_V4F32((AkReal32*) pvOutPtr, vYn);
					++pvOutPtr;		
					--i;
				}

				m_uCurrOffset += uVectorsToProcess * 4;	

				if ( m_uCurrOffset == m_uDelayLineLength )
				{
					pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) m_pfDelay;
					m_uCurrOffset = 0;
				}
				AKASSERT( m_uCurrOffset < m_uDelayLineLength );

				uVectorsRemainingToProcess -= uVectorsToProcess;
				uVectorsBeforeWrap = (m_uDelayLineLength - m_uCurrOffset)/4;
			}
		}
	}	

#else // SPU functions

	// Requirement: in_uNumFrames % 4 == 0
	// pfScratchMem is a LS location that can handle at least in_uNumFrames
	void CAllPassFilterLight::ProcessBuffer( AkReal32 * io_pfIOBuf, AkUInt32 in_uNumFrames, AkReal32 * io_pfScratchMem, AkUInt32 in_uDMATag )
	{
		if ( !m_pfDelay ) // we have no delay line, bail out
			return;

		AKASSERT( (in_uNumFrames % 4) == 0 );	// must be a multiple of 4 and larger than 0
		const AKSIMD_V4F32 vG = AKSIMD_LOAD1_V4F32( m_fG );
		AKSIMD_V4F32 * AK_RESTRICT pvIOPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfIOBuf;
		AkUInt32 uFramesBeforeWrap = m_uDelayLineLength - m_uCurrOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Pull delay memory to local storage
			AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( 2*in_uNumFrames*sizeof(AkReal32) );
			AKASSERT( uDMASize <= 16*1024 );
			AKASSERT( ((AkUIntPtr)&m_pfDelay[m_uCurrOffset] & 0xF) == 0 );
			AkDmaGet( "CAllPassFilterLight::Delay",io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
			AkDmaWait(1<<in_uDMATag); 

			AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfScratchMem;
			// Not wrapping this time
			for (AkUInt32 uVectors = 0; uVectors < in_uNumFrames/4 ; uVectors++)
			{
				AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvIOPtr);
				AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
				++pvDelayPtr;
				AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
				AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
				++pvDelayPtr;
				AKSIMD_STORE_V4F32((AkReal32*) pvIOPtr, vYn);
				++pvIOPtr;				
			}
			
			// Put delay memory back to main memory
			AkDmaPut( "CAllPassFilterLight::Delay", io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
			AkDmaWait(1<<in_uDMATag); 
			
			m_uCurrOffset += in_uNumFrames;
			AKASSERT( m_uCurrOffset >= 0 && m_uCurrOffset < m_uDelayLineLength );
		}
		else
		{
			AkUInt32 uVectorsRemainingToProcess = in_uNumFrames/4;
			AkUInt32 uVectorsBeforeWrap = uFramesBeforeWrap/4;
			while ( uVectorsRemainingToProcess )
			{
				AkUInt32 uVectorsToProcess = AkMin(uVectorsRemainingToProcess,uVectorsBeforeWrap);

				// Pull delay memory to local storage
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( 2*uVectorsToProcess*sizeof(AKSIMD_V4F32) );
				AKASSERT( uDMASize <= 16*1024 );
				AKASSERT( ((AkUIntPtr)&m_pfDelay[m_uCurrOffset] & 0xF) == 0 );
				AkDmaGet( "CAllPassFilterLight::Delay",io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); 

				AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfScratchMem;

				AkUInt32 i = uVectorsToProcess;
				while( i > 0 )
				{
					AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvIOPtr);
					AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
					++pvDelayPtr;
					AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
					AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
					++pvDelayPtr;
					AKSIMD_STORE_V4F32((AkReal32*) pvIOPtr, vYn);
					++pvIOPtr;		
					--i;
				}

				// Put delay memory back to main memory
				AkDmaPut( "CAllPassFilterLight::Delay", io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); 

				m_uCurrOffset += uVectorsToProcess * 4;	
				if ( m_uCurrOffset == m_uDelayLineLength )
				{
					pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) m_pfDelay;
					m_uCurrOffset = 0;
				}
				AKASSERT( m_uCurrOffset >= 0 && m_uCurrOffset < m_uDelayLineLength );

				uVectorsRemainingToProcess -= uVectorsToProcess;
				uVectorsBeforeWrap = (m_uDelayLineLength - m_uCurrOffset)/4;
			}
		}
	}	


	// pfScratchMem is a LS location that can handle at least in_uNumFrames
	void CAllPassFilterLight::ProcessBuffer( AkReal32 * in_pfInBuf, AkReal32 * out_pfOutBuf, AkUInt32 in_uNumFrames, AkReal32 * io_pfScratchMem, AkUInt32 in_uDMATag )
	{
		if ( !m_pfDelay ) // we have no delay line, bail out
			return;

		AKASSERT( (in_uNumFrames % 4) == 0 );	// must be a multiple of 4 and larger than 0
		const AKSIMD_V4F32 vG = AKSIMD_LOAD1_V4F32( m_fG );
		AKSIMD_V4F32 * AK_RESTRICT pvInPtr = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInBuf;
		AKSIMD_V4F32 * AK_RESTRICT pvOutPtr = (AKSIMD_V4F32 * AK_RESTRICT) out_pfOutBuf;
		AkUInt32 uFramesBeforeWrap = m_uDelayLineLength - m_uCurrOffset;
		if ( uFramesBeforeWrap > in_uNumFrames )
		{
			// Pull delay memory to local storage
			AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( 2*in_uNumFrames*sizeof(AkReal32) );
			AKASSERT( uDMASize <= 16*1024 );
			AKASSERT( ((AkUIntPtr)&m_pfDelay[m_uCurrOffset] & 0xF) == 0 );
			AkDmaGet( "CAllPassFilterLight::Delay",io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
			AkDmaWait(1<<in_uDMATag); 

			AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfScratchMem;
			// Not wrapping this time
			for (AkUInt32 uVectors = 0; uVectors < in_uNumFrames/4 ; uVectors++)
			{
				AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvInPtr++);
				AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
				++pvDelayPtr;
				AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
				AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
				AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
				++pvDelayPtr;
				AKSIMD_STORE_V4F32((AkReal32*) pvOutPtr, vYn);
				++pvOutPtr;				
			}
			
			// Put delay memory back to main memory
			AkDmaPut( "CAllPassFilterLight::Delay", io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
			AkDmaWait(1<<in_uDMATag); 
			
			m_uCurrOffset += in_uNumFrames;
			AKASSERT( m_uCurrOffset >= 0 && m_uCurrOffset < m_uDelayLineLength );
		}
		else
		{
			AkUInt32 uVectorsRemainingToProcess = in_uNumFrames/4;
			AkUInt32 uVectorsBeforeWrap = uFramesBeforeWrap/4;
			while ( uVectorsRemainingToProcess )
			{
				AkUInt32 uVectorsToProcess = AkMin(uVectorsRemainingToProcess,uVectorsBeforeWrap);

				// Pull delay memory to local storage
				AkUInt32 uDMASize = AK_ALIGN_SIZE_FOR_DMA( 2*uVectorsToProcess*sizeof(AKSIMD_V4F32) );
				AKASSERT( uDMASize <= 16*1024 );
				AKASSERT( ((AkUIntPtr)&m_pfDelay[m_uCurrOffset] & 0xF) == 0 );
				AkDmaGet( "CAllPassFilterLight::Delay",io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); 

				AKSIMD_V4F32 * AK_RESTRICT pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) io_pfScratchMem;

				AkUInt32 i = uVectorsToProcess;
				while( i > 0 )
				{
					AKSIMD_V4F32 vXn = AKSIMD_LOAD_V4F32((AkReal32*) pvInPtr++);
					AKSIMD_V4F32 vFwdMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);	
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vXn);
					++pvDelayPtr;
					AKSIMD_V4F32 vFbkMem = AKSIMD_LOAD_V4F32((AkReal32*) pvDelayPtr);
					AKSIMD_V4F32 vYn = AKSIMD_MADD_V4F32( vG, AKSIMD_SUB_V4F32( vXn, vFbkMem), vFwdMem );
					AKSIMD_STORE_V4F32((AkReal32*) pvDelayPtr, vYn);
					++pvDelayPtr;
					AKSIMD_STORE_V4F32((AkReal32*) pvOutPtr, vYn);
					++pvOutPtr;		
					--i;
				}

				// Put delay memory back to main memory
				AkDmaPut( "CAllPassFilterLight::Delay", io_pfScratchMem, (std::uint64_t)&m_pfDelay[m_uCurrOffset], uDMASize, in_uDMATag,0,0);
				AkDmaWait(1<<in_uDMATag); 

				m_uCurrOffset += uVectorsToProcess * 4;	
				if ( m_uCurrOffset == m_uDelayLineLength )
				{
					pvDelayPtr = (AKSIMD_V4F32 * AK_RESTRICT) m_pfDelay;
					m_uCurrOffset = 0;
				}
				AKASSERT( m_uCurrOffset >= 0 && m_uCurrOffset < m_uDelayLineLength );

				uVectorsRemainingToProcess -= uVectorsToProcess;
				uVectorsBeforeWrap = (m_uDelayLineLength - m_uCurrOffset)/4;
			}
		}
	}	

#endif
} // namespace DSP
