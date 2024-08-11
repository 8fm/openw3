///////////////////////////////////////////////////////////////////////
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with
//  the terms of that agreement.
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_FOG_AND_SKY
#define ST_INCLUDE_FOG_AND_SKY


///////////////////////////////////////////////////////////////////////  
//  FogComputeLerp
//
//	Given a distance from the camera, FogComputeLerp will return a [0,1] value
//	intended for use with a lerp in the pixel shader, as in:
//
//	  float4 vFinalColor = lerp(fog_color, unfogged_color, return_value);
//
//	1.0 is no fog, 0.0 is full fog

float FogComputeLerp(float fDistanceFromCamera)
{
	float fScalar = 1.0;

 // LAVA++ Commenting this shit as it doesn't compile and we don't need it anyway
/*
	if (ST_FOG_CURVE == ST_FOG_CURVE_EXP)
	{
		fScalar = 1.0 / exp(m_fFogDensity * fDistanceFromCamera);
	}
	else if (ST_FOG_CURVE == ST_FOG_CURVE_EXP2)
	{
		const float c_fExponent = m_fFogDensity * fDistanceFromCamera;
		fScalar = 1.0 / exp(c_fExponent * c_fExponent);
	}
	else if (ST_FOG_CURVE == ST_FOG_CURVE_LINEAR)
	{
		fScalar = saturate((m_fFogEndDist - fDistanceFromCamera) / m_fFogSpan);
	}
	else if (ST_FOG_CURVE == ST_FOG_CURVE_USER)
	{
		// to be determined by SpeedTree licensees if desired; stubbed out with simple linear equation
		fScalar = saturate((m_fFogEndDist - fDistanceFromCamera) / m_fFogSpan);
	}
*/
	return fScalar;
}


///////////////////////////////////////////////////////////////////////  
//  FogComputeSkyColor
//
//	This is how SpeedTree's notional sky system is computed, with this
//	function called from a pixel shader. It computes a gradient sky with
//	the option of adding a simple sun disk.
//
//	vDirection is normally computed in the pixel shader as the incoming
//	world space position from the vertex shader, normalized.

float3 FogComputeSkyColor(float3 vDirection, bool bAddSun)
{
/* LAVA++ Don't need this shit and it won't compile since I removed some constant buffers

	float fInterp = ST_COORDSYS_Z_UP ? vDirection.z : vDirection.y;
	
	// sigmoid (s-curve) function to ensure smooth sky blending
	fInterp = 12.0 * ((fInterp + 0.5) * -0.6667 + 0.5);
	fInterp = 1.0 / (1.0 + exp(fInterp));
	
	float3 vSkyWithFog = lerp(m_vFogColor, m_vSkyColor, fInterp);
	
	if (bAddSun)
	{
		float fSunLight = saturate(dot(vDirection, m_vDirLight.m_vDir.xyz) + m_fSunSize);
			
		fSunLight = pow(fSunLight, m_fSunSpreadExponent);
		vSkyWithFog = lerp(vSkyWithFog, m_vSunColor, fSunLight);
	}

	return vSkyWithFog;

LAVA--*/
	
	return (float3)0;
}


///////////////////////////////////////////////////////////////////////  
//  FogComputeVertexColor
//
//	To ensure a smooth fog effect, instead of fogging to a single color, the
//	SpeedTree reference application fogs to a computed sky color. This
//	acts as a convenience wrapper to FogComputeSkyColor(), taking a world
//	space position value instead of a direction.

float3 FogComputeVertexColor(float3 vWorldPosition)
{
	float3 vSkyDirection = normalize(vWorldPosition - m_vCameraPosition);

	return FogComputeSkyColor(vSkyDirection, false);
}

#endif // ST_INCLUDE_FOG_AND_SKY
