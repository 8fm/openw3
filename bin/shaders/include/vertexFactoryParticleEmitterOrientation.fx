/// Header for emitter aligned particle vertices
/// It's esential to recompile all shaders after changing any of those lines

/// The particle input stream
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float4		Color			: COLOR0;
	float3		Param0			: TEXCOORD0;	// Rotation.xy, Frame
	float2		Size			: TEXCOORD1;
	float2		Param1			: TEXCOORD2;	// UV
};

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	VS_FAT_VERTEX ret = DefaultVertex();
	
	// Particles do not have the world matrix
	//ret.LocalToWorld = float4x4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 );
	
	// Calculate vertex position
	float3 worldPosition = input.Position;
	worldPosition = mul( float4( worldPosition, 1 ), VSC_LocalToWorld );

	float3 worldNormal = -VSC_CameraVectorForward.xyz;
	
	// Calculate the bilboard vectors
	const float2 rotCosSin = input.Param0.xy;
	//const float2 bilCoords = 2 * ( input.Param1 - 0.5f );
	float rotateBilCoordX = rotCosSin.y * input.Size.x + rotCosSin.x * input.Size.y;
	float rotateBilCoordY = rotCosSin.x * input.Size.x - rotCosSin.y * input.Size.y;

	// Calculate rotated bilboard vectors
	const float3 worldU = normalize(ret.LocalToWorld[0].xyz) * rotateBilCoordX;
	const float3 worldV = normalize(ret.LocalToWorld[1].xyz) * rotateBilCoordY;
	
	// Assemble vertex position
	worldPosition += worldU;
	worldPosition += worldV;
	
	// Copy stream parameters
	ret.VertexPosition = worldPosition;
	ret.VertexNormal = worldNormal;
	ret.VertexColor = input.Color;
	
	// Calculate terrain world position
	ret = SetPosition( ret, worldPosition );
	
	// Set tangent space
	ret.WorldNormal = worldNormal;
	ret.WorldTangent = normalize( -worldU );	
	ret.WorldBinormal = normalize( worldV );
	
	// Texture coordinates
	ret.UV = input.Param1;

	// Animation frame
	ret.AnimationFrame = input.Param0.z;
	
	// Done
	return ret;
}	
