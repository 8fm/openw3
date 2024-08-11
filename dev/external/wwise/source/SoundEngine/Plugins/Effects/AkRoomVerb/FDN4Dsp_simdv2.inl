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

AkForceInline void FDN4::ReadAndAttenuateDelays( AKSIMD_V2F32 &vfDelay0, AKSIMD_V2F32 &vfDelay1, AKSIMD_V2F32 &vfDelay2, AKSIMD_V2F32 &vfDelay3, 
							AKSIMD_V2F32 &vfDelay0_, AKSIMD_V2F32 &vfDelay1_, AKSIMD_V2F32 &vfDelay2_, AKSIMD_V2F32 &vfDelay3_,
							AKSIMD_V2F32 &vfFFbk1, AKSIMD_V2F32 vfB0, AKSIMD_V2F32 vfA1, AKSIMD_V2F32 &vfFFbk1_, AKSIMD_V2F32 vfB0_, AKSIMD_V2F32 vfA1_)
{
#if defined(AK_IOS) || defined(AK_VITA)
	AKSIMD_V2F32 vfTime0, vfTime1, vfTime2, vfTime3;
	AKSIMD_V2F32 vfTime0_, vfTime1_, vfTime2_, vfTime3_;
/*
	FDNDelayLine[0].ReadSamples<0>( vfTime0, vfTime1, vfTime2, vfTime3 );
	FDNDelayLine[1].ReadSamples<1>( vfTime0, vfTime1, vfTime2, vfTime3 );
	FDNDelayLine[2].ReadSamples<0>( vfTime0_, vfTime1_, vfTime2_, vfTime3_ );
	FDNDelayLine[3].ReadSamples<1>( vfTime0_, vfTime1_, vfTime2_, vfTime3_ );
*/
	FDNDelayLine[0].ReadSamples(vfDelay0, vfDelay0_); // A0 A1 A2 A3
	FDNDelayLine[1].ReadSamples(vfDelay1, vfDelay1_); // B0 B1 B2 B3
	FDNDelayLine[2].ReadSamples(vfDelay2, vfDelay2_); // C0 C1 C2 C3
	FDNDelayLine[3].ReadSamples(vfDelay3, vfDelay3_); // D0 D1 D2 D3

	AKSIMD_V2F32X2 vTrns1 = AKSIMD_TRANSPOSE_V2F32( vfDelay0, vfDelay1 );
	AKSIMD_V2F32X2 vTrns2 = AKSIMD_TRANSPOSE_V2F32( vfDelay2, vfDelay3 );
	vfTime0  = vTrns1.val[0];
	vfTime1  = vTrns1.val[1];
	vfTime0_ = vTrns2.val[0];
	vfTime1_ = vTrns2.val[1];

	vTrns1 = AKSIMD_TRANSPOSE_V2F32( vfDelay0_, vfDelay1_ );
	vTrns2 = AKSIMD_TRANSPOSE_V2F32( vfDelay2_, vfDelay3_ );
	vfTime2  = vTrns1.val[0];
	vfTime3  = vTrns1.val[1];
	vfTime2_ = vTrns2.val[0];
	vfTime3_ = vTrns2.val[1];
#else
	FDNDelayLine[0].ReadSamples(vfDelay0, vfDelay0_); // A0 A1 A2 A3
	FDNDelayLine[1].ReadSamples(vfDelay1, vfDelay1_); // B0 B1 B2 B3
	FDNDelayLine[2].ReadSamples(vfDelay2, vfDelay2_); // C0 C1 C2 C3
	FDNDelayLine[3].ReadSamples(vfDelay3, vfDelay3_); // D0 D1 D2 D3

	AKSIMD_V2F32 vfTime0 = AKSIMD_UNPACKLO_V2F32(vfDelay0, vfDelay1);	//A0 B0		//Rotate matrix
	AKSIMD_V2F32 vfTime0_ = AKSIMD_UNPACKLO_V2F32(vfDelay2, vfDelay3);	//C0 D0		//Rotate matrix
	AKSIMD_V2F32 vfTime1 = AKSIMD_UNPACKHI_V2F32(vfDelay0, vfDelay1);	//A1 B1		//Rotate matrix
	AKSIMD_V2F32 vfTime1_ = AKSIMD_UNPACKHI_V2F32(vfDelay2, vfDelay3);	//C1 D1		//Rotate matrix
	AKSIMD_V2F32 vfTime2 = AKSIMD_UNPACKLO_V2F32(vfDelay0_, vfDelay1_);	//A2 B2		//Rotate matrix
	AKSIMD_V2F32 vfTime2_ = AKSIMD_UNPACKLO_V2F32(vfDelay2_, vfDelay3_);//C2 D2		//Rotate matrix
	AKSIMD_V2F32 vfTime3 = AKSIMD_UNPACKHI_V2F32(vfDelay0_, vfDelay1_);	//A3 B3		//Rotate matrix
	AKSIMD_V2F32 vfTime3_ = AKSIMD_UNPACKHI_V2F32(vfDelay2_, vfDelay3_);//C3 D3		//Rotate matrix
#endif

	//Apply low pass filter
	vfTime0 = AKSIMD_MUL_V2F32(vfTime0, vfB0);
	vfTime0 = AKSIMD_MADD_V2F32(vfFFbk1, vfA1, vfTime0);
	vfTime0_ = AKSIMD_MUL_V2F32(vfTime0_, vfB0_);
	vfTime0_ = AKSIMD_MADD_V2F32(vfFFbk1_, vfA1_, vfTime0_);

	vfTime1 = AKSIMD_MUL_V2F32(vfTime1, vfB0);
	vfTime1 = AKSIMD_MADD_V2F32(vfTime0, vfA1, vfTime1);
	vfTime1_ = AKSIMD_MUL_V2F32(vfTime1_, vfB0_);
	vfTime1_ = AKSIMD_MADD_V2F32(vfTime0_, vfA1_, vfTime1_);

	vfTime2 = AKSIMD_MUL_V2F32(vfTime2, vfB0);
	vfTime2 = AKSIMD_MADD_V2F32(vfTime1, vfA1, vfTime2);
	vfTime2_ = AKSIMD_MUL_V2F32(vfTime2_, vfB0_);
	vfTime2_ = AKSIMD_MADD_V2F32(vfTime1_, vfA1_, vfTime2_);

	vfTime3 = AKSIMD_MUL_V2F32(vfTime3, vfB0);
	vfTime3 = AKSIMD_MADD_V2F32(vfTime2, vfA1, vfTime3);
	vfTime3_ = AKSIMD_MUL_V2F32(vfTime3_, vfB0_);
	vfTime3_ = AKSIMD_MADD_V2F32(vfTime2_, vfA1_, vfTime3_);

	vfFFbk1 = vfTime3;
	vfFFbk1_ = vfTime3_;

	//Rotate back the matrix
#if defined(AK_IOS) || defined(AK_VITA)
	AKSIMD_V2F32X2 vTemp1 = AKSIMD_TRANSPOSE_V2F32( vfTime0, vfTime1 );
	AKSIMD_V2F32X2 vTemp2 = AKSIMD_TRANSPOSE_V2F32( vfTime2, vfTime3 );
	vfDelay0  = vTemp1.val[0];
	vfDelay1  = vTemp1.val[1];
	vfDelay0_ = vTemp2.val[0];
	vfDelay1_ = vTemp2.val[1];

	vTemp1 = AKSIMD_TRANSPOSE_V2F32( vfTime0_, vfTime1_ );
	vTemp2 = AKSIMD_TRANSPOSE_V2F32( vfTime2_, vfTime3_ );
	vfDelay2  = vTemp1.val[0];
	vfDelay3  = vTemp1.val[1];
	vfDelay2_ = vTemp2.val[0];
	vfDelay3_ = vTemp2.val[1];
#else
	vfDelay0 = AKSIMD_UNPACKLO_V2F32(vfTime0, vfTime1);	//A0 A1
	vfDelay1 = AKSIMD_UNPACKHI_V2F32(vfTime0, vfTime1);	//B0 B1
	vfDelay2 = AKSIMD_UNPACKLO_V2F32(vfTime0_, vfTime1_); //C0 C1
	vfDelay3 = AKSIMD_UNPACKHI_V2F32(vfTime0_, vfTime1_); //D0 D1
	vfDelay0_ = AKSIMD_UNPACKLO_V2F32(vfTime2, vfTime3);//A2 A3
	vfDelay1_ = AKSIMD_UNPACKHI_V2F32(vfTime2, vfTime3);//B2 B3
	vfDelay2_ = AKSIMD_UNPACKLO_V2F32(vfTime2_, vfTime3_);//C2 C3
	vfDelay3_ = AKSIMD_UNPACKHI_V2F32(vfTime2_, vfTime3_);//D2 D3
#endif
}

AkForceInline void FDN4::FeedbackAndInputReinjection( AKSIMD_V2F32 vfDelay0, AKSIMD_V2F32 vfDelay1, AKSIMD_V2F32 vfDelay2, AKSIMD_V2F32 vfDelay3, AkReal32 * AK_RESTRICT &pfInBuf )
{
	const AKSIMD_V2F32 vfHalf = {-0.5f, -0.5f};

	// Feedback matrix
	AKSIMD_V2F32 vfScaledDelaySum = vfDelay0;
	vfScaledDelaySum = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay1);
	vfScaledDelaySum = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay2);
	vfScaledDelaySum = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay3);
	vfScaledDelaySum = AKSIMD_MUL_V2F32(vfScaledDelaySum, vfHalf);

	AKSIMD_V2F32 vfFeedback0 = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay1);
	AKSIMD_V2F32 vfFeedback1 = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay2);
	AKSIMD_V2F32 vfFeedback2 = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay3);
	AKSIMD_V2F32 vfFeedback3 = AKSIMD_ADD_V2F32(vfScaledDelaySum, vfDelay0);

	// Input reinjection
	AKSIMD_V2F32 vfIn = AKSIMD_LOAD_V2F32(pfInBuf);
	pfInBuf += 2;

	vfFeedback0 = AKSIMD_ADD_V2F32(vfFeedback0, vfIn);
	vfFeedback1 = AKSIMD_ADD_V2F32(vfFeedback1, vfIn);
	vfFeedback2 = AKSIMD_ADD_V2F32(vfFeedback2, vfIn);
	vfFeedback3 = AKSIMD_ADD_V2F32(vfFeedback3, vfIn);

	FDNDelayLine[0].WriteSamplesNoWrapCheck( vfFeedback0 );
	FDNDelayLine[1].WriteSamplesNoWrapCheck( vfFeedback1 );
	FDNDelayLine[2].WriteSamplesNoWrapCheck( vfFeedback2 );
	FDNDelayLine[3].WriteSamplesNoWrapCheck( vfFeedback3 );
}

AkForceInline void FDN4::ReadAndAttenuateDelays2( AKSIMD_V2F32 &vfDelayOut, AKSIMD_V2F32 &vfDelayOut_, AKSIMD_V2F32 vfB0, AKSIMD_V2F32 &vfFFbk1, AKSIMD_V2F32 vfA1, AKSIMD_V2F32 vfB0_, AKSIMD_V2F32 &vfFFbk1_, AKSIMD_V2F32 vfA1_ )
{
	vfDelayOut[0] = FDNDelayLine[0].ReadSample( );
	vfDelayOut[1] = FDNDelayLine[1].ReadSample( );
	vfDelayOut_[0] = FDNDelayLine[2].ReadSample( );
	vfDelayOut_[1] = FDNDelayLine[3].ReadSample( );		

	// Low-pass attenuation
	vfDelayOut = AKSIMD_MUL_V2F32(vfDelayOut, vfB0);
	vfDelayOut = AKSIMD_MADD_V2F32(vfFFbk1, vfA1, vfDelayOut);
	vfFFbk1 = vfDelayOut;

	vfDelayOut_ = AKSIMD_MUL_V2F32(vfDelayOut_, vfB0_);
	vfDelayOut_ = AKSIMD_MADD_V2F32(vfFFbk1_, vfA1_, vfDelayOut_);
	vfFFbk1_ = vfDelayOut_;
}

AkForceInline void FDN4::FeedbackAndReinjection2( AKSIMD_V2F32 vfPartialSum, AKSIMD_V2F32 &vfDelayOut, AKSIMD_V2F32 &vfDelayOut_, AkReal32 * AK_RESTRICT &pfInBuf)
{
	// Feedback matrix
	AKSIMD_V2F32 vfScaledDelaySum = AKSIMD_SET_V2F32(-0.5f*(vfPartialSum[0]+vfPartialSum[1]));
	vfDelayOut = AKSIMD_ADD_V2F32(vfDelayOut, vfScaledDelaySum);
	vfDelayOut_ = AKSIMD_ADD_V2F32(vfDelayOut_, vfScaledDelaySum);

	//Rotate left. A B C D -> B C D A
	AKSIMD_V2F32 vfFeedback = AKSIMD_HILO_V2F32(vfDelayOut, vfDelayOut_);  

	// Input reinjection
	AKSIMD_V2F32 vfIn = AKSIMD_SET_V2F32(*pfInBuf++);
	vfFeedback = AKSIMD_ADD_V2F32(vfFeedback, vfIn);

	FDNDelayLine[0].WriteSampleNoWrapCheck( vfFeedback[0] );
	FDNDelayLine[1].WriteSampleNoWrapCheck( vfFeedback[1] );

	//Rotate left. A B C D -> B C D A
	vfFeedback = AKSIMD_HILO_V2F32(vfDelayOut_, vfDelayOut);  

	// Input reinjection
	vfFeedback = AKSIMD_ADD_V2F32(vfFeedback, vfIn);

	FDNDelayLine[2].WriteSampleNoWrapCheck( vfFeedback[0] );
	FDNDelayLine[3].WriteSampleNoWrapCheck( vfFeedback[1] );
}

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

	AKSIMD_V2F32 vfFFbk1,vfB0,vfA1;
	AKSIMD_V2F32 vfFFbk1_,vfB0_,vfA1_;

	vfFFbk1[0] = delayLowPassFilter[0].fFFbk1;
	vfFFbk1[1] = delayLowPassFilter[1].fFFbk1;
	vfFFbk1_[0] = delayLowPassFilter[2].fFFbk1;
	vfFFbk1_[1] = delayLowPassFilter[3].fFFbk1;

	vfB0[0] = delayLowPassFilter[0].fB0;
	vfB0[1] = delayLowPassFilter[1].fB0;
	vfB0_[0] = delayLowPassFilter[2].fB0;
	vfB0_[1] = delayLowPassFilter[3].fB0;

	//Negate A1 so we can use MADD operations
	vfA1[0] = -delayLowPassFilter[0].fA1;
	vfA1[1] = -delayLowPassFilter[1].fA1;
	vfA1_[0] = -delayLowPassFilter[2].fA1;
	vfA1_[1] = -delayLowPassFilter[3].fA1;

#if defined(AK_WIIU)
	for(AkUInt32 i = 0; i < 4; i++)
		FDNDelayLine[i].InitCache(in_uNumFrames);
#endif

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
			AKSIMD_V2F32 vfDelay0 ; 
			AKSIMD_V2F32 vfDelay0_;
			AKSIMD_V2F32 vfDelay1 ;
			AKSIMD_V2F32 vfDelay1_;
			AKSIMD_V2F32 vfDelay2 ;
			AKSIMD_V2F32 vfDelay2_;
			AKSIMD_V2F32 vfDelay3 ;
			AKSIMD_V2F32 vfDelay3_;

			ReadAndAttenuateDelays(vfDelay0, vfDelay1, vfDelay2, vfDelay3, vfDelay0_, vfDelay1_, vfDelay2_, vfDelay3_, 
				vfFFbk1, vfB0, vfA1, vfFFbk1_,vfB0_,vfA1_);

			//Low part
			in_Output.SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);	
			FeedbackAndInputReinjection(vfDelay0, vfDelay1, vfDelay2, vfDelay3, pfInBuf);

			//High part
			in_Output.SumToOutputBuffers(vfDelay0_, vfDelay1_, vfDelay2_, vfDelay3_);
			FeedbackAndInputReinjection(vfDelay0_, vfDelay1_, vfDelay2_, vfDelay3_, pfInBuf);	
		}

		//Complete the odd samples.
		for ( AkUInt32 i = uProcessThisIter4*4; i < uProcessThisIter; i++ )
		{
			AKSIMD_V2F32 vfDelayOut;
			AKSIMD_V2F32 vfDelayOut_;

			ReadAndAttenuateDelays2(vfDelayOut, vfDelayOut_, vfB0, vfFFbk1, vfA1, vfB0_, vfFFbk1_, vfA1_);

			// Sum to output buffer
			AKSIMD_V2F32 vfPartialSum;
			in_Output.SumToOutputBuffersSingle(vfDelayOut, vfDelayOut_, vfPartialSum);
			FeedbackAndReinjection2(vfPartialSum, vfDelayOut, vfDelayOut_, pfInBuf);
		}

		for(AkUInt32 i = 0; i < 4; i++)
			FDNDelayLine[i].WrapDelayLine();

		uToProcess -= uProcessThisIter;
	}
	while ( uToProcess > 0 );

#if defined(AK_WIIU)
	for(AkUInt32 i = 0; i < 4; i++)
	{
		FDNDelayLine[i].WriteDelayData();
		FDNDelayLine[i].TermCache();
	}
#endif

	delayLowPassFilter[0].fFFbk1 = vfFFbk1[0];
	delayLowPassFilter[1].fFFbk1 = vfFFbk1[1];
	delayLowPassFilter[2].fFFbk1 = vfFFbk1_[0];
	delayLowPassFilter[3].fFFbk1 = vfFFbk1_[1];
}


struct SingleOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf;

	AkForceInline void SumToOutputBuffers( AKSIMD_V2F32 vfDelay0, AKSIMD_V2F32 vfDelay1, AKSIMD_V2F32 vfDelay2, AKSIMD_V2F32 vfDelay3 )
	{		
		AKSIMD_V2F32 vfOut1 = AKSIMD_LOAD_V2F32( pfOutBuf );
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay0);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay1);
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay2);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay3);
		AKSIMD_STORE_V2F32( pfOutBuf, vfOut1 );

		pfOutBuf += 2;
	}

	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V2F32 vfDelayOut, AKSIMD_V2F32 vfDelayOut_, AKSIMD_V2F32& vfPartialSum)
	{
		vfPartialSum = AKSIMD_ADD_V2F32(vfDelayOut, vfDelayOut_);	//A B + C D -> (A+C) (B+D)
		*pfOutBuf++ += vfPartialSum[0]-vfPartialSum[1];	 //(A+C)-(B+D)
	}
};


//Derive from SingleOutput since the first buffer is treated the same way.  
//No, the functions are NOT virtual, the base class version will be hidden by the derived one.  
//Since the calling function is templated with the explicit child class, the child version of function will be called directly
//and everything will be inlined properly.
struct DualOutput : public SingleOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf2;

	AkForceInline void SumToOutputBuffers( AKSIMD_V2F32 vfDelay0, AKSIMD_V2F32 vfDelay1, AKSIMD_V2F32 vfDelay2, AKSIMD_V2F32 vfDelay3 )
	{
		//Call base class first
		SingleOutput::SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);

		//Sum for the second output buffer
		AKSIMD_V2F32 vfOut1 = AKSIMD_LOAD_V2F32( pfOutBuf2 );
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay0);
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay1);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay2);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay3);
		AKSIMD_STORE_V2F32( pfOutBuf2, vfOut1 );
		pfOutBuf2 += 2;
	}

	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V2F32 vfDelayOut, AKSIMD_V2F32 vfDelayOut_, AKSIMD_V2F32& vfPartialSum)
	{
		SingleOutput::SumToOutputBuffersSingle(vfDelayOut, vfDelayOut_, vfPartialSum);

		AKSIMD_V2F32 vfPartialDiff = AKSIMD_SUB_V2F32(vfDelayOut, vfDelayOut_);	//A B - C D -> (A-C) (B-D)
		*pfOutBuf2++ += vfPartialDiff[0]+vfPartialDiff[1];	 //(A-C)+(B-D)
	}
};

//Derive from SingleOutput since the first buffer is treated the same way.  
//No, the functions are NOT virtual, the base class version will be hidden by the derived one.  
//Since the calling function is templated with the explicit child class, the child version of function will be called directly
//and everything will be inlined properly.
struct TripleOutput : public DualOutput
{
	AkReal32 * AK_RESTRICT pfOutBuf3;

	AkForceInline void SumToOutputBuffers( AKSIMD_V2F32 vfDelay0, AKSIMD_V2F32 vfDelay1, AKSIMD_V2F32 vfDelay2, AKSIMD_V2F32 vfDelay3 )
	{
		//Call base class first
		DualOutput::SumToOutputBuffers(vfDelay0, vfDelay1, vfDelay2, vfDelay3);

		//Sum for the second output buffer
		AKSIMD_V2F32 vfOut1 = AKSIMD_LOAD_V2F32( pfOutBuf3 );
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay0);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay1);
		vfOut1 = AKSIMD_SUB_V2F32(vfOut1, vfDelay2);
		vfOut1 = AKSIMD_ADD_V2F32(vfOut1, vfDelay3);
		AKSIMD_STORE_V2F32( pfOutBuf3, vfOut1 );
		pfOutBuf3 += 2;
	}

	AkForceInline void SumToOutputBuffersSingle(AKSIMD_V2F32 vfDelayOut, AKSIMD_V2F32 vfDelayOut_, AKSIMD_V2F32& vfPartialSum)
	{
		DualOutput::SumToOutputBuffersSingle(vfDelayOut, vfDelayOut_, vfPartialSum);
		AKSIMD_V2F32 vfPartialDiff = AKSIMD_SUB_V2F32(vfDelayOut, vfDelayOut_);	//A B - C D -> (A-C) (B-D)
		*pfOutBuf3++ += vfPartialDiff[0]-vfPartialDiff[1];
	}
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