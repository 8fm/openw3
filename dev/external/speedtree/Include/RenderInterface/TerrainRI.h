///////////////////////////////////////////////////////////////////////  
//  TerrainRI.h
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


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#include "Core/FileSystem.h"
#include "Forest/Terrain.h"
#include "RenderInterface/ForestRI.h"
#include "RenderInterface/ShaderConstantBuffers.h"


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
	//  Constants

	const st_int32 c_nNumTerrainSplatLayers = 3;


	///////////////////////////////////////////////////////////////////////  
	//  Structure STerrainRenderInfo

	struct ST_DLL_LINK STerrainRenderInfo
	{
								STerrainRenderInfo( );

			SAppState			m_sAppState;

			// shader
			CFixedString		m_strShaderPath;

			// splat parameters
			CFixedString		m_strNormalMap;
			CFixedString		m_strSplatMap;
			CFixedString		m_astrSplatLayers[c_nNumTerrainSplatLayers];
			st_float32			m_afSplatTileValues[c_nNumTerrainSplatLayers];
			st_float32			m_fNormalMapBlueScalar;

			// lighting
			st_bool				m_bShadowsEnabled;
			st_bool				m_bCastShadows;
			Vec3				m_vMaterialAmbient;
			Vec3				m_vMaterialDiffuse;


			// fog
			st_float32			m_fFogStartDistance;
			st_float32			m_fFogEndDistance;
			Vec3				m_vFogColor;

			// LOD
			st_int32			m_nNumLodLevels;
			st_int32			m_nMaxTerrainRes;
			st_float32			m_fCellSize;

			// misc
			st_bool				m_bDepthOnlyPrepass;
			st_int32			m_nMaxAnisotropy;
			st_bool				m_bTexturingEnabled;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CTerrainRI

	#define CTerrainRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CTerrainRI_TemplateList
	class ST_DLL_LINK CTerrainRI : public CTerrain
	{
	public:
											CTerrainRI( );
											~CTerrainRI( );

			// graphics initialization
			st_bool							InitGfx(st_int32 nNumLods, st_int32 nMaxTileRes, st_float32 fCellSize, const SVertexDecl& sVertexDecl);
			void							ReleaseGfxResources(void);

			void							SetRenderInfo(const STerrainRenderInfo& sInfo);
			const STerrainRenderInfo&		GetRenderInfo(void) const;
			void							SetMaxAnisotropy(st_int32 nMaxAnisotropy);

			// rendering functions
			st_bool							Render(ERenderPass ePass,
												   const STerrainCullResults& sCullResults, 
												   const SSimpleMaterial& sLighting,
												   CRenderStats& cStats);

			// culling & LOD
			void							CullAndComputeLOD(const CView& cView, st_int32 nFrameIndex, STerrainCullResults& sCullResults);

	private:
			// render support
			st_bool							StartRender(ERenderPass ePass, const SSimpleMaterial& sLighting);
			st_bool							EndRender(void) const;

			// VBO management
			st_bool							InitVboPool(void);
			TGeometryBufferClass*			NextVbo(void);
			TGeometryBufferClass*			NewVbo(void) const;

			st_bool							InitIndexBuffer(void);
			st_bool							InitRenderStates(void);

			// texture & materials
			CRenderStateRI_t				m_aRenderStates[RENDER_PASS_COUNT];
			st_int32						m_nMaxAnisotropy;
			TTextureClass					m_tGreyTexture;	// small fallback when texturing is disabled

			// terrain splat data
			STerrainRenderInfo				m_sRenderInfo;

			// render mechanics
			CArray<TGeometryBufferClass*>	m_aDynamicVboPool;
			CArray<TGeometryBufferClass*>	m_aOriginalVboPool;
			TGeometryBufferClass			m_tIndexBuffer;
			const SVertexDecl*				m_pVertexDecl;
			TShaderConstantBufferClass		m_cConstantBuffer;
			STerrainCBLayout				m_sConstantBufferLayout;
	};
	#define CTerrainRI_t CTerrainRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>

	// include inline functions
	#include "TerrainRI_inl.h"


} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
