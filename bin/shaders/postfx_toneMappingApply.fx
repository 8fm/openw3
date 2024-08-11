#include "postfx_common.fx"

// Debug macro.
// Uncomment this to force tonemapping to be linear with given scale.
//#define FORCED_LINEAR_MULTIPLIER		1


Texture2D<float4> TextureColor       : register( t0 );

Texture2D TextureLumFinal	    	: register( t1 );

#define vSimpleTonemapping  			PSC_Custom_4
#define vNewToneMappingParamsScaling	PSC_Custom_6
#define vNewToneMappingParamsCurve0   	PSC_Custom_7
#define vNewToneMappingParamsCurve1   	PSC_Custom_8
#define vExperimentalParams				PSC_Custom_16

#if ENABLE_CURVE_BLENDING
#define vSimpleTonemappingB  			PSC_Custom_9
#define vNewToneMappingParamsScalingB	PSC_Custom_10
#define vNewToneMappingParamsCurve0B  	PSC_Custom_11
#define vNewToneMappingParamsCurve1B   	PSC_Custom_12
#define vBlendFactor					PSC_Custom_13
#define vExperimentalParamsB			PSC_Custom_17
#endif

#if SCURVE_DISPLAY
#define vScreenSizeInfo					PSC_Custom_15
#endif

#if FIXED_LUMINANCE
#define vFixedLuminance					PSC_Custom_18.x
#endif

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   	: SYS_POSITION;
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

/* original
float ShoulderStrength = 0.22;
float LinearStrength = 0.3;
float LinearAngle = 0.10;
float ToeStrength = 0.20;
float ToeNumerator = 0.01;
float ToeDenominator = 0.30;
*/   
#define DEFINE_APPLY_SCURVE( _T )													\
																					\
	_T ApplySCurve( _T x, float4 curveParams0, float4 curveParams1 )				\
	{																				\
		float ShoulderStrength = curveParams0.x;									\
		float LinearStrength = curveParams0.y;										\
		float LinearAngle = curveParams0.z;											\
		float ToeStrength = curveParams1.x;											\
		float ToeNumerator = curveParams1.y;										\
		float ToeDenominator = curveParams1.z;										\
																					\
		float A=ShoulderStrength;													\
		float B=LinearStrength;														\
		float C=LinearAngle;														\
		float D=ToeStrength;														\
		float E=ToeNumerator;														\
		float F=ToeDenominator;														\
																					\
		return max( 0, ((x*(A*x+C*B)+D*E)/(x*(A*x+B)+D*F)) - E/F );					\
	}

DEFINE_APPLY_SCURVE( float )
DEFINE_APPLY_SCURVE( float3 ) 

float4 ApplyDebugCurveVisualisation( float2 texcoord, float4 col, float4 curve0, float4 curve1, float4 scalingParams )
{
	float _scale = 4;

	float2 crd = _scale * float2 ( texcoord.x, texcoord.y ) - 0.1;
	float f = ApplySCurve( crd.x, curve0, curve1 ) / ApplySCurve( 1.0, curve0, curve1 );

	float y = crd.y;
	float thresh = _scale * 0.00125;
	if ( abs( f - y ) <= thresh )
		return 1;
	//if ( abs( crd.x - whitepoint ) <= thresh )
	//	return float4 ( 1,1,0,1 );	
	if ( abs( crd.x - 1.0 ) <= thresh || abs( crd.y - 1.0 ) <= thresh )
		return 1;
	if ( abs( crd.x - 0.0 ) <= 1 * thresh || abs( crd.y - 0.0 ) <= 1 * thresh )
		return 0;
	
	return col;
}

float3 PerformTonemapping( uint2 pixelCoord, float4 curveParams0, float4 curveParams1, float4 scalingParams, float2 luminanceClampMinMax, float keyLuminance, float4 experimentalParams )
{
	float3 result = 0;

#if defined(FORCED_LINEAR_MULTIPLIER)
	{
		result.xyz  = TextureColor[ pixelCoord ].xyz;
		result.xyz *= FORCED_LINEAR_MULTIPLIER;
	}
#else
	{
		if ( 1 || scalingParams.y <= 0 )		//< forcing new tonemapping. will be completely removed after some incubation time.
		{
			const float whitepoint = 11.2;
			
			const float clampShapeValue = experimentalParams.z;
			const float clampShapeRange = whitepoint * experimentalParams.x;
			float keyLumClamped = keyLuminance;
			keyLumClamped = min( max( keyLumClamped, luminanceClampMinMax.x ), luminanceClampMinMax.y );
			keyLumClamped = pow( max( keyLumClamped, 0.0001 ) / clampShapeRange, clampShapeValue ) * clampShapeRange;
			
			const float exposure = experimentalParams.x / keyLumClamped;
			const float postScale = experimentalParams.y;
			return postScale * ApplySCurve( exposure * TextureColor[pixelCoord].xyz, curveParams0, curveParams1 ) / ApplySCurve( whitepoint, curveParams0, curveParams1 );
		}
		else
		{
			const float postScale		= scalingParams.y;
			const float keyLumClamped	= min( max( max( keyLuminance, 0.0001 ), luminanceClampMinMax.x ), luminanceClampMinMax.y );
			const float whitepoint		= scalingParams.z * keyLumClamped;
			const float inputPoint		= 1.0;
			const float curveMul		= postScale / ApplySCurve( inputPoint, curveParams0, curveParams1 );
			const float sampleScale		= inputPoint / whitepoint;
	
			result.xyz  = ApplySCurve( sampleScale * TextureColor[ pixelCoord ].xyz, curveParams0, curveParams1 );
			result.xyz *= curveMul;
		}
	}
#endif

	return result;
}

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	
	const uint2 pixelCoord		= i.pos.xy;

#if FIXED_LUMINANCE
	const float keyLuminance	= vFixedLuminance;
#else
	const float keyLuminance	= TextureLumFinal[ uint2(0, 0) ].x;
#endif
	
	float4 result = float4 ( 0, 0, 0, 1 );

#if ENABLE_CURVE_BLENDING
	float3 resultA = PerformTonemapping( pixelCoord, vNewToneMappingParamsCurve0,  vNewToneMappingParamsCurve1,  vNewToneMappingParamsScaling,  vSimpleTonemapping.yz,  keyLuminance, vExperimentalParams );
	float3 resultB = PerformTonemapping( pixelCoord, vNewToneMappingParamsCurve0B, vNewToneMappingParamsCurve1B, vNewToneMappingParamsScalingB, vSimpleTonemappingB.yz, keyLuminance, vExperimentalParamsB );
	result.xyz = lerp( resultA, resultB, vBlendFactor.x );
#else
	result.xyz = PerformTonemapping( pixelCoord, vNewToneMappingParamsCurve0, vNewToneMappingParamsCurve1, vNewToneMappingParamsScaling, vSimpleTonemapping.yz, keyLuminance, vExperimentalParams );
#endif

#if SCURVE_DISPLAY
	const float2 vis_coord = float2( pixelCoord.x, vScreenSizeInfo.y - pixelCoord.y ) / vScreenSizeInfo.zw;
	#if ENABLE_CURVE_BLENDING
		result = ApplyDebugCurveVisualisation( vis_coord * float2 ( -ddy(vis_coord.y) / ddx(vis_coord.x), 1.0 ), result, vNewToneMappingParamsCurve0, vNewToneMappingParamsCurve1, vNewToneMappingParamsScaling );
		result = ApplyDebugCurveVisualisation( vis_coord * float2 ( -ddy(vis_coord.y) / ddx(vis_coord.x), 1.0 ), result, vNewToneMappingParamsCurve0B, vNewToneMappingParamsCurve1B, vNewToneMappingParamsScalingB );
	#else
		result = ApplyDebugCurveVisualisation( vis_coord * float2 ( -ddy(vis_coord.y) / ddx(vis_coord.x), 1.0 ), result, vNewToneMappingParamsCurve0, vNewToneMappingParamsCurve1, vNewToneMappingParamsScaling );
	#endif
#endif

	return result;
}
#endif
