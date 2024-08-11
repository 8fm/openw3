
#define GLOBAL_OCEAN_CONSTANTS_EXCLUDE_SHAPES

#include "globalOceanConstants.fx"

SamplerState	s_TextureColor				: register( s0 );
Texture2D		t_TextureColor				: register( t0 );

SamplerState	s_TextureIntersection		: register( s2 );
Texture2D		t_TextureIntersection		: register( t2 );

SamplerState	s_depth						: register( s3 );
Texture2D		t_depth						: register( t3 );

Texture2D				t_furier : register (t5);
SamplerState			s_furier : register (s5);

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		   : SYS_POSITION;
	float2 coord       : TEXCOORD0;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coord = i.pos.xy * VSC_Custom_0.xy + VSC_Custom_0.zw;
	return o;
}

#endif

#ifdef PIXELSHADER
float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	
	float4 result = 0;
	float2 uv = i.coord.xy;    

	float2 intersectionVal = t_TextureIntersection.Sample( s_TextureIntersection, uv.xy / PSC_ViewportSubSize.zw ).xy;		
		
	// early exit
	if( intersectionVal.y + intersectionVal.x < 0.001f ) return t_TextureColor.Sample( s_TextureColor, i.coord ).xyzw;
		
	float depthNoRef = DeprojectDepthRevProjAware( SAMPLE_LEVEL( t_depth, s_depth, uv.xy, 0 ).x );

	float edgeRefFactor = 0.5*pow(saturate(intersectionVal.y), 8.0f);

	float2 refCoord = 0.002f*cameraPosition.xy+ + 0.15f*i.coord;
	refCoord.xy += 0.2f*edgeRefFactor;
	
	float2 ref = 30.0f * t_furier.Sample( s_furier, refCoord ).xy;		
	ref.xy *= intersectionVal.x;	
	
	float depthWWater = DeprojectDepthRevProjAware( SYS_SAMPLE( PSSMP_SceneDepth, uv.xy - ref ).x );
	float depthNoWater = DeprojectDepthRevProjAware( SAMPLE_LEVEL( t_depth, s_depth, uv.xy - ref, 0 ).x );	
	
	float depth = lerp( depthNoWater, depthWWater, intersectionVal.x );
	
	ref.xy *= max( 0.2f, saturate( depth / 20.0f ) );
	ref.xy -= 0.05f*edgeRefFactor;

	depthWWater = DeprojectDepthRevProjAware( SYS_SAMPLE( PSSMP_SceneDepth, uv.xy - ref ).x );
	depthNoWater = DeprojectDepthRevProjAware( SAMPLE_LEVEL( t_depth, s_depth, uv.xy - ref, 0 ).x );	

	depth = lerp( depthNoWater, depthWWater, intersectionVal.x );

	float sceneDepth = saturate( 0.2f * pow( depth, 0.8f ) );
	float fogDepth = saturate( depth/50.0f );

	float3 sceneColor = t_TextureColor.Sample( s_TextureColor, uv.xy - ref ).xyz;
	
		//return float4( sceneDepth, sceneDepth, sceneDepth,1 );		
	
	const uint2 pixelCoord = (uint2)(i.pos.xy);	
	float3 wPos = PositionFromDepthRevProjAware( SYS_SAMPLE( PSSMP_SceneDepth, uv.xy - ref.xy ).x, pixelCoord );
	
	float depthBasedLightBump = clamp( saturate( pow( 0.1f*abs(wPos.z), 1.6f ) ), 0.1f, 1.0f);			
	float mixedColorDepthFactor = lerp( 0.0, saturate( globalUnderwaterFogIntensity ), sceneDepth );

		//return float4(mixedColorDepthFactor, mixedColorDepthFactor, mixedColorDepthFactor, 1);

	mixedColorDepthFactor += 0.15f*intersectionVal.y;
	mixedColorDepthFactor = clamp( mixedColorDepthFactor, 0.5f, 0.9f );
	mixedColorDepthFactor *= saturate(globalUnderwaterFogIntensity);

	float2 blurOffsets[8];
		blurOffsets[0] = float2(0.0f,1.0f);
		blurOffsets[1] = float2(1.0f,1.0f);
		blurOffsets[2] = float2(1.0f,-1.0f);
		blurOffsets[3] = float2(0.0f,-1.0f);
		blurOffsets[4] = float2(-1.0f,-1.0f);
		blurOffsets[5] = float2(-1.0f,1.0f);
		blurOffsets[6] = float2(-1.0f,0.0f);
		blurOffsets[7] = float2(-1.0f,1.0f);

	float2 ssOffset = float2(0.0015f/PSC_ViewportSubSize.zw);

    float samplesNum = 0;

	[unroll]
    for ( int tap_i=0; tap_i<8; tap_i+=1 )
    {        
	    float2 coord	= (i.coord - ref) + ssOffset.xy * blurOffsets[tap_i].xy;

		float depthBlurred = DeprojectDepthRevProjAware( SYS_SAMPLE( PSSMP_SceneDepth, coord.xy ).x );
		result.xyz		+= lerp( t_TextureColor.Sample( s_TextureColor, coord ).xyz, globalUnderwaterFogColor.xyz, saturate( depthBlurred/100.0f ) );
		
		samplesNum		+= 1.0f;		
	}
	
	result.xyz /= samplesNum;	

	float3 fullBlur = result.xyz;

		//return float4( fullBlur.xyz,1 );

	float3 noBlurColored =  lerp( sceneColor.xyz, globalUnderwaterFogColor.xyz, mixedColorDepthFactor );	
	float3 fullBlurColored =  lerp( fullBlur.xyz, globalUnderwaterFogColor.xyz, mixedColorDepthFactor );
	
		//return float4( sceneColor.xyz, 1);
		//return float4( fullBlurColored.xyz, 1);		

	result.xyz = lerp( noBlurColored.xyz, fullBlurColored.xyz, max( 0.5f ,fogDepth) );	

	// depth based color correction
	float3 depthColorCorr = (0.3f*result.xyz + 0.7f*normalize(globalUnderwaterFogColor.xyz)*dot(result.xyz, 0.577f));
	
	result.xyz = lerp( depthColorCorr.xyz, result.xyz, saturate(4.0*(fogDepth-0.01f)) );	

	result.xyz = lerp( 0.1*( 1.0-fogDepth )*(globalUnderwaterFogColor.xyz) + 1.5*result.xyz, result.xyz, ( 1.0f - saturate( globalUnderwaterFogIntensity ) )*depthBasedLightBump );

	result.xyz *= globalUnderwaterBrightness;

	result.xyz = lerp( sceneColor.xyz, result.xyz, pow( saturate( intersectionVal.x ), 1.0f) );			

	/* DISABLED TEMPORARY
	if( globalWaterCaustiscGain > 0.0f )
	{
		float3 wPos = PositionFromDepth( depth, i.pos.xy - ref ).xyz;		
			
		float3 cameraRight = worldToView[0].xyz;
		float3 cameraUp = worldToView[1].xyz;
		float3 cameraForward = worldToView[2].xyz;

		// calc flat normals
		float pixelOffset = 1.0f;	
		const uint2 pixelCoord = (uint2)(i.pos.xy - ref);
	
		const float2 pixelCoord2 = pixelCoord + float2(pixelOffset, 0.0f);
		float3 a = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord2 ).x, pixelCoord2 );
			
		const float2 pixelCoord3 = pixelCoord + float2(-pixelOffset, 0.0f);
		float3 b = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord3 ).x, pixelCoord3 );
			
		const float2 pixelCoord4 = pixelCoord + float2(0.0f, pixelOffset );
		float3 c = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord4 ).x, pixelCoord4 );
			
		const float2 pixelCoord5 = pixelCoord + float2(0.0f, -1.0f*pixelOffset );
		float3 d = PositionFromDepthRevProjAware( SYS_SAMPLE_TEXEL( PSSMP_SceneDepth, pixelCoord5 ).x, pixelCoord5 );

		float3 N_flat = ( cross( a - b, d - c ) );
		float3 normal = normalize(N_flat);

		float3 dir = normalize( wPos.xyz - cameraPosition.xyz );
		float  dd = dot(cameraForward.xyz, dir);
		dir*=(1.0f/dd);
    
		float3 hitPos = cameraPosition.xyz + (dir)*depth;
		float2 cs_uv = ( hitPos.xy*0.18f ) - 0.015f*normal.xy;
		
		float2 waves_UV_medium = 0.4f*(hitPos.xy) / ( 13.0f );
		float3 furier_mediumN =  SAMPLE( t_furier, s_furier, waves_UV_medium ).xyz;
	
		float delta = SAMPLE( t_furier, s_furier, waves_UV_medium + 50.0f * furier_mediumN.xy ).z;
				
		float f_cs = 5.0f*pow( saturate(0.5f*delta), 3.0f );

		float vSS = dot( lightDir.xyz, cameraRight.xyz );
		float vSS2 = dot( lightDir.xyz, cameraForward.xyz );

		float2 waves_UV_mediumSS = 0.4f*( float2( i.coord.x + (i.coord.y-0.5f)*vSS, 0.0f ) );
		
		waves_UV_mediumSS.x = i.coord.x + lightDir.x;

		float deltaSS = SAMPLE( t_furier, s_furier, waves_UV_mediumSS ).z;

		float depthCut = pow( saturate(1.0f - depth*0.03f), 2.0f );

		deltaSS = pow( saturate(0.4f*deltaSS), 1.4f );
		deltaSS *= pow( saturate(intersectionVal.x), 4.0f );
		deltaSS *= saturate(1.0f-1.3f*i.coord.y);
		deltaSS *= saturate(1.3f*i.coord.y);
		deltaSS *= 1.0f-depthCut;

		// cut by depth
		f_cs *= depthCut;
		f_cs *= saturate( pow( 0.5f*depth, 3.0f ) );		

		// cut by normal
		float normal_factor = pow( saturate( normal.z ), 0.8f );
		f_cs *= normal_factor;
	
		f_cs *= lerp(0.1f, 1.0f, saturate(2.0f*lightDir.z) );		
		f_cs *= saturate( pow(sceneDepth, 0.5f) );
		f_cs += 100.0f* deltaSS;

		result.xyz *= 1.0f + f_cs * globalWaterCaustiscGain;// * globalUnderwaterBrightness;
	}
	*/

	return result;
}
#endif 
