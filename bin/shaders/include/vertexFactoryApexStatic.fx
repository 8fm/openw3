/// Header for vertex factory from static mesh
/// It's esential to recompile all shaders after changing any of those lines

/// The input stream for the static mesh
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float3		Normal			: NORMAL;
	float4		Tangent			: TANGENT;
	float2		UV				: TEXCOORD0;
};

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{	
	VS_FAT_VERTEX ret = DefaultVertex();

	// Decompress position
	ret.VertexPosition = input.Position;

	// Foliage color
	ret.FoliageColor = float3(1, 1, 1);

	// Calculate final vertex position
	ret = CalcPosition( ret, ret.VertexPosition );
	
	ret.SkeletalExtraData.z = VSC_Custom_1.x;

	// When doing solid shadow pass do not include more detailed data
#if !defined(RS_PASS_SHADOW_DEPTH_SOLID)
	// Decompress normal
	ret.VertexNormal = input.Normal;
	
	// Calculate tangent space vectors ( in world space )
	// we have no control on setting Tangents/Binormals/Normals in apex destruction module
	// so cross had to be swapped. Then you can correct >>clothing<< TBN's via DCC (Max/Maya) because there we have access to 
	// flipping TBN's data. So reexport will fix possible not good looking meshes
	float3 Binormal = cross( input.Tangent.xyz, input.Normal ) * input.Tangent.w;
	
	ret = CalcTangentSpace( ret, input.Normal, Binormal, input.Tangent.xyz );
	
	// Texture coordinates
	ret.UV = input.UV;
	ret.UV.y = 1 - input.UV.y;
#endif

	// Done
	return ret;
}
