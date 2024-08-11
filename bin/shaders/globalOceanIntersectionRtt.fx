#include "globalOceanConstants.fx"

// fourier
Texture2D				t_furier : register (t5);
SamplerState			s_furier : register (s5);

//--------------------------------------------------------------------------------------
// Vertex shader
//--------------------------------------------------------------------------------------

struct VS_INPUT
{
	float4 pos			: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos		    : SYS_POSITION;
	float2 coord        : TEXCOORD0;
	float3 worldPos		: TEXCOORD1;
	float hmapHeight	: TEXCOORD2;
};


#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;

	o.pos   = i.pos;
	o.coord = (i.pos.xy + 1.0f)*0.5f;
	
	o.worldPos.xyz = VSC_CameraPosition.xyz	+ VSC_CameraVectorRight.xyz * i.pos.x * SimulationCamera.tanFov_x_ratio * SimulationCamera.nearPlane 
											+ VSC_CameraVectorUp.xyz * i.pos.y * SimulationCamera.tanFov * SimulationCamera.nearPlane
											+ VSC_CameraVectorForward.xyz * SimulationCamera.nearPlane;

	const int clipmapLevel = DetermineClipmapLevel( o.worldPos.xy );
	o.hmapHeight = GetTerrainHeight( o.worldPos.xy, clipmapLevel );
	
	return o;
}

#endif

//--------------------------------------------------------------------------------------
// Pixel Shader
//--------------------------------------------------------------------------------------


#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{	    
	float4 result = 0;
	float2 uv = i.worldPos.xy;

	//return float4( 0, 0,0,1);

	float intersectionColor = 0.0f;
		
	float3 globalPos = i.worldPos.xyz;
	
	float hmapHeight = i.hmapHeight;
	
	// lakes	
	float referenceWaterLevel = getWaterAreaOffset( (int)Lakes.numLakes, globalPos );
	float3 shoreProximity = getWaterShoreProximityDampValue(hmapHeight, referenceWaterLevel);


	float2 waves_UV_biggest = globalPos.xy / (GlobalWater.uvScale.x);	
	float2 waves_UV_medium = globalPos.xy / (GlobalWater.uvScale.y);
	float2 waves_UV_small = globalPos.xy / (GlobalWater.uvScale.z);

	float4 furier_large = SAMPLE_LEVEL( t_furier, s_furier, waves_UV_biggest, 0 );
	float4 furier_mediumN =  SAMPLE_LEVEL( t_furier, s_furier, waves_UV_medium, 0 );

	furier_large.z = F_LARGE_Nz;
	furier_large.xyz = normalize(furier_large.xyz);

	furier_mediumN.z = F_MEDIUM_Nz;
	furier_mediumN.xyz = normalize(furier_mediumN.xyz);

	// damp the largest waves near coastline and in caves		
	float3 h = GlobalWater.amplitudeScale.xyz * shoreProximity.xyz;

	float resultingH = h.x * furier_large.w + h.y * furier_mediumN.w;

	float caveDampValue = lerp(0.1f, 1.0f, getWaterBelowGroundDampValue( hmapHeight, referenceWaterLevel ));

	// choppy			
	float3 choppyFactor = caveDampValue*float3(GlobalWater.choppyFactors.x, GlobalWater.choppyFactors.y, 0.0f );	

	globalPos.z = caveDampValue * resultingH + referenceWaterLevel;		
	
	globalPos.xy -= shoreProximity.x*choppyFactor.x * furier_large.xy;
	globalPos.xy -= shoreProximity.y*choppyFactor.y * furier_mediumN.xy;

	const float epsOffset = 0.001f;

	float intersectionColor2 = 0.0f;

	if( i.worldPos.z < globalPos.z ) intersectionColor = 1.0f;
	else
		intersectionColor = 0.0f;

	if( i.worldPos.z < (globalPos.z+epsOffset) ) intersectionColor2 = 1.0f;
	else
		intersectionColor2 = 0.0f;	
		
	float smoothBlur =  1.0f - 2.0f*saturate( 1.2f*abs( i.worldPos.z - globalPos.z) );
	
	float smoothBlur2 =  1.0f - 2.0f*saturate( 1.2f*abs( i.worldPos.z - globalPos.z - epsOffset) );

	result.x = saturate( intersectionColor + pow( saturate( smoothBlur ), 8.0f ) );
	result.y = saturate(  pow( saturate( smoothBlur2 ), 12.0f ) );

	return result;
}

#endif
