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

#include "Include_SetUp.fx"
#include "Include_Uniforms.fx"
#include "Include_Utility.fx"
#include "Include_FogAndSky.fx"
#include "Include_UserInterpolants.fx"
#include "Include_RollingWindNoise.fx"
#include "Include_Wind.fx"


///////////////////////////////////////////////////////////////////////
//	Helper function

bool IsHorizontalBillboard(float fFlag)
{
	// vertical value flag = 1.0
	// horizontal value flag = 0.0

	return (fFlag < 0.5);
}


///////////////////////////////////////////////////////////////////////
//	Vertex shader entry point
//
//	Main lighting/wind vertex shader for billboard geometry
//
//	The complexity of the inputs here are a stark contrast to the shaders
//	using the 3D tree geometry. The SRT Exporter will automatically generate
//	the input and output parameters based on effect settings and LOD values.
//	The number of combinations is too great to manage using the preprocessor.
//	For the billboard shader, it's more manageable.

#if (!ST_OPENGL)

	void main(
		//
		// vertex shader INPUT parameters by platform
		//
		#if (ST_XBOX_360)
			int in_nIndex : INDEX,
		#elif (ST_PS4)
			// instance data
			float4 in_vInstancePosAndScalar			: POP0,
			float4 in_vInstanceUpVectorAndLod1		: POP1,
			float4 in_vInstanceRightVectorAndLod2	: POP2,
			// per-vertex data
			float4 in_vAttrib0						: ST_ATTR0, // position
			float2 in_vAttrib1						: ST_ATTR1, // texcoords
		#else
			// instance stream (ST_ATTR2 through ST_ATTR4)
			float4 in_vAttrib2						: ST_ATTR2,
			float4 in_vAttrib3						: ST_ATTR3,
			float4 in_vAttrib4						: ST_ATTR4,

			// per-vertex data
			float4 in_vAttrib0						: ST_ATTR0, // position
			float2 in_vAttrib1						: ST_ATTR1, // texcoords
		#endif

		//
		// vertex shader OUTPUT parameters
		//
		out float4 v2p_vInterpolant0				: ST_VS_OUTPUT, // projection
		out float3 v2p_vInterpolant1				: TEXCOORD0,	// texcoords, alpha scalar
		out float4 v2p_vInterpolant2				: TEXCOORD1,	// tangent-space normal, fog scalar
		out float3 v2p_vBinormalInterpolant			: TEXCOORD2,	// world-space binormal
		out float3 v2p_vHueVariation				: TEXCOORD3		// hue variation

		// optional VS OUTPUT paramters, based on user config
		#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
			, out float3 v2p_vFogInterpolant		: TEXCOORD4		// dynamic fog color
		#endif
		) // end main decl

#else // !ST_OPENGL

	//
	// vertex shader INPUT parameters for GLSL
	//
	attribute float4 in_vAttrib0; // position
	attribute float3 in_vAttrib1; // texcoords

	#ifdef ST_OPENGL_NO_INSTANCING
		// GLSL only: pull instance data from beginning of constant table
		#define in_vAttrib2 g_vInstancePosAndScalar
		#define in_vAttrib3 g_vInstanceUpAndLod1
		#define in_vAttrib4 g_vInstanceRightAndLod2
	#else
		// GLSL only: pull instance float4s from table
		#define in_vAttrib2 A_avSingleUniformBlock[INSTANCE_BLOCK_POS_AND_SCALAR + gl_InstanceID * 3]
		#define in_vAttrib3 A_avSingleUniformBlock[INSTANCE_BLOCK_UP_AND_LOD1 + gl_InstanceID * 3]
		#define in_vAttrib4 A_avSingleUniformBlock[INSTANCE_BLOCK_RIGHT_AND_LOD2 + gl_InstanceID * 3]
	#endif

	//
	// vertex shader OUTPUT parameters
	//
	#define v2p_vInterpolant0 gl_Position		// projection
	varying float3 v2p_vInterpolant1;			// texcoords, alpha scalar
	varying float4 v2p_vInterpolant2;			// tangent-space normal, fog scalar
	varying float3 v2p_vBinormalInterpolant;	// world-space binormal
	varying float3 v2p_vHueVariation;			// hue variation

	// optional VS OUTPUT paramters, based on user config
	#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
		varying float3 v2p_vFogInterpolant;		// dynamic fog color
	#endif

	void main(void)

#endif

{
	//
	// platform-specific instancing data retrieval
	//
	#if (!ST_XBOX_360)
		// pull input values from incoming attributes
		float3 in_vCutoutPosition = in_vAttrib0.xyz;
		float  in_fBillboardFlag = in_vAttrib0.w;
		float2 in_vTexCoords = in_vAttrib1.xy;

		// instance input values
		#if (ST_PS4)
			float3 in_vInstancePosition = in_vInstancePosAndScalar.xyz;
			float  in_fInstanceScalar = in_vInstancePosAndScalar.w;
			float3 in_vInstanceUpVector = in_vInstanceUpVectorAndLod1.xyz;
			float3 in_vInstanceRightVector = in_vInstanceRightVectorAndLod2.xyz;
		#else
			float3 in_vInstancePosition = in_vAttrib2.xyz;
			float  in_fInstanceScalar = in_vAttrib2.w;
			float3 in_vInstanceUpVector = in_vAttrib3.xyz;
			float3 in_vInstanceRightVector = in_vAttrib4.xyz;
		#endif
	#else
		// compute the instance index
		int nNumIndicesPerInstance = g_vInstancingParams_360.x;
		int nInstanceIndex = (nIndex + 0.5) / nNumIndicesPerInstance;

		// compute the mesh index
		int nMeshIndex = nIndex - nInstanceIndex * nNumIndicesPerInstance;

		// fetch the actual mesh vertex data
		float4 vAttrib0;
		float4 vAttrib1;
		asm
		{
			vfetch vAttrib0, nMeshIndex, position0; // ST_ATTR0
			vfetch vAttrib1, nMeshIndex, texcoord0; // ST_ATTR1
		};
		float3 vCornerPos = vSlot0.xyz;
		float  fBillboardFlag = vSlot0.w;
		float2 vTexCoordsIn = vSlot1.xy;

		// fetch the instance data
		float4 in_vInstancePosAndScalar;
		float4 in_vInstanceUpVectorAndPad;
		float4 in_vInstanceRightVectorAndPad;
		asm
		{
			vfetch in_vInstancePosAndScalar, nInstanceIndex, texcoord1; // ST_ATTR9
			vfetch in_vInstanceUpVectorAndPad, nInstanceIndex, texcoord2; // ST_ATTR10
			vfetch in_vInstanceRightVectorAndPad, nInstanceIndex, texcoord3; // ST_ATTR11
		};

		float3 in_vInstancePosition = in_vInstancePosAndScalar.xyz;
		float  in_fInstanceScalar = in_vInstancePosAndScalar.w;
		float3 in_vInstanceUpVector = in_vInstanceUpVectorAndPad.xyz;
		float3 in_vInstanceRightVector = in_vInstanceRightVectorAndPad.xyz;
	#endif

	// setup all possible output parameters
	float4 out_vProjection = float4(0.0, 0.0, 0.0, 0.0);
	float2 out_vTexCoords = float2(0.0, 0.0);
	float3 out_vNormalInTangentSpace = float3(0.0, 0.0, 0.0);
	float  out_fAlphaScalar = 1.0;
	float  out_fFogScalar = 0.0;
	float3 out_vFogColor = float3(1.0, 1.0, 1.0);
	float3 out_vHueVariation = float3(0.0, 0.0, 0.0);

	// to batch efficiently, a billboard is drawn for every tree on the screen, including fully 3D trees;
	// we scale to zero area the billboards that occur when only 3D geometry should be rendered
	float fDistance = distance(m_vLodRefPosition, in_vInstancePosition);
	float fVisibilityScalar = (fDistance < m_fBillboardStartDist) ? 0.0 : in_fInstanceScalar;

	// alpha value
	out_fAlphaScalar = FadeBillboard(fDistance);

	// additional fade vars if horz billboard is present
	// determine fade value between vertical and horizontal billboards based on the angle the camera direction vector hits the horiz plane
	const float c_fHorzVertBillboardFadeOverlap	= 1.5; // smaller = less overlap, larger = more overlap
	const float c_fHorzOrientDot = abs(dot(m_vCameraDirection.xyz, in_vInstanceUpVector));
	const float c_fHorizontalFade = ST_EFFECT_HAS_HORZ_BB ? c_fHorzVertBillboardFadeOverlap * saturate((c_fHorzOrientDot - m_fBillboardHorzFade) / m_fOneMinusBillboardHorzFade) : 1.0;

	// values for these vars to be assigned either in vertical or horizontal code path
	float3 vOutVector, vRightVector, vNormal, vBinormal, vTangent;
	float3x3 mOrientation;

	// horizontal billboard path (but conditional only fires if ST_EFFECT_HAS_HORZ_BB is 1)
	if (ST_EFFECT_HAS_HORZ_BB && IsHorizontalBillboard(in_fBillboardFlag))
	{
		// lighting vectors
		vNormal = in_vInstanceUpVector;
		vTangent = in_vInstanceRightVector;
		vBinormal = normalize(cross(in_vInstanceUpVector, in_vInstanceRightVector));

		// orientation
		vOutVector = vBinormal;
		mOrientation = BuildOrientationMatrix(in_vInstanceRightVector, vOutVector, in_vInstanceUpVector);

		if ((ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP) || ST_COORDSYS_LEFT_HANDED && ST_COORDSYS_Z_UP)
			vBinormal = -vBinormal;

		// texcoords
		out_vTexCoords = in_vTexCoords;

		// modify horizontal fade based on camera angle
		out_fAlphaScalar *= saturate(c_fHorizontalFade);

	}
	// vertical billboard path
	else
	{
		// lighting vectors plus some alternate coord system adjustments
		vNormal = -m_vCameraDirection;
		vBinormal = in_vInstanceUpVector;
		vTangent = normalize(cross(vBinormal, vNormal));
		if (ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP)
			vTangent = -vTangent;

		if (ST_COORDSYS_LEFT_HANDED)
		{
			vNormal = -vNormal;
			if (ST_COORDSYS_Y_UP)
				vTangent = -vTangent;
		}

		// pick out the correct image from the billboard atlas based on orientation; BillboardSelectMapFromAtlas() is a non-trivial function call
		float3 vLookupRight = in_vInstanceRightVector;
		float3 vLookupUp = in_vInstanceUpVector;
		float3 vLookupOut = ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP ? -normalize(cross(vLookupUp, vLookupRight)) : normalize(cross(vLookupUp, vLookupRight));
		out_vTexCoords = BillboardSelectMapFromAtlas(in_vTexCoords,
													 vLookupRight,
													 vLookupOut,
													 vLookupUp,
													 m_vCameraDirection.xyz);

		// orientation; includes spinning to face the camera but also properly aligning its up vector
		vOutVector = normalize(ST_COORDSYS_Y_UP ? float3(m_vCameraDirection.x, 0.0, m_vCameraDirection.z) : float3(m_vCameraDirection.xy, 0.0));
		vRightVector = normalize(-cross(in_vInstanceUpVector, vOutVector));
		mOrientation = BuildBillboardOrientationMatrix(vRightVector, vOutVector, in_vInstanceUpVector);

		// if a horizontal billboard is present, modify the vertical billboard's alpha fade value
		out_fAlphaScalar *= ST_EFFECT_HAS_HORZ_BB ? saturate(c_fHorzVertBillboardFadeOverlap - c_fHorizontalFade) : 1.0;
	}

	// modify position with orientation/camera-facing matrix
	float3 vPosition = mul_float3x3_float3(mOrientation, in_vCutoutPosition);

	// optional wind effect
	if (ST_WIND_LOD_BILLBOARD_GLOBAL)
	{
		ST_MULTIPASS_STABILIZE
		{
			// wind effect, only in the plane of the camera so the billboards don't z-fight due to wind
			float3 vWindOffset = WindGlobalBehavior(vPosition,
													in_vInstancePosition,
													float4(m_sGlobal.m_fTime, m_sGlobal.m_fDistance, m_sGlobal.m_fHeight, m_sGlobal.m_fHeightExponent),//g_vWindGlobal,
													float4(m_vDirection, m_fStrength),//g_vWindVector,
													float3(m_sGlobal.m_fAdherence, m_sBranch1.m_fDirectionAdherence, m_sBranch2.m_fDirectionAdherence),//g_vWindBranchAdherences.xyz,
													false) - vPosition;
			vPosition += LimitToCameraPlane(vWindOffset);
		}
	}

	// scale corner
	vPosition *= fVisibilityScalar;

	// add corner to instance's position in world space
	vPosition += in_vInstancePosition;

	// scale alpha value based on global alpha scalar
	out_fAlphaScalar *= m_fAlphaScalar;

	// fog
	if (ST_FOG_COLOR == ST_FOG_COLOR_CONSTANT)
		out_fFogScalar = FogComputeLerp(fDistance);
	else if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
	{
		out_fFogScalar = FogComputeLerp(fDistance);
		out_vFogColor = FogComputeVertexColor(vPosition);
	}
	else
		out_vFogColor = float3(1.0, 1.0, 1.0);

	// hue variation
	if (ST_EFFECT_HUE_VARIATION)
		out_vHueVariation = GenerateHueVariationByPos(m_fHueVariationByPos, in_vInstanceRightVector);

	// setup normal mapping in pixel shader
	out_vNormalInTangentSpace.x = dot(m_vDirLight.m_vDir.xyz, vTangent);
	out_vNormalInTangentSpace.y = dot(m_vDirLight.m_vDir.xyz, vBinormal);
	out_vNormalInTangentSpace.z = dot(m_vDirLight.m_vDir.xyz, vNormal);

	// final screen projection
	out_vProjection = ProjectToScreen(float4(vPosition, 1.0));

	// pack values into output attributes
	v2p_vInterpolant0 = out_vProjection;
	v2p_vInterpolant1 = float3(out_vTexCoords, out_fAlphaScalar);
	v2p_vInterpolant2 = float4(out_vNormalInTangentSpace, out_fFogScalar);
	v2p_vBinormalInterpolant = in_vInstanceUpVector;
	v2p_vHueVariation = out_vHueVariation;
	#if (ST_FOG_COLOR == ST_FOG_COLOR_DYNAMIC)
		v2p_vFogInterpolant = out_vFogColor;
	#endif
}

