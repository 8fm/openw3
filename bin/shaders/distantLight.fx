#include "common.fx"
#include "globalConstantsVS.fx"
#include "globalConstantsPS.fx"
#include "include_constants.fx"

Texture2D		tTextureDepth		: register( t0 );

Texture2D		tTextureAlbedo		: register( t1 );
Texture2D		tTextureNormals		: register( t2 );
	
#define fIntensityScale VSC_Custom_0.x

struct VS_INPUT
{
	float4 pos 	: POSITION0;
	float4 color: COLOR0;
	float4 uv  	: TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos   : SYS_POSITION;
	float4 light : TEXCOORD0;
	float4 color : TEXCOORD1;
};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	
	float4 finalPos = i.pos;
	float radius = abs( i.uv.x );
	
	finalPos.xyz = VSC_CameraVectorRight 	* i.uv.x 	+ finalPos.xyz;
	finalPos.xyz = VSC_CameraVectorUp 		* i.uv.y 	+ finalPos.xyz;
	// finalPos.xyz = VSC_CameraVectorForward * -abs( i.uv.x ) + finalPos.xyz;
	
	o.pos   = mul( finalPos, VSC_WorldToScreen );
	
	o.color.rgb = fIntensityScale * i.color.rgb;
	o.color.a = i.color.a;
	
	o.light.xyz = i.pos.xyz;
	o.light.w = 1.0 / radius;

	return o;
}

#endif

#ifdef PIXELSHADER

float4 ps_main( VS_OUTPUT i ) : SYS_TARGET_OUTPUT0
{
	// Depth testing and smoothing
	float depth = tTextureDepth[ (int2)i.pos.xy ].x;
	
	float3 worldPos = PositionFromDepthRevProjAware( depth, i.pos.xy );
	
	const float3 lightDir = i.light.xyz - worldPos;
	const float dist2 = dot( lightDir , lightDir );
	const float dist = sqrt( dist2 );

	const float normDist = i.light.w * dist;
	
	if( normDist > 0.99f ) discard;

	const float att_num = 1.0f - (normDist*normDist)*(normDist*normDist);

	const float att = ( att_num * att_num ) / ( dist2 * i.color.a + 1.0f );
	
	const float3 albedo = tTextureAlbedo[(uint2)i.pos.xy].xyz;
	const float3 normal = normalize( tTextureNormals[ (int2)i.pos.xy ].xyz - 0.5f );

	const float nDotL = max( dot( lightDir/dist , normal ) , 0.0f );
	
	float3 finalColor = ( i.color.rgb * pow(albedo,2.2) ) * ( nDotL * att );
	
	//----------DEBUG
	//finalColor = i.color.rgb;
#if IS_DEBUG_DISTANT_LIGHT
	finalColor += float3(20,0,0) * saturate( 1.1f - att * 1000.0f );
#endif
	//----------DEBUG
	
	return float4( finalColor , 1.0f );
}

#endif
