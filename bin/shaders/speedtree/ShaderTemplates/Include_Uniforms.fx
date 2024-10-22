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

