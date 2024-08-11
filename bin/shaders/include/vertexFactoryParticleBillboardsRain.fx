/// Header for vertex factory from billboarded particles
/// It's esential to recompile all shaders after changing any of those lines

/// The input stream for billboarded particle
struct VS_INPUT
{
	float3		Position		: POSITION0;
	float4		Color			: COLOR0;
	float3		Param0			: TEXCOORD0;	// Rotation.xy, Frame
	float2		Size			: TEXCOORD1;
	float2		Param1			: TEXCOORD2;	// UV
	float3		Param2			: TEXCOORD3;	// motion direction
	float		Stretch			: TEXCOORD4;	// stretch per velocity
	float		MotionBlend		: TEXCOORD5;
};

//SamplerState	 sRainMap 				: register( s3 ): register( t3 );

//#define rainToWorld			VSC_Custom_0
//#define worldToRain			VSC_Custom_4

// Generate vertex
VS_FAT_VERTEX GenerateVertex( VS_INPUT input )
{
	// Calculate vertex position - it will get clipped :) 
	/*float3 worldPosition = float3(-5000,-5000,-5000);
	
	const float3 worldNormal = -VSC_CameraVectorForward.xyz;
	// Calculate the bilboard vectors
	const float2 rotCosSin = input.Param0.xy;
	
	float rotateBilCoordX = 0.0f;
	float rotateBilCoordY = 0.0f;

	// Calculate rotated bilboard vectors
	float3 worldU = float3(0,0,0);
	float3 worldV = float3(0,0,0);*/
	
	VS_FAT_VERTEX ret = DefaultVertex();

	// Don't know how those matrices work. It's dead anyway.
	/*{
		float4 rainMapPos = mul( float4(input.Position.xyz, 1.0f), worldToRain );
		rainMapPos /= rainMapPos.w;
		if ( (rainMapPos.x > -1.0f) && (rainMapPos.y > -1.0f) && (rainMapPos.x < 1.0f) && (rainMapPos.y < 1.0f) && (rainMapPos.z > 0.0f) && (rainMapPos.z < 1.0f) )
		{
			// prefetch texture shit as early as possible to avoid waiting for result latency - about 20 cycles in vertex shader
			float4 rainMapSample = tex2Dlod( sRainMap, float4( float2(0.5f,0.5f) + float2(0.5f,-0.5f) * rainMapPos.xy, 0.0f, 0.0f) );
			
			if ( length(input.Param2 - float3(0.0f,0.0f,0.0f)) < 0.05f )
			{
				rotateBilCoordX = rotCosSin.y * input.Size.x + rotCosSin.x * input.Size.y;
				rotateBilCoordY = rotCosSin.x * input.Size.x - rotCosSin.y * input.Size.y;
		
				// Particles do not have the world matrix
				ret.LocalToWorld = float4x4( 1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1 );

				// Calculate rotated bilboard vectors
				worldU = VSC_CameraVectorRight * rotateBilCoordX;
				worldV = VSC_CameraVectorUp * rotateBilCoordY;		
		
				float4 position = mul ( float4( rainMapPos.xy, rainMapSample.x, 1), rainToWorld );
				worldPosition = position.xyz;
			} 
			else
			{
				rotateBilCoordX = rotCosSin.y + rotCosSin.x;
				rotateBilCoordY = rotCosSin.x - rotCosSin.y;
	
				float3 vertToCam = normalize(VSC_CameraPosition - input.Position.xyz);
				float3 motionDirection = normalize(input.Param2);
				float3 particleRightVector = normalize( cross( vertToCam, motionDirection ) );

				// Calculate rotated bilboard vectors
				worldU = motionDirection * ( 1.0f + input.MotionBlend * input.Stretch) * input.Size.x * rotateBilCoordX;
				worldV = particleRightVector * input.Size.y * rotateBilCoordY;
	
				ret.MotionBlend = input.MotionBlend;
				
				if ( (rainMapSample.x * 0.95f) > rainMapPos.z )
				{
					worldPosition = input.Position.xyz;
				}
				else
				{
					// return shit - clip vertex
		
					// Copy stream parameters
					ret.VertexPosition = worldPosition;
			
					// Calculate terrain world position
					ret = SetPosition( ret, worldPosition );
					return ret;
				}
			}
		}
		else
		{
			// return shit - clip vertex
		
			// Copy stream parameters
			ret.VertexPosition = worldPosition;
			
			// Calculate terrain world position
			ret = SetPosition( ret, worldPosition );
			return ret;
		}
	}
	
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
	ret.UV = input.Param1.yx;

	// Animation frame
	ret.AnimationFrame = input.Param0.z;*/
	
	// Done
	return ret;
}	
