#if IS_CONSTANTBUFFER_PATCH
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common.fx"
#include "commonCS.fx"

Texture2D<float4>	TexInterpolator			: register(t0);
RW_BYTEBUFFER		BufferToPatch			: register(u0);

CS_CUSTOM_CONSTANT_BUFFER
	float4 patchParams;
	float4 value0;
	float4 value1;
END_CS_CUSTOM_CONSTANT_BUFFER

[NUMTHREADS(1, 1, 1)]
void cs_main()
{
	const float interpolator = saturate( TexInterpolator[int2(0,0)].x ) > 0.5 ? 0 : 1;
	const float4 value = lerp( value0, value1, interpolator );
	const uint byteOffset = (uint)patchParams.x;

	BufferToPatch.Store( byteOffset,   asuint(value.x) );
	BufferToPatch.Store( byteOffset+4, asuint(value.y) );
	BufferToPatch.Store( byteOffset+8, asuint(value.z) );
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#else
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "postfx_common.fx"
#include "include_constants.fx"

#define vCameraTestPosition		(PSC_Custom_0.xyz)

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
};

#ifdef VERTEXSHADER
VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;

	return o;
}
#endif

#ifdef PIXELSHADER


float CalcCameraInteriorFactor( float3 worldSpacePosition )
{
	const float interiorVolumeFactor = CalculateVolumeCutByPixelCoord( screenDimensions.xy / 2, worldSpacePosition, false );

	float4 result = 0;

	const float3 viewSpacePosition = mul( worldToView, float4 ( worldSpacePosition, 1.0 ) ).xyz;

	[loop]
	for( uint dimmerIndex = 0; dimmerIndex < dimmerNum; ++dimmerIndex )
	{
		DimmerParams dimmer = dimmers[ dimmerIndex ];
		
		const float3 off = mul( float4 ( viewSpacePosition, 1.0 ), dimmer.viewToLocal ).xyz;
		const bool rect_test = all( abs(off) < 1 );
		
		[branch]
		if ( rect_test )
		{
			//const float fadeAlpha = ((dimmer.fadeAlphaAndInsideMarkFactor) >> 16) / (float)0xffff;
			const float insideMarkFactor = ((dimmer.fadeAlphaAndInsideMarkFactor) & 0xffff) / (float)0xffff;
			const float v_radial = saturate( 1 - length( off ) );
			const float v_radial_margin = saturate( v_radial * dimmer.marginFactor );
			
			float4 influence = 0;
			if ( -1 == dimmer.marginFactor )
			{
				influence.x = 0;
				influence.y = 1;
				influence.z = 0;
				influence.w = 0;
			}
			else if ( 0 == dimmer.outsideMarkFactor )
			{
				const float limit = GetInteriorFactorFullLimitSqrt();
				influence.x = insideMarkFactor;
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

	return saturate( 1 - insideFactor );
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	return CalcCameraInteriorFactor( vCameraTestPosition );
}
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
