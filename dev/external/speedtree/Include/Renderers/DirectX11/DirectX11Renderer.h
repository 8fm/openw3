///////////////////////////////////////////////////////////////////////  
//  DirectX11Renderer.h
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
#define VC_EXTRALEAN
#include <d3d11.h>
#include <d3dx11.h>
#include <d3dx10math.h>
#include <dxerr.h>
#include "RenderInterface/ForestRI.h"
#include "RenderInterface/TerrainRI.h"
#include "RenderInterface/SkyRI.h"


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
	//  Platform-specific macros

	#ifndef ST_SAFE_RELEASE
		#define ST_SAFE_RELEASE(p) { if ((p)) { (p)->Release( ); (p) = NULL; } }
	#endif

	#ifndef NDEBUG
		#ifndef  ST_NAME_DX11_OBJECT
			#define ST_NAME_DX11_OBJECT(object, name) object->SetPrivateData(WKPDID_D3DDebugObjectName, sizeof(name) - 1, (name))
		#endif
	#else
		#define ST_NAME_DX11_OBJECT(object, name)
		#pragma warning (disable : 4390) // silence warnings about ST_NAME_DX11_OBJECT being empty
	#endif


	///////////////////////////////////////////////////////////////////////  
	//  Class DX11

	class ST_DLL_LINK_STATIC_MEMVAR DX11
	{
	public:
	static	ID3D11Device*			   ST_CALL_CONV Device(void) { return m_pDx11; }
	static	void					   ST_CALL_CONV	SetDevice(ID3D11Device* pDevice);
	static	ID3D11DeviceContext*	   ST_CALL_CONV DeviceContext(void) { return m_pDx11Context; }
	static	void					   ST_CALL_CONV	SetDeviceContext(ID3D11DeviceContext* pDeviceContext);

	private:
	static	ID3D11Device*							m_pDx11;
	static	ID3D11DeviceContext*					m_pDx11Context;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CStateBlockDirectX11

	class ST_DLL_LINK CStateBlockDirectX11
	{
	public:
										CStateBlockDirectX11( );

			st_bool						Init(const SAppState& sAppState, const SRenderState& sRenderState);
			st_bool						Bind(void) const;
			void						ReleaseGfxResources(void);

	private:
			ID3D11DepthStencilState*	m_pDepth;
			ID3D11RasterizerState*		m_pRasterizer;
			ID3D11BlendState*			m_pBlend;
	};
	typedef CStateBlockRI<CStateBlockDirectX11> CStateBlock;


	///////////////////////////////////////////////////////////////////////  
	//  Class CTextureDirectX11

	class ST_DLL_LINK_STATIC_MEMVAR CTextureDirectX11
	{
	public:
										CTextureDirectX11( );
	virtual								~CTextureDirectX11( );

			// loading
			st_bool						Load(const char* pFilename, st_int32 nMaxAnisotropy = 0);
			st_bool						LoadColor(st_uint32 ulColor);
			st_bool						LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise);
			st_bool						LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
			st_bool						ReleaseGfxResources(void);
			st_bool						IsValid(void) const;

			// render
			ID3D11ShaderResourceView*	GetTextureObject(void) const;
	static	void           ST_CALL_CONV SetSamplerStates(void);

			// other
			const STextureInfo&			GetInfo(void) const;
			st_bool						IsGeneratedUniformColor(void) const;
			CTextureDirectX11&			operator=(const CTextureDirectX11& cRight);
			st_bool						operator!=(const CTextureDirectX11& cRight) const;

	private:
	static	void	       ST_CALL_CONV CreateSharedSamplers(st_int32 nMaxAnisotropy);

			ID3D11ShaderResourceView*	m_pTexture;
			STextureInfo				m_sInfo;
			st_bool						m_bIsGeneratedUniformColor;

	static	ID3D11SamplerState*			m_pStandardSampler;			// bark, leaves, terrain, etc.
	static	ID3D11SamplerState*			m_pShadowMapCompareSampler;	// shadow map comparison 
	static	ID3D11SamplerState*			m_pPointSampler;			// unfiltered, used for render target lookups
	static	ID3D11SamplerState*			m_pLinearClampSampler;		// bloom/blur effects
	static	st_int32					m_nSharedSamplerRefCount;
	static	st_int32					m_nMaxAnisotropy;
	};
	typedef CTextureRI<CTextureDirectX11> CTexture;


	///////////////////////////////////////////////////////////////////////  
    //  Class CRenderTargetDirectX11

    class ST_DLL_LINK_STATIC_MEMVAR CRenderTargetDirectX11
    {
    public:
                                        CRenderTargetDirectX11( );
                                        ~CRenderTargetDirectX11( );

			st_bool					    InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
			void					    ReleaseGfxResources(void);

			void						Clear(const Vec4& vColor);

            st_bool                     SetAsTarget(void);                     
            void                        ReleaseAsTarget(void);                        
			st_bool                     BindAsTexture(st_int32 nRegisterIndex, st_bool bPointFilter) const;
			void						UnBindAsTexture(st_int32 nRegisterIndex) const;
			ID3D11Texture2D*			GetDx11Texture(void);

			void					    OnResetDevice(void);
			void					    OnLostDevice(void);

	static	st_bool						SetGroupAsTarget(const CRenderTargetDirectX11** pTargets, st_int32 nNumTargets, st_bool bClear = true);
	static	void						ReleaseGroupAsTarget(const CRenderTargetDirectX11** pTargets, st_int32 nNumTargets);
	static	st_bool						ResolveSubresource(CRenderTargetRI<CRenderTargetDirectX11>* pNonMsaaDest, CRenderTargetRI<CRenderTargetDirectX11>* pMsaaSrc);

	private:
            st_bool                    	CreateColorTarget(void);
            st_bool                    	CreateDepthTarget(void);

			ERenderTargetType			m_eType;
            st_int32					m_nWidth;
            st_int32					m_nHeight;
			st_int32					m_nNumSamples;

			ID3D11Texture2D*           	m_pTexture;
			ID3D11ShaderResourceView*  	m_pShaderResourceView;
			ID3D11RenderTargetView*    	m_pRenderTargetView;
			ID3D11DepthStencilView*    	m_pDepthStencilView;

			// used to save and restore the current view and viewport
	static	ID3D11RenderTargetView*		m_apPreviouslyActiveRenderTargetViews[c_nMaxNumRenderTargets];
	static	ID3D11DepthStencilView*		m_pPreviouslyActiveDepthStencilView;
	static	D3D11_VIEWPORT				m_sPreviouslyActiveViewport;
	};
	typedef CRenderTargetRI<CRenderTargetDirectX11> CRenderTarget;


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantDirectX11

	// todo: how much of this class can we delete?
	class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantDirectX11
	{
	public:
	static	st_bool        ST_CALL_CONV Init(void);
	static	void           ST_CALL_CONV ReleaseGfxResources(void);
	static	void           ST_CALL_CONV Reset(void);

	static	st_bool        ST_CALL_CONV Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w);
	static	st_bool        ST_CALL_CONV Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4]);
	static  st_bool        ST_CALL_CONV Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues);
	static	st_bool        ST_CALL_CONV SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);
	static	st_bool        ST_CALL_CONV SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);

	static	st_bool        ST_CALL_CONV SetTexture(st_int32 nRegister, const CTexture& cTexture, st_bool bSubmitImmediately = true);
	static	void           ST_CALL_CONV SubmitSetTexturesInBatch(void);

	static	st_bool        ST_CALL_CONV CommitConstants(void);

	private:
	static	void           ST_CALL_CONV	FlagConstantBufferAsUpdated(const SStaticShaderConstant& sRegister);

	static	ID3D11Buffer*				m_apConstantBuffers[SHADER_CONSTANT_GROUP_COUNT];
	static	st_bool						m_abBufferUpdated[SHADER_CONSTANT_GROUP_COUNT];
	static	st_int32					m_anMapRegisterToGroup[c_nNumMainShaderUniformSlots];
	static	st_float32					m_afConstantsMirror[c_nNumMainShaderUniformSlots * 4];
	static	ID3D11ShaderResourceView*	c_apCachedTextureSets[TEXTURE_REGISTER_COUNT];
	static	st_int32					m_anTextureRegisterRange[2]; // [0] = min, [1] = max
	};
	typedef CShaderConstantRI<CShaderConstantDirectX11, CTexture> CShaderConstant;


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantBufferDirectX11

	class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantBufferDirectX11
	{
	public:
										CShaderConstantBufferDirectX11( );
										~CShaderConstantBufferDirectX11( );

			st_bool        ST_CALL_CONV Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister);
			void           ST_CALL_CONV ReleaseGfxResources(void);

			st_bool        ST_CALL_CONV	Update(void) const;
			st_bool        ST_CALL_CONV	Bind(void) const;

	private:
			ID3D11Buffer*				m_pConstantBuffer;
			void*						m_pLayout;
			size_t						m_siSizeOfLayout;
			st_int32					m_nRegister;
	};
	// todo: delete
	//#define CShaderConstantBuffer(TLayout) CShaderConstantBufferRI<CShaderConstantBufferDirectX11, TLayout>
	typedef CShaderConstantBufferRI<CShaderConstantBufferDirectX11> CShaderConstantBuffer;

	
	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderTechniqueDirectX11

	class ST_DLL_LINK CShaderTechniqueDirectX11
	{
	public:
			///////////////////////////////////////////////////////////////////////  
			//  Class CVertexShader

			class ST_DLL_LINK CVertexShader
			{
			public:
											CVertexShader( );

					st_bool					Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
					st_bool					IsValid(void) const;
					void					ReleaseGfxResources(void);

					ID3D11VertexShader*		m_pShader;
					DWORD*					m_pCompiledShaderCode;
					st_uint32				m_uiCompiledShaderCodeSize;
			};


			///////////////////////////////////////////////////////////////////////  
			//  Class CPixelShader

			class ST_DLL_LINK CPixelShader
			{
			public:
											CPixelShader( );

					st_bool					Load(const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState);
					st_bool					IsValid(void) const;
					void					ReleaseGfxResources(void);

					ID3D11PixelShader*		m_pShader;
					DWORD*					m_pCompiledShaderCode;
					st_uint32				m_uiCompiledShaderCodeSize;
			};

			st_bool							Link(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader);
			st_bool							Bind(const CVertexShader& cVertexShader, const CPixelShader& cPixelShader) const;
	static	void			   ST_CALL_CONV UnBind(void);
			st_bool							ReleaseGfxResources(void);

			st_bool 						LoadProgramBinary(const char*, const char*) { return false; }
			st_bool 						SaveProgramBinary(const char*, const char*) { return false; }

	static	CFixedString   	   ST_CALL_CONV	GetCompiledShaderExtension(void);
	static	CFixedString   	   ST_CALL_CONV	GetCompiledShaderFolder(void);
	static	st_bool     	   ST_CALL_CONV	VertexDeclNeedsInstancingAttribs(void);
	};
	typedef CShaderTechniqueRI<CShaderTechniqueDirectX11> CShaderTechnique;


	///////////////////////////////////////////////////////////////////////  
	//  Class CGeometryBufferDirectX11

	class ST_DLL_LINK CGeometryBufferDirectX11
	{
	public:
											CGeometryBufferDirectX11( );
	virtual									~CGeometryBufferDirectX11( );

			// vertex buffer
			st_bool							SetVertexDecl(const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl);
			st_bool							CreateVertexBuffer(st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize);
			st_bool							OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream);
			st_bool							VertexBufferIsValid(void) const;

			st_bool							EnableFormat(void) const;
	static	st_bool		       ST_CALL_CONV DisableFormat(void);
			st_bool							BindVertexBuffer(st_uint32 uiStream, st_uint32 uiVertexSize) const;
	static	st_bool		       ST_CALL_CONV UnBindVertexBuffer(st_uint32 uiStream);

			// mesh instancing support
	static	st_bool            ST_CALL_CONV InstancingRequiresSeparateVertexStream(void);
			ID3D11Buffer*					VertexBuffer(void) const;

			// index buffer
			st_bool							SetIndexFormat(EIndexFormat eFormat);
			st_bool							CreateIndexBuffer(st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices);
			st_bool							OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset);
			st_bool							IndexBufferIsValid(void) const;
			st_bool							ClearIndices(void);
			st_uint32						IndexSize(void) const;

			st_bool							BindIndexBuffer(void) const;
	static	st_bool			   ST_CALL_CONV UnBindIndexBuffer(void);

			void							ReleaseGfxResources(void);

			// render functions
			st_bool							RenderIndexed(EPrimitiveType ePrimType, 
														  st_uint32 uiStartIndex, 
														  st_uint32 uiNumIndices, 
														  st_uint32 uiMinIndex,
														  st_uint32 uiNumVertices) const;
			st_bool							RenderIndexedInstanced(EPrimitiveType ePrimType,
															       st_uint32 uiStartIndex,
																   st_uint32 uiNumIndices,
															       st_uint32 uiNumInstances,
															       st_uint32 uiStartInstanceLocation) const;
			st_bool							RenderArrays(EPrimitiveType ePrimType, 
														 st_uint32 uiStartVertex, 
														 st_uint32 uiNumVertices) const;
	static	st_bool			   ST_CALL_CONV IsPrimitiveTypeSupported(EPrimitiveType ePrimType);

	private:
			st_bool							IsFormatSet(void) const;

			ID3D11InputLayout*				m_pVertexLayout;
			ID3D11Buffer*					m_pVertexBuffer;
			ID3D11Buffer*					m_pIndexBuffer;
			EIndexFormat					m_eIndexFormat;
			st_uint32						m_uiCurrentVertexBufferSize;	// in bytes
			st_uint32						m_uiCurrentIndexBufferSize;		// in bytes
			st_bool							m_bDynamic;
	};
	typedef CGeometryBufferRI<CGeometryBufferDirectX11, CShaderTechnique> CGeometryBuffer;


	///////////////////////////////////////////////////////////////////////  
	//  Class CInstancingMgrDirectX11
	//
	//	One CInstancingMgrDirectX11 object is used per base tree.

	class ST_DLL_LINK CInstancingMgrDirectX11
	{
	public:
											CInstancingMgrDirectX11( );
											~CInstancingMgrDirectX11( );

			// functions called by CInstancingMgrRI in the RenderInterface library
			st_bool							Init(SVertexDecl::EInstanceType eInstanceType, 
												 st_int32 nNumLods, 
												 const CGeometryBuffer* pGeometryBuffers, 
												 st_int32 nNumGeometryBuffers);
			void							ReleaseGfxResources(void);

			st_bool							Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances);
			st_bool							Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats) const;

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
	typedef CInstancingMgrRI<CInstancingMgrDirectX11, CGeometryBuffer> CInstancingMgr;


	// include inline functions
	#include "RenderInterface/TemplateTypedefs.h"
	#include "Renderers/DirectX11/Texture_inl.h"
	#include "Renderers/DirectX11/Shaders_inl.h"
	#include "Renderers/DirectX11/GeometryBuffer_inl.h"
	#include "Renderers/DirectX11/InstancingManager_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif
