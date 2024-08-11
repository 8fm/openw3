#include "terrainUtilities.fx"

/// Terrain GBuffer material
/// It's esential to recompile all terrain shaders after changing any of those lines

// ====== Texture arrays

// t0/s0 is reserved for height clipmap

SamplerState			sAnisoWrapSampler : register( s0 );

TEXTURE2D_ARRAY<uint>	tCMClipMap 	: register( t5 );
SamplerState  			sControlMap : register( s5 );

TEXTURE2D_ARRAY<float4>	tColorMap			: register( t3 );
SamplerState  			sLinearClampNoMip	: register( s3 );

TEXTURE2D_ARRAY<float2>	tNormals	: register( t4 );

#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
Texture2D<float4>		tStampColor		: register( t6 );
SamplerState  			sStampColor		: register( s6 );
Texture2D<uint>			tStampControl	: register( t7 );
#endif

#ifdef MS_DISPLAY_MASK
Texture2D<uint>			tGrassMask			: register( t8 );
#endif

#ifdef MS_DISPLAY_OVERLAY
Texture2D				tOverlay			: register( t9 );
#endif

#ifdef MS_HEIGHTMAP
TEXTURE2D_ARRAY<float>	tHeightmap 			: register( t10 );
SamplerState  			sHeightmap			: register( s10 );
#endif

#include "terrainConstants.fx"
DEFINE_TERRAIN_PARAMS_CB( 5 );

// ======

// PSC_Custom_0 -- stamp parameters (when RS_TERRAIN_TOOL_STAMP_VISIBLE defined)
//   x: whether color stamp is available
//   y: whether control stamp is available
//   z: size of original (unscaled) stamp data
//   w: size of stamp in world

struct TerrainMaterialBlended
{
	float4		Diffuse;
	float4		Normal;
	float4		Params;

	float		Roughness;
};

/////////////////////////////////////////////////////////
// Utilities

#define USE_FLAT_TERRAIN_NORMAL		0
#define USE_FLAT_NORMALS			0
#define USE_LOWEST_MIPS				0
#define FLAT_NORMAL					float4 ( 1, 0.5, 1, 0.5 )

float4 SampleTriplanarColor( TEXTURE2D_ARRAY texArray, SamplerState texSampler, float texIndex, float3x2 triplanarUV, float3 weights, float3x4 ddxys )
{
	float4 color = 0.0f;
#if USE_LOWEST_MIPS
	float _sample_level = 999;
	[branch] if ( weights.z > 0.0f ) color += weights.z * SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[0], texIndex ), _sample_level );
	[branch] if ( weights.y > 0.0f ) color += weights.y * SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[1], texIndex ), _sample_level );
	[branch] if ( weights.x > 0.0f ) color += weights.x * SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[2], texIndex ), _sample_level );
#else
	[branch] if ( weights.z > 0.0f ) color += weights.z * SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[0], texIndex ), ddxys[0].xy, ddxys[0].zw );
	[branch] if ( weights.y > 0.0f ) color += weights.y * SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[1], texIndex ), ddxys[1].xy, ddxys[1].zw );
	[branch] if ( weights.x > 0.0f ) color += weights.x * SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[2], texIndex ), ddxys[2].xy, ddxys[2].zw );
#endif
	return color;
}

float4 SampleTriplanarNormal( TEXTURE2D_ARRAY texArray, SamplerState texSampler, float texIndex, float3x2 triplanarUV, float3 weights, float3 vertexNormal, float3x4 ddxys )
{
#if USE_FLAT_NORMALS
	return FLAT_NORMAL;
#else

	float4 colorXY = 0.0f;
	float4 colorXZ = 0.0f;
	float4 colorYZ = 0.0f;
#if USE_LOWEST_MIPS
	float _sample_level = 999;
	[branch] if ( weights.z > 0.0f ) colorXY = SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[0], texIndex ), _sample_level );
	[branch] if ( weights.y > 0.0f ) colorXZ = SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[1], texIndex ), _sample_level );
	[branch] if ( weights.x > 0.0f ) colorYZ = SAMPLE_LEVEL( texArray, texSampler, float3( triplanarUV[2], texIndex ), _sample_level );
#else
	[branch] if ( weights.z > 0.0f ) colorXY = SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[0], texIndex ), ddxys[0].xy, ddxys[0].zw );
	[branch] if ( weights.y > 0.0f ) colorXZ = SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[1], texIndex ), ddxys[1].xy, ddxys[1].zw );
	[branch] if ( weights.x > 0.0f ) colorYZ = SAMPLE_GRADIENT( texArray, texSampler, float3( triplanarUV[2], texIndex ), ddxys[2].xy, ddxys[2].zw );
#endif

	// those ifs could be switched to step lerp if any performance problems occur
	if( vertexNormal.y < 0 ) 
	{
		colorXZ.y = 1 - colorXZ.y;
	}
	colorYZ.y = 1 - colorYZ.y;

	return weights.z * colorXY + weights.y * colorXZ + weights.x * colorYZ;

#endif
}

float4 SampleOnePlanarColor( TEXTURE2D_ARRAY texArray, SamplerState texSampler, float texIndex, float2 uv, float4 ddxy )
{
#if USE_LOWEST_MIPS
	float _sample_level = 999;
	float4 colorXY = SAMPLE_LEVEL( texArray, texSampler, float3( uv, texIndex ), _sample_level );
#else
	float4 colorXY = SAMPLE_GRADIENT( texArray, texSampler, float3( uv, texIndex ), ddxy.xy, ddxy.zw );
#endif

	return colorXY;
}

float4 SampleOnePlanarNormal( TEXTURE2D_ARRAY texArray, SamplerState texSampler, float texIndex, float2 uv, float4 ddxy )
{
#if USE_FLAT_NORMALS
	return FLAT_NORMAL;
#else
	return SampleOnePlanarColor( texArray, texSampler, texIndex, uv, ddxy );
#endif
}

float3 CombineNormalsDerivatives( float3 normal1, float3 normal2, float3 ratios )
{
	// ratios.x = weight of first normal
	// ratios.y = weight of second normal
	// ratios.z = bumpiness of final normal
	
	float2 dxy = float2( -normal1.xy / normal1.zz );
	float2 dxy2 = float2( -normal2.xy / normal2.zz );
	return  normalize( float3( -ratios.x * dxy - ratios.y * dxy2, ratios.z ) );
}

float3 CombineNormalsTangent( float3 vertexNormal, float3 textureNormal, float3 worldTangent, float3 worldBinormal )
{	
	return textureNormal.x * worldTangent + textureNormal.y * worldBinormal + textureNormal.z * vertexNormal;
}

float3 DecompressNormal( float4 normal )
{	
// 	// DXT1/DXT5
// 	normal  = 2.0 * normal - 1.0;
// 	float x = normal.w * normal.x;
// 	float y = normal.y;
// 	float z = pow( max( 0, 1 - x*x - y*y ), 0.5 );
// 
// 	return float3( x,y,z );

	return normalize( normal.xyz - 0.5 );
}



void EvaluateMaterial(
	float3 vertexNormal,
	float3 worldTangent,
	float3 worldBinormal,

	float2 subUV,						// fraction between clipmap samples
	float3 weights,						// triplanar weights

	uint4 horizontalTexturesIndices,
	uint4 verticalTexturesIndices,
	uint4 slopeThresholdIndices,
	uint4 uvMultFlagsVertical,

	float3x2 triplanarUV,				// unscaled UV for each plane
	float3x4 triplanarDDXY,				// (ddx,ddy) of each UV
	
	TEXTURE2D_ARRAY diffuseArray,
	TEXTURE2D_ARRAY normalArray,

	out float3 outColor,
	out float3 outNormal,
	out float4 outMaterialParams,
	out float outRoughness
	)
{
	//const float scales[7] = { 0.333f, 0.166f, 0.05f, 0.025f, 0.0125f, 0.0075f, 0.00375f };
	const float slopeThresholds[8] = { 0.0f, 0.125f, 0.25, 0.375f, 0.5f, 0.625f, 0.75f, 0.98f };

#if MS_LOW_DETAIL

	float2x4 texParamsHorizontalVec[4];
	float2x4 texParamsVerticalVec[4];
	for ( uint i=0; i<4; ++i )
	{
		texParamsHorizontalVec[i] = TexParams[ horizontalTexturesIndices[i] ];
		texParamsVerticalVec[i] = TexParams[ verticalTexturesIndices[i] ];
	}

	const float2x4 texParamsHorizontal = lerp( lerp( texParamsHorizontalVec[0], texParamsHorizontalVec[2], subUV.y ), lerp( texParamsHorizontalVec[1], texParamsHorizontalVec[3], subUV.y ), subUV.x );
	const float2x4 texParamsVertical = lerp( lerp( texParamsVerticalVec[0], texParamsVerticalVec[2], subUV.y ), lerp( texParamsVerticalVec[1], texParamsVerticalVec[3], subUV.y ), subUV.x );


	const float slopeBasedDamp = texParamsVertical[0].y;


	// Use slope to lerp between vertical and horizontal surface colors, normals and material params (spec, gloss, fallof)
	float3 finalColor;
	float3 finalNormal;
	float4 materialParams;
	float finalRougness;


	// Interpolate samples across inter-vertex space
	float4 color;
	float3 normal;
	float roughness;

	const float horizontalToVerticalBlendSharpness = texParamsHorizontal[0].x;

	// Apply dampening to the backround normal
	float vertexSlope = dot( vertexNormal, float3( 0, 0, 1 ) );
	float3 flattenedCombinedVerticalNormal = lerp( vertexNormal, float3( 0, 0, 1 ), vertexSlope );
	float3 biasedFlattenedCombinedVerticalNormal = normalize( lerp( vertexNormal, flattenedCombinedVerticalNormal, slopeBasedDamp ) );

	// Pick a single plane, based on heighest weight.
	int weightBasedIdx = all( weights.zz >= weights.xy ) ? 0 :
				all( weights.yy >= weights.xz ) ? 1 :
				2;

	float4 colorVec[4];
	float4 normalVec[4];
	[unroll]
	for ( uint i=0; i<4; ++i )
	{
		const float slopeThreshold = slopeThresholds[ slopeThresholdIndices[i] ];
		const float verticalSurfaceTangent = ComputeSlopeTangent( biasedFlattenedCombinedVerticalNormal, slopeThreshold, saturate( slopeThreshold + horizontalToVerticalBlendSharpness ) );
		const bool useHorz = verticalSurfaceTangent < 0.5f;

		const int idx = useHorz ? 0 : weightBasedIdx;

		const float scale = useHorz ? 0.333f : scales[ uvMultFlagsVertical[i] ];

		const float2 planarUV = triplanarUV[ idx ] * scale;

		const float4 DDXY = triplanarDDXY[ idx ] * scale;

		const uint4 textureIndices = useHorz ? horizontalTexturesIndices : verticalTexturesIndices;
		float textureIndex = (float)textureIndices[i];

		colorVec[i] = SampleOnePlanarColor( diffuseArray, sAnisoWrapSampler, textureIndex, planarUV, DDXY );
		normalVec[i] = SampleOnePlanarNormal( normalArray, sAnisoWrapSampler, textureIndex, planarUV, DDXY );
	}

	// Interpolate samples across inter-vertex space
	color = lerp( lerp( colorVec[0], colorVec[2], subUV.y ), lerp( colorVec[1], colorVec[3], subUV.y ), subUV.x );

	float4 normalCompressed = lerp( lerp( normalVec[0], normalVec[2], subUV.y ), lerp( normalVec[1], normalVec[3], subUV.y ), subUV.x );
	normal = DecompressNormal( normalCompressed );
	roughness = normalCompressed.w;


	// Use slope to lerp between vertical and horizontal surface colors, normals and material params (spec, gloss, fallof)
	outColor			= color;
	outNormal			= CombineNormalsTangent( vertexNormal, normal, worldTangent, worldBinormal );
	outMaterialParams	= texParamsHorizontal[1];
	outRoughness		= roughness;


#else


	// Find factors for bilinear interpolation between four corner samples
	float4 lerpw;
	{
		const float x0 = 1 - subUV.x;
		const float x1 = subUV.x;
		const float y0 = 1 - subUV.y;
		const float y1 = subUV.y;
		lerpw = float4( x0*y0, x1*y0, x0*y1, x1*y1 );
	}

	float4 horizontalColor;
	float3 horizontalNormal;
	float horizontalRoughness;
	float slopeThreshold;
	float2x4 texParamsHorizontal;
	{
		horizontalColor		= 0.0f;
		slopeThreshold		= 0.0f;
		texParamsHorizontal	= 0.0f;
		float4 horizontalNormalCompressed = 0.0f;

		const float4 DDXY = triplanarDDXY[0] * 0.333f;

		[unroll]
		for ( uint i=0; i<4; ++i )
		{
			float textureIndex = (float)horizontalTexturesIndices[i];

			horizontalColor				+= lerpw[i] * SampleOnePlanarColor( diffuseArray, sAnisoWrapSampler, textureIndex, triplanarUV[0] * 0.333f, DDXY );
			horizontalNormalCompressed	+= lerpw[i] * SampleOnePlanarNormal( normalArray, sAnisoWrapSampler, textureIndex, triplanarUV[0] * 0.333f, DDXY );
			texParamsHorizontal			+= lerpw[i] * TexParams[ horizontalTexturesIndices[i] ];
			slopeThreshold				+= lerpw[i] * slopeThresholds[ slopeThresholdIndices[i] ];
		}

		horizontalNormal = DecompressNormal( horizontalNormalCompressed );
		horizontalRoughness = horizontalNormalCompressed.w;
	}

	
	float4 verticalColor;
	float3 verticalNormal;
	float verticalRoughness;
	float2x4 texParamsVertical;
	{
		verticalColor					= 0.0f;
		texParamsVertical				= 0.0f;
		float4 verticalNormalCompressed	= 0.0f;

		[unroll]
		for ( uint i=0; i<4; ++i )
		{
			const float3x4 DDXY = triplanarDDXY * scales[ uvMultFlagsVertical[i] ];
			const float3x2 uv = triplanarUV * scales[ uvMultFlagsVertical[i] ];
			
			float textureIndex = (float)verticalTexturesIndices[i];
			verticalColor				+= lerpw[i] * SampleTriplanarColor( diffuseArray, sAnisoWrapSampler, textureIndex, uv, weights, DDXY );
			verticalNormalCompressed	+= lerpw[i] * SampleTriplanarNormal( normalArray, sAnisoWrapSampler, textureIndex, uv, weights, vertexNormal.xyz, DDXY );
			texParamsVertical			+= lerpw[i] * TexParams[ verticalTexturesIndices[i] ];
		}

		verticalNormal = DecompressNormal( verticalNormalCompressed );
		verticalRoughness = verticalNormalCompressed.w;
	}


	const float horizontalToVerticalBlendSharpness = texParamsHorizontal[0].x;
	const float slopeBasedDamp = texParamsVertical[0].y;
	const float slopeBasedNormalDamp = texParamsVertical[0].z;

	// Combine vertex normal with background / overlay material normals
	const float3 combinedVerticalNormal = CombineNormalsTangent( vertexNormal, verticalNormal, worldTangent, worldBinormal );
	const float3 combinedHorizontalNormal = CombineNormalsTangent( vertexNormal, horizontalNormal, worldTangent, worldBinormal );

	// Apply dampening to the backround normal
	const float3 flattenedCombinedVerticalNormal = lerp( combinedVerticalNormal, float3( 0, 0, 1 ), vertexNormal.z );
	const float3 biasedFlattenedCombinedVerticalNormal = normalize( lerp( combinedVerticalNormal, flattenedCombinedVerticalNormal, slopeBasedDamp ) );

	// Compute slope tangent for the dampened background normal
	const float verticalSurfaceTangent = ComputeSlopeTangent( biasedFlattenedCombinedVerticalNormal, slopeThreshold, saturate( slopeThreshold + horizontalToVerticalBlendSharpness ) );
	const float3 fullNormalCombination = CombineNormalsDerivatives( combinedVerticalNormal, combinedHorizontalNormal, 
																float3( 1.0f - slopeBasedNormalDamp, slopeBasedNormalDamp, 1.0f ) );
	
	// Use slope to lerp between vertical and horizontal surface colors, normals and material params (spec, gloss, fallof)
	outColor			= lerp( horizontalColor, verticalColor, verticalSurfaceTangent );
	outNormal			= lerp( fullNormalCombination, combinedVerticalNormal, verticalSurfaceTangent );
	outMaterialParams	= lerp( texParamsHorizontal[1], texParamsVertical[1], verticalSurfaceTangent );
	outRoughness		= lerp( horizontalRoughness, verticalRoughness, verticalSurfaceTangent );

#endif
}


// THE function
TerrainMaterialBlended CalcTerrainMaterial
( 
		float3  worldPos,
		float3 	vertexNormal,	// If pregenerated normal maps are in use AND stamp is visible, then this is the stamp normal
		float2 	regionUV,
		int4 	levelIndex,
		float2	regionUVColorMap,
		TEXTURE2D_ARRAY diffuseArray,
		TEXTURE2D_ARRAY normalArray
#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
		, float2 stampUV, float useStamp, float stampAdditive
#endif
		)
{
	TerrainMaterialBlended output = (TerrainMaterialBlended)0;

#ifdef MS_PREGENERATED_MAPS
	// Sample the normals clipmap
	float3 pregenNormal = DecompressPregenNormal( SAMPLE_LEVEL( tNormals, sLinearClampNoMip, float3( regionUV, levelIndex.x ), 0 ) );
#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	// in this case, vertexNormal contains a stamp normal. Blend it with the pregenerated one.
	if ( useStamp )
	{
		vertexNormal = normalize( stampAdditive * pregenNormal + vertexNormal );
	}
	else
	{
		vertexNormal = pregenNormal;
	}
#else
	// in this case, vertexNormal is a stub, not connected with any domain shader output. Overwrite it with our nice pregenerated normal :)
	vertexNormal = pregenNormal;
#endif
#endif	// else - vertexNormal contains a fair normal vector computed in earlier stage. Nothing to do here.

#if USE_FLAT_TERRAIN_NORMAL
	vertexNormal = float3 ( 0, 0, 1 );
#endif

	// Compute tangent and binormal
	// ace_todo: this is a quick fix for horizontal terrain normals which hurt eyes at this moment. 
	//           vertical normals still don't appear 100% right to me, so this need to be fixed also. 
	float3 worldTangent  = float3 ( 1, 0, 0 );
	worldTangent  = normalize( worldTangent - vertexNormal * dot( vertexNormal, worldTangent ) );
	float3 worldBinormal = cross( vertexNormal, worldTangent );
	
	float3 clipmapDim;
	tCMClipMap.GetDimensions( clipmapDim.x, clipmapDim.y, clipmapDim.z );

#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	float4 stampSettings = { PSC_Custom_0 };
	bool stampHaveColor = useStamp && stampSettings.x;
	bool stampHaveControl = useStamp && stampSettings.y;
	float2 stampSize = stampSettings.ww;
	float2 stampTexelScale = clipmapDim.xy / stampSize;
#endif

	// Sample the central control map value. Do it first, so we can discard early.
	float2 cmTexelCoordsF;
	int2 cmTexelCoords;
	uint4 controlMapValues;
#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	if ( stampHaveControl )
	{
		cmTexelCoordsF = stampUV * stampSize;
		cmTexelCoords = cmTexelCoordsF;
		controlMapValues.x = tStampControl.Load( int4( cmTexelCoords * stampTexelScale, 0, 0 ) );
	}
	else
#endif
	{
		// Not entirely sure why this is needed, but it is required to put the texel samples at the vertices, instead of in the middle
		// of each quad.
		cmTexelCoordsF = regionUV * clipmapDim.xy - 0.5;
		cmTexelCoords = cmTexelCoordsF;
		controlMapValues.x = tCMClipMap.Load( int4( cmTexelCoords, levelIndex.x, 0 ) );
	}

#ifndef MS_DISPLAY_HOLES
	#ifdef MS_DISCARD_PASS
	if ( controlMapValues.x == 0 )
	{
		discard;
	}
	#endif
#endif

	// Sample neighbour control map values
#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	if ( stampHaveControl )
	{
		controlMapValues.y = tStampControl.Load( int4( (cmTexelCoords + int2(1,0)) * stampTexelScale, 0, 0 ) );
		controlMapValues.z = tStampControl.Load( int4( (cmTexelCoords + int2(0,1)) * stampTexelScale, 0, 0 ) );
		controlMapValues.w = tStampControl.Load( int4( (cmTexelCoords + int2(1,1)) * stampTexelScale, 0, 0 ) );
	}
	else
#endif
	{
		controlMapValues.y = tCMClipMap.Load( int4( cmTexelCoords + int2(1,0), levelIndex.x, 0 ) );
		controlMapValues.z = tCMClipMap.Load( int4( cmTexelCoords + int2(0,1), levelIndex.x, 0 ) );
		controlMapValues.w = tCMClipMap.Load( int4( cmTexelCoords + int2(1,1), levelIndex.x, 0 ) );
	}


	// Decode control map values
	uint4 horizontalTexturesIndices;
	uint4 verticalTexturesIndices;
	uint4 slopeThresholdIndices;
	uint4 uvMultFlagsVertical;
	DecodeControlMapValues( controlMapValues, horizontalTexturesIndices, verticalTexturesIndices, slopeThresholdIndices, uvMultFlagsVertical );

	// Compute triplanar mapping weights
	const float3 tighten = (float3)0.576f;
	const float3 absVertexNormal = abs( normalize( vertexNormal ) );
	float3 weights = absVertexNormal - tighten;
	weights = max( weights, float3( 0.0f, 0.0f, 0.0f ) );

	// Normalize weights
	float totalWeight = weights.x + weights.y + weights.z;
	weights /= totalWeight;
	
	// Use three planes for UV projection
	const float3x2 triplanarUV = {
		worldPos.xy,
		-worldPos.xz,
		-worldPos.yz
	};

	// ddx/ddy of each UV set. Don't apply any scaling to the UVs, so that they are smooth and ddx/ddy have no discontinuities.
	// Instead, we'll just scale the derivatives later. d(c*f(x))/dx = c*d(f(x))/dx
	const float3x4 triplanarDDXY = {
		ddx( triplanarUV[0] ), ddy( triplanarUV[0] ),
		ddx( triplanarUV[1] ), ddy( triplanarUV[1] ),
		ddx( triplanarUV[2] ), ddy( triplanarUV[2] ),
	};

	const float2 fractions = frac( cmTexelCoordsF );


	float3 finalColor;
	float3 finalNormal;
	float4 materialParams;
	float finalRoughness;

	EvaluateMaterial(
		vertexNormal,
		worldTangent,
		worldBinormal,

		fractions,
		weights,

		horizontalTexturesIndices,
		verticalTexturesIndices,
		slopeThresholdIndices,
		uvMultFlagsVertical,

		triplanarUV,				// unscaled UV for each plane
		triplanarDDXY,				// (ddx,ddy) of each UV

		diffuseArray,
		normalArray,

		finalColor,
		finalNormal,
		materialParams,
		finalRoughness
		);


	// sample value from colormap as well as neighbours and blend between them
	float4 colorMap	= tColorMap.Sample( sLinearClampNoMip, float3( regionUVColorMap, levelIndex.y ), int2(0,0) );

#ifdef RS_TERRAIN_TOOL_STAMP_VISIBLE
	if ( stampHaveColor )
	{
		// Blend with the stamp colormap. Just using the stamp directly can cause horrible artefacts at the edges of the stamp.
		// They'll still show from control/height samples, but at least the color can be a bit less jarring.
		colorMap = lerp( colorMap, tStampColor.Sample( sStampColor, stampUV ), useStamp );
	}
#endif

	// Apply color map
	output.Diffuse = float4 ( ApplyColorMap( finalColor.xyz, colorMap.xyz ), 1.0 );

#if 0	// Colorize based on casting planes importance
	output.Diffuse = float4( weights.z, weights.x, weights.y, 1.0f );
#endif

//showing only colormap
#if 0
	output.Diffuse = float4( colorMap.xyz, 1.0f );
#endif

#if 0	// Colorize based on number of casting planes (one-planar, duo-planar, tri-planar mapping)
	int numSamples = 0;
	if ( weights.x > 0.0f )
	{
		++numSamples;
	}
	if ( weights.y > 0.0f )
	{
		++numSamples;
	}
	if ( weights.z > 0.0f )
	{
		++numSamples;
	}

	if ( numSamples == 1 )
	{
		output.Diffuse = float4( 0.0f, 1.0f, 0.0f, 1.0f );
	}
	else if ( numSamples == 2 )
	{
		output.Diffuse = float4( 1.0f, 1.0f, 0.0f, 1.0f );
	}
	else if ( numSamples == 3 )
	{
		output.Diffuse = float4( 1.0f, 0.0f, 0.0f, 1.0f );
	}
#endif

#ifdef MS_DISPLAY_HOLES
	if ( controlMapValues.x == 0  )
	{
		output.Diffuse = float4( 1.0f, 0.0f, 0.0f, 1.0f );
	}
#endif

#if 0
	float debugLevelValue = levelIndex.x;
	if ( debugLevelValue == 0 )
	{
		output.Diffuse = float4( 1.0f, 0.0f, 0.0f, 1.0f );
	}
	if ( debugLevelValue == 1 )
	{
		output.Diffuse = float4( 0.0f, 1.0f, 0.0f, 1.0f );
	}
	if ( debugLevelValue == 2 )
	{
		output.Diffuse = float4( 0.0f, 0.0f, 1.0f, 1.0f );
	}
	if ( debugLevelValue == 3 )
	{
		output.Diffuse = float4( 0.0f, 1.0f, 1.0f, 1.0f );
	}
	if ( debugLevelValue == 4 )
	{
		output.Diffuse = float4( 1.0f, 0.0f, 1.0f, 1.0f );
	}
#endif
	
#if 0
	{
		output.Diffuse = float4 ( colorMap.xyz, 1 );
	}
#endif

#if 0
	{
		output.Diffuse.xyz = pow( pregenNormal * 0.5 + 0.5, 1.0 / 2.2 );
	}
#endif

#if 0 // Used to visualize the error map, but can be reused for other purposes when needed
	float3 errorMapDim;
	tOverlay.GetDimensions( errorMapDim.x, errorMapDim.y, errorMapDim.z );

	float2 emTexelCoordsF = regionUV * errorMapDim.xy;
	int2 emTexelCoords = emTexelCoordsF;
	output.Diffuse = max( tOverlay.Load( int4( emTexelCoords, levelIndex.x, 0 ) ), float4(0.33,0.33,0.33,0.33) );
	output.Normal = float4( 0.0f, 0.0f, 1.0f, 1.0f );
	output.Params = float4( 0.0f, 0.0f, 0.0f, 0.0f );
	output.Roughness = 0.0f;
#endif

#if 0
	#ifdef MS_DISCARD_PASS
		output.Diffuse = float4( 1.0f, 0.0f, 1.0f, 1.0f );
	#endif
#endif

#ifdef MS_DISPLAY_MASK
	{
		// Overwrite output color by grass mask
		float2 grassMaskDim;
		tGrassMask.GetDimensions( grassMaskDim.x, grassMaskDim.y );
		const float grassMaskRes = PSC_Custom_1.x;
		const float2 terrainEdgePos = -TerrainParams.terrainEdgeLength.xx/2.0f;
		const int2 grassMaskCoord = ( saturate( ( worldPos.xy - terrainEdgePos.xy ) / TerrainParams.terrainEdgeLength.xx ) ) * grassMaskRes;

		const int bitIndex = ( grassMaskCoord.y * grassMaskRes + grassMaskCoord.x );
		const int byteIndex = bitIndex / 8;
		const int row = byteIndex / grassMaskDim.x;
		const int col = byteIndex - row * grassMaskDim.x;

		const uint byteVal = tGrassMask.Load( int3( col, row, 0 ) );
		const bool isMaskedOut =  ( ( byteVal & ( 1 << ( (bitIndex) % 8 ) ) ) == 0 );
		if ( isMaskedOut )
		{
			output.Diffuse = float4( 1.0f, 0.0f, 0.0f, 0.0f );
		}
		else
		{
			output.Diffuse = float4( 0.0f, 1.0f, 0.0f, 0.0f );
		}
	}
#endif

#ifdef MS_DISPLAY_OVERLAY
	{
		float4 overlayRect = PSC_Custom_3.xyzw;
		if ( worldPos.x > overlayRect.x && worldPos.x < overlayRect.z && worldPos.y > overlayRect.y && worldPos.y < overlayRect.w )
		{
			float overlayWidth = abs( overlayRect.z - overlayRect.x );
			float overlayHeight = abs( overlayRect.w - overlayRect.y );
			float2 overlayUV = float2( ( worldPos.x - overlayRect.x ) / overlayWidth, ( worldPos.y - overlayRect.y ) / overlayHeight );

			float2 dims;
			tOverlay.GetDimensions( dims.x, dims.y );
			float4 visualisation = tOverlay.Load( int3( overlayUV * dims, 0 ) );

			// Simple average of RGB to get a grey value, so we don't end up trying to color green grass red, and it comes out overly dark.
			// Also, bump it up a bit so it has minimum brightness of 0.1, to avoid near-black spots.
			float grey = dot( output.Diffuse.xyz, float3( 0.333f, 0.333f, 0.333f ) ) * 0.9f + 0.1f;
			output.Diffuse = visualisation.a > 0.0f ? grey * visualisation : output.Diffuse;
		}
	}
#endif

#ifdef MS_WIREFRAME
	output.Diffuse = float4( 0.0f, 0.0f, 0.0f, 1.0f );
#endif

#ifdef MS_HEIGHTMAP
	float heightmapValue = tHeightmap.Sample( sHeightmap, float3( regionUVColorMap, levelIndex.y ), int2(0,0) );
	output.Diffuse	= float4( heightmapValue, heightmapValue, heightmapValue, 1.0f );
#endif

#ifdef MS_BLACK_MASK
	output.Diffuse	= float4( 0.0f, 0.0f, 0.0f, 1.0f );
#endif

	output.Normal = float4( finalNormal, 1.0f );
	output.Params = materialParams;
	output.Roughness = finalRoughness;

	/*output.Diffuse = float4( 0.0f, 0.0f, 1.0f, 1.0f );
	output.Normal = float4( 0.0f, 1.0f, 0.0f, 1.0f );
	output.Params = float4( 0.0f, 0.0f, 0.0f, 0.0f );
	output.Roughness = 0.0f;*/
	return output;
}
