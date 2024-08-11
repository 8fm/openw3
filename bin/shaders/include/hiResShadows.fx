Texture2D<float>			HiResShadowmapTexture		: register(t11);
SamplerComparisonState		HiResShadowmapCmpSampler	: register(s9);

static const float2 HiResShadowPoissonDiskSamples[12] =
{ 
	float2(-0.5077208f, -0.6170899f),
	float2(-0.05154902f, -0.7508098f),
	float2(-0.7641042f, 0.2009152f),
	float2(0.007284774f, -0.205947f),
	float2(-0.366204f, 0.7323797f),
	float2(-0.9239582f, -0.2559643f),
	float2(-0.286924f, 0.278053f),
	float2(0.7723012f, 0.1573329f),
	float2(0.378513f, 0.4052199f),
	float2(0.4630217f, -0.6914619f),
	float2(0.423523f, 0.863028f),
	float2(0.8558643f, -0.3382554f),
}; 

float CalcHiResShadows( in float3 worldSpacePosition )
{
	// transform to shadow space
	float4 pos = mul( float4(worldSpacePosition,1), PSC_Custom_Matrix );
		
	// shadow bias
	// gpu_assembly_opts: 3 less movs
	//pos.z -= PSC_Custom_HiResShadowsParams.y;

	// normalize
	const float2 uv = float2( 0.5f, 0.5f ) + pos.xy * float2( 0.5f, -0.5f );
	const float zReceiver = pos.z - PSC_Custom_HiResShadowsParams.y;

	// 12 poisson taps
	float test = 0;
	{
		const float scale = PSC_Custom_HiResShadowsParams.x;

		// rotation matrix
		// TODO: not enough slots for this...
		#if 0
			int2 pixelPos = (int2)( pixelCoord.xy * screenDimensions.xy );
			const float2 rotA = 2.0f * NoiseTexture[ pixelPos & 31 ].xy - 1.0f;
			const float2 rotB = float2( -rotA.y, rotA.x );
		#else
			const float2 rotA = float2( 1.0f, 0.0f );
			const float2 rotB = float2( 0.0f, 1.0f );
		#endif

		[unroll]
		for ( int i=0; i<12; ++i )
		{
			// rotate sample position
			float2 rotDelta;
			rotDelta.x = dot( HiResShadowPoissonDiskSamples[i].xy, rotA );
			rotDelta.y = dot( HiResShadowPoissonDiskSamples[i].xy, rotB );

			// sample with rotated poisson disk
			float2 pos = uv.xy + rotDelta.xy * scale;
			test += SAMPLE_CMP_LEVEL0( HiResShadowmapTexture, HiResShadowmapCmpSampler, pos.xy, zReceiver );
		}
		test = pow( test/12.0f, 2 );
	}

	// 
	return test;
}
