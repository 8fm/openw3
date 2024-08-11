#include "common.fx"
#include "commonCS.fx"

// This stuff is needed on PC only.
// On XOne not needed because we're running 64 threads per group, 
// but enabling this doesn't degrade performance.
#define ENABLE_HISTOGRAM_BARRIERS		1

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#define THREADS_NUM		(64)
#define NUMBINS			(64 * HISTOGRAM_CAPACITY_MULTIPLIER)

#if IS_HISTOGRAM_PREPARE
	RW_STRUCTBUFFER(uint)	g_GlobalHistogram	: register(u0);
	Texture2D<float4> 		Input 				: register(t0);
	Texture2D<float> 		DepthInput			: register(t1);
#endif

#if IS_HISTOGRAM_GATHER
	RW_STRUCTBUFFER(uint)	g_GlobalHistogram	: register(u0);
	RW_TEXTURE2D<float>		OutputTexture 		: register(u1);
#endif

GROUPSHARED uint			LocalHistogram[ NUMBINS ];

#if HAS_DEBUG_HISTORGRAM_OUTPUT
	RW_TEXTURE2D<float4> OutputDebugTexture 	: register(u2);
#endif

CS_CUSTOM_CONSTANT_BUFFER
	float4 histogramParams;
	float4 fullScreenSize;
	float4 skyLumParams;
END_CS_CUSTOM_CONSTANT_BUFFER

/////////////////////////////////////////////////////////////////////////////////////////////////////////

uint GetBucketByLuminance( float luminance )
{
	uint bucket = (uint)( log(1.0f + luminance) * (0.5f * NUMBINS) );			
	return clamp( bucket, 0, (uint)NUMBINS - 1 );
}

float GetLuminanceByBucket( uint bucket )
{
	return exp((((float)bucket + 0.5) / (NUMBINS/2)))- 1.0f;			
}

#if IS_HISTOGRAM_PREPARE
	float CalcLumForPixelCoord( uint2 pixelCoord )
	{
		float4 color = Input[pixelCoord];			
		float luminance = dot(color.xyz, RGB_LUMINANCE_WEIGHTS_LINEAR_Histogram );
		luminance = lerp( luminance, skyLumParams.x, DepthInput[pixelCoord*(int)skyLumParams.z] == skyLumParams.w ? skyLumParams.y : 0 );		// ace_fix: this is far from being accurate, but this is for a quick test only ATM
		return luminance;
	}
#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_HISTOGRAM_PREPARE

[NUMTHREADS(THREADS_NUM, 1, 1)]
void cs_main(uint3 GroupID : SYS_GROUP_ID, uint3 Thread : SYS_GROUP_THREAD_ID )
{
	// Zero out our values.
	[unroll]
	for ( int i=0; i<HISTOGRAM_CAPACITY_MULTIPLIER; ++i )
	{
		int off = i * THREADS_NUM;
		LocalHistogram[ off + Thread.x ] = 0;
	}

	#if ENABLE_HISTOGRAM_BARRIERS
		GROUP_BARRIER_GROUP_SYNC;
	#endif

	// GroupID.x gives us the row.
	// i gives us the column (pixel)
	for( uint i=0; i<histogramParams.x; i+=THREADS_NUM )
	{		
		uint2 location = uint2( i + Thread.x, GroupID.x );
		if ( location.x < histogramParams.x )
		{
			float intensity = CalcLumForPixelCoord( location );
			int Bin = GetBucketByLuminance( intensity );
			uint decoy;
			INTERLOCKED_ADD( LocalHistogram[ Bin ], 1, decoy );
		}
	}

	#if ENABLE_HISTOGRAM_BARRIERS
		GROUP_BARRIER_GROUP_SYNC;
	#endif

	//
	[unroll]
	for ( int i=0; i<HISTOGRAM_CAPACITY_MULTIPLIER; ++i )
	{
		int off = i * THREADS_NUM;
		uint decoy;
		INTERLOCKED_ADD( g_GlobalHistogram[ off + Thread.x ], LocalHistogram[ off + Thread.x ], decoy );
	}
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_HISTOGRAM_GATHER && HAS_DEBUG_HISTORGRAM_OUTPUT

void OutputDebugHistogram( int firstBucketCounterValue, int bucketCounter )
{	
	/*
	for ( int i=0; i<histogramScreenSize.x; ++i )
	for ( int j=0; j<histogramScreenSize.y; ++j )
	{
		//OutputDebugTexture[ int2(i,j) ] = float4 ( float3(1,1,1) * Input[ int2(i,j) ], 1 );
		OutputDebugTexture[ int2(i,j) ] = float4 ( float3(1,1,1) * CalcLumForPixelCoord( int2(i,j) ), 1 );				
	}
	*/
			
	const int offset_x = 10;
	const int offset_y = 20;	

	const int bucketIndexDivider = 4096 / NUMBINS;
	const int numBucketsTotal = 4096;
	const int valueDivider = 2 * 64 / HISTOGRAM_CAPACITY_MULTIPLIER;
	const int numBucketsInColum = 16;

	const int numColumns = (numBucketsTotal + numBucketsInColum - 1) / numBucketsInColum;
	uint prevValue = 0;
	for ( int col_i=0; col_i<numColumns; ++col_i )
	{
		uint currValue = 0;
		uint currNum = 0;
		for ( int i=0; i<numBucketsInColum; ++i )
		{
			int currBucketIdx = col_i * numBucketsInColum + i;
			if ( currBucketIdx >= numBucketsTotal )
			{
				break;
			}

			currValue += LocalHistogram[currBucketIdx / bucketIndexDivider];
			currNum += 1;
		}

		currValue = ceil( currValue / ((float)max( 1, currNum ) * (float)valueDivider) );

		int2 colBaseCoord = int2( offset_x + col_i, (int)fullScreenSize.y - offset_y );
		{
			bool isLeftSide = col_i*numBucketsInColum < firstBucketCounterValue*bucketIndexDivider;
			bool isRightSide = col_i*numBucketsInColum > bucketCounter*bucketIndexDivider;
			if ( isLeftSide || isRightSide )
			{
				for ( int i=colBaseCoord.y+1; i<fullScreenSize.y; ++i )
				{
					OutputDebugTexture[ int2(colBaseCoord.x, i) ] = float4( 1, 0, (isRightSide ? 1 : 0), 1 );
				}
			}
		}

		if ( colBaseCoord.x >= fullScreenSize.x )
		{
			break;
		}

		for ( uint value_i=0; value_i<=currValue; ++value_i )
		{
			int2 currCoord = int2( colBaseCoord.x, colBaseCoord.y - (int)value_i );
			if ( currCoord.y < 0 )
			{
				continue;
			}

			if ( 0 == value_i )
			{
				OutputDebugTexture[ currCoord ] = float4( 1, 1, 0, 1 );
			}
			else
			{
				OutputDebugTexture[ currCoord ] = float4( 1, 1, 1, 1 );
			}
		}
				
		if ( prevValue <= currValue )
		{
			for ( int i=prevValue; i<=currValue; ++i )
			{
				int2 currCoord = int2( colBaseCoord.x - 1, colBaseCoord.y - i - 1 );
				if ( currCoord.x >= 0 && currCoord.y >= 0 )
				{
					OutputDebugTexture[ currCoord ] = float4( 0, 0, 0, 1 );
				}
			}

			{
				int2 currCoord = int2( colBaseCoord.x, colBaseCoord.y - currValue - 1 );
				if ( currCoord.x >= 0 && currCoord.y >= 0 )
				{
					OutputDebugTexture[ currCoord ] = float4( 0, 0, 0, 1 );
				}
			}
		}
		else
		{
			for ( int i=prevValue; i>=(int)currValue; --i )
			{
				int2 currCoord = int2( colBaseCoord.x, colBaseCoord.y - i - 1 );
				if ( currCoord.x >= 0 && currCoord.y >= 0 )
				{
					OutputDebugTexture[ currCoord ] = float4( 0, 0, 0, 1 );
				}
			}
		}

		prevValue = currValue;
	}
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_HISTOGRAM_GATHER

[NUMTHREADS(THREADS_NUM, 1, 1)]
void cs_main( uint3 Thread : SYS_GROUP_THREAD_ID )
{
	const float2 histogramScreenSize	= histogramParams.xy;
	const float rejectThreshold0 		= histogramParams.z;
	const float rejectThreshold1 		= histogramParams.w;
	
	// Load histogram into local memory for faster access
	[unroll]
	for ( int i=0; i<HISTOGRAM_CAPACITY_MULTIPLIER; ++i )
	{
		int off = i * THREADS_NUM;
		int valueIdx = off + Thread.x;
		LocalHistogram[ valueIdx ] = g_GlobalHistogram[ valueIdx ];
	}

	#if ENABLE_HISTOGRAM_BARRIERS
		GROUP_BARRIER_GROUP_SYNC;
	#endif

	// Determine key luminance
	if ( 0 == Thread.x )
	{
		uint PixelCount = histogramScreenSize.x * histogramScreenSize.y;

		const int desiredCount0 = clamp( (int)(((float)PixelCount) * rejectThreshold0), 0, (int)PixelCount - 1 );
		const int desiredCount1 = clamp( (int)(((float)PixelCount) * rejectThreshold1), desiredCount0, (int)PixelCount - 1 );

		int accum = 0;
		int bucketCounter = 0;
		{
			bool finished = false;
			while ( !finished ) //&& bucketCounter < NUMBINS )
			{
				const uint currCount = LocalHistogram[bucketCounter];
				
				const int new_accum = accum + currCount;
				if ( new_accum > desiredCount0 )
				{
					finished = true;
				}
				else
				{
					bucketCounter += 1;
					accum = new_accum;
				}
			}
		}

	#if HAS_DEBUG_HISTORGRAM_OUTPUT
		const int firstBucketCounterValue = bucketCounter;
	#endif
		
		float output = 0;
		{
			bool finished = false;
			const int orig_accum = accum;
			while ( !finished ) //&& bucketCounter < NUMBINS )
			{
				const uint currCount = LocalHistogram[bucketCounter];				

				accum  += currCount;
				output += (float)currCount * GetLuminanceByBucket( bucketCounter );
				if ( accum > desiredCount1 )
				{
					output /= (float)(accum - orig_accum);
					finished = true;
				}
				else
				{
					bucketCounter += 1;
				}
			}
		}

		OutputTexture[uint2(0,0)] = output;

	#if HAS_DEBUG_HISTORGRAM_OUTPUT
		OutputDebugHistogram( firstBucketCounterValue, bucketCounter );
	#endif
	}
}

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////////////
