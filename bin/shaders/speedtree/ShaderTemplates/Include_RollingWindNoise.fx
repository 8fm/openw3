///////////////////////////////////////////////////////////////////////  
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) CONFIDENTIAL AND PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization, Inc. and
//  may not be copied, disclosed, or exploited except in accordance with 
//  the terms of that agreement.
//
//      Copyright (c) 2003-2013 IDV, Inc.
//      All rights reserved in all media.
//
//      IDV, Inc.
//      Web: http://www.idvinc.com

#ifndef ST_INCLUDE_ROLLING_WIND_NOISE
#define ST_INCLUDE_ROLLING_WIND_NOISE

#ifdef ST_VERTEX_SHADER
	#include "Include_SamplersAndTextureMacros.fx"
#endif
DeclareTexture(PerlinNoiseKernel, 9);


///////////////////////////////////////////////////////////////////////
//  WindNoiseTurbulence

float WindNoiseTurbulence(float2 vPos, float fStartingZoom)
{
	float fZoom = fStartingZoom;
	float fValue = 0.0;

	while (fZoom >= 1.0)
	{
		// lookup the noise pattern; .r contains primary, .gba contains near values to help smooth
		float4 vTexSample = SampleTextureSpecial(PerlinNoiseKernel, frac(vPos / fZoom));

		// option A) use a single texture sample for speed (disabled), or...
		{
			//fValue += vTexSample.r * fZoom;
		}

		// option B) use a composite of the rgba lookups for a smoother noise pattern
		{
			// todo: restore
			fValue += dot(vTexSample, float4(0.25, 0.25, 0.25, 0.25)) * fZoom;
		}

		fZoom /= 2.0;
	}

	return (0.5 * fValue / fStartingZoom);
}


///////////////////////////////////////////////////////////////////////
//  WindNoiseMarble

float WindNoiseMarble(float2 vPos, float fTwist, float fPeriod, float fTurbulence)
{
	// c_xPeriod and c_yPeriod together define the angle of the lines
	// c_xPeriod and c_yPeriod both 0 ==> it becomes a normal clouds or turbulence pattern

	const float c_fPeriodX = 5.0 * fPeriod;	// defines repetition of marble lines in x direction
	const float c_fPeriodY = 5.0 * fPeriod;	// defines repetition of marble lines in y direction

	float xyValue = vPos.x * c_fPeriodX + vPos.y * c_fPeriodY + fTwist * WindNoiseTurbulence(vPos, fTurbulence);

	return abs(sin(ST_PI * xyValue));
}


///////////////////////////////////////////////////////////////////////
//  WindFieldStrengthAt

float WindFieldStrengthAt(float2 vPos)
{
	float fStrength = 0.5;

	if (!ST_DIRECTX9)
	{
		fStrength = 1.0 - WindNoiseMarble(m_sRolling.m_fNoiseSize * (vPos - m_sRolling.m_vOffset),
										  m_sRolling.m_fNoiseTwist,
										  m_sRolling.m_fNoisePeriod,
										  m_sRolling.m_fNoiseTurbulence);
	}

	return fStrength;
}

#endif // ST_INCLUDE_ROLLING_WIND_NOISE

