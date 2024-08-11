#ifndef LOCAL_REFLECTION_INCLUDED
#define LOCAL_REFLECTION_INCLUDED

#define ENABLE_REFLECTION_MANIPULATION_STEEPNESS			0
#define ENABLE_REFLECTION_MANIPULATION_SIDES_BEND			1



float3 CalcDirReflectedWorld( float3 worldPos, float3 worldNormal )
{
	float3 dir_reflected_world	= 0;
	{
	#if ENABLE_REFLECTION_MANIPULATION_STEEPNESS
		{
			// Here we're changing the steepness of the reflected dir, so that more area would be covered by reflection.
			// This transformation needs to be carefully selected, so that we would be able to reproject
			// the reflection for the need of accumulation buffer fetching.

			dir_reflected_world	= reflect( normalize(worldPos - cameraPosition.xyz), worldNormal );

			const float cosAng	= dot( dir_reflected_world, worldNormal );
			const float ang		= acos( cosAng );
			const float ang2	= HALF_PI - (HALF_PI - ang) * 0.5;
		
			float _sinAng2, _cosAng2;
			sincos( ang2, _sinAng2, _cosAng2 );

			dir_reflected_world.xy	= normalize( dir_reflected_world.xy ) * _sinAng2;
			dir_reflected_world.z	= _cosAng2 * sign( dir_reflected_world.z );
		}
	#else
		{
			dir_reflected_world	= reflect( normalize(worldPos - cameraPosition.xyz), worldNormal );
		}
	#endif
	}

	return dir_reflected_world;
}

float4 SampleRLRSkyEnvProbe( Texture2D texRLRSky, SamplerState smpRLRSky, float4 rlrSkyParams, float3 dir )
{
	dir.z = abs( dir.z );
	float2 uv = clamp( CubeToParaboloid( dir, GetCubeDirParaboloidIndexUp() ) * rlrSkyParams.xy, 0.5, rlrSkyParams.xy - 0.5f ) / rlrSkyParams.zw;
	float4 value = SAMPLE_LEVEL( texRLRSky, smpRLRSky, uv, 0 );
	return float4( value.xyz, 1 );
}

float4 SampleRLREnvProbe( bool isGlobalReflection, float3 posWorld, float3 dirWorld, Texture2D texRLRSky, SamplerState smpRLRSky, float4 rlrSkyParams )
{
	const uint mip_index = 0;

	float3 reflectionValue = 0;
	[branch]
	if ( isGlobalReflection )
	{
		reflectionValue = SampleRLRSkyEnvProbe( texRLRSky, smpRLRSky, rlrSkyParams, dirWorld ).xyz;
	}
	else
	{
		reflectionValue = CalcEnvProbeReflection_MipLod( posWorld, dirWorld, mip_index, 0 );
	}

	return float4 ( reflectionValue, 1 );
}

#endif
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
