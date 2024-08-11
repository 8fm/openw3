///////////////////////////////////////////////////////////////////////  
//  OrbisRenderer.h
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
//  *** Release version 7.0.0 ***


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#include "Core/Matrix.h"
#include "Core/FixedArray.h"
#include "RenderInterface/ForestRI.h"
#include "RenderInterface/TerrainRI.h"
#include "RenderInterface/SkyRI.h"
#include "Utilities/Utility.h"
#include <gnm.h>
#include <gnf.h>
#include <gnmx.h>
#include <gnmx/shader_parser.h>
#include <gnmx/fetchshaderhelper.h>


///////////////////////////////////////////////////////////////////////  
//  Renderer defines

// Replace all texture extension with gtf. This allows the same SRT assets to be used on
// Orbis as on other platforms when the Orbis native format is GNF.
#define CONVERT_TEXTURE_EXTENSIONS_TO_GNF


///////////////////////////////////////////////////////////////////////  
//  All SpeedTree SDK classes and variables are under the namespace "SpeedTree"

namespace SpeedTree
{
	///////////////////////////////////////////////////////////////////////  
	//  Class PSP2

	class ST_DLL_LINK Orbis
	{
	public:

	// current rendering context
	static	void							ST_CALL_CONV	SetContext(sce::Gnmx::GfxContext* pContext)	{ m_pContext = pContext; }
	static	sce::Gnmx::GfxContext*			ST_CALL_CONV	Context(void)								{ return m_pContext; }

	static void								ST_CALL_CONV	SetMainRenderTargets(sce::Gnm::RenderTarget* pRenderTarget, sce::Gnm::DepthRenderTarget* pDepthRenderTarget);
	static void								ST_CALL_CONV	BindMainRenderTargets(void);

	static void								ST_CALL_CONV	Initialize(void);
	static void								ST_CALL_CONV	Finalize(void);
	static off_t							ST_CALL_CONV	RoundUpToAlignment(off_t uiSize, off_t uiAlignment);
	static void*							ST_CALL_CONV	Allocate(unsigned int uiSize, unsigned int uiAlignment, bool bGPU);
	static void								ST_CALL_CONV	Release(void* pLocation);

	private:
	static	sce::Gnmx::GfxContext*			m_pContext;
	static	sce::Gnm::RenderTarget*			m_pRenderTarget;
	static	sce::Gnm::DepthRenderTarget*	m_pDepthRenderTarget;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CStateBlockOrbis

	class ST_DLL_LINK CStateBlockOrbis
	{
	public:
										CStateBlockOrbis( );

			st_bool						Init(const SAppState& sAppState, const SRenderState& sRenderState);
			st_bool						Bind(void) const;
			void						ReleaseGfxResources(void);

	private:
			SAppState					m_sAppState;
			SRenderState				m_sRenderState;
	};
	typedef CStateBlockRI<CStateBlockOrbis> CStateBlock;


	///////////////////////////////////////////////////////////////////////  
	//  Class CTextureOrbis

	class ST_DLL_LINK CTextureOrbis
	{
	public:
													CTextureOrbis( );
	virtual											~CTextureOrbis( );

			// loading
			st_bool									Load(const char* pFilename, st_int32 nMaxAnisotropy = 0);
			st_bool									LoadColor(st_uint32 uiColor); // 0xAABBGGRR
			st_bool									LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise);
			st_bool									LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
			st_bool									ReleaseGfxResources(void);
			st_bool									IsValid(void) const;
			const STextureInfo&						GetInfo(void) const;

			// render
			const sce::Gnm::Texture*				GetTextureObject(void) const;
	static	void					ST_CALL_CONV	SetSamplerStates(void);

	static	void					ST_CALL_CONV	SetSamplers(st_int32 nMaxAnisotropy) { }

			// other
			st_bool									operator!=(const CTextureOrbis& cRight) const;

	private:
			sce::Gnm::Texture						m_cTexture;
			STextureInfo							m_sInfo;
	};
	typedef CTextureRI<CTextureOrbis> CTexture;


	///////////////////////////////////////////////////////////////////////  
    //  Class CRenderTargetOrbis

    class ST_DLL_LINK CRenderTargetOrbis
    {
    public:
                                        CRenderTargetOrbis( );
                                        ~CRenderTargetOrbis( );

			st_bool					    InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
			void					    ReleaseGfxResources(void);

			void						Clear(const Vec4& vColor);

            st_bool                     SetAsTarget(void);                     
            void                        ReleaseAsTarget(void);                        
            st_bool                     BindAsTexture(st_int32 nRegister, st_bool bPointFilter) const;
			void						UnBindAsTexture(st_int32 nRegister) const;

			void					    OnResetDevice(void);
			void					    OnLostDevice(void);

	static	st_bool						SetGroupAsTarget(const CRenderTargetOrbis** pTargets, st_int32 nNumTargets, st_bool bClear = true);
	static	void						ReleaseGroupAsTarget(const CRenderTargetOrbis** pTargets, st_int32 nNumTargets);

	private:
			ERenderTargetType			m_eType;
            st_int32					m_nWidth;
            st_int32					m_nHeight;

			sce::Gnm::RenderTarget		m_cRenderTarget;
			sce::Gnm::DepthRenderTarget	m_cDepthRenderTarget;
			sce::Gnm::Texture			m_cTexture;
			mutable volatile uint64_t*	m_pWaitLabel;
	};
	typedef CRenderTargetRI<CRenderTargetOrbis> CRenderTarget;

	
	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantOrbis

	class ST_DLL_LINK CShaderConstantOrbis
	{
	public:
			friend class CShaderTechniqueOrbis;

	static	st_bool        ST_CALL_CONV	Init(void);
	static	void           ST_CALL_CONV	ReleaseGfxResources(void);
	static	void		   ST_CALL_CONV Reset(void);

	static	st_bool        ST_CALL_CONV	Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w);
	static	st_bool        ST_CALL_CONV	Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4]);
	static  st_bool        ST_CALL_CONV	Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues);
	static	st_bool        ST_CALL_CONV	SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);
	static	st_bool        ST_CALL_CONV	SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);

	static	st_bool        ST_CALL_CONV	SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately = true);
	static	void           ST_CALL_CONV SubmitSetTexturesInBatch(void) { }

	};
	typedef CShaderConstantRI<CShaderConstantOrbis, CTexture> CShaderConstant;


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantBufferOrbis

	class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantBufferOrbis
	{
	public:
										CShaderConstantBufferOrbis( );
										~CShaderConstantBufferOrbis( );

			st_bool        ST_CALL_CONV Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister);
			void           ST_CALL_CONV ReleaseGfxResources(void);

			st_bool        ST_CALL_CONV	Update(void) const;
			st_bool        ST_CALL_CONV	Bind(void) const;

	private:
			void*						m_pLayout;
			size_t						m_siSizeOfLayout;
			st_int32					m_nRegister;
			st_float32*					m_pData;
	};
	typedef CShaderConstantBufferRI<CShaderConstantBufferOrbis> CShaderConstantBuffer;


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderTechniqueOrbis

	class ST_DLL_LINK CShaderTechniqueOrbis
	{
	public:

			///////////////////////////////////////////////////////////////////////  
			//  Class CVertexShader

			class CVertexShader
			{
			public:
					friend class CShaderTechniqueOrbis;

													CVertexShader( );

					st_bool							Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
					st_bool							IsValid(void) const;
					void							ReleaseGfxResources(void);

			private:
					sce::Gnmx::VsShader*			m_pShader;
					uint32_t						m_uiShaderModifier;
					void*							m_pFetchShader;
			};

			///////////////////////////////////////////////////////////////////////  
			//  Class CPixelShader

			class CPixelShader
			{
			public:
					friend class CShaderTechniqueOrbis;

													CPixelShader( );

					st_bool							Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
					st_bool							IsValid(void) const;
					void							ReleaseGfxResources(void);

			private:
					sce::Gnmx::PsShader*			m_pShader;
			};

			st_bool									Link(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader);
			st_bool									Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const;
	static	void			   ST_CALL_CONV			UnBind(void);
			st_bool									ReleaseGfxResources(void);

			st_bool 								LoadProgramBinary(const char*, const char*) { return false; }
			st_bool 								SaveProgramBinary(const char*, const char*) { return false; }

	static	CFixedString   	   ST_CALL_CONV			GetCompiledShaderExtension(void);
	static	CFixedString   	   ST_CALL_CONV			GetCompiledShaderFolder(void);
	static	st_bool     	   ST_CALL_CONV			VertexDeclNeedsInstancingAttribs(void);
	};
	typedef CShaderTechniqueRI<CShaderTechniqueOrbis> CShaderTechnique;
	

	///////////////////////////////////////////////////////////////////////  
	//  Class CGeometryBufferOrbis

	class ST_DLL_LINK CGeometryBufferOrbis
	{
	public:
			friend class CInstancingMgrOrbis;

									CGeometryBufferOrbis( );
	virtual							~CGeometryBufferOrbis( );

			// vertex buffer
			st_bool					SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl);
			st_bool					CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize);
			st_bool					OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream = 0);
			st_bool					VertexBufferIsValid(void) const;

			st_bool					EnableFormat(void) const;
	static	st_bool    ST_CALL_CONV	DisableFormat(void);
			st_bool					BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const;
	static	st_bool    ST_CALL_CONV	UnBindVertexBuffer(st_uint32 uiStream);

			// mesh instancing support
	static	st_bool    ST_CALL_CONV InstancingRequiresSeparateVertexStream(void);
			st_bool					BindVertexBufferForInstancing(const CGeometryBufferOrbis* pInstanceBuffer) const;
			void					SetNeedsWaitBeforeUpdate(void) const;

			// index buffer
			st_bool					SetIndexFormat(EIndexFormat eFormat);
			st_bool					CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices);
			st_bool					OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset);
			st_bool					IndexBufferIsValid(void) const;
			st_uint32				IndexSize(void) const;
			st_bool					ClearIndices(void);

			st_bool					BindIndexBuffer(void) const;
	static	st_bool    ST_CALL_CONV	UnBindIndexBuffer(void);

			void					ReleaseGfxResources(void);

			// render functions
			st_bool					RenderIndexed(EPrimitiveType ePrimType, 
													st_uint32 uiStartIndex, 
													st_uint32 uiNumIndices, 
													st_uint32 uiMinIndex = 0,
													st_uint32 uiNumVertices = 0) const;
			st_bool					RenderIndexedInstanced(EPrimitiveType ePrimType,
													st_uint32 uiStartIndex,
													st_uint32 uiNumIndices,
													st_uint32 uiNumInstances,
													st_uint32 uiStartInstanceLocation = 0) const;
			st_bool					RenderArrays(EPrimitiveType ePrimType, 
													st_uint32 uiStartVertex, 
													st_uint32 uiNumVertices) const;
	
			void					BuildOrbisVertexBuffer(void);	
	static	st_bool					IsPrimitiveTypeSupported(EPrimitiveType ePrimType);
	static	sce::Gnm::PrimitiveType	FindOrbisPrimitiveType(EPrimitiveType ePrimType);


	private:
			struct ST_DLL_LINK SAttribParams
			{
												SAttribParams( );

				st_bool							IsActive(void) const;

				st_int32						m_nOffset;
				sce::Gnm::DataFormat			m_eDataType;
			};

			void*								m_pVertexBuffer;
			void*								m_pIndexBuffer;

			st_uint32							m_uiNumVertices;
			st_uint32							m_uiVertexSize;
			st_uint32							m_uiNumIndices;
			EIndexFormat						m_eIndexFormat;

			SAttribParams						m_asAttribParams[VERTEX_ATTRIB_COUNT];
			CFixedArray<sce::Gnm::Buffer, 16>	m_vBuffers;
	};
	typedef CGeometryBufferRI<CGeometryBufferOrbis, CShaderTechnique> CGeometryBuffer;


	///////////////////////////////////////////////////////////////////////  
	//  Class CInstancingMgrOrbis
	//
	//	One CInstancingMgrOrbis object is used per base tree.

	class ST_DLL_LINK CInstancingMgrOrbis
	{
	public:
											CInstancingMgrOrbis( );
	virtual									~CInstancingMgrOrbis( );

			// functions called by CInstancingMgrRI in the RenderInterface library
			st_bool							Init(SVertexDecl::EInstanceType eInstanceType, st_int32 nNumLods, const CGeometryBuffer* pGeometryBuffers, st_int32 nNumGeometryBuffers);
			void							ReleaseGfxResources(void);

			st_bool							Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances);
			st_bool							Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const;

			// queries
			st_int32						NumInstances(st_int32 nLod) const;

	private:
			struct SInstancesPerLod
			{
											SInstancesPerLod( );

				st_int32					m_nNumInstances;
				CGeometryBuffer				m_cInstanceBuffer;
			};
			CArray<SInstancesPerLod>		m_aInstances;		// one element per LOD level in the base tree

			const CGeometryBuffer*			m_pObjectBuffers;

			size_t							m_siSizeOfInstanceData;
	};
	typedef CInstancingMgrRI<CInstancingMgrOrbis, CGeometryBuffer> CInstancingMgr;


	// include inline functions
	#include "RenderInterface/TemplateTypedefs.h"
	#include "Renderers/Orbis/Texture_inl.h"
	#include "Renderers/Orbis/Shaders_inl.h"
	#include "Renderers/Orbis/GeometryBuffer_inl.h"
	#include "Renderers/Orbis/InstancingManager_inl.h"

} // end namespace SpeedTree



#include "Core/ExportEnd.h"
