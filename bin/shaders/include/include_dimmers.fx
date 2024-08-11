
#ifndef DIMMERS_MAGIC_VALUES_INCLUDED
#define DIMMERS_MAGIC_VALUES_INCLUDED

float GetInteriorFactorFullLimitSqrt()
{
	return 0.99803728957; // sqrt( 254.0/255.0 );
}

float GetInteriorFactorFullHalfLimitInv()
{
	return 1.00196463654; // 1.0 / (254.5/255.0);
}

#endif

#ifdef ENABLE_DIMMERS_TILED_DEFERRED

float2 CalcDimmersFactorAndInteriorFactorTiledDeferred( float3 worldSpacePosition, int2 pixelCoord, float interiorVolumeFactor )
{
	float4 result = 0;

	const float fadeRefValue = CalcDissolvePattern( pixelCoord, 3 );
	const float3 viewSpacePosition = mul( worldToView, float4 ( worldSpacePosition, 1.0 ) ).xyz;

	for( uint tDimmerIdx = 0; tDimmerIdx < NumTileDimmersOpaque; ++tDimmerIdx )
	{
		DimmerParams dimmer = dimmers[ TileDimmerListOpaque[tDimmerIdx] ];
		
		const float3 off = mul( float4 ( viewSpacePosition, 1.0 ), dimmer.viewToLocal ).xyz;
		const bool rect_test = all( abs(off) < 1 );
		
		[branch]
		if ( rect_test )
		{
			const float fadeAlpha = (dimmer.fadeAlphaAndInsideMarkFactor >> 16) / (float)0xffff;
			const float insideMarkFactor = (dimmer.fadeAlphaAndInsideMarkFactor & 0xffff) / (float)0xffff;
			const float v_radial = saturate( 1 - length( off ) );
			const float v_radial_margin = saturate( v_radial * dimmer.marginFactor );
			
			float4 influence = 0;
			if ( -1 == dimmer.marginFactor )
			{
				influence.x = 0;
				influence.y = fadeAlpha >= (1 - fadeRefValue) ? 1 : 0;
				influence.z = 0;
				influence.w = 0;
			}
			else
			{
				const float limit = GetInteriorFactorFullLimitSqrt();
				influence.x = insideMarkFactor * (fadeAlpha >= fadeRefValue ? 1 : 0);
				influence.y = min( limit, v_radial * dimmer.outsideMarkFactor );
				influence.z = v_radial_margin * dimmer.insideAmbientLevel;
				influence.w = v_radial_margin * dimmer.outsideAmbientLevel;
			}

			result = max( result, influence );
		}
	}

	result.y *= result.y; //< better appearance according to lucek
	
	float insideFactor   = min( result.x > 0 ? 0 : 1, interiorVolumeFactor * interiorVolumeFactor );
	insideFactor = 1 - max( insideFactor, result.y );

	float insideAmbient  = min( result.x > 0 ? 1 - result.x : 1 - interiorParams.y, 1 - result.z );
	float outsideAmbient = 1 - result.w;

	return float2 ( lerp( outsideAmbient, insideAmbient, insideFactor ), 1 - insideFactor );
}

#endif

#ifdef ENABLE_DIMMERS_TRANSPARENCY

#ifdef __PSSL__
ByteBuffer TileDimmers : register(t21);
#else
ByteAddressBuffer TileDimmers : register(t21);
#endif

float2 CalcDimmersFactorAndInteriorFactorTransparency( float3 worldSpacePosition, int2 pixelCoord, bool allowSmoothBorder = true )
{
	const float interiorVolumeFactor = CalculateVolumeCutByPixelCoord( pixelCoord, worldSpacePosition, allowSmoothBorder );

	float4 result = 0;

	const float fadeRefValue = CalcDissolvePattern( pixelCoord, 3 );
	const uint2 tileIdx = pixelCoord.xy / ((uint)TILE_SIZE_INTERIOR_FULLRES.xx);
    const uint bufferIdx = ((tileIdx.y * (int)numTiles.z) + tileIdx.x) * MAX_DIMMERS_PER_TILE;

	const float3 viewSpacePosition = mul( worldToView, float4 ( worldSpacePosition, 1.0 ) ).xyz;

	for( uint tDimmerIdx = 0; tDimmerIdx < MAX_DIMMERS_PER_TILE; ++tDimmerIdx )
	{
		uint dimmerIndex = TileDimmers.Load((bufferIdx + tDimmerIdx)*4);		

		[branch]
		if ( dimmerIndex >= TILED_DEFERRED_DIMMERS_CAPACITY )
		{
			break;
		}

		DimmerParams dimmer = dimmers[ dimmerIndex ];
		
		const float3 off = mul( float4 ( viewSpacePosition, 1.0 ), dimmer.viewToLocal ).xyz;
		const bool rect_test = all( abs(off) < 1 );

		[branch]
		if ( rect_test )
		{
			const float fadeAlpha = (dimmer.fadeAlphaAndInsideMarkFactor >> 16) / (float)0xffff;
			const float insideMarkFactor = (dimmer.fadeAlphaAndInsideMarkFactor & 0xffff) / (float)0xffff;		
			const float v_radial = saturate( 1 - length( off ) );
			const float v_radial_margin = saturate( v_radial * dimmer.marginFactor );
			
			float4 influence = 0;
			if ( -1 == dimmer.marginFactor )
			{
				influence.x = 0;
				influence.y = fadeAlpha >= (1 - fadeRefValue) ? 1 : 0;
				influence.z = 0;
				influence.w = 0;
			}
			else
			{
				const float limit = GetInteriorFactorFullLimitSqrt();
				influence.x = insideMarkFactor * (fadeAlpha >= fadeRefValue ? 1 : 0);
				influence.y = min( limit, v_radial * dimmer.outsideMarkFactor );
				influence.z = v_radial_margin * dimmer.insideAmbientLevel;
				influence.w = v_radial_margin * dimmer.outsideAmbientLevel;
			}

			result = max( result, influence );
		}
	}

	result.y *= result.y; //< better appearance according to lucek

	float insideFactor   = min( result.x > 0 ? 0 : 1, interiorVolumeFactor * interiorVolumeFactor );
	insideFactor = 1 - max( insideFactor, result.y );

	float insideAmbient  = min( result.x > 0 ? 1 - result.x : 1 - interiorParams.y, 1 - result.z );
	float outsideAmbient = 1 - result.w;

	return saturate( float2 ( lerp( outsideAmbient, insideAmbient, insideFactor ), 1 - insideFactor ) );
}

float CalcDimmersFactorTransparency( float3 worldSpacePosition, int2 pixelCoord )
{
	return CalcDimmersFactorAndInteriorFactorTransparency( worldSpacePosition, pixelCoord ).x;
}

#endif

float GetProbesSSAOScaleByDimmerFactor( float dimmerFactor )						{ return dimmerFactor; }
