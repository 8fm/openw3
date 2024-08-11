/// Common header for all dynamically compiled pixel shaders
/// It's esential to recompile all shaders after changing any of those lines

// common
#include "common.fx"
#include "globalConstantsPS.fx"
#include "include_sharedConsts.fx"
#include "include_globalFog.fx"
#include "include_utilities.fx"
#include "include_envProbe.fx"

#ifdef PIXELSHADER

// calculate subUV coordinates
float2 CalcSubUV( float2 uv, float frame, float width, float height )
{
   const float2 invSize = 1.0f / float2( width, height );
   float offsetU = floor( frame ) * invSize.x;
   float offsetV = floor( frame * invSize.x ) * invSize.y;
   return float2( offsetU, offsetV ) + uv * invSize;
}
	
float4 ImageSpaceSharpen( float4 colBase, float4 colMip, float sharpness, float gamma = 2.2 ) 
{ 
	float4 col0 = colBase;
	float4 col1 = colMip;
	
	col0 = pow( col0, gamma ); 
	col1 = pow( col1, gamma ); 
	
	float4 result = col0; 	
	float3 weights = float3 ( 0.3, 0.5, 0.2 ); 	
	float lum0 = dot(weights, col0.xyz);
	float lum1 = dot(weights, col1.xyz);
	result.xyz = col0.xyz * max( 0, lerp( lum0, lum1, -sharpness ) ) / max( 0.0001, lum0 ); 
	result.xyz = pow( result.xyz, 1.0 / gamma ); 

	return result; 
}

float2 EyeRaytrace( float3 worldPos, float3 N, float3 T, float3 B, float3 V, float refractionIndex, float refractionAmount, float eyeRadius, float eyePlaneDist, float irisSize, float leftEyeHorizDegAngle, float rightEyeHorizDegAngle, bool isRightEye )
{
	float3 eyeDirection;
	{
		float s, c;
		sincos( DEG2RAD(isRightEye ? rightEyeHorizDegAngle : leftEyeHorizDegAngle), s, c );
		eyeDirection = float3( s, c, 0 );

		float3x3 mat = isRightEye ? 
			float3x3( PSC_Custom_EyeOrientationRight_AxisX.xyz, PSC_Custom_EyeOrientationRight_AxisY.xyz, PSC_Custom_EyeOrientationRight_AxisZ.xyz ) :
			float3x3( PSC_Custom_EyeOrientationLeft_AxisX.xyz, PSC_Custom_EyeOrientationLeft_AxisY.xyz, PSC_Custom_EyeOrientationLeft_AxisZ.xyz );
		eyeDirection = mul( eyeDirection, mat );
	}

	T = normalize( T );
	B = normalize( B );
	N = normalize( N );
	V = normalize( V );

	float3 camDir = -V;

	float3 local_t = float3 ( 1, 0, 0 );
	float3 local_n = float3 ( 0, 1, 0 );
	float3 local_b = float3 ( 0, 0, 1 );

	local_n = eyeDirection;
	local_t = normalize( T - local_n * dot( T, local_n ) );
	local_b = cross( local_t, local_n );

	float3 eye_center = worldPos - N * eyeRadius;
	float3 pos_o = mul( float3x3( local_t, local_n, local_b ), (worldPos - eye_center).xyz );

	float2 coord = 0;
	float3 cam_dir_o = mul( float3x3( local_t, local_n, local_b ), camDir.xyz );
	float3 N_o = mul( float3x3( local_t, local_n, local_b ), N.xyz );
	float3 refr_dir	= refract(cam_dir_o, N_o, refractionIndex);

	float dist = max( 0, pos_o.y - eyePlaneDist );
	coord = pos_o.xz + refr_dir.xz * dist / max( 0.0001, abs(refr_dir.y) );

	coord  = lerp( pos_o.xz, coord, refractionAmount );
	coord *= irisSize;
	coord  = coord / (2 * eyeRadius) + 0.5;

	return coord;
}

#endif
	
	
