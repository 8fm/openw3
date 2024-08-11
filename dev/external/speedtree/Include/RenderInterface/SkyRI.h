///////////////////////////////////////////////////////////////////////  
//  SkyRI.h
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
#include "Core/Random.h"
#include "RenderInterface/ForestRI.h"


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
	//  Structure SSkyRenderInfo

	struct ST_DLL_LINK SSkyRenderInfo
	{
							SSkyRenderInfo( );

			// app state
			SAppState		m_sAppState;

			// shader file location
			CFixedString	m_strShaderPath;

			// sun
			Vec3			m_vSunColor;
			st_float32		m_fSunSize;
			st_float32		m_fSunSpreadExponent;

			// sky/fog
			Vec3			m_vSkyColor;
			Vec3			m_vFogColor;
			st_float32		m_fFogStartDistance;
			st_float32		m_fFogEndDistance;
			st_float32		m_fFogDensity;
			st_float32		m_fSkyFogMin;
			st_float32		m_fSkyFogMax;
			st_float32		m_fNearClip;
			st_float32		m_fFarClip;
			CFixedString	m_strCloudTextureFilename;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CSkyRI

	#define CSkyRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CSkyRI_TemplateList
	class ST_DLL_LINK CSkyRI
	{
	public:
											CSkyRI( );
											~CSkyRI( );

			// graphics initialization
			st_bool							Init(void);
			void							ReleaseGfxResources(void);

			// config/info
			void							SetRenderInfo(const SSkyRenderInfo& sInfo);
			const SSkyRenderInfo&			GetRenderInfo(void) const;

			// rendering
			st_bool							Render(CRenderStats& cStats);

	private:
			// init support
			st_bool							InitRenderState(const SVertexDecl& sVertexDecl);
			st_bool							InitSkyGeometryBuffer(const SVertexDecl& sVertexDecl);

			// sky render parameters (from SFC file)
			SSkyRenderInfo					m_sRenderInfo;

			// render state & shaders
			CRenderStateRI_t				m_cSkyState;

			// geometry
			TGeometryBufferClass			m_tSkyGeometryBuffer;
	};
	#define CSkyRI_t CSkyRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>

	// include inline functions
	#include "SkyRI_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
