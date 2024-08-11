/// Header for screen particles vertex factory
/// It's esential to recompile all shaders after changing any of those lines

/// The input stream for the particle
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
	ret.LocalToWorld = float4x4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 );
	
	// Calculate vertex position
	float3 screenPosition = input.Position;
	float3 worldNormal = VSC_CameraVectorForward.xyz;
	
	// Calculate the bilboard vectors
	const float2 rotCosSin = input.Param0.xy;
	//const float2 bilCoords = 2 * ( input.Param1 - 0.5f );
	
	float2 screenSize=VSC_ViewportParams.xy;
	float2 particleSizeN = input.Size.xy;
	
	float rotateBilCoordX = rotCosSin.y * particleSizeN.x + rotCosSin.x * particleSizeN.y;
	float rotateBilCoordY = rotCosSin.x * particleSizeN.x - rotCosSin.y * particleSizeN.y;

	// Calculate rotated bilboard vectors
	const float2 screenU = float2(1.0,0.0) * rotateBilCoordX;
	const float2 screenV = float2(0.0,1.0) * rotateBilCoordY;
	
	// Assemble vertex position
	screenPosition.xy += screenU;
	screenPosition.xy += screenV;
	screenPosition.z = VSGetProjectionCameraNearestDepth;
	
	// Copy stream parameters
	ret.VertexPosition = screenPosition;
	ret.VertexNormal = worldNormal;
	ret.VertexColor = input.Color;
	
	// Calculate position in screen space
	ret.ScreenPosition = float4(screenPosition.x, screenPosition.y, screenPosition.z, 1.0);	
	
	// Set tangent space
	ret.WorldTangent = normalize( VSC_CameraVectorRight );	
	ret.WorldBinormal = normalize( VSC_CameraVectorUp );
	
	// Texture coordinates
	ret.UV = input.Param1;

	// Animation frame
	ret.AnimationFrame = input.Param0.z;
	
	// Done
	return ret;
}	
