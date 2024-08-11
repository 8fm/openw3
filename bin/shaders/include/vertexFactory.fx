/// Header for vertex factory functionality
/// It's esential to recompile all shaders after changing any of those lines

/// The FAT output vertex
struct VS_FAT_VERTEX
{
	float4x4	LocalToWorld;
	float3		VertexPosition;
	float3		VertexNormal;
	float4		VertexColor;	
	float3		WorldPosition;
	float3		WorldNormal;
	float3		WorldBinormal;
	float3		WorldTangent;
	float4		ViewPosition;
	float4		ScreenPosition;
	float4		LightPosition;
	float2		UV;
	float2		UV2;
	float4		ProjUV;
	float4		Color;
	float		AnimationFrame;
	float		MotionBlend;
	float4		DissolveParams;
	float3		FoliageColor;
	float3		SkeletalExtraData;
	int4		TexIndex;
	float2		DissolveUV;
	float3		ClippingEllipsePos;
};

// Decompress normal vector
float3 DecompressNormal( float3 compressedNormal )
{
	return 2.0 * compressedNormal - 1.0;
}

// Project vertex
VS_FAT_VERTEX SetPosition( VS_FAT_VERTEX vertex, float3 worldPosition )
{
	// Calculate position in world space
	vertex.WorldPosition = worldPosition;

	// Calculate position in view space
	vertex.ViewPosition = mul( float4( vertex.WorldPosition, 1 ), VSC_WorldToView );
	
	// Calculate position in screen space
	vertex.ScreenPosition = mul( float4( vertex.WorldPosition, 1 ), VSC_WorldToScreen );	
	
	// Calculate position in light space
	vertex.LightPosition = float4 ( 0, 0, 0, 1 ); // ace_todo: remove LightPosition since it's unused anyway (or at least not properly initiated)
	
	// Return updated data
	return vertex;
}

// Project vertex
VS_FAT_VERTEX CalcPosition( VS_FAT_VERTEX vertex, float3 localPosition )
{
	// Calculate position in world space
	vertex.WorldPosition = mul( float4( localPosition, 1 ), vertex.LocalToWorld ).xyz;;

	// Calculate position in view space
	vertex.ViewPosition = mul( float4( localPosition, 1 ), mul( vertex.LocalToWorld, VSC_WorldToView) );
	
	// Calculate position in screen space
	vertex.ScreenPosition = mul( float4( localPosition, 1 ), mul( vertex.LocalToWorld, VSC_WorldToScreen) );
	
	// Calculate position in light space
	vertex.LightPosition = float4 ( 0, 0, 0, 1 ); // ace_todo: remove LightPosition since it's unused anyway (or at least not properly initiated)
	
	// Return updated data
	return vertex;
}

// Calculate world space tangent space
VS_FAT_VERTEX CalcTangentSpace( VS_FAT_VERTEX vertex, float3 localNormal, float3 localBinormal, float3 localTangent )
{
	// Transform local tangent space to world tangent space
	vertex.WorldNormal = normalize( mul( localNormal, (float3x3)vertex.LocalToWorld ) );
	vertex.WorldBinormal = normalize( mul( localBinormal, (float3x3)vertex.LocalToWorld ) );
	vertex.WorldTangent = normalize( mul( localTangent, (float3x3)vertex.LocalToWorld ) );
	
	// Return updated data
	return vertex;	
}	

// Create default vertex
VS_FAT_VERTEX DefaultVertex()
{
	VS_FAT_VERTEX ret = (VS_FAT_VERTEX)0;
	
	// Use the constants version of local to world
	ret.LocalToWorld = VSC_LocalToWorld;
	ret.LocalToWorld[0][3] = 0.f;
	ret.LocalToWorld[1][3] = 0.f;
	ret.LocalToWorld[2][3] = 0.f;
	ret.LocalToWorld[3][3] = 1.f;
	
	// Default color is white
	ret.VertexColor = float4( 1,1,1,1 );
	ret.Color = float4( 1,1,1,1 );
	ret.FoliageColor = float3( 1,1,1 );
	
	// Default normal vector
	ret.VertexNormal = float3( 0,0,1 );
	
	// Default tangent space
	ret.WorldNormal = float3( 0,0,1 );
	ret.WorldBinormal = float3( 0,1,0 );
	ret.WorldTangent = float3( 1,0,0 );

	// No dissolve params
	ret.DissolveParams = VSC_DissolveParams;

	// Init to 1,1,1 makes sure it is outside of the unit sphere, so won't be clipped.
	ret.ClippingEllipsePos = float3( 1, 1, 1 );
	
	return ret;
}
	
/// Load 3 float4 vectors into 4x3 matrix
float4x3 LoadMatrix43( float4 x, float4 y, float4 z )
{
	return float4x3( x.x, y.x, z.x, x.y, y.y, z.y, x.z, y.z, z.z, x.w, y.w, z.w );
}

/// Load 4 float4 vectors into 4x3 matrix
float4x4 LoadMatrix44( float4 x, float4 y, float4 z, float4 w )
{
	return float4x4( x.x, x.y, x.z, x.w, y.x, y.y, y.z, y.w, z.x, z.y, z.z, z.w, w.x, w.y, w.z, w.w );
}


//////////////////////////////////////////////////////////////////////////
// Skinning

STRUCTBUFFER(float4x4) b_SkinBuffer : register( t0 );

float4x4 TexFetchBoneMatrixWithExtraData( uint BoneIndex, float4 SkinningData, out float extraData )
{
	float4x4 result = transpose(b_SkinBuffer[ SkinningData.x + BoneIndex * SkinningData.y ]);
	extraData = result[3][3];
	result[3][3] = 1.0f;
	return result;
}

float4x4 TexFetchBoneMatrix( uint BoneIndex, float4 SkinningData )
{
	float dummyData;
	return TexFetchBoneMatrixWithExtraData( BoneIndex, SkinningData, dummyData );
}

// Compile skinning matrix
float4x4 CompileSkinningMatrix( float4 BlendWeights, uint4 BlendIndices, float4 SkinningData )
{
	// Generate the skinning matrix
	float4x4 ret = (float4x4) 0;
	float weightSum = BlendWeights.x + BlendWeights.y + BlendWeights.z + BlendWeights.w;	
	ret += TexFetchBoneMatrix( BlendIndices.x, SkinningData ) * (BlendWeights.x / weightSum);
	ret += TexFetchBoneMatrix( BlendIndices.y, SkinningData ) * (BlendWeights.y / weightSum);
	ret += TexFetchBoneMatrix( BlendIndices.z, SkinningData ) * (BlendWeights.z / weightSum);
	ret += TexFetchBoneMatrix( BlendIndices.w, SkinningData ) * (BlendWeights.w / weightSum);
	// Return composed matrix
	return ret;
}

float4 ApplySkinning( float4 localPosition, float4 BlendWeights, uint4 BlendIndices, float4 SkinningData, out float4x4 SkinningMatrix, out float wet )
{
	float4 skinnedPosition = (float4)0;

	// skinnedPosition uses each matrix separately instead of the combined matrix below. This gives more accurate results.
	float4 accumulatedExtraData = (float4) 0.0f;
	float weightSum = BlendWeights.x + BlendWeights.y + BlendWeights.z + BlendWeights.w;
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.x, SkinningData, accumulatedExtraData.x )) * (BlendWeights.x / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.y, SkinningData, accumulatedExtraData.y )) * (BlendWeights.y / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.z, SkinningData, accumulatedExtraData.z )) * (BlendWeights.z / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.w, SkinningData, accumulatedExtraData.w )) * (BlendWeights.w / weightSum);
	
	// Generate the skinning matrix
	SkinningMatrix = CompileSkinningMatrix( BlendWeights, BlendIndices, SkinningData );

	// Wetness is actually 1-extraData
	float4 reversedExtraData = 1.0f - accumulatedExtraData;
	reversedExtraData *= (BlendWeights/weightSum);
	wet = saturate( 5.0f*(reversedExtraData.x + reversedExtraData.y + reversedExtraData.z + reversedExtraData.w) );

	return skinnedPosition;
}


// Similar to ApplySkinning, except the raw extraData (averaged, weighted by bone weights) is returned instead of "wetness".
float4 ApplySkinningWithExtraData( float4 localPosition, float4 BlendWeights, uint4 BlendIndices, float4 SkinningData, out float4x4 SkinningMatrix, out float extraData )
{
	float4 skinnedPosition = (float4)0;

	// skinnedPosition uses each matrix separately instead of the combined matrix below. This gives more accurate results.
	float4 accumulatedExtraData = (float4) 0.0f;
	float weightSum = BlendWeights.x + BlendWeights.y + BlendWeights.z + BlendWeights.w;
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.x, SkinningData, accumulatedExtraData.x )) * (BlendWeights.x / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.y, SkinningData, accumulatedExtraData.y )) * (BlendWeights.y / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.z, SkinningData, accumulatedExtraData.z )) * (BlendWeights.z / weightSum);
	skinnedPosition += mul( localPosition, TexFetchBoneMatrixWithExtraData( BlendIndices.w, SkinningData, accumulatedExtraData.w )) * (BlendWeights.w / weightSum);
	
	// Generate the skinning matrix
	SkinningMatrix = CompileSkinningMatrix( BlendWeights, BlendIndices, SkinningData );

	extraData = dot( accumulatedExtraData, BlendWeights / weightSum );

	return skinnedPosition;
}


//////////////////////////////////////////////////////////////////////////
// Vertex Collapse

Texture2D		t_VertexCollapseTexture		: register( t1 );

float3 ApplyVertexCollapseSkinned( float3 skinnedPosition, float coeff, uint4 blendIndices, float4 blendWeights, float4 skinningData )
{
	// Bones are sorted by decreasing weight, so first one is most influential.
	uint targetBone = blendIndices.x;
	float3 bonePos = t_VertexCollapseTexture.Load( int3( targetBone, 0, 0 ) ).xyz;
	bonePos = mul( float4(bonePos,1), TexFetchBoneMatrix( targetBone, skinningData ) ).xyz;
	return lerp( bonePos, skinnedPosition, saturate(coeff) );
}


float3 ApplyVertexCollapse( float3 position, float coeff )
{
	return lerp( float3( 0.0f, 0.0f, position.z ), position, saturate(coeff) );
}


// Each vertex factory implements function:
// VS_FAT_VERTEX GenerateVertex( VS_INPUT input );
// that generates the output vertex
	
