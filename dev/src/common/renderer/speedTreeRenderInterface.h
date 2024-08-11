/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#ifdef USE_SPEED_TREE

class CSpeedTreeInstanceRingBuffer
{
public:
	Uint8*				m_lockedData;
	Int32				m_currentOffset;
	GpuApi::BufferRef	m_instanceBufferRef;
	Uint32				m_size;
	Bool				m_isLocked;

	CSpeedTreeInstanceRingBuffer() : m_lockedData(nullptr), m_currentOffset(0), m_size(0), m_isLocked(false) {}
	void SetRingBufferSize( Uint32 size );
	RED_INLINE Uint32 GetRingBufferSize() { return m_size; }
	Bool AppendData( Uint8* data, size_t size );
	Bool GetCurrentInstancePtr( Uint8*& outData, size_t size );
	Bool Lock();
	void Unlock();
	void Bind( Uint32 reg, Uint32 offset, Uint32 stride );
	void ReleaseBuffer();
};

// this structure should be before the includes because it's going to be used to keep stats from the rendering
struct SSpeedTreeRenderStats
{
	static void NextFrame();
	static Int32 ReadIndex();
	static const Int32 c_bufferCount = 2;
	static Int32 s_bufferIndex;
	static Uint64 s_grassLayerCount[ c_bufferCount ];
	static Uint64 s_visibleGrassCellCount[ c_bufferCount ];
	static Uint64 s_visibleGrassCellArrayCapacity[ c_bufferCount ];
	static Uint64 s_visibleGrassCellArraySize[ c_bufferCount ];
	static Uint64 s_visibleGrassInstanceCount[ c_bufferCount ];
	static Uint64 s_visibleGrassInstanceArrayCapacity[ c_bufferCount ];
	static Uint64 s_visibleGrassInstanceArraySize[ c_bufferCount ];
	static Uint64 s_visibleTreeCellCount[ c_bufferCount ];
	static Uint64 s_visibleTreeCellArrayCapacity[ c_bufferCount ];
	static Uint64 s_visibleTreeCellArraySize[ c_bufferCount ];
	static Uint64 s_visibleTreeInstanceCount[ c_bufferCount ];
	static Uint64 s_visibleTreeInstanceArrayCapacity[ c_bufferCount ];
	static Uint64 s_visibleTreeInstanceArraySize[ c_bufferCount ];

	static Float s_maxGrassLayerCullDistance[ c_bufferCount ];
	static Float s_minGrassCellSize[ c_bufferCount ];
	static Float s_maxGrassCellSize[ c_bufferCount ];

	static Uint64 s_treesRendered[ c_bufferCount ];
	static Uint64 s_billboardsRendered[ c_bufferCount ];
	static Uint64 s_grassRendered[ c_bufferCount ];

	static Uint64 s_treeDrawcalls[ c_bufferCount ];
	static Uint64 s_billboardDrawcalls[ c_bufferCount ];
	static Uint64 s_grassDrawcalls[ c_bufferCount ];
};

#include <Core/Core.h>
#include <RenderInterface/ForestRI.h>
#include <Utilities/Utility.h>
#include <RenderInterface/ShaderConstants.h>
#include "renderTexture.h"

namespace SpeedTree
{
	// Render state block, implemented through GpuApi draw contexts
	class ST_DLL_LINK CStateBlockGPUAPI
	{
	public:
		CStateBlockGPUAPI( );

		st_bool						Init( const SAppState& sAppState, const SRenderState& sRenderState, st_bool isInteractive );
		st_bool						Bind() const;
		void						ReleaseGfxResources();
		void						AddLightChannelInteractiveFlag();
		st_int32					GetDrawContextRefValue() const;

	private:
		GpuApi::EDepthStencilStateMode		m_depthStencilMode;
		GpuApi::ERasterizerMode				m_rasterizerMode;
		GpuApi::EBlendMode					m_blendMode;
		Int32								m_drawContextRefValue;
	};
	typedef CStateBlockRI<CStateBlockGPUAPI> CStateBlock;

	//////////////////////////////////////////////////////////////////////////

	class ST_DLL_LINK_STATIC_MEMVAR CTextureGPUAPI
	{
	public:
		CTextureGPUAPI();
		~CTextureGPUAPI();

		// Part of the policy
		st_bool										Load( const char* pFilename, st_int32 nMaxAnisotropy = 0 );
		st_bool										LoadColor( st_uint32 ulColor );
		st_bool										LoadNoise( st_int32 nWidth, st_int32 nHeight, st_float32 fLowNoise, st_float32 fHighNoise );
		st_bool										LoadPerlinNoiseKernel(st_int32 nWidth, st_int32 nHeight, st_int32 nDepth);
		st_bool										ReleaseGfxResources();
		st_bool										IsValid() const;
		static	void ST_CALL_CONV					SetSamplers( st_int32 nMaxAnisotropy ); // (comment by SpeedTree) todo: maybe not needed 
		static	void ST_CALL_CONV					SetSamplerStates(void);
		const STextureInfo&							GetInfo() const;

		// Internal (not called by Speed Tree RI templates)
		CRenderTexture*								GetRenderTexture() const;
		st_bool										IsGeneratedUniformColor() const;
	//static	GpuApi::SamplerStateRef ST_CALL_CONV	GetFilteredMipMapSampler();
	//static	GpuApi::SamplerStateRef ST_CALL_CONV	GetPointNoMipMapSampler();

		CTextureGPUAPI&								operator=( const CTextureGPUAPI& cRight );
		st_bool										operator!=( const CTextureGPUAPI& cRight ) const;

	private:

	/*static	void ST_CALL_CONV						CreateSharedFilteredMipMapSampler( st_int32 nMaxAnisotropy );
	static	void ST_CALL_CONV						CreateSharedPointNoMipMapSampler();*/

		CRenderTexture*								m_texture;

		STextureInfo								m_sInfo;
		st_bool										m_bIsGeneratedUniformColor;

	};
	typedef CTextureRI<CTextureGPUAPI> CTexture;

	///////////////////////////////////////////////////////////////////////  

	//class ST_DLL_LINK_STATIC_MEMVAR CRenderTargetGPUAPI
	//{
	//public:
	//	CRenderTargetGPUAPI();
	//	~CRenderTargetGPUAPI();

	//	st_bool					    InitGfx( ERenderTargetType eType, st_int32 nWidth, st_int32 nHeight );
	//	void					    ReleaseGfxResources();

	//	void						Clear( const Vec4& vColor );
	//	st_bool                     BindAsTexture( st_int32 nSamplerIndex );
	//	void						UnBindAsTexture( st_int32 nSamplerIndex );

	//	void					    OnResetDevice();
	//	void					    OnLostDevice();

	//private:
	//	st_bool                     CreateColorTarget();
	//	st_bool                     CreateDepthTarget();

	//	ERenderTargetType			m_eType;
	//	st_int32					m_nWidth;
	//	st_int32					m_nHeight;

	//	GpuApi::TextureRef			m_texture;

	//	//GpuApi::SamplerStateRef		m_pPreviousSamplerState;
	//};
	//typedef CRenderTargetRI<CRenderTargetGPUAPI> CRenderTarget;


	///////////////////////////////////////////////////////////////////////  

	class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantGPUAPI
	{
	public:
		struct SCBuffer
		{
			GpuApi::BufferRef	m_bufferRef;
			GpuApi::Uint32		m_bufSize;

			SCBuffer()
				: m_bufSize( 0 )
			{}
		};

	public:
		static	st_bool    ST_CALL_CONV Init();
		static	void       ST_CALL_CONV ReleaseGfxResources();
		static	void       ST_CALL_CONV Reset();

		static	st_bool    ST_CALL_CONV Set4f( const SStaticShaderConstant& sRegister, st_float32 x, st_float32 y, st_float32 z, st_float32 w );
		static	st_bool    ST_CALL_CONV Set4fv( const SStaticShaderConstant& sRegister, const st_float32 afValues[4] );
		static  st_bool    ST_CALL_CONV Set4fvArray( const SStaticShaderConstant& sRegister, st_int32 nNum4fValues, const st_float32* p4fValues );
		static	st_bool    ST_CALL_CONV SetMatrix( const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16] );
		static	st_bool    ST_CALL_CONV SetMatrixTranspose( const SStaticShaderConstant& sRegister, const st_float32 afMatrix[16] );

		static	void       ST_CALL_CONV SubmitSetTexturesInBatch(void);

		static	st_bool    ST_CALL_CONV CommitConstants();

	private:
		static	void       ST_CALL_CONV	FlagConstantBufferAsUpdated(const SStaticShaderConstant& sRegister);

		static	SCBuffer				m_apConstantBuffers[SHADER_CONSTANT_GROUP_COUNT];
		static	st_bool					m_abBufferUpdated[SHADER_CONSTANT_GROUP_COUNT];
		static	st_int32				m_anMapRegisterToGroup[c_nNumMainShaderUniformSlots];
		static	st_float32				m_afConstantsMirror[c_nNumMainShaderUniformSlots * 4];
	};


	///////////////////////////////////////////////////////////////////////  

	class ST_DLL_LINK CShaderConstant : public CShaderConstantRI<CShaderConstantGPUAPI, CTexture>
	{
	public:
		static	st_bool    ST_CALL_CONV SetTexture( st_int32 nSampler, const CTexture& cTexture, st_bool bSubmitImmediately/*LAVA:Adding distance*/, Float distance = 0.0f );
	};

	class ST_DLL_LINK_STATIC_MEMVAR CShaderConstantBufferGPUAPI
	{
	public:
		CShaderConstantBufferGPUAPI( );
		~CShaderConstantBufferGPUAPI( );

		st_bool        ST_CALL_CONV Init(void* pLayout, size_t siSizeOfLayout, st_int32 nRegister);
		void           ST_CALL_CONV ReleaseGfxResources(void);

		st_bool        ST_CALL_CONV	Update(void) const;
		st_bool        ST_CALL_CONV	Bind(void) const;

	private:
	private:
		mutable GpuApi::BufferRef	m_constantBuffer;
		void*						m_data;
		size_t						m_dataSize;
		st_int32					m_register;
	};
	// todo: delete
	//#define CShaderConstantBuffer(TLayout) CShaderConstantBufferRI<CShaderConstantBufferDirectX11, TLayout>
	typedef CShaderConstantBufferRI<CShaderConstantBufferGPUAPI> CShaderConstantBuffer;


	///////////////////////////////////////////////////////////////////////  

	class ST_DLL_LINK CShaderTechniqueGPUAPI
	{
	public:
		///////////////////////////////////////////////////////////////////////  
		//  Class CVertexShader

		class ST_DLL_LINK CVertexShader
		{
		public:
			CVertexShader();

			st_bool					Load( const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState );
			st_bool					IsValid() const;
			void					ReleaseGfxResources();

			GpuApi::ShaderRef		m_shaderRef;
			void*					m_compiledShaderCode;
			Uint32					m_compiledShaderCodeSize;
		};


		///////////////////////////////////////////////////////////////////////  
		//  Class CPixelShader

		class ST_DLL_LINK CPixelShader
		{
		private:
			static st_bool			IsForcedNullShader( const SRenderState& sRenderState );

		public:
			CPixelShader();

			static void				AdaptCacheFilename( CFixedString &refCacheFilename, const SRenderState& sRenderState );

			st_bool					Load( const char* pFilename, const SAppState& sAppState, const SRenderState& sRenderState );
			st_bool					IsValid() const;
			void					ReleaseGfxResources();

			GpuApi::ShaderRef		m_shaderRef;

			// For shadow passes, we don't want a pixel shader if we don't need it for discarding. So, it's possible that we'll
			// leave m_shaderRef null when loading, but for it to be a perfectly valid situation. We need to track whether it's
			// null because of error or because of this.
			Bool					m_forcedNull;
		};

		st_bool							Link( const CVertexShader& cVertexShader, const CPixelShader& cPixelShader );
		st_bool							Bind( const CVertexShader& cVertexShader, const CPixelShader& cPixelShader ) const;
		static	void			   ST_CALL_CONV UnBind();
		st_bool							ReleaseGfxResources();

		st_bool 						LoadProgramBinary( const char*, const char* ) { return false; }
		st_bool 						SaveProgramBinary( const char*, const char* ) { return false; }

		static	CFixedString   	   ST_CALL_CONV	GetCompiledShaderExtension();
		static	CFixedString   	   ST_CALL_CONV	GetCompiledShaderFolder();
		static	st_bool     	   ST_CALL_CONV	VertexDeclNeedsInstancingAttribs();
	};
	typedef CShaderTechniqueRI<CShaderTechniqueGPUAPI> CShaderTechnique;


	///////////////////////////////////////////////////////////////////////  

	class ST_DLL_LINK CGeometryBufferGPUAPI
	{
	public:
		CGeometryBufferGPUAPI();
		~CGeometryBufferGPUAPI( );

		// vertex buffer
		st_bool							SetVertexDecl( const SVertexDecl& sVertexDecl, const CShaderTechnique* pTechnique, const SVertexDecl& sInstanceVertexDecl );
		st_bool							CreateVertexBuffer( st_bool bDynamic, const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize );
		st_bool							OverwriteVertices( const void* pVertexData, st_uint32 uiNumVertices, st_uint32 uiVertexSize, st_uint32 uiOffset, st_uint32 uiStream );

		// direct vertex buffer write
		void*							LockForWrite(st_uint32 uiBufferSize); // will resize on-the-fly if necessary
		st_bool							UnlockFromWrite(void) const;

		st_bool							VertexBufferIsValid() const;

		st_bool							EnableFormat() const;
		static	st_bool		       ST_CALL_CONV DisableFormat();
		st_bool							BindVertexBuffer( st_uint32 uiStream, st_uint32 uiVertexSize ) const;
		static	st_bool		       ST_CALL_CONV UnBindVertexBuffer( st_uint32 uiStream );

		// mesh instancing support
		static	st_bool            ST_CALL_CONV InstancingRequiresSeparateVertexStream();

		// index buffer
		st_bool							SetIndexFormat( EIndexFormat eFormat );
		st_bool							CreateIndexBuffer( st_bool bDynamic, const void* pIndexData, st_uint32 uiNumIndices );
		st_bool							OverwriteIndices( const void* pIndexData, st_uint32 uiNumIndices, st_uint32 uiOffset );
		st_bool							IndexBufferIsValid() const;
		st_bool							ClearIndices();
		st_uint32						IndexSize() const;

		st_bool							BindIndexBuffer() const;
		static	st_bool			   ST_CALL_CONV UnBindIndexBuffer();

		void							ReleaseGfxResources();

		// render functions
		st_bool							RenderIndexed( EPrimitiveType ePrimType, 
			st_uint32 uiStartIndex, 
			st_uint32 uiNumIndices, 
			st_uint32 uiMinIndex,
			st_uint32 uiNumVertices ) const;
		st_bool							RenderIndexedInstanced( EPrimitiveType ePrimType,
			st_uint32 uiStartIndex,
			st_uint32 uiNumIndices,
			st_uint32 uiNumInstances,
			st_uint32 uiStartInstanceLocation ) const;
		st_bool							RenderArrays( EPrimitiveType ePrimType, 
			st_uint32 uiStartVertex, 
			st_uint32 uiNumVertices ) const;
		static	st_bool			   ST_CALL_CONV IsPrimitiveTypeSupported( EPrimitiveType ePrimType );

	private:
		st_bool						IsFormatSet() const;

		GpuApi::VertexLayoutRef		m_vertexLayout;
		GpuApi::BufferRef			m_vertexBufferRef;
		GpuApi::BufferRef			m_indexBufferRef;
		EIndexFormat				m_eIndexFormat;
		Uint32						m_uiCurrentIndexBufferSize;
		Bool						m_dynamic;
		mutable void*				m_commandList;
		mutable void*				m_lockedBuffer;
	};
	typedef CGeometryBufferRI<CGeometryBufferGPUAPI, CShaderTechnique> CGeometryBuffer;


	///////////////////////////////////////////////////////////////////////  
	//  Class CInstancingMgrGPUAPI
	//	One CInstancingMgrGPUAPI object is used per base tree.

	class ST_DLL_LINK CInstancingMgrGPUAPI
	{
	public:
		CInstancingMgrGPUAPI( );
		~CInstancingMgrGPUAPI( );

		// functions called by CInstancingMgrRI in the RenderInterface library
		st_bool							Init(SVertexDecl::EInstanceType eInstanceType, st_int32 nNumLods, const CGeometryBuffer* pGeometryBuffers, st_int32 nNumGeometryBuffers);
		void							ReleaseGfxResources(void);

		st_bool							Update(st_int32 nLod, const st_byte* pInstanceData, st_int32 nNumInstances/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);
		st_bool							UpdateWithFrustumTest(st_int32 nLod, const TGrassInstArray& instanceArray/*LAVA++*/, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer, st_int32& outNumInstances );
		st_bool							UpdateWithFrustumTest(st_int32 nLod, const CArray<SBillboardInstanceVertex>& aInstances/*LAVA++*/, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer, st_int32& outNumInstances );
		st_bool							Render(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const;

		void*							LockInstanceBufferForWrite(st_int32 nLod, st_int32 nNumInstances);
		st_bool							UnlockInstanceBufferFromWrite(st_int32 nLod);

		st_int32						NumInstances(st_int32 nLod) const;

		void							OverwriteOffsetAndCount( Uint32 lod, Uint32 offset, Uint32 instanceCount );

	private:
		struct SInstancesPerLod
		{
			SInstancesPerLod( );

			st_int32					m_nNumInstances;

			// Instances are provided through a vertex stream.
			Uint32						m_instanceBufferOffset;

			st_bool						m_bBufferLocked;
		};
		CArray<SInstancesPerLod>		m_asInstances;		// one element per LOD level in the base tree

		const CGeometryBuffer*			m_pObjectBuffers;
		size_t							m_siSizeOfPerInstanceData;
	};
	typedef CInstancingMgrRI<CInstancingMgrGPUAPI, CGeometryBuffer> CInstancingMgr;

	//////////////////////////////////////////////////////////////////////////

	typedef CVisibleInstancesRI<CStateBlock,
		CTexture, 
		CGeometryBuffer,
		CInstancingMgr,
		CShaderTechnique,
		CShaderConstant,
		CShaderConstantBuffer > CVisibleInstancesRender;


	///////////////////////////////////////////////////////////////////////  

	typedef CForestRI<CStateBlock,
		CTexture, 
		CGeometryBuffer,
		CInstancingMgr,
		CShaderTechnique, 
		CShaderConstant,
		CShaderConstantBuffer > CForestRender;


	///////////////////////////////////////////////////////////////////////  

	typedef CRenderStateRI<CStateBlock,
		CTexture, 
		CShaderTechnique,
		CShaderConstant,
		CShaderConstantBuffer> CRenderState; 


	///////////////////////////////////////////////////////////////////////  

	typedef CTreeRI<CStateBlock,
		CTexture, 
		CGeometryBuffer,
		CShaderTechnique,
		CShaderConstant,
		CShaderConstantBuffer> CTreeRender;

	void GetSpeedTreeGeneralStats( CTreeRender* treeRender, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats >& stats );
	void GatherInformationAboutTextures( const CRenderState &state, TDynArray< SSpeedTreeResourceMetrics::SGeneralSpeedTreeStats > &stats );

#include "Texture_inl.h"
#include "Shaders_inl.h"
#include "GeometryBuffer_inl.h"
#include "InstancingManager_inl.h"

	// Override speed tree filesystem
	class ST_DLL_LINK CLavaFileSystem : public CFileSystem
	{
	public:
		bool		FileExists(const st_char* pFilename);

		size_t		FileSize(const st_char* pFilename);

		st_byte*	LoadFile(const char* pFilename, ETermHint eTermHint);

		void		Release(st_byte* pBuffer);
	};
}

static	SpeedTree::CLavaFileSystem		GLavaFileSystem;
static	SpeedTree::CFileSystemInterface	GFileSystemInterface(&GLavaFileSystem);

#endif
