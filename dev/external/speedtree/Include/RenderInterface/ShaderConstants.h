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

// todo: remove this entire file
#pragma once
#include "Core/ExportBegin.h"
#include "Core/Types.h"


///////////////////////////////////////////////////////////////////////
//  Packing

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(push, 4)
#endif


///////////////////////////////////////////////////////////////////////
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////
	//  Constants/limits

	const st_int32 c_nMaxBillboardTexCoords = 24;
	#if defined(SPEEDTREE_OPENGL) || defined(__CELLOS_LV2__) || defined(__psp2__)
		const st_int32 c_nNumMainShaderUniformSlots = 510;
		#define SPEEDTREE_OPENGL_UNIFORM_BLOCK_NAME "A_avSingleUniformBlock"
	#elif defined(_XBOX)
		const st_int32 c_nNumMainShaderUniformSlots = 123;
	#else
		const st_int32 c_nNumMainShaderUniformSlots = 122;
	#endif

	enum EShaderConstantGroupType
	{
		SHADER_CONSTANT_GROUP_SINGLE_INSTANCE,
		SHADER_CONSTANT_GROUP_CAMERA,
		SHADER_CONSTANT_GROUP_LIGHTING,
		SHADER_CONSTANT_GROUP_WIND,
		SHADER_CONSTANT_GROUP_SHADOWS,
		SHADER_CONSTANT_GROUP_BILLBOARDS,
		SHADER_CONSTANT_GROUP_OTHER,
		SHADER_CONSTANT_GROUP_MULTI_INSTANCES,
		SHADER_CONSTANT_GROUP_COUNT
	};

	struct ST_DLL_LINK SStaticShaderConstant
	{
		st_int32					m_nRegister;
		st_int32					m_nNumVectors;	// 1 vector is a float4, 4 is a float4x4
		EShaderConstantGroupType	m_eGroup;
	};

	static const st_int32 c_anUpdateFreqRanges[SHADER_CONSTANT_GROUP_COUNT][2] =
	{
		// pairs are [start_index, # of slots used]
		{   0,   3 }, // SHADER_CONSTANT_GROUP_SINGLE_INSTANCE
		{   3,  21 }, // SHADER_CONSTANT_GROUP_CAMERA
		{  24,  11 }, // SHADER_CONSTANT_GROUP_LIGHTING
		{  35,  29 }, // SHADER_CONSTANT_GROUP_WIND
		{  64,  23 }, // SHADER_CONSTANT_GROUP_SHADOWS
		{  87,  25 }, // SHADER_CONSTANT_GROUP_BILLBOARDS
		{ 112,  10 }, // SHADER_CONSTANT_GROUP_OTHER
		{ 122,   1 }  // SHADER_CONSTANT_GROUP_MULTI_INSTANCES
	};


	// DirectX10/11-only instancing data
	#if defined(SPEEDTREE_DIRECTX10) || defined(SPEEDTREE_DIRECTX11) || defined(_DURANGO) || defined(__ORBIS__)
		const st_int32 c_nMaxInstanceBatchSize = 1000;
		const st_int32 c_nNumSlotsPerInstance = 3;
	#endif

	// OpenGL-only instancing data
	#if defined(SPEEDTREE_OPENGL) || defined(__CELLOS_LV2__) || defined(__psp2__)
		const st_int32 c_nMaxInstanceBatchSize = 129;
		const st_int32 c_nNumSlotsPerInstance = 3;
	#endif

#ifdef OLD_WAY
	///////////////////////////////////////////////////////////////////////
	//  Shader global variable table

	// SHADER_CONSTANT_GROUP_SINGLE_INSTANCE
	const SStaticShaderConstant g_vInstancePosAndScalar   = {   0,   1, SHADER_CONSTANT_GROUP_SINGLE_INSTANCE };
	const SStaticShaderConstant g_vInstanceUpAndLod1      = {   1,   1, SHADER_CONSTANT_GROUP_SINGLE_INSTANCE };
	const SStaticShaderConstant g_vInstanceRightAndLod2   = {   2,   1, SHADER_CONSTANT_GROUP_SINGLE_INSTANCE };

	// SHADER_CONSTANT_GROUP_CAMERA
	const SStaticShaderConstant g_mModelViewProj          = {   3,   4, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_mModelViewProjAlt       = {   7,   4, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_mProjectionInverse      = {  11,   4, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_mCameraFacingMatrix     = {  15,   4, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_vCameraPosition         = {  19,   1, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_vLodRefPosition         = {  20,   1, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_vCameraDirection        = {  21,   1, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_vViewport               = {  22,   1, SHADER_CONSTANT_GROUP_CAMERA };
	const SStaticShaderConstant g_vFarClip                = {  23,   1, SHADER_CONSTANT_GROUP_CAMERA };

	// SHADER_CONSTANT_GROUP_LIGHTING
	const SStaticShaderConstant g_vMaterial0              = {  24,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vMaterial1              = {  25,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vMaterial2              = {  26,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vMaterial3              = {  27,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vMaterial4              = {  28,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vLightDir               = {  29,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vAlphaMaskScalar        = {  30,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vLodProfile             = {  31,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vHueVariationParams     = {  32,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vHueVariationColor      = {  33,   1, SHADER_CONSTANT_GROUP_LIGHTING };
	const SStaticShaderConstant g_vAmbientImageScalar     = {  34,   1, SHADER_CONSTANT_GROUP_LIGHTING };

	// SHADER_CONSTANT_GROUP_WIND
	const SStaticShaderConstant g_vWindVector             = {  35,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindGlobal             = {  36,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindBranch             = {  37,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindBranchTwitch       = {  38,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindBranchWhip         = {  39,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindBranchAnchor       = {  40,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindBranchAdherences   = {  41,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindTurbulences        = {  42,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf1Ripple        = {  43,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf1Tumble        = {  44,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf1Twitch        = {  45,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf2Ripple        = {  46,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf2Tumble        = {  47,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindLeaf2Twitch        = {  48,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindFrondRipple        = {  49,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindRollingBranch      = {  50,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindRollingLeafAndDir  = {  51,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindRollingNoise       = {  52,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindAnimation          = {  53,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions0           = {  54,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions1           = {  55,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions2           = {  56,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions3           = {  57,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions4           = {  58,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions5           = {  59,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions6           = {  60,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions7           = {  61,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions8           = {  62,   1, SHADER_CONSTANT_GROUP_WIND };
	const SStaticShaderConstant g_vWindOptions9           = {  63,   1, SHADER_CONSTANT_GROUP_WIND };

	// SHADER_CONSTANT_GROUP_SHADOWS
	const SStaticShaderConstant g_vShadowMapRanges        = {  64,   1, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_vShadowFadeParams       = {  65,   1, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_vShadowMapTexelOffset   = {  66,   1, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_vShadowMapWritingActive = {  67,   1, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_avShadowSmoothingTable  = {  68,   3, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_mLightModelViewProj0    = {  71,   4, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_mLightModelViewProj1    = {  75,   4, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_mLightModelViewProj2    = {  79,   4, SHADER_CONSTANT_GROUP_SHADOWS };
	const SStaticShaderConstant g_mLightModelViewProj3    = {  83,   4, SHADER_CONSTANT_GROUP_SHADOWS };

	// SHADER_CONSTANT_GROUP_BILLBOARDS
	const SStaticShaderConstant g_vBillboardParams        = {  87,   1, SHADER_CONSTANT_GROUP_BILLBOARDS };
	const SStaticShaderConstant g_avBillboardTexCoords    = {  88,  24, SHADER_CONSTANT_GROUP_BILLBOARDS };

	// SHADER_CONSTANT_GROUP_OTHER
	const SStaticShaderConstant g_vTerrainSplatTiles      = { 112,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vSunColor               = { 113,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vSunParams              = { 114,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vSkyColor               = { 115,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vFogColor               = { 116,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vFogParams              = { 117,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vBloomParams1           = { 118,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vBloomParams2           = { 119,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vBloomParams3           = { 120,   1, SHADER_CONSTANT_GROUP_OTHER };
	const SStaticShaderConstant g_vHandTuned              = { 121,   1, SHADER_CONSTANT_GROUP_OTHER };

	// SHADER_CONSTANT_GROUP_MULTI_INSTANCES
	const SStaticShaderConstant g_vInstancingParams_360   = { 122,   1, SHADER_CONSTANT_GROUP_MULTI_INSTANCES }; // Xbox 360-only
	const SStaticShaderConstant g_avInstanceBlock         = { 122, 387, SHADER_CONSTANT_GROUP_MULTI_INSTANCES }; // OpenGL-only
#endif

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
