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

#if (ST_WIND_IS_ACTIVE)

#ifndef ST_INCLUDE_WIND
#define ST_INCLUDE_WIND


///////////////////////////////////////////////////////////////////////
//  WindRoll

float WindRoll(float fCurrent,
			   float fMaxScale,
			   float fMinScale,
			   float fSpeed,
			   float fRipple,
			   float3 vPos,
			   float fTime,
			   float4 vWindVector)
{
	float fWindAngle = dot(vPos, -vWindVector.xyz) * fRipple;
	float fAdjust = TrigApproximate(float4(fWindAngle + fTime * fSpeed, 0.0, 0.0, 0.0)).x;
	fAdjust = (fAdjust + 1.0) * 0.5;

	return fCurrent * fMaxScale;
}


///////////////////////////////////////////////////////////////////////
//  WindTwitch

float WindTwitch(float fTrigOffset, float fAmount, float fSharpness, float fTime)
{
	const float c_fTwitchFudge = 0.87;

	float4 vOscillations = TrigApproximate(float4(fTime + fTrigOffset * fTrigOffset, c_fTwitchFudge * fTime + fTrigOffset, 0.0, 0.0));

	//float fTwitch = sin(fFreq1 * fTime + (vPos.x + vPos.z)) * cos(fFreq2 * fTime + vPos.y);
	float fTwitch = vOscillations.x * vOscillations.y * vOscillations.y;
	fTwitch = (fTwitch + 1.0) * 0.5;

	return fAmount * pow(saturate(fTwitch), fSharpness);
}


///////////////////////////////////////////////////////////////////////
//  WindOscillate
//
//  This function computes an oscillation value and whip value if necessary.
//  Whip and oscillation are combined like this to minimize calls to
//  TrigApproximate( ) when possible.

float WindOscillate(float3 vPos,
					float fTime,
					float fOffset,
					float fWeight,
					float fWhip,
					bool bWhip,
					bool bComplex,
					float fTwitch,
					float fTwitchFreqScale,
					float4 vWindVector,
					inout float4 vOscillations)
{
	float fOscillation = 1.0;
	if (bComplex)
	{
		if (bWhip)
			vOscillations = TrigApproximate(float4(fTime + fOffset, fTime * fTwitchFreqScale + fOffset, fTwitchFreqScale * 0.5 * (fTime + fOffset), fTime + fOffset + (1.0 - fWeight)));
		else
			vOscillations = TrigApproximate(float4(fTime + fOffset, fTime * fTwitchFreqScale + fOffset, fTwitchFreqScale * 0.5 * (fTime + fOffset), 0.0));

		float fFineDetail = vOscillations.x;
		float fBroadDetail = vOscillations.y * vOscillations.z;

		float fTarget = 1.0;
		float fAmount = fBroadDetail;
		if (fBroadDetail < 0.0)
		{
			fTarget = -fTarget;
			fAmount = -fAmount;
		}

		fBroadDetail = lerp(fBroadDetail, fTarget, fAmount);
		fBroadDetail = lerp(fBroadDetail, fTarget, fAmount);

		fOscillation = fBroadDetail * fTwitch * (1.0 - vWindVector.w) + fFineDetail * (1.0 - fTwitch);

		if (bWhip)
			fOscillation *= 1.0 + (vOscillations.w * fWhip) * fWeight;
	}
	else
	{
		if (bWhip)
			vOscillations = TrigApproximate(float4(fTime + fOffset, fTime * 0.689 + fOffset, 0.0, fTime + fOffset + (1.0 - fWeight)));
		else
			vOscillations = TrigApproximate(float4(fTime + fOffset, fTime * 0.689 + fOffset, 0.0, 0.0));

		fOscillation = vOscillations.x + vOscillations.y * vOscillations.x;

		if (bWhip)
			fOscillation *= 1.0 + (vOscillations.w * fWhip) * fWeight;
	}

	return fOscillation;
}


///////////////////////////////////////////////////////////////////////
//  WindTurbulence

float WindTurbulence(float fTime, float fOffset, float fGlobalTime, float fTurbulence)
{
	const float c_fTurbulenceFactor = 0.1;

	float4 vOscillations = TrigApproximate(float4(fTime * c_fTurbulenceFactor + fOffset, fGlobalTime * fTurbulence * c_fTurbulenceFactor + fOffset, 0.0, 0.0));

	return 1.0 - (vOscillations.x * vOscillations.y * vOscillations.x * vOscillations.y * fTurbulence);
}


///////////////////////////////////////////////////////////////////////
//  WindGlobalBehavior
//
//  This function positions any tree geometry based on their untransformed
//	position and 4 wind floats.

float3 WindGlobalBehavior(float3 vPos,
						  float3 vInstancePos,
						  float4 vWindGlobal,
						  float4 vWindVector,
						  float3 vWindBranchAdherences,
						  bool bPreserveShape)
{
	float fLength = 1.0;
	if (bPreserveShape)
		fLength = length(vPos.xyz);

	// compute how much the height contributes
	float fAdjust = max(vPos.z - (1.0 / vWindGlobal.z) * 0.25, 0.0) * vWindGlobal.z;
	if (fAdjust != 0.0)
		fAdjust = pow(abs(fAdjust), vWindGlobal.w);

	// primary oscillation
	float4 vOscillations = TrigApproximate(float4(vInstancePos.x + vWindGlobal.x, vInstancePos.y + vWindGlobal.x * 0.8, 0.0, 0.0));
	float fOsc = vOscillations.x + (vOscillations.y * vOscillations.y);
	float fMoveAmount = vWindGlobal.y * fOsc;

	// move a minimum amount based on direction adherence
	fMoveAmount += (vWindBranchAdherences.x / vWindGlobal.z);

	// adjust based on how high up the tree this vertex is
	fMoveAmount *= fAdjust;

	// xy component
	vPos.xy += vWindVector.xy * fMoveAmount;

	if (bPreserveShape)
		vPos.xyz = normalize(vPos.xyz) * fLength;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindLeafRipple

float3 WindLeafRipple(float3 vPos,
					  float3 vNormal,
					  float fScale,
					  float fPackedRippleDir,
					  float fTime,
					  float fAmount,
					  bool bDirectional,
					  float fTrigOffset)
{
	// compute how much to move
	float4 vInput = float4(fTime + fTrigOffset, 0.0, 0.0, 0.0);
	float fMoveAmount = fAmount * TrigApproximate(vInput).x;

	if (bDirectional)
	{
		vPos.xyz += vNormal.xyz * fMoveAmount * fScale;
	}
	else
	{
		float3 vRippleDir = UnpackNormalFromFloat(fPackedRippleDir);
		vPos.xyz += vRippleDir * fMoveAmount * fScale;
	}

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindLeafTumble

float3 WindLeafTumble(float3 vPos,
					  inout float3 vNormal,
					  inout float3 vTangent,
					  float  fScale,
					  float3 vAnchor,
					  float3 vGrowthDir,
					  float  fTrigOffset,
					  float  fTime,
					  float  fFlip,
					  float  fTwist,
					  float  fAdherence,
					  float3 vTwitch,
					  bool   bTwitch,
					  float4 vWindVector)
{
	// compute all oscillations up front
	float3 vFracs = frac((vAnchor + fTrigOffset) * 30.3);
	float fOffset = vFracs.x + vFracs.y + vFracs.z;
	float4 vOscillations = TrigApproximate(float4(fTime + fOffset, fTime * 0.75 - fOffset, fTime * 0.01 + fOffset, fTime * 1.0 + fOffset));

	// move to the origin and get the growth direction
	float3 vOriginPos = vPos.xyz - vAnchor;
	float fLength = length(vOriginPos);

	// twist
	float fOsc = vOscillations.x + vOscillations.y * vOscillations.y;
	float3x3 matTumble = ArbitraryAxisRotationMatrix(vGrowthDir, fScale * fTwist * fOsc);

	// with wind
	float3 vAxis = wind_cross(vGrowthDir, vWindVector.xyz);
	float fDot = clamp(dot(vWindVector.xyz, vGrowthDir), -1.0, 1.0);
	vAxis.z += fDot;
	vAxis = normalize(vAxis);

	float fAngle = acos(fDot);

	fOsc = vOscillations.y - vOscillations.x * vOscillations.x;

	float fTwitch = 0.0;
	if (bTwitch)
		fTwitch = WindTwitch(fTrigOffset, vTwitch.x, vTwitch.y, vTwitch.z + fOffset);

	matTumble = mul_float3x3_float3x3(matTumble, ArbitraryAxisRotationMatrix(vAxis, fScale * (fAngle * fAdherence + fOsc * fFlip + fTwitch)));

	vNormal = mul_float3x3_float3(matTumble, vNormal);
	vTangent = mul_float3x3_float3(matTumble, vTangent);
	vOriginPos = mul_float3x3_float3(matTumble, vOriginPos);

	vOriginPos = normalize(vOriginPos) * fLength;

	return (vOriginPos + vAnchor);
}


///////////////////////////////////////////////////////////////////////
//  WindLeaf1

float3 WindLeaf1(float3 vPos,
				 float3 vInstancePos,
				 inout float3 vNormal,
				 inout float3 vTangent,
				 float  fScale,
				 float3 vAnchor,
				 float  fPackedGrowthDir,
				 float  fPackedRippleDir,
				 float  fTrigOffset,
				 float4 vWindVector,
				 float3 vWindLeaf1Ripple,
				 float4 vWindLeaf1Tumble,
				 float3 vWindLeaf1Twitch,
				 float  fEffectFade)
{
	if (ST_WIND_EFFECT_LEAF_OCCLUSION_1)
	{
		float2 vDir = -normalize(vAnchor.xy);
		float fDot = dot(vDir.xy, vWindVector.xy);
		float fDirContribution = (fDot + 1.0) * 0.5;
		fDirContribution = lerp(vWindLeaf1Ripple.z, 1.0, fDirContribution);
		fScale *= fDirContribution;
	}

	float fRippleScalar = 1.0;
	float fTumbleScalar = 1.0;
	if (ST_WIND_EFFECT_ROLLING)
	{
		float fNoise = WindFieldStrengthAt(vAnchor.xy + vInstancePos.xy);

		fRippleScalar = lerp(1.0, lerp(m_sRolling.m_fLeafRippleMin, 1.0, fNoise), fEffectFade);
		fTumbleScalar = lerp(1.0, lerp(m_sRolling.m_fLeafTumbleMin, 1.0, fNoise), fEffectFade);
	}

	if (ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1)
	{
		vPos = WindLeafRipple(vPos,
							  vNormal,
							  fScale * fRippleScalar,
							  fPackedRippleDir,
							  vWindLeaf1Ripple.x,
							  vWindLeaf1Ripple.y,
							  true,
							  fTrigOffset);
	}
	else if (ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1)
	{
		vPos = WindLeafRipple(vPos,
							  vNormal,
							  fScale * fRippleScalar,
							  fPackedRippleDir,
							  vWindLeaf1Ripple.x,
							  vWindLeaf1Ripple.y,
							  false,
							  fTrigOffset);
	}

	if (ST_WIND_EFFECT_LEAF_TUMBLE_1)
	{
		if ( ST_LEAVES_PRESENT && !ST_FACING_LEAVES_PRESENT )
		{
			float3 vGrowthDir = UnpackNormalFromFloat(fPackedGrowthDir);
			
			vPos = WindLeafTumble(vPos,
							  vNormal,
							  vTangent,
							  fScale * fTumbleScalar,
							  vAnchor,
							  vGrowthDir,
							  fTrigOffset,
							  vWindLeaf1Tumble.x,
							  vWindLeaf1Tumble.y,
							  vWindLeaf1Tumble.z,
							  vWindLeaf1Tumble.w,
							  vWindLeaf1Twitch.xyz,
							  ST_WIND_EFFECT_LEAF_TWITCH_1,
							  vWindVector);
		}
	}

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindLeaf2

float3 WindLeaf2(float3	vPos,
				 float3 vInstancePos,
				 inout float3 vNormal,
				 inout float3 vTangent,
				 float  fScale,
				 float3 vAnchor,
				 float  fPackedGrowthDir,
				 float  fPackedRippleDir,
				 float  fTrigOffset,
				 float4 vWindVector,
				 float3 vWindLeaf2Ripple,
				 float4 vWindLeaf2Tumble,
				 float3 vWindLeaf2Twitch,
				 float  fEffectFade)
{
	if (ST_WIND_EFFECT_LEAF_OCCLUSION_2)
	{
		float2 vDir = ST_COORDSYS_Z_UP ? -normalize(vAnchor.xy) : -normalize(vAnchor.xz);
		float fDot = dot(vDir.xy, vWindVector.xy);
		float fDirContribution = (fDot + 1.0) * 0.5;
		fDirContribution = lerp(vWindLeaf2Ripple.z, 1.0, fDirContribution);
		fScale *= fDirContribution;
	}

	float fRippleScalar = 1.0;
	float fTumbleScalar = 1.0;
	if (ST_WIND_EFFECT_ROLLING)
	{
		float fNoise = WindFieldStrengthAt(vAnchor.xy + vInstancePos.xy);

		fRippleScalar = lerp(1.0, lerp(m_sRolling.m_fLeafRippleMin, 1.0, fNoise), fEffectFade);
		fTumbleScalar = lerp(1.0, lerp(m_sRolling.m_fLeafTumbleMin, 1.0, fNoise), fEffectFade);
	}

	if (ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2)
	{
		vPos = WindLeafRipple(vPos,
							  vNormal,
							  fScale * fRippleScalar,
							  fPackedRippleDir,
							  vWindLeaf2Ripple.x,
							  vWindLeaf2Ripple.y,
							  true,
							  fTrigOffset);
	}
	else if (ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2)
	{
		vPos = WindLeafRipple(vPos,
							  vNormal,
							  fScale * fRippleScalar,
							  fPackedRippleDir,
							  vWindLeaf2Ripple.x,
							  vWindLeaf2Ripple.y,
							  false,
							  fTrigOffset);
	}

	if (ST_WIND_EFFECT_LEAF_TUMBLE_2)
	{
		if ( ST_LEAVES_PRESENT && !ST_FACING_LEAVES_PRESENT )
		{
			float3 vGrowthDir = UnpackNormalFromFloat(fPackedGrowthDir);

			vPos = WindLeafTumble(vPos,
							  vNormal,
							  vTangent,
							  fScale * fTumbleScalar,
							  vAnchor,
							  vGrowthDir,
							  fTrigOffset,
							  vWindLeaf2Tumble.x,
							  vWindLeaf2Tumble.y,
							  vWindLeaf2Tumble.z,
							  vWindLeaf2Tumble.w,
							  vWindLeaf2Twitch.xyz,
							  ST_WIND_EFFECT_LEAF_TWITCH_2,
							  vWindVector);
		}
	}

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindFrondRippleOneSided

float3 WindFrondRippleOneSided(float3 vPos,
							   inout float3 vDirection,
							   float  fU,
							   float  fV,
							   float  fRippleScale,
							   float4 vWindFrondRipple)
{
	float fOffset = 0.0;
	if (fU < 0.5)
		fOffset = 0.75;

	float4 vOscillations = TrigApproximate(float4(vWindFrondRipple.x * fV * vWindFrondRipple.z + fOffset, 0.0, 0.0, 0.0));

	float fAmount = fRippleScale * vOscillations.x * vWindFrondRipple.y;
	float3 vOffset = fAmount * vDirection;
	vPos.xyz += vOffset;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindFrondRippleTwoSided

float3 WindFrondRippleTwoSided(float3 vPos,
							   inout float3 vDirection,
							   float  fU,
							   float  fLengthPercent,
							   float  fPackedRippleDir,
							   float  fRippleScale,
							   float4 vWindFrondRipple)
{
	float4 vOscillations = TrigApproximate(float4(vWindFrondRipple.x * fLengthPercent * vWindFrondRipple.z, 0.0, 0.0, 0.0));

	float3 vRippleDir = UnpackNormalFromFloat(fPackedRippleDir);

	float fAmount = fRippleScale * vOscillations.x * vWindFrondRipple.y;
	float3 vOffset = fAmount * vRippleDir;

	vPos.xyz += vOffset;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindSimpleBranch

float3 WindSimpleBranch(float3 vPos,
						float3 vInstancePos,
						float  fWeight,
						float  fDirection,
						float  fTime,
						float  fDistance,
						float  fTwitch,
						float  fTwitchScale,
						float  fWhip,
						bool   bWhip,
						bool   bComplex,
						float4 vWindVector)
{
	// turn the offset back into a nearly normalized vector
	float3 vLocalWindVector = UnpackNormalFromFloat(fDirection);
	vLocalWindVector = vLocalWindVector * fWeight;

	// try to fudge time a bit so that instances aren't in sync
	fTime += vInstancePos.x + vInstancePos.y;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillate(vPos,
							   fTime,
							   fDirection,
							   fWeight,
							   fWhip,
							   bWhip,
							   bComplex,
							   fTwitch,
							   fTwitchScale,
							   vWindVector,
							   vOscillations);

	vPos.xyz += vLocalWindVector * fOsc * fDistance;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindDirectionalBranch

float3 WindDirectionalBranch(float3 vPos,
							 float3 vInstancePos,
							 float  fWeight,
							 float  fDirection,
							 float  fTime,
							 float  fDistance,
							 float  fTurbulence,
							 float  fAdherence,
							 float  fTwitch,
							 float  fTwitchScale,
							 float  fWhip,
							 bool   bWhip,
							 bool   bComplex,
							 bool   bTurbulence,
							 float  fFieldStrength,
							 float  fWindAnimation,
							 float4 vWindVector,
							 float  fEffectFade)
{
	// turn the offset back into a nearly normalized vector
	float3 vLocalWindVector = UnpackNormalFromFloat(fDirection);
	vLocalWindVector = vLocalWindVector * fWeight;

	// try to fudge time a bit so that instances aren't in sync
	fTime += vInstancePos.x + vInstancePos.y;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillate(vPos,
							   fTime,
							   fDirection,
							   fWeight,
							   fWhip,
							   bWhip,
							   bComplex,
							   fTwitch,
							   fTwitchScale,
							   vWindVector,
							   vOscillations);

	vPos.xyz += vLocalWindVector * fOsc * fDistance;

	// add in the direction, accounting for turbulence
	float fAdherenceScale = 1.0;

	if (bTurbulence)
		fAdherenceScale = WindTurbulence(fTime, fDirection, fWindAnimation, fTurbulence);

	if (bWhip)
	{
		const float c_fWindStrength = vWindVector.w;
		fAdherenceScale += vOscillations.w * c_fWindStrength * fWhip;
	}

	float3 vAdjustedWind = vLocalWindVector.xyz;
	if (ST_WIND_EFFECT_ROLLING)
		vAdjustedWind.z += m_sRolling.m_fBranchVerticalOffset * fEffectFade;

	vPos.xyz += vWindVector.xyz * fAdherence * fAdherenceScale * fWeight * fFieldStrength;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindDirectionalBranchFrondStyle

float3 WindDirectionalBranchFrondStyle(float3 vPos,
									   float3 vInstancePos,
									   float  fWeight,
									   float  fDirection,
									   float  fTime,
									   float  fDistance,
									   float  fTurbulence,
									   float  fAdherence,
									   float  fTwitch,
									   float  fTwitchScale,
									   float  fWhip,
									   bool   bWhip,
									   bool   bComplex,
									   bool   bTurbulence,
									   float  fFieldStrength,
									   float  fWindAnimation,
									   float4 vWindVector,
									   float3 vWindBranchAnchor)
{
	// turn the offset back into a nearly normalized vector
	float3 vLocalWindVector = UnpackNormalFromFloat(fDirection);
	vLocalWindVector = vLocalWindVector * fWeight;

	// try to fudge time a bit so that instances aren't in sync
	fTime += vInstancePos.x + vInstancePos.y;

	// oscillate
	float4 vOscillations;
	float fOsc = WindOscillate(vPos,
							   fTime,
							   fDirection,
							   fWeight,
							   fWhip,
							   bWhip,
							   bComplex,
							   fTwitch,
							   fTwitchScale,
							   vWindVector,
							   vOscillations);

	vPos.xyz += vLocalWindVector * fOsc * fDistance;

	// add in the direction, accounting for turbulence
	float fAdherenceScale = 1.0;
	if (bTurbulence)
		fAdherenceScale = WindTurbulence(fTime, fDirection, fWindAnimation, fTurbulence);

	if (bWhip)
	{
		const float c_fWindStrength = vWindVector.w;
		fAdherenceScale += vOscillations.w * c_fWindStrength * fWhip;
	}

	float3 vWindAdherenceVector = vWindBranchAnchor.xyz - vPos.xyz;
	vPos.xyz += vWindAdherenceVector * fAdherence * fAdherenceScale * fWeight * fFieldStrength;

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  WindBranchBehavior

float3 WindBranchBehavior(float3 vPos,
						  float3 vInstancePos,
						  float4 vWindData,
						  float4 vWindBranch,
						  float  fWindAnimation,
						  float4 vWindVector,
						  float3 vWindBranchAnchor,
						  float4 vWindBranchTwitch,
						  float2 vWindBranchWhip,
						  float2 vWindTurbulences,
						  float3 vWindBranchAdherences,
						  float  fFieldStrength,
						  float  fEffectFade)
{
	
	if ( ST_USED_AS_GRASS ) // LAVA: only one path for grass wind. Reducing shader length...
	{
		vPos = WindDirectionalBranch(vPos,
									 vInstancePos,
									 vWindData.x,
									 vWindData.y,
									 vWindBranch.x,
									 vWindBranch.y,
									 vWindTurbulences.x,
									 vWindBranchAdherences.y,
									 vWindBranchTwitch.x,
									 vWindBranchTwitch.y,
									 vWindBranchWhip.x,
									 ST_WIND_EFFECT_BRANCH_WHIP_1,
									 ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1,
									 ST_WIND_EFFECT_BRANCH_TURBULENCE_1,
									 fFieldStrength,
									 fWindAnimation,
									 vWindVector,
									 fEffectFade);
	}
	else // LAVA: This is the normal path, but for us, it will only happen in tree rendering - not grass.
	{
		// level 1
		if (ST_WIND_EFFECT_BRANCH_SIMPLE_1)
			vPos = WindSimpleBranch(vPos,
									vInstancePos,
									vWindData.x,
									vWindData.y,
									vWindBranch.x,
									vWindBranch.y,
									vWindBranchTwitch.x,
									vWindBranchTwitch.y,
									vWindBranchWhip.x,
									ST_WIND_EFFECT_BRANCH_WHIP_1,
									ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1,
									vWindVector);
		else if (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1)
			vPos = WindDirectionalBranch(vPos,
										 vInstancePos,
										 vWindData.x,
										 vWindData.y,
										 vWindBranch.x,
										 vWindBranch.y,
										 vWindTurbulences.x,
										 vWindBranchAdherences.y,
										 vWindBranchTwitch.x,
										 vWindBranchTwitch.y,
										 vWindBranchWhip.x,
										 ST_WIND_EFFECT_BRANCH_WHIP_1,
										 ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1,
										 ST_WIND_EFFECT_BRANCH_TURBULENCE_1,
										 fFieldStrength,
										 fWindAnimation,
										 vWindVector,
										 fEffectFade);
		else if (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1)
		{
			vPos = WindDirectionalBranchFrondStyle(vPos,
												   vInstancePos,
												   vWindData.x,
												   vWindData.y,
												   vWindBranch.x,
												   vWindBranch.y,
												   vWindTurbulences.x,
												   vWindBranchAdherences.y,
												   vWindBranchTwitch.x,
												   vWindBranchTwitch.y,
												   vWindBranchWhip.x,
												   ST_WIND_EFFECT_BRANCH_WHIP_1,
												   ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1,
												   ST_WIND_EFFECT_BRANCH_TURBULENCE_1,
												   fFieldStrength,
												   fWindAnimation,
												   vWindVector,
												   vWindBranchAnchor);
		}
	} // LAVA - just closing bracket

	// level 2
	if ( !ST_USED_AS_GRASS )	// LAVA: Don't use hierarchical geometry for grass at all.
	{
		if (ST_WIND_EFFECT_BRANCH_SIMPLE_2)
			vPos = WindSimpleBranch(vPos,
									vInstancePos,
									vWindData.z,
									vWindData.w,
									vWindBranch.z,
									vWindBranch.w,
									vWindBranchTwitch.z,
									vWindBranchTwitch.w,
									vWindBranchWhip.y,
									ST_WIND_EFFECT_BRANCH_WHIP_2,
									ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2,
									vWindVector);
		else if (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2)
			vPos = WindDirectionalBranch(vPos,
										 vInstancePos,
										 vWindData.z,
										 vWindData.w,
										 vWindBranch.z,
										 vWindBranch.w,
										 vWindTurbulences.y,
										 vWindBranchAdherences.z,
										 vWindBranchTwitch.z,
										 vWindBranchTwitch.w,
										 vWindBranchWhip.y,
										 ST_WIND_EFFECT_BRANCH_WHIP_2,
										 ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2,
										 ST_WIND_EFFECT_BRANCH_TURBULENCE_2,
										 fFieldStrength,
										 fWindAnimation,
										 vWindVector,
										 fEffectFade);
		else if (ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2)
			vPos = WindDirectionalBranchFrondStyle(vPos,
												   vInstancePos,
												   vWindData.z,
												   vWindData.w,
												   vWindBranch.z,
												   vWindBranch.w,
												   vWindTurbulences.y,
												   vWindBranchAdherences.z,
												   vWindBranchTwitch.z,
												   vWindBranchTwitch.w,
												   vWindBranchWhip.y,
												   ST_WIND_EFFECT_BRANCH_WHIP_2,
												   ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2,
												   ST_WIND_EFFECT_BRANCH_TURBULENCE_2,
												   fFieldStrength,
												   fWindAnimation,
												   vWindVector,
												   vWindBranchAnchor);
	}
	

	return vPos;
}


///////////////////////////////////////////////////////////////////////
//  SpeedTreeWindModel
//
//	todo: comment this function and those above
//	todo: comment these parameters
//	todo: ST_MULTIPASS_STABILIZE

// todo: need to condense these parameters into a single struct? SWindDynamics?

// todo: remove
#ifdef ST_WIND_EFFECT_ROLLING
	#undef ST_WIND_EFFECT_ROLLING
	#define ST_WIND_EFFECT_ROLLING 0
#endif

void SpeedTreeWindModel(// per vertex parameters
						inout float3	vVertexPosition,
						inout float3	vVertexNormal,
						float3			vVertexTangent,
						float2			vVertexDiffuseTexCoords,
						float3			vVertexInstancePos,
						float4			vVertexWindBranchData,
						float3			vVertexWindExtraData,
						float			fVertexGeometryHint,
						float3			vVertexLeafAnchorPoint,
						float			fVertexWindFlags,
						// global wind behavior parameters (tied to shader constants)
						float4			vWindParamGlobal,
						float4			vWindParamVector,
						float3			vWindParamBranchAdherences,
						bool			bWindParamPreserveShape,
						// branch wind behavior parameters (tied to shader constants)
						float			vWindParamTime,
						float4			vWindParamBranch,
						float3			vWindParamBranchAnchor,
						float4			vWindParamBranchTwitch,
						float2			vWindParamBranchWhip,
						float2			vWindParamTurbulences,
						// frond wind parameters (tied to shader constants)
						float4			vWindParamFrondRipple,
						// leaf wind parameters (tied to shader constants)
						float3			vWindParamLeaf1Ripple,
						float4			vWindParamLeaf1Tumble,
						float3			vWindParamLeaf1Twitch,
						float3			vWindParamLeaf2Ripple,
						float4			vWindParamLeaf2Tumble,
						float3			vWindParamLeaf2Twitch,
						// misc parameters
						float3x3		mOrientation,
						float			fLeafWindTrigOffset,
						float			fLodTransition
					   )
{
	// needed later to measure effect
	float3 vOriginalVertexPosition = vVertexPosition;

	// rolling wind
	float fFieldStrength = 1.0;
	// todo: restore?
	float fRollingEffectFade = 1.0;//ST_WIND_LOD_ROLLING_FADE ? fLodTransition : 1.0;

	if ( !ST_USED_AS_GRASS )	// LAVA: not used for grass. Reducing shader length.
	{
		if (ST_WIND_EFFECT_ROLLING && ST_WIND_LOD_FULL)
		{
			float fNoise = WindFieldStrengthAt(vVertexPosition.xy + vVertexInstancePos.xy);

			fFieldStrength = lerp(1.0, lerp(m_sRolling.m_fBranchFieldMin, 1.0, fNoise), fRollingEffectFade);
		}
	}

	// global component
	float3 vGlobalWindEffect = float3(0.0, 0.0, 0.0);
	
	if ( !ST_USED_AS_GRASS )	// LAVA: not used for grass. Reducing shader length
	{
		if (ST_WIND_EFFECT_GLOBAL_WIND)
			vGlobalWindEffect = WindGlobalBehavior(vVertexPosition,
												   vVertexInstancePos,
												   vWindParamGlobal,
												   vWindParamVector,
												   vWindParamBranchAdherences,
												   bWindParamPreserveShape) - vVertexPosition;
	}
											   
	// branch component
	float3 vBranchWindEffect = float3(0.0, 0.0, 0.0);
	if (ST_WIND_BRANCH_WIND_ACTIVE)
	{
		vBranchWindEffect = WindBranchBehavior(vVertexPosition,
											   vVertexInstancePos,
											   vVertexWindBranchData,
											   vWindParamBranch,
											   vWindParamTime,
											   vWindParamVector,
											   vWindParamBranchAnchor,
											   vWindParamBranchTwitch,
											   vWindParamBranchWhip,
											   vWindParamTurbulences,
											   vWindParamBranchAdherences,
											   fFieldStrength,
											   fRollingEffectFade) - vVertexPosition;
	}

	// frond component
	float3 vFrondWindEffect = float3(0.0, 0.0, 0.0);
	if (ST_FRONDS_PRESENT && VERTEX_PROPERTY_WINDEXTRADATA_PRESENT)
	{
		if (ST_ONLY_FRONDS_PRESENT || fVertexGeometryHint == ST_GEOMETRY_TYPE_HINT_FRONDS)
		{
			// pull frond-specific parameters from vVertexWindExtraData
			const float c_fVertexFrondPackedRippleDir = vVertexWindExtraData.x;
			const float c_fVertexRippleScale = vVertexWindExtraData.y;
			const float c_fVertexLengthPercent = vVertexWindExtraData.z;
														   
			if ( ST_USED_AS_GRASS )	// LAVA: We know that our grass uses one sided frond wind
			{
				vFrondWindEffect = WindFrondRippleOneSided(vVertexPosition,
														   vVertexNormal,
														   vVertexDiffuseTexCoords.x,
														   vVertexDiffuseTexCoords.y,
														   c_fVertexRippleScale,
														   vWindParamFrondRipple) - vVertexPosition;
			}
			else // LAVA: code path for trees.
			{
				if (ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED)
				{
					vFrondWindEffect = WindFrondRippleOneSided(vVertexPosition,
															   vVertexNormal,
															   vVertexDiffuseTexCoords.x,
															   vVertexDiffuseTexCoords.y,
															   c_fVertexRippleScale,
															   vWindParamFrondRipple) - vVertexPosition;
				}
				else if (ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED)
				{
					vFrondWindEffect = WindFrondRippleTwoSided(vVertexPosition,
															   vVertexNormal,
															   vVertexDiffuseTexCoords.x,
															   c_fVertexLengthPercent,
															   c_fVertexFrondPackedRippleDir,
															   c_fVertexRippleScale,
															   vWindParamFrondRipple) - vVertexPosition;
				}
			}
		}
	}

	// leaf component
	float3 vLeafWindEffect = float3(0.0, 0.0, 0.0);
	// todo: trying to track hitching/weird leaf card bug
	//if ((ST_LEAVES_PRESENT || ST_FACING_LEAVES_PRESENT) && VERTEX_PROPERTY_LEAFANCHORPOINT_PRESENT && VERTEX_PROPERTY_WINDEXTRADATA_PRESENT)
	
	if ( !ST_USED_AS_GRASS )	// LAVA: Leaves wind not used for grass. Reducing shader length.
	{
		if (ST_LEAVES_PRESENT || ST_FACING_LEAVES_PRESENT)
		{
			// determine the leaf anchor point based on the type of leaf geometry
			const float3 c_vWindAnchor = (ST_FACING_LEAVES_PRESENT && !ST_LEAVES_PRESENT) ? vVertexPosition : mul_float3x3_float3(mOrientation, vVertexLeafAnchorPoint);

			// pull leaf-specific parameters from vVertexWindExtraData
			const float c_fVertexWindScale = vVertexWindExtraData.x;
			const float c_fVertexPackedGrowthDir = vVertexWindExtraData.y;
			const float c_fVertexPackedRippleDir = vVertexWindExtraData.z;

			// this condition is only necessary if a non-leaf geometry type is mixed into the draw call
			// LAVA: We don't want to check anything with the unified shaders model - we know that we have leaves already.
			//if (ST_ONLY_LEAVES_PRESENT || fVertexGeometryHint > ST_GEOMETRY_TYPE_HINT_FRONDS)
			if ( true )
			{
				if (fVertexWindFlags > 0.0) // is leaf wind type 1 or 2 in effect?
				{
					vLeafWindEffect = WindLeaf2(vVertexPosition,
												vVertexInstancePos,
												vVertexNormal,
												vVertexTangent,
												c_fVertexWindScale,
												c_vWindAnchor,
												c_fVertexPackedGrowthDir,
												c_fVertexPackedRippleDir,
												fLeafWindTrigOffset,
												vWindParamVector,
												vWindParamLeaf2Ripple,
												vWindParamLeaf2Tumble,
												vWindParamLeaf2Twitch,
												fRollingEffectFade) - vVertexPosition;
				}
				else
				{
					vLeafWindEffect = WindLeaf1(vVertexPosition,
												vVertexInstancePos,
												vVertexNormal,
												vVertexTangent,
												c_fVertexWindScale,
												c_vWindAnchor,
												c_fVertexPackedGrowthDir,
												c_fVertexPackedRippleDir,
												fLeafWindTrigOffset,
												vWindParamVector,
												vWindParamLeaf1Ripple,
												vWindParamLeaf1Tumble,
												vWindParamLeaf1Twitch,
												fRollingEffectFade) - vVertexPosition;
				}
			}
		}
	}

	// handle smooth LOD transition; there are four distinct levels of details for wind:
	//
	//	1. No wind effect [LOWEST/FASTEST]
	//	2. Global wind effect (gentle sway in trees and/or billboards)
	//  3. Branch wind effect (branches move independently, often includes global if set in Modeler's wind settings)
	//  4. Frond and leaf wind effects [HIGHEST/SLOWEST] (detailed wind, often includes branch & global when set in Modeler's wind settings)
	//
	//	these states (none, global, branch, full) allow these particular effects to pass through only if they're enabled in the Modeler
	//
	//	possible wind LOD states:
	//	- none, no LOD transition (ST_WIND_LOD_NONE)
	//	- global, no LOD tranition (ST_WIND_LOD_GLOBAL)
	//	- up-to-branch wind, no LOD transition (ST_WIND_LOD_BRANCH)
	//	- all effects enabled, no LOD transition (ST_WIND_LOD_FULL)
	//	- LOD transition between no wind and global wind (ST_WIND_LOD_NONE_X_GLOBAL)
	//	- LOD transition between no wind and branch wind (ST_WIND_LOD_NONE_X_BRANCH)
	//	- LOD transition between no wind and full wind (ST_WIND_LOD_NONE_X_FULL)
	//	- LOD transition between global wind and branch wind (ST_WIND_LOD_GLOBAL_X_BRANCH)
	//	- LOD transition between global wind and full wind (ST_WIND_LOD_GLOBAL_X_FULL)
	//	- LOD transition between branch wind and full wind (ST_WIND_LOD_BRANCH_X_FULL)

	// todo: since we may be actually evaluating these at runtime, there's got to be a simpler way (e.g. upload coeffs for one long interp equation?)
	//
	// todo: maybe something like:
	//
	//	vVertexPosition += vGlobalWindEffect * GLOBAL_AMT + vBranchWindEffect + BRANCH_AMT + ...
	//
	//	where GLOBAL_AMT, etc can be #defined as either 0.0, 1.0, or fLodTransition

	if ( ST_USED_AS_GRASS ) // LAVA: Don't want this if ladder ...
	{
		vVertexPosition += vBranchWindEffect + vFrondWindEffect;
	}
	else
	{
		if (ST_WIND_LOD_NONE_X_GLOBAL)
			vVertexPosition += vGlobalWindEffect * fLodTransition;
		else if (ST_WIND_LOD_NONE_X_BRANCH)
			vVertexPosition += (vGlobalWindEffect + vBranchWindEffect) * fLodTransition;
		else if (ST_WIND_LOD_NONE_X_FULL)
			vVertexPosition += (vGlobalWindEffect + vBranchWindEffect + vFrondWindEffect + vLeafWindEffect) * fLodTransition;
		else if (ST_WIND_LOD_GLOBAL_X_BRANCH)
			vVertexPosition += vGlobalWindEffect + vBranchWindEffect * fLodTransition;
		else if (ST_WIND_LOD_GLOBAL_X_FULL)
			vVertexPosition += vGlobalWindEffect + (vBranchWindEffect + vFrondWindEffect + vLeafWindEffect) * fLodTransition;
		else if (ST_WIND_LOD_BRANCH_X_FULL)
			vVertexPosition += vGlobalWindEffect + vBranchWindEffect + (vFrondWindEffect + vLeafWindEffect) * fLodTransition;
		else if (ST_WIND_LOD_GLOBAL)
			vVertexPosition += vGlobalWindEffect;
		else if (ST_WIND_LOD_BRANCH)
			vVertexPosition += vGlobalWindEffect + vBranchWindEffect;
		else if (ST_WIND_LOD_FULL)
			vVertexPosition += vGlobalWindEffect + vBranchWindEffect + vFrondWindEffect + vLeafWindEffect;
		else if (ST_WIND_LOD_NONE)
		{
			// do nothing, not vVertexPosition fall through unchanged
		}
	}
	
}

#endif // ST_INCLUDE_WIND

#endif // ST_WIND_IS_ACTIVE

