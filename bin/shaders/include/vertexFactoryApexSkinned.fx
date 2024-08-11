/// Header for vertex factory from skinned mesh
/// It's esential to recompile all shaders after changing any of those lines

/// The input stream for the static mesh
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float3		Normal			: NORMAL;
	float4		Tangent			: TANGENT;
	float2		UV				: TEXCOORD0;
	uint4		BlendIndices	: BLENDINDICES0;
#ifdef MS_HAS_EXTRA_STREAMS
	float4		BlendWeights	: BLENDWEIGHT0;
#endif
};


#ifdef MS_DISCARD_PASS
#define ClippingEllipseMatrix	VSC_Custom_Matrix
#endif

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	VS_FAT_VERTEX ret = DefaultVertex();
	
	// Decompress position
	float4 localPosition = float4( input.Position.xyz, 1.f );
	float4 skinnedPosition = localPosition;
	float chunkAlpha = 1.0f;

	// Compile the skinning matrix for the vertex

	float4x4 SkinningMatrix = (float4x4) 0;
#ifdef MS_HAS_EXTRA_STREAMS
	skinnedPosition = ApplySkinningWithExtraData( localPosition, input.BlendWeights, input.BlendIndices, VSC_SkinningData, SkinningMatrix, chunkAlpha );
#else
	SkinningMatrix = TexFetchBoneMatrixWithExtraData( input.BlendIndices.x, VSC_SkinningData, chunkAlpha );
	skinnedPosition = mul( localPosition, SkinningMatrix);
#endif

	// Modify dissolve parameters so that we take into account individual chunk alphas.
	// Doesn't work in a general case, but it's fine for apex, where we don't need to cross-fade between LODs.
	ret.DissolveParams.y = -1 + chunkAlpha * ( ret.DissolveParams.y + 1 );


#ifdef MS_DISCARD_PASS
	// TODO : This could be branched out if we had the dissolve flags shared between VS and PS.
	ret.ClippingEllipsePos.xyz = mul( localPosition, ClippingEllipseMatrix ).xyz;
#endif

	ret.VertexPosition = skinnedPosition.xyz;
	
	// Calculate final vertex position
	ret = CalcPosition( ret, ret.VertexPosition );
	
	ret.LocalToWorld = mul( SkinningMatrix, ret.LocalToWorld );
	
	// When doing solid shadow pass do not include more detailed data
#if !defined(RS_PASS_SHADOW_DEPTH_SOLID)
	// Decompress vertex normal
	ret.VertexNormal = input.Normal;
	
	// Calculate tangent space vectors ( in world space )
	// we have no control on setting Tangents/Binormals/Normals in apex destruction module
	// so cross had to be swapped. Then you can correct TBN's via DCC (Max/Maya) because there we have access to 
	// flipping TBN's data. So reexport will fix possible not good looking meshes
	float3 Binormal = cross( input.Tangent.xyz, input.Normal ) * input.Tangent.w;
	
	ret = CalcTangentSpace( ret, input.Normal, Binormal, input.Tangent.xyz );	
	// Texture coordinates
	ret.UV = input.UV;
	ret.UV.y = 1 - ret.UV.y;
#endif

	// Done	
	return ret;
}
