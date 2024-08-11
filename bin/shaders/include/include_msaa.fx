#ifndef MSAA_INCLUDED
#define MSAA_INCLUDED

#if MSAA_NUM_SAMPLES > 1

	uint2 GetMSAABufferSampleCoord(uint2 coords, uint sampleIndex)
	{
		return coords * uint2(MSAA_NUM_SAMPLES_X, MSAA_NUM_SAMPLES_Y) + uint2( sampleIndex % MSAA_NUM_SAMPLES_X, sampleIndex / MSAA_NUM_SAMPLES_X );
		//uint2 mFramebufferDimensions = uint2( 1920, 1080 );
		//return coords + uint2( sampleIndex % MSAA_NUM_SAMPLES_X, sampleIndex / MSAA_NUM_SAMPLES_X ) * mFramebufferDimensions;
	}

#else

	uint2 GetMSAABufferSampleCoord(uint2 coords, uint sampleIndex)
	{
		uint2 dummy = uint2(sampleIndex, sampleIndex);
		return coords+dummy-dummy;
	}

#endif

#endif
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
