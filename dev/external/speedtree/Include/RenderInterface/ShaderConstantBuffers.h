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
//      http://www.idvinc.com
//
//  *** Release version 7.0 ***


///////////////////////////////////////////////////////////////////////
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#include "Core/Types.h"
#include "Core/Vector.h"


///////////////////////////////////////////////////////////////////////
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(push, 4) // todo
#endif


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	// todo: update
	#define CONST_BUF_REGISTER_FRAME			0
	#define CONST_BUF_REGISTER_BASE_TREE		1
	#define CONST_BUF_REGISTER_MATERIAL			2
	#define CONST_BUF_REGISTER_WIND_DYNAMICS	3
	#define CONST_BUF_REGISTER_FOG_AND_SKY		4
	#define CONST_BUF_REGISTER_TERRAIN			5
	#define CONST_BUF_REGISTER_BLOOM			6

	// todo: write automatically
	// limits and sizes
	#define MAX_NUM_BILLBOARDS_PER_BASE_TREE	24
	#define NUM_EFFECT_CONFIG_FLOAT4S			5
	#define NUM_WIND_CONFIG_FLOAT4S				7
	#define NUM_WIND_LOD_FLOAT4S				3

	// todo: formalize padding 
	struct SFrameCBLayout // set once per frame
	{
		// projections
		Mat4x4			m_mModelViewProj3d;
		Mat4x4			m_mProjectionInverse3d;
		Mat4x4			m_mModelViewProj2d;
		Mat4x4			m_mCameraFacingMatrix;

		// other camera
		Vec3			m_vCameraPosition;		float pad1;
		Vec3			m_vCameraDirection;		float pad2;
		Vec3			m_vLodRefPosition;		float pad3;
		Vec2			m_vViewport;
		Vec2			m_vViewportInverse;
		st_float32		m_fFarClip;				Vec3 pad4;

		struct SDirLight
		{
			Vec3		m_vAmbient;			float pad0;
			Vec3		m_vDiffuse;			float pad1;
			Vec3		m_vSpecular;		float pad2;
			Vec3		m_vTransmission;	float pad3;
			Vec3		m_vDir;				float pad4;
		};
		SDirLight		m_sDirLight;

		struct SShadows
		{
			Vec4			m_vMapRanges;
			st_float32		m_fFadeStartPercent;
			st_float32		m_fFadeInverseDistance;			// todo: was g_fShadowMapParam (g_vShadowParams.z); remove this once shader has been updated
			st_float32		m_fTerrainAmbientOcclusion;		float pad0;

			Vec4			m_avSmoothingTable[3];
			st_float32		m_fShadowMapWritingActive;		// 1.0 if on, 0.0 if off
			Vec2			m_vTexelOffset;					float pad1;
			Mat4x4			m_amLightModelViewProjs[4];
		};

		// shadows
		SShadows		m_sShadows;

		// misc
		st_float32		m_fWallTime;						Vec3  pad5;
	};

	struct SWindDynamicsCBLayout
	{
		Vec3			m_vDirection;
		st_float32		m_fStrength;
		Vec3			m_vAnchor;					float pad0;

		struct SGlobal
		{
			st_float32	m_fGlobalWindTime;
			st_float32	m_fGlobalWindDistance;
			st_float32	m_fHeight;
			st_float32	m_fHeightExponent;
			st_float32	m_fAdherence;				Vec3 pad1;
		};
		SGlobal			m_sGlobal;

		struct SBranchWind
		{
			st_float32	m_fBranchWindTime;
			st_float32	m_fBranchWindDistance;
			st_float32	m_fTwitch;
			st_float32	m_fTwitchFreqScale;

			st_float32	m_fWhip;
			st_float32	m_fDirectionAdherence;
			st_float32	m_fTurbulence;				float pad2;

		};
		SBranchWind		m_sBranch1;
		SBranchWind		m_sBranch2;

		struct SLeaf
		{
			st_float32	m_fRippleTime;
			st_float32	m_fRippleDistance;
			st_float32	m_fLeewardScalar;
			st_float32	m_fTumbleTime;
			st_float32	m_fTumbleFlip;
			st_float32	m_fTumbleTwist;
			st_float32	m_fTumbleDirectionAdherence;
			st_float32	m_fTwitchThrow;
			st_float32	m_fTwitchSharpness;
			st_float32	m_fTwitchTime;				Vec2 pad3;
		};
		SLeaf			m_sLeaf1;
		SLeaf			m_sLeaf2;

		struct SFrondRipple
		{
			st_float32	m_fFrondRippleTime;
			st_float32	m_fFrondRippleDistance;
			st_float32	m_fTile;
			st_float32	m_fLightingScalar;
		};
		SFrondRipple	m_sFrondRipple;

		struct SRolling
		{
			st_float32	m_fBranchFieldMin;
			st_float32	m_fBranchLightingAdjust;
			st_float32	m_fBranchVerticalOffset;
			st_float32	m_fLeafRippleMin;

			st_float32	m_fLeafTumbleMin;
			st_float32	m_fNoisePeriod;
			st_float32	m_fNoiseSize;
			st_float32	m_fNoiseTurbulence;

			st_float32	m_fNoiseTwist;				
			Vec2		m_vOffset;					float pad4;
		};
		SRolling		m_sRolling;
	};

	struct SBaseTreeCBLayout // set once per base tree (SRT file)
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
		Vec2			pad0;
		Vec3			m_vHueVariationColor;

		// ambient image
		st_float32		m_fAmbientImageScalar;

		// billboards
		st_float32		m_fNumBillboards;
		st_float32		m_fRadiansPerImage;			// value computed on CPU to reduce GPU load
		Vec2			pad1;
		Vec4			m_avBillboardTexCoords[MAX_NUM_BILLBOARDS_PER_BASE_TREE];

		// LAVA++
		Vec4			m_vLavaCustomBaseTreeParams;
		// LAVA--
	};

	struct SMaterialCBLayout // multiple materials per base tree
	{
		Vec3		m_vAmbientColor;					float pad0;
		Vec3		m_vDiffuseColor;					float pad1;
		Vec3		m_vSpecularColor;					float pad2;
		Vec3		m_vTransmissionColor;
		st_float32	m_fShininess;

		st_float32	m_BranchSeamWeight;
		st_float32	m_fOneMinusAmbientContrastFactor;
		st_float32	m_fTransmissionShadowBrightness;
		st_float32	m_fTransmissionViewDependency;
		st_float32	m_fAlphaScalar;						Vec3  pad3;

		// lighting flags
		Vec4		m_avEffectConfigFlags[NUM_EFFECT_CONFIG_FLOAT4S];

		// wind settings (from the Modeler)
		Vec4		m_avWindConfigFlags[NUM_WIND_CONFIG_FLOAT4S];

		// wind LOD flags
		Vec4		m_avWindLodFlags[NUM_WIND_LOD_FLOAT4S];

		// LAVA++
		Vec4		m_vLavaCustomMaterialParams;
		// LAVA--
	};

	// these values are used by the SpeedTree reference app example sky and fog systems; will be replaced by client's
	struct SFogAndSkyCBLayout
	{
		// fog
		Vec3			m_vFogColor;
		st_float32		m_fFogDensity;

		st_float32		m_fFogEndDist;
		st_float32		m_fFogSpan;				Vec2  pad3;		// (fog_end_dist - fog_start_dist)
		// todo: comment padding

		// sky
		Vec3			m_vSkyColor;			float pad4;

		// sun
		Vec3			m_vSunColor;
		st_float32		m_fSunSize;
		st_float32		m_fSunSpreadExponent;	Vec3  pad7;
	};

	// these values are used by the SpeedTree reference app example terrain system; will be replaced by client's
	struct STerrainCBLayout
	{
		Vec3			m_vSplatTiles;
		st_float32		m_fNormalMapBlueScalar;
	};

	// these values are used by the SpeedTree reference app example bloom system; will be replaced by client's
	struct SBloomCBLayout
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
		st_float32		pad0;
	};


} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"

