	float3 GetInstancePosition( float3 iPosition, VS_INPUT input )
	{
		float3 vertexPos = iPosition;
	#if defined( MS_INSTANCING )
		float scale = input.PosScale.w;
		vertexPos *= scale;
		float rotationAngle = input.ColorRot.w * 359.f;
		float rotationRad = DEG2RAD( rotationAngle );
		
		float2 rotSinCos;
		sincos( rotationRad, rotSinCos.x, rotSinCos.y );
		vertexPos.xy = float2( rotSinCos.y * vertexPos.x - rotSinCos.x * vertexPos.y, rotSinCos.y * vertexPos.y + rotSinCos.x * vertexPos.x );
		
		vertexPos += input.PosScale.xyz;
	#endif
		return vertexPos;
	}
	
	float3 GetInstanceTranslation( VS_INPUT input )
	{
		float3 translation;
	#if defined( MS_INSTANCING )
		translation = input.PosScale.xyz;
	#else
		translation = float3(VSC_LocalToWorld[3][0], VSC_LocalToWorld[3][1], VSC_LocalToWorld[3][2]);
	#endif
		return translation;
	}

	float3 GetInstanceColor( VS_INPUT input )
	{
	#if defined( MS_INSTANCING )
		return float4( input.ColorRot.xyz * 2, 1 );
	#else
		return VSC_FoliageColor.xyz;
	#endif
	}

	float3 GetInstanceNormal( float3 iNormal, VS_INPUT input )
	{
		float3 normal;
	#if defined( MS_INSTANCING )
		// instance rotation should be taken into account
		normal = normalize( mul( iNormal, transpose( (float3x3)VSC_LocalToWorld ) ) );
	#else
		normal = normalize( mul( iNormal, transpose( (float3x3)VSC_LocalToWorld ) ) );
	#endif
		return normal;
	}
	
	float3 CalcInstancePosition( float3 iPosition, float3 iTranslation, float iScale, float iRotation )
	{
		float3 vertexPos = iPosition;
	#ifdef MS_INSTANCING
		float scale = iScale;
		vertexPos *= scale;
		float rotationAngle = iRotation * 359.f;
		float rotationRad = DEG2RAD( rotationAngle );
		
		float2 rotSinCos;
		sincos( rotationRad, rotSinCos.x, rotSinCos.y );
		vertexPos.xy = float2( rotSinCos.y * vertexPos.x - rotSinCos.x * vertexPos.y, rotSinCos.y * vertexPos.y + rotSinCos.x * vertexPos.x );
		
		vertexPos += iTranslation;
	#endif
		return vertexPos;
	}