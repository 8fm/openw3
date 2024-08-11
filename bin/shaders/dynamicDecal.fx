#define COMPILE_IN_DYNAMIC_DECALS
#define CALC_LIGHTING_PBR_PIPELINE_EXPLICIT_ENVPROBE_LOD

#define USE_VERTEX_FOG
//#define USE_DISTANCE_FADE


#include "commonPS.fx"
#ifdef VERTEXSHADER
#include "commonVS.fx"
#endif
#include "include_constants.fx"


// Default roughness, if no normal map provided.
#define MATERIAL_DEFAULT_ROUGHNESS		0.5
// Threshold when fading the decal based on distance from the scene. Larger values can cause decals to bleed
// through nearby occluders, and the decal can get clipped away sooner than the mesh, when the camera comes
// close. But smaller values can cause flickering on vertex-animated meshes.
#define DISTANCE_FADE_THRESHOLD			0.04


#define ClippingEllipseMatrix	VSC_Custom_Matrix
#define DecalFade				VSC_Custom_0.x

#define SpecularColor			PSC_Custom_0.xyz
#define SpecularScale			PSC_Custom_1.x
#define SpecularBase			PSC_Custom_1.y
#define AdditiveNormals			(PSC_Custom_1.z != 0.0f)
#define RandomColorMultiplier	PSC_Custom_2.xyz

Texture2D<float4>	tDecalDiffuse	: register( t0 );
SamplerState		sDecalDiffuse	: register( s0 );

#ifdef USE_NORMALS
Texture2D<float4>	tDecalNormals	: register( t1 );
SamplerState		sDecalNormals	: register( s1 );
#endif


#if MSAA_NUM_SAMPLES > 1
Texture2DMS<float4>	tSceneNormals	: register( t2 );
Texture2DMS<float>	tSceneDepth		: register( t3 );
Texture2DMS<uint2>	tSceneStencil	: register( t4 );
#else
Texture2D<float4>	tSceneNormals	: register( t2 );
Texture2D<float>	tSceneDepth		: register( t3 );
Texture2D<uint2>	tSceneStencil	: register( t4 );
#endif


// The input stream for the decal. Many options are only used in some situations...
struct VS_INPUT
{
	float3		Position			: POSITION0;
#ifdef USE_SKINNING
	uint4		BlendIndices		: BLENDINDICES0;
#ifndef SINGLE_BONE
	float4		BlendWeights		: BLENDWEIGHT0;
#endif
#endif

	float3		Normal				: NORMAL;
#ifdef USE_NORMALS
	float4		Tangent				: TANGENT;
#endif

	float4		Data				: TEXCOORD4;		// position within decal space, fade.
};

struct VS_OUTPUT
{
	float4		Position			: SYS_POSITION;
	float4		UVFade				: TEXCOORD0;
	float3		ClippingEllipsePos	: TEXCOORD1;

	float3		WorldNormal			: NORMAL;
#ifdef USE_NORMALS
	float3		WorldTangent		: TANGENT;
	float3		WorldBinormal		: BINORMAL;
#endif

	float3		WorldPosition		: TEXCOORD2;

#ifdef USE_VERTEX_FOG
	float4		paramsFog			: TEXCOORD3;
	float4		paramsAerial		: TEXCOORD4;
#endif
};

struct PS_OUTPUT
{
	float4		Color				: SYS_TARGET_OUTPUT0;
};


#ifdef VERTEXSHADER

VS_OUTPUT vs_main( VS_INPUT input )
{
	VS_OUTPUT o;
	
	float4 localPosition = float4( input.Position.xyz * VSC_QS.xyz + VSC_QB.xyz, 1 );
#ifdef USE_SKINNING
	float4x4 SkinningMatrix = (float4x4) 0;
	
#ifdef SINGLE_BONE
	float4 blendWeights = float4(1,0,0,0);
#else
	float4 blendWeights = input.BlendWeights;
#endif
	float wet = 0.0f;
	float4 skinnedPosition			= ApplySkinning( localPosition, blendWeights, input.BlendIndices, VSC_SkinningData, SkinningMatrix, wet );
	float4x4 skinnedLocalToWorld	= mul( SkinningMatrix, VSC_LocalToWorld );
#else
	float4 skinnedPosition			= localPosition;
	float4x4 skinnedLocalToWorld	= VSC_LocalToWorld;
#endif


	o.ClippingEllipsePos = mul( localPosition, ClippingEllipseMatrix ).xyz;

	// Move the point closer to the camera by the distance fade threshold. This way we can still draw with depth testing, but can
	// handle cases where an animated VS brings a vertex closer to the camera, or variation from LOD differences.
	float4 worldPosition = mul( skinnedPosition, VSC_LocalToWorld );

	float3 camToVert = worldPosition.xyz - VSC_CameraPosition.xyz;
	float distToCam = length( camToVert );
	float newDistToCam = max( distToCam - DISTANCE_FADE_THRESHOLD, 0.0f );
	float3 shiftedWorldPos = VSC_CameraPosition.xyz + camToVert * ( newDistToCam / distToCam );

	o.Position = mul( float4( shiftedWorldPos, 1.0f ), VSC_WorldToScreen );
	
	o.UVFade.xyz	= input.Data.xyz;
	o.UVFade.w		= input.Data.w * DecalFade;

	// Transform local tangent space to world tangent space
	o.WorldNormal	= normalize( mul( DecompressNormal( input.Normal ),		(float3x3)skinnedLocalToWorld ) );
#ifdef USE_NORMALS
	o.WorldTangent	= normalize( mul( DecompressNormal( input.Tangent.xyz ),	(float3x3)skinnedLocalToWorld ) );
	
	float binormSign = input.Tangent.w * 2 - 1;
	o.WorldBinormal	= normalize( cross( o.WorldNormal, o.WorldTangent ) * binormSign );
#endif

	o.WorldPosition = worldPosition.xyz;
	
#ifdef USE_VERTEX_FOG
	SFogData fogData = CalculateFogFull( false, false, worldPosition.xyz );
	o.paramsFog = fogData.paramsFog;
	o.paramsAerial = fogData.paramsAerial;
#endif

	return o;
}

[MAX_VERTEX_COUNT(3)]
void gs_main( GS_INPUT_TRIANGLE VS_OUTPUT input[3], inout GS_BUFFER_TRIANGLE<VS_OUTPUT> triStream )
{
	// Since a decal probably won't be completely covering a mesh, we'll only pass through triangles that might be drawn on.

#if 1
	float3 us, vs, ws, fades, clips;
	for ( int i = 0; i < 3; ++i )
	{
		us[i] = input[i].UVFade.x;
		vs[i] = input[i].UVFade.y;
		ws[i] = input[i].UVFade.z;
		fades[i] = input[i].UVFade.w;
	
		clips[i] = dot( input[i].ClippingEllipsePos, input[i].ClippingEllipsePos );
	}

	// If all vertices have 0 fade, then we can skip the triangle.
	if ( all( fades <= 0 ) ) return;

	// If all vertices are off one side of the texture (e.g. all have U<0), we can skip it. It's not a perfect test
	// and may miss large triangles, but it's fast and should be good enough.
	if ( all( us < 0 ) || all( us > 1 ) || all( vs < 0 ) || all( vs > 1 ) ) return;

	// If all vertices are in front of the near or behind the far planes, skip.
	if ( all( ws < 0 ) || all ( ws > 1 ) ) return;

	// If all vertices are inside the clipping ellipse, we can skip the triangle.
	if ( all( clips <= 1 ) ) return;
#endif

	// Didn't get rejected, so pass-through.
	triStream.Append( input[0] );
	triStream.Append( input[1] );
	triStream.Append( input[2] );
}

#endif


#ifdef PIXELSHADER


struct SurfaceProperties
{
	float3 Color;
	float3 Specular;
	float Roughness;
	float3 Normal;
	float Alpha;
};


// Get surface properties for this pixel. This is largely based on what pbr_std generates.
SurfaceProperties GetSurfaceProps( VS_OUTPUT input )
{
	SurfaceProperties props = (SurfaceProperties)0;

	float2 uv = input.UVFade.xy;
	float dist = input.UVFade.z;
	float fade = saturate( input.UVFade.w );

	float4 colorTexSample = tDecalDiffuse.Sample( sDecalDiffuse, uv );

	props.Color = pow( abs(colorTexSample.xyz), GAMMA_TO_LINEAR_EXPONENT );
	
	props.Color *= RandomColorMultiplier;
	props.Alpha = colorTexSample.w * fade;

#ifdef USE_NORMALS
	float4 normalsTexSample = tDecalNormals.Sample( sDecalNormals, uv );

	float3 normalSample = normalsTexSample.xyz * 2 - 1;
	props.Normal = normalize( normalSample.x * input.WorldTangent + normalSample.y * input.WorldBinormal + normalSample.z * input.WorldNormal );

	props.Roughness = normalsTexSample.w;
#else
	props.Normal = normalize( input.WorldNormal );
	props.Roughness = MATERIAL_DEFAULT_ROUGHNESS;
#endif
	
	float3 specularLinear = pow( abs(SpecularColor), GAMMA_TO_LINEAR_EXPONENT );
	float3 finalSpecLinear = saturate( ( specularLinear + SpecularBase ) * SpecularScale );
	props.Specular = pow( finalSpecLinear, GAMMA_TO_LINEAR_EXPONENT_INV );

	return props;
}

float3 SampleSceneNormal( int2 screenPosition )
{
#if MSAA_NUM_SAMPLES > 1
	return normalize( tSceneNormals.Load( screenPosition, 0 ).xyz * 2 - 1 );
#else
	return normalize( tSceneNormals[screenPosition].xyz * 2 - 1 );
#endif
}

float3 SampleSceneRoughness( int2 screenPosition )
{
#if MSAA_NUM_SAMPLES > 1
	return tSceneNormals.Load( screenPosition, 0 ).w;
#else
	return tSceneNormals[screenPosition].w;
#endif
}




/// This stuff here is largely based on pbr_std, for a basic material with optional normal map.
PS_OUTPUT ps_main( VS_OUTPUT input )
{
	const uint2 pixelCoord = (uint2)input.Position.xy;

#if 0
	{
		// For debug... shows effectiveness of GS at clipping "un-decal'd" triangles.
		PS_OUTPUT output;
		output.Color = 1.0f;
		return output;
	}
#endif

	// Discard if we're inside the clipping ellipse.
	clip( dot( input.ClippingEllipsePos, input.ClippingEllipsePos ) - 1.0 );

	// Discard if we're outside the decal, or faded out
	clip( input.UVFade.xyzw );
	clip( 1.0 - input.UVFade.xyz );

#if 0
	{
		// For debug... shows where we're inside the decal's space
		PS_OUTPUT output;
		output.Color = 1.0;
		return output;
	}
#endif

	SurfaceProperties surfaceProps = GetSurfaceProps( input );

#ifdef USE_DISTANCE_FADE
	// If the difference in depth between this point and the matching point in the scene is significantly different,
	// we want to skip it. This prevents cases where the decal ends up floating where the underlying surface is
	// alpha-tested.
#if MSAA_NUM_SAMPLES > 1
	const float sceneDepth = tSceneDepth.Load( input.Position.xy, 0 ).x;
#else
	const float sceneDepth = tSceneDepth[input.Position.xy].x;
#endif
#endif

#if MSAA_NUM_SAMPLES > 1
	const uint sceneStencil = GetStencilValue( tSceneStencil.Load( input.Position.xy, 0 ) );
#else
	const uint sceneStencil = GetStencilValue( tSceneStencil[input.Position.xy] );
#endif

#ifdef USE_DISTANCE_FADE
	// Compare the decal's position with the position of the same point in the scene. If the points are far apart,
	// then we are probably on a part of the surface that was alpha-tested out (or behind another object). We do
	// allow some wiggle room, because the decal will not have any vertex shader animation (like for fake fur) that
	// the underlying mesh might have, so it can vary a little.
	float3 sceneWorldPosition = PositionFromDepthRevProjAware( sceneDepth, pixelCoord );
	float decalSceneDist = length( sceneWorldPosition - input.WorldPosition );
	surfaceProps.Alpha *= 1.0 - saturate( decalSceneDist / DISTANCE_FADE_THRESHOLD );
#endif

	// Discard if we're totally faded out.
	clip( surfaceProps.Alpha - 0.001f );

#ifdef USE_NORMALS
#ifdef ADDITIVE_NORMALS
	// For additive normals, we need to combine the surface normal with the existing scene normal.
	float3 sceneNormal = SampleSceneNormal( input.Position.xy );
	surfaceProps.Normal = sceneNormal + surfaceProps.Normal * surfaceProps.Alpha;
#endif
#else
	// Don't have a normal map, so the decal will use the scene normals directly.
	surfaceProps.Normal = SampleSceneNormal( input.Position.xy );
	surfaceProps.Roughness = SampleSceneRoughness( input.Position.xy ).x;
#endif


	// Pass the surface properties off the the PBR lighting calculations.
#ifdef __PSSL__
	SCompileInLightingParams extraLightingParams;
#else
	SCompileInLightingParams extraLightingParams = (SCompileInLightingParams)0;
#endif

	extraLightingParams.stencilValue = sceneStencil;
	const bool useSSAO = true;
	const bool useShadowBuffer = true;
	const float transmission = 0.0f;

	PS_OUTPUT output;
	output.Color = float4( 0.0f, 0.0f, 0.0f, 1.0f );

	float probeMipIndex = PrecomputeCalculateLightingEnvProbeMipIndex( input.WorldPosition.xyz, surfaceProps.Normal.xyz, input.Position.xy );

#ifdef USE_VERTEX_FOG
	const bool applyGlobalFog = false;
#else
	const bool applyGlobalFog = true;
#endif

	output.Color.xyz = CalculateLightingPBRPipeline( input.WorldPosition, surfaceProps.Color, surfaceProps.Normal, 
		input.WorldNormal, surfaceProps.Specular, surfaceProps.Roughness, transmission, input.Position.xy, 
		extraLightingParams, useSSAO, useShadowBuffer, probeMipIndex, applyGlobalFog );

#ifdef USE_VERTEX_FOG
	SFogData fogData;
	fogData.paramsFog = input.paramsFog;
	fogData.paramsAerial = input.paramsAerial;
	output.Color.xyz = ApplyFog_AlphaBlend( output.Color.xyz, surfaceProps.Alpha, fogData ).xyz;
#endif

	// Pre-multiplied alpha
	output.Color *= surfaceProps.Alpha;

	return output;
}


#endif
