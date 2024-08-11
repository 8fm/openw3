
#if defined( VERTEX_SHADER )

Texture2D<float4> 	TerrainPigmentTex 	: register(t3);
SamplerState 		TerrainPigmentSmp	: register(s3);

TEXTURE2D_ARRAY<float2>	TerrainNormalsTex		: register( t4 );
SamplerState			TerrainNormalsSmp		: register( s4 );

START_CB( PigmentConsts, 11 )
	float4		pigmentWorldAreaScaleBias;	
	float4x4	m_transformMatrices[ 16 ];
	uint		m_activeCollisionsNum;
	float4		terrainNormalsAreaParams;
	float4		terrainNormalsParams;
	float4		alphaScalarMulParams;
END_CB

float3 ComputeCollisionOffset( float3 worldPosition, float3 worldNormal, float3 localPosition, float bendingEnabled )
{		
	float3 resultingOffset = float3(0.0f, 0.0f, 0.0f);
	float3 resultingNormal = worldNormal.xyz;
	resultingNormal.z = 0.0f;
	resultingNormal.xyz = normalize( resultingNormal.xyz );

	float localZdamp = pow( clamp( localPosition.z, 0.0f, 1.0f ), 1.5f );

	const float3 maximumBendDistance = float3( 1.25f, 1.25f, 1.25f );
	
	float hideFactor = 0.0f;

	for( uint i=0; i<m_activeCollisionsNum; i++ )
	{			
		float4 collisionPoint = m_transformMatrices[i][3].xyzw;	
		
		float3 R1 = m_transformMatrices[i][0].xyz;
		float3 R2 = m_transformMatrices[i][1].xyz;
		float3 R3 = m_transformMatrices[i][2].xyz;

		float3 X = worldPosition.xyz - collisionPoint.xyz;
		float3 D = float3(0.0f, 0.0f, 0.0f);

		D.x = dot( X, R1 ) / dot( R1, R1 );
		D.y = dot( X, R2 ) / dot( R2, R2 );
		D.z = dot( X, R3 ) / dot( R3, R3 );
		D.xyz = normalize(D.xyz);

		float3 rad = R1*D.x + R2*D.y + R3*D.z;

		float d = dot( X, rad )/dot( rad, rad );		

		// inside
		if( d < 1.0f )
		{
			// max
			if( length(rad.xyz - X.xyz) > length( resultingOffset.xyz ) )
			{
				resultingOffset.xyz = (rad.xyz - X.xyz);
				// Check if intensity is smaller than 0.0f - then use hide factor
				hideFactor = -(localPosition.z + 1.0f) * (1.0f - pow( d, 0.5f )) * step( collisionPoint.w, 0.0f );
			}			
		}
		// outside		
	}

	// hacking offset z always down
	resultingOffset.z = -1.0f*abs(resultingOffset.z);

	resultingOffset.xyz = localZdamp * bendingEnabled * length( resultingOffset.xyz ) * ( 0.2f*resultingNormal.xyz + 0.8f*resultingOffset.xyz );

	resultingOffset.z += hideFactor;

	return clamp( resultingOffset.xyz, -1.0f*maximumBendDistance, maximumBendDistance );
}

float4 SampleTerrainPigment( float3 worldSpacePosition )
{
	float2 coord = worldSpacePosition.xy * pigmentWorldAreaScaleBias.xy + pigmentWorldAreaScaleBias.zw;

	float4 value = 0;
	
	[branch]
	if ( all( abs( coord - 0.5 ) < 0.5 ) )
	{
		// ace_pigment_fix: add smooth border (alpha value)

		value = float4 ( SAMPLE_LEVEL( TerrainPigmentTex, TerrainPigmentSmp, coord, 0 ).xyz, 1 );
	}

	return value;
}

float3 SampleTerrainNormal( float3 worldSpacePosition, float2 randomValue01 )
{
	float2 coord = worldSpacePosition.xy * terrainNormalsAreaParams.x + terrainNormalsAreaParams.yz;

	float3 resultNormal = float3( 0, 0, 1 );
	
	[branch]
	if ( all( abs( coord - 0.5 ) < 0.5 ) )
	{
		// ace_pigment_fix: add smooth border (alpha value)

		float2 smpData = SAMPLE_LEVEL( TerrainNormalsTex, TerrainNormalsSmp, float3( coord, terrainNormalsAreaParams.w ), 0 ).xy;
		smpData += (randomValue01 - 0.5) * terrainNormalsParams.x;
		resultNormal = float3 ( smpData, sqrt( 1 - saturate( dot( smpData.xy, smpData.xy ) ) ) );
	}

	return resultNormal;
}

float SpeedTreeGrassAlphaScalarMultiplier( float distance )
{
	return 1 + alphaScalarMulParams.x * saturate( distance * alphaScalarMulParams.y + alphaScalarMulParams.z );
}

#endif


#if defined( PIXEL_SHADER )

float3 ApplyTerrainPigment( float4 pigment, float3 albedo, float dist, float distInfluence, float localVerticalPos, float vertStartPos, float vertGradient, float amountScale )
{
	float3 result = albedo;
	
	if ( amountScale > 0 )
	{
		float pigmentAmount = 0;
		{
			// add pigment for distant positions
			pigmentAmount += saturate( distInfluence * (dist - terrainPigmentParams.y) / max( 0.0001, terrainPigmentParams.z ) );
			
			// add pigment based on localVerticalPosition
			pigmentAmount += saturate( 1 - (localVerticalPos - vertStartPos) / max( 0.0001, vertGradient ) );	
			
			//
			pigmentAmount *= pigment.a;
			
			//
			pigmentAmount = saturate( pigmentAmount * amountScale );
			
		}

		float3 pigmented = albedo;
		{
			pigmented = saturate( dot( albedo, float3 ( 0.3, 0.5, 0.2 ) ) ) * terrainPigmentParams.x * pigment.xyz;
		}
		
		result = lerp( albedo, pigmented, pigmentAmount );
	}
	
	return result;
}

#endif


