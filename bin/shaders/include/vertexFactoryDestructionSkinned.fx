/// Header for vertex factory from skinned mesh
/// It's esential to recompile all shaders after changing any of those lines


#ifdef MS_DISCARD_PASS
#define ClippingEllipseMatrix	VSC_Custom_Matrix
#endif

/// The input stream for the static mesh
struct VS_INPUT
{
	float3		Position		: POSITION0;
	uint4		BlendIndices	: BLENDINDICES0;
	float4		BlendWeights	: BLENDWEIGHT0;
	float2		UV				: TEXCOORD0;
	float3		Normal			: NORMAL;
	float4		Tangent			: TANGENT;

#ifdef MS_HAS_EXTRA_STREAMS
	float4		Color			: COLOR0;
	float2		UV2				: TEXCOORD1;
#endif

#ifdef MS_UVDISSOLVE_SEPARATE
	float2		DissolveUV		: TEXCOORD2;
#endif

#ifdef MS_INSTANCING
	float4x3	LocalToWorld		: INSTANCE_TRANSFORM;
	float4		DetailLevelParams	: INSTANCE_LOD_PARAMS;
	float4		SkinningData		: INSTANCE_SKINNING_DATA;
#endif
};

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	VS_FAT_VERTEX ret = DefaultVertex();

#ifdef MS_INSTANCING
	ret.LocalToWorld._m00_m10_m20_m30 = float4( input.LocalToWorld._m00_m10_m20_m30 );
	ret.LocalToWorld._m01_m11_m21_m31 = float4( input.LocalToWorld._m01_m11_m21_m31 );
	ret.LocalToWorld._m02_m12_m22_m32 = float4( input.LocalToWorld._m02_m12_m22_m32 );
	ret.LocalToWorld._m03_m13_m23_m33 = float4( 0.0f, 0.0f, 0.0f, 1.0f );
	ret.DissolveParams = input.DetailLevelParams;
#endif
	
	// Decompress position
	float3 vertexPos = input.Position * VSC_QS.xyz + VSC_QB.xyz;

	// Copy stream parameters
	ret.VertexPosition = vertexPos;
	
	// Decompress position
	float4 localPosition = float4( vertexPos.xyz, 1.f );
	float chunkAlpha = 1.0f;

	// Compile the skinning matrix for the vertex

	float4x4 SkinningMatrix = (float4x4) 0;
#ifdef MS_INSTANCING
	SkinningMatrix = TexFetchBoneMatrixWithExtraData( input.BlendIndices.x, input.SkinningData, chunkAlpha );
#else
	SkinningMatrix = TexFetchBoneMatrixWithExtraData( input.BlendIndices.x, VSC_SkinningData, chunkAlpha );
#endif

	// Modify dissolve parameters so that we take into account individual chunk alphas.
	ret.DissolveParams.y = -1 + chunkAlpha * ( ret.DissolveParams.y + 1 );

#ifdef MS_DISCARD_PASS
	// TODO : This could be branched out if we had the dissolve flags shared between VS and PS.
	ret.ClippingEllipsePos.xyz = mul( localPosition, ClippingEllipseMatrix ).xyz;
#endif

	ret.VertexPosition = localPosition.xyz;
	
	// Calculate final vertex position
	ret = CalcPosition( ret, ret.VertexPosition );
	
	ret.LocalToWorld = mul( SkinningMatrix, ret.LocalToWorld );
	
	// Calculate position in view space
	ret.ViewPosition = mul( localPosition, mul( ret.LocalToWorld, VSC_WorldToView) );
	
	// Calculate position in screen space
	ret.ScreenPosition = mul( localPosition, mul( ret.LocalToWorld, VSC_WorldToScreen) );
	
	// Calculate position in light space
	ret.LightPosition = float4 ( 0, 0, 0, 1 ); // ace_todo: remove LightPosition since it's unused anyway (or at least not properly initiated)
	

//*/
	// When doing solid shadow pass do not include more detailed data
#if !defined(RS_PASS_SHADOW_DEPTH_SOLID)

	float3 normal = DecompressNormal( input.Normal );
	float3 tangent = DecompressNormal( input.Tangent );
	float binormSign = input.Tangent.w * 2 - 1;
	float3 binormal = normalize( cross( normal, tangent ) * binormSign );

	ret.VertexNormal = normal;
	
	// Calculate tangent space vectors ( in world space )
	ret = CalcTangentSpace( ret, normal, binormal, tangent );
	
	// Texture coordinates
	ret.UV = input.UV;

	// Extra streams
#ifdef MS_HAS_EXTRA_STREAMS
	ret.VertexColor = input.Color;
	ret.UV2 = input.UV2;
#endif

#endif

#ifdef MS_DISCARD_PASS
	// TODO : This could be branched out if we had the dissolve flags shared between VS and PS.
#ifdef MS_UVDISSOLVE_SEPARATE
	ret.DissolveUV = input.DissolveUV;
#else
	ret.DissolveUV = input.UV;
#endif
#endif
	
	// Done	
	return ret;
}
