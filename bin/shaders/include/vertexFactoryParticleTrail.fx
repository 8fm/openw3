/// Header for trail particle vertex factory
/// It's esential to recompile all shaders after changing any of those lines

/// Particle trail
/// The input stream for the trail particle
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float4		Color			: COLOR0;
	float2		UV				: TEXCOORD0;
	float		Frame			: TEXCOORD1;
};

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	VS_FAT_VERTEX ret = DefaultVertex();
	
	// Calculate vertex position
	float3 worldPosition = input.Position;
	worldPosition = mul( float4( worldPosition, 1 ), VSC_LocalToWorld );

	float3 worldNormal = -VSC_CameraVectorForward.xyz;
	
	// Copy stream parameters
	ret.VertexPosition = worldPosition;
	ret.VertexNormal = worldNormal;
	ret.VertexColor = input.Color;
	
	// Calculate terrain world position
	ret = SetPosition( ret, worldPosition );
	
	// Set tangent space
	ret.WorldNormal = worldNormal;
	
	// Texture coordinates
	ret.UV = input.UV;
	
	// Animation frame
	ret.AnimationFrame = input.Frame;
	
	// Done
	return ret;
}	
