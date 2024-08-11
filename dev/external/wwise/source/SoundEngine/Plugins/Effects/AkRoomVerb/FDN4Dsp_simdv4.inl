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

//All this code is compiled in the header and inlined in only one function
//Current scope is the FDN4 class (private)


//Apply low pass filter
AkForceInline void FDN4::ApplyLowPassFilter(
			AKSIMD_V4F32 &vfTime0, AKSIMD_V4F32 &vfTime1, AKSIMD_V4F32 &vfTime2, AKSIMD_V4F32 &vfTime3,
			AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1, AKSIMD_V4F32 &vfFFbk )
{
	vfTime0 = AKSIMD_MUL_V4F32(vfTime0, vfB0);
	vfTime0 = AKSIMD_MADD_V4F32(vfFFbk, vfA1, vfTime0);

	vfTime1 = AKSIMD_MUL_V4F32(vfTime1, vfB0);
	vfTime1 = AKSIMD_MADD_V4F32(vfTime0, vfA1, vfTime1);

	vfTime2 = AKSIMD_MUL_V4F32(vfTime2, vfB0);
	vfTime2 = AKSIMD_MADD_V4F32(vfTime1, vfA1, vfTime2);

	vfTime3 = AKSIMD_MUL_V4F32(vfTime3, vfB0);
	vfTime3 = AKSIMD_MADD_V4F32(vfTime2, vfA1, vfTime3);

	vfFFbk = vfTime3;
}

#if defined(AK_CPU_ARM_NEON)
AkForceInline void FDN4::ReadAndAttenuateDelays(
				AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3,
				AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1 )
{
	AKSIMD_V4F32 vfTime0, vfTime1, vfTime2, vfTime3;
	AKSIMD_V4F32X2 vfTemp0, vfTemp1;

	FDNDelayLine[0].ReadSamples( vfDelay0 ); // A0 A1 A2 A3
	FDNDelayLine[1].ReadSamples( vfDelay1 ); // B0 B1 B2 B3
	FDNDelayLine[2].ReadSamples( vfDelay2 ); // C0 C1 C2 C3
	FDNDelayLine[3].ReadSamples( vfDelay3 ); // D0 D1 D2 D3

	vfTemp0 = AKSIMD_TRANSPOSE_V4F32( vfDelay0, vfDelay1 );
	vfTemp1 = AKSIMD_TRANSPOSE_V4F32( vfDelay2, vfDelay3 );

	vfTime0 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[0], vfTemp1.val[0], AKSIMD_SHUFFLE(1,0,1,0) );
	vfTime1 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[1], vfTemp1.val[1], AKSIMD_SHUFFLE(1,0,1,0) );
	vfTime2 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[0], vfTemp1.val[0], AKSIMD_SHUFFLE(3,2,3,2) );
	vfTime3 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[1], vfTemp1.val[1], AKSIMD_SHUFFLE(3,2,3,2) );

	//Apply low pass filter
	ApplyLowPassFilter( vfTime0, vfTime1, vfTime2, vfTime3, vfB0, vfA1, vfFFbk );

	//Rotate back the matrix
	vfTemp0 = AKSIMD_TRANSPOSE_V4F32( vfTime0, vfTime1 );
	vfTemp1 = AKSIMD_TRANSPOSE_V4F32( vfTime2, vfTime3 );

	vfDelay0 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[0], vfTemp1.val[0], AKSIMD_SHUFFLE(1,0,1,0) );
	vfDelay1 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[1], vfTemp1.val[1], AKSIMD_SHUFFLE(1,0,1,0) );
	vfDelay2 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[0], vfTemp1.val[0], AKSIMD_SHUFFLE(3,2,3,2) );
	vfDelay3 = AKSIMD_SHUFFLE_V4F32( vfTemp0.val[1], vfTemp1.val[1], AKSIMD_SHUFFLE(3,2,3,2) );
}
#elif defined(AK_XBOX360)
AkForceInline void FDN4::ReadAndAttenuateDelays(
				AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3,
				AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1 )
{
	FDNDelayLine[0].ReadSamples( vfDelay0 ); // A0 A1 A2 A3
	FDNDelayLine[1].ReadSamples( vfDelay1 ); // B0 B1 B2 B3
	FDNDelayLine[2].ReadSamples( vfDelay2 ); // C0 C1 C2 C3
	FDNDelayLine[3].ReadSamples( vfDelay3 ); // D0 D1 D2 D3

	const __vector4i vShuffle0 = { 0x00010203, 0x10111213, 0x04050607, 0x14151617 }; // {A0,A1,A2,A3} {B0,B1,B2,B3} => {A0,B0,A1,B1}
	const __vector4i vShuffle1 = { 0x08090a0b, 0x18191a1b, 0x0c0d0e0f, 0x1c1d1e1f }; // {A0,A1,A2,A3} {B0,B1,B2,B3} => {A2,B2,A3,B3}
	const __vector4i vShuffle2 = { 0x00010203, 0x04050607, 0x10111213, 0x14151617 }; // {A0,B0,A1,B1} {C0,D0,C1,D1} => {A0,B0,C0,D0}
	const __vector4i vShuffle3 = { 0x08090a0b, 0x0c0d0e0f, 0x18191a1b, 0x1c1d1e1f }; // {A0,B0,A1,B1} {C0,D0,C1,D1} => {A1,B1,C1,D1}

	AKSIMD_V4F32 vfTemp0 = __vperm( vfDelay0, vfDelay1, *(__vector4*)vShuffle0 );
	AKSIMD_V4F32 vfTemp1 = __vperm( vfDelay0, vfDelay1, *(__vector4*)vShuffle1 );
	AKSIMD_V4F32 vfTemp2 = __vperm( vfDelay2, vfDelay3, *(__vector4*)vShuffle0 );
	AKSIMD_V4F32 vfTemp3 = __vperm( vfDelay2, vfDelay3, *(__vector4*)vShuffle1 );

	AKSIMD_V4F32 vfTime0 = __vperm( vfTemp0, vfTemp2, *(__vector4*)vShuffle2 );
	AKSIMD_V4F32 vfTime1 = __vperm( vfTemp0, vfTemp2, *(__vector4*)vShuffle3 );
	AKSIMD_V4F32 vfTime2 = __vperm( vfTemp1, vfTemp3, *(__vector4*)vShuffle2 );
	AKSIMD_V4F32 vfTime3 = __vperm( vfTemp1, vfTemp3, *(__vector4*)vShuffle3 );

	//Apply low pass filter
	ApplyLowPassFilter( vfTime0, vfTime1, vfTime2, vfTime3, vfB0, vfA1, vfFFbk );

	//Rotate back the matrix
	vfTemp0 = __vperm( vfTime0, vfTime1, *(__vector4*)vShuffle0 );
	vfTemp1 = __vperm( vfTime0, vfTime1, *(__vector4*)vShuffle1 );
	vfTemp2 = __vperm( vfTime2, vfTime3, *(__vector4*)vShuffle0 );
	vfTemp3 = __vperm( vfTime2, vfTime3, *(__vector4*)vShuffle1 );

	vfDelay0 = __vperm( vfTemp0, vfTemp2, *(__vector4*)vShuffle2 );
	vfDelay1 = __vperm( vfTemp0, vfTemp2, *(__vector4*)vShuffle3 );
	vfDelay2 = __vperm( vfTemp1, vfTemp3, *(__vector4*)vShuffle2 );
	vfDelay3 = __vperm( vfTemp1, vfTemp3, *(__vector4*)vShuffle3 );
}
#else
AkForceInline void FDN4::ReadAndAttenuateDelays(
	AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3,
	AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfA1 )
{
	FDNDelayLine[0].ReadSamples( vfDelay0 ); // A0 A1 A2 A3
	FDNDelayLine[1].ReadSamples( vfDelay1 ); // B0 B1 B2 B3
	FDNDelayLine[2].ReadSamples( vfDelay2 ); // C0 C1 C2 C3
	FDNDelayLine[3].ReadSamples( vfDelay3 ); // D0 D1 D2 D3

	AKSIMD_V4F32 vfTemp0 = AKSIMD_SHUFFLE_V4F32( vfDelay0, vfDelay1, AKSIMD_SHUFFLE(1,0,1,0) ); // {A0,A1,A2,A3} {B0,B1,B2,B3} => {A0,A1,B0,B1}
	AKSIMD_V4F32 vfTemp1 = AKSIMD_SHUFFLE_V4F32( vfDelay0, vfDelay1, AKSIMD_SHUFFLE(3,2,3,2) ); // {A0,A1,A2,A3} {B0,B1,B2,B3} => {A2,A3,B2,B3}
	AKSIMD_V4F32 vfTemp2 = AKSIMD_SHUFFLE_V4F32( vfDelay2, vfDelay3, AKSIMD_SHUFFLE(1,0,1,0) ); // {C0,C1,C2,C3} {D0,D1,D2,D3} => {C0,C1,D0,D1}
	AKSIMD_V4F32 vfTemp3 = AKSIMD_SHUFFLE_V4F32( vfDelay2, vfDelay3, AKSIMD_SHUFFLE(3,2,3,2) ); // {C0,C1,C2,C3} {D0,D1,D2,D3} => {C2,C3,D2,D3}

	AKSIMD_V4F32 vfTime0 = AKSIMD_SHUFFLE_V4F32( vfTemp0, vfTemp2, AKSIMD_SHUFFLE(2,0,2,0) ); // {A0,A1,B0,B1} {C0,C1,D0,D1} => {A0,B0,C0,D0}
	AKSIMD_V4F32 vfTime1 = AKSIMD_SHUFFLE_V4F32( vfTemp0, vfTemp2, AKSIMD_SHUFFLE(3,1,3,1) ); // {A0,A1,B0,B1} {C0,C1,D0,D1} => {A1,B1,C1,D1}
	AKSIMD_V4F32 vfTime2 = AKSIMD_SHUFFLE_V4F32( vfTemp1, vfTemp3, AKSIMD_SHUFFLE(2,0,2,0) ); // {A2,A3,B2,B3} {C2,C3,D2,D3} => {A2,B2,C2,D2}
	AKSIMD_V4F32 vfTime3 = AKSIMD_SHUFFLE_V4F32( vfTemp1, vfTemp3, AKSIMD_SHUFFLE(3,1,3,1) ); // {A2,A3,B2,B3} {C2,C3,D2,D3} => {A3,B3,C3,D3}

	//Apply low pass filter
	ApplyLowPassFilter( vfTime0, vfTime1, vfTime2, vfTime3, vfB0, vfA1, vfFFbk );

	//Rotate back the matrix
	vfTemp0 = AKSIMD_SHUFFLE_V4F32( vfTime0, vfTime1, AKSIMD_SHUFFLE(1,0,1,0) );
	vfTemp1 = AKSIMD_SHUFFLE_V4F32( vfTime0, vfTime1, AKSIMD_SHUFFLE(3,2,3,2) );
	vfTemp2 = AKSIMD_SHUFFLE_V4F32( vfTime2, vfTime3, AKSIMD_SHUFFLE(1,0,1,0) );
	vfTemp3 = AKSIMD_SHUFFLE_V4F32( vfTime2, vfTime3, AKSIMD_SHUFFLE(3,2,3,2) );

	vfDelay0 = AKSIMD_SHUFFLE_V4F32( vfTemp0, vfTemp2, AKSIMD_SHUFFLE(2,0,2,0) );
	vfDelay1 = AKSIMD_SHUFFLE_V4F32( vfTemp0, vfTemp2, AKSIMD_SHUFFLE(3,1,3,1) );
	vfDelay2 = AKSIMD_SHUFFLE_V4F32( vfTemp1, vfTemp3, AKSIMD_SHUFFLE(2,0,2,0) );
	vfDelay3 = AKSIMD_SHUFFLE_V4F32( vfTemp1, vfTemp3, AKSIMD_SHUFFLE(3,1,3,1) );
}
#endif

AkForceInline void FDN4::FeedbackAndInputReinjection( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3, AkReal32 * AK_RESTRICT &pfInBuf )
{
	const AKSIMD_V4F32 vfHalf = AKSIMD_SET_V4F32( -0.5f );

	AKSIMD_V4F32 vfIn = AKSIMD_LOADU_V4F32(pfInBuf);
	pfInBuf += 4;

	// Feedback matrix
	AKSIMD_V4F32 vfScaledDelaySum = vfDelay0;
	vfScaledDelaySum = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay1);
	vfScaledDelaySum = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay2);
	vfScaledDelaySum = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay3);
	vfScaledDelaySum = AKSIMD_MADD_V4F32(vfScaledDelaySum, vfHalf, vfIn);

	AKSIMD_V4F32 vfFeedback0 = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay1);
	AKSIMD_V4F32 vfFeedback1 = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay2);
	AKSIMD_V4F32 vfFeedback2 = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay3);
	AKSIMD_V4F32 vfFeedback3 = AKSIMD_ADD_V4F32(vfScaledDelaySum, vfDelay0);

	// Input reinjection
	FDNDelayLine[0].WriteSamplesNoWrapCheck( vfFeedback0 );
	FDNDelayLine[1].WriteSamplesNoWrapCheck( vfFeedback1 );
	FDNDelayLine[2].WriteSamplesNoWrapCheck( vfFeedback2 );
	FDNDelayLine[3].WriteSamplesNoWrapCheck( vfFeedback3 );
}

AkForceInline void FDN4::ReadAndAttenuateDelays2( AKSIMD_V4F32 &vfDelayOut, AKSIMD_V4F32 &vfB0, AKSIMD_V4F32 &vfFFbk, AKSIMD_V4F32 &vfA1 )
{
	AKSIMD_GETELEMENT_V4F32( vfDelayOut, 0 ) = FDNDelayLine[0].ReadSample( );
	AKSIMD_GETELEMENT_V4F32( vfDelayOut, 1 ) = FDNDelayLine[1].ReadSample( );
	AKSIMD_GETELEMENT_V4F32( vfDelayOut, 2 ) = FDNDelayLine[2].ReadSample( );
	AKSIMD_GETELEMENT_V4F32( vfDelayOut, 3 ) = FDNDelayLine[3].ReadSample( );

	// Low-pass attenuation
	vfDelayOut = AKSIMD_MUL_V4F32(vfDelayOut, vfB0);
	vfDelayOut = AKSIMD_MADD_V4F32(vfFFbk, vfA1, vfDelayOut);
	vfFFbk = vfDelayOut;
}

#if defined(AK_CPU_ARM_NEON)
AkForceInline void FDN4::FeedbackAndReinjection2( AKSIMD_V2F32 &vfPartialSum, AKSIMD_V4F32 &vfDelayOut, AkReal32 * AK_RESTRICT &pfInBuf)
{
	// Feedback matrix
	AKSIMD_V4F32 vfScaledDelaySum = AKSIMD_SET_V4F32( -0.5f * (AKSIMD_GETELEMENT_V2F32(vfPartialSum, 0) + AKSIMD_GETELEMENT_V2F32(vfPartialSum, 1)) );
	vfDelayOut = AKSIMD_ADD_V4F32(vfDelayOut, vfScaledDelaySum);

	//Rotate left. A B C D -> B C D A
	AKSIMD_V4F32 vfFeedback;
	
	// todo: use rotate if available
	AKSIMD_GETELEMENT_V4F32( vfFeedback, 0 ) = AKSIMD_GETELEMENT_V4F32( vfDelayOut, 1 );
	AKSIMD_GETELEMENT_V4F32( vfFeedback, 1 ) = AKSIMD_GETELEMENT_V4F32( vfDelayOut, 2 );
	AKSIMD_GETELEMENT_V4F32( vfFeedback, 2 ) = AKSIMD_GETELEMENT_V4F32( vfDelayOut, 3 );
	AKSIMD_GETELEMENT_V4F32( vfFeedback, 3 ) = AKSIMD_GETELEMENT_V4F32( vfDelayOut, 0 );

	// Input reinjection
	AKSIMD_V4F32 vfIn = AKSIMD_SET_V4F32(*pfInBuf++);
	vfFeedback = AKSIMD_ADD_V4F32(vfFeedback, vfIn);

	FDNDelayLine[0].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 0 ) );
	FDNDelayLine[1].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 1 ) );
	FDNDelayLine[2].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 2 ) );
	FDNDelayLine[3].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 3 ) );
}
#else
AkForceInline void FDN4::FeedbackAndReinjection2( AKSIMD_V4F32 &vfDelayOut, AkReal32 * AK_RESTRICT &pfInBuf)
{
	// Feedback matrix
	const AKSIMD_V4F32 vfMinusHalf = AKSIMD_SET_V4F32( -0.5f );
	AKSIMD_V4F32 vfScaledDelaySum = AKSIMD_DOTPRODUCT( vfDelayOut, vfMinusHalf );
	vfDelayOut = AKSIMD_ADD_V4F32( vfScaledDelaySum, vfDelayOut );

	//Rotate left. A B C D -> B C D A
	AKSIMD_V4F32 vfFeedback = AKSIMD_SHUFFLE_BCDA( vfDelayOut );

	// Input reinjection
	AKSIMD_V4F32 vfIn = AKSIMD_SET_V4F32(*pfInBuf++);
	vfFeedback = AKSIMD_ADD_V4F32(vfFeedback, vfIn);

	FDNDelayLine[0].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 0 ) );
	FDNDelayLine[1].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 1 ) );
	FDNDelayLine[2].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 2 ) );
	FDNDelayLine[3].WriteSampleNoWrapCheck( AKSIMD_GETELEMENT_V4F32( vfFeedback, 3 ) );
}
#endif

//Generic function for the FDN process.  The goal of the template is to keep the same code 
//for the 3 variants: single, dual and triple output buffers.  
//Normally, if the compiler does its job all the code should be inlined properly, as in the non-templated version.
//The only difference between the 3 versions is the way SumToOutputBuffers is done.  This is now handled by the 
//policy class for each of the 3 versions.
//This way is less trouble to maintain, there only one copy of the code :)

template<class OutputPolicy>
AkForceInline void FDN4::GenericProcessBuffer(AkReal32 * in_pfInBuffer, AkUInt32 in_uNumFrames, OutputPolicy in_Output)
{
	AkReal32 * AK_RESTRICT pfInBuf = (AkReal32 * AK_RESTRICT) in_pfInBuffer;

	AKSIMD_V4F32 vfFFbk;

	AKSIMD_GETELEMENT_V4F32( vfFFbk, 0 ) = delayLowPassFilter[0].fFFbk1;
	AKSIMD_GETELEMENT_V4F32( vfFFbk, 1 ) = delayLowPassFilter[1].fFFbk1;
	AKSIMD_GETELEMENT_V4F32( vfFFbk, 2 ) = delayLowPassFilter[2].fFFbk1;
	AKSIMD_GETELEMENT_V4F32( vfFFbk, 3 ) = delayLowPassFilter[3].fFFbk1;

	AKSIMD_V4F32 vfB0,vfA1;

	AKSIMD_GETELEMENT_V4F32( vfB0, 0 ) = delayLowPassFilter[0].fB0;
	AKSIMD_GETELEMENT_V4F32( vfB0, 1 ) = delayLowPassFilter[1].fB0;
	AKSIMD_GETELEMENT_V4F32( vfB0, 2 ) = delayLowPassFilter[2].fB0;
	AKSIMD_GETELEMENT_V4F32( vfB0, 3 ) = delayLowPassFilter[3].fB0;

	//Negate A1 so we can use MADD operations
	AKSIMD_GETELEMENT_V4F32( vfA1, 0 ) = -delayLowPassFilter[0].fA1;
	AKSIMD_GETELEMENT_V4F32( vfA1, 1 ) = -delayLowPassFilter[1].fA1;
	AKSIMD_GETELEMENT_V4F32( vfA1, 2 ) = -delayLowPassFilter[2].fA1;
	AKSIMD_GETELEMENT_V4F32( vfA1, 3 ) = -delayLowPassFilter[3].fA1;

	AkUInt32 uToProcess = in_uNumFrames;
	do
	{
		//Compute remaining samples in each delay lines before wrap
		AkUInt32 uProcessThisIter = uToProcess;
		for(AkUInt32 i = 0; i < 4; i++)
			uProcessThisIter = AkMin(uProcessThisIter, FDNDelayLine[i].uDelayLineLength - FDNDelayLine[i].uCurOffset);
		
		AkUInt32 uProcessThisIter4 = uProcessThisIter/4;

		for ( AkUInt32 i = 0; i < uProcessThisIter4; i++ )
		{
			AKSIMD_V4F32 vfDelay0, vfDelay1, vfDelay2, vfDelay3;

			ReadAndAttenuateDelays(vfDelay0, vfDelay1, vfDelay2, vfDelay3, vfFFbk, vfB0, vfA1);

			in_Output.SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);	
			FeedbackAndInputReinjection(vfDelay0, vfDelay1, vfDelay2, vfDelay3, pfInBuf);
		}

		//Complete the odd samples.
		for ( AkUInt32 i = uProcessThisIter4*4; i < uProcessThisIter; i++ )
		{
			AKSIMD_V4F32 vfDelayOut;

			ReadAndAttenuateDelays2(vfDelayOut, vfB0, vfFFbk, vfA1);

			// Sum to output buffer
#if defined(AK_CPU_ARM_NEON)
			AKSIMD_V2F32 vfPartialSum,vfPartialDiff;
			in_Output.SumToOutputBuffersSingle(vfDelayOut, vfPartialSum, vfPartialDiff);
			FeedbackAndReinjection2(vfPartialSum, vfDelayOut, pfInBuf);
#else
			in_Output.SumToOutputBuffersSingle(vfDelayOut);
			FeedbackAndReinjection2(vfDelayOut, pfInBuf);
#endif
		}

		for(AkUInt32 i = 0; i < 4; i++)
			FDNDelayLine[i].WrapDelayLine();

		uToProcess -= uProcessThisIter;
	}
	while ( uToProcess > 0 );

	delayLowPassFilter[0].fFFbk1 = AKSIMD_GETELEMENT_V4F32( vfFFbk, 0 );
	delayLowPassFilter[1].fFFbk1 = AKSIMD_GETELEMENT_V4F32( vfFFbk, 1 );
	delayLowPassFilter[2].fFFbk1 = AKSIMD_GETELEMENT_V4F32( vfFFbk, 2 );
	delayLowPassFilter[3].fFFbk1 = AKSIMD_GETELEMENT_V4F32( vfFFbk, 3 );
}


struct SingleOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf;

	AkForceInline void SumToOutputBuffers( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3 )
	{		
		AKSIMD_V4F32 vfOut = AKSIMD_LOADU_V4F32( pfOutBuf );
		AKSIMD_V4F32 vfSum0 = AKSIMD_ADD_V4F32( vfDelay0, vfDelay2 );
		AKSIMD_V4F32 vfSum1 = AKSIMD_ADD_V4F32( vfDelay1, vfDelay3 );
		vfOut = AKSIMD_ADD_V4F32( vfOut, vfSum0 );
		vfOut = AKSIMD_SUB_V4F32( vfOut, vfSum1 );
		AKSIMD_STOREU_V4F32( pfOutBuf, vfOut );
		pfOutBuf += 4;
	}

#if defined(AK_CPU_ARM_NEON)
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut, AKSIMD_V2F32 &vfPartialSum, AKSIMD_V2F32 &vfPartialDiff)
	{
		
		vfPartialSum = AKSIMD_ADD_V2F32( vget_low_f32(vfDelayOut), vget_high_f32(vfDelayOut) );	//A B + C D -> (A+C) (B+D)
		*pfOutBuf++ += AKSIMD_GETELEMENT_V2F32(vfPartialSum, 0) - AKSIMD_GETELEMENT_V2F32(vfPartialSum, 1);	 //(A+C)-(B+D)
	}
#else
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut)
	{
		const AKSIMD_V4F32 vfSigns = { 1.f, -1.f, 1.f, -1.f };
		AKSIMD_V4F32 vfSum = AKSIMD_DOTPRODUCT( vfDelayOut, vfSigns );
		*pfOutBuf++ += AKSIMD_GETELEMENT_V4F32( vfSum, 0 );
	}
#endif
};


//Derive from SingleOutput since the first buffer is treated the same way.  
//No, the functions are NOT virtual, the base class version will be hidden by the derived one.  
//Since the calling function is templated with the explicit child class, the child version of function will be called directly
//and everything will be inlined properly.
struct DualOutput : public SingleOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf2;

	AkForceInline void SumToOutputBuffers( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3 )
	{
		//Call base class first
		SingleOutput::SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);

		//Sum for the second output buffer
		AKSIMD_V4F32 vfOut = AKSIMD_LOADU_V4F32( pfOutBuf2 );
		AKSIMD_V4F32 vfSum0 = AKSIMD_ADD_V4F32( vfDelay0, vfDelay1 );
		AKSIMD_V4F32 vfSum1 = AKSIMD_ADD_V4F32( vfDelay2, vfDelay3 );
		vfOut = AKSIMD_ADD_V4F32( vfOut, vfSum0 );
		vfOut = AKSIMD_SUB_V4F32( vfOut, vfSum1 );
		AKSIMD_STOREU_V4F32( pfOutBuf2, vfOut );
		pfOutBuf2 += 4;
	}

#if defined(AK_CPU_ARM_NEON)
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut, AKSIMD_V2F32 &vfPartialSum, AKSIMD_V2F32 &vfPartialDiff)
	{
		SingleOutput::SumToOutputBuffersSingle(vfDelayOut, vfPartialSum, vfPartialDiff);
		vfPartialDiff = AKSIMD_SUB_V2F32( vget_low_f32(vfDelayOut), vget_high_f32(vfDelayOut) );	//A B - C D -> (A-C) (B-D)
		*pfOutBuf2++ += AKSIMD_GETELEMENT_V2F32(vfPartialDiff, 0) + AKSIMD_GETELEMENT_V2F32(vfPartialDiff, 1);	 //(A-C)+(B-D)
	}
#else
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut)
	{
		SingleOutput::SumToOutputBuffersSingle(vfDelayOut);
		const AKSIMD_V4F32 vfSigns = { 1.f, 1.f, -1.f, -1.f };
		AKSIMD_V4F32 vfSum = AKSIMD_DOTPRODUCT( vfDelayOut, vfSigns );
		*pfOutBuf2++ += AKSIMD_GETELEMENT_V4F32( vfSum, 0 );
	}
#endif
};

//Derive from SingleOutput since the first buffer is treated the same way.  
//No, the functions are NOT virtual, the base class version will be hidden by the derived one.  
//Since the calling function is templated with the explicit child class, the child version of function will be called directly
//and everything will be inlined properly.
struct TripleOutput : public DualOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf3;

	AkForceInline void SumToOutputBuffers( AKSIMD_V4F32 &vfDelay0, AKSIMD_V4F32 &vfDelay1, AKSIMD_V4F32 &vfDelay2, AKSIMD_V4F32 &vfDelay3 )
	{
		//Call base class first
		DualOutput::SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);

		//Sum for the second output buffer
		AKSIMD_V4F32 vfOut = AKSIMD_LOADU_V4F32( pfOutBuf3 );
		AKSIMD_V4F32 vfSum0 = AKSIMD_ADD_V4F32( vfDelay0, vfDelay3 );
		AKSIMD_V4F32 vfSum1 = AKSIMD_ADD_V4F32( vfDelay1, vfDelay2 );
		vfOut = AKSIMD_ADD_V4F32( vfOut, vfSum0 );
		vfOut = AKSIMD_SUB_V4F32( vfOut, vfSum1 );
		AKSIMD_STOREU_V4F32( pfOutBuf3, vfOut );
		pfOutBuf3 += 4;
	}

#if defined(AK_CPU_ARM_NEON)
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut, AKSIMD_V2F32 &vfPartialSum, AKSIMD_V2F32 &vfPartialDiff)
	{
		DualOutput::SumToOutputBuffersSingle(vfDelayOut, vfPartialSum, vfPartialDiff);
		*pfOutBuf3++ += AKSIMD_GETELEMENT_V2F32(vfPartialDiff, 0) - AKSIMD_GETELEMENT_V2F32(vfPartialDiff, 1);
	}
#else
	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V4F32 &vfDelayOut)
	{
		DualOutput::SumToOutputBuffersSingle(vfDelayOut);
		const AKSIMD_V4F32 vfSigns = { 1.f, -1.f, -1.f, 1.f };
		AKSIMD_V4F32 vfSum = AKSIMD_DOTPRODUCT( vfDelayOut, vfSigns );
		*pfOutBuf3++ += AKSIMD_GETELEMENT_V4F32( vfSum, 0 );
	}
#endif
};

void FDN4::ProcessBufferAccum(	
							  AkReal32 * in_pfInBuffer, 
							  AkReal32 * io_pfOutBuffer, 
							  AkUInt32 in_uNumFrames)
{
	SingleOutput out;
	out.pfOutBuf = io_pfOutBuffer;
	GenericProcessBuffer<SingleOutput>(in_pfInBuffer, in_uNumFrames, out);
}

void FDN4::ProcessBufferAccum(	
							  AkReal32 * in_pfInBuffer, 
							  AkReal32 * io_pfOutBuffer1,
							  AkReal32 * io_pfOutBuffer2, 
							  AkUInt32 in_uNumFrames)
{
	DualOutput out;
	out.pfOutBuf = io_pfOutBuffer1;
	out.pfOutBuf2 = io_pfOutBuffer2;
	GenericProcessBuffer<DualOutput>(in_pfInBuffer, in_uNumFrames, out);
}

void FDN4::ProcessBufferAccum(	
							  AkReal32 * in_pfInBuffer, 
							  AkReal32 * io_pfOutBuffer1, 
							  AkReal32 * io_pfOutBuffer2, 
							  AkReal32 * io_pfOutBuffer3, 
							  AkUInt32 in_uNumFrames)
{
	TripleOutput out;
	out.pfOutBuf = io_pfOutBuffer1;
	out.pfOutBuf2 = io_pfOutBuffer2;
	out.pfOutBuf3 = io_pfOutBuffer3;
	GenericProcessBuffer<TripleOutput>(in_pfInBuffer, in_uNumFrames, out);
}