///////////////////////////////////////////////////////////////////////  
//  GraphicsApiAbstractionRI.h
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
#include "Core/FileSystem.h"
#include "Forest/Forest.h"
#include "RenderInterface/ShaderConstants.h"
#include "Utilities/Utility.h"


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

	const st_int32 c_nShaderCacheInitialReserve = 40;
	const st_int32 c_nTextureCacheInitialReserve = 50;
	const st_int32 c_nStateBlockCacheInitialReserve = 40;
    const st_int32 c_nNoiseTexWidth = 32;


	///////////////////////////////////////////////////////////////////////  
	//  Structure SSimpleMaterial
	//
	//	Mostly used to store the color values of our example/simple 
	//	directional light.

	struct ST_DLL_LINK SSimpleMaterial
	{
							SSimpleMaterial( );

			Vec3			m_vAmbient;
			Vec3			m_vDiffuse;
			Vec3			m_vSpecular;
			Vec3			m_vTransmission;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CResourceCache

	#define CResourceCache_TemplateList template<class TKey, class TResource>

	CResourceCache_TemplateList
	class ST_DLL_LINK CResourceCache
	{
	public:
							CResourceCache(EGfxResourceType eResourceType, st_int32 nInitialReserve = 10);

			TResource*		Retrieve(const TKey& tKey) const;
            st_bool			Add(const TKey& tKey, const TResource& tResource, size_t siSize); // returns true if unique
            st_bool			Add_NoCoreTracking(const TKey& tKey, const TResource& tResource); // returns true if unique
            st_int32		Release(const TKey& tKey); // returns reference count
            st_int32		Release_NoCoreTracking(const TKey& tKey); // returns reference count
			st_int32		Size(void) const;

	private:
			struct ST_DLL_LINK SCacheEntry 
			{
							SCacheEntry( ) :
								m_nRefCount(0)
							{
							}

				TResource	m_tResource;
				st_int32	m_nRefCount;
			};

            EGfxResourceType   m_eResourceType;

			typedef CMap<TKey, SCacheEntry> CCacheMap;
			CCacheMap		m_mCache;
	};
	#define CResourceCache_t CResourceCache<TKey, TResource>


	///////////////////////////////////////////////////////////////////////  
	//  Structure SAppState

	struct ST_DLL_LINK SAppState
	{
            enum EOverrideDepthTest
            {
                OVERRIDE_DEPTH_TEST_PASSTHROUGH,
                OVERRIDE_DEPTH_TEST_ENABLE,
                OVERRIDE_DEPTH_TEST_DISABLE
            };

							            SAppState( ) :
								            m_bMultisampling(false),
								            m_bAlphaToCoverage(false),
								            m_bDepthPrepass(false),
											m_bDeferred(false),
                                            m_eShadowConfig(SRenderState::SHADOW_CONFIG_OFF),
                                            m_eOverrideDepthTest(OVERRIDE_DEPTH_TEST_PASSTHROUGH)
							            {	
							            }

			st_bool			            m_bMultisampling;
			st_bool			            m_bAlphaToCoverage;
			st_bool			            m_bDepthPrepass;
			st_bool						m_bDeferred;
            SRenderState::EShadowConfig m_eShadowConfig;

            // override flags
            EOverrideDepthTest          m_eOverrideDepthTest;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CStateBlockRI

	enum EDepthTestFunc
	{
		DEPTHTEST_NEVER,
		DEPTHTEST_LESS,
		DEPTHTEST_EQUAL,
		DEPTHTEST_LESS_EQUAL,
		DEPTHTEST_GREATER,
		DEPTHTEST_NOT_EQUAL,
		DEPTHTEST_GREATER_EQUAL,
		DEPTHTEST_ALWAYS
	};

	#define CStateBlockRI_TemplateList template<class TStateBlockPolicy>

	CStateBlockRI_TemplateList
	class ST_DLL_LINK CStateBlockRI
	{
	public:
										CStateBlockRI( );

			st_bool						Init(const SAppState& sAppState, const SRenderState& sRenderState, st_bool isInteractive);		// LAVA edit: add isInteractive flag
			void						ReleaseGfxResources(void);
			st_bool						Bind(void) const;

			// queries
			const SAppState&			GetAppState(void) const;
			// LAVA ++
			TStateBlockPolicy*			GetStateBlockPolicy() { return &m_tStateBlockPolicy; }
			// LAVA --
	private:
			void						GetStateBlockKey(CFixedString& strKey, const SAppState& sAppState, const SRenderState& sRenderState) const;

			typedef CResourceCache<CFixedString, TStateBlockPolicy> CStateBlockCache;

			TStateBlockPolicy			m_tStateBlockPolicy;
			SAppState					m_sAppState;
			CFixedString				m_strHashKey;
	static	CStateBlockCache*			m_pCache;
	};
	#define CStateBlockRI_t CStateBlockRI<TStateBlockPolicy>


	///////////////////////////////////////////////////////////////////////  
	//  Enumeration EForestTextureSampler

	enum EForestTextureRegister // must be in the same order as ETextureLayer in Core.h
	{
		// register 0
		TEXTURE_REGISTER_DIFFUSE = TL_DIFFUSE,
		TEXTURE_REGISTER_TERRAIN_SPLAT = TL_DIFFUSE,

		// register 1
		TEXTURE_REGISTER_NORMAL = TL_NORMAL,
		TEXTURE_REGISTER_TERRAIN_NORMAL = TL_NORMAL,

		// register 2
		TEXTURE_REGISTER_DETAIL = TL_DETAIL_DIFFUSE,
		TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_0 = TL_DETAIL_DIFFUSE,

		// register 3
		TEXTURE_REGISTER_DETAIL_NORMAL = TL_DETAIL_NORMAL,
		TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_1 = TL_DETAIL_NORMAL,

		// register 4
		TEXTURE_REGISTER_SPECULAR_MASK = TL_SPECULAR_MASK,
		TEXTURE_REGISTER_TERRAIN_SPLAT_LAYER_2 = TL_SPECULAR_MASK,

		// register 5
		TEXTURE_REGISTER_TRANSMISSION_MASK = TL_TRANSMISSION_MASK,

		// register 6
		TEXTURE_REGISTER_AUX_ATLAS_1 = TL_AUX_ATLAS1,

		// register 7
		TEXTURE_REGISTER_AUX_ATLAS_2 = TL_AUX_ATLAS2,

		// register 8
		TEXTURE_REGISTER_FIZZLE_NOISE, // fade/fizzling with alpha testing

		// register 9
		TEXTURE_REGISTER_PERLIN_NOISE_KERNEL, // rolling wind noise

		// register 10
		TEXTURE_REGISTER_IMAGE_BASED_AMBIENT_LIGHTING,

		// shadow register (11-14)
		TEXTURE_REGISTER_SHADOW_MAP_0,
		TEXTURE_REGISTER_SHADOW_MAP_1,
		TEXTURE_REGISTER_SHADOW_MAP_2,
		TEXTURE_REGISTER_SHADOW_MAP_3,

		TEXTURE_REGISTER_COUNT
	};


	///////////////////////////////////////////////////////////////////////  
	//  Enumeration ESamplerRegister

	enum ESamplerRegister // must be in the same order as ETextureLayer in Core.h
	{
		SAMPLER_REGISTER_STANDARD,
		SAMPLER_REGISTER_SHADOW_MAP_COMPARE,
		SAMPLER_REGISTER_POINT,
		SAMPLER_REGISTER_NO_MIPMAP,

		SAMPLER_REGISTER_COUNT
	};


	///////////////////////////////////////////////////////////////////////  
	//  Structure STextureInfo

	struct ST_DLL_LINK_STATIC_MEMVAR STextureInfo
	{
									STextureInfo( );

			st_int32				m_nWidth;
			st_int32				m_nHeight;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CTextureRI

	#define CTextureRI_TemplateList template<class TTexturePolicy>
	
	CTextureRI_TemplateList
	class ST_DLL_LINK CTextureRI
	{
	public:
										CTextureRI( );
										~CTextureRI( );

			// loading
			st_bool						Load(const char* pFilename, st_int32 nMaxAnisotropy = 0);
			st_bool						LoadColor(st_uint32 uiColor); // 0xrrggbbaa
			st_bool						LoadNoise(st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise = 0.0f, st_float32 fHighNoise = 1.0f);
			st_bool						LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
			st_bool						ReleaseGfxResources(void);

			// state/init
	static	void						SetSamplerStates(void);

			// other
			const char*					GetFilename(void) const;
			const STextureInfo&			GetInfo(void) const;
			st_bool						IsValid(void) const;
			CTextureRI&					operator=(const CTextureRI& cRight);
			st_bool						operator!=(const CTextureRI& cRight) const;

			TTexturePolicy				m_tTexturePolicy;

	private:
			typedef CResourceCache<CFixedString, TTexturePolicy> CTextureCache;

			SpeedTree::CFixedString		m_strFilename;
	static	CTextureCache*				m_pCache;
	};
	#define CTextureRI_t CTextureRI<TTexturePolicy>


	///////////////////////////////////////////////////////////////////////  
	//  Enumeration ERenderTargetType

	enum ERenderTargetType
	{
		RENDER_TARGET_TYPE_COLOR,
		RENDER_TARGET_TYPE_DEPTH,
		RENDER_TARGET_TYPE_SHADOW_MAP,
		RENDER_TARGET_TYPE_NULL
	};

	const st_int32 c_nMaxNumRenderTargets = 4;


	///////////////////////////////////////////////////////////////////////  
	//  Class CRenderTarget

	#define CRenderTargetRI_TemplateList template<class TRenderTargetPolicy>

	CRenderTargetRI_TemplateList
	class ST_DLL_LINK CRenderTargetRI
	{
	public:
									CRenderTargetRI( );
									~CRenderTargetRI( );

			st_bool					InitGfx(ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight, st_int32 nNumSamples);
			void					ReleaseGfxResources(void);

			void					Clear(const Vec4& vColor);

            st_bool                 SetAsTarget(void);                        
            void                    ReleaseAsTarget(void);
			st_bool                 BindAsTexture(st_int32 nRegisterIndex, st_bool bPointFilter = true) const;
			void					UnBindAsTexture(st_int32 nRegisterIndex) const;

			void					OnResetDevice(void);
			void					OnLostDevice(void);

	static	st_bool					SetGroupAsTarget(const CRenderTargetRI<TRenderTargetPolicy>* pTargets, st_int32 nNumTargets, st_bool bClear = true);
	static	void					ReleaseGroupAsTarget(const CRenderTargetRI<TRenderTargetPolicy>* pTargets, st_int32 nNumTargets);

			TRenderTargetPolicy		m_tRenderTargetPolicy;
	};
	#define CRenderTargetRI_t CRenderTargetRI<TRenderTargetPolicy>


	///////////////////////////////////////////////////////////////////////  
	//  Supporting data types for CGeometryBuffer

	enum EPrimitiveType
	{
		PRIMITIVE_POINTS, 
		PRIMITIVE_LINE_STRIP, 
		PRIMITIVE_LINE_LOOP, 
		PRIMITIVE_LINES, 
		PRIMITIVE_TRIANGLE_STRIP, 
		PRIMITIVE_TRIANGLE_FAN, 
		PRIMITIVE_TRIANGLES, 
		PRIMITIVE_QUAD_STRIP, 
		PRIMITIVE_QUADS
	};

	enum EIndexFormat
	{
		INDEX_FORMAT_UNSIGNED_16BIT,
		INDEX_FORMAT_UNSIGNED_32BIT
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CGeometryBufferRI

	#define CGeometryBufferRI_TemplateList template<class TGeometryBufferPolicy, class TShaderTechniqueClass>

	CGeometryBufferRI_TemplateList
	class ST_DLL_LINK CGeometryBufferRI
	{
	public:
											CGeometryBufferRI(st_bool bDynamicVB = false, st_bool bDynamicIB = false, const char* pResourceId = nullptr);
			virtual							~CGeometryBufferRI( );

			// vertex buffer
			st_bool							SetVertexDecl(const SVertexDecl& sVertexDecl, const TShaderTechniqueClass* pTechnique, const SVertexDecl& sInstanceVertexDecl = SVertexDecl( ));
			st_bool							AppendVertices(const void* pVertexData, st_uint32 uiNumVertices);
			st_bool							EndVertices(void);
			st_bool							CreateUninitializedVertexBuffer(st_uint32 uiNumVertices); // alternate to AppendVertices/EndVertices
			st_bool							OverwriteVertices(const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiOffset, st_uint32 uiStream = 0);

			// direct vertex buffer write
			void*							LockForWrite(st_int32 uiBufferSize); // will resize on-the-fly if needed
			st_bool							UnlockFromWrite(void) const;

			st_uint32						NumVertices(void) const;
			st_uint32						VertexSize(void) const; // in bytes
			st_bool							ClearVertices(void);

			st_bool							EnableFormat(void) const;
	static	st_bool            ST_CALL_CONV DisableFormat(void);
			st_bool							BindVertexBuffer(st_uint32 uiStream = 0) const;
	static	st_bool            ST_CALL_CONV UnBindVertexBuffer(st_uint32 uiStream = 0);

			// index buffer
			st_bool							SetIndexFormat(EIndexFormat eFormat);
			st_bool							AppendIndices(const void* pIndexData, st_uint32 uiNumIndices);
			st_bool							EndIndices(void);
			st_bool							CreateUninitializedIndexBuffer(st_uint32 uiNumIndices); // alternate to AppendIndices/EndIndices
			st_bool							OverwriteIndices(const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset);
			st_uint32						NumIndices(void) const;
			st_uint32						IndexSize(void) const; // in bytes
			st_bool							ClearIndices(void);

			st_bool							BindIndexBuffer(void) const;
	static	st_bool		       ST_CALL_CONV UnBindIndexBuffer(void);

			// render functions
			st_bool							RenderIndexed(EPrimitiveType ePrimType, 
														  st_uint32 uiStartIndex, 
														  st_uint32 uiNumIndices,
														  st_uint32 uiMinIndex = 0,
														  st_uint32 uiNumVertices = 0) const;
			st_bool							RenderIndexedInstanced(EPrimitiveType ePrimType,
																   st_uint32 uiStartIndex,
																   st_uint32 uiNumIndices,
																   st_uint32 uiNumInstances,
																   st_uint32 uiStartInstanceLocation = 0) const;
			st_bool							RenderArrays(EPrimitiveType ePrimType, st_uint32 uiStartVertex, st_uint32 uiNumVertices) const;
	static	st_bool            ST_CALL_CONV IsPrimitiveTypeSupported(EPrimitiveType ePrimType);

			void							ReleaseGfxResources(void);

			// memory management
			st_bool							Reserve(st_uint32 uiNumVertices, st_uint32 uiNumIndices);

			// mesh instancing support
			const SVertexDecl&				GetVertexDecl(void) const;
			const TShaderTechniqueClass*	GetTechnique(void) const;

			TGeometryBufferPolicy			m_tGeometryBufferPolicy;

	private:
			// vertex buffer
			SVertexDecl						m_sVertexDecl;
			st_bool							m_bDynamicVB;
			CArray<st_byte>					m_aVertexData;
			st_uint32						m_uiNumVertices;

			// index buffer
			st_bool							m_bDynamicIB;
			CArray<st_byte>					m_aIndexData;
			st_uint32						m_uiIndexSize;			// in bytes
			st_uint32						m_uiNumIndices;

			// memory management
			st_int32						m_nVertexHeapHandle;
			st_int32						m_nIndexHeapHandle;
			const char*						m_pResourceId;

			// instancing support
			const TShaderTechniqueClass*	m_pTechnique;
			SVertexDecl						m_sInstanceVertexDecl;
	};
	#define CGeometryBufferRI_t CGeometryBufferRI<TGeometryBufferPolicy, TShaderTechniqueClass>


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantRI

	#define CShaderConstantRI_TemplateList template<class TShaderConstantPolicy, class TTextureClass>

	CShaderConstantRI_TemplateList
	class ST_DLL_LINK CShaderConstantRI
	{
	public:
	static	st_bool	   ST_CALL_CONV Init(void);
	static	void	   ST_CALL_CONV ReleaseGfxResources(void);
	static	void       ST_CALL_CONV Reset(void);

	static	st_bool	   ST_CALL_CONV Set1f(const SStaticShaderConstant& sRegister, st_float32 x);
	static	st_bool	   ST_CALL_CONV Set2f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y);
	static	st_bool	   ST_CALL_CONV Set2fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[2]);
	static	st_bool	   ST_CALL_CONV Set3f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z);
	static	st_bool	   ST_CALL_CONV Set3fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[3]);
	static	st_bool	   ST_CALL_CONV Set4f(const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w);
	static	st_bool	   ST_CALL_CONV Set4fv(const SStaticShaderConstant& sRegister, const st_float32 afValues[4]);
	static  st_bool	   ST_CALL_CONV Set4fvArray(const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues);
	static	st_bool	   ST_CALL_CONV SetMatrix(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);
	static	st_bool	   ST_CALL_CONV SetMatrixTranspose(const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16]);
	static	st_bool    ST_CALL_CONV SetTexture(st_int32 nRegister, const TTextureClass& cTexture, st_bool bSubmitImmediately = true);
	static	void       ST_CALL_CONV SubmitSetTexturesInBatch(void);
	};
	#define CShaderConstantRI_t CShaderConstantRI<TShaderConstantPolicy, TTextureClass>


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderConstantBufferRI

	#define CShaderConstantBufferRI_TemplateList template<class TShaderConstantBufferPolicy>

	CShaderConstantBufferRI_TemplateList
	class ST_DLL_LINK CShaderConstantBufferRI
	{
	public:
			st_bool						Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister);
			void						ReleaseGfxResources(void);

			st_bool        				Update(void) const;				// copy values to GPU constant buffer
			st_bool						Bind(void) const;	// make active

			TShaderConstantBufferPolicy	m_tShaderConstantBufferPolicy;
	};
	#define CShaderConstantBufferRI_t CShaderConstantBufferRI<TShaderConstantBufferPolicy>


	///////////////////////////////////////////////////////////////////////  
	//  Class CShaderTechniqueRI

	#define CShaderTechniqueRI_TemplateList template<class TShaderTechniquePolicy>
	#define CShaderTechniqueRI_t CShaderTechniqueRI<TShaderTechniquePolicy>
	
	CShaderTechniqueRI_TemplateList
	class ST_DLL_LINK CShaderTechniqueRI
	{
	public:
			typedef typename TShaderTechniquePolicy::CVertexShader TVertexShader;
			typedef typename TShaderTechniquePolicy::CPixelShader TPixelShader;

												CShaderTechniqueRI( );
												~CShaderTechniqueRI( );

			// render loop related
			st_bool								Bind(void) const;
	static  st_bool	               ST_CALL_CONV	UnBind(void);

			st_bool								IsValid(void) const;
			const char*							GetName(void) const;
			void								SetName(const char* pName);

			// loading related
			st_bool                ST_CALL_CONV	Load(const char* pVertexFilename, 
													 const char* pPixelFilename, 
													 const SAppState& sAppState,
													 const SRenderState& sRenderState);
			void				   ST_CALL_CONV	ReleaseGfxResources(void);

	static	CFixedString   		   ST_CALL_CONV GetCompiledShaderExtension(void);
	static	CFixedString   		   ST_CALL_CONV GetCompiledShaderFolder(void);
	static	st_bool                ST_CALL_CONV	VertexDeclNeedsInstancingAttribs(void);

			// utility
			const TVertexShader&				GetVertexShader(void) const	{ return m_tVertexShader; }
			const TPixelShader&					GetPixelShader(void) const { return m_tPixelShader; }

	private:
			typedef CResourceCache<CFixedString, TVertexShader> CVertexShaderCache;
			typedef CResourceCache<CFixedString, TPixelShader> CPixelShaderCache;

			TVertexShader						m_tVertexShader;
			TPixelShader						m_tPixelShader;
			CFixedString						m_strVertexShaderName;
			CFixedString						m_strPixelShaderName;
			// LAVA++
			CFixedString                        m_strPixelShaderCacheName;
			// LAVA--

			TShaderTechniquePolicy				m_tShaderTechniquePolicy;
	
	static	CVertexShaderCache*					m_pVertexShaderCache;
	static	CPixelShaderCache*					m_pPixelShaderCache;
	};


	// include inline functions
	#include "ResourceCache_inl.h"
	#include "StateBlockRI_inl.h"
    #include "TextureRI_inl.h"
    #include "RenderTargetRI_inl.h"
	#include "GeometryBufferRI_inl.h"
	#include "ShaderRI_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
