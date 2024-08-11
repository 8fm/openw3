///////////////////////////////////////////////////////////////////////  
$HEADER$
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


///////////////////////////////////////////////////////////////////////
//	Lots of #defines controlled by the user settings during SRT compilation

$DEFINES$


///////////////////////////////////////////////////////////////////////
//	Include files

#define VERTEX_SHADER
#include "Include_SetUp.fx"
#include "Include_Uniforms.fx"
#include "Include_Utility.fx"
#include "Include_UserInterpolants.fx"
#include "Include_RollingWindNoise.fx"
#include "Include_Wind.fx"

// LAVA++
#include "../../include/common.fx"
#include "../../include/globalConstantsVS.fx"
#include "../../include/include_utilities.fx"
#if (ST_USED_AS_GRASS)
	#include "../../include/include_pigment.fx"
#endif
// LAVA--

///////////////////////////////////////////////////////////////////////
//	Vertex shader entry point
//
//	Main lighting/wind deferred vertex shader for 3D geometry

$VERTEX_MAIN_FUNCTION_DECL$
{
	#if (ST_XBOX_360)
		// compute the instance index
		int nNumIndicesPerInstance = g_vInstancingParams_360.x;
		int nInstanceIndex = (nIndex + 0.5f) / nNumIndicesPerInstance;

		// compute the mesh index
		int nMeshIndex = nIndex - nInstanceIndex * nNumIndicesPerInstance;

		// fetch the actual mesh vertex data
		float4 in_vInstancePosAndScalar;
		float4 in_vInstanceUpVectorAndLod1;
		float4 in_vInstanceRightVectorAndLod2;
		$VERTEX_360_VFETCH$
	#endif

	// set up all possible input vertex properties; give initial values even to those properties that will not be used
	// to avoid compilation warnings on some platforms
	float3 in_vPosition = float3(0.0, 0.0, 0.0);
	float3 in_vLodPosition = float3(0.0, 0.0, 0.0);
	float  in_fGeometryTypeHint = 0.0;
	float2 in_vDiffuseTexCoords = float2(0.0, 0.0);
	float3 in_vLeafCardCorner = float3(0.0, 0.0, 0.0);
	float  in_fLeafCardLodScalar = 0.0;
	float  in_fLeafCardSelfShadowOffset = 0.0;
	float4 in_vWindBranchData = float4(0.0, 0.0, 0.0, 0.0);
	float3 in_vWindExtraData = float3(0.0, 0.0, 0.0);
	float  in_fWindFlags = 0.0;
	float3 in_vLeafAnchorPoint = float3(0.0, 0.0, 0.0);
	float  in_fBoneID = 0.0;
	float3 in_vBranchSeamDiffuse = float3(0.0, 0.0, 0.0);
	float2 in_vBranchSeamDetail = float2(0.0, 0.0);
	float2 in_vDetailTexCoords = float2(0.0, 0.0);
	float3 in_vNormal = float3(0.0, 0.0, 0.0);
	float3 in_vTangent = float3(0.0, 0.0, 0.0);
	float  in_fAmbientOcclusion = 0.0;
	float2 in_vLightMapTexCoords = float2(0.0, 0.0);

	// unpack incoming vertex properties, if necessary
	$VERTEX_UNPACK_INPUT_ATTRIBS$

	// let those vertex properties that were passed in set the correct initial values, all others will be unused
	$VERTEX_GET_CORE_PARAMS_FROM_INPUT_ATTRIBS$

	// pull in instancing data if available and supported in rendering code
	#if (ST_OPENGL)
		#ifdef ST_OPENGL_NO_INSTANCING
			float4 in_vInstancePosAndScalar = g_vInstancePosAndScalar;
			float4 in_vInstanceUpVectorAndLod1 = g_vInstanceUpAndLod1;
			float4 in_vInstanceRightVectorAndLod2 = g_vInstanceRightAndLod2;
		#else
			float4 in_vInstancePosAndScalar =  A_avSingleUniformBlock[INSTANCE_BLOCK_POS_AND_SCALAR + gl_InstanceID * 3];
			float4 in_vInstanceUpVectorAndLod1 =  A_avSingleUniformBlock[INSTANCE_BLOCK_UP_AND_LOD1 + gl_InstanceID * 3];
			float4 in_vInstanceRightVectorAndLod2 =  A_avSingleUniformBlock[INSTANCE_BLOCK_RIGHT_AND_LOD2 + gl_InstanceID * 3];
		#endif
	#endif
	#define c_vInstancePos				in_vInstancePosAndScalar.xyz
	#define c_fInstanceScalar			in_vInstancePosAndScalar.w
	#define c_vInstanceUpVector			in_vInstanceUpVectorAndLod1.xyz
	#define c_vInstanceRightVector		in_vInstanceRightVectorAndLod2.xyz
	//LAVA++
	// Terrible hack! We don't really have any control over the instance values in the vertex buffers, so we've
	// packed the LodTransition value together with the tree fade amount. It works so the integer portion is the
	// fade (0: fully opaque, 255: transparent), and the fractional is LodTransition. We scale LodTransition back
	// a bit so that small floating point inaccuracies don't end up affecting the fade value.
	const float c_fInstanceLodTransition  	= ( in_vInstanceUpVectorAndLod1.w - floor( in_vInstanceUpVectorAndLod1.w ) ) / 0.75f;
	float c_fInstanceFade 			= floor( in_vInstanceUpVectorAndLod1.w ) / 255;
	//LAVA--
	#define c_fInstanceLodValue			in_vInstanceRightVectorAndLod2.w

	// all possible values that can be output from the vertex shader;
	// todo: which can be erased for deferred?
	float3 out_vNormal = float3(0.0, 0.0, 0.0);
	float3 out_vTangent = float3(0.0, 0.0, 0.0);
	float  out_fTransmissionFactor = 0.0;
	float  out_fAmbientOcclusion = 0.0;
	float  out_fRenderEffectsFade = 0.0;
	float  out_fFogScalar = 0.0;
	float  out_fFadeToBillboard = 0.0;
	float  out_fShadowDepth = 0.0;
	float4 out_vProjection = float4(0.0, 0.0, 0.0, 0.0);
	float2 out_vDiffuseTexCoords = float2(0.0, 0.0);
	float2 out_vDetailTexCoords = float2(0.0, 0.0);
	float3 out_vBranchSeamDiffuse = float3(0.0, 0.0, 0.0);
	float2 out_vBranchSeamDetail = float2(0.0, 0.0);
	float3 out_vNormalInTangentSpace = float3(0.0, 0.0, 0.0);
	float3 out_vSpecularHalfVector = float3(0.0, 0.0, 0.0);
	float3 out_vHueVariation = float3(0.0, 0.0, 0.0);
	float3 out_vFogColor = float3(0.0, 0.0, 0.0);
	float4 out_vShadowMapProjection0 = float4(0.0, 0.0, 0.0, 0.0);
	float4 out_vShadowMapProjection1 = float4(0.0, 0.0, 0.0, 0.0);
	float4 out_vShadowMapProjection2 = float4(0.0, 0.0, 0.0, 0.0);
	float4 out_vShadowMapProjection3 = float4(0.0, 0.0, 0.0, 0.0);

    //LAVA++
	float3 localPosition = in_vPosition;
	//LAVA--

	// the trig offset is a big part of having the wind behave distinctly among instances; this sets the
	// base value and may be modified later in this shader
	float fLeafWindTrigOffset = in_vPosition.x + in_vPosition.y;

	// branches, fronds, and leaf meshes use smooth LOD
	if (!ST_USED_AS_GRASS && ST_EFFECT_SMOOTH_LOD && (ST_BRANCHES_PRESENT || ST_FRONDS_PRESENT || ST_LEAVES_PRESENT))
		in_vPosition = lerp(in_vLodPosition, in_vPosition, c_fInstanceLodTransition);

	// need to retain a copy of the normal before it's oriented; for use with hue variation code later
	float3 vUnmodifiedNormal = in_vNormal;

	// build an orientation matrix using the incoming up and right vector; will rotate and tilt the instance
	float3 vInstanceOutVector = normalize(cross(c_vInstanceUpVector, c_vInstanceRightVector));
	float3x3 mOrientation = BuildOrientationMatrix(c_vInstanceRightVector, vInstanceOutVector, c_vInstanceUpVector);
	ST_MULTIPASS_STABILIZE
	{
		in_vPosition = mul_float3x3_float3(mOrientation, in_vPosition);
		in_vNormal = mul_float3x3_float3(mOrientation, in_vNormal);
		in_vTangent = mul_float3x3_float3(mOrientation, in_vTangent);
	}

	// fLeafWindTrigOffset would be the same value for all four corners of the leaf card without this additional offset
	if (ST_FACING_LEAVES_PRESENT)
		fLeafWindTrigOffset += in_vLeafCardCorner.x + in_vLeafCardCorner.y;

	// this single wind function encapsulates all possible speedtree wind effects and has various functions and effects that
	// are turned on and off through macros that are set during shader compilation; it also handles smooth LOD transitions from
	// more complex to simpler wind effects
	#if (ST_WIND_IS_ACTIVE)
		SpeedTreeWindModel(// per-vertex parameters
						   in_vPosition,
						   in_vNormal,
						   in_vTangent,
						   in_vDiffuseTexCoords,
						   c_vInstancePos,
						   in_vWindBranchData,
						   in_vWindExtraData,
						   in_fGeometryTypeHint,
						   in_vLeafAnchorPoint,
						   in_fWindFlags,
						   // todo
						   // global wind behavior parameters (tied to shader constants)
						   float4(m_sGlobal.m_fTime, m_sGlobal.m_fDistance, m_sGlobal.m_fHeight, m_sGlobal.m_fHeightExponent),//g_vWindGlobal,
						   float4(m_vDirection, m_fStrength),//g_vWindVector,
						   float3(m_sGlobal.m_fAdherence, m_sBranch1.m_fDirectionAdherence, m_sBranch2.m_fDirectionAdherence),//g_vWindBranchAdherences.xyz,
						   true,
						   // branch wind behavior parameters (tied to shader constants)
						   m_fWallTime,
						   float4(m_sBranch1.m_fTime, m_sBranch1.m_fDistance, m_sBranch2.m_fTime, m_sBranch2.m_fDistance),//g_vWindBranch,
						   m_vAnchor,//g_vWindBranchAnchor.xyz,
						   float4(m_sBranch1.m_fTwitch, m_sBranch1.m_fTwitchFreqScale, m_sBranch2.m_fTwitch, m_sBranch2.m_fTwitchFreqScale),//g_vWindBranchTwitch,
						   float2(m_sBranch1.m_fWhip, m_sBranch2.m_fWhip),//g_vWindBranchWhip.xy,
						   float2(m_sBranch1.m_fTurbulence, m_sBranch2.m_fTurbulence),//g_vWindTurbulences.xy,
						   // frond wind parameters (tied to shader constants)
						   float4(m_sFrondRipple.m_fTime, m_sFrondRipple.m_fDistance, m_sFrondRipple.m_fTile, m_sFrondRipple.m_fLightingScalar),//g_vWindFrondRipple,
						   // leaf wind parameters (tied to shader constants)
						   float3(m_sLeaf1.m_fRippleTime, m_sLeaf1.m_fRippleDistance, m_sLeaf1.m_fLeewardScalar),//g_vWindLeaf1Ripple.xyz,
						   float4(m_sLeaf1.m_fTumbleTime, m_sLeaf1.m_fTumbleFlip, m_sLeaf1.m_fTumbleTwist, m_sLeaf1.m_fTumbleDirectionAdherence),//g_vWindLeaf1Tumble,
						   float3(m_sLeaf1.m_fTwitchThrow, m_sLeaf1.m_fTwitchSharpness, m_sLeaf1.m_fTwitchTime),//g_vWindLeaf1Twitch.xyz,
						   float3(m_sLeaf2.m_fRippleTime, m_sLeaf2.m_fRippleDistance, m_sLeaf2.m_fLeewardScalar),//g_vWindLeaf2Ripple.xyz,
						   float4(m_sLeaf2.m_fTumbleTime, m_sLeaf2.m_fTumbleFlip, m_sLeaf2.m_fTumbleTwist, m_sLeaf2.m_fTumbleDirectionAdherence),//g_vWindLeaf2Tumble,
						   float3(m_sLeaf2.m_fTwitchThrow, m_sLeaf2.m_fTwitchSharpness, m_sLeaf2.m_fTwitchTime),//g_vWindLeaf2Twitch.xyz,
						   // misc parameters
						   mOrientation,
						   fLeafWindTrigOffset,
						   c_fInstanceLodTransition
						  );
	#endif

	if (ST_FACING_LEAVES_PRESENT)
	{
		// LOD interpolation; cards are shrunk & grown, depending on the LOD setting
		if (ST_EFFECT_SMOOTH_LOD)
			in_vLeafCardCorner.xy *= lerp(in_fLeafCardLodScalar, 1.0, c_fInstanceLodTransition);

		// build the corner coordinate and apply camera-facing transform
		float4 vLeafCardCorner = float4(ConvertFromStdCoordSys(float3(0.0, -in_vLeafCardCorner.x, in_vLeafCardCorner.y)), 1.0);
		vLeafCardCorner = mul_float4x4_float4(m_mCameraFacingMatrix, vLeafCardCorner);
		in_vPosition += vLeafCardCorner.xyz;

		// in_vLeafCardCorner.z contains an offset value to prevent z-fighting from a bunch of flat cards that may be lying in the
		// same plane -- z-fighting can be worse with wind as the cards move in and out of the same plane
		in_vPosition += in_vLeafCardCorner.z * m_vCameraDirection;
	}

	// derive binormal from normal and tangent
	float3 vBinormal = cross(in_vNormal, in_vTangent);

	// scale the whole tree
	in_vPosition *= c_fInstanceScalar;

	// move instance into position
	in_vPosition += c_vInstancePos;
	
#if (ST_USED_AS_GRASS)
		in_vPosition.xyz += ComputeCollisionOffset( in_vPosition.xyz, in_vNormal.xyz, localPosition.xyz,  m_vLavaCustomBaseTreeParams.z );
		//m_vLavaCustomBaseTreeParams.z - bending disabled for meshes with m_grassRandomColorEnabled=false to remove bending effect on rocks
#endif

	// shadow reception
	if (ST_SHADOWS_ENABLED)
	{
		out_vShadowMapProjection0 = ProjectToLightSpace(in_vPosition.xyz, m_sShadows.m_amLightModelViewProjs[0]);
		out_vShadowMapProjection1 = ProjectToLightSpace(in_vPosition.xyz, m_sShadows.m_amLightModelViewProjs[1]);
		out_vShadowMapProjection2 = ProjectToLightSpace(in_vPosition.xyz, m_sShadows.m_amLightModelViewProjs[2]);
		out_vShadowMapProjection3 = ProjectToLightSpace(in_vPosition.xyz, m_sShadows.m_amLightModelViewProjs[3]);
	}

	// distance (may be used a few times or not at all and optimized out)
	float fDistanceFromCamera = distance(m_vLodRefPosition, in_vPosition); // in_vPosition has been scaled and translated by this point

	// pass hints to pixel shader for fading by alpha value (fade to billboard & grass rendering)
	if ( ST_USED_AS_GRASS )
	{
		out_fFadeToBillboard = FadeGrass(fDistanceFromCamera) * m_fAlphaScalar; // grass just fades out, not to a billboard

		//LAVA++
		#if (ST_USED_AS_GRASS)
		out_fFadeToBillboard *= SpeedTreeGrassAlphaScalarMultiplier( fDistanceFromCamera );
		#endif
		//LAVA--
	}
	else  if ( ST_ONLY_BRANCHES_PRESENT )
	{
		float fade = Fade3dTree(c_fInstanceLodValue);
		fade = saturate( fade * 1.5 - 0.5 ); //< looks better (better fits the leaves)
		fade *= m_fAlphaScalar;

		c_fInstanceFade = lerp( 1.0, c_fInstanceFade, fade );		
		out_fFadeToBillboard = 1;
	}
	else
	{
		out_fFadeToBillboard = Fade3dTree(c_fInstanceLodValue) * m_fAlphaScalar;
	}

	// hue variation
	{
		// vary by instance position (the entire tree instance will be tinted based on it's unique pos/orientation)
		out_vHueVariation = GenerateHueVariationByPos(m_fHueVariationByPos, c_vInstanceRightVector);

		// vary each vertex by a value composed of its unique attributes
		out_vHueVariation += GenerateHueVariationByVertex(m_fHueVariationByVertex, vUnmodifiedNormal);

		if (ST_EFFECT_HUE_VARIATION_FADE)
			out_vHueVariation = saturate(c_fInstanceLodTransition * out_vHueVariation);
	}

	// simple parameter copying from input attribs to output interpolants
	out_fAmbientOcclusion = in_fAmbientOcclusion;
	out_vDiffuseTexCoords = in_vDiffuseTexCoords;
	out_vNormal = in_vNormal;
	out_vTangent = in_vTangent;
	out_fRenderEffectsFade = c_fInstanceLodTransition;
	out_vDetailTexCoords = in_vDetailTexCoords;
	out_vBranchSeamDiffuse = in_vBranchSeamDiffuse;
	out_vBranchSeamDetail = in_vBranchSeamDetail;
	
	// LAVA++	
	float4 vRandomColorData = SpeedTreeCalcRandomColorData( c_vInstancePos, c_vInstanceRightVector.xyz );
	
#if (ST_USED_AS_GRASS)
	[branch]
	if ( m_vLavaCustomBaseTreeParams.w > 0 )
	{
		float3 n, t;
		n  	= SampleTerrainNormal( c_vInstancePos, vRandomColorData.xy );
		t  	= float3 ( 1, 0, 0 );
		t  	= normalize( t - n * dot( n, t ) );
		
		/*
		float3 b = cross( n, t );
		float3x3 mat = BuildOrientationMatrix( t, b, n );
		out_vNormal.xyz  	= mul_float3x3_float3( mat, out_vNormal.xyz );
		vBinormal.xyz  	= mul_float3x3_float3( mat, vBinormal.xyz );
		out_vTangent.xyz  	= mul_float3x3_float3( mat, out_vTangent.xyz );
		*/

		out_vTangent.xyz = t;
		out_vNormal.xyz = n;
	}
#endif

	// final screen projection
	out_vProjection = ProjectToScreen(float4(in_vPosition, 1.0));

	// need projected position before depth for shadow maps can be computed
	out_fShadowDepth = out_vProjection.z / m_fFarClip;

	// pack each outgoing value into output interpolants; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	$VERTEX_PUT_CORE_PARAMS_INTO_PIXEL_ATTRIBS$

	// LAVA--
	// assign user interpolant values here, e.g.:
	#if (ST_USER_INTERPOLANT0)
		v2p_vUserInterpolant0 = vRandomColorData;
	#endif
	
    #if (ST_USER_INTERPOLANT1)
		v2p_vUserInterpolant1 = float4 ( in_vPosition.xyz, localPosition.z );
	#endif
	
	#if (ST_USER_INTERPOLANT2)
		#if (ST_USED_AS_GRASS)
			v2p_vUserInterpolant2 = SampleTerrainPigment( c_vInstancePos.xyz );
		#else
			v2p_vUserInterpolant2 = float4( c_fInstanceFade, 0, 0, 0 );
		#endif
	#endif

	//	#if (ST_USER_INTERPOLANT1)
	//	    v2p_vUserInterpolant1 = float4(1, 1, 1, 1);
	//	#endif
	//
	//	if defined as non-zero, ST_USER_INTERPOLANT0 through ST_USER_INTERPOLANT3 should be set in
	//	Template_UserInterpolants.fx, which will carry through both the vertex
	//	and pixel shader templates
}
