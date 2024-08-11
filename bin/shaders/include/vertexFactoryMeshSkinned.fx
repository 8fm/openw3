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

/*
	// Compile the skinning matrix for the vertex
	ret.LocalToWorld = mul( CompileSkinningMatrix( input.BlendWeights, input.BlendIndices ), ret.LocalToWorld );
	
	// Calculate final vertex position
	ret = CalcPosition( ret, ret.VertexPosition );
/*/

	float4 localPosition = float4( ret.VertexPosition, 1 );
	float4x4 SkinningMatrix = (float4x4) 0;
    
	// We are smugling wetness parameter throu skinning matrix
	// so we can have some shader response when characters 
	// have their clothes ...wet
	float wet = 0.0f;
#ifdef MS_INSTANCING
	float4 skinnedPosition = ApplySkinning( localPosition, input.BlendWeights, input.BlendIndices, input.SkinningData, SkinningMatrix, wet );
#	if defined(MS_HAS_VERTEX_COLLAPSE) && defined(MS_HAS_EXTRA_STREAMS)
		// Apply vertex collapse
		skinnedPosition.xyz = ApplyVertexCollapseSkinned( skinnedPosition.xyz, input.Color.w, input.BlendIndices, input.BlendWeights, input.SkinningData );
#	endif
#else
	float4 skinnedPosition = ApplySkinning( localPosition, input.BlendWeights, input.BlendIndices, VSC_SkinningData, SkinningMatrix, wet );
#	if defined(MS_HAS_VERTEX_COLLAPSE) && defined(MS_HAS_EXTRA_STREAMS)
		// Apply vertex collapse
		skinnedPosition.xyz = ApplyVertexCollapseSkinned( skinnedPosition.xyz, input.Color.w, input.BlendIndices, input.BlendWeights, VSC_SkinningData );
#	endif
#endif

    ret.SkeletalExtraData.y = 0.0f;
	ret.SkeletalExtraData.x = saturate(input.Normal.z);
    ret.SkeletalExtraData.z = wet;
    
#ifdef MS_DISCARD_PASS
	// TODO : This could be branched out if we had the dissolve flags shared between VS and PS.
	ret.ClippingEllipsePos.xyz = mul( localPosition, ClippingEllipseMatrix ).xyz;
#endif

	// Calculate position in world space
	ret.WorldPosition = mul( skinnedPosition, ret.LocalToWorld );

	// Calculate position in view space
	ret.ViewPosition = mul( skinnedPosition, mul( ret.LocalToWorld, VSC_WorldToView) );
	
	// Calculate position in screen space
#ifdef __PSSL__ 
	// MG: New orbis-wave-psslc compiler comes with fastmath enabled by default. The only case where I found it causing problems was cases where we render some skinned geometry twice, overlaying an additional effect.
	// This was the case for effects such as frost, covering the wild hunt warriors. Similarly for hi res entity shadows.
	// "__invariant" tells the compiler to disable fastmath for all computations leading to the specified value.
	ret.ScreenPosition = __invariant( mul( skinnedPosition, mul( ret.LocalToWorld, VSC_WorldToScreen) ) );
#else
	ret.ScreenPosition = mul( skinnedPosition, mul( ret.LocalToWorld, VSC_WorldToScreen) );
#endif
	
	// Calculate position in light space
	ret.LightPosition = float4 ( 0, 0, 0, 1 ); // ace_todo: remove LightPosition since it's unused anyway (or at least not properly initiated)
	
	ret.LocalToWorld = mul( SkinningMatrix, ret.LocalToWorld );
	
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
