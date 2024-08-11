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

#pragma once
#include "Forest/Forest.h"


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{

	///////////////////////////////////////////////////////////////////////  
	//	Forward declaration
	
	struct SMyCmdLineOptions;


	///////////////////////////////////////////////////////////////////////  
	//	Channels to use when loading

	enum EChannels
	{
		CHANNEL_NONE		= 0,
		CHANNEL_RED			= (1 << 0),
		CHANNEL_GREEN		= (1 << 1),
		CHANNEL_BLUE		= (1 << 2),
		CHANNEL_ALPHA		= (1 << 3),
		CHANNEL_ALL			= (CHANNEL_RED | CHANNEL_GREEN | CHANNEL_BLUE | CHANNEL_ALPHA)
	};


	/////////////////////////////////////////////////////////////////////
	// class CMyConfigFile

	class CMyConfigFile
	{
	public:
												CMyConfigFile( );
												~CMyConfigFile( );

			st_bool								Load(const CFixedString& strFilename, SMyCmdLineOptions* pCmdLineOptions);
			CFixedString						AssetFullPath(const CFixedString& strPartialFilename) const;
			const CArray<CFixedString>&			GetErrors(void) const;

	private:
			struct SParseNode;

			st_bool								Preprocess(const CFixedString& strFilename, CString& strData);
			void								SetupParser(SParseNode* pRoot);
			void								Parse(st_byte*& pData, SParseNode* pRoot, st_byte* pDataOffset, const CFixedString& strHierarchy = "");

			void								PostProcess(SMyCmdLineOptions* pCmdLineOptions);

			CArray<CFixedString>				m_aErrors;
			CFixedString						m_strConfigFilePath;

	public:
			typedef CFixedString CFilename;
		
			// the data below is all what's parsed and is specified to the parser in SetupParser()

			// world/global parameters
			struct SWorld
			{
												SWorld( ) :
													m_fNearClip(0.5f),
													m_fFarClip(5000.0f),
													m_fFieldOfView(40.0f), // in degrees
													m_fGlobalTreeLodScalar(1.0f),
													m_f3dTreeCellSize(1200.0f),
                                            	    m_fGrassDensityScalar(1.0f),
													m_fRandomPopulationScalar(1.0f),
													m_fAlphaTestScalar3d(0.1f),
													m_fAlphaTestScalarGrass(0.1f),
													m_fAlphaTestScalarBillboards(0.1f),
													m_fTreeSurfaceAdhesion(0.2f),
													m_fGrassSurfaceAdhesion(1.0f)
												{
													m_afTwoPassBlendScalars[0] = 0.5f;
													m_afTwoPassBlendScalars[1] = 2.0f;
												}

				st_float32						m_fNearClip;
				st_float32						m_fFarClip;
				st_float32						m_fFieldOfView; // in degrees
				st_float32						m_fGlobalTreeLodScalar;
				st_float32						m_f3dTreeCellSize;
                st_float32                  	m_fGrassDensityScalar;
				st_float32						m_fRandomPopulationScalar;
				st_float32						m_fAlphaTestScalar3d;
				st_float32						m_fAlphaTestScalarGrass;
				st_float32						m_fAlphaTestScalarBillboards;
				st_float32						m_afTwoPassBlendScalars[2];
				st_float32						m_fTreeSurfaceAdhesion;
				st_float32						m_fGrassSurfaceAdhesion;
			};
			SWorld								m_sWorld;

			// forward render
			struct SForwardRender
			{
												SForwardRender( ) :
													m_bDepthOnlyPrepass(false)
												{
												}

				st_bool							m_bDepthOnlyPrepass;
				CFilename						m_strPostEffectsShaderPath;
			};
			SForwardRender						m_sForwardRender;

			// deferred render
			struct SDeferredRender
			{
												SDeferredRender( ) :
													m_bEnabled(false),
													m_bDistanceBlur(false)
												{
												}

				st_bool							m_bEnabled;
				st_bool							m_bDistanceBlur;
				CFilename						m_strPostEffectsShaderPath;
			};
			SDeferredRender						m_sDeferredRender;

			// navigation
			struct SNavigation
			{
												SNavigation( ) :
													m_fFollowHeight(5.5f),
													m_fSpeedScalar(1.0f)
												{
													m_afStartingAzimuthAndPitch[0] = 0.0f;
													m_afStartingAzimuthAndPitch[1] = 0.0f;
												}

				CFilename						m_strCameraFilename;
				st_float32						m_fFollowHeight;
				st_float32						m_fSpeedScalar;

				st_float32						m_afStartingPosition[2];
				st_float32						m_afStartingAzimuthAndPitch[2];
			};
			SNavigation							m_sNavigation;

			// wind
			struct SWind
			{
												SWind( ) :
													m_bEnabled(false),
													m_bGusting(true),
													m_fStrength(0.25f),
													m_vDirection(1.0f, 0.0f, 0.0f)
												{
												}

				st_bool							m_bEnabled;
				st_bool							m_bGusting;
				st_float32						m_fStrength;
				Vec3							m_vDirection;
			};
			SWind								m_sWind;

			// fog
			struct SFog
			{
												SFog( ) :
													m_fDensity(0.001f),
													m_vColor(1.0f, 1.0f, 1.0f)
												{
													m_afLinear[0] = 500.0f;
													m_afLinear[1] = 5000.0f;
												}

				st_float32						m_afLinear[2];	// [0] = start distance, [1] = end distance
				st_float32						m_fDensity;
				Vec3							m_vColor;
			};
			SFog								m_sFog;

			// lighting
			struct SLighting
			{
												SLighting( ) :
													m_vAmbient(1.0f, 1.0f, 1.0f),
													m_vDiffuse(1.0f, 1.0f, 1.0f),
													m_vSpecular(1.0f, 1.0f, 1.0f),
													m_vTransmission(1.0f, 1.0f, 1.0f),
													m_vDirection(0.707f, 0.0f, -0.707f)
												{
												}
			
				Vec3							m_vAmbient;
				Vec3							m_vDiffuse;
				Vec3							m_vSpecular;
				Vec3							m_vTransmission;
				Vec3							m_vDirection;

				struct STwilight
				{
												STwilight( ) :
													m_vHorizonColor(1.0f, 1.0f, 1.0f),
													m_vAmbient(1.0f, 1.0f, 1.0f),
													m_vDiffuse(1.0f, 1.0f, 1.0f),
													m_vSpecular(1.0f, 1.0f, 1.0f),
													m_vTransmission(1.0f, 1.0f, 1.0f)
												{
												}
					
					Vec3						m_vHorizonColor;
					Vec3						m_vAmbient;
					Vec3						m_vDiffuse;
					Vec3						m_vSpecular;
					Vec3						m_vTransmission;
				};
				STwilight						m_sTwilight;

				struct SImageBasedAmbientLighting
				{
					CFilename					m_strFilename;
				};
				SImageBasedAmbientLighting		m_sImageBasedAmbientLighting;

				struct SBloom
				{
												SBloom( ) :
													m_bEnabled(false),
													m_nDownsample(6),
													m_fBrightPass(0.25f),
													m_fBrightFloor(0.3f),
													m_nBlur(15),
													m_fSkyBleed(0.1675f),
													m_fBloomEffectScalar(1.3f),
													m_fMainEffectScalar(1.0f)
												{
												}

					st_bool						m_bEnabled;
					st_int32					m_nDownsample;
					st_float32					m_fBrightPass;
					st_float32					m_fBrightFloor;
					st_int32					m_nBlur;
					st_float32					m_fSkyBleed;
					st_float32					m_fBloomEffectScalar;
					st_float32					m_fMainEffectScalar;
				};
				SBloom							m_sBloom;
			};
			SLighting							m_sLighting;

			// shadows
			struct SShadows
			{
												SShadows( ) :
													m_bEnabled(false),
													m_nResolution(0),
													m_nNumMaps(2),
													m_fFadePercent(0.25f),
													m_fBehindCameraDistance(1000.0f)
												{
													m_afMapRanges[0] = 250.0f;
													m_afMapRanges[1] = 500.0f;
													m_afMapRanges[2] = 750.0f;
													m_afMapRanges[3] = 1000.0f;
													m_anMapUpdateIntervals[0] = 1;
													m_anMapUpdateIntervals[1] = 2;
													m_anMapUpdateIntervals[2] = 3;
													m_anMapUpdateIntervals[3] = 4;
												}

				st_bool							m_bEnabled;
				st_int32						m_nResolution;
				st_int32						m_nNumMaps;
				st_float32						m_afMapRanges[c_nMaxNumShadowMaps];
				st_int32						m_anMapUpdateIntervals[c_nMaxNumShadowMaps];
				st_float32						m_fFadePercent;
				st_float32						m_fBehindCameraDistance;
			};
			SShadows							m_sShadows;

			// terrain
			struct STerrain
			{
												STerrain( ) :
													m_nMaxResolution(33),
													m_nNumLodLevels(5),
													m_fCellSize(1200.0f),
													m_nTiles(0),
													m_vAmbient(1.0f, 1.0f, 1.0f),
													m_vDiffuse(1.0f, 1.0f, 1.0f),
													m_fAmbientOcclusionBrightness(0.5f),
													m_fAmbientOcclusionDistance(25.0f),
													m_bCastShadows(false),
													m_vHeightMapSize(1000.0f, 1000.0f, 100.0f)
												{
												}

				// general
				CFilename						m_strShaderPath;
				st_int32						m_nMaxResolution;
				st_int32						m_nNumLodLevels;
				st_float32						m_fCellSize;
				st_int32						m_nTiles;

				// lighting
				CFilename						m_strNormalMapFilename;
				Vec3							m_vAmbient;
				Vec3							m_vDiffuse;
				st_float32						m_fAmbientOcclusionBrightness;
				st_float32						m_fAmbientOcclusionDistance;
				st_bool							m_bCastShadows;

				// height map
				CFilename						m_strHeightMapFilename;
				Vec3							m_vHeightMapSize;

				// splat map
				CFilename						m_strSplatMapFilename;
				struct SSplatLayer
				{
												SSplatLayer( ) :
													m_fRepeat(1.0f),
													m_vApproxColor(1.0f, 1.0f, 1.0f)
												{
												}

					CFilename					m_strFilename;
					st_float32					m_fRepeat;
					Vec3						m_vApproxColor;
				};
				SSplatLayer						m_asSplatLayers[3];
			};
			STerrain							m_sTerrain;

			// sky
			struct SSky
			{
												SSky( ) :
													m_vColor(0.38f, 0.52f, 0.75f),
													m_fSunSize(0.008f),
													m_fSunSpreadExponent(100.0f),
													m_vSunColor(1.0f, 1.0f, 1.0f)
												{
													m_afFogRange[0] = -0.5f;
													m_afFogRange[1] = 1.0f;
												}

				CFilename						m_strShaderPath;
				Vec3							m_vColor;
				st_float32						m_afFogRange[2];

				CFilename						m_strTexture;

				// sun
				st_float32						m_fSunSize;
				st_float32						m_fSunSpreadExponent;
				Vec3							m_vSunColor;

				struct STwilight
				{
												STwilight( ) :
													m_fSunSize(0.008f),
													m_fSunSpreadExponent(100.0f),
													m_vSunColor(1.0f, 1.0f, 1.0f),
													m_vColor(0.38f, 0.52f, 0.75f)
												{
												}
					
					st_float32					m_fSunSize;
					st_float32					m_fSunSpreadExponent;
					Vec3						m_vSunColor;
					Vec3						m_vColor;
				};

				STwilight						m_sTwilight;
			};
			SSky								m_sSky;

			// demo settings (never needed for games)
			struct SDemo
			{									SDemo( ) :
													m_fSunSpeed(0.3f),
													m_bKeyboardLock(false)
												{
													m_afCameraTime[0] = 5.0f;
													m_afCameraTime[1] = 10.0f;
													m_afCameraSpeed[0] = 1.0f;
													m_afCameraSpeed[1] = 7.0f;
												}

				st_float32						m_afCameraTime[2];
				st_float32						m_afCameraSpeed[2];
				st_float32						m_fSunSpeed;
				st_bool							m_bKeyboardLock;
			};
			SDemo								m_sDemo;

			// overlays
			struct SOverlay
			{
													SOverlay( ) :
														m_fSize(0.25f),
														m_nAnchor(5)
													{
														m_afPos[0] = m_afPos[1] = 0.5f;
													}

				CFilename							m_strFilename;
				st_float32							m_afPos[2];
				st_float32							m_fSize;
				st_int32							m_nAnchor; // 0-8
			};
			CFilename								m_strOverlayShaderPath;
			CArray<SOverlay>						m_aOverlays;


			// random trees
			struct SRandomTreePopulationParams
			{
													SRandomTreePopulationParams( ) :
														m_nQuantity(0),
														m_nSeed(rand( )),
														m_fLodScalar(1.0f),
														m_bFollowTerrain(true),
														m_vArea(-10000.0f, 10000.0f, -10000.0f, 10000.0f),
														m_fAmbientImageScalar(1.0f)
													{
														m_afScalarRange[0] = 1.0f;
														m_afScalarRange[1] = 1.0f;
														m_afElevationRange[0] = -FLT_MAX;
														m_afElevationRange[1] = FLT_MAX;
														m_afSlopeRange[0] = 0.0f;
														m_afSlopeRange[1] = 1.0f;
													}
			
				CFilename							m_strSrtFilename;
				st_int32							m_nQuantity;
				st_int32							m_nSeed;
				st_float32							m_fLodScalar;

				st_bool								m_bFollowTerrain;
				Vec4								m_vArea;
				st_float32							m_afScalarRange[2];
				st_float32							m_afElevationRange[2];
				st_float32							m_afSlopeRange[2];

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;
			};
			CArray<SRandomTreePopulationParams>		m_aRandomTreePopulations;


			// manual trees
			struct SManualTreePopulationParams
			{
													SManualTreePopulationParams( ) :
														m_fLodScalar(1.0f),
														m_bFollowTerrain(true),
														m_fAmbientImageScalar(1.0f)
													{
														m_aInstances.SetHeapDescription("CMyConfigFile::CArray");
													}

				CFilename							m_strSrtFilename;
				st_float32							m_fLodScalar;
				st_bool								m_bFollowTerrain;

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;

				struct SInstance
				{
													SInstance( ) :
														m_fScalar(1.0f),
														m_vUp(0.0f, 0.0f, 1.0f),
														m_vRight(1.0f, 0.0f, 0.0f)
													{
													}

					Vec3							m_vPos;
					st_float32						m_fScalar;
					Vec3							m_vUp;
					Vec3							m_vRight;
				};
				CArray<SInstance>					m_aInstances;
			};
			CArray<SManualTreePopulationParams>		m_aManualTreePopulations;


			// stf files
			struct SStfTreePopulationParams
			{
													SStfTreePopulationParams( ) :
														m_bFollowTerrain(true),
														m_fAmbientImageScalar(1.0f)
													{
													}

				CFilename							m_strFilename;
				st_bool								m_bFollowTerrain;

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;
			};
			CArray<SStfTreePopulationParams>		m_aStfTreePopulations;


			// swa files
			struct SSwaTreePopulationParams
			{
													SSwaTreePopulationParams( ) :
														m_bFollowTerrain(true),
														m_fAmbientImageScalar(1.0f)
													{
													}

				CFilename							m_strFilename;
				st_bool								m_bFollowTerrain;

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;
			};
			CArray<SSwaTreePopulationParams>		m_aSwaTreePopulations;


			// speedforest files
			struct SSpeedForestPopulationParams
			{
													SSpeedForestPopulationParams( ) :
														m_fLodScalar(1.0f),
														m_bFollowTerrain(true),
														m_fAmbientImageScalar(1.0f)
													{
													}

				CFilename							m_strSpeedForestFilename;
				CFilename							m_strSrtFilename;
				st_float32							m_fLodScalar;
				st_bool								m_bFollowTerrain;

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;
			};
			CArray<SSpeedForestPopulationParams>	m_aSpeedForestPopulations;


			// grass
			struct SGrassPopulationParams
			{
													SGrassPopulationParams( ) :
														m_fDensity(0.05f),
														m_fSurfaceAdhesion(-1.0f), // negative means to use the global grass surface adhesion value
														m_bFollowTerrain(true),
														m_vArea(0.0f, 0.0f, 0.0f, 0.0f),
														m_fCellSize(50.0f),
														m_bCastShadows(false),
														m_eMaskChannel(CHANNEL_ALPHA),
														m_fAmbientImageScalar(1.0f)
													{
														m_afScalarRange[0] = m_afScalarRange[1] = 1.0f;
														m_afElevationRange[0] = -FLT_MAX;
														m_afElevationRange[1] = FLT_MAX;
														m_afSlopeRange[0] = 0.0f;
														m_afSlopeRange[1] = 1.0f;
														m_afLodRange[0] = m_afLodRange[1] = -1.0f;
													}

				CFilename							m_strSrtFilename;
				st_float32							m_fDensity;
				st_float32							m_fSurfaceAdhesion;
				st_bool								m_bFollowTerrain; // currently ignored
				st_float32							m_afScalarRange[2];
				st_float32							m_afElevationRange[2];
				st_float32							m_afSlopeRange[2];
				st_float32							m_afLodRange[2];
				Vec4								m_vArea;
				st_float32							m_fCellSize;
				st_bool								m_bCastShadows;

				CFilename							m_strMaskFilename;
				EChannels							m_eMaskChannel;

				CCore::SHueVariationParams			m_sHueVariationParams;
				st_float32							m_fAmbientImageScalar;
			};
			CArray<SGrassPopulationParams>	m_aGrassPopulations;
	};

	typedef CMyConfigFile CMyConfigFile;

} // end namespace SpeedTree


