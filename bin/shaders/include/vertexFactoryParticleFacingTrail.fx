/// Header for camera facing trail particle vertex factory
/// It's esential to recompile all shaders after changing any of those lines

/// Particle camera facing trail
/// The input stream for the camera facing trail particle
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float4		Color			: COLOR0;
	float3		PrevPosition	: TEXCOORD0;
	float2		UV				: TEXCOORD1;
	float		Stretch			: TEXCOORD2;
	float		Frame			: TEXCOORD3;
};

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	VS_FAT_VERTEX ret = DefaultVertex();
	
	// Calculate vertex position
	float3 direction = normalize( input.Position - input.PrevPosition );
	float3 view = normalize( VSC_CameraPosition - input.Position );
	float3 posOffsetDir = normalize( cross( direction, view ) );
	
	float3 worldPosition = input.Position + input.Stretch * posOffsetDir;
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
