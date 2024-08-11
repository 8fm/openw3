///////////////////////////////////////////////////////////////////////  
//
//	All source files prefixed with "My" indicate an example implementation, 
//	meant to detail what an application might (and, in most cases, should)
//	do when interfacing with the SpeedTree SDK.  These files constitute 
//	the bulk of the SpeedTree Reference Application and are not part
//	of the official SDK, but merely example or "reference" implementations.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement
//
//      Copyright (c) 2003-2014 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		http://www.idvinc.com


/////////////////////////////////////////////////////////////////////
// Preprocessor

#include "MyConfigFile.h"
#include "MyCmdLineOptions.h"
#include "Utilities/Utility.h"
using namespace SpeedTree;


/////////////////////////////////////////////////////////////////////
// Prototypes

void GetToken(st_byte*& pData, CFixedString& strToken, st_bool bFixSlashes = false, st_bool bKeepQuotes = false);

template <class T> st_bool ParseData(st_byte*& pData, st_byte* pValue);

typedef st_bool (*ParseFunction)(st_byte*&, st_byte*);
template <class T> ParseFunction GetParseFunction(const T&) { return ParseData<T>; }
template <class T, int U> ParseFunction GetParseFunction(const T (&)[U]) { return ParseData<T[U]>; }

typedef st_byte* (*AllocateFunction)(void);
template <class T> st_byte* Allocate(void) { return (st_byte*)(st_new(T, "CMyConfigFile::SParseNode")); }

typedef void (*DeallocateFunction)(st_byte*);
template <class T> void Deallocate(st_byte* pMem) { st_delete<T>((T*&) pMem); }

typedef void (*StoreFunction)(st_byte*, st_byte*);
template <class T> void Store(st_byte* pStruct, st_byte* pArray) { ((CArray<T>*)pArray)->push_back(*((T*)pStruct)); }


/////////////////////////////////////////////////////////////////////
// struct SParseNode

struct CMyConfigFile::SParseNode
{
							SParseNode( ) :
								m_siOffset(0),
								m_fpParse(NULL),
								m_fpAllocate(NULL),
								m_fpDeallocate(NULL),
								m_fpStore(NULL),
								m_pParent(NULL)
							{
								m_vChildren.SetHeapDescription("CMyConfigFile::SParseNode::CArray");
							}

							~SParseNode( )
							{
								for (size_t i = 0; i < m_vChildren.size( ); ++i)
									st_delete<SParseNode>(m_vChildren[i]);
							}

		CFixedString		m_strName;
		size_t				m_siOffset;
		ParseFunction		m_fpParse;
		AllocateFunction	m_fpAllocate;
		DeallocateFunction	m_fpDeallocate;
		StoreFunction		m_fpStore;
		SParseNode*			m_pParent;
		CArray<SParseNode*>	m_vChildren;
};


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::CMyConfigFile

CMyConfigFile::CMyConfigFile( )
{
	m_aErrors.SetHeapDescription("CMyConfigFile::CArray");
	m_aRandomTreePopulations.SetHeapDescription("CMyConfigFile::CArray");
	m_aManualTreePopulations.SetHeapDescription("CMyConfigFile::CArray");
	m_aStfTreePopulations.SetHeapDescription("CMyConfigFile::CArray");
	m_aSwaTreePopulations.SetHeapDescription("CMyConfigFile::CArray");
	m_aSpeedForestPopulations.SetHeapDescription("CMyConfigFile::CArray");
	m_aGrassPopulations.SetHeapDescription("CMyConfigFile::CArray");
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::~CMyConfigFile

CMyConfigFile::~CMyConfigFile( )
{
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::SetupParser

void CMyConfigFile::SetupParser(SParseNode* pRoot)
{
	SParseNode* pCurrent = pRoot;

	#ifdef __GNUC__
		// GCC emits unwarranted warnings with the default offsetof macro
		#define st_offsetof(T, var ) (((size_t) &((T*) 0x00000001)->var) - 0x00000001)
	#else
		#define st_offsetof(T, var) offsetof(T, var)
	#endif 

	// begin a regular group that is in the data file but not necessarily in code
	#define BEGIN_GROUP(name) { \
		SParseNode* pNew = st_new(SParseNode, "CMyConfigFile::SParseNode"); \
		pNew->m_strName = #name; \
		pNew->m_pParent = pCurrent; \
		pCurrent->m_vChildren.push_back(pNew); \
		pCurrent = pNew; }

	// begin a group that will be added to an array in code
	#define BEGIN_ARRAY_GROUP(name, type, array, parentclass) { \
		SParseNode* pNew = st_new(SParseNode, "CMyConfigFile::SParseNode"); \
		pNew->m_strName = #name; \
		pNew->m_pParent = pCurrent; \
		pNew->m_siOffset = st_offsetof(parentclass, array); \
		pNew->m_fpAllocate = Allocate<type>; \
		pNew->m_fpDeallocate = Deallocate<type>; \
		pNew->m_fpStore = Store<type>; \
		pCurrent->m_vChildren.push_back(pNew); \
		pCurrent = pNew; }

	// end either type of group
	#define END_GROUP() { pCurrent = pCurrent->m_pParent; }

	// parse a parameter
	#define PARAMETER(name, var) { \
		SParseNode* pNew = st_new(SParseNode, "CMyConfigFile::SParseNode"); \
		pNew->m_strName = #name; \
		pNew->m_siOffset = st_offsetof(CMyConfigFile, var); \
		pNew->m_fpParse = GetParseFunction(var); \
		pNew->m_pParent = pCurrent; \
		pCurrent->m_vChildren.push_back(pNew); }

	// parse an array parameter
	#define ARRAY_PARAMETER(name, var, parentclass) { \
		SParseNode* pNew = st_new(SParseNode, "CMyConfigFile::SParseNode"); \
		pNew->m_strName = #name; \
		pNew->m_siOffset = st_offsetof(parentclass, var); \
		parentclass sTemp; \
		pNew->m_fpParse = GetParseFunction(sTemp.var); \
		pNew->m_pParent = pCurrent; \
		pCurrent->m_vChildren.push_back(pNew); }

	// define the parse tree using macros

	// world
	BEGIN_GROUP(world);
		PARAMETER(near_clip, m_sWorld.m_fNearClip);
		PARAMETER(far_clip, m_sWorld.m_fFarClip);
		PARAMETER(field_of_view, m_sWorld.m_fFieldOfView);
		PARAMETER(fov, m_sWorld.m_fFieldOfView);
		PARAMETER(tree_lod_scalar, m_sWorld.m_fGlobalTreeLodScalar);
		PARAMETER(3dtree_cell_size, m_sWorld.m_f3dTreeCellSize);
        PARAMETER(grass_density_scalar, m_sWorld.m_fGrassDensityScalar);
		PARAMETER(random_population_scalar, m_sWorld.m_fRandomPopulationScalar);
		PARAMETER(alpha_test_scalar, m_sWorld.m_fAlphaTestScalar3d);
		PARAMETER(alpha_test_scalar_3d, m_sWorld.m_fAlphaTestScalar3d);
		PARAMETER(alpha_test_scalar_grass, m_sWorld.m_fAlphaTestScalarGrass);
		PARAMETER(alpha_test_scalar_billboard, m_sWorld.m_fAlphaTestScalarBillboards);
		PARAMETER(alpha_test_scalar_billboards, m_sWorld.m_fAlphaTestScalarBillboards);
		PARAMETER(two_pass_blend_scalars, m_sWorld.m_afTwoPassBlendScalars);
		PARAMETER(tree_surface_adhesion, m_sWorld.m_fTreeSurfaceAdhesion);
		PARAMETER(grass_surface_adhesion, m_sWorld.m_fGrassSurfaceAdhesion);
	END_GROUP( );

	// foward_render
	BEGIN_GROUP(forward_render);
		// depth pre-pass (four aliases)
		PARAMETER(depth_prepass, m_sForwardRender.m_bDepthOnlyPrepass);
		PARAMETER(depth_pre_pass, m_sForwardRender.m_bDepthOnlyPrepass);
		PARAMETER(z_pre_pass, m_sForwardRender.m_bDepthOnlyPrepass);
		PARAMETER(z_prepass, m_sForwardRender.m_bDepthOnlyPrepass);
		PARAMETER(post_effects_shader_path, m_sForwardRender.m_strPostEffectsShaderPath);
		PARAMETER(shader_path, m_sForwardRender.m_strPostEffectsShaderPath);
	END_GROUP( );

	// deferred_render
	BEGIN_GROUP(deferred_render);
		PARAMETER(enabled, m_sDeferredRender.m_bEnabled);
		PARAMETER(distance_blur, m_sDeferredRender.m_bDistanceBlur);
		PARAMETER(post_effects_shader_path, m_sDeferredRender.m_strPostEffectsShaderPath);
		PARAMETER(shader_path, m_sDeferredRender.m_strPostEffectsShaderPath);
	END_GROUP( );

	// navigation
	BEGIN_GROUP(navigation);
		PARAMETER(camera_file, m_sNavigation.m_strCameraFilename);
		PARAMETER(follow_height, m_sNavigation.m_fFollowHeight);
		PARAMETER(speed_scalar, m_sNavigation.m_fSpeedScalar);
		PARAMETER(start_pos, m_sNavigation.m_afStartingPosition);
		PARAMETER(start_dir, m_sNavigation.m_afStartingAzimuthAndPitch);
	END_GROUP( );

	// wind
	BEGIN_GROUP(wind);
		PARAMETER(enabled, m_sWind.m_bEnabled);
		PARAMETER(gusting, m_sWind.m_bGusting);
		PARAMETER(strength, m_sWind.m_fStrength);
		PARAMETER(direction, m_sWind.m_vDirection);
		PARAMETER(dir, m_sWind.m_vDirection);
	END_GROUP( );

	// fog
	BEGIN_GROUP(fog);
		PARAMETER(linear, m_sFog.m_afLinear);
		PARAMETER(color, m_sFog.m_vColor);
		PARAMETER(density, m_sFog.m_fDensity);
	END_GROUP( );

	// lighting
	BEGIN_GROUP(lighting);
		PARAMETER(ambient, m_sLighting.m_vAmbient);
		PARAMETER(diffuse, m_sLighting.m_vDiffuse);
		PARAMETER(specular, m_sLighting.m_vSpecular);
		PARAMETER(transmission, m_sLighting.m_vTransmission);
		PARAMETER(direction, m_sLighting.m_vDirection);
		BEGIN_GROUP(twilight);
			PARAMETER(horizon_color, m_sLighting.m_sTwilight.m_vHorizonColor);
			PARAMETER(ambient, m_sLighting.m_sTwilight.m_vAmbient);
			PARAMETER(diffuse, m_sLighting.m_sTwilight.m_vDiffuse);
			PARAMETER(specular, m_sLighting.m_sTwilight.m_vSpecular);
			PARAMETER(transmission, m_sLighting.m_sTwilight.m_vTransmission);
		END_GROUP( );
		BEGIN_GROUP(image_based_ambient_lighting);
			PARAMETER(filename, m_sLighting.m_sImageBasedAmbientLighting.m_strFilename);
		END_GROUP( );
		BEGIN_GROUP(bloom);
			PARAMETER(enabled, m_sLighting.m_sBloom.m_bEnabled);
			 PARAMETER(downsample, m_sLighting.m_sBloom.m_nDownsample);
			 PARAMETER(bright_pass, m_sLighting.m_sBloom.m_fBrightPass);
			 PARAMETER(bright_floor, m_sLighting.m_sBloom.m_fBrightFloor);
			 PARAMETER(blur, m_sLighting.m_sBloom.m_nBlur);
			 PARAMETER(sky_bleed, m_sLighting.m_sBloom.m_fSkyBleed);
			 PARAMETER(bloom_effect_scalar, m_sLighting.m_sBloom.m_fBloomEffectScalar);
			 PARAMETER(main_effect_scalar, m_sLighting.m_sBloom.m_fMainEffectScalar);
		END_GROUP( );
	END_GROUP( );

	// shadows
	BEGIN_GROUP(shadows);
		PARAMETER(enabled, m_sShadows.m_bEnabled);
		PARAMETER(resolution, m_sShadows.m_nResolution);
		PARAMETER(num_maps, m_sShadows.m_nNumMaps);
		PARAMETER(map_distances, m_sShadows.m_afMapRanges);
		PARAMETER(map_update_intervals, m_sShadows.m_anMapUpdateIntervals);
		PARAMETER(fade, m_sShadows.m_fFadePercent);
		PARAMETER(fade_percent, m_sShadows.m_fFadePercent);
		PARAMETER(behind_camera_dist, m_sShadows.m_fBehindCameraDistance);
		PARAMETER(behind_camera_distance, m_sShadows.m_fBehindCameraDistance);
	END_GROUP( );

	// terrain
	BEGIN_GROUP(terrain);
		// general
		PARAMETER(shader_path, m_sTerrain.m_strShaderPath);
		PARAMETER(max_resolution, m_sTerrain.m_nMaxResolution);
		PARAMETER(lod_levels, m_sTerrain.m_nNumLodLevels);
		PARAMETER(cell_size, m_sTerrain.m_fCellSize);
		PARAMETER(tiles, m_sTerrain.m_nTiles);
		// lighting
		PARAMETER(normal_map_filename, m_sTerrain.m_strNormalMapFilename);
		PARAMETER(ambient, m_sTerrain.m_vAmbient);
		PARAMETER(diffuse, m_sTerrain.m_vDiffuse);
		PARAMETER(ambient_occlusion_brightness, m_sTerrain.m_fAmbientOcclusionBrightness);
		PARAMETER(ambient_occlusion_distance, m_sTerrain.m_fAmbientOcclusionDistance);
		PARAMETER(cast_shadows, m_sTerrain.m_bCastShadows);
		PARAMETER(casts_shadows, m_sTerrain.m_bCastShadows);
		// height map
		PARAMETER(height_map_filename, m_sTerrain.m_strHeightMapFilename);
		PARAMETER(height_map_size, m_sTerrain.m_vHeightMapSize);
		// splat map
		PARAMETER(splat_map_filename, m_sTerrain.m_strSplatMapFilename);
		BEGIN_GROUP(splat_layer_0);
			PARAMETER(filename, m_sTerrain.m_asSplatLayers[0].m_strFilename);
			PARAMETER(repeat, m_sTerrain.m_asSplatLayers[0].m_fRepeat);
			PARAMETER(approx_color, m_sTerrain.m_asSplatLayers[0].m_vApproxColor);
		END_GROUP( );
		BEGIN_GROUP(splat_layer_1);
			PARAMETER(filename, m_sTerrain.m_asSplatLayers[1].m_strFilename);
			PARAMETER(repeat, m_sTerrain.m_asSplatLayers[1].m_fRepeat);
			PARAMETER(approx_color, m_sTerrain.m_asSplatLayers[1].m_vApproxColor);
		END_GROUP( );
		BEGIN_GROUP(splat_layer_2);
			PARAMETER(filename, m_sTerrain.m_asSplatLayers[2].m_strFilename);
			PARAMETER(repeat, m_sTerrain.m_asSplatLayers[2].m_fRepeat);
			PARAMETER(approx_color, m_sTerrain.m_asSplatLayers[2].m_vApproxColor);
		END_GROUP( );
	END_GROUP( );

	// sky
	BEGIN_GROUP(sky);
		PARAMETER(shader_path, m_sSky.m_strShaderPath);
		PARAMETER(color, m_sSky.m_vColor);
		PARAMETER(fog_range, m_sSky.m_afFogRange);
		PARAMETER(sun_size, m_sSky.m_fSunSize);
		PARAMETER(sun_spread_exponent, m_sSky.m_fSunSpreadExponent);
		PARAMETER(sun_color, m_sSky.m_vSunColor);
		PARAMETER(texture, m_sSky.m_strTexture);
		BEGIN_GROUP(twilight);
			PARAMETER(sun_size, m_sSky.m_sTwilight.m_fSunSize);
			PARAMETER(sun_spread_exponent, m_sSky.m_sTwilight.m_fSunSpreadExponent);
			PARAMETER(color, m_sSky.m_sTwilight.m_vColor);
			PARAMETER(sun_color, m_sSky.m_sTwilight.m_vSunColor);
		END_GROUP( );
	END_GROUP( );

	// demo
	BEGIN_GROUP(demo);
		PARAMETER(camera_time, m_sDemo.m_afCameraTime);
		PARAMETER(camera_speed, m_sDemo.m_afCameraSpeed);
		PARAMETER(sun_speed, m_sDemo.m_fSunSpeed);
		PARAMETER(keyboard_lock, m_sDemo.m_bKeyboardLock);
	END_GROUP( );

	// overlays
	BEGIN_GROUP(overlays);
		PARAMETER(shader_path, m_strOverlayShaderPath);
		BEGIN_ARRAY_GROUP(overlay, SOverlay, m_aOverlays, CMyConfigFile);
			ARRAY_PARAMETER(filename, m_strFilename, SOverlay);
			ARRAY_PARAMETER(pos, m_afPos, SOverlay);
			ARRAY_PARAMETER(size, m_fSize, SOverlay);
			ARRAY_PARAMETER(anchor, m_nAnchor, SOverlay);
		END_GROUP( );
	END_GROUP( );

	// random trees
	BEGIN_ARRAY_GROUP(random_tree_population, SRandomTreePopulationParams, m_aRandomTreePopulations, CMyConfigFile);
		ARRAY_PARAMETER(filename, m_strSrtFilename, SRandomTreePopulationParams);
		ARRAY_PARAMETER(quantity, m_nQuantity, SRandomTreePopulationParams);
		ARRAY_PARAMETER(seed, m_nSeed, SRandomTreePopulationParams);
		ARRAY_PARAMETER(area, m_vArea, SRandomTreePopulationParams);
		ARRAY_PARAMETER(lod_scalar, m_fLodScalar, SRandomTreePopulationParams);
		ARRAY_PARAMETER(scalar_range, m_afScalarRange, SRandomTreePopulationParams);
		ARRAY_PARAMETER(elevation_range, m_afElevationRange, SRandomTreePopulationParams);
		ARRAY_PARAMETER(slope_range, m_afSlopeRange, SRandomTreePopulationParams);

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SRandomTreePopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SRandomTreePopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SRandomTreePopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SRandomTreePopulationParams);
	END_GROUP( );

	// manual trees
	BEGIN_ARRAY_GROUP(manual_tree_population, SManualTreePopulationParams, m_aManualTreePopulations, CMyConfigFile);
		ARRAY_PARAMETER(filename, m_strSrtFilename, SManualTreePopulationParams);
		ARRAY_PARAMETER(lod_scalar, m_fLodScalar, SManualTreePopulationParams);
		ARRAY_PARAMETER(follow_terrain, m_bFollowTerrain, SManualTreePopulationParams);
		BEGIN_ARRAY_GROUP(instance, SManualTreePopulationParams::SInstance, SManualTreePopulationParams::m_aInstances, CMyConfigFile::SManualTreePopulationParams);
			ARRAY_PARAMETER(pos, m_vPos, SManualTreePopulationParams::SInstance);
			ARRAY_PARAMETER(xyz, m_vPos, SManualTreePopulationParams::SInstance);
			ARRAY_PARAMETER(scalar, m_fScalar, SManualTreePopulationParams::SInstance);
			ARRAY_PARAMETER(up, m_vUp, SManualTreePopulationParams::SInstance);
			ARRAY_PARAMETER(right, m_vRight, SManualTreePopulationParams::SInstance);
		END_GROUP( );

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SManualTreePopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SManualTreePopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SManualTreePopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SManualTreePopulationParams);
	END_GROUP( );

	// stf files
	BEGIN_ARRAY_GROUP(stf_tree_population, SStfTreePopulationParams, m_aStfTreePopulations, CMyConfigFile);
		ARRAY_PARAMETER(filename, m_strFilename, SStfTreePopulationParams);
		ARRAY_PARAMETER(follow_terrain, m_bFollowTerrain, SStfTreePopulationParams);

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SStfTreePopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SStfTreePopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SStfTreePopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SStfTreePopulationParams);
	END_GROUP( );

	// swa files
	BEGIN_ARRAY_GROUP(swa_tree_population, SSwaTreePopulationParams, m_aSwaTreePopulations, CMyConfigFile);
		ARRAY_PARAMETER(filename, m_strFilename, SSwaTreePopulationParams);
		ARRAY_PARAMETER(follow_terrain, m_bFollowTerrain, SSwaTreePopulationParams);

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SSwaTreePopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SSwaTreePopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SSwaTreePopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SSwaTreePopulationParams);
	END_GROUP( );

	// speedforest files
	BEGIN_ARRAY_GROUP(speedforest_population, SSpeedForestPopulationParams, m_aSpeedForestPopulations, CMyConfigFile);
		ARRAY_PARAMETER(speedforest_filename, m_strSpeedForestFilename, SSpeedForestPopulationParams);
		ARRAY_PARAMETER(srt_filename, m_strSrtFilename, SSpeedForestPopulationParams);
		ARRAY_PARAMETER(follow_terrain, m_bFollowTerrain, SSpeedForestPopulationParams);
		ARRAY_PARAMETER(lod_scalar, m_fLodScalar, SSpeedForestPopulationParams);

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SSpeedForestPopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SSpeedForestPopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SSpeedForestPopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SSpeedForestPopulationParams);
	END_GROUP( );

	// grass
	BEGIN_ARRAY_GROUP(grass_population, SGrassPopulationParams, m_aGrassPopulations, CMyConfigFile);
		ARRAY_PARAMETER(filename, m_strSrtFilename, SGrassPopulationParams);
		ARRAY_PARAMETER(density, m_fDensity, SGrassPopulationParams);
		ARRAY_PARAMETER(scalar_range, m_afScalarRange, SGrassPopulationParams);
		ARRAY_PARAMETER(surface_adhesion, m_fSurfaceAdhesion, SGrassPopulationParams);
		ARRAY_PARAMETER(follow_terrain, m_bFollowTerrain, SGrassPopulationParams);
		ARRAY_PARAMETER(elevation_range, m_afElevationRange, SGrassPopulationParams);
		ARRAY_PARAMETER(slope_range, m_afSlopeRange, SGrassPopulationParams);
		ARRAY_PARAMETER(lod_range, m_afLodRange, SGrassPopulationParams);
		ARRAY_PARAMETER(mask_filename, m_strMaskFilename, SGrassPopulationParams);
		ARRAY_PARAMETER(mask_channel, m_eMaskChannel, SGrassPopulationParams);
		ARRAY_PARAMETER(area, m_vArea, SGrassPopulationParams);
		ARRAY_PARAMETER(cell_size, m_fCellSize, SGrassPopulationParams);
		ARRAY_PARAMETER(cast_shadows, m_bCastShadows, SGrassPopulationParams);
		ARRAY_PARAMETER(casts_shadows, m_bCastShadows, SGrassPopulationParams);

		// hue variation (common to all population blocks)
		BEGIN_GROUP(hue_variation);
			ARRAY_PARAMETER(by_pos, m_sHueVariationParams.m_fByPos, SGrassPopulationParams);
			ARRAY_PARAMETER(by_vertex, m_sHueVariationParams.m_fByVertex, SGrassPopulationParams);
			ARRAY_PARAMETER(color, m_sHueVariationParams.m_vColor, SGrassPopulationParams);
		END_GROUP( );

		ARRAY_PARAMETER(image_ambient_scalar, m_fAmbientImageScalar, SGrassPopulationParams);
	END_GROUP( );
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::Load

st_bool CMyConfigFile::Load(const CFixedString& strFilename, SMyCmdLineOptions* pCmdLineOptions)
{
	m_aErrors.clear( );

	CString strData;
	strData.SetHeapDescription("CMyConfigFile CString");
	if (!Preprocess(strFilename, strData))
	{
		m_aErrors.push_back(CFixedString::Format("Could not open file [%s]", strFilename.c_str( )));
	}
	else
	{
		// return the file path so that AssetFullPath() will work
		m_strConfigFilePath = strFilename.Path( );

		// create the root of the parse tree
		SParseNode sRoot;
		SetupParser(&sRoot);

		// parse the data as a byte stream
		st_byte* pData = (st_byte*) strData.c_str( );
		Parse(pData, &sRoot, (st_byte*) this);

		// post-process those values that require it (e.g. global random population scalar gets
		// multiplied against local population quantities, etc)
		PostProcess(pCmdLineOptions);
	}

	return m_aErrors.empty( );	
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::AssetFullPath

CFixedString CMyConfigFile::AssetFullPath(const CFixedString& strPartialFilename) const
{
	if (IsAbsolutePath(strPartialFilename.c_str( )))
		return strPartialFilename;
	else
		return CFileSystem::CleanPlatformFilename(m_strConfigFilePath + "/" + strPartialFilename);
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFileParser::GetErrors

const CArray<CFixedString>&	CMyConfigFile::GetErrors(void) const
{
	return m_aErrors;
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::Preprocess

st_bool CMyConfigFile::Preprocess(const CFixedString& strFilename, CString& strData)
{
	st_bool bReturn = false;
	strData.clear( );

	FILE* pFile = fopen(strFilename.c_str( ), "r");
	if (pFile != NULL)
	{
		// find size of file
		fseek(pFile, 0L, SEEK_END);
        st_int32 nNumBytes = st_int32(ftell(pFile));
        if (fseek(pFile, 0, SEEK_SET) >= 0)
		{
			// read whole file into buffer, removing comments and newlines
			CString strTokenData;
			strTokenData.SetHeapDescription("CMyConfigFile CString");
			strTokenData.reserve(nNumBytes * 2);
			st_bool bLastSlash = false;
			st_bool bLastStar = false;
			st_bool bIgnoringComment = false;
			st_bool bCommentCPlusPlus = false;
			st_bool bLastWhitespace = false;
			do
			{
				st_char chTemp = st_char(fgetc(pFile));

				if (bIgnoringComment)
				{
					if (bCommentCPlusPlus)
					{
						if (chTemp == '\r' || chTemp == '\n')
							bIgnoringComment = false;
					}
					else
					{
						if (bLastStar && chTemp == '/')
							bIgnoringComment = false;
						bLastStar = (chTemp == '*');
					}
					bLastSlash = false;
				}
				else
				{
					switch (chTemp)
					{
					case '*':
						if (bLastSlash)
						{
							bIgnoringComment = true;
							bCommentCPlusPlus = false;
							bLastStar = false;
						}
						else
						{
							strTokenData += chTemp;
							bLastWhitespace = false;
						}
						bLastSlash = false;
						break;
					case '/':
						if (bLastSlash)
						{
							bIgnoringComment = true;
							bCommentCPlusPlus = true;
						}
						bLastSlash = true;
						bLastStar = false;
						break;
					case '\r':
					case '\n':
					case ' ':
					case '\t':
					case -1: // eof
						if (bLastSlash)
							strTokenData += '/';
						if (!bLastWhitespace)
							strTokenData += ' ';
						bLastSlash = false;
						bLastStar = false;
						bLastWhitespace = true;
						break;
					default:
						{
							if (bLastSlash)
								strTokenData += '/';
							if ((chTemp == '{' || chTemp == '}') && !bLastWhitespace)
								strTokenData += ' '; 
							strTokenData += chTemp;
							if (chTemp == '{' || chTemp == '}')
							{
								// make sure of a buffer around brackets
								strTokenData += ' ';
								bLastWhitespace = true;
							}
							else
								bLastWhitespace = false;
							bLastSlash = false;
							bLastStar = false;
						}
						break;
					}
				}
			}
			while (!feof(pFile));

			// now scan the tokens for defines and apply them accordingly
			CMap<CFixedString, CFixedString> mapDefines;
			mapDefines.SetHeapDescription("CMyConfigFile CMap");

			st_byte* pData = (st_byte*) strTokenData.c_str( );
			strData.reserve(strTokenData.size( ));
			CFixedString strToken;
			st_bool bSkippingPlatform = false;
			while (*pData != 0)
			{
				st_bool bHandled = false;
				GetToken(pData, strToken, false, true);

				if (Strcmpi(strToken.substr(0, 10), "#platform_"))
				{
					bSkippingPlatform = true;
					if (Strcmpi(strToken, "#platform_all") ||
						#if defined(__CELLOS_LV2__)
							Strcmpi(strToken, "#platform_ps3"))
						#elif defined(_XBOX)
							Strcmpi(strToken, "#platform_xbox"))
						#elif defined(_DURANGO)
							Strcmpi(strToken, "#platform_durango"))
						#elif defined(__ORBIS__)
							Strcmpi(strToken, "#platform_orbis"))
						#elif defined(__psp2__)
							Strcmpi(strToken, "#platform_psp2"))
						#else
							Strcmpi(strToken, "#platform_win"))
						#endif
					{
						bSkippingPlatform = false;
						continue;
					}
				}

				if (bSkippingPlatform)
					continue;
				
				if (Strcmpi(strToken, "#define"))
				{
					CFixedString strName;
					GetToken(pData, strName);
					GetToken(pData, strToken);
					mapDefines[strName] = strToken;
					bHandled = true;
				}

				if (!bHandled)
				{
					st_uint32 uiStartPos = 0;
					st_uint32 uiEndPos = 0;
					do
					{
						uiStartPos = st_uint32(strToken.find('%', 0));
						uiEndPos = st_uint32(strToken.find('%', uiStartPos + 1));
						if (uiStartPos != st_uint32(CFixedString::npos) && uiEndPos != st_uint32(CFixedString::npos))
						{
							CFixedString strSub = strToken.substr(uiStartPos + 1, uiEndPos - uiStartPos - 1);
							CMap<CFixedString, CFixedString>::iterator iter = mapDefines.find(strSub);
							if (iter == mapDefines.end( ))
							{
								Error("Unrecognized define [%s]\n", strSub.c_str( ));
								strToken = strToken.substr(0, uiStartPos) + strToken.substr(uiEndPos + 1);
							}
							else
							{
								strToken = strToken.substr(0, uiStartPos) + iter->second + strToken.substr(uiEndPos + 1);
							}
						}
					}
					while (uiStartPos != st_uint32(CFixedString::npos) && uiEndPos != st_uint32(CFixedString::npos));

					strData += ' ';
					strData += strToken.c_str( );
				}
			}

			while (!strData.empty( ) && strData[strData.length( ) - 1] == ' ')
				strData.resize(strData.size( ) - 1);

			bReturn = true;
		}

		fclose(pFile);
	}

	return bReturn;
}


/////////////////////////////////////////////////////////////////////
// GetToken

void GetToken(st_byte*& pData, CFixedString& strToken, st_bool bFixSlashes, st_bool bKeepQuotes)
{
	strToken.clear( );
	while (*pData == ' ')
		++pData;
	if (*pData == '"')
	{
		if (!bKeepQuotes)
			++pData;
		do
		{
			strToken += *pData++;
		}
		while (*pData != 0 && *pData != '"');
		if (bKeepQuotes)
			strToken += *pData;
		++pData;
	}
	else if (*pData != 0)
	{
		do
		{
			strToken += *pData++;
		}
		while (*pData != 0 && *pData != ' ');
	}

	
	#if defined(_XBOX) || defined(__CELLOS_LV2__) || defined(__APPLE__)
		if (bFixSlashes)
		{
			st_char* pChar = (st_char*)strToken.c_str( );
			do
			{
				#ifdef _XBOX
					if (*pChar == '/')
						*pChar = '\\';
				#endif
				#if defined(__CELLOS_LV2__) || defined(__APPLE__)
					if (*pChar == '\\')
						*pChar = '/';
				#endif
			}
			while (*(++pChar) != 0);
		}
	#else
		ST_UNREF_PARAM(bFixSlashes);
	#endif
}


/////////////////////////////////////////////////////////////////////
// PutTokenBack

void PutTokenBack(st_byte*& pData, const CFixedString& strToken)
{
	st_uint32 uiSize = (st_uint32)strToken.size( );

	if (*(pData - 1) == '"')
		uiSize += 2;

	pData -= uiSize;
}


/////////////////////////////////////////////////////////////////////
// CMyConfigFile::Parse

void CMyConfigFile::Parse(st_byte*& pData, SParseNode* pCurrent, st_byte* pDataOffset, const CFixedString& strHierarchy)
{
	if (*pData == 0)
		return;

	if (pCurrent->m_vChildren.empty( ))
	{
		// parse parameter data
		if (pCurrent->m_fpParse != NULL && !pCurrent->m_fpParse(pData, pDataOffset + pCurrent->m_siOffset))
			m_aErrors.push_back(CFixedString::Format("Parameter [%s] did not parse correctly", strHierarchy.c_str( )));
	}
	else
	{
		CFixedString strToken;

		// check we're opening a block for this token
		if (!pCurrent->m_strName.empty( ))
		{
			GetToken(pData, strToken);
			if (strToken != "{")
			{
				PutTokenBack(pData, strToken);
				m_aErrors.push_back(CFixedString::Format("Missing block after token [%s]", pCurrent->m_strName.c_str( )));
				return;
			}
		}

		// allocate array data if needed
		st_byte* pArrayData = NULL;
		if (pCurrent->m_fpAllocate != NULL)
			pArrayData = pCurrent->m_fpAllocate( );

		while (*pData != 0)
		{
			GetToken(pData, strToken);

			// finished block
			if (strToken.empty( ) || strToken == "}")
				break;

			CFixedString strThisHierarchy = strHierarchy;
			if (!strThisHierarchy.empty( ))
				strThisHierarchy += ':';
			strThisHierarchy += strToken;

			st_bool bHandled = false;
			for (CArray<SParseNode*>::iterator iter = pCurrent->m_vChildren.begin( ); !bHandled && iter != pCurrent->m_vChildren.end( ); ++iter)
			{
				if (Strcmpi((*iter)->m_strName.c_str( ), strToken.c_str( )))
				{
					Parse(pData, *iter, (pArrayData ? pArrayData : pDataOffset), strThisHierarchy);
					bHandled = true;
				}
			}

			if (!bHandled)
			{
				GetToken(pData, strToken);
				if (strToken == "{")
				{
					while (!strToken.empty( ) && strToken != "}")
						GetToken(pData, strToken);
					m_aErrors.push_back(CFixedString::Format("Skipped unrecognized block [%s]", strThisHierarchy.c_str( )));
				}
				else
				{
					PutTokenBack(pData, strToken);
					m_aErrors.push_back(CFixedString::Format("Skipped unrecognized parameter [%s]", strThisHierarchy.c_str( )));
				}					
			}
		}

		// save array data if needed
		if (pCurrent->m_fpAllocate != NULL)
		{
			pCurrent->m_fpStore(pArrayData, pDataOffset + pCurrent->m_siOffset);
			pCurrent->m_fpDeallocate(pArrayData);
		}
	}
}


/////////////////////////////////////////////////////////////////////
//	CMyConfigFile::PostProcess
//
//	React to or make corrections to the input values.

void CMyConfigFile::PostProcess(SMyCmdLineOptions* pCmdLineOptions)
{
	if (m_sDeferredRender.m_bEnabled)
	{
		#ifdef __psp2__
			Error("The Reference Application for PS Vita does not have a fully-implemented deferred renderer");
			exit(-1);
		#endif
	}

	// filename paths
	{
		#define CONVERT_TO_FULL_PATH(path) (path) = AssetFullPath(path)
		#define CONVERT_ARRAY_TO_FULL_PATH(array, path) for (size_t i = 0; i < array.size( ); ++i) array[i].path = AssetFullPath(array[i].path)

		CONVERT_TO_FULL_PATH(m_sForwardRender.m_strPostEffectsShaderPath);
		CONVERT_TO_FULL_PATH(m_sDeferredRender.m_strPostEffectsShaderPath);
		CONVERT_TO_FULL_PATH(m_sNavigation.m_strCameraFilename);
		CONVERT_TO_FULL_PATH(m_sLighting.m_sImageBasedAmbientLighting.m_strFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_strShaderPath);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_strNormalMapFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_strHeightMapFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_strSplatMapFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_asSplatLayers[0].m_strFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_asSplatLayers[1].m_strFilename);
		CONVERT_TO_FULL_PATH(m_sTerrain.m_asSplatLayers[2].m_strFilename);
		CONVERT_TO_FULL_PATH(m_sSky.m_strShaderPath);
		CONVERT_TO_FULL_PATH(m_sSky.m_strTexture);
		CONVERT_TO_FULL_PATH(m_strOverlayShaderPath);
		CONVERT_ARRAY_TO_FULL_PATH(m_aOverlays, m_strFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aRandomTreePopulations, m_strSrtFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aManualTreePopulations, m_strSrtFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aStfTreePopulations, m_strFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aSwaTreePopulations, m_strFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aSpeedForestPopulations, m_strSpeedForestFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aSpeedForestPopulations, m_strSrtFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aGrassPopulations, m_strSrtFilename);
		CONVERT_ARRAY_TO_FULL_PATH(m_aGrassPopulations, m_strMaskFilename);

		#undef CONVERT_TO_FULL_PATH
	}

	// adjust terrain resolution
	const st_int32 c_nMaxTerrainRes = 129;
	if (m_sTerrain.m_nMaxResolution > c_nMaxTerrainRes)
	{
		CCore::SetError("Terrain resolution of [%d] specified, clamping to %d\n", m_sTerrain.m_nMaxResolution, c_nMaxTerrainRes);
		m_sTerrain.m_nMaxResolution = c_nMaxTerrainRes;
	}

	// adjust population quantities based on world population scalar
	for (size_t i = 0; i < m_aRandomTreePopulations.size( ); ++i)
		m_aRandomTreePopulations[i].m_nQuantity = st_int32(m_aRandomTreePopulations[i].m_nQuantity * m_sWorld.m_fRandomPopulationScalar);

    // adjust grass densities based on world grass density scalar
	for (size_t i = 0; i < m_aGrassPopulations.size( ); ++i)
        m_aGrassPopulations[i].m_fDensity *= m_sWorld.m_fGrassDensityScalar;

	// adjust grass surface adhesion values
	for (size_t i = 0; i < m_aGrassPopulations.size( ); ++i)
		m_aGrassPopulations[i].m_fSurfaceAdhesion = (m_aGrassPopulations[i].m_fSurfaceAdhesion < 0.0f) ? m_sWorld.m_fGrassSurfaceAdhesion : m_aGrassPopulations[i].m_fSurfaceAdhesion;

	// clamp values
	m_sShadows.m_nNumMaps = Clamp(m_sShadows.m_nNumMaps, 1, c_nMaxNumShadowMaps);

	// camera direction
	if (CCoordSys::IsLeftHanded( ))
	{
		m_sNavigation.m_afStartingAzimuthAndPitch[0] = -m_sNavigation.m_afStartingAzimuthAndPitch[0];
		if (CCoordSys::IsYAxisUp( ))
			m_sNavigation.m_afStartingAzimuthAndPitch[1] = -m_sNavigation.m_afStartingAzimuthAndPitch[1];
	}

	// only certain platforms have support for deferred + multisampling in our example setup
	#if !defined(SPEEDTREE_DIRECTX11) && !defined(_DURANGO) && !defined(__ORBIS__)
		assert(pCmdLineOptions);
		if (pCmdLineOptions->m_nNumSamples > 1 && (m_sDeferredRender.m_bEnabled || m_sLighting.m_sBloom.m_bEnabled))
		{
			Warning("This platform doesn't currently support deferred rendering + multisampling, setting samples to 1");
			pCmdLineOptions->m_nNumSamples = 1;
		}
	#else
		ST_UNREF_PARAM(pCmdLineOptions);
	#endif
}


/////////////////////////////////////////////////////////////////////////////
//	Function::IsDigit

inline st_bool IsDigit(st_char chChar)
{
	return (chChar >= '0' && chChar <= '9');
}


/////////////////////////////////////////////////////////////////////////////
//	Function::ParseInt

inline st_bool ParseInt(st_byte*& pData, st_int32& iReturn)
{
	iReturn = 0;

	if (*pData == ' ')
		++pData;

	st_bool bNegative = false;
	if (*pData == '-')
	{
		bNegative = true;
		++pData;
	}
	else if (*pData == '+')
		++pData;

	st_bool bFound = false;
	while (IsDigit(*pData) && *pData != 0)
	{
		iReturn *= 10;
		iReturn += (*pData++ - 48);
		bFound = true;
	}

	if (!bFound)
		return false;

	if (bNegative)
		iReturn = -iReturn;

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//	Function::ParseFloat

inline st_bool ParseFloat(st_byte*& pData, st_float32& fReturn)
{
	fReturn = 0.0f;

	if (*pData == ' ')
		++pData;

	st_bool bNegative = false;
	if (*pData == '-')
	{
		bNegative = true;
		++pData;
	}
	else if (*pData == '+')
		++pData;

	st_bool bFound = false;
	while (IsDigit(*pData) && *pData != 0)
	{
		fReturn *= 10.0f;
		fReturn += (*pData++ - 48);
		bFound = true;
	}

	if (*pData == '.')
	{
		++pData;
		st_float32 fPlace = 0.1f;
		st_float32 fFrac = 0.0f;
		while (IsDigit(*pData) && *pData != 0)
		{
			fFrac += (*pData++ - 48) * fPlace;
			fPlace *= 0.1f;
			bFound = true;
		}
		fReturn += fFrac;
	}

	if (!bFound)
		return false;

	if (*pData == 'e' || *pData == 'E')
	{
		++pData;
		int iExponent;
		if (ParseInt(pData, iExponent))
		{
			fReturn *= (st_float32)pow(10.0f, st_float32(iExponent));
		}
	}

	if (bNegative)
		fReturn = -fReturn;

	return true;
}


/////////////////////////////////////////////////////////////////////////////
//	Function::ParseBool

inline st_bool ParseBool(st_byte*& pData, st_bool& bReturn)
{
	st_bool bSuccess = false;
	
	CFixedString strTemp;
	GetToken(pData, strTemp);
	
	bReturn = false;
	if (strTemp == "0" || Strcmpi(strTemp, "false"))
	{
		bSuccess = true;
		bReturn = false;
	}
	else if (strTemp == "1" || Strcmpi(strTemp, "true"))
	{
		bSuccess = true;
		bReturn = true;
	}

	return bSuccess;	
}


/////////////////////////////////////////////////////////////////////////////
//	Function::ParseChannels

inline st_bool ParseChannels(st_byte*& pData, EChannels& eReturn)
{
	CFixedString strTemp;
	GetToken(pData, strTemp);
	
	st_bool bSuccess = true;
	eReturn = CHANNEL_NONE;
	if (Strcmpi(strTemp, "red"))
		eReturn = CHANNEL_RED;
	else if (Strcmpi(strTemp, "green"))
		eReturn = CHANNEL_GREEN;
	else if (Strcmpi(strTemp, "blue"))
		eReturn = CHANNEL_BLUE;
	else if (Strcmpi(strTemp, "alpha"))
		eReturn = CHANNEL_ALPHA;
	else if (Strcmpi(strTemp, "all"))
		eReturn = CHANNEL_ALL;
	else
		bSuccess = false;

	return bSuccess;	
}


/////////////////////////////////////////////////////////////////////
// Template specializations for ParseData

template <> st_bool ParseData<st_bool>(st_byte*& pData, st_byte* pValue) { return ParseBool(pData, *(st_bool*)pValue); }
template <> st_bool ParseData<st_float32>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, *(st_float32*)pValue); }
template <> st_bool ParseData<st_float32[2]>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, *(st_float32*)pValue) && ParseFloat(pData, *((st_float32*)pValue + 1)); }
template <> st_bool ParseData<st_float32[4]>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, *(st_float32*)pValue) && ParseFloat(pData, *((st_float32*)pValue + 1)) && ParseFloat(pData, *((st_float32*)pValue + 2)) && ParseFloat(pData, *((st_float32*)pValue + 3)); }
template <> st_bool ParseData<st_float32[5]>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, *(st_float32*)pValue) && ParseFloat(pData, *((st_float32*)pValue + 1)) && ParseFloat(pData, *((st_float32*)pValue + 2)) && ParseFloat(pData, *((st_float32*)pValue + 3)) && ParseFloat(pData, *((st_float32*)pValue + 4)); }
template <> st_bool ParseData<Vec3>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, ((Vec3*)pValue)->x) && ParseFloat(pData, ((Vec3*)pValue)->y) && ParseFloat(pData, ((Vec3*)pValue)->z); }
template <> st_bool ParseData<Vec4>(st_byte*& pData, st_byte* pValue) { return ParseFloat(pData, ((Vec4*)pValue)->x) && ParseFloat(pData, ((Vec4*)pValue)->y) && ParseFloat(pData, ((Vec4*)pValue)->z) && ParseFloat(pData, ((Vec4*)pValue)->w); }
template <> st_bool ParseData<st_int32>(st_byte*& pData, st_byte* pValue) { return ParseInt(pData, *(st_int32*)pValue); }
template <> st_bool ParseData<st_int32[2]>(st_byte*& pData, st_byte* pValue) { return ParseInt(pData, *(st_int32*)pValue) && ParseInt(pData, *((st_int32*)pValue + 1)); }
template <> st_bool ParseData<st_int32[4]>(st_byte*& pData, st_byte* pValue) { return ParseInt(pData, *(st_int32*)pValue) && ParseInt(pData, *((st_int32*)pValue + 1)) && ParseInt(pData, *((st_int32*)pValue + 2)) && ParseInt(pData, *((st_int32*)pValue + 3)); }
template <> st_bool ParseData<CFixedString>(st_byte*& pData, st_byte* pValue) { GetToken(pData, *(CFixedString*)pValue); return !((CFixedString*)pValue)->empty( ); }
template <> st_bool ParseData<EChannels>(st_byte*& pData, st_byte* pValue) { return ParseChannels(pData, *(EChannels*)pValue); }

