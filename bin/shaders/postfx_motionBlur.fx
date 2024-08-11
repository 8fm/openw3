#include "postfx_common.fx"
#include "include_constants.fx"
#include "include_sharedConsts.fx"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Characters standout based on forward or radial distance. forward is ofcourse cheaper.
#define USE_CHARACTERS_FORWARD_DISTANCE		1

#define USE_CALC_SAMPLES_DISCARD			1

#define OFFSET_ENCODE_SCALE					5
#define OFFSET_ENCODE_BIAS					1.0

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef PIXELSHADER

#define			vMotionBlurParams		PSC_Custom_0
#define			vMotionBlurParams2		PSC_Custom_1
#define			vMotionBlurParams3		PSC_Custom_2
#define			mReprojectionMatrix		PSC_Custom_Matrix

float StrengthForDepthRevProjAware( float depth, float2 pixelCoord, bool isFullSize )
{
	float distance = DeprojectDepthRevProjAware( depth );
	return vMotionBlurParams.x * saturate( distance * vMotionBlurParams.z + vMotionBlurParams.w ) + vMotionBlurParams.y;
}

float MotionbBlurAmountByPixelDisplace( float pixelDisplace )
{
	return saturate( pixelDisplace * vMotionBlurParams3.x );
}

float CalcCharactersBlurScale( float dist )
{
	return vMotionBlurParams2.x * saturate( dist * vMotionBlurParams2.z + vMotionBlurParams2.w ) + vMotionBlurParams2.y;
}

float2 EncodeMotionVector( float2 motionVector )
{
  	motionVector = motionVector * OFFSET_ENCODE_SCALE + OFFSET_ENCODE_BIAS;
  	return clamp( motionVector, 0, 2 * OFFSET_ENCODE_BIAS );

// note! 
// there is some flickering present with this solution, so it should be solved before using it (probably near zero vectors)
//
//  float len = length( motionVector );
//  motionVector /= max( 0.001, len );
// 	return float2( 8 * (atan2( motionVector.y, motionVector.x ) * (1.0 / (2.f * PI)) + 0.5), len );
}

float2 DecodeMotionVector( float2 encodedMotionVector )
{
 	return (encodedMotionVector - OFFSET_ENCODE_BIAS) / OFFSET_ENCODE_SCALE;

// 	float2 dir;
// 	sincos( 0.125 * encodedMotionVector.x * (2.f * PI) - PI, dir.y, dir.x );
// 	return dir * encodedMotionVector.y;
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


#if IS_DOWNSAMPLE

Texture2D	t_TextureColor			: register( t0 );
Texture2D	t_TextureDepth			: register( t1 );
Texture2D<uint2> StencilTexture		: register( t2 );

SamplerState	s_Linear				: register( s0 );
SamplerState	s_Point					: register( s1 );

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif


#ifdef PIXELSHADER

struct PS_OUTPUT
{
	float4 color		: SYS_TARGET_OUTPUT0;
	float4 intensity	: SYS_TARGET_OUTPUT1;
};

PS_OUTPUT ps_main( VS_OUTPUT i )
{
	const int2 pixelCoord = i.pos.xy;

	float2 inputPos = i.pos.xy;
	float4 resultColor = 0;
	float4 resultIntensity = 0;

#if 1
	[unroll]
	for ( int i=0; i<2; ++i )
	[unroll]
	for ( int j=0; j<2; ++j )
	{
		int2 currPixelCoord = 2 * pixelCoord + int2( i, j );
		float depth = t_TextureDepth[ currPixelCoord ].x;

		float4 hpos0 = float4( ((currPixelCoord.xy + 0.5) * screenDimensions.zw * 2 - 1) * float2( 1, -1 ), TransformDepthRevProjAware( depth ), 1 );
		float4 hpos1 = mul( hpos0, mReprojectionMatrix );
		hpos1.xy /= max( hpos1.w, 0.25 );
		
	#if USE_CHARACTERS_FORWARD_DISTANCE
		const float distance	= DeprojectDepthRevProjAware( depth );
	#else
		const float distance	= length( PositionFromDepthRevProjAware( depth, currPixelCoord ).xyz - cameraPosition.xyz );
	#endif

		const uint  stencil		= GetStencilValue( StencilTexture[ currPixelCoord ] );	
		const float maskValue	= (stencil & LC_Characters) ? CalcCharactersBlurScale( distance ) : 1;		

		const float2 crdDiff = (hpos0.xy - hpos1.xy) * float2( 0.5, -0.5 );

		const float2 motionVectorUnmasked	= crdDiff * StrengthForDepthRevProjAware( depth, currPixelCoord, true );
		const float2 motionVectorMasked		= motionVectorUnmasked * maskValue;
		const float motionPixelDisplace		= length( motionVectorMasked * halfScreenDimensions.xy );

		float blendWeight = max( 0.01, motionPixelDisplace );

		resultColor	+= float4 ( t_TextureColor[ currPixelCoord ].xyz * blendWeight, 1 );
		resultIntensity += float4 ( motionVectorUnmasked, blendWeight, 1 ); // Unmasked goes to final displace in order to cover blind spots that are inside nearby characters silhouettes
	}	
#endif

#if 0
	//NOTE THAT THIS IS NOT EXACTLY THE SAME.
	//SIMPLIFIED MOTION VECTOR IS OK, BUT IT DOESN"T CONTRIBUTE TO THE BLEND WEIGHT, WHICH IS NOT COOL...

	float avgDepth = 0;
	float avgMask  = 0;

	[unroll]
	for ( int i=0; i<2; ++i )
	[unroll]
	for ( int j=0; j<2; ++j )
	{
		int2 currPixelCoord = 2 * pixelCoord + int2( i, j );
		float depth = t_TextureDepth[ currPixelCoord ].x;
		float deprojDepth = DeprojectDepthRevProjAware( depth );

	#if USE_CHARACTERS_FORWARD_DISTANCE
		const float distance	= deprojDepth;
	#else
		const float distance	= length( PositionFromDepthRevProjAware( depth, currPixelCoord ).xyz - cameraPosition.xyz );
	#endif

		const uint  stencil		= GetStencilValue( StencilTexture[ currPixelCoord ] );	
		const float maskValue	= (stencil & LC_Characters) ? CalcCharactersBlurScale( distance ) : 1;		

		avgDepth	+= deprojDepth;
		avgMask		+= maskValue;

		resultColor += float4 ( t_TextureColor[ currPixelCoord ].xyz * maskValue, 1 );
	}	

	{
		avgDepth *= 0.25;
		avgMask  *= 0.25;

		float projDepth = ProjectDepth( avgDepth );

		float4 hpos0 = float4( (inputPos * halfScreenDimensions.zw * 2 - 1) * float2( 1, -1 ), projDepth, 1 );
		float4 hpos1 = mul( hpos0, mReprojectionMatrix );
		hpos1.xy /= max( hpos1.w, 0.25 );

		//const float2 crd0 = (hpos0.xy * float2( 0.5, -0.5 ) + 0.5);
 		//const float2 crd1 = (hpos1.xy * float2( 0.5, -0.5 ) + 0.5);
		const float2 crdDiff = (hpos0.xy - hpos1.xy) * float2( 0.5, -0.5 );

		const float2 motionVector2 = crdDiff * StrengthForDepthRevProjAware( TransformDepthRevProjAware( projDepth ), pixelCoord, false );
 		const float2 motionVector = motionVector2 * avgMask;
 		const float motionPixelDisplace = length( motionVector * halfScreenDimensions.xy );

		resultIntensity.xy = 4 * motionVector2;
		resultIntensity.z = 4 * avgMask;
	}
#endif
	
	resultIntensity.xy = EncodeMotionVector( 0.25 * resultIntensity.xy );
	resultColor.xyz *= 0.25;
	resultIntensity.z *= 0.25;
	
	PS_OUTPUT output;	
	output.color		= resultColor;
	output.intensity	= resultIntensity;

	return output;
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_CALC

Texture2D		t_TextureColor			: register( t0 );
Texture2D		t_TextureIntensities	: register( t1 );
Texture2D		t_TextureDepthFullRes	: register( t2 );
SamplerState	s_Linear				: register( s0 );
SamplerState	s_Point					: register( s1 );
#define			iPassIndex				PSC_Custom_4.x

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float2 ClampCoord( float2 crd0, float2 crd1, float2 clampMax )
{
	float t = 1;
 	if ( crd1.x > clampMax.x )	t = (clampMax.x - crd0.x) / (crd1.x - crd0.x);
 	if ( crd1.x < 0 )			t = crd0.x / (crd0.x - crd1.x);
 	if ( crd1.y > clampMax.y )	t = min( t, (clampMax.y - crd0.y) / (crd1.y - crd0.y) );
 	if ( crd1.y < 0 )			t = min( t, crd0.y / (crd0.y - crd1.y) );
	crd1 = lerp( crd0, crd1, t );
	return crd1;
}

float4 SampleColor( float2 clampMax, float2 currCrd )
{
#if !USE_CALC_SAMPLES_DISCARD
	currCrd = min( currCrd, clampMax );
#endif

	float3 currCol = SAMPLE_LEVEL( t_TextureColor, s_Linear, currCrd, 0 ).xyz;
	float3 currIntensities = SAMPLE_LEVEL( t_TextureIntensities, s_Linear, currCrd, 0 ).xyz;

	float weight = 1;

#if USE_CALC_SAMPLES_DISCARD
	weight = all( currCrd <= clampMax ) && all( currCrd >= 0 );
#endif
	/*
	{
		float centerRange = (num_samples - 1) / 2.0;
		float extentRange = 1.0 / (1.2 * centerRange);
		weight = saturate( 1 - abs(i - centerRange) * extentRange );
	}
	//*/

	return float4( currCol, currIntensities.z ) * weight;
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const int2 pixelCoord = i.pos.xy;	
	const float4 refIntensities = t_TextureIntensities[pixelCoord];
	
	float2 crdCenter = i.pos.xy * halfSurfaceDimensions.zw;
	float2 crdDelta = DecodeMotionVector( refIntensities.xy ) * halfScreenDimensions.xy * halfSurfaceDimensions.zw;

	const float num_samples = 7;

	if ( 1 == iPassIndex )
	{
		crdDelta *= 1.0 / num_samples;
	}	

	const float2 clampMax = (halfScreenDimensions.xy - 1.0) * halfSurfaceDimensions.zw;
	
	const float2 crdStepDelta = crdDelta * 0.3333;

	float4 result = 0;
	result += SampleColor( clampMax, crdCenter - crdDelta );
	result += SampleColor( clampMax, crdCenter - 2 * crdStepDelta );
	result += SampleColor( clampMax, crdCenter - 1 * crdStepDelta );
	result += SampleColor( clampMax, crdCenter + 1 * crdStepDelta );
	result += SampleColor( clampMax, crdCenter + 2 * crdStepDelta );
	result += SampleColor( clampMax, crdCenter + crdDelta );

	result += float4( SAMPLE_LEVEL( t_TextureColor, s_Point, crdCenter, 0 ).xyz, refIntensities.z );
	
	result.xyz /= max( 0.001, result.w );
	
	if ( 0 == iPassIndex )
	{
		result.xyz *= refIntensities.z;
	}
	
	return float4 ( result.xyz, 1 );
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_SHARPEN

Texture2D		t_TextureColor			: register( t0 );
Texture2D		t_TextureIntensities	: register( t1 );
SamplerState	s_Linear				: register( s0 );

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	const float2 clampedPosXY = min( i.pos.xy, halfScreenDimensions.xy - 1 );

	const int2 pixelCoord = clampedPosXY;
	const float2 crd = clampedPosXY * halfSurfaceDimensions.zw;
	const float2 crdHalfPixel = 0.5 * halfSurfaceDimensions.zw;

	float3 centerColor = t_TextureColor[pixelCoord].xyz;
	float centerWeight = t_TextureIntensities[pixelCoord].z;
	if ( centerWeight <= 0.0 )
	{
		return float4 ( centerColor, 1 );
	}

	centerColor /= centerWeight;

	float weightsSum = 0;
	float3 blurredColor = 0;
	{
		float weight;

		weight = max( 0.00, SAMPLE_LEVEL( t_TextureIntensities, s_Linear, crd + float2(  1,  1 ) * crdHalfPixel, 0 ).z );	
		if ( weight > 0.001 )
		{
			blurredColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, crd + float2(  1,  1 ) * crdHalfPixel, 0 ).xyz / weight;	
			weightsSum += 1;
		}
		
		/*
		weight = max( 0.00, SAMPLE_LEVEL( t_TextureIntensities, s_Linear, crd + float2( -1,  1 ) * crdHalfPixel, 0 ).z );	
		if ( weight > 0.001 )
		{
			blurredColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, crd + float2( -1,  1 ) * crdHalfPixel, 0 ).xyz / weight;	
			weightsSum += 1;
		}
		
		weight = max( 0.00, SAMPLE_LEVEL( t_TextureIntensities, s_Linear, crd + float2(  1, -1 ) * crdHalfPixel, 0 ).z );	
		if ( weight > 0.001 )
		{
			blurredColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, crd + float2(  1, -1 ) * crdHalfPixel, 0 ).xyz / weight;	
			weightsSum += 1;
		}
		*/
		
		weight = max( 0.00, SAMPLE_LEVEL( t_TextureIntensities, s_Linear, crd + float2( -1, -1 ) * crdHalfPixel, 0 ).z );	
		if ( weight > 0.001 )
		{
			blurredColor += SAMPLE_LEVEL( t_TextureColor, s_Linear, crd + float2( -1, -1 ) * crdHalfPixel, 0 ).xyz / weight;	
			weightsSum += 1;
		}

		if ( weightsSum <= 0 )
		{
			return float4( centerColor, 1 );
		}
		
		blurredColor /= max( 0.001, weightsSum );
	}	

	const float3 lumWeights = float3( 0.3, 0.5, 0.2 );
	const float  lumCenter = dot( lumWeights, centerColor );
	const float  lumAround = dot( lumWeights, blurredColor );
	const float  lumTarget = max( 0, lerp( lumAround, lumCenter, vMotionBlurParams3.y ) );

	float3 resultColor = centerWeight * centerColor * (lumTarget / max( 0.001, lumCenter ));
	
	return float4 ( resultColor, 1 );
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#if IS_APPLY

Texture2D		t_TextureColorFullRes	: register( t0 );
Texture2D		t_TextureColorBlurred	: register( t1 );
Texture2D		t_TextureDepth			: register( t2 );
Texture2D		t_TextureIntensities	: register( t3 );
Texture2D<uint2> StencilTexture			: register( t4 );
SamplerState	s_Linear				: register( s0 );

struct VS_INPUT
{
	float4 pos		   : POSITION0;
};

struct VS_OUTPUT
{
	float4 pos	   		: SYS_POSITION;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   		= i.pos;
	
	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{ 
	const int2 pixelCoord = i.pos.xy;
	const float2 blurredUV = min( i.pos.xy, screenDimensions.xy - 2 ) * surfaceDimensions.zw;

	float depth = t_TextureDepth[pixelCoord].x;
	float blurredAmount = 0;
	{
		float4 hpos0 = float4( (i.pos.xy * screenDimensions.zw * 2 - 1) * float2( 1, -1 ), TransformDepthRevProjAware( depth ), 1 );
		float4 hpos1 = mul( hpos0, mReprojectionMatrix );
		hpos1.xy /= max( hpos1.w , 0.25 );

		const float2 crd0 = (hpos0.xy * float2( 0.5, -0.5 ) + 0.5);
		const float2 crd1 = (hpos1.xy * float2( 0.5, -0.5 ) + 0.5);
		float pixelDisplace = length( (crd0 - crd1) * screenDimensions.xy ) * StrengthForDepthRevProjAware( depth, pixelCoord, true );	
		blurredAmount = MotionbBlurAmountByPixelDisplace( pixelDisplace );

	#if USE_CHARACTERS_FORWARD_DISTANCE
		const float distance = DeprojectDepthRevProjAware( depth );
	#else
		const float distance = length( PositionFromDepthRevProjAware( depth, pixelCoord ).xyz - cameraPosition.xyz );
	#endif

		blurredAmount *= ( GetStencilValue( StencilTexture[ pixelCoord ] ) & LC_Characters) ? CalcCharactersBlurScale( distance ) : 1;
	}
	
	float3 blurredColor	= SAMPLE_LEVEL( t_TextureColorBlurred, s_Linear, blurredUV, 1 ).xyz;
	float3 sharpColor	= t_TextureColorFullRes[ pixelCoord ].xyz;

	return float4 ( lerp( sharpColor, blurredColor, blurredAmount ), 1 );
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
