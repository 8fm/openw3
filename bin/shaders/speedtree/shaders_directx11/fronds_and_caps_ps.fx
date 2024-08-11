///////////////////////////////////////////////////////////////////////  
//  fronds_and_caps_ps.fx
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

#define ST_OPENGL false
#define ST_PIXEL_SHADER
#define ST_COORDSYS_Z_UP true
#define ST_COORDSYS_Y_UP false
#define ST_COORDSYS_LEFT_HANDED false
#define ST_COORDSYS_RIGHT_HANDED true

// user preference defines (render settings)
#define ST_ALPHA_TEST_NOISE false
#define ST_DEFERRED_A2C_ENABLED false
#define ST_FOG_CURVE ST_FOG_CURVE_DISABLED
#define ST_FOG_COLOR ST_FOG_COLOR_CONSTANT
#define ST_SHADOWS_ENABLED false
#define ST_SHADOWS_SMOOTH false
#define ST_SHADOWS_NUM_MAPS 2
#define ST_BRANCHES_PRESENT false
#define ST_ONLY_BRANCHES_PRESENT false
#define ST_FRONDS_PRESENT true
#define ST_ONLY_FRONDS_PRESENT true
#define ST_LEAVES_PRESENT false
#define ST_ONLY_LEAVES_PRESENT false
#define ST_FACING_LEAVES_PRESENT false
#define ST_ONLY_FACING_LEAVES_PRESENT false
#define ST_RIGID_MESHES_PRESENT false
#define ST_ONLY_RIGID_MESHES_PRESENT false
#define PIXEL_PROPERTY_PROJECTION_PRESENT true
#define PIXEL_PROPERTY_FOGSCALAR_PRESENT false
#define PIXEL_PROPERTY_FOGCOLOR_PRESENT false
#define PIXEL_PROPERTY_DIFFUSETEXCOORDS_PRESENT true
#define PIXEL_PROPERTY_DETAILTEXCOORDS_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXLIGHTINGCOLOR_PRESENT false
#define PIXEL_PROPERTY_NORMALINTANGENTSPACE_PRESENT false
#define PIXEL_PROPERTY_NORMAL_PRESENT true
#define PIXEL_PROPERTY_BINORMAL_PRESENT false
#define PIXEL_PROPERTY_TANGENT_PRESENT true
#define PIXEL_PROPERTY_SPECULARHALFVECTOR_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXSPECULARDOT_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXAMBIENTCONTRAST_PRESENT false
#define PIXEL_PROPERTY_FADETOBILLBOARD_PRESENT true
#define PIXEL_PROPERTY_TRANSMISSIONFACTOR_PRESENT false
#define PIXEL_PROPERTY_RENDEREFFECTSFADE_PRESENT true
#define PIXEL_PROPERTY_AMBIENTOCCLUSION_PRESENT true
#define PIXEL_PROPERTY_BRANCHSEAMDIFFUSE_PRESENT false
#define PIXEL_PROPERTY_BRANCHSEAMDETAIL_PRESENT false
#define PIXEL_PROPERTY_SHADOWDEPTH_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION0_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION1_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION2_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION3_PRESENT false
#define PIXEL_PROPERTY_HUEVARIATION_PRESENT true
#define ST_USED_AS_GRASS false
#define ST_MULTIPASS_ACTIVE false

// effect LOD macros
#define ST_EFFECT_FORWARD_LIGHTING_PER_VERTEX         (m_avEffectConfigFlags[0].x > 0.5)
#define ST_EFFECT_FORWARD_LIGHTING_PER_PIXEL          (m_avEffectConfigFlags[0].y > 0.5)
#define ST_EFFECT_FORWARD_LIGHTING_TRANSITION         (m_avEffectConfigFlags[0].z > 0.5)
#define ST_EFFECT_AMBIENT_OCCLUSION                   (m_avEffectConfigFlags[0].w > 0.5)
#define ST_EFFECT_AMBIENT_CONTRAST                    (m_avEffectConfigFlags[1].x > 0.5)
#define ST_EFFECT_AMBIENT_CONTRAST_FADE               (m_avEffectConfigFlags[1].x > 1.5)
#define ST_EFFECT_DETAIL_LAYER                        (m_avEffectConfigFlags[1].y > 0.5)
#define ST_EFFECT_DETAIL_LAYER_FADE                   (m_avEffectConfigFlags[1].y > 1.5)
#define ST_EFFECT_DETAIL_NORMAL_LAYER                 (m_avEffectConfigFlags[1].z > 0.5)
#define ST_EFFECT_SPECULAR                            (m_avEffectConfigFlags[1].w > 0.5)
#define ST_EFFECT_SPECULAR_FADE                       (m_avEffectConfigFlags[1].w > 1.5)
#define ST_EFFECT_TRANSMISSION                        (m_avEffectConfigFlags[2].x > 0.5)
#define ST_EFFECT_TRANSMISSION_FADE                   (m_avEffectConfigFlags[2].x > 1.5)
#define ST_EFFECT_BRANCH_SEAM_SMOOTHING               (m_avEffectConfigFlags[2].y > 0.5)
#define ST_EFFECT_BRANCH_SEAM_SMOOTHING_FADE          (m_avEffectConfigFlags[2].y > 1.5)
#define ST_EFFECT_SMOOTH_LOD                          (m_avEffectConfigFlags[2].z > 0.5)
#define ST_EFFECT_FADE_TO_BILLBOARD                   (m_avEffectConfigFlags[2].w > 0.5)
#define ST_EFFECT_HAS_HORZ_BB                         (m_avEffectConfigFlags[3].x > 0.5)
#define ST_EFFECT_BACKFACE_CULLING                    (m_avEffectConfigFlags[3].y > 0.5)
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING              (m_avEffectConfigFlags[3].z > 0.5)
#define ST_EFFECT_AMBIENT_IMAGE_LIGHTING_FADE         (m_avEffectConfigFlags[3].z > 1.5)
#define ST_EFFECT_HUE_VARIATION                       (m_avEffectConfigFlags[3].w > 0.5)
#define ST_EFFECT_HUE_VARIATION_FADE                  (m_avEffectConfigFlags[3].w > 1.5)
#define ST_EFFECT_SHADOW_SMOOTHING                    (m_avEffectConfigFlags[4].x > 0.5)
#define ST_EFFECT_DIFFUSE_MAP_OPAQUE                  (m_avEffectConfigFlags[4].y > 0.5)

// wind config macros
#define ST_WIND_IS_ACTIVE 1
#define ST_WIND_EFFECT_GLOBAL_WIND                    (m_avWindConfigFlags[0].x > 0.5)
#define ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE          (m_avWindConfigFlags[0].y > 0.5)
#define ST_WIND_EFFECT_BRANCH_SIMPLE_1                (m_avWindConfigFlags[0].z > 0.5)
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1           (m_avWindConfigFlags[0].w > 0.5)
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1     (m_avWindConfigFlags[1].x > 0.5)
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_1            (m_avWindConfigFlags[1].y > 0.5)
#define ST_WIND_EFFECT_BRANCH_WHIP_1                  (m_avWindConfigFlags[1].z > 0.5)
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1           (m_avWindConfigFlags[1].w > 0.5)
#define ST_WIND_EFFECT_BRANCH_SIMPLE_2                (m_avWindConfigFlags[2].x > 0.5)
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2           (m_avWindConfigFlags[2].y > 0.5)
#define ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2     (m_avWindConfigFlags[2].z > 0.5)
#define ST_WIND_EFFECT_BRANCH_TURBULENCE_2            (m_avWindConfigFlags[2].w > 0.5)
#define ST_WIND_EFFECT_BRANCH_WHIP_2                  (m_avWindConfigFlags[3].x > 0.5)
#define ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2           (m_avWindConfigFlags[3].y > 0.5)
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1    (m_avWindConfigFlags[3].z > 0.5)
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1         (m_avWindConfigFlags[3].w > 0.5)
#define ST_WIND_EFFECT_LEAF_TUMBLE_1                  (m_avWindConfigFlags[4].x > 0.5)
#define ST_WIND_EFFECT_LEAF_TWITCH_1                  (m_avWindConfigFlags[4].y > 0.5)
#define ST_WIND_EFFECT_LEAF_OCCLUSION_1               (m_avWindConfigFlags[4].z > 0.5)
#define ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2    (m_avWindConfigFlags[4].w > 0.5)
#define ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2         (m_avWindConfigFlags[5].x > 0.5)
#define ST_WIND_EFFECT_LEAF_TUMBLE_2                  (m_avWindConfigFlags[5].y > 0.5)
#define ST_WIND_EFFECT_LEAF_TWITCH_2                  (m_avWindConfigFlags[5].z > 0.5)
#define ST_WIND_EFFECT_LEAF_OCCLUSION_2               (m_avWindConfigFlags[5].w > 0.5)
#define ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED         (m_avWindConfigFlags[6].x > 0.5)
#define ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED         (m_avWindConfigFlags[6].y > 0.5)
#define ST_WIND_EFFECT_FROND_RIPPLE_ADJUST_LIGHTING   (m_avWindConfigFlags[6].z > 0.5)
#define ST_WIND_EFFECT_ROLLING                        (m_avWindConfigFlags[6].w > 0.5)

// wind LOD macros
#define ST_WIND_LOD_GLOBAL                            (m_avWindLodFlags[0].x > 0.5)
#define ST_WIND_LOD_BRANCH                            (m_avWindLodFlags[0].y > 0.5)
#define ST_WIND_LOD_FULL                              (m_avWindLodFlags[0].z > 0.5)
#define ST_WIND_LOD_NONE_X_GLOBAL                     (m_avWindLodFlags[0].w > 0.5)
#define ST_WIND_LOD_NONE_X_BRANCH                     (m_avWindLodFlags[1].x > 0.5)
#define ST_WIND_LOD_NONE_X_FULL                       (m_avWindLodFlags[1].y > 0.5)
#define ST_WIND_LOD_GLOBAL_X_BRANCH                   (m_avWindLodFlags[1].z > 0.5)
#define ST_WIND_LOD_GLOBAL_X_FULL                     (m_avWindLodFlags[1].w > 0.5)
#define ST_WIND_LOD_BRANCH_X_FULL                     (m_avWindLodFlags[2].x > 0.5)
#define ST_WIND_LOD_NONE                              (m_avWindLodFlags[2].y > 0.5)
#define ST_WIND_LOD_ROLLING_FADE                      (m_avWindLodFlags[2].z > 0.5)
#define ST_WIND_BRANCH_WIND_ACTIVE                    (m_avWindLodFlags[2].w > 0.5)
#define ST_WIND_LOD_BILLBOARD_GLOBAL                  true


///////////////////////////////////////////////////////////////////////
//	Include files

#define SPEEDTREE_PIXEL_SHADER
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

#ifndef ST_INCLUDE_SETUP
#define ST_INCLUDE_SETUP


///////////////////////////////////////////////////////////////////////
//	Include files

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

#ifndef ST_INCLUDE_SYMBOLS
#define ST_INCLUDE_SYMBOLS

#define true					1
#define false					0

// should match EFogCurve in Core.h
#define ST_FOG_CURVE_DISABLED	0
#define ST_FOG_CURVE_LINEAR		1
#define ST_FOG_CURVE_EXP		2
#define ST_FOG_CURVE_EXP2		3
#define ST_FOG_CURVE_USER		4

// should match EFogColorType in Core.h
#define ST_FOG_COLOR_CONSTANT	0
#define ST_FOG_COLOR_DYNAMIC	1

#endif // ST_INCLUDE_SYMBOLS


///////////////////////////////////////////////////////////////////////
//	Set up default values for configuration/per-platform macros if not previously set

// coordinate system
#if !defined(ST_COORDSYS_Z_UP) && !defined(ST_COORDSYS_Y_UP)
	#define ST_COORDSYS_Z_UP true
	#define ST_COORDSYS_Y_UP false
#endif
#if !defined(ST_COORDSYS_RIGHT_HANDED) && !defined(ST_COORDSYS_LEFT_HANDED)
	#define ST_COORDSYS_RIGHT_HANDED true
	#define ST_COORDSYS_LEFT_HANDED false
#endif

// platforms
#ifndef ST_OPENGL
	#define ST_OPENGL false
#endif
#ifndef ST_DIRECTX9
	#define ST_DIRECTX9 false
#endif
#ifndef ST_XBOX_360
	#define ST_XBOX_360 false
#endif
#ifndef ST_DIRECTX11
	#define ST_DIRECTX11 false
#endif
#ifndef ST_XBOX_ONE
	#define ST_XBOX_ONE false
#endif
#ifndef ST_PS3
	#define ST_PS3 false
#endif
#ifndef ST_PS4
	#define ST_PS4 false
#endif


///////////////////////////////////////////////////////////////////////
//	Global compiler settings

#if (!ST_OPENGL)
	#pragma pack_matrix(row_major)
#else
	// todo: revisit this with new 7.0 constant buffer approach
	#extension GL_ARB_explicit_attrib_location : enable
#endif


///////////////////////////////////////////////////////////////////////  
//	Platform-adapting macros

// specifying outputs for both vertex and pixel shaders
#if (ST_DIRECTX9) || (ST_PS3)
	#define ST_VS_OUTPUT		POSITION
	#define ST_PS_OUTPUT		COLOR
	#define ST_RENDER_TARGET0	COLOR0
	#define ST_RENDER_TARGET1	COLOR1
	#define ST_RENDER_TARGET2	COLOR2
	#define ST_RENDER_TARGET3	COLOR3
#elif (ST_PS4)
	#define ST_VS_OUTPUT		S_POSITION
	#define ST_PS_OUTPUT		S_TARGET_OUTPUT
	#define ST_RENDER_TARGET0	S_TARGET_OUTPUT0
	#define ST_RENDER_TARGET1	S_TARGET_OUTPUT1
	#define ST_RENDER_TARGET2	S_TARGET_OUTPUT2
	#define ST_RENDER_TARGET3	S_TARGET_OUTPUT3
#elif (ST_DIRECTX11)
	#define ST_VS_OUTPUT		SV_POSITION
	#define ST_PS_OUTPUT		SV_TARGET
	#define ST_RENDER_TARGET0	SV_Target0
	#define ST_RENDER_TARGET1	SV_Target1
	#define ST_RENDER_TARGET2	SV_Target2
	#define ST_RENDER_TARGET3	SV_Target3
#endif

// specifying pixel color return
#if (ST_OPENGL)
	#if (__VERSION__ >= 150)
		out vec4 vOutFragColor;
		#define ST_PIXEL_COLOR_RETURN(color) vOutFragColor = color; return
	#else
		#define ST_PIXEL_COLOR_RETURN(color) gl_FragColor = color; return
	#endif
#else
	#define ST_PIXEL_COLOR_RETURN(color) return color
#endif

#define ST_UNREF_PARAM(x)		(x) = (x)
// todo
#define ST_TRANSPARENCY_ACTIVE	(!ST_EFFECT_DIFFUSE_MAP_OPAQUE || ST_EFFECT_FADE_TO_BILLBOARD || ST_USED_AS_GRASS)
#define ST_ALPHA_KILL_THRESHOLD	0.1


///////////////////////////////////////////////////////////////////////  
//  Semantic Bindings
//
//	Differences between NVIDIA & ATI hardware, as well as Cg and HLSL compilers
//	call for this abstract of vertex attribute semantics

#if (ST_DIRECTX9) || (ST_DIRECTX11) || (ST_XBOX_360)
	#define ST_ATTR0	POSITION
	#define ST_ATTR1	TEXCOORD0
	#define ST_ATTR2	TEXCOORD1
	#define ST_ATTR3	TEXCOORD2
	#define ST_ATTR4	TEXCOORD3
	#define ST_ATTR5	TEXCOORD4
	#define ST_ATTR6	TEXCOORD5
	#define ST_ATTR7	TEXCOORD6
	#define ST_ATTR8	TEXCOORD7
	#define ST_ATTR9	TEXCOORD8
	#define ST_ATTR10	TEXCOORD9
	#define ST_ATTR11	TEXCOORD10
	#define ST_ATTR12	TEXCOORD11
	#define ST_ATTR13	TEXCOORD12
	#define ST_ATTR14	TEXCOORD13
	#define ST_ATTR15	TEXCOORD14
#endif


///////////////////////////////////////////////////////////////////////  
//  Xbox-specific command
//
//	With the way the 360 shader compiler geneates code & optimizes, wind 
//	compuations on 360 don't quite match between depth-only and lighting
//	passes.

#if (ST_XBOX_360) && (ST_MULTIPASS_ACTIVE)
	// ST_MULTIPASS_ACTIVE is #defined by the SRT Exporter for all shaders when
	// depth-only prepass is selected in the Compiler app
	#define ST_MULTIPASS_STABILIZE [isolate]
#else
	#define ST_MULTIPASS_STABILIZE
#endif


///////////////////////////////////////////////////////////////////////  
//  Getting GLSL syntax more in line with HLSL/Cg the rest of our platforms use

#if (ST_OPENGL)

	///////////////////////////////////////////////////////////////////////  
	//  Synchronize vector types
	
	#define float2		vec2
	#define float3		vec3
	#define float4		vec4
	#define float3x3	mat3
	#define float4x4	mat4

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float saturate(float fValue)
	{
		return clamp(fValue, 0.0, 1.0);
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float2 saturate(float2 vValue)
	{
		return clamp(vValue, float2(0.0, 0.0), float2(1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float3 saturate(float3 vValue)
	{
		return clamp(vValue, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  saturate (clamp in GLSL)

	float4 saturate(float4 vValue)
	{
		return clamp(vValue, float4(0.0, 0.0, 0.0, 0.0), float4(1.0, 1.0, 1.0, 1.0));
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  lerp() is mix() in GLSL

	#define lerp mix
	#define clip discard
	#define frac fract
	#define fmod mod
	#define wind_cross(a, b) cross((b), (a))
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4x4_float4

	float4 mul_float4x4_float4(float4x4 mMatrix, float4 vVector)
	{
		return mMatrix * vVector;
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4_float4x4

	float4 mul_float4_float4x4(float4 vVector, float4x4 mMatrix)
	{
		return vVector * mMatrix;
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3x3

	float3x3 mul_float3x3_float3x3(float3x3 mMatrixA, float3x3 mMatrixB)
	{
		return mMatrixA * mMatrixB;
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3

	float3 mul_float3x3_float3(float3x3 mMatrix, float3 vVector)
	{
		return mMatrix * vVector;
	}
	

	///////////////////////////////////////////////////////////////////////  
	//  const doesn't mean the same thing in GLSL

	#define const
	#define static
	
#else

	///////////////////////////////////////////////////////////////////////  
	//  mul_float4x4_float4

	float4 mul_float4x4_float4(float4x4 mMatrix, float4 vVector)
	{
		return mul(mMatrix, vVector);
	}
	
	
	///////////////////////////////////////////////////////////////////////  
	//  mul_float4_float4x4

	float4 mul_float4_float4x4(float4 vVector, float4x4 mMatrix)
	{
		return mul(vVector, mMatrix);
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3x3

	float3x3 mul_float3x3_float3x3(float3x3 mMatrixA, float3x3 mMatrixB)
	{
		return mul(mMatrixA, mMatrixB);
	}


	///////////////////////////////////////////////////////////////////////  
	//  mul_float3x3_float3

	float3 mul_float3x3_float3(float3x3 mMatrix, float3 vVector)
	{
		return mul(mMatrix, vVector);
	}

	
	///////////////////////////////////////////////////////////////////////  
	//  cross()'s parameters are backwards in GLSL

	#define wind_cross(a, b) cross((a), (b))

#endif


///////////////////////////////////////////////////////////////////////  
//  Global constants
//
//	These correspond to the EGeometryTypeHint enumeration defined in Core.h

#define ST_GEOMETRY_TYPE_HINT_BRANCHES			0.0
#define ST_GEOMETRY_TYPE_HINT_FRONDS			1.0
#define ST_GEOMETRY_TYPE_HINT_LEAVES			2.0
#define ST_GEOMETRY_TYPE_HINT_FACING_LEAVES	3.0
#define ST_GEOMETRY_TYPE_HINT_RIGIDMESHES		4.0

#endif // ST_INCLUDE_SETUP
///////////////////////////////////////////////////////////////////////
//  Include_Uniforms.fx (** generated by SpeedTree utility **)
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
//      http://www.idvinc.com

// shader constants for DirectX11
#if (ST_DIRECTX11) || (ST_PS4)

	#if (ST_PS4)
		#define cbuffer ConstantBuffer
	#endif

	// todo: formalize
	#define st_float32	float
	#define Vec2		float2
	#define Vec3		float3
	#define Vec4		float4
	#define Mat4x4		float4x4

	struct SDirLight
	{
		Vec3		m_vAmbient;
		Vec3		m_vDiffuse;
		Vec3		m_vSpecular;
		Vec3		m_vTransmission;
		Vec3		m_vDir;
	};

	struct SShadows
	{
		Vec4			m_vShadowMapRanges;
		st_float32		m_fFadeStartPercent;			// was m_vShadowParams.x
		st_float32		m_fFadeInverseDistance;			// todo: was m_vShadowParams.z; remove this once shader has been updated
		st_float32		m_fTerrainAmbientOcclusion;		// todo: was m_vShadowParams.y

		Vec4			m_avShadowSmoothingTable[3];
		st_float32		m_fShadowMapWritingActive;		// 1.0 if on, 0.0 if off
		Vec2			m_vShadowMapTexelOffset;
		Mat4x4			m_amLightModelViewProjs[4];
	};

	cbuffer FrameCB : register(b0)
	{
		// projections
		Mat4x4			m_mModelViewProj3d;
		Mat4x4			m_mProjectionInverse3d;
		Mat4x4			m_mModelViewProj2d;
		Mat4x4			m_mCameraFacingMatrix;

		// camera
		Vec3			m_vCameraPosition;
		Vec3			m_vCameraDirection;
		Vec3			m_vLodRefPosition;
		Vec2			m_vViewport;
		Vec2			m_vViewportInverse; // (1.0 / m_vViewport)
		st_float32		m_fFarClip;

		// simple lighting
		SDirLight		m_vDirLight;

		// shadows
		SShadows		m_sShadows;

		// misc
		st_float32		m_fWallTime;
	};

	cbuffer BaseTreeCB : register(b1) // set once per base tree (SRT file)
	{
		// 3D LOD values
		st_float32		m_f3dLodHighDetailDist;			// distance at which LOD transition from highest 3D level begins
		st_float32		m_f3dLodRange;					// 3d_low_detail_dist - 3d_high_detail_dist
		st_float32		m_f3dGrassStartDist;			// distance at which grass begins to fade in and 3D fade out
		st_float32		m_f3dGrassRange;				// length in world units that transition occurs over

		// billboard LOD values
		st_float32		m_fBillboardHorzFade;			// x = [0-1] range; closer to 0.0 keeps vertical around longer, closer to 1.0 keeps horizontal longer
		st_float32		m_fOneMinusBillboardHorzFade;
		st_float32		m_fBillboardStartDist;			// distance at which the billboard begins to fade in and 3D fade out
		st_float32		m_fBillboardRange;				// length in world units that transition occurs over

		// hue variation
		st_float32		m_fHueVariationByPos;
		st_float32		m_fHueVariationByVertex;
		Vec3			m_vHueVariationColor;

		// ambient image
		st_float32		m_fAmbientImageScalar;

		// billboards
		st_float32		m_fNumBillboards;
		st_float32		m_fRadiansPerImage;			// value computed on CPU to reduce GPU load
		Vec4			m_avBillboardTexCoords[24];
	};

	cbuffer MaterialCB : register(b2) // multiple materials per base tree
	{
		Vec3			m_vAmbientColor;
		Vec3			m_vDiffuseColor;
		Vec3			m_vSpecularColor;
		Vec3			m_vTransmissionColor;
		st_float32		m_fShininess;
		st_float32		m_fBranchSeamWeight;
		st_float32		m_fOneMinusAmbientContrastFactor;
		st_float32		m_fTransmissionShadowBrightness;
		st_float32		m_fTransmissionViewDependency;
		st_float32		m_fAlphaScalar;

		// effect settings (specular, transmission, etc)
		Vec4			m_avEffectConfigFlags[5];

		// wind settings (from the Modeler)
		Vec4			m_avWindConfigFlags[7];

		// wind config flags (what's on/off for this tree as set by the Modeler)
		Vec4			m_avWindLodFlags[3];
	};

	struct SGlobal
	{
		st_float32		m_fTime;
		st_float32		m_fDistance;
		st_float32		m_fHeight;
		st_float32		m_fHeightExponent;
		st_float32		m_fAdherence;
	};

	struct SBranchWind
	{
		st_float32		m_fTime;
		st_float32		m_fDistance;
		st_float32		m_fTwitch;
		st_float32		m_fTwitchFreqScale;
		st_float32		m_fWhip;
		st_float32		m_fDirectionAdherence;
		st_float32		m_fTurbulence;
	};

	struct SLeaf
	{
		st_float32		m_fRippleTime;
		st_float32		m_fRippleDistance;
		st_float32		m_fLeewardScalar;
		st_float32		m_fTumbleTime;
		st_float32		m_fTumbleFlip;
		st_float32		m_fTumbleTwist;
		st_float32		m_fTumbleDirectionAdherence;
		st_float32		m_fTwitchThrow;
		st_float32		m_fTwitchSharpness;
		st_float32		m_fTwitchTime;
	};

	struct SFrondRipple
	{
		st_float32		m_fTime;
		st_float32		m_fDistance;
		st_float32		m_fTile;
		st_float32		m_fLightingScalar;
	};

	struct SRolling
	{
		st_float32		m_fBranchFieldMin;
		st_float32		m_fBranchLightingAdjust;
		st_float32		m_fBranchVerticalOffset;
		st_float32		m_fLeafRippleMin;
		st_float32		m_fLeafTumbleMin;
		st_float32		m_fNoisePeriod;
		st_float32		m_fNoiseSize;
		st_float32		m_fNoiseTurbulence;
		st_float32		m_fNoiseTwist;
		Vec2			m_vOffset;
	};

	cbuffer WindDynamicsCB : register(b3)
	{
		Vec3			m_vDirection;
		st_float32		m_fStrength;
		Vec3			m_vAnchor;
		SGlobal			m_sGlobal;

		SBranchWind		m_sBranch1;
		SBranchWind		m_sBranch2;
		SLeaf			m_sLeaf1;
		SLeaf			m_sLeaf2;
		SFrondRipple	m_sFrondRipple;

		SRolling		m_sRolling;
	};

	// these values are used by the SpeedTree reference app example sky and fog systems; will be replaced by client's
	cbuffer FogAndSkyCB : register(b4)
	{
		// fog
		Vec3			m_vFogColor;
		st_float32		m_fFogDensity;
		st_float32		m_fFogEndDist;
		st_float32		m_fFogSpan;						// fom_end_dist - fom_start_dist

		// sky
		Vec3			m_vSkyColor;

		// sun
		Vec3			m_vSunColor;
		st_float32		m_fSunSize;
		st_float32		m_fSunSpreadExponent;
	};

	// these values are used by the SpeedTree reference app example terrain system; will be replaced by client's
	cbuffer TerrainCB : register(b5)
	{
		Vec3			m_vSplatTiles;
		st_float32		m_fNormalMapBlueScalar;
	};

	// these values are used by the SpeedTree reference app example bloom system; will be replaced by client's
	cbuffer BloomCB : register(b6)
	{
		st_float32		m_fBrightPass;
		st_float32		m_fDownsample;
		st_float32		m_fDownsampleLoopStart;
		st_float32		m_fDownsampleLoopEnd;
		st_float32		m_fBlurKernelSize;
		st_float32		m_fBlurKernelStep;
		st_float32		m_fBlurPixelOffset;
		st_float32		m_fBloomEffectScalar;
		st_float32		m_fHighPassFloor;
		st_float32		m_fSkyBleed;
		st_float32		m_fFinalMainScalar;
	};

	// LAVA+
	cbuffer ConstBuffer7 : register(b7)
	{
		float4 g_vRandomColorParams;
		float4 g_vGrassParams0;
		float4 g_vGrassParams1;
		float4 g_vLavaReserved0;
		float4 g_vLavaReserved1;
		float4 g_vLavaReserved2;
	};
	// LAVA-

	struct SInstanceData
	{
		float4		m_vPosAndScalar;       // [xyz] = position, [w] = instance size scalar
		float4		m_vUpVectorAndLod0;    // [xyz] = normalized up dir, [w] (Lod Transition) not used by billboards & grass
		float4		m_vRightVectorAndLod1; // [xyz] = normalized right dir, [w] (Lod Value) not used by billboards & grass
	};

	#undef cbuffer

#endif

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

#ifndef ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS
#define ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS


///////////////////////////////////////////////////////////////////////
//  Common texture and sampler setup defines

#if (ST_DIRECTX11)

	shared SamplerState samStandard : register(s0);
	shared SamplerComparisonState samShadowMapComparison : register(s1);
	shared SamplerState samPoint : register(s2);
	shared SamplerState samLinearClamp : register(s3);

	#define DeclareTexture(name, reg) 								Texture2D name : register(t##reg);
	#define DeclareTextureMS(name, reg, num_samples)				Texture2DMS<float4, num_samples> name : register(t##reg);
	#define DeclareShadowMapTexture(name, reg) 						Texture2D name : register(t##reg);

	#define SampleTexture(name, texcoord) 							name.Sample(samStandard, texcoord)
	// todo
	#define SampleTextureSpecial(name, texcoord)					name.SampleLevel(samStandard, texcoord, 0)
	#define SampleTextureLinearClamp(name, texcoord)				name.SampleLevel(samLinearClamp, texcoord, 0)
	#define SampleTextureLod(name, texcoord, lod) 			 		name.SampleLevel(samPoint, texcoord, lod)
	#define SampleTextureMS(name, texcoord, sample) 				name.Load(texcoord, sample)
	#define SampleTextureCompare(name, sampler, texcoord)			((name).SampleCmpLevelZero(samShadowMapComparison, (texcoord).xy, (texcoord).z))
	
#elif (ST_PS4)

	SamplerState samStandard : register(s0);
	SamplerComparisonState samShadowMapComparison : register(s1);
	SamplerState samPoint : register(s2);
	SamplerState samLinearClamp : register(s3);

	#define DeclareTexture(name, reg) 								Texture2D name : register(t##reg);
	#define DeclareTextureMS(name, reg, num_samples)				Texture2D name : register(t##reg);
	#define DeclareShadowMapTexture(name, reg) 						Texture2D name : register(t##reg);

	#define SampleTexture(name, texcoord) 							name.Sample(samStandard, texcoord)
	
	#define SampleTextureSpecial(name, texcoord)					name.SampleLOD(samStandard, texcoord, 0) // todo
	#define SampleTextureLinearClamp(name, texcoord)				name.SampleLOD(samLinearClamp, texcoord, 0) // todo
	#define SampleTextureLod(name, texcoord, lod) 			 		name.SampleLOD(samPoint, texcoord, lod)
	#define SampleTextureMS(name, texcoord, sample)					name.Sample(samStandard, texcoord)
	#define SampleTextureCompare(name, sampler, texcoord) 			((name).SampleCmpLOD0(samShadowMapComparison, (texcoord).xy, (texcoord).z))

#elif (ST_OPENGL)

	#define DeclareTexture(name, sam_reg) 							uniform sampler2D name
	#define DeclareShadowMapTexture(name, sam_reg)					uniform sampler2DShadow name

	#if (__VERSION__ >= 150)
		#define SampleTexture(name, texcoord) 						texture(name, texcoord)
		#define SampleTextureLod(name, texcoord, lod) 			 	textureLod(name, texcoord, lod)
		#define SampleTextureLinearClamp(name, texcoord)			texture(name, texcoord)
		#define SampleTextureCompare(texture, sampler, texcoord) 	textureProj(sampler, texcoord)
	#else
		#define SampleTexture(name, texcoord) 						texture2D(name, texcoord)
		#define SampleTextureLinearClamp(name, texcoord)			texture2D(name, texcoord)
		#define SampleTextureLod(name, texcoord, lod) 			 	texture2D(name, texcoord)
		#define SampleTextureCompare(texture, sampler, texcoord) 	shadow2DProj(sampler, texcoord).r
	#endif
	
#else

	#if (ST_DIRECTX9) || (ST_PS3)
		#define DeclareTexture(name, reg) 							texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
		#define DeclareTextureMS(name, reg, num_samples)			texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
		#define DeclareShadowMapTexture(name, reg) 					texture name; sampler2D sam##name : register(s##reg) = sampler_state { Texture = <name>; }
	#endif
	
	#define SampleTexture(name, texcoord) 							tex2D(sam##name, texcoord)
	#define SampleTextureLod(name, texcoord, lod) 					tex2Dlod(sam##name, float4((texcoord).xy, 0.0, lod))
	#define SampleTextureCompare(name, sampler, texcoord) 			tex2Dproj(sampler, texcoord).r
	#define SampleTextureLinearClamp(name, texcoord)				tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	// todo
	#define SampleTextureSpecial(name, texcoord)					tex2Dlod(sam##name, float4((texcoord).xy, 0.0, 0))
	#define SampleTextureMS(name, texcoord, sample) 				tex2D(sam##name, texcoord)

#endif

#endif // ST_INCLUDE_SAMPLERS_AND_TEXTURE_MACROS

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

#ifndef ST_INCLUDE_UTILITY
#define ST_INCLUDE_UTILITY


///////////////////////////////////////////////////////////////////////
//  Constants

#define ST_PI		3.14159265358979323846
#define ST_TWO_PI	6.283185307179586476925


///////////////////////////////////////////////////////////////////////
//  ConvertToStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from an alternate coordinate system to SpeedTree's right-
//	handed Z-up system if an alternate has been selected. The supported alternate
//	coordinate systems are right-handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertToStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, -vCoord.z, vCoord.y);
}


///////////////////////////////////////////////////////////////////////
//  ConvertFromStdCoordSys
//
//	By default, SpeedTree operates in a right-handed, Z-up coordinate system. This
//	function will convert from SpeedTree's standard system to an alternate coord system
//	if one has been selected. The supported alternate coordinate systems are right-
//	handed Y-up, left-handed Y-up, and left-handed Z-up.

float3 ConvertFromStdCoordSys(float3 vCoord)
{
	if (ST_COORDSYS_Z_UP)
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, -vCoord.y, vCoord.z) : vCoord;
	else
		return ST_COORDSYS_LEFT_HANDED ? float3(vCoord.x, vCoord.z, vCoord.y) : float3(vCoord.x, vCoord.z, -vCoord.y);
}



///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system.

float3x3 BuildOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_COORDSYS_Z_UP)
		return ST_OPENGL ? float3x3(vRight, vOut, vUp) : float3x3(vRight.x, vOut.x, vUp.x,
																  vRight.y, vOut.y, vUp.y,
																  vRight.z, vOut.z, vUp.z);
	else
		return ST_OPENGL ? float3x3(vRight, vUp, vOut) : float3x3(vRight.x, vUp.x, vOut.x,
																  vRight.y, vUp.y, vOut.y,
																  vRight.z, vUp.z, vOut.z);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: BuildBillboardOrientationMatrix
//
//	Given three vectors to define an orientation, a matrix is built based on
//	the rendering platform and chosen coordinate system to turn the facing
//	leaf card geometry; this function is pretty basic, the algorithm for
//	determining the vector values is in the calling code.

float3x3 BuildBillboardOrientationMatrix(float3 vRight, float3 vOut, float3 vUp)
{
	if (ST_OPENGL)
		return float3x3(vRight, vOut, vUp);
	else
	{
		if (ST_COORDSYS_LEFT_HANDED)
			return float3x3(-vRight.x, vOut.x, vUp.x,
							-vRight.y, vOut.y, vUp.y,
							-vRight.z, vOut.z, vUp.z);
		else
			return float3x3(vRight.x, vOut.x, vUp.x,
							vRight.y, vOut.y, vUp.y,
							vRight.z, vOut.z, vUp.z);
	}
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: DotProductLighting
//
//	Simple lighting equation SpeedTree uses in a few shaders; SpeedTree lerps
//	from the ambient color to diffuse by the dot product of the normal and
//	light direction vectors.

float3 DotProductLighting(float3 vAmbientColor, float3 vDiffuseColor, float fDot)
{
	return lerp(vAmbientColor, vDiffuseColor, saturate(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: AmbientContrast
//
//	This is one technique SpeedTree employs to help keep the dark-side lighting
//	of vegetation interesting (non-flat). This is simpler than our image-based
//	ambient lighting but not mutually exclusive.

float AmbientContrast(float fContrastFactor, float fDot)
{
	return lerp(fContrastFactor, 1.0, abs(fDot));
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: Fade3dTree
//
//	The SDK's Core library will compute a 3D instance's LOD value such that:
//		- [1.0 -> 0.0] means the 3D tree is moving from highest 3D LOD to
//                     lowest 3D LOD
//		- [0.0 -> -1.0] means the 3D tree is fading gradually from lowest
//                      3D LOD to full billboard
//
//	Return value for Fade3dTree: 1.0 is no fade, 0.0 is full fade

float Fade3dTree(float fLodValue)
{
	// C code:
	//if (fLodValue < 0.0f)
	//{
	//	// fade is active
	//	return 1.0f - -fLodValue;
	//}
	//else
	//	// fade is inactive
	//	return 1.0f;

	return 1.0 - saturate(-fLodValue);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeBillboard
//
//	Given how far a billboard is from the camera (or LOD ref point) in world
//	units, this function will generate a [0.0, 1.0] scalar value that begins
//	at distance m_fBillboardStartDist and spans a distance of m_fBillboardRange.
//
//	Since billboards fade in as the camera moves further away, the scale value
//	will be 0.0 when (dist <= m_fBillboardStartDist), making the billboard invisible.
//	As the distance moves toward (m_fBillboardStartDist + m_fBillboardRange), it will
//	move linearly to 1.0, making the billboard fully visible.

float FadeBillboard(float fDistanceFromCamera)
{
	return saturate((fDistanceFromCamera - m_fBillboardStartDist) / m_fBillboardRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: FadeGrass
//
//	Given how far a grass instance is from the camera (or LOD ref point) in world
//	units, this function will generate a [0.0, 1.0] scalar value that begins
//	at distance m_f3dGrassStartDist and spans a distance of m_fBillboardRange.
//
//	Since grass instances fade out as the camera moves further away, the scale value
//	will be 1.0 when (dist <= m_f3dGrassStartDist), making the grass fully visible.
//	As the distance moves toward (m_f3dGrassRange + m_fBillboardRange), it will
//	move linearly to 0.0, making the grass invisible and able to be culled.

float FadeGrass(float fDistanceFromCamera)
{
	return saturate(1.0 - (fDistanceFromCamera - m_f3dGrassStartDist) / m_f3dGrassRange);
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ComputeTransmissionFactor
//
//	Transmission (SpeedTree's approximation of subsurface scattering), is
//	computed using the surface normal together with a "view dependency" value
//	set by the artist in the Modeler. View dependency controls how much the
//	viewing angle limits the effect of translucency. A value of 1.0 (the
//	default) fades light transmission in as the camera looks towards the
//	light source. A value of 0.0 allows light transmission on the front-lit
//	side.

float ComputeTransmissionFactor(float3 vNormal, float fViewDependency, bool bBackfaceCulling)
{
	// adjust normal to compensate for backfacing geometry
	if (bBackfaceCulling)
		vNormal = -vNormal;

	// how much is the camera looking into the light source?
	float fBackContribution = ST_COORDSYS_LEFT_HANDED ? (dot(-m_vCameraDirection, m_vDirLight.m_vDir.xyz) + 1.0) * 0.5 :
														(dot(m_vCameraDirection, m_vDirLight.m_vDir.xyz) + 1.0) * 0.5;

	// compute the result to get the right falloff
	fBackContribution *= fBackContribution;
	fBackContribution *= fBackContribution;

	// use the reverse normal to compute scatter
	float fScatterDueToNormal = (dot(-m_vDirLight.m_vDir.xyz, -vNormal) + 1.0) * 0.5;

	// choose between scatter and back contribution based on artist-controlled parameter
	float fTransmissionFactor = lerp(fScatterDueToNormal, fBackContribution, fViewDependency);

	// back it off based on how much leaf is in the way
	float fReductionDueToNormal = ST_COORDSYS_LEFT_HANDED ? (dot(-m_vCameraDirection, vNormal) + 1.0) * 0.5 :
															(dot(m_vCameraDirection, vNormal) + 1.0) * 0.5;
	fTransmissionFactor *= fReductionDueToNormal;

	return fTransmissionFactor;
}


///////////////////////////////////////////////////////////////////////
//  Helper Function: ProjectToScreen
//
//	Simple screen projection using a composite modelview / projection matrix
//	uploaded to m_mModelViewProj3d. SpeedTree subtracts the camera position
//	from the incoming position to help curb floating-point resolution issues
//	with large position values.

float4 ProjectToScreen(float4 vPos)
{
	// the camera stay at the origin, so everything else is translated to it
	vPos.xyz -= m_vCameraPosition;

	return mul_float4x4_float4(m_mModelViewProj3d, vPos);
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float4 version.

float4 DecodeVectorFromColor(float4 vColor)
{
	return vColor * 2.0 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeVectorFromColor
//
//	Converts a color of range [0, 1] to a vector of range [-1, 1].
//
//	float3 version.

float3 DecodeVectorFromColor(float3 vColor)
{
	return vColor * 2.0 - 1.0;
}

// LAVA+
///////////////////////////////////////////////////////////////////////  
//	Helper Function: DecodeVectorFromDXTn

float3 DecodeNormalFromDXTn(float4 vDXTn)
{
	float3 vRawNormal = float3 ( 2.0 * vDXTn.wy - 1.0, 0 );
	vRawNormal.z = sqrt( max( 0, 1 - vRawNormal.x * vRawNormal.x - vRawNormal.y * vRawNormal.y ) );			
	return vRawNormal;
}
// LAVA-

///////////////////////////////////////////////////////////////////////
//	Helper Function: EncodeVectorToColor
//
//	Converts a vector of range [-1, 1] to a color of range [0, 1].

float3 EncodeVectorToColor(float3 vVector)
{
	return vVector * 0.5 + 0.5;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: DecodeFloat4FromUBytes
//
//	This function helps to align SpeedTree's supported platforms behaviors when
//	storing normal values as unsigned bytes.

// todo: really?
float4 DecodeFloat4FromUBytes(float4 vCompressedBytes)
{
	if (ST_DIRECTX11 || ST_PS4)
		return vCompressedBytes * 2.0 - 1.0;
	else if (ST_XBOX_360)
		// 360 automatically normalizes signed byte values when uploaded as D3DDECLTYPE_BYTE4N, but
		// free swizzle is still used to counter big endian
		return vCompressedBytes.wzyx / 127.5 - 1.0;
	else
		return vCompressedBytes / 127.5 - 1.0;
}


///////////////////////////////////////////////////////////////////////
//	Helper Function: CheckForEarlyExit
//
//	Basically this is a function to encapsulate alpha testing. It depends
//	on whether the app is using alpha testing or alpha-to-coverage to
//	handle transparency. Also note OpenGL's use of discard versus HLSL's
//	use of clip. In OpenGL, discard is not a valid call from a vertex shader
//	so it is #ifdef'd out.

// LAVA+
float GetEarlyExitThreshold()
{
	#ifdef ALPHA_TEST_NOISE
		#define c_fAlphaKillThreshhold m_fAlphaScalar
	#else
		#define c_fAlphaKillThreshhold 0.1
	#endif

	return c_fAlphaKillThreshhold;
}
// LAVA-

// LAVA+
void CheckForEarlyExit_CustomThreshold(float fAlphaValue, float fAlphaKillThreshhold)
{
	// THIS FUNCTION IS OUTDATED NOW. IF USE IT - CHECK ALL THE DEF'S AGAIN.
	#if !defined(DIFFUSE_MAP_IS_OPAQUE) || defined(FADE_TO_BILLBOARD) || (ST_USED_AS_GRASS)
		#ifdef ST_OPENGL
			// at least one GLSL compiler will not permit discard to be present in a vertex
			// shader's source, even if it isn't used
			#ifdef ST_PIXEL_SHADER
				if (fAlphaValue < fAlphaKillThreshhold)
					discard;
			#endif
		#else
			clip(fAlphaValue - fAlphaKillThreshhold);
		#endif
	#endif
}
// LAVA-

void CheckForEarlyExit(float fAlphaValue, bool bTransparencyActive)
{
	if (bTransparencyActive)
	{
		#if (ST_OPENGL)
			// at least one GLSL compiler will not permit discard to be present in a vertex
			// shader's source, even if it isn't used, so we put this #ifdef around it
			#ifdef ST_PIXEL_SHADER
				if (fAlphaValue < ST_ALPHA_KILL_THRESHOLD)
					discard;
			#endif
		#else
			clip(fAlphaValue - ST_ALPHA_KILL_THRESHOLD);
		#endif
	}
}


///////////////////////////////////////////////////////////////////////
//  CubicSmooth
//
//	Used in conjunction with TrigApproximate()

float4 CubicSmooth(float4 vData)
{
	return vData * vData * (3.0 - 2.0 * vData);
}


///////////////////////////////////////////////////////////////////////
//  TriangleWave
//
//	Used in conjunction with TrigApproximate()

float4 TriangleWave(float4 vData)
{
	return abs((frac(vData + 0.5) * 2.0) - 1.0);
}


///////////////////////////////////////////////////////////////////////
//  TrigApproximate
//
//	Faster/approximate sin wave.

float4 TrigApproximate(float4 vData)
{
	return (CubicSmooth(TriangleWave(vData)) - 0.5) * 2.0;
}


///////////////////////////////////////////////////////////////////////
//  UnpackNormalFromFloat
//
//	Designed to work with 16-bit float values, this function pulls a float3
//	of range [-1, 1] from a single 16-bit float value. Also works with 32-bit
//	floats, but spreads the error better for 16.

float3 UnpackNormalFromFloat(float fValue)
{
	#define ST_DECODE_KEY float3(16.0, 1.0, 0.0625)

	// decode into [0,1] range
	float3 vDecodedValue = frac(fValue / ST_DECODE_KEY);

	// move back into [-1,1] range & normalize
	return (vDecodedValue * 2.0 - 1.0);

	#undef ST_DECODE_KEY
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Stereographic(float3 n)
{
	float scale = 1.7777;

	float2 enc = n.xy / (n.z + 1.0);
	enc /= scale;
	enc = enc * 0.5 + 0.5;

	return enc;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Stereographic
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float3 UnpackNormalFromFloat2_Stereographic(float2 enc2)
{
	float4 enc = float4(enc2, 0, 0);

	float scale = 1.7777;
	float3 nn = enc.xyz * float3(2.0 * scale, 2.0 * scale,0) + float3(-scale, -scale, 1.0);
	float g = 2.0 / dot(nn.xyz, nn.xyz);

	float3 n;
	n.xy = g * nn.xy;
	n.z = g - 1.0;

	return n;
}


////////////////////////////////////////////////////////////
//	PackNormalIntoFloat_Spheremap
//
//	http://aras-p.info/texts/CompactNormalStorage.html

float2 PackNormalIntoFloat2_Spheremap(float3 n)
{
	float f = sqrt(8.0 * n.z + 8.0);

	return n.xy / f + 0.5;
}


////////////////////////////////////////////////////////////
//	UnpackNormalFromFloat_Spheremap

float3 UnpackNormalFromFloat2_Spheremap(float2 enc2)
{
	float2 fenc = enc2 * 4.0 - 2.0;
	float f = dot(fenc, fenc);
	float g = sqrt(1.0 - f / 4.0);
	float3 n;
	n.xy = fenc * g;
	n.z = 1.0 - f / 2.0;

	return normalize(n);
}


///////////////////////////////////////////////////////////////////////
//  ArbitraryAxisRotationMatrix
//
//  Constructs an arbitrary axis rotation matrix. Resulting matrix will
//	rotate a point fAngle radians around vAxis.

float3x3 ArbitraryAxisRotationMatrix(float3 vAxis, float fAngle)
{
	// compute sin/cos of fAngle
	float2 vSinCos;
	#if (ST_OPENGL)
		vSinCos.x = sin(fAngle);
		vSinCos.y = cos(fAngle);
	#else
		sincos(fAngle, vSinCos.x, vSinCos.y);
	#endif

	#define c_var vSinCos.y
	#define s_var vSinCos.x
	#define x_var vAxis.x
	#define y_var vAxis.y
	#define z_var vAxis.z
	float t_var = 1.0 - c_var;

	return float3x3(t_var * x_var * x_var + c_var,			t_var * x_var * y_var - s_var * z_var,	t_var * x_var * z_var + s_var * y_var,
					t_var * x_var * y_var + s_var * z_var,	t_var * y_var * y_var + c_var,			t_var * y_var * z_var - s_var * x_var,
					t_var * x_var * z_var - s_var * y_var,	t_var * y_var * z_var + s_var * x_var,	t_var * z_var * z_var + c_var);

	#undef c_var
	#undef s_var
	#undef x_var
	#undef y_var
	#undef z_var
}


///////////////////////////////////////////////////////////////////////
//  LimitToCameraPlane
//
//	Used in conjunction with billboard wind -- it confines the wind motion
//	to the plane defined by the screen

float3 LimitToCameraPlane(float3 vDir)
{
	const float3 c_vCameraRight = ST_OPENGL ? m_mCameraFacingMatrix[1].xyz : float3(m_mCameraFacingMatrix[0].y, m_mCameraFacingMatrix[1].y, m_mCameraFacingMatrix[2].y);
	const float3 c_vCameraUp = ST_OPENGL ? m_mCameraFacingMatrix[2].xyz : float3(m_mCameraFacingMatrix[0].z, m_mCameraFacingMatrix[1].z, m_mCameraFacingMatrix[2].z);

	return dot(vDir, c_vCameraRight) * c_vCameraRight + dot(vDir, c_vCameraUp) * c_vCameraUp;
}


///////////////////////////////////////////////////////////////////////
//  ProjectToLightSpace
//
//	Used to project SpeedTree geometry into a shadow map.

float4 ProjectToLightSpace(float3 vPos, float4x4 mLightViewProj)
{
	float4 vProjection = mul_float4x4_float4(mLightViewProj, float4(vPos, 1.0));

	if (ST_DIRECTX9 || ST_DIRECTX11)
	{
		vProjection.xy = 0.5 * vProjection.xy / vProjection.w + float2(0.5, 0.5) + m_sShadows.m_vShadowMapTexelOffset;
		vProjection.y = 1.0 - vProjection.y;
	}
	else
	{
		vProjection.xyz += vProjection.www;
		vProjection.xy += m_sShadows.m_vShadowMapTexelOffset;
		vProjection.xyz *= 0.5;

		if (ST_PS4)
			vProjection.y = 1.0 - vProjection.y;
	}

	return vProjection;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByPos
//
//	Determines a unique [0, 1] float3 based on attributes unique to an instance;
//	in this case we're using a combination of the instance's 3D position
//	and orientation vector.

float3 GenerateHueVariationByPos(float fVariationScalar, float3 vInstanceRightVector)
{
	return fVariationScalar * fmod(vInstanceRightVector, float3(1.0, 1.0, 1.0)) * m_vHueVariationColor;
}


///////////////////////////////////////////////////////////////////////
//  GenerateHueVariationByVertex
//
//	Determine a unique [0, 1] float3 value based on attributes unique to a vertex
//	that *do not* change with LOD. We're using the normal in combination
//	with the instance's orientation vector.
//
//	This function can be modified as needed, but care should be take not to
//	use vertex attributes that might change as LOD changes, else the hue
//	variation will change along with it.

float3 GenerateHueVariationByVertex(float fVariationScalar, float3 vNormal)
{
	return fVariationScalar * sin(vNormal * 20.0f) * m_vHueVariationColor.rgb;
}


///////////////////////////////////////////////////////////////////////
//  FadeShadowByDepth
//
//	In the SpeedTree example shadow system, the shadow fades out after a
//	certain distance away from the camera. Given a shadow map look up value,
//	this function will push it to 1.0 (white / no shadow) based on a couple
//	of parameters. Function paramters:
//
//		fShadowMapValue: the raw value from the shadowmap look up (0.0 is
//						 fully shadowed, 1.0 is fully lit)
//
//		fDepthAtShadowedPoint: distance from the camera to the pixel
//
//		fStartingFadeDepth: [0.0, 1.0] value designating where the shadow
//							should begin to fade. 0.0 is at the camera pos,
//							1.0 is m_fFarClip.x
//
//		fFadeFactor: value computed on upload to save shader instructions; it's
//					 1.0f / (end_of_last_cascade - fStartingFadeDepth), where
//					 end_of_last_cascade is in same units as fStartingFadeDepth

float FadeShadowByDepth(float fShadowMapValue, float fDepthAtShadowedPoint, float fStartingFadeDepth, float fFadeFactor)
{
	float fFade = saturate((fDepthAtShadowedPoint - fStartingFadeDepth) * fFadeFactor);

	return lerp(fShadowMapValue, 1.0, fFade);
}


///////////////////////////////////////////////////////////////////////
//  Helper function: BillboardSelectMapFromAtlas
//
//	Billboard instances are rendered in batches of the same base tree (e.g. all
//	palm trees are rendered in one call), but they may all have arbitrary
//	orientations. Each base tree also probably has several billboard renderings
//	of the tree from different angles (360-degree billboards), all in the same
//	billboard atlas that's current bound. This function will figure out which
//	atlas entry is the correct one.
//
//	Given a camera, an instance orientation (right, out, up), and the number of
//	360-degree billboard images uploaded for the current base tree, this function
//	will determine the correct texcoords per vertex from the billboard atlas.
//
//	The coordinate system figures heavily into the computation, so you'll see use
//	of ST_COORDSYS_RIGHT_HANDED, ST_COORDSYS_Z_UP, etc.

float2 BillboardSelectMapFromAtlas(float2 vUnitTexCoords,		// texcoords of cutout billboard, defined in Compiler, in [0.0,1.0] range;
																// will be converted to correct smaller texcoords to access sub-image in atlas
								   float3 vRightVector,			// right orientation of current instance
								   float3 vOutVector,			// out orientation of current instance
								   float3 vUpVector,			// up orientation of current instance
								   float3 vCameraDirection)		// which way the main camera is facing
{
	// algorithmic depends on opposite direction
	float3 vDir = -vCameraDirection.xyz;

	// given a particular coordinate system some adjustments are necessary for algorithm that expects right-handed/Z-up
	if (ST_COORDSYS_LEFT_HANDED)
	{
		if (ST_COORDSYS_Y_UP)
			vDir = float3(-vDir.x, vDir.y, -vDir.z);
		else
			vRightVector = float3(-vRightVector.x, -vRightVector.y, vRightVector.z);
	}

	// project camera direction onto plane that holds vRightVector and vOutVector vectors
	float fUpDot = dot(vDir, vUpVector);
	float3 vProjectedCamera = normalize(vDir - fUpDot * vUpVector);

	// determine which angle the projection lands on; corresponds to the correct bb sub-image
	float fRightDot = dot(vRightVector, vProjectedCamera);
	if (ST_PS3)
		fRightDot = clamp(fRightDot, -1.0, 1.0);
	float fRightAngle = acos(fRightDot);

	// need a value in [0,2pi] range, not [-pi,pi]
	float fOutDot = dot(vOutVector, vProjectedCamera);
	if (fOutDot < 0.0)
		fRightAngle = ST_TWO_PI - fRightAngle;

	// bump angle to be in the middle of the pie slice
	fRightAngle += ST_PI / m_fNumBillboards;
	fRightAngle = fmod(fRightAngle, ST_TWO_PI);

	// convert angle to billboard image index
	int nImageIndex	= int(fRightAngle / m_fRadiansPerImage);

	// m_avBillboardTexCoords holds the texcoords for each 360-degree image for a given base tree; each single
	// entry xyzw defines a sub-rectangle in the billboard atlas and does it using four values:
	//
	//		(left u texcoord, bottom v texcoord, width of u span, height of v span)
	//
	// for example, if there are four evenly-spaced billboard images in a given atlas and no other textures
	// reside there, the four entries in m_avBillboardTexCoords might be:
	//
	//		m_avBillboardTexCoords[0] = (0.0, 0.5, 0.5, 0.5) // top left bb image
	//		m_avBillboardTexCoords[1] = (0.5, 0.5, 0.5, 0.5) // top right bb image
	//		m_avBillboardTexCoords[1] = (0.0, 1.0, 0.5, 0.5) // bottom left bb image
	//		m_avBillboardTexCoords[1] = (0.5, 1.0, 0.5, 0.5) // bottom right bb image
	//
	// note that because billboard images may be oriented vertically or horizontally, the .x coordinate in
	// each entry may be negative, indicating a horizontal orientation; the example above assumes all
	// vertical orientations

	// nImage index holds the correct offset into this table
	#ifndef g_avBillboardTexCoords
		float4 vTableEntry = m_avBillboardTexCoords[nImageIndex];
	#else
		// in opengl, g_avBillboardTexCoords is defined a bit differently
		float4 vTableEntry = A_avSingleUniformBlock[BILLBOARD_TEXCOORDS_LOCATION + nImageIndex];
	#endif

	// some further adjustment is still needed for alternate coordinate systems
	if (ST_COORDSYS_RIGHT_HANDED && ST_COORDSYS_Y_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;
	else if (ST_COORDSYS_LEFT_HANDED && ST_COORDSYS_Z_UP)
		vUnitTexCoords.x = 1.0 - vUnitTexCoords.x;

	// if .x coordinate is negative, the billboard is oriented horizontally; this code also applies to the unit
	// texcoords to the sub-image of the billboard atlas
	float2 vAtlasTexCoords = (vTableEntry.x < 0.0) ? float2(-vTableEntry.x + vTableEntry.z * vUnitTexCoords.y,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.x) :
													 float2(vTableEntry.x + vTableEntry.z * vUnitTexCoords.x,
															vTableEntry.y + vTableEntry.w * vUnitTexCoords.y);

	return vAtlasTexCoords;
}


///////////////////////////////////////////////////////////////////////
//  RgbToLuminance
//
//	Centralized function so that users can choose between an accurate conversion
//	or quick and dirty.

float RgbToLuminance(float3 vRGB)
{
	// decent conversion
	//return dot(vRGB, float3(0.299, 0.587, 0.114)); // CCIR 601 luminance coeffs

	// very quick and dirty
	return vRGB.g;
}

#endif // ST_INCLUDE_UTILITY
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

#ifndef ST_INCLUDE_TREE_TEXTURES
#define ST_INCLUDE_TREE_TEXTURES


///////////////////////////////////////////////////////////////////////
//	Texture/material bank used for geometry types
//
//	In OpenGL/GLSL, changes in these sampler names (or additions) should be followed by 
// a change to BindTextureSamplers() in Shaders_inl.h in the OpenGL render library.

DeclareTexture(DiffuseMap, 0);
DeclareTexture(NormalMap, 1);
DeclareTexture(DetailDiffuseMap, 2);
DeclareTexture(DetailNormalMap, 3);
DeclareTexture(SpecularMaskMap, 4);
DeclareTexture(TransmissionMaskMap, 5);
DeclareTexture(AuxAtlas1, 6);
//LAVA++
// For tree fading, bind our own dissolve pattern. This ensures a consistent dissolve experience
// between meshes and trees, and also we can be sure that it's been bound (ST might not bind its
// own noise-dissolve when not needed). Slot 7 is PSSMP_Dissolve
DeclareTexture(DissolveMap, 7);
//DeclareTexture(AuxAtlas2, 7);
//LAVA--
DeclareTexture(NoiseMap, 8);
DeclareTexture(PerlinNoiseKernel, 9);
DeclareTexture(ImageBasedAmbientLighting, 10);

#endif // ST_INCLUDE_TREE_TEXTURES
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

#ifndef ST_INCLUDE_TEXTURE_UTILITY
#define ST_INCLUDE_TEXTURE_UTILITY


// todo: why is this only being used by the deferred pixel shader?

///////////////////////////////////////////////////////////////////////
//  LookupColorWithDetail
//
//	This function encapsulates the diffuse and detail layer texture lookups.
//
//	It will always look up the diffuse texture. If a detail layer is present,
//	it will look it up and lerp between the diffuse and detail layer by the
//	detail map's alpha channel value. If the detail layer should fade away,
//	it will handle that was well.

float3 LookupColorWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = SampleTexture(DiffuseMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_LAYER)
	{
		float4 texDetailColor = SampleTexture(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		vFinal = lerp(vFinal, texDetailColor.rgb, texDetailColor.a);
	}

	return vFinal;
}


///////////////////////////////////////////////////////////////////////
//  LookupNormalWithDetail
//
//	This function encapsulates the diffuse and detail layer normal map
//	lookups.
//
//	It will always look up the diffuse normal texture. If a detail normal
//	layer is present, it will look it up and lerp between the diffuse and
//	detail layer normals by the detail normal's alpha channel value
//	by the detail's *color* alpha channel value (the same one used in
//	LookupColorWithDetail). If the detail layer should fade away, it will
//	handle that was well.
//
//	The normal map color values [0,1] are converted to a vector [-1,1] before
//	returning.

float3 LookupNormalWithDetail(float2 vDiffuseTexCoords, float2 vDetailTexCoords, float fRenderEffectsFade)
{
	float3 vFinal = SampleTexture(NormalMap, vDiffuseTexCoords).rgb;

	if (ST_EFFECT_DETAIL_NORMAL_LAYER)
	{
		float4 texDetailColor = SampleTexture(DetailDiffuseMap, vDetailTexCoords);
		if (ST_EFFECT_DETAIL_LAYER_FADE)
			texDetailColor.a *= fRenderEffectsFade;

		float3 vDetailNormal = SampleTexture(DetailNormalMap, vDetailTexCoords).rgb;
		vFinal = lerp(vFinal, vDetailNormal, texDetailColor.a);
	}

	return DecodeVectorFromColor(vFinal);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_Billboard
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the billboard's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_Billboard(float2 vTexCoords)
{
	#define ST_BILLBOARD_ALPHA_NOISE_SCALAR 30.0

	return (ST_ALPHA_TEST_NOISE ? SampleTexture(NoiseMap, vTexCoords * ST_BILLBOARD_ALPHA_NOISE_SCALAR).a : 1.0);
}


///////////////////////////////////////////////////////////////////////  
//	AlphaTestNoise_3dTree
//
//	The SpeedTree SDK generates a small noise texture to facilitate a
//	"fizzle" effect when alpha testing is used over alpha-to-coverage.
//	Alpha fizzle is the term applied to the "fizzling" from one LOD to
//	another based upon changing alpha testing values in unison with
//	returning alpha noise from the pixel shader.
//
//	This function uses the 3d tree's texcoords to lookup the noise
//	texture to get a smooth fizzle effect.

float AlphaTestNoise_3dTree(float2 vTexCoords)
{
	#define ST_TREE_ALPHA_NOISE_SCALAR	0.5

	return (ST_ALPHA_TEST_NOISE ? SampleTexture(NoiseMap, vTexCoords * ST_TREE_ALPHA_NOISE_SCALAR).a : 1.0);
}

#endif // ST_INCLUDE_TEXTURE_UTILITY

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

#ifndef ST_INCLUDE_USER_INTERPOLANTS
#define ST_INCLUDE_USER_INTERPOLANTS


//	Due to the way shaders are generated from templates in the SpeedTree 6.x and 7.x systems, it's
//	more or less impossible to modify the templates to add extra interpolants (those values
//	passed from the vertex to the pixel shaders). We added this system to make it manageable.
//
//	Users have the option of adding up to four extra float4 interpolants by modifying the
//	source contained in this file. By adding "#define ST_USER_INTERPOLANT0" to this file, an
//	extra float4 will be passed from the vertex shader to the pixel shader. If #define'd, 
//	float4 v2p_vUserInterpolant ("v2p" = vertex-to-pixel) will be created. It must be
//	fully assigned in the vertex shader, else the compilation is likely to fail. Search 
//  for "ST_USER_INTERPOLANT0" in Template_3dGeometry_Vertex.fx or
//  Template_3dGeometry_Vertex_Deferred.fx) to find where this should be done.
//
//	Keep in mind that users may put conditions on when the user interpolants are active.
//	That is, interpolants don't have to be active for every generated shader. If, for example,
//	you're passing a new value to be used with specular lighting, you can check for the
//	presence of specular lighting by using code like:
//	
//	#ifdef SPECULAR // defined by the Compiler when generating
//					// each shader, based on effect LOD dialog
//		#define ST_USER_INTERPOLANT0
//	#endif
//	
//	Then, within the vertex shader, only assign the interpolant when present:
//
//	#ifdef ST_USER_INTERPOLANT0
//		v2p_vUserInterpolant0 = float4(0, 0, 0, 0);
//	#endif
//
//	Be sure to only access the v2p_vUserInterpolantN variables in the pixel shader when they're
//	defined.
//
//	Note that these changes apply to the forward- and deferred-render templates used for the
//	3D tree geometry only. Billboard templates are not affected since they do not use dynamic
//	interpolants. Users are free to modify the less complex billboard templates as needed.
//
//	Below is a nearly-complete list of the macros defined by the Compiler when various render 
//	& wind states are set by the user. They can be used in your own expressions to manage when
//	interpolants should be used.
//
//	todo: what to do with all this? a lot is out of date
//
//	  Macro								Additional Description
//  -------------------------------------------------------------------------------------------
//	ALPHA_TEST_NOISE					Alpha test noise is active (when a2c isn't available)
//	BACKFACES_CULLED					
//	LIGHTING_MODEL_PER_VERTEX			
//	LIGHTING_MODEL_PER_PIXEL
//	LIGHTING_MODEL_GRADUAL_CHANGE		Forward rendering; transition between per-vertex and per-pixel lighting
//	LIGHTING_MODEL_DEFERRED				Deferred rendering is active (always per-pixel)
//	AMBIENT_CONTRAST					Ambient contrast is active, labeled "Contrast" in Effect LOD dialog
//	AMBIENT_CONTRAST_FADE				Ambient contrast effect is fading in/out in this LOD
//	AMBIENT_OCCLUSION
//	DIFFUSE_MAP_IS_OPAQUE				There is no alpha channel in the diffuse texture (can skip texkill)
//	DETAIL_LAYER						
//	DETAIL_LAYER_FADE					Detail layer is fading in/out in this LOD
//	DETAIL_NORMAL_LAYER					Detail layer has and is using its own normal map
//	SPECULAR							Specular lighting is in effect (forward rendering)
//	SPECULAR_FADE						Specular effect is fading in/out in this LOD
//	TRANSMISSION						Transmission lighting is in effect (forward rendering)
//	TRANSMISSION_FADE					Transmission effect is fading in/out in this LOD
//	BRANCH_SEAM_SMOOTHING				
//	BRANCH_SEAM_SMOOTHING_FADE
//	LOD_METHOD_SMOOTH					LOD setting is smooth as opposed to popping
//	FADE_TO_BILLBOARD					This shader is the last LOD and fades out to make way for billboard
//	HAS_HORZ_BB							Indicates a horizontal billboard is present
//	FOG_COLOR_TYPE_CONSTANT				Using SpeedTree's fog system, the color is constant
//	FOG_COLOR_TYPE_DYNAMIC				Using SpeedTree's fog system, the color is computed per-pixel (see sky color in ref app)
//	SHADOWS_ENABLED						Geometry rendered in this shader receives shadows
//	SHADOWS_SMOOTH						Shadows will be filtered above standard PCF (expensive)
//	ST_SHADOWS_NUM_MAPS					Number of cascaded shadow maps, one to four
//	ST_BRANCHES_PRESENT					This shader intended to be used to render branch geometry (may be mixed)
//	ONLY_BRANCHES_PRESENT				This shader intended to be used to render *only* branch geometry
//	ST_FRONDS_PRESENT
//	ONLY_FRONDS_PRESENT
//	ST_LEAVES_PRESENT
//	ONLY_LEAVES_PRESENT
//	FACING_ST_LEAVES_PRESENT
//	ONLY_FACING_ST_LEAVES_PRESENT
//	RIGID_MESHES_PRESENT
//	ONLY_RIGID_MESHES_PRESENT
//	VERTEX_PROPERTY_TANGENT_PRESENT		Flag indicating that a per-vertex tangent value is in the vertex decl
//	USED_AS_GRASS						This shader is intended to render grass geometry
//
//
// Wind state macros appear in this section:
//
//	  Macro					Description
//  -------------------------------------------------------------------------
//	WIND_LOD_GLOBAL
//	WIND_LOD_BRANCH
//	WIND_LOD_FULL
//	WIND_LOD_NONE_X_GLOBAL
//	WIND_LOD_GLOBAL
//	WIND_LOD_NONE_X_BRANCH
//	WIND_LOD_BRANCH
//	WIND_LOD_NONE_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_GLOBAL_X_BRANCH
//	WIND_LOD_BRANCH
//	WIND_LOD_GLOBAL_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_BRANCH_X_FULL
//	WIND_LOD_FULL
//	WIND_LOD_NONE
//	ST_WIND_EFFECT_GLOBAL_WIND
//	ST_WIND_EFFECT_GLOBAL_PRESERVE_SHAPE
//	ST_WIND_EFFECT_BRANCH_SIMPLE_1
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_1
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_1
//	ST_WIND_EFFECT_BRANCH_TURBULENCE_1
//	ST_WIND_EFFECT_BRANCH_WHIP_1
//	ST_WIND_EFFECT_BRANCH_ROLLING_1
//	ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_1
//	ST_WIND_EFFECT_BRANCH_SIMPLE_2
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_2
//	ST_WIND_EFFECT_BRANCH_DIRECTIONAL_FROND_2
//	ST_WIND_EFFECT_BRANCH_TURBULENCE_2
//	ST_WIND_EFFECT_BRANCH_WHIP_2
//	ST_WIND_EFFECT_BRANCH_ROLLING_2
//	ST_WIND_EFFECT_BRANCH_OSC_COMPLEX_2
//	ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_1
//	ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_1
//	ST_WIND_EFFECT_LEAF_TUMBLE_1
//	ST_WIND_EFFECT_LEAF_TWITCH_1
//	ST_WIND_EFFECT_LEAF_ROLL_1
//	ST_WIND_EFFECT_LEAF_OCCLUSION_1
//	ST_WIND_EFFECT_LEAF_RIPPLE_VERTEX_NORMAL_2
//	ST_WIND_EFFECT_LEAF_RIPPLE_COMPUTED_2
//	ST_WIND_EFFECT_LEAF_TUMBLE_2
//	ST_WIND_EFFECT_LEAF_TWITCH_2
//	ST_WIND_EFFECT_LEAF_ROLL_2
//	ST_WIND_EFFECT_LEAF_OCCLUSION_2
//	ST_WIND_EFFECT_FROND_RIPPLE_ONE_SIDED
//	ST_WIND_EFFECT_FROND_RIPPLE_TWO_SIDED
//	ST_WIND_EFFECT_FROND_RIPPLE_ADJUST_LIGHTING
//	WIND_LOD_BILLBOARD_GLOBAL
//
//
//	The following macros are not render state related, but may be helpful:
//
//	  Macro					Description
//  -------------------------------------------------------------------------
//	DEFERRED_RENDER
//	FORWARD_RENDER
//	OPENGL
//	DIRECTX9
//	DIRECTX11
//	XBOX_360
//	PS3
//	PSP2
//	BILLBOARD_SHADER
//	TREE_SHADER
//	VERTEX_SHADER
//	PIXEL_SHADER
//	ST_COORDSYS_Y_UP
//	ST_COORDSYS_Z_UP
//	ST_COORDSYS_RIGHT_HANDED
//	ST_COORDSYS_LEFT_HANDED


//	Define all or none of the four user interpolants; use #ifdef conditions to restrict
//	use if viable (see top of this file for details)

#define ST_USER_INTERPOLANT0 1		// LAVA: random colorization
#define ST_USER_INTERPOLANT1 1		// LAVA: worldPosition
#if (ST_USED_AS_GRASS) && !defined(DEPTH_ONLY)
	#define ST_USER_INTERPOLANT2 1	// LAVA: pigment
#else
	#define ST_USER_INTERPOLANT2 0
#endif
#define ST_USER_INTERPOLANT3 1		// LAVA: tree fading

#endif // ST_INCLUDE_USER_INTERPOLANTS
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

#ifndef ST_INCLUDE_GBUFFER
#define ST_INCLUDE_GBUFFER

#if (!ST_OPENGL)

	///////////////////////////////////////////////////////////////////////
	//  Deferred pixel shader output
	//
	//	This is an example SpeedTree G-buffer only. You will need to define
	//	your own G-buffer format here and populate the targets by modifying
	//	Template_3dGeometry_Pixel_Deferred.fx and Template_Billboard_Pixel_Deferred.fx.

	struct SMultiRenderTargets
	{
		#if (ST_XBOX_360)
			float4	m_vRenderTargets[2] : COLOR0;
		#else
			#if (ST_DEFERRED_A2C_ENABLED)
				float4	m_vNullRenderTarget	: ST_RENDER_TARGET0;	// NULL render target (alpha used for alpha-to-coverage)
				float4	m_vRenderTarget0    : ST_RENDER_TARGET1;	// [rgb] = albedo.rgb, [a] = specular power
				float4	m_vRenderTarget1    : ST_RENDER_TARGET2;	// [rg] = normal UV, [b] = specular mask [a] = transmission mask
			#else
//				float4	m_vRenderTarget0	: ST_RENDER_TARGET0;	// [rgb] = albedo.rgb, [a] = specular power
//				float4	m_vRenderTarget1    : ST_RENDER_TARGET1;	// [rg] = normal UV, [b] = specular mask [a] = transmission mask
// LAVA++
				float4	m_vRenderTarget0    : ST_RENDER_TARGET0;
				float4	m_vRenderTarget1    : ST_RENDER_TARGET1;
				float4	m_vRenderTarget2    : ST_RENDER_TARGET2;


#ifdef __PSSL__
				uint	m_Coverage	: S_COVERAGE;
#else
				uint    m_Coverage	: SV_Coverage;
#endif

					
// LAVA--
			#endif
		#endif
	};

	#define ST_OUTPUT_TYPE SMultiRenderTargets
#else
	#define ST_OUTPUT_TYPE void
#endif

#endif // ST_INCLUDE_GBUFFER


// LAVA++

#ifndef COMMON_FX_H_INCLUDED
#define COMMON_FX_H_INCLUDED

// This define heavily reduces the temp registers usages in some simple loops
// because for instance in case of global shadowmask calculation (globalShadow.fx) without it shadow sampling interations don't add up
// to some registers, but instead store every sampling result in a separate
// register and then merge at the very end :)
// So this stuff could be included per shader, but IMO it's better to keep it global, becasue we won't miss any important shader.
// !!! IN CASE ANYONE HAD AND IDEA TO REMOVE THIS FROM HERE, 
// !!! REMEMBER ABOUT globalShadow.fx and some other simple shaders.
#define __XBOX_WAVESIM_ITERATION_N	1


/// Common header for all dynamically compiled shaders
/// It's esential to recompile all shaders after changing any of those lines

// Realize global constants in compiled shaders

#define REG( _name, _reg, _type ) _type _name;
#define REGI( _name, _reg, _type ) _type _name : register( i##_reg );
#define REGB( _name, _reg, _type ) _type _name : register( b##_reg );
#define REG_ARRAY( _name,_reg, _type, _size )  _type _name[ _size ] : register( c##_reg );
#define SYS_SAMPLER( _name,_reg )	SamplerState	s_##_name	: register( s##_reg ); \
									Texture2D		t_##_name	: register( t##_reg );
#define HALF_PIXEL_OFFSET 0.0f

// =====================================================================
// There are differences in sample functions names between HLSL and PSSL. Use ONLY macros below.
#define SYS_SAMPLE( _name, _coord )					t_##_name.Sample( s_##_name, _coord )
#define SYS_SAMPLE_TEXEL( _name, _coord )			t_##_name[ _coord ]
#define SAMPLE( _texture, _sampler, _coord )		_texture.Sample( _sampler, _coord )

#ifdef __PSSL__
	#define SYS_SAMPLE_LEVEL( _name, _coord, _level )	t_##_name.SampleLOD( s_##_name, _coord, _level )
	#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLOD( _sampler, _coord, _level )
	#define SAMPLE_CMP_LEVEL0( _texture, _sampler, _coord, _compareValue ) _texture.SampleCmpLOD0( _sampler, _coord, _compareValue )
	#define SAMPLE_GRADIENT( _texture, _sampler, _coord, _ddx, _ddy ) _texture.SampleGradient( _sampler, _coord, _ddx, _ddy )
	#define SYS_STATIC 
	#define SAMPLE_MIPMAPS( _texture, _miplevel, _location ) _texture.MipMaps[ _miplevel ][ _location ]
#else
	#define SYS_SAMPLE_LEVEL( _name, _coord, _level )	t_##_name.SampleLevel( s_##_name, _coord, _level )
	#define SAMPLE_LEVEL( _texture, _sampler, _coord, _level )	_texture.SampleLevel( _sampler, _coord, _level )
	#define SAMPLE_CMP_LEVEL0( _texture, _sampler, _coord, _compareValue ) _texture.SampleCmpLevelZero( _sampler, _coord, _compareValue )
	#define SAMPLE_GRADIENT( _texture, _sampler, _coord, _ddx, _ddy ) _texture.SampleGrad( _sampler, _coord, _ddx, _ddy )
	#define SYS_STATIC static
	#define SAMPLE_MIPMAPS( _texture, _miplevel, _location ) _texture.mips[_miplevel][ _location ]
#endif

// ============================
// Constant buffer block defines

#ifdef __PSSL__
	#define START_CB( _name, _reg )	ConstantBuffer	_name : register( b##_reg )	 {		// PSSL
#else
	#define START_CB( _name, _reg )	cbuffer			_name : register( b##_reg )	 {		// DX11
#endif

#define END_CB  }																	// PSSL + DX11

// ============================
// PER RENDERING API SEMANTICS DEFINES

#ifdef __PSSL__
	#define SYS_POSITION					S_POSITION
	#define SYS_TARGET_OUTPUT0				S_TARGET_OUTPUT0
	#define SYS_TARGET_OUTPUT1				S_TARGET_OUTPUT1
	#define SYS_TARGET_OUTPUT2				S_TARGET_OUTPUT2
	#define SYS_TARGET_OUTPUT3				S_TARGET_OUTPUT3
	#define SYS_TARGET_OUTPUT4				S_TARGET_OUTPUT4
	#define SYS_TARGET_OUTPUT5				S_TARGET_OUTPUT5
	#define SYS_TARGET_OUTPUT6				S_TARGET_OUTPUT6
	#define SYS_TARGET_OUTPUT7				S_TARGET_OUTPUT7
	#define SYS_DEPTH_OUTPUT				S_DEPTH_OUTPUT
	#define SYS_VERTEX_ID					S_VERTEX_ID
	#define SYS_INSTANCE_ID					S_INSTANCE_ID
	#define SYS_PRIMITIVE_ID				S_PRIMITIVE_ID
	#define SYS_GSINSTANCE_ID				S_GSINSTANCE_ID
	#define SYS_OUTPUT_CONTROL_POINT_ID		S_OUTPUT_CONTROL_POINT_ID
	#define SYS_EDGE_TESS_FACTOR			S_EDGE_TESS_FACTOR
	#define SYS_INSIDE_TESS_FACTOR			S_INSIDE_TESS_FACTOR
	#define SYS_DOMAIN_LOCATION				S_DOMAIN_LOCATION
	#define SYS_FRONT_FACE					S_FRONT_FACE
	#define SYS_COVERAGE					S_COVERAGE
	#define SYS_CLIP_DISTANCE0				S_CLIP_DISTANCE0
	#define SYS_CLIP_DISTANCE1				S_CLIP_DISTANCE1
	#define SYS_CULL_DISTANCE0				S_CULL_DISTANCE0
	#define SYS_CULL_DISTANCE1				S_CULL_DISTANCE1
	#define SYS_RENDER_TARGET_ARRAY_INDEX	S_RENDER_TARGET_INDEX
	#define SYS_VIEWPORT_ARRAY_INDEX		S_VIEWPORT_INDEX
	#define SYS_DISPATCH_THREAD_ID			S_DISPATCH_THREAD_ID
	#define SYS_GROUP_ID					S_GROUP_ID
	#define SYS_GROUP_INDEX					S_GROUP_INDEX
	#define SYS_GROUP_THREAD_ID				S_GROUP_THREAD_ID

	// textures
	#define TEXTURE2D						Texture2D
	#define TEXTURE2D_MS					MS_Texture2D
	#define TEXTURE2D_ARRAY					Texture2D_Array
	#define TEXTURE2D_ARRAY_MS				MS_Texture2D_Array
	#define TEXTURECUBE						TextureCube
	#define TEXTURECUBE_ARRAY				TextureCube_Array
	#define RW_TEXTURE2D					RW_Texture2D
	#define RW_BYTEBUFFER					RW_ByteBuffer
	#define RW_STRUCTBUFFER(_type)			RW_RegularBuffer< _type >

	// hull,domain,geometry,compute shaders
	//#define MAX_VERTEX_COUNT				MAX_VERTEX_COUNT
	#define GS_INPUT_TRIANGLE				Triangle
	#define GS_INPUT_POINT					Point
	#define GS_BUFFER_POINT					PointBuffer
	#define GS_BUFFER_LINE					LineBuffer
	#define GS_BUFFER_TRIANGLE				TriangleBuffer
	//#define DOMAIN_PATCH_TYPE				DOMAIN_PATCH_TYPE
	#define HS_PARTITIONING					PARTITIONING_TYPE
	#define HS_OUTPUT_TOPOLOGY				OUTPUT_TOPOLOGY_TYPE
	#define HS_OUTPUT_CONTROL_POINTS		OUTPUT_CONTROL_POINTS
	#define HS_PATCH_CONSTANT_FUNC			PATCH_CONSTANT_FUNC
	#define HS_MAX_TESS_FACTOR				MAX_TESS_FACTOR

	#define NUMTHREADS						NUM_THREADS
	#define GROUPSHARED						thread_group_memory
	#define	NOINTERPOLATION					nointerp

	// synchronization
	#define INTERLOCKED_ADD( _dest, _val, _prev )	AtomicAdd( _dest, _val, _prev )
	#define INTERLOCKED_MIN( _dest, _val )	AtomicMin( _dest, _val )
	#define INTERLOCKED_MAX( _dest, _val )	AtomicMax( _dest, _val )

	#define GROUP_BARRIER_GROUP_SYNC		ThreadGroupMemoryBarrierSync()

	// Bitwise ops
	#define REVERSE_BITS					ReverseBits
	
	// attributes
	#define EARLY_DEPTH_STENCIL				FORCE_EARLY_DEPTH_STENCIL
	
	// parameter modifiers
	#define nointerpolation					nointerp
	#define noperspective					nopersp
	#define PARAM_NOINTERP					nointerp
	#define PARAM_NOPERSP					nopersp
	
#else

	#define SYS_POSITION					SV_Position
	#define SYS_TARGET_OUTPUT0				SV_Target0
	#define SYS_TARGET_OUTPUT1				SV_Target1
	#define SYS_TARGET_OUTPUT2				SV_Target2
	#define SYS_TARGET_OUTPUT3				SV_Target3
	#define SYS_TARGET_OUTPUT4				SV_Target4
	#define SYS_TARGET_OUTPUT5				SV_Target5
	#define SYS_TARGET_OUTPUT6				SV_Target6
	#define SYS_TARGET_OUTPUT7				SV_Target7
	#define SYS_DEPTH_OUTPUT				SV_Depth
	#define SYS_VERTEX_ID					SV_VertexID
	#define SYS_INSTANCE_ID					SV_InstanceID
	#define SYS_PRIMITIVE_ID				SV_PrimitiveID
	#define SYS_GSINSTANCE_ID				SV_GSInstanceID
	#define SYS_OUTPUT_CONTROL_POINT_ID		SV_OutputControlPointID
	#define SYS_EDGE_TESS_FACTOR			SV_TessFactor
	#define SYS_INSIDE_TESS_FACTOR			SV_InsideTessFactor
	#define SYS_DOMAIN_LOCATION				SV_DomainLocation
	#define SYS_FRONT_FACE					SV_IsFrontFace
	#define SYS_COVERAGE					SV_Coverage
	#define SYS_CLIP_DISTANCE0				SV_ClipDistance0
	#define SYS_CLIP_DISTANCE1				SV_ClipDistance1
	#define SYS_CULL_DISTANCE0				SV_CullDistance0
	#define SYS_CULL_DISTANCE1				SV_CullDistance1
	#define SYS_RENDER_TARGET_ARRAY_INDEX	SV_RenderTargetArrayIndex	
	#define SYS_VIEWPORT_ARRAY_INDEX		SV_ViewportArrayIndex
	#define SYS_DISPATCH_THREAD_ID			SV_DispatchThreadID
	#define SYS_GROUP_ID					SV_GroupID
	#define SYS_GROUP_INDEX					SV_GroupIndex
	#define SYS_GROUP_THREAD_ID				SV_GroupThreadID
	
	// textures
	#define TEXTURE2D						Texture2D
	#define TEXTURE2D_MS					Texture2DMS
	#define TEXTURE2D_ARRAY					Texture2DArray
	#define TEXTURE2D_ARRAY_MS				Texture2DMSArray
	#define TEXTURECUBE						TextureCube
	#define TEXTURECUBE_ARRAY				TextureCubeArray
	#define RW_TEXTURE2D					RWTexture2D
	#define RW_BYTEBUFFER					RWByteAddressBuffer
	#define RW_STRUCTBUFFER(_type)			RWStructuredBuffer< _type >

	// hull,domain,geometry,compute shaders
	#define MAX_VERTEX_COUNT				maxvertexcount
	#define GS_INPUT_TRIANGLE				triangle
	#define GS_INPUT_POINT					point
	#define GS_BUFFER_POINT					PointStream
	#define GS_BUFFER_LINE					LineStream
	#define GS_BUFFER_TRIANGLE				TriangleStream
	#define DOMAIN_PATCH_TYPE				domain
	#define HS_PARTITIONING					partitioning
	#define HS_OUTPUT_TOPOLOGY				outputtopology
	#define HS_OUTPUT_CONTROL_POINTS		outputcontrolpoints
	#define HS_PATCH_CONSTANT_FUNC			patchconstantfunc
	#define HS_MAX_TESS_FACTOR				maxtessfactor
	#define NUMTHREADS						numthreads
	#define GROUPSHARED						groupshared
	#define	NOINTERPOLATION					nointerpolation

	// synchronization
	#define INTERLOCKED_ADD( _dest, _val, _prev )	InterlockedAdd( _dest, _val, _prev )
	#define INTERLOCKED_MIN( _dest, _val )	InterlockedMin( _dest, _val )
	#define INTERLOCKED_MAX( _dest, _val )	InterlockedMax( _dest, _val )

	#define GROUP_BARRIER_GROUP_SYNC		GroupMemoryBarrierWithGroupSync()

	// Bitwise ops
	#define REVERSE_BITS					reversebits
	
	// attributes
	#define EARLY_DEPTH_STENCIL				earlydepthstencil

	// parameter modifiers
	#define PARAM_NOINTERP					nointerpolation
	#define PARAM_NOPERSP					noperspective
	
#endif

// ============================

#define PI				3.14159265358979323846264338327f
#define HALF_PI			1.570796326794896619231321691635f
#define DEG2RAD( x ) ( ((x) / 180.0f) * PI )

// Wrapper for custom user defined constants
#define custom_register( x ) : register(c##x)

// Render states, defined by compiler
//#define RS_TERRAIN_TOOL_ACTIVE
//#define RS_USE_SHADOW_MASK
//#define RS_PASS_NO_LIGHTING
//#define RS_PASS_HIT_PROXIES
//#define RS_PASS_DEFERRED_LIGHTING
//#define RS_PASS_BASE_LIGHTING
//#define RS_PASS_POINT_LIGHTING
//#define RS_PASS_SPOT_LIGHTING
//#define RS_PASS_SHADOW_DEPTH
//#define RS_PASS_LIGHT_PRESPASS
//#define RS_TWOSIDED

// Material states, defined by compiler
//#define MS_SELECTED

#endif
/// List of SYSTEM GLOBAL constants for vertex and pixel shader
/// It's esential to recompile all shaders after changing any of those lines

/// Common header for all dynamically compiled shaders
/// It's esential to recompile all shaders after changing any of those lines
#ifndef GLOBAL_CONSTANTS_FX_H_INCLUDED
#define GLOBAL_CONSTANTS_FX_H_INCLUDED

#ifdef __cplusplus
# define BUILD_RGB_LUMINANCE_WEIGHTS( x, y, z )				Vector( x, y, z, 0.f )
#else
# define BUILD_RGB_LUMINANCE_WEIGHTS( x, y, z )				float3( x, y, z )
#endif

////////////////////////////////////////////////
/// Gamma related parameters
////////////////////////////////////////////////

#define GAMMA_TO_LINEAR_EXPONENT		2.2
#define GAMMA_TO_LINEAR_EXPONENT_INV	(1.0 / GAMMA_TO_LINEAR_EXPONENT)


////////////////////////////////////////////////
/// Luminance related parameters
////////////////////////////////////////////////

#define RGB_LUMINANCE_WEIGHTS_LINEAR						BUILD_RGB_LUMINANCE_WEIGHTS( 0.2126f, 0.7152f, 0.0722f )
#define RGB_LUMINANCE_WEIGHTS_GAMMA							BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )

#define RGB_LUMINANCE_WEIGHTS_LINEAR_Histogram				RGB_LUMINANCE_WEIGHTS_LINEAR
#define RGB_LUMINANCE_WEIGHTS_LINEAR_ApplyFogDataFull		BUILD_RGB_LUMINANCE_WEIGHTS( 0.333, 0.555, 0.222 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_Sepia					BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_ParametricBalance		BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333/1.1, 0.5555/1.1, 0.2222/1.1 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_FlareGrab				BUILD_RGB_LUMINANCE_WEIGHTS( 0.3f, 0.59f, 0.11f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_FlareApply				BUILD_RGB_LUMINANCE_WEIGHTS( .3f, .59f, .11f )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_DofScatter				BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333, 0.3333, 0.3333 )
#define RGB_LUMINANCE_WEIGHTS_LINEAR_DownsampleLum4			BUILD_RGB_LUMINANCE_WEIGHTS( 0.2126f, 0.7152f, 0.0722f )

#ifdef __cplusplus
# define RGB_LUMINANCE_WEIGHTS_MatBlockDesaturate			BUILD_RGB_LUMINANCE_WEIGHTS( 0.299f, 0.587f, 0.114f )
# define RGB_LUMINANCE_WEIGHTS_EnvTransparencyColorFilter	BUILD_RGB_LUMINANCE_WEIGHTS( 0.33f, 0.34f, 0.33f )
# define RGB_LUMINANCE_WEIGHTS_GAMMA_SelectionDesaturate	BUILD_RGB_LUMINANCE_WEIGHTS( 0.3333f, 0.3334f, 0.3333f )
# define RGB_LUMINANCE_WEIGHTS_COLOR						Color ( 54, 182, 19 )
# define RGB_LUMINANCE_WEIGHTS_COLOR_GAMMA					Color ( 76, 149, 30 )
#endif


////////////////////////////////////////////////
/// Various parameters
////////////////////////////////////////////////


#define DISSOLVE_TEXTURE_SIZE				16
#define POISSON_ROTATION_TEXTURE_SIZE		32
#define VOLUME_TEXTURE_SLOT					24

// Poisson samples count may need binaries recompilation...
#define POISSON_SHADOW_SAMPLES_COUNT		16
#define POISSON_SHADOW_SAMPLES_COUNT_2		8

// Maximum accumulative refraction offset. Lower values give better offseting precision, 
// but limits offset range and may cause some inaccuracies when multiple refractive surfaces 
// overlap. Feel free to change it as you want - there shouldn't be any dependencies except 
// of materials compiled in code.
#define ACCUMULATIVE_REFRACTION_MAX_OFFSET	0.15

#define WEATHER_VOLUMES_SIZE_DIV			2
#define WEATHER_VOLUMES_SIZE_MUL			(1.f / WEATHER_VOLUMES_SIZE_DIV)

#define GLOBAL_SHADOW_BUFFER_CHANNEL_SSAO		0
#define GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW		1

#define SHARED_CONSTS_COLOR_GROUPS_CAPACITY		39

////////////////////////////////////////////////
/// Realtime reflections
////////////////////////////////////////////////

#define REALTIME_REFLECTIONS_SIZE_DIV			4
#define REALTIME_REFLECTIONS_MASK_GAMMA			2.0
#define REALTIME_REFLECTIONS_MASK_GAMMA_INV		(1.0 / REALTIME_REFLECTIONS_MASK_GAMMA)

#define REALTIME_REFLECTIONS_MAX_OFFSET_VALUE		0.2
#define REALTIME_REFLECTIONS_MAX_OFFSET_VALUE_INV	(1.0 / REALTIME_REFLECTIONS_MAX_OFFSET_VALUE)

#ifndef __cplusplus
// Info about the input offset should be in rlr_apply.fx shader.
float4 EncodeRealtimeReflectionsMask( float2 offset, float amount )
{
	float2 encodedOffset = offset * (REALTIME_REFLECTIONS_MAX_OFFSET_VALUE_INV * 0.5) + 0.5;
	float encodedAmount = pow( max( 0, amount ), REALTIME_REFLECTIONS_MASK_GAMMA_INV );
	return float4( encodedOffset, 0, encodedAmount );
}
#endif

////////////////////////////////////////////////
/// Light channels
////////////////////////////////////////////////


#ifndef __cplusplus
# define LC_Characters	(1 << 1)
#endif


////////////////////////////////////////////////

// Base capacity is 64
#define HISTOGRAM_CAPACITY_MULTIPLIER		4


////////////////////////////////////////////////


#endif

/// Register
#ifdef __cplusplus

	#ifndef REG
		#define REG( _name, _reg, _type ) const Uint32 _name = (_reg);
		#define REGI( _name, _reg, _type ) const Uint32 _name = (_reg);
		#define REGB( _name, _reg, _type ) const Uint32 _name = (_reg);
		#define REG_ARRAY( _name,_reg, _type, _size ) const Uint32 _name = (_reg);
		#define SYS_SAMPLER( _name,_reg ) const Uint32 _name = (_reg);
		#define VERTEXSHADER 1
		#define PIXELSHADER 1
	#endif

	#ifndef START_CB
		#define START_CB( _name, _reg )		// EMPTY
		#define END_CB						// EMPTY
	#endif
															
#endif

////////////////////////////////////////////////
/// Pixel shader samplers
////////////////////////////////////////////////

/// Common pixel shader sampler indices
SYS_SAMPLER( PSSMP_NormalsFitting,			13	)
SYS_SAMPLER( PSSMP_SceneColor,				6	)
SYS_SAMPLER( PSSMP_AtlasMapping,			7	)
SYS_SAMPLER( PSSMP_SceneDepth,				7	)
SYS_SAMPLER( PSSMP_Dissolve,				7	)
SYS_SAMPLER( PSSMP_UVDissolve,				8	)
SYS_SAMPLER( PSSMP_GlobalShadowAndSSAO,		12	)

////////////////////////////////////////////////
/// Pixel shader constants
////////////////////////////////////////////////

	// ace_todo!!! duzo z tych rzeczy mozna wyrzucic, bo zostawilismy samego defereda
	
#ifdef PIXELSHADER


START_CB( GlobalPixelConsts, 0 )
/// Common pixel shader constants
REG( PSC_ConstColor, 0, float4 )						// frequent
REG( PSC_HitProxyColor, 1, float4 )						// frequent
REG( PSC_TimeVector, 2, float4 )						// global
REG( PSC_CameraPosition, 3, float4 )					// camera
REG( PSC_ViewportSize, 4, float4 )						// global
REG( PSC_CameraVectorRight, 5, float4 )					// camera
REG( PSC_CameraVectorForward, 6, float4 )				// camera
REG( PSC_CameraVectorUp, 7, float4 )					// camera
REG( PSC_CameraNearFar, 8, float4 )						// camera
REG( PSC_ColorOne, 9, float4x4 )						// frequent				[MATRIX]
REG( PSC_ColorTwo, 13, float4x4 )						// frequent				[MATRIX]
REG( PSC_AtlasOffsets, 17, float4 )						// old
REG( PSC_AtlasSize, 18, float4 )						// old
REG( PSC_TerrainLayerHighlight, 19, float4 )			// old
REG( PSC_EnvTranspFilterDistMinColor, 20, float4 )		// global
REG( PSC_WeatherAndPrescaleParams, 21, float4 )			// global
REG( PSC_Custom0, 22, float4 )							// freq
REG( PSC_Custom1, 23, float4 )							// freq
REG( PSC_ViewportSubSize, 24, float4 )					// ? camera or global ?
REG( PSC_GlobalLightDir, 25, float4 )					// global
REG( PSC_GlobalLightColorAndSpecScale, 26, float4 )		// global
REG( PSC_WaterShadingParams, 27, float4 )				// global
REG( PSC_WaterSimulationParams, 28, float4 )			// global
REG( PSC_TransparencyParams, 29, float4 )				// freq
REG( PSC_VSMData, 30, float4 )							// freq
REG( PSC_WaterShadingParamsExtra, 31, float4 )			// global
REG( PSC_SelectionEffect, 32, float4 )					// frequent
REG( PSC_GameplayEffectsRendererParameter, 33, float4 )	// global
REG( PSC_Custom2, 34, float4 )							// frequent
REG( PSC_SunSpecularColor, 35, float4 )					// old
REG( PSC_EnvTranspFilterParams, 36, float4 )			// global
REG( PSC_EnvTranspFilterDistMaxColor, 37, float4 )		// global

REG( PSC_CameraScreenToWorld, 38, float4x4 )			// camera				[MATRIX]
REG( PSC_HeadCenterPosAndDimming, 42, float4 )			// freq (skinned)
REG( PSC_HeadCenterAxisX, 43, float4 )					// freq (skinned)
REG( PSC_HeadCenterAxisY, 44, float4 )					// freq (skinned)
REG( PSC_HeadCenterAxisZ, 45, float4 )					// freq (skinned)
REG( PSC_MorphRatio, 46, float4 )						// (per morphed mesh)
REG( PSC_WeatherShadingParams, 47, float4 )					// 1 per frame
REG( PSC_SkyboxShadingParams, 48, float4 )				// 1 per frame
REG( PSC_CharactersLightingBoost, 49, float4 )			// freq
REG( PSC_FXColor, 50, float4 )							// freq
// !! >> DONT FORGET ABOUT PSC_AtlasDataFirstReg here << :)
END_CB

START_CB( CustomPixelConsts, 1 )
REG( PSC_Custom_0 ,  58, float4 )
REG( PSC_Custom_1 ,  59, float4 )
REG( PSC_Custom_2 ,  60, float4 )
REG( PSC_Custom_3 ,  61, float4 )
REG( PSC_Custom_4 ,  62, float4 )
REG( PSC_Custom_5 ,  63, float4 )
REG( PSC_Custom_6 ,  64, float4 )
REG( PSC_Custom_7 ,  65, float4 )
REG( PSC_Custom_8 ,  66, float4 )
REG( PSC_Custom_9 ,  67, float4 )
REG( PSC_Custom_10,  68, float4 )
REG( PSC_Custom_11,  69, float4 )
REG( PSC_Custom_12,  70, float4 )
REG( PSC_Custom_13,  71, float4 )
REG( PSC_Custom_14,  72, float4 )
REG( PSC_Custom_15,  73, float4 )
REG( PSC_Custom_16,  74, float4 )
REG( PSC_Custom_17,  75, float4 )
REG( PSC_Custom_18,  76, float4 )
REG( PSC_Custom_19,  77, float4 )
REG( PSC_Custom_20,  78, float4 )
REG( PSC_Custom_21,  79, float4 )
REG( PSC_Custom_22,  80, float4 )
REG( PSC_Custom_23,  81, float4 )
REG( PSC_Custom_24,  82, float4 )
REG( PSC_Custom_25,  83, float4 )
REG( PSC_Custom_26,  84, float4 )
REG( PSC_Custom_27,  85, float4 )
REG( PSC_Custom_28,  86, float4 )
REG( PSC_Custom_29,  87, float4 )
REG( PSC_Custom_30,  88, float4 )
REG( PSC_Custom_31,  89, float4 )
REG( PSC_Custom_Matrix, 90, float4x4 )
#ifndef __cplusplus
float4   vShadowParams[4];
#else
REG( PSC_Custom_ShadowParams0,  94, float4 )		// unused
REG( PSC_Custom_ShadowParams1,  95, float4 )		// unused
REG( PSC_Custom_ShadowParams2,  96, float4 )		// unused
REG( PSC_Custom_ShadowParams3,  97, float4 )		// unused
#endif
END_CB
#endif

#define PSC_Custom_EyeOrientationLeft_AxisX		PSC_Custom_10
#define PSC_Custom_EyeOrientationLeft_AxisY		PSC_Custom_11
#define PSC_Custom_EyeOrientationLeft_AxisZ		PSC_Custom_12
#define PSC_Custom_EyeOrientationRight_AxisX	PSC_Custom_13
#define PSC_Custom_EyeOrientationRight_AxisY	PSC_Custom_14
#define PSC_Custom_EyeOrientationRight_AxisZ	PSC_Custom_15
#define PSC_Custom_HiResShadowsParams			PSC_Custom_20

////////////////////////////////////////////////
/// Pixel shader custom constants
////////////////////////////////////////////////


//atlas data maps first 32 custom registers
#define PSC_NumAtlasDataRegs	32
#define PSC_AtlasDataFirstReg	57
#define PSC_AtlasDataFirstRegId	c52
#ifndef UTILITIES_H_INCLUDED
#define UTILITIES_H_INCLUDED

#define YCBCR_INTERLEAVE_HORIZONTAL 0

float3 PackYCbCr( float3 col )
{
   float3 _packed = float3(	0.299f * col.r + 0.587f * col.g + 0.114f * col.b,   
							0.5f + (-0.168 * col.r - 0.331f * col.g + 0.5f * col.b),
							0.5f + (0.5 * col.r - 0.418f * col.g - 0.081f * col.b) );
   //_packed = saturate( _packed );
   return _packed;
}

float3 UnpackYCbCr( float3 YCbCr )
{
   float y  = YCbCr.x;
   float cb = YCbCr.y;
   float cr = YCbCr.z;
   float3 unpacked = float3 ( 
      y + 1.402 * (cr - 0.5),
      y - 0.344 * (cb - 0.5) - 0.714 * (cr - 0.5),
      y + 1.772 * (cb - 0.5) );
   unpacked = saturate( unpacked );
   return unpacked;
}

float2 EncodeTwoChannelsYCbCr( float3 col, uint2 vpos )
{
	float3 yCbCr = PackYCbCr( saturate( col ) );
	
	// funny note: (0 == (vpos.y % 2)) won't work for speedtree shaders compilation.
	//             maybe because the "dx9 compatibility" shaders compilation setting is disabled?
#if YCBCR_INTERLEAVE_HORIZONTAL
	return float2( yCbCr.x, (0 == (vpos.y & 1)) ? yCbCr.y :  yCbCr.z );
#else
	return float2( yCbCr.x, (0 == (vpos.y & 1)) ? yCbCr.y :  yCbCr.z );
#endif
}

float3 SampleTwoChannelYCbCr( Texture2D<float4> _tex, uint2 pixelCoord, out float4 outEncodedValue )
{
	float4 v = _tex[pixelCoord];
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex[pixelCoord + uint2(1,0)].y;
	else							v.yz = float2 ( _tex[pixelCoord - uint2(1,0)].y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex[pixelCoord + uint2(0,1)].y;
 	else							v.yz = float2 ( _tex[pixelCoord - uint2(0,1)].y, v.y );
#endif
	
	return v.xyz;
}

float3 SampleTwoChannelYCbCrArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord, out float4 outEncodedValue )
{
	uint3 samplePixelCoord = uint3(pixelCoord, sliceIndex);
	float4 v = _tex[samplePixelCoord];
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex[samplePixelCoord + uint3(1,0,0)].y;
	else							v.yz = float2 ( _tex[samplePixelCoord - uint3(1,0,0)].y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex[samplePixelCoord + uint3(0,1,0)].y;
 	else							v.yz = float2 ( _tex[samplePixelCoord - uint3(0,1,0)].y, v.y );
#endif
	
	return v.xyz;
}

float3 DecodeTwoChannelColor( Texture2D<float4> _tex, uint2 pixelCoord, out float4 outEncodedValue )
{
	return UnpackYCbCr( SampleTwoChannelYCbCr( _tex, pixelCoord, outEncodedValue ) );
}

float3 DecodeTwoChannelColorArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord, out float4 outEncodedValue )
{
	return UnpackYCbCr( SampleTwoChannelYCbCrArray( _tex, sliceIndex, pixelCoord, outEncodedValue ) );
}

float3 DecodeTwoChannelColor( TEXTURE2D_MS<float4> _tex, uint2 pixelCoord, int sampleIndex, out float4 outEncodedValue )
{
	float4 v = _tex.Load( (int2)pixelCoord, sampleIndex );
	outEncodedValue = v;

#if YCBCR_INTERLEAVE_HORIZONTAL
	if ( 0 == pixelCoord.x % 2 )	v.z  = _tex.Load( pixelCoord + uint2(1,0), sampleIndex ).y;
	else							v.yz = float2 ( _tex.Load(pixelCoord - int2(1,0), sampleIndex ).y, v.y );
#else
 	if ( 0 == pixelCoord.y % 2 )	v.z  = _tex.Load( (int2)pixelCoord + int2(0,1), sampleIndex ).y;
 	else							v.yz = float2 ( _tex.Load((int2)pixelCoord - int2(0,1), sampleIndex ).y, v.y );
#endif
	
	return UnpackYCbCr( v.xyz );
}

float3 DecodeTwoChannelColor( Texture2D<float4> _tex, uint2 pixelCoord )
{
	float4 _decoy;
	return DecodeTwoChannelColor( _tex, pixelCoord, _decoy );
}

float3 DecodeTwoChannelColorArray( TEXTURE2D_ARRAY<float4> _tex, uint sliceIndex, uint2 pixelCoord )
{
	float4 _decoy;
	return DecodeTwoChannelColorArray( _tex, sliceIndex, pixelCoord, _decoy );
}

float CalcCubeMipLevel( int resolution, float3 N, float3 N2 )
{
	float a = acos( dot( N, N2 ) );
	float pd = a / (0.5 * 3.14159285) * resolution;
	return max( 0, log2( pd ) );
}

float CalcCubeMipLevel( int resolution, float3 N, int2 pixelCoord )
{
	N = normalize( N );

#ifdef VERTEXSHADER
	// VS does not support ddx/ddy...
	return CalcCubeMipLevel(resolution, N, N);
#else
	float3 dx = ddx( N );
	float3 dy = ddy( N );

	if ( (pixelCoord.x & 1) )
		dx *= -1;
	if ( (pixelCoord.y & 1) )
		dy *= -1;

	// ace_ibl_optimize: optimize this (we can merge stuff from inside of CalcCubeMipLevel)
	
	return max( CalcCubeMipLevel( resolution, N, normalize(N+dx) ), CalcCubeMipLevel( resolution, N, normalize(N+dy) ) );
#endif
}

float SpeedTreeCalcRandomFactor( float3 pos )
{
   float period = 9.51462632126623;
   pos.xy = fmod( pos.xy, period );
   
   float2 p = pos.xy + float2(pos.z, 0);
   const float2 r = float2( 23.1406926327792690, 2.6651441426902251 );
   return frac( cos( fmod( 123456789.0, 1e-7 + 256.0 * dot(p,r) ) ) );  
}

float4 SpeedTreeCalcRandomColorData( float3 pos )
{
   float3 colorWeights = float3 (
      SpeedTreeCalcRandomFactor( pos ),
      SpeedTreeCalcRandomFactor( pos + 3.6979714896946124 ),
      SpeedTreeCalcRandomFactor( pos + 2.5654710701512515 ) );
   colorWeights /= max( 0.00001, dot( colorWeights, 1 ) );

   return float4 ( colorWeights, 1.0 );
}

#define DEFINE_IS_TEXTURE_BOUND_FUNCTION( _TexType )	\
	bool IsTextureBound( _TexType tex )					\
	{													\
		uint w, h;										\
		tex.GetDimensions( w, h );						\
		return w > 0;									\
	}

DEFINE_IS_TEXTURE_BOUND_FUNCTION( TextureCube )
DEFINE_IS_TEXTURE_BOUND_FUNCTION( Texture2D )

bool IsTextureBound( TEXTURECUBE_ARRAY tex )
{										
	uint w, h, e;							
	tex.GetDimensions( w, h, e );
	return w > 0;						
}

float4 ParaboloidToCube( float2 uv, int idx )
{
	uv = uv * 2 - 1;

	float3 dir;
	dir.x = 2.0f * uv.x;
	dir.y = -2.0f * uv.y;
	dir.z = -1.f + dot(uv,uv);
	dir *= (idx ? 1.f : -1.f);
	dir /= (dot(uv,uv) + 1.f);
	float alpha = (dot(uv,uv) <= 1) ? 1 : 0;	

	return float4 ( dir, alpha );
}

int GetCubeDirParaboloidIndex( float3 dir )
{
	return dir.z < 0 ? 1 : 0;
}	

/// 'dir' MUST BE NORMALIZED !!!
float2 CubeToParaboloid( float3 dir, int idx )
{
	float2 uv0 = float2( 0.5, -0.5 ) * dir.xy / max(0.001, 1.0 - dir.z) + 0.5;
 	float2 uv1 = float2( -0.5, 0.5 ) * dir.xy / max(0.001, 1.0 + dir.z) + 0.5;
	return idx ? uv0 : uv1;
}

/// 'dir' MUST BE NORMALIZED !!!
float2 CubeToParaboloid( float3 dir )
{
	return CubeToParaboloid( dir, GetCubeDirParaboloidIndex( dir ) );
}

#endif
#ifndef SHARED_PIXEL_CONSTS_INCLUDED
#define SHARED_PIXEL_CONSTS_INCLUDED

#define CUBE_ARRAY_CAPACITY			7


struct ShaderCullingEnvProbeParams
{
	float4x3	viewToLocal;
	float3		normalScale;
	uint		probeIndex;				// ace_todo: this adds some redundancy
};

struct ShaderCommonEnvProbeParams
{
	float		weight;
	float3		probePos;
	float3		areaMarginScale;
	float4x4	areaWorldToLocal;
	float4		intensities;
	float4x4	parallaxWorldToLocal;	
	uint		slotIndex;
};

START_CB( SharedPixelConsts, 12 )
	// generic
	float4 cameraPosition;
	float4x4 worldToView;
	float4x4 screenToView_UNUSED;
	float4x4 viewToWorld;
	float4x4 projectionMatrix;
	float4x4 screenToWorld;
	float4 cameraNearFar;
	float4 cameraDepthRange;						//< ace_todo: unused -> remove
	float4 screenDimensions;	
	float4 numTiles;	
	float4x4 lastFrameViewReprojectionMatrix;
	float4x4 lastFrameProjectionMatrix;
	float4 localReflectionParam0;
	float4 localReflectionParam1;

	// lighting
	float4 translucencyColorFront;			//< ace_todo: unused -> remove
	float4 translucencyParams0;
	float4 translucencyParams1;	
	
	// fog
	float4 fogSunDir;
	float4 fogColorFront;
	float4 fogColorMiddle;
	float4 fogColorBack;
	float4 fogBaseParams;
	float4 fogDensityParamsScene;
	float4 fogDensityParamsSky;			
	float4 aerialColorFront;
	float4 aerialColorMiddle;
	float4 aerialColorBack;
	float4 aerialParams;

	// speed tree
	float4 speedTreeBillboardsParams;
	float4 speedTreeParams;
	float4 speedTreeRandomColorLumWeightsTrees;
	float4 speedTreeRandomColorParamsTrees0;
	float4 speedTreeRandomColorParamsTrees1;
	float4 speedTreeRandomColorParamsTrees2;
	float4 speedTreeRandomColorLumWeightsBranches;
	float4 speedTreeRandomColorParamsBranches0;
	float4 speedTreeRandomColorParamsBranches1;
	float4 speedTreeRandomColorParamsBranches2;
	float4 speedTreeRandomColorLumWeightsGrass;
	float4 speedTreeRandomColorParamsGrass0;
	float4 speedTreeRandomColorParamsGrass1;
	float4 speedTreeRandomColorParamsGrass2;

	//
	float4 terrainPigmentParams;
	float4 speedTreeBillboardsGrainParams0;
	float4 speedTreeBillboardsGrainParams1;

	// weather env / blending params
	float4 weatherAndPrescaleParams;
	float4 skyboxShadingParams;
	
	// ssao
	float4 ssaoParams;

	// msaaParams
	float4 msaaParams;

	// envmap
	float4 envMapColorLight;			//< ace_todo: unused -> remove
	float4 envMapColorShadow;			//< ace_todo: unused -> remove

	//
	float4 surfaceDimensions;

	// envprobes
	int								numCullingEnvProbeParams;						// ace_todo: remove culling params when I'm convinced we're dripping tiled culling for envProbes
	ShaderCullingEnvProbeParams		cullingEnvProbeParams[CUBE_ARRAY_CAPACITY - 1];	// doesn't include global probe
	ShaderCommonEnvProbeParams		commonEnvProbeParams[CUBE_ARRAY_CAPACITY];

	//
	float4 pbrSimpleParams0;
	float4 pbrSimpleParams1;

	//
	float4 cameraFOV;

	//
	float4 ssaoClampParams0;
	float4 ssaoClampParams1;
	
	//
	float4 fogCustomValuesEnv0;
	float4 fogCustomRangesEnv0;
	float4 fogCustomValuesEnv1;
	float4 fogCustomRangesEnv1;
	float4 mostImportantEnvsBlendParams;

	float4 fogDensityParamsClouds;

	// sky
	float4 skyColor;
	float4 skyColorHorizon;
	float4 sunColorHorizon;
	float4 sunBackHorizonColor;
	float4 sunColorSky;
	float4 skyParams1;
	float4 skyParams2;

	//
	float4x4 screenToWorldRevProjAware;

	//
	float4 lightColorParams;
	
	//
	float4 colorGroups[SHARED_CONSTS_COLOR_GROUPS_CAPACITY];
END_CB

#define GBUFF_MATERIAL_MASK_DEFAULT				0
#define GBUFF_MATERIAL_MASK_ENCODED_DEFAULT		0
#define GBUFF_MATERIAL_FLAG_GRASS				1
#define GBUFF_MATERIAL_FLAG_TREES				2
#define GBUFF_MATERIAL_FLAG_BRANCHES			4
#define GBUFF_MATERIAL_FLAG_BILLBOARDS			8

float EncodeGBuffMaterialFlagsMask( uint mask )
{
	return (mask + 0.5) / 255.f;
}

uint DecodeGBuffMaterialFlagsMask( float encodedMask )
{
	return (uint)(encodedMask * 255 + 0.5);
}

float3 CalcSkyColor( in float3 worldPos, in float3 worldDirection )
{
	const float3 light_dir = skyParams2.yzw;

	float3 overallColor = skyColor.xyz;
	float3 worldVec = normalize(worldDirection);
	float sunDot_xy = dot(normalize(light_dir.xy),normalize(worldVec.xy));
	float sunDot_xyz = dot(light_dir.xyz,worldVec.xyz);

	float3 sunHorizonEffect = lerp( sunBackHorizonColor.xyz, sunColorHorizon.xyz, sunDot_xy *0.5f + 0.5f );
	float3 horizonColor = lerp(skyColorHorizon.xyz, sunHorizonEffect, ( sunDot_xy * sunDot_xy ) * (1 - worldVec.z * worldVec.z ) * skyParams1.w);
	float sunEffect = pow( saturate( sunDot_xyz ), skyParams2.x );
	overallColor = lerp(overallColor,sunColorSky.xyz,sunEffect);

	worldPos = cameraPosition.xyz + 1000 * normalize(worldPos - cameraPosition.xyz);

	float hor_factor = saturate( 1.0 / max( 0.0001, 0.1 + ((worldPos.z + 710) / 1000.0) * skyParams1.z ) );
	hor_factor = pow( hor_factor, 2.8 );
	hor_factor = 1 - hor_factor;

	float3 colorWithHorizon = lerp(horizonColor,overallColor,hor_factor);
	float3 color = colorWithHorizon * skyParams1.x * skyParams1.y;

	return color;
}

bool IsReversedProjectionCamera()
{
	return 1.f == cameraDepthRange.y;
}

# define PSGetProjectionCameraSkyDepth			(1 - cameraDepthRange.y)
# define PSGetProjectionCameraNearestDepth		(cameraDepthRange.y)

float DecodeGlobalShadowBufferSSAO( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_SSAO];
}

float DecodeGlobalShadowBufferShadow( float4 bufferValue )
{
	return bufferValue[GLOBAL_SHADOW_BUFFER_CHANNEL_SHADOW];
}

float TransformDepthRevProjAware( float depth )
{
	return depth * cameraDepthRange.x + cameraDepthRange.y;
}

float3 PositionFromDepth(in float depth, in float2 pixelCoord, float2 customScreenDimensions )
{
    float2 cpos = (pixelCoord + 0.5f) / customScreenDimensions;
    cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    float4 positionWS = mul(screenToWorld, float4(cpos, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepth(in float depth, in float2 pixelCoord )
{
	return PositionFromDepth( depth, pixelCoord, screenDimensions.xy );
}

float3 PositionFromDepthRevProjAware(in float depth, in float2 pixelCoord, float2 customScreenDimensions )
{
    float2 cpos = (pixelCoord + 0.5f) / customScreenDimensions;
    cpos *= 2.0f;
    cpos -= 1.0f;
    cpos.y *= -1.0f;
    float4 positionWS = mul(screenToWorldRevProjAware, float4(cpos, depth, 1.0f) );
    return positionWS.xyz / positionWS.w;
}

float3 PositionFromDepthRevProjAware(in float depth, in float2 pixelCoord )
{
	return PositionFromDepthRevProjAware( depth, pixelCoord, screenDimensions.xy );
}

float ProjectDepth( in float depth )
{
	return 1.0 / (cameraNearFar.x * depth) - cameraNearFar.y / cameraNearFar.x;
}

#define DEPROJECT_DEPTH_DIV_THRESHOLD 0.0001
float DeprojectDepth( in float depth )
{
	// make linear and deproject (now it should be in view space)
	float div = depth * cameraNearFar.x + cameraNearFar.y;
	return 1.0 / max( DEPROJECT_DEPTH_DIV_THRESHOLD, div );
}

float DeprojectDepthRevProjAware( in float depth )
{
	depth = TransformDepthRevProjAware( depth );							// ace_fix precision might not be the best here !!!
	// make linear and deproject (now it should be in view space)
	float div = depth * cameraNearFar.x + cameraNearFar.y;
	return 1.0 / max( DEPROJECT_DEPTH_DIV_THRESHOLD, div );
}

bool IsSkyByProjectedDepth( float depth )
{
	return depth >= 1.0;
}

bool IsSkyByProjectedDepthRevProjAware( float depth )
{
	return TransformDepthRevProjAware( depth ) >= 1.0;
}

bool IsSkyByLinearDepth( float depth )
{
	return depth >= 0.999 * DeprojectDepth( 1.0 ); //< ace_todo: this is kinda shitty
}

float CalcEnvProbesDistantBrightnessByHorizDist( float horizDist )
{
	return lerp( 1.0, pbrSimpleParams1.z, saturate( horizDist * lightColorParams.x + lightColorParams.y ) );
}

float CalcEnvProbesDistantBrightness( float3 worldSpacePosition )
{
	float displaceHoriz = length( worldSpacePosition.xy - cameraPosition.xy );
	return CalcEnvProbesDistantBrightnessByHorizDist( displaceHoriz );
}

float3 ModulateSSAOByTranslucency( float3 ssaoValue, float translucency )
{
	return lerp( float3(1.0, 1.0, 1.0), ssaoValue, lerp( 1.0, ssaoParams.z, translucency ) ); // ace_optimize (test solution for too dark ssao on grass)
}

float3 ProcessSampledSSAO( float ssaoValue )
{
	return saturate( ssaoValue * ssaoClampParams0.xyz + ssaoClampParams1.xyz );
}

float3 SpeedTreeApplyRandomColorAlbedo_CustomColors( float4 randColorData, float3 lumWeights, float4 randomColorParams0, float4 randomColorParams1, float4 randomColorParams2, float3 col )
{
	const float  lum = dot( lumWeights.xyz, col.xyz );
	const float3 colorWeights = randColorData.xyz;
	const float4 mergedData = colorWeights.x * randomColorParams0 + colorWeights.y * randomColorParams1 + colorWeights.z * randomColorParams2;
	const float3 randColor = max( 0.0, 1 - mergedData.xyz );
	const float  randSaturation = mergedData.w;
	
	col = pow( max( 0, col ), 2.2 );
	col = max( 0.0, lum + (col - lum) * randSaturation );
	col = col * randColor.xyz;
	col = pow( col, 1.0 / 2.2 );

	return col;
}

float3 SpeedTreeApplyRandomColorAlbedoTrees( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsTrees.xyz, speedTreeRandomColorParamsTrees0, speedTreeRandomColorParamsTrees1, speedTreeRandomColorParamsTrees2, col );
}

float3 SpeedTreeApplyRandomColorAlbedoBranches( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsBranches.xyz, speedTreeRandomColorParamsBranches0, speedTreeRandomColorParamsBranches1, speedTreeRandomColorParamsBranches2, col );
}

float3 SpeedTreeApplyRandomColorAlbedoGrass( float4 randColorData, float3 col )
{
	return SpeedTreeApplyRandomColorAlbedo_CustomColors( randColorData, speedTreeRandomColorLumWeightsGrass.xyz, speedTreeRandomColorParamsGrass0, speedTreeRandomColorParamsGrass1, speedTreeRandomColorParamsGrass2, col );
}

float ApplyAlphaTransform( float alpha, float2 alphaTransform )
{
	return alpha * alphaTransform.x + alphaTransform.y;
}

float2 AddAlphaTransformOffset( float2 alphaTransform, float offset )
{
	return float2 ( alphaTransform.x, alphaTransform.y + offset );
}

uint BuildMSAACoverageMask_AlphaToCoverage( float alphaValue )
{
	uint result = 0xffffffff;
	
	[branch]
	if ( msaaParams.y > 0 )
	{
		/*
		// test code for manual dithering (requires vpos passed in)
		float r = 0;
		r += vpos.x * 6.2123125431414;
		r += vpos.y * 4.3125431414511;
		alphaValue -= fmod( r, msaaParams.y );
		*/

		result = ((alphaValue > 0) ? 0xfffffff1 : 0xfffffff0) + ((alphaValue > msaaParams.y) ? 2 : 0) + ((alphaValue > msaaParams.z) ? 4 : 0) + ((alphaValue > msaaParams.w) ? 8 : 0);
	}

	return result;
}

uint BuildMSAACoverageMask_AlphaToCoverage( float origAlpha, float2 alphaTransform, float alphaThreshold )
{
	return BuildMSAACoverageMask_AlphaToCoverage( (ApplyAlphaTransform( origAlpha, alphaTransform ) - alphaThreshold) / max( 0.0001, (ApplyAlphaTransform( 1.0, alphaTransform ) - alphaThreshold) ) );	
}

uint BuildMSAACoverageMask_AlphaTestSS( /*float origAlpha,*/ Texture2D tex, SamplerState smp, float2 coord, float2 alphaTransform, float alphaThreshold )
{	
#define BUILD_COVERAGE_MASK_SUPERSAMPLE(_NumSamples)				\
	{																\
		float2 alphaTransformFinal = AddAlphaTransformOffset( alphaTransform, -alphaThreshold );	\
																	\
		coverage = 0;												\
		float2 texCoord_ddx = ddx(coord);							\
		float2 texCoord_ddy = ddy(coord);							\
																	\
		[unroll]													\
		for (int i = 0; i < _NumSamples; ++i)						\
		{															\
			float2 texelOffset = msaaOffsets[i].x * texCoord_ddx;	\
			texelOffset += msaaOffsets[i].y * texCoord_ddy;													\
			float currAlpha = tex.Sample( smp, coord + texelOffset ).a;										\
			coverage |= ApplyAlphaTransform( currAlpha, alphaTransformFinal ) >= 0 ? 1 << i : 0;			\
		}																									\
	}

	uint coverage = 0xffffffff;
		
	[branch]
	if ( 1 != msaaParams.x )
	{
		[branch]
		if ( 2 == msaaParams.x )
		{
			SYS_STATIC const float2 msaaOffsets[2] = 
			{ 
				float2(0.25, 0.25), 
				float2(-0.25,-0.25) 
			};

			BUILD_COVERAGE_MASK_SUPERSAMPLE( 2 );
		}
		else if ( 4 == msaaParams.x )
		{
			SYS_STATIC const float2 msaaOffsets[4] =
			{
				float2(-0.125, -0.375), 
				float2(0.375, -0.125),
				float2(-0.375, 0.125), 
				float2(0.125, 0.375)
			};
			
			BUILD_COVERAGE_MASK_SUPERSAMPLE( 4 );
		}
	}

	return coverage;
	
#undef BUILD_COVERAGE_MASK_SUPERSAMPLE
}

#endif
/* make sure leave an empty line at the end of ifdef'd files because of SpeedTree compiler error when including two ifdef'ed files one by one : it produces something like "#endif#ifdef XXXX" which results with a bug */
#ifndef ENVPROBE_FX_H_INCLUDED
#define ENVPROBE_FX_H_INCLUDED


#define REFLECTION_CUBE_RESOLUTION		128
#define AMBIENT_CUBE_NUM_MIPS			2
#define REFLECTION_CUBE_NUM_MIPS		6

#define MAX_ENVPROBES_PER_TILE			8


#define ENVPROBE_MIPLEVEL_SCALE	0.725
float CalcEnvProbeMipLevel( float3 N, int2 pixelCoord )
{
	return ENVPROBE_MIPLEVEL_SCALE * CalcCubeMipLevel( REFLECTION_CUBE_RESOLUTION, N, pixelCoord );
}

float EncodeEnvProbeMipLevel( float3 N, int2 pixelCoord )
{
	float lvl = CalcEnvProbeMipLevel( N, pixelCoord );
	return lvl / REFLECTION_CUBE_NUM_MIPS;
}

float DecodeEnvProbeMipLevel( float encodedValue )
{
	return encodedValue * REFLECTION_CUBE_NUM_MIPS;
}

/// 'dir' MUST BE NORMALIZED !!!
float2 CalcEnvProbeAtlasCoord( float3 dir, int sliceIndex )
{
	const int sphereFaceIndex = GetCubeDirParaboloidIndex( dir );
	float2 crd = CubeToParaboloid( dir, sphereFaceIndex );

	float extent = 1 << (REFLECTION_CUBE_NUM_MIPS - 1);
	float sphereRes = REFLECTION_CUBE_RESOLUTION;
	float sumRes = extent * 2 + sphereRes;
	crd = lerp( extent / sumRes, 1 - extent / sumRes, crd );

	crd = ( float2( 0, sliceIndex ) + (crd + float2( sphereFaceIndex, 0 )) ) / float2( 2, CUBE_ARRAY_CAPACITY );	
	return crd;
}

float3 SampleEnvProbeAmbient( TEXTURE2D<float4> tex, int sliceIndex, SamplerState smp, float3 N, float mipIndex = 0 )
{
	float2 coord = CalcEnvProbeAtlasCoord( N, sliceIndex );
	return SAMPLE_LEVEL( tex, smp, coord, mipIndex ).xyz;
}

float3 SampleEnvProbeReflection( TEXTURE2D<float4> tex, int sliceIndex, SamplerState smp, float3 N, float mipIndex )
{
	float2 coord = CalcEnvProbeAtlasCoord( N, sliceIndex );
	return SAMPLE_LEVEL( tex, smp, coord, mipIndex ).xyz;
}

#ifdef DEFERRED

	TEXTURE2D<float4>			EnvProbeAtlasAmbient	: register(t6);
	TEXTURE2D<float4>			EnvProbeAtlasReflection	: register(t7);
	SamplerState 				EnvProbeSampler			: register(s6);

#else

	TEXTURE2D<float4>			EnvProbeAtlasAmbient	: register(t22);
	TEXTURE2D<float4>			EnvProbeAtlasReflection	: register(t23);
	SamplerState 				EnvProbeSampler			: register(s11);

#endif

float3 UnnormalizedParallaxCorrection( ShaderCommonEnvProbeParams params, float3 pos, float3 N )
{
	const float3 CubemapPositionWS = params.probePos;
	const float4 PositionWS = float4 ( pos, 1.0 );	
	const float3 RayWS = N;
	const float3 ReflDirectionWS = N;

	float3 RayLS = mul( (float3x3)params.parallaxWorldToLocal, ReflDirectionWS );
	float3 PositionLS = mul( params.parallaxWorldToLocal, PositionWS ).xyz;

	float3 ISectVal0 = 1.f / RayLS;
	float3 ISectVal1 = PositionLS * ISectVal0;	
	float3 FirstPlaneIntersect = ISectVal0 - ISectVal1; // (Unitary - PositionLS) / RayLS;
	float3 SecondPlaneIntersect = -ISectVal0 - ISectVal1; // (-Unitary - PositionLS) / RayLS;
	float3 FurthestPlane = max(FirstPlaneIntersect, SecondPlaneIntersect);
	float Distance = min(FurthestPlane.x, min(FurthestPlane.y, FurthestPlane.z));

	// Use Distance in WS directly to recover intersection
	float3 IntersectPositionWS = PositionWS.xyz + RayWS * Distance;
	return IntersectPositionWS - CubemapPositionWS;
}

float3 NormalizedParallaxCorrection( ShaderCommonEnvProbeParams params, float3 pos, float3 N )
{
	return normalize( UnnormalizedParallaxCorrection( params, pos, N ) );
}

#define CALCENVPROBES_WEIGHTS_START		0.00001
#define CALCENVPROBES_WEIGHTS_LIMIT		0.999

// 'allowAmbient' and 'allowReflection' are here just in case to help compiler to compile out unneeded code
// if this function gets used for calculating either ambient or reflection only.
void CalcEnvProbes_MipLod( out float3 outAmbient, out float3 outReflection, float3 worldSpacePosition, float3 N, float3 R, float ambientMipIndex, float reflectionMipIndex, float roughness, bool allowLocalProbes = true, bool allowAmbient = true, bool allowReflection = true )
{
	const float roughnessScaleMagic = (REFLECTION_CUBE_NUM_MIPS * 1.75) * roughness; // manipulated in order for better specular matching
	const float reflectionGlossLod	= roughnessScaleMagic * roughness;
	const float reflectionCustomLod	= max( reflectionGlossLod, reflectionMipIndex );

	float  weightsSum = CALCENVPROBES_WEIGHTS_START;
	float3 envProbeAmbient = float3 ( 0, 0, 0 );
	float3 envProbeReflection = float3 ( 0, 0, 0 );
					
	const uint GLOBAL_SLOT_INDEX = 0;

	if ( allowLocalProbes )
	{
		[unroll] //< heavy performance improvement because of the unroll in test scenes!!
		for ( uint slot_i=1; slot_i<CUBE_ARRAY_CAPACITY && weightsSum < CALCENVPROBES_WEIGHTS_LIMIT; ++slot_i )
		{
			const ShaderCommonEnvProbeParams params = commonEnvProbeParams[slot_i];
			
			float3 local_pos = mul( params.areaWorldToLocal, float4 ( worldSpacePosition, 1.0 ) ).xyz;
			float3 neg_abs_local_pos = 1 - abs(local_pos);
			
			[branch]
			if ( all( neg_abs_local_pos > 0 ) )
			{
				float3 off = saturate( (neg_abs_local_pos) * params.areaMarginScale );
				const float probeWeight = (1 - weightsSum) * params.weight * ( off.x * off.y * off.z );
		
				weightsSum += probeWeight;
				const float valueScale = probeWeight * params.intensities.x;
				if ( allowAmbient )
				{
					envProbeAmbient += valueScale * SampleEnvProbeAmbient( EnvProbeAtlasAmbient, params.slotIndex, EnvProbeSampler, N, ambientMipIndex );
				}
				if ( allowReflection )
				{
					envProbeReflection += valueScale * SampleEnvProbeReflection( EnvProbeAtlasReflection, params.slotIndex, EnvProbeSampler, NormalizedParallaxCorrection( params, worldSpacePosition, R ), reflectionCustomLod );
				}
			}
		}
	}

	[branch]
	if ( weightsSum < CALCENVPROBES_WEIGHTS_LIMIT )
	{
		const ShaderCommonEnvProbeParams params = commonEnvProbeParams[GLOBAL_SLOT_INDEX];
		const float probeWeight = (1 - weightsSum) * params.weight;
		
		weightsSum += probeWeight;				
		const float valueScale = probeWeight * params.intensities.x;	
		if ( allowAmbient )
		{
			envProbeAmbient += valueScale * SampleEnvProbeAmbient( EnvProbeAtlasAmbient, params.slotIndex, EnvProbeSampler, N, ambientMipIndex );
		}
		if ( allowReflection )
		{
			envProbeReflection += valueScale * SampleEnvProbeReflection( EnvProbeAtlasReflection, params.slotIndex, EnvProbeSampler, R, reflectionCustomLod );
		}
	}

	const float invWeightsSum = 1.f / weightsSum;
	outAmbient = envProbeAmbient * invWeightsSum;
	outReflection = envProbeReflection * invWeightsSum;
}

uint CalcEnvProbes_MipLodComplexity( float3 worldSpacePosition )
{
	float weightsSum = CALCENVPROBES_WEIGHTS_START;
	uint complexity = 0;

	[unroll]
	for ( uint slot_i=1; slot_i<CUBE_ARRAY_CAPACITY && weightsSum < CALCENVPROBES_WEIGHTS_LIMIT; ++slot_i )
	{
		const ShaderCommonEnvProbeParams params = commonEnvProbeParams[slot_i];
		float3 local_pos = mul( params.areaWorldToLocal, float4 ( worldSpacePosition, 1.0 ) ).xyz;
		float3 neg_abs_local_pos = 1 - abs(local_pos);
			
		[branch]
		if ( all( neg_abs_local_pos > 0 ) )
		{
			float3 off = saturate( (neg_abs_local_pos) * params.areaMarginScale );
			const float probeWeight = (1 - weightsSum) * params.weight * ( off.x * off.y * off.z );
		
			weightsSum += probeWeight;

			complexity += 1;
		}
	}

	[branch]
	if ( weightsSum < CALCENVPROBES_WEIGHTS_LIMIT )
	{
		complexity += 1;
	}

	return complexity;
}

float3 CalcEnvProbeAmbient( float3 worldSpacePosition, float3 N, bool allowLocalProbes = true, float mipIndex = 0 )
{
	float3 resultAmbient, resultReflection;
	CalcEnvProbes_MipLod( resultAmbient, resultReflection, worldSpacePosition, N, N, mipIndex, 0, 1, allowLocalProbes, true, false );
	return resultAmbient;
}

float3 CalcGlobalEnvProbeAmbient( float3 worldSpacePosition, float3 N )
{
	return CalcEnvProbeAmbient( worldSpacePosition, N, false );
}

float3 CalcEnvProbeReflection_MipLod( float3 worldSpacePosition, float3 R, float mipLod, float roughness, bool allowLocalProbes = true )
{
	float3 resultAmbient, resultReflection;
	CalcEnvProbes_MipLod( resultAmbient, resultReflection, worldSpacePosition, R, R, 0, mipLod, roughness, allowLocalProbes, false, true );
	return resultReflection;
}

float3 CalcEnvProbeReflection_NormalBasedMipLod( float3 worldSpacePosition, float3 V, float3 N, float roughness, int2 pixelCoord, bool allowLocalProbes )
{
	const float3 R = reflect( -V, N );		
	float mipLod = CalcEnvProbeMipLevel( R, pixelCoord );
	return CalcEnvProbeReflection_MipLod( worldSpacePosition, R, mipLod, roughness, allowLocalProbes );
}

void CalcEnvProbes_NormalBasedMipLod( out float3 outAmbient, out float3 outReflection, float3 worldSpacePosition, float3 V, float3 ambientN, float3 reflectionN, float ambientMipIndex, float roughness, int2 pixelCoord, bool allowLocalProbes )
{
	const float3 R = reflect( -V, reflectionN );		
	float reflectionMipLod = CalcEnvProbeMipLevel( R, pixelCoord );
	CalcEnvProbes_MipLod( outAmbient, outReflection, worldSpacePosition, ambientN, R, ambientMipIndex, reflectionMipLod, roughness, allowLocalProbes );
}

#endif
#if (ST_USED_AS_GRASS)

#if defined( VERTEX_SHADER )

Texture2D<float4> 	TerrainPigmentTex 	: register(t3);
SamplerState 		TerrainPigmentSmp	: register(s3);

START_CB( PigmentConsts, 11 )
	float4		pigmentWorldAreaScaleBias;	
	float4x4	m_transformMatrices[ 16 ];
	uint		m_activeCollisionsNum;
END_CB

float3 ComputeCollisionOffset( float3 worldPosition, float3 worldNormal, float3 localPosition )
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

	resultingOffset.xyz = localZdamp * length( resultingOffset.xyz ) * ( 0.2f*resultingNormal.xyz + 0.8f*resultingOffset.xyz );

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


#endif

float3 SpeedTreeApplyRandomColorAlbedoRuntimeNonGrass( float4 randColorData, float3 col, float controlValue )
{
	if ( controlValue > 0.501 )
	{		
		if ( controlValue > 0.521 )
		{
			col = SpeedTreeApplyRandomColorAlbedoTrees( randColorData, col );				
		}
		else if ( controlValue > 0.511 )
		{
			col = SpeedTreeApplyRandomColorAlbedoBranches( randColorData, col );		
		}
		else
		{
			col = SpeedTreeApplyRandomColorAlbedoGrass( randColorData, col );
		}
	}

	return col;	
}

float3 SpeedTreeApplyRandomColorAlbedoRuntime( float4 randColorData, float3 col )
{
#if (ST_USED_AS_GRASS)
	if ( g_vGrassParams1.x > 0.501 )
	{
		col = SpeedTreeApplyRandomColorAlbedoGrass( randColorData, col );
	}
#else
	col = SpeedTreeApplyRandomColorAlbedoRuntimeNonGrass( randColorData, col, m_fOneMinusAmbientContrastFactor );
#endif

	return col;
}

float SpeedTreeBuildGBuffMaterialMask()
{
	uint mask = GBUFF_MATERIAL_MASK_DEFAULT;

#if (ST_USED_AS_GRASS)
	//if ( g_vGrassParams1.x > 0.501 )
	{
		mask = GBUFF_MATERIAL_FLAG_GRASS;
	}
#else
	const float controlValue = m_fOneMinusAmbientContrastFactor;
	if ( controlValue > 0.501 )
	{		
		if ( controlValue > 0.521 )
		{
			mask = GBUFF_MATERIAL_FLAG_TREES;
		}
		else if ( controlValue > 0.511 )
		{
			mask = GBUFF_MATERIAL_FLAG_BRANCHES;
		}
		else
		{
			mask = GBUFF_MATERIAL_FLAG_GRASS;
		}
	}
#endif

	return EncodeGBuffMaterialFlagsMask( mask );
}

#if (ST_USED_AS_GRASS)

float3 ApplyTerrainPigmentRuntime( float4 pigment, float3 albedo, float localVerticalPos, float dist )
{
	const float ambientContrast = m_fOneMinusAmbientContrastFactor;

	const float amountScale = g_vGrassParams0.x;
	const float distInfluence = g_vGrassParams0.y;
	const float pigmentStartPos = g_vGrassParams0.z;
	const float pigmentGradient = g_vGrassParams0.w;
	
	return ApplyTerrainPigment( pigment, albedo, dist, distInfluence, localVerticalPos, pigmentStartPos, pigmentGradient, amountScale );
}

#endif

// LAVA--

///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind forward pixel shader for 3D geometry

SMultiRenderTargets main(// input interpolants
			  float4 v2p_vInterpolant0 : ST_VS_OUTPUT
			, float4 v2p_vInterpolant1 : TEXCOORD0
			, float4 v2p_vInterpolant2 : TEXCOORD1
			, float3 v2p_vInterpolant3 : TEXCOORD2
			, float3 v2p_vInterpolant4 : TEXCOORD3

            // user interpolants
            #if (ST_USER_INTERPOLANT0)
              , float4 v2p_vUserInterpolant0 : TEXCOORD4
            #endif
            #if (ST_USER_INTERPOLANT1)
              , float4 v2p_vUserInterpolant1 : TEXCOORD5
            #endif
            #if (ST_USER_INTERPOLANT2)
              , float4 v2p_vUserInterpolant2 : TEXCOORD6
            #endif
            #if (ST_USER_INTERPOLANT3)
              , float4 v2p_vUserInterpolant3 : TEXCOORD7
            #endif
           )
{
	// output
	#if (!ST_OPENGL)
		// GLSL uses a different return mechanism
		SMultiRenderTargets sOutput;
	#endif
	
	//LAVA++
	// For trees, in non-depth-only passes (not shadows), dissolve the tree based on fade parameter.
	#if !(ST_USED_AS_GRASS) && (ST_USER_INTERPOLANT3)
	{
		float2 texSize;
		float2 tex = v2p_vInterpolant0.xy / 16;	// HACK : dissolve texture is 16x16
		tex = tex - floor( tex );
		float dissolveTex = SampleTexture(DissolveMap, tex).x;
		if ( v2p_vUserInterpolant3.x > dissolveTex )
		{
			discard;
		}
	}
	#endif
	//LAVA--

	// setup outputs to be passed/packed into g-buffer out
	float3 out_vAlbedo = float3(0.0, 0.0, 0.0);
	float  out_fAlpha = 1.0;
	float3 out_vNormal = float3(0.0, 0.0, 0.0);
	float  out_fSpecularMask = 0.0;
	float  out_fSpecularPower = 30.0;
	float  out_fTransmissionMask = 0.0;

	// LAVA++
	uint   out_Coverage = 0xffffffff;
	float3 out_vWorldPosition = v2p_vUserInterpolant1.xyz;
	const float localVerticalPosition = v2p_vUserInterpolant1.w;
	const float cameraDist = length( out_vWorldPosition - m_vCameraPosition.xyz );
	// LAVA--
	// all possible variables that may come from the vertex shader; the "v2p" prefix indicates that these
	// variables are values that go from [v]ertex-[2]-[p]ixel shader
	float  v2p_fFogScalar = 0.0;
	float3 v2p_vFogColor = float3(0.0, 0.0, 0.0);
	float2 v2p_vDiffuseTexCoords = float2(0.0, 0.0);
	float3 v2p_vPerVertexLightingColor = float3(0.0, 0.0, 0.0);
	float3 v2p_vNormalInTangentSpace = float3(0.0, 0.0, 0.0);
	float  v2p_fPerVertexAmbientContrast = 0.0;
	float  v2p_fRenderEffectsFade = 1.0;
	float  v2p_fAmbientOcclusion = 0.0;
	float3 v2p_vHueVariation = float3(0.0, 0.0, 0.0);
	float2 v2p_vDetailTexCoords = float2(0.0, 0.0);
	float3 v2p_vNormal = float3(0.0, 0.0, 0.0);
	float3 v2p_vBinormal = float3(0.0, 0.0, 0.0);
	float3 v2p_vTangent = float3(0.0, 0.0, 0.0);
	float3 v2p_vSpecularHalfVector = float3(0.0, 0.0, 0.0);
	float  v2p_fPerVertexSpecularDot = 0.0;
	float  v2p_fFadeToBillboard = 1.0;
	float  v2p_fTransmissionFactor = 0.0;
	float3 v2p_vBranchSeamDiffuse = float3(0.0, 0.0, 0.0);
	float2 v2p_vBranchSeamDetail = float2(0.0, 0.0);
	float  v2p_fShadowDepth = 0.0;
	float4 v2p_vShadowMapProjection0 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection1 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection2 = float4(0.0, 0.0, 0.0, 0.0);
	float4 v2p_vShadowMapProjection3 = float4(0.0, 0.0, 0.0, 0.0);

	// set initial values for those v2p parameters that are in use
    v2p_vDiffuseTexCoords = v2p_vInterpolant1.xy;
    v2p_vNormal = v2p_vInterpolant2.xyz;
    v2p_vTangent = v2p_vInterpolant3.xyz;
    v2p_fFadeToBillboard = v2p_vInterpolant1.z;
    v2p_fRenderEffectsFade = v2p_vInterpolant1.w;
    v2p_fAmbientOcclusion = v2p_vInterpolant2.w;
    v2p_vHueVariation = v2p_vInterpolant4.xyz;

	// diffuse texture lookup
	float4 texDiffuse = SampleTexture(DiffuseMap, v2p_vDiffuseTexCoords);

	// adjust alpha value if grass or fading to billboard (alpha "fizzle" with noise)
	if (ST_EFFECT_FADE_TO_BILLBOARD || ST_USED_AS_GRASS)
	{
		out_fAlpha = texDiffuse.a * v2p_fFadeToBillboard;

		if (ST_ALPHA_TEST_NOISE)
		{
			float fDelta = (m_fAlphaScalar - v2p_fFadeToBillboard) / m_fAlphaScalar;
			out_fAlpha -= AlphaTestNoise_3dTree(v2p_vDiffuseTexCoords) * fDelta;
		}
	}
	else
		out_fAlpha = texDiffuse.a * m_fAlphaScalar;

	// attempt to get earliest possible exit if pixel is transparent
	CheckForEarlyExit(out_fAlpha, ST_TRANSPARENCY_ACTIVE);

	// branch seam blending (active only for branch geometry)
	const float2 c_fSeamDiffuseTexcoords = v2p_vBranchSeamDiffuse.xy;
	const float2 c_fSeamDetailTexcoords = v2p_vBranchSeamDetail.xy;
	const float c_fSeamBlendWeight = v2p_vBranchSeamDiffuse.z;
	float fSeamBlend = min(pow(c_fSeamBlendWeight, m_fBranchSeamWeight), 1.0);

	// extract and save diffuse color & intensity
	{
		// diffuse color
		float3 vDiffuseColorOutput = LookupColorWithDetail(v2p_vDiffuseTexCoords, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);

		if (ST_EFFECT_BRANCH_SEAM_SMOOTHING)
		{
			float3 vColorAtSeam = LookupColorWithDetail(v2p_vBranchSeamDiffuse.xy, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);
			vDiffuseColorOutput = lerp(vColorAtSeam, vDiffuseColorOutput, fSeamBlend);
		}

		if (ST_EFFECT_HUE_VARIATION)
			vDiffuseColorOutput.rgb += v2p_vHueVariation;

		// LAVA++
		// out_vAlbedo.rgb = vDiffuseColorOutput * m_vDiffuseColor;
		out_vAlbedo.rgb = SpeedTreeApplyRandomColorAlbedoRuntime( v2p_vUserInterpolant0, vDiffuseColorOutput ) * m_vDiffuseColor.rgb;
		// LAVA--
		
		if (ST_EFFECT_AMBIENT_OCCLUSION)
        {
			// LAVA++
			float ambientOcclusion = v2p_fAmbientOcclusion;
			const float ambientOcclusionAmount = speedTreeParams.x;
			ambientOcclusion = saturate( lerp( 1.0, ambientOcclusion, ambientOcclusionAmount ) );

			out_vAlbedo.rgb *= ambientOcclusion;
			// LAVA--
        }
		
		// LAVA++
		// Apply pigment
		#if (ST_USED_AS_GRASS)
		{
			const float4 pigment = v2p_vUserInterpolant2;
			out_vAlbedo.rgb = ApplyTerrainPigmentRuntime( pigment, out_vAlbedo.rgb, localVerticalPosition, cameraDist );
		}
		#endif
		// LAVA--
	}

	// extract and encode normal
	{
		float3 vOutputNormal = LookupNormalWithDetail(v2p_vDiffuseTexCoords, v2p_vDetailTexCoords, v2p_fRenderEffectsFade);

		if (ST_EFFECT_BRANCH_SEAM_SMOOTHING)
		{
			float3 vNormalAtSeam = LookupNormalWithDetail(v2p_vBranchSeamDiffuse.xy, v2p_vBranchSeamDetail, v2p_fRenderEffectsFade);
			vOutputNormal = lerp(vNormalAtSeam, vOutputNormal, fSeamBlend);
		}

		// orient normal to align with triangle
		float3 vBinormal = cross(v2p_vNormal, v2p_vTangent);
		float3x3 mTriangleOrientation = BuildOrientationMatrix(v2p_vTangent, vBinormal, v2p_vNormal);
		float3 vOrientedNormal = normalize(mul_float3x3_float3(mTriangleOrientation, vOutputNormal));

		// encode vector back into color
		out_vNormal = vOrientedNormal;
	}

	// lookup texture shared by both specular and transmission
	float4 texSpecularTransmissionMask = SampleTexture(SpecularMaskMap, v2p_vDiffuseTexCoords);

	// specular
	if (ST_EFFECT_SPECULAR)
	{
		// mask is rgb, but we can store only one color component
		out_fSpecularMask = RgbToLuminance(texSpecularTransmissionMask.rgb);

		// apply specular color, but again, just the one color component
		out_fSpecularMask *= RgbToLuminance(m_vSpecularColor);

		if (ST_EFFECT_SPECULAR_FADE)
			out_fSpecularMask *= v2p_fRenderEffectsFade;

		// specular curve
		out_fSpecularPower = m_fShininess;
		
        // LAVA++
		out_fSpecularMask *= m_vSpecularColor.g;
		// LAVA--
	}

	// transmission
	if (ST_EFFECT_TRANSMISSION)
	{
		out_fTransmissionMask = texSpecularTransmissionMask.a;

		// apply transmission color, but just the one color component
		out_fTransmissionMask *= RgbToLuminance(m_vTransmissionColor);

		// fade effect out if enabled
		if (ST_EFFECT_TRANSMISSION_FADE)
			out_fTransmissionMask *= v2p_fRenderEffectsFade;
        
		// LAVA++
		out_fTransmissionMask *= m_vTransmissionColor.g;
		// LAVA--
	}
	
	// build material mask
	const float out_encodedGBuffMaterialMask = SpeedTreeBuildGBuffMaterialMask();

	// output syntax is platform-specific, encapsulated in an include
	///////////////////////////////////////////////////////////////////////
	//  InlinePixel_DeferredOutput.fx
	//
	//	Designed to be included at the end of SpeedTree's deferred pixel shaders,
	//	this code excerpt will handle the per-platform multi-render target output.
	//	It excepts that these values have been declared and assigned prior to
	//	being invoked:
	//
	//		- float3 out_vAlbedo: diffuse color, in our example this also contains
	//							  ambient occlusion
	//
	//		- float out_fAlpha: alpha value of the pixel
	//
	//		- float3 out_vNormal: surface normal (in world space in our example)
	//
	//		- float out_fSpecularMask: used to mask specular lighting, 0.0 = masked, 1.0 = pass
	//
	//		- float out_fSpecularPower: directly related to the specular shininess value as
	//							        defined in the Modeler app, but with (shininess* 126 + 1)
	//									applied
	//
	//		- float out_fTransmissionMask: mask used for out approximate transmission/sub-surface-
	//									   scattering approach, 0.0 = masked, 1.0 = pass

	{
		#if (ST_OPENGL)

			// render target 0
			output_FragmentColor[0].rgb = out_vAlbedo;
			output_FragmentColor[0].a = out_fSpecularPower / 128.0;

			// render target 1
			output_FragmentColor[1].rg = PackNormalIntoFloat2_Stereographic(out_vNormal);
			output_FragmentColor[1].b = out_fSpecularMask;
			output_FragmentColor[1].a = out_fTransmissionMask;

		#elif (!ST_XBOX_360)

// LAVA++
			const float packedEvProbeLevel = EncodeEnvProbeMipLevel( reflect( normalize( out_vWorldPosition.xyz - m_vCameraPosition.xyz ), out_vNormal.xyz ), v2p_vInterpolant0.xy );

			// render target 0
			sOutput.m_vRenderTarget0.rg 	= EncodeTwoChannelsYCbCr( out_vAlbedo, v2p_vInterpolant0.xy );
            sOutput.m_vRenderTarget0.b 		= 0;
       		sOutput.m_vRenderTarget0.a 		= out_fTransmissionMask;

			// render target 1
            sOutput.m_vRenderTarget1.rgb 	= out_vNormal;
			sOutput.m_vRenderTarget1.a 		= out_fSpecularPower / 128.0;
			
            sOutput.m_vRenderTarget2.rg 	= EncodeTwoChannelsYCbCr( out_fSpecularMask, v2p_vInterpolant0.xy );
			sOutput.m_vRenderTarget2.b 		= packedEvProbeLevel;
			sOutput.m_vRenderTarget2.a 		= out_encodedGBuffMaterialMask;
			
			sOutput.m_Coverage = out_Coverage;
			#if (ST_DEFERRED_A2C_ENABLED)
//ACE_TODO: add support for this !!!
				sOutput.m_vNullRenderTarget.rgba = float4(0.0, 0.0, 0.0, out_fAlpha);
			#endif

// LAVA--
			return sOutput;

		#else // Xbox 360

			// render target 0
			sOutput.m_vRenderTargets[0].rgb = out_vAlbedo;
			sOutput.m_vRenderTargets[0].a = out_fSpecularPower / 128.0;

			// render target 1
			sOutput.m_vRenderTargets[1].rg = PackNormalIntoFloat2_Stereographic(out_vNormal);
			sOutput.m_vRenderTargets[1].b = out_fSpecularMask;
			sOutput.m_vRenderTargets[1].a = out_fTransmissionMask;
			
			return sOutput;

		#endif
	}
}
