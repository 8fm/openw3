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

// Simple mix of 2 signals with interpolating volume
// Can be used as wet/dry mix
// To be used on mono signals, create as many instances as there are channels if need be

#include <AK/Tools/Common/AkAssert.h>
#include <AK/SoundEngine/Common/AkSimd.h>

namespace DSP
{
	// Out of place processing routine
	void Accumulate(
		AkReal32 * out_pfOutput, 
		AkReal32 * in_pfInput1, 
		AkReal32 * in_pfInput2, 	
		AkUInt32 in_uNumFrames )
	{
		// Friendly reminder for RESTRICT constraints
		AKASSERT( !((out_pfOutput == in_pfInput1) ||  (out_pfOutput == in_pfInput2)) );
		AKSIMD_V4F32 * AK_RESTRICT pvInput1 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput1;
		AKSIMD_V4F32 * AK_RESTRICT pvInput2 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput2;
		AKSIMD_V4F32 * AK_RESTRICT pvOutput = (AKSIMD_V4F32 * AK_RESTRICT) out_pfOutput;
		const AKSIMD_V4F32 * pvEnd = (AKSIMD_V4F32 *) ( out_pfOutput + in_uNumFrames );
		AKASSERT( (AkUIntPtr(pvInput1) & 0xF) == 0 );
		AKASSERT( (AkUIntPtr(pvInput2) & 0xF) == 0 );
		AKASSERT( (AkUIntPtr(pvOutput) & 0xF) == 0 );
		AKASSERT( (AkUIntPtr(pvEnd) & 0xF) == 0 );
		while ( pvOutput < pvEnd )
		{
			AKSIMD_V4F32 vIn1 = AKSIMD_LOAD_V4F32(pvInput1);
			AKSIMD_V4F32 vIn2 = AKSIMD_LOAD_V4F32(pvInput2);
			AKSIMD_V4F32 vOut = AKSIMD_ADD_V4F32( vIn1, vIn2 );
			AKSIMD_STORE_V4F32( pvOutput, vOut );
			pvInput1++;
			pvInput2++;
			pvOutput++;
		}
	}

	// In-place processing routine ( io_pfIn1Out == Input1 and output buffer )
	void Accumulate( 
		AkReal32 * io_pfIn1Out, 
		AkReal32 * in_pfInput2, 
		AkUInt32 in_uNumFrames )
	{
		AKSIMD_V4F32 * AK_RESTRICT pvIn1Out = (AKSIMD_V4F32 * AK_RESTRICT) io_pfIn1Out;
		AKSIMD_V4F32 * AK_RESTRICT pvInput2 = (AKSIMD_V4F32 * AK_RESTRICT) in_pfInput2;
		const AKSIMD_V4F32 * pvEnd = (AKSIMD_V4F32 *) ( io_pfIn1Out + in_uNumFrames );
		AKASSERT( (AkUIntPtr(io_pfIn1Out) & 0xF) == 0 );
		AKASSERT( (AkUIntPtr(pvInput2) & 0xF) == 0 );
		AKASSERT( (AkUIntPtr(pvEnd) & 0xF) == 0 );

#ifdef AK_USE_PREFETCH
		for( AkUInt32 uPrefetchOffset = 0; uPrefetchOffset < AKSIMD_ARCHMAXPREFETCHSIZE/2; uPrefetchOffset += AKSIMD_ARCHCACHELINESIZE )
		{
			AKSIMD_PREFETCHMEMORY(uPrefetchOffset, pvIn1Out);
			AKSIMD_PREFETCHMEMORY(uPrefetchOffset, pvInput2);
		}
#endif	

#if defined(__SPU__)
#define ACCUMULATE_LOOP_UNROLL 8
#else
#define ACCUMULATE_LOOP_UNROLL 4 // default unroll
#endif

#define		UNROLL_ITER( __Offset__ )\
			vIn1 = AKSIMD_LOAD_V4F32(pvIn1Out+(__Offset__));\
			vIn2 = AKSIMD_LOAD_V4F32(pvInput2+(__Offset__));\
			vOut = AKSIMD_ADD_V4F32( vIn1, vIn2 );\
			AKSIMD_STORE_V4F32( pvIn1Out+(__Offset__), vOut );

		AkUInt32 uNumUnrolledIter = in_uNumFrames/(ACCUMULATE_LOOP_UNROLL*4);
#if defined(__SPU__)

		while ( uNumUnrolledIter-- )
		{
#ifdef AK_USE_PREFETCH			
			// Prefetch another line ahead
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvIn1Out);
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvInput2);
#endif			
			AKSIMD_V4F32 vIn1, vIn2, vOut;
			UNROLL_ITER(0);
			UNROLL_ITER(1);
			UNROLL_ITER(2);
			UNROLL_ITER(3);
			UNROLL_ITER(4);
			UNROLL_ITER(5);
			UNROLL_ITER(6);
			UNROLL_ITER(7);

			pvInput2+=ACCUMULATE_LOOP_UNROLL;
			pvIn1Out+=ACCUMULATE_LOOP_UNROLL;
		}

#else
		
		while ( uNumUnrolledIter-- )
		{
#ifdef AK_USE_PREFETCH			
			// Prefetch another line ahead
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvIn1Out);
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvInput2);
#endif			
			AKSIMD_V4F32 vIn1, vIn2, vOut;

			UNROLL_ITER(0);
			UNROLL_ITER(1);
			UNROLL_ITER(2);
			UNROLL_ITER(3);

			pvInput2+=ACCUMULATE_LOOP_UNROLL;
			pvIn1Out+=ACCUMULATE_LOOP_UNROLL;
		}

#endif // defined(AK_PS3)

		// Handle remaining iterations untill the end
		while ( pvIn1Out < pvEnd )
		{
#ifdef AK_USE_PREFETCH			
			// Prefetch another line ahead
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvIn1Out);
			AKSIMD_PREFETCHMEMORY(AKSIMD_ARCHMAXPREFETCHSIZE/2, pvInput2);
#endif			

			AKSIMD_V4F32 vIn1 = AKSIMD_LOAD_V4F32(pvIn1Out);
			AKSIMD_V4F32 vIn2 = AKSIMD_LOAD_V4F32(pvInput2);
			AKSIMD_V4F32 vOut = AKSIMD_ADD_V4F32( vIn1, vIn2 );
			AKSIMD_STORE_V4F32( pvIn1Out, vOut );
			pvInput2++;
			pvIn1Out++;
		}
	}

} // namespace DSP
