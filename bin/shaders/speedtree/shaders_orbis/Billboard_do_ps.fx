///////////////////////////////////////////////////////////////////////  
//  Billboard_do_ps.fx
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
#define ST_FOG_CURVE ST_FOG_CURVE_LINEAR
#define ST_FOG_COLOR ST_FOG_COLOR_DYNAMIC
#define ST_SHADOWS_ENABLED false
#define ST_SHADOWS_SMOOTH false
#define ST_SHADOWS_NUM_MAPS 2
#define ST_BRANCHES_PRESENT false
#define ST_ONLY_BRANCHES_PRESENT false
#define ST_FRONDS_PRESENT false
#define ST_ONLY_FRONDS_PRESENT false
#define ST_LEAVES_PRESENT false
#define ST_ONLY_LEAVES_PRESENT false
#define ST_FACING_LEAVES_PRESENT false
#define ST_ONLY_FACING_LEAVES_PRESENT false
#define ST_RIGID_MESHES_PRESENT false
#define ST_ONLY_RIGID_MESHES_PRESENT false
#define PIXEL_PROPERTY_PROJECTION_PRESENT false
#define PIXEL_PROPERTY_FOGSCALAR_PRESENT false
#define PIXEL_PROPERTY_FOGCOLOR_PRESENT false
#define PIXEL_PROPERTY_DIFFUSETEXCOORDS_PRESENT false
#define PIXEL_PROPERTY_DETAILTEXCOORDS_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXLIGHTINGCOLOR_PRESENT false
#define PIXEL_PROPERTY_NORMALINTANGENTSPACE_PRESENT false
#define PIXEL_PROPERTY_NORMAL_PRESENT false
#define PIXEL_PROPERTY_BINORMAL_PRESENT false
#define PIXEL_PROPERTY_TANGENT_PRESENT false
#define PIXEL_PROPERTY_SPECULARHALFVECTOR_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXSPECULARDOT_PRESENT false
#define PIXEL_PROPERTY_PERVERTEXAMBIENTCONTRAST_PRESENT false
#define PIXEL_PROPERTY_FADETOBILLBOARD_PRESENT false
#define PIXEL_PROPERTY_TRANSMISSIONFACTOR_PRESENT false
#define PIXEL_PROPERTY_RENDEREFFECTSFADE_PRESENT false
#define PIXEL_PROPERTY_AMBIENTOCCLUSION_PRESENT false
#define PIXEL_PROPERTY_BRANCHSEAMDIFFUSE_PRESENT false
#define PIXEL_PROPERTY_BRANCHSEAMDETAIL_PRESENT false
#define PIXEL_PROPERTY_SHADOWDEPTH_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION0_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION1_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION2_PRESENT false
#define PIXEL_PROPERTY_SHADOWMAPPROJECTION3_PRESENT false
#define PIXEL_PROPERTY_HUEVARIATION_PRESENT false
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
		
		// LAVA++
		Vec4			m_vLavaCustomBaseTreeParams;
		// LAVA--
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
		
		// LAVA++
		Vec4		m_vLavaCustomMaterialParams;
		// LAVA--
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
DeclareTexture(AuxAtlas2, 7);
DeclareTexture(NoiseMap, 8);
DeclareTexture(PerlinNoiseKernel, 9);
DeclareTexture(ImageBasedAmbientLighting, 10);
//LAVA++
// For tree fading, bind our own dissolve pattern. This ensures a consistent dissolve experience
// between meshes and trees, and also we can be sure that it's been bound (ST might not bind its
// own noise-dissolve when not needed).
DeclareTexture(DissolveMap, 11);
DeclareTexture(MipNoiseTexture, 12); // Adding grain to the billboards, to make them less blobby
// LAVA--


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
#define ST_USER_INTERPOLANT2 1		// LAVA: pigment for grass or tree fading for trees
#define ST_USER_INTERPOLANT3 0

#endif // ST_INCLUDE_USER_INTERPOLANTS


///////////////////////////////////////////////////////////////////////
//	Pixel shader entry point
//
//	Main lighting/wind vertex shader for billboard geometry

#if (!ST_OPENGL)

	void main(
			  float4 v2p_vInterpolant0 : ST_VS_OUTPUT, // projection
			  float3 v2p_vInterpolant1 : TEXCOORD0  // texcoords, alpha scalar
			 )

#else // !ST_OPENGL

	// v2p_vInterpolant0 / projection is implicit
	varying float3 v2p_vInterpolant1; // texcoords, alpha scalar

	void main(void)

#endif // !ST_OPENGL

{
	float4 vPixelShaderReturn = float4(0.0, 0.0, 0.0, 1.0);

	// input attributes via interpolants
	float2 v2p_vTexCoords = v2p_vInterpolant1.xy;
	float  v2p_fAlphaScalar = v2p_vInterpolant1.z;

	// diffuse texture lookup
	//
	// .rgb = written by the Compiler app to be (diffuse_map * diffuse_light_scalar * ambient_occlusion)
	//   .a = alpha mask
	float4 texDiffuseTexture = SampleTexture(DiffuseMap, v2p_vTexCoords);

	// test for possible early exit condition
	vPixelShaderReturn.a = texDiffuseTexture.a * v2p_fAlphaScalar;
	if (ST_ALPHA_TEST_NOISE)
	{
		float fDelta = (m_fAlphaScalar - v2p_fAlphaScalar) / m_fAlphaScalar;
		vPixelShaderReturn.a -= AlphaTestNoise_Billboard(v2p_vTexCoords) * fDelta;
	}

	// attempt to get earliest possible exit if pixel is transparent
	CheckForEarlyExit(vPixelShaderReturn.a, ST_TRANSPARENCY_ACTIVE);
}
