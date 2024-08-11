
#include "commonPS.fx"
#include "globalConstantsVS.fx"

Texture2D<float4>	tDecalDiff		: register( t0 );
SamplerState		sDecalDiff		: register( s0 );

#define SceneDepth t_PSSMP_SceneDepth

#ifdef USE_NORMALS

	Texture2D<float4>    tNormal    : register( t4 );
	SamplerState        sNormal    : register( s4 );

#else

	Texture2D<float4>	SceneNormals	: register( t2 );

#endif

#ifdef PIXELSHADER
#if IS_FOCUS_MODE 		

#define FocusModeColor		PSC_Custom_0.xyz
#define FocusModeParams		PSC_Custom_1

// Fade out by distance
float DistanceFade( float3 wPos )
{	
	const float3 PlayerPosition = FocusModeParams.xyz;
	const float EffectDistanceShift = FocusModeParams.w;

	float distance = length( wPos - PlayerPosition );
	// extending view the distance over the time	
	
	float distanceMax = 0.0f; 
	
	if( EffectDistanceShift > 0.03f )
	{
		distance *= 0.12f;
		distanceMax = 1.0f + pow(abs(EffectDistanceShift), 2.8f) * 120.0f;
	}
	else
	{
		distanceMax = 1.0f + pow(abs(EffectDistanceShift), 0.8f) * 120.0f;		
	}	

	distance = 1.0f - pow( distance/distanceMax, 1.6f );
	
	return saturate( distance );
}
#endif
#endif

START_CB( SSDecalConstants, 6 )
	struct SDecalConstants
	{
		float4x4	ProjectionMatrix;
		float4		SubUVClip;
		float4		SpecularColor;
		float		NormalThreshold;
		float		DecalFade;
		float		Specularity;
		float		Scale;
		float4		DiffuseColor;
		float4		Tangent;
		float4		Up;
		float		Zdepthfade;
	} DecalConstants;
END_CB

struct VS_INPUT
{
	float4 pos 		: POSITION0;
};

struct VS_OUTPUT
{
	float4 pos 			: SYS_POSITION;
	float3 orientation	: TEXCOORD1;
};

struct PS_INPUT
{
	float4 pos 			: SYS_POSITION;
	float3 orientation	: TEXCOORD1;
};

struct PS_OUTPUT
{
	float4 color	   : SYS_TARGET_OUTPUT0;
#ifdef USE_SPECULAR
	float4 specularity : SYS_TARGET_OUTPUT1;
#endif

#ifdef USE_NORMALS

	#ifdef USE_SPECULAR
		float4 normal : SYS_TARGET_OUTPUT2;
	#else
		float4 normal : SYS_TARGET_OUTPUT1;
	#endif

#endif

};

#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT i )
{
	VS_OUTPUT o;
	
	float4 worldPos = mul( i.pos, VSC_LocalToWorld );
	
	o.pos = mul( worldPos, VSC_WorldToScreen );

	// local Z axis
	o.orientation = float3( VSC_LocalToWorld[2][0], VSC_LocalToWorld[2][1], VSC_LocalToWorld[2][2] );

	return o;
}
#endif

#ifdef PIXELSHADER

#ifndef USE_NORMALS

	float3 GetWorldNormal( in uint2 pixelCoord )
	{
		float3 n = SceneNormals[pixelCoord].xyz;
		return normalize( n - 0.5f );
	}

#endif

void ps_main( PS_INPUT i, out PS_OUTPUT o )
{
	const uint2 pixelCoord = i.pos.xy;
	// sample hardware depth buffer
	float depthReversedProj = SceneDepth[ pixelCoord ].x;
	
	// calculate world position
	float2 screenSpaceUV = i.pos.xy / PSC_ViewportSubSize.xy;
	float3 wPos = PositionFromDepthRevProjAware( depthReversedProj, pixelCoord );
	
	// transfer world position to decal space and scale inside the UV
	float4 decalUV = mul( float4( wPos, 1.0f), DecalConstants.ProjectionMatrix ).xyzw * DecalConstants.Scale;
	decalUV -= DecalConstants.Scale*0.5f- 0.5f;
	// clip pixels outside the box
	clip( decalUV.xyz - float3( DecalConstants.SubUVClip.xy , 0.0f ) );
	clip( float3( DecalConstants.SubUVClip.zw , 1.0f ) - decalUV.xyz );

#ifndef USE_NORMALS
	// clip sharp edges
	float normalAngle = dot( GetWorldNormal( pixelCoord ), normalize( i.orientation ) );
	clip( normalAngle - DecalConstants.NormalThreshold );
#endif
	
	float4 tex_color = tDecalDiff.Sample( sDecalDiff, decalUV.xy ) * DecalConstants.DiffuseColor;
	tex_color.a *= DecalConstants.DecalFade;

#if IS_FOCUS_MODE	
	o.color = DistanceFade( wPos ) * float4 ( FocusModeColor.rgb, tex_color.a );
	// Decals in foc us mode desn't have its specularity
#else
	
	// fade decal based on distance to near and far planes
	tex_color.w *= saturate( (1.0f - abs((decalUV.z - 0.5f)*2.0f)) * abs(DecalConstants.Zdepthfade));
	o.color = tex_color;

		#ifdef USE_NORMALS
		float4 nrmS = tNormal.Sample( sNormal, decalUV.xy );
		float3 nrm = normalize( nrmS-0.5 );
		
		const float3 t = DecalConstants.Tangent.xyz;
		const float3 n = DecalConstants.Up.xyz;
		const float3 b = cross( n, t );

		// Reconstruct world-space normal orientation
		float3 right = t * nrm.x;
		float3 forw = b * nrm.y;
		float3 up = n * nrm.z;
		
		o.normal = float4( ( right + forw + up ) * 0.5f + 0.5f , saturate( tex_color.a * 4.0 ) );
	#endif

	#ifdef USE_SPECULAR
		o.specularity = float4( lerp( tex_color.rgb , float3( 1,1,1 ) , DecalConstants.SpecularColor.a ) * DecalConstants.SpecularColor.rgb * DecalConstants.Specularity , tex_color.a );
	#endif

#endif
}

#endif
