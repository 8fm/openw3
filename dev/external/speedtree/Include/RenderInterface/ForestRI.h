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
//  *** Release version 7.0.0 ***


///////////////////////////////////////////////////////////////////////  
//  Preprocessor

#pragma once
#include "Core/ExportBegin.h"
#define SPEEDTREE_RENDER_STATS
#include "RenderInterface/GraphicsApiAbstractionRI.h"
#include "RenderInterface/ShaderConstantBuffers.h"
#include "Core/Timer.h"


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
	//  Enumeration EStatsCategory

	enum EStatsCategory
	{
		// main geometry types
		STATS_CATEGORY_3D_TREES,
		STATS_CATEGORY_BILLBOARDS,
		STATS_CATEGORY_TERRAIN,
		STATS_CATEGORY_SKY,

		// utility
		STATS_CATEGORY_COUNT
	};
	
	// ctremblay +- CRenderStats Not thread safe. Memory Stomp all over the place. To be sure no one use them. I completely remove the code everywhere.

	///////////////////////////////////////////////////////////////////////  
	//  Enumeration ETextureAlphaRenderMode

	enum ETextureAlphaRenderMode
	{
		TRANS_TEXTURE_ALPHA_TESTING,
		TRANS_TEXTURE_ALPHA_TO_COVERAGE,
		TRANS_TEXTURE_BLENDING,
		TRANS_TEXTURE_NOTHING,
		TRANS_TEXTURE_UNASSIGNED
	};


	///////////////////////////////////////////////////////////////////////  
	//  Structure SForestRenderInfo

	struct ST_DLL_LINK SForestRenderInfo
	{
									SForestRenderInfo( );

		// app-level
		SAppState					m_sAppState;

		// general rendering
		st_int32					m_nMaxAnisotropy;				// see note below
		st_bool						m_bHorizontalBillboards;		// see note below
		st_bool						m_bDepthOnlyPrepass;			// see note below
		st_float32					m_fNearClip;
		st_float32					m_fFarClip;
		st_bool						m_bTexturingEnabled;
		st_float32					m_fTextureAlphaScalar3d;
		st_float32					m_fTextureAlphaScalarGrass;
		st_float32					m_fTextureAlphaScalarBillboards;

		// lighting
		SSimpleMaterial				m_sLightMaterial;
		CFixedString				m_strImageBasedAmbientLightingFilename;

		// fog
		st_float32					m_fFogStartDistance;
		st_float32					m_fFogEndDistance;
		st_float32					m_fFogDensity;
		Vec3						m_vFogColor;

		// sky
		Vec3						m_vSkyColor;
		st_float32					m_fSkyFogMin;
		st_float32					m_fSkyFogMax;

		// sun
		Vec3						m_vSunColor;
		st_float32					m_fSunSize;
		st_float32					m_fSunSpreadExponent;

		// shadows
		st_bool						m_bShadowsEnabled;
		st_int32					m_nShadowsNumMaps;
		st_int32					m_nShadowsResolution;
		st_float32					m_afShadowMapRanges[c_nMaxNumShadowMaps];
		st_float32					m_fShadowFadePercent;

		// *note: these values will be ignored if changed after CForestRI::InitGfx() has been called
	};


    ///////////////////////////////////////////////////////////////////////  
    //  Enumeration ETextureBindMode

    enum ETextureBindMode
    {
        TEXTURE_BIND_ENABLED,       // textures are bound as loaded in the render state object
        TEXTURE_BIND_DISABLED,      // no textures are bound
        TEXTURE_BIND_FALLBACK       // fallback textures are bound (used for "untextured" render mode)
    };


	///////////////////////////////////////////////////////////////////////  
	//  Class CRenderStateRI

	#define CRenderStateRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CRenderStateRI_TemplateList
	class ST_DLL_LINK CRenderStateRI : public SRenderState
	{
	public:

											CRenderStateRI( );
	virtual									~CRenderStateRI( );

			// creation/destruction
			st_bool							InitGfx(const SAppState& sAppState,
													const CArray<CFixedString>& aSearchPaths, 
													st_int32 nMaxAnisotropy,
													st_float32 fTextureAlphaScalar,
													const CFixedString& strVertexShaderBaseName,
													const CFixedString& strPixelShaderBaseName,
													const CWind* pWind,
													st_bool isInteractive);		// LAVA edit: add isInteractive flag
			void							ReleaseGfxResources(void);
			const TShaderTechniqueClass&	GetTechnique(void) const;
			const TStateBlockClass&			GetStateBlock(void) const;

			// LAVA++
			const TTextureClass&			GetTextureClass( st_uint32 uiIndex ) const;
			// LAVA--

			// render loop
			st_bool							BindConstantBuffer(void) const;
			st_bool							BindShader(void) const;
			st_bool							BindTextures(ERenderPass ePass, ETextureBindMode eTextureBindMode/*LAVA:Adding distance*/, Float distance = 0.0f) const;
			st_bool							BindStateBlock(void) const;
			st_bool							BindMaterialWhole(ERenderPass ePass, ETextureBindMode eTextureBindMode/*LAVA:Adding distance*/, Float distance) const;
	static	st_bool	   ST_CALL_CONV			UnBind(void);
	static	void	   ST_CALL_CONV			ClearLastBoundTextures(void);

			// utility
			CRenderStateRI&					operator=(const CRenderStateRI& sRight);
			CRenderStateRI&					operator=(const SRenderState& sRight);
			st_int64						GetHashKey(void) const;

			void							ReleaseTextures();

	private:
			st_bool							LoadTextures(const CArray<CFixedString>& aSearchPaths, st_int32 nMaxAnisotropy);
			st_bool							InitTexture(const char* pFilename,
														TTextureClass& tTextureObject,
														const CArray<CFixedString>& aSearchPaths, 
														st_int32 nMaxAnisotropy);
			st_bool							InitConstantBuffer(const SAppState& sAppState, st_float32 fTextureAlphaScalar, const CWind* pWind);
			st_bool							BindTexture(st_int32 nLayer, const TTextureClass& tTexture/*LAVA:Adding distance*/, Float distance = 0.0f) const;
			st_bool							LoadShaders(const CArray<CFixedString>& aShaderSearchPaths, 
														const CFixedString& strVertexShaderBaseName,
														const CFixedString& strPixelShaderBaseName);

	// LAVA++ Let's make it public and call it from the guaranteed rendering thread, instead of InitGfx (loading thread most of the time)
	public:
		static	st_bool	   ST_CALL_CONV 		InitFallbackTextures(void);
		static	void	   ST_CALL_CONV 		ReleaseFallbackTextures(void);
		void									SetTextureAlphaScalar( st_float32 fTextureAlphaScalar );
	// LAVA--
	
	private:

			TShaderTechniqueClass			m_cTechnique;								// houses vertex & pixel shaders
			TStateBlockClass				m_cStateBlock;								// houses depth, rasterizer, and blend state
			TShaderConstantBufferClass		m_cConstantBuffer;							// todo: light color won't propagate by default
	mutable	SMaterialCBLayout				m_sConstantBufferLayout;
			st_int64						m_lSortKey;

			TTextureClass					m_atTextureObjects[TL_NUM_TEX_LAYERS];		// this render state's texture bank
	static  st_bool							m_bFallbackTexturesInited;
	static	TTextureClass					m_atLastBoundTextures[TL_NUM_TEX_LAYERS];	// used to prevent redundant texture binds
	static	TTextureClass					m_atFallbackTextures[TL_NUM_TEX_LAYERS];	// small, simple textures used as defaults
	static	st_int32 						m_nFallbackTextureRefCount;

			// LAVA++
			mutable st_bool m_isDirty;				// ctremblay: we can't update constant buffer on loading thead. Delay it and execute on Render Thread
			st_float32 m_forcedTextureAlphaScalar;	// acichocki: this cached value is to keep our custom textureAlphaScalar in case tree got reinitialized after SetTextureAlphaScalar call. this shouldn't happen. just a sanity check.
			// LAVA-- 
	};

#define CRenderStateRI_t CRenderStateRI<TStateBlockClass, TTextureClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>


	///////////////////////////////////////////////////////////////////////  
	//  Class CTreeRI

	#define CTreeRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CTreeRI_TemplateList
	class ST_DLL_LINK CTreeRI : public CTree
	{
			// LAVA++
	public:
			enum EBillboardColorType
			{
				BCT_None,
				BCT_Grass,
				BCT_Branches,
				BCT_Trees,

				BCT_DEFAULT = BCT_Trees
			};

			struct SCachedUserData
			{
				SCachedUserData ();
				void Init( st_int32 numStrings, const char * const *strings );
				void Reset();

				EBillboardColorType	m_billboardColorType;
				st_bool m_grassRandomColorEnabled;
				st_bool m_grassTerrainNormalsEnabled;
				st_bool m_grassFloodPigmentEnabled;
				st_bool m_grassPigmentEnabled;
				// LAVA++ add isInteractive flag to distinguish interactive foliage (highlighted in focus mode)
				st_bool m_interactiveEnabled;
				// LAVA--
				//st_bool m_allowGlobalMaterialSettings; TODO: remove it if it really is not needed
			};
			// LAVA--

	public:
														CTreeRI( );
			virtual										~CTreeRI( );
	
			// graphics
			st_bool										InitGfx(const SAppState& sAppState,
																const CArray<CFixedString>& aSearchPaths,
																st_int32 nMaxAnisotropy = 0,
																st_float32 fTextureAlphaScalar = 1.0f);
			void										ReleaseTextures();
			void										ReleaseGfxResources(void);
			st_bool										GraphicsAreInitialized(void) const;

			// 3d geometry buffers
			const TGeometryBufferClass*					GetGeometryBuffer(st_int32 nLod, st_int32 nDrawCall) const;
			st_int32									GetGeometryBufferOffset(st_int32 nLod, st_int32 nDrawCall) const;
			const CArray<TGeometryBufferClass>&			GetGeometryBuffers(void) const;
			void										GetVertexBufferUsage(st_int32& nVertexBuffers, st_int32& nIndexBuffers) const;

			// render states
			st_bool										BindConstantBuffers(void) const;
			st_bool										UpdateWindConstantBuffer(void) const;
			const CArray<CRenderStateRI_t>&				Get3dRenderStates(ERenderPass eShaderType) const;
			const CRenderStateRI_t&						GetBillboardRenderState(ERenderPass eShaderType) const;

			// billboards
			const TGeometryBufferClass&					GetBillboardGeometryBuffer(void) const;

			// LAVA++
			const SCachedUserData&						GetCachedUserData() const { return m_cachedUserData; }
			void										SetTextureAlphaScalars( st_float32 alphaScalar3d, st_float32 alphaScalarGrass, st_float32 alphaScalarBillboards );

			// When we change grass distance, we want to update constant buffer values, before constant buffer is updated on GPU.
			void										UpdateBaseTreeCBDataBasedOnLodProfile();
			// LAVA--

	protected:
			st_int32									FindMaxNumDrawCallsPerLod(void);

			st_bool										InitRenderStates(const SAppState& sAppState, 
																		 const CArray<CFixedString>& aSearchPaths, 
																		 st_int32 nMaxAnisotropy,
																		 st_float32 fTextureAlphaScalar,
																		 const SCachedUserData& cachedUserData );		// LAVA edit: we pass cachedUserData, so we can create more different state key hashes
			st_bool										Init3dGeometry(void);
			st_bool										InitBillboardGeometry(void);
			st_bool										InitConstantBuffers(void);

			st_bool										InitTexture(const char* pFilename,
																	TTextureClass& texTextureObject,
																	const CArray<CFixedString>& aSearchPaths, 
																	st_int32 nMaxAnisotropy);

			// materials/textures
			CArray<CRenderStateRI_t>					m_aa3dRenderStates[RENDER_PASS_COUNT];
			CRenderStateRI_t							m_aBillboardRenderStates[RENDER_PASS_COUNT];

			// geometry
			CArray<TGeometryBufferClass>				m_atGeometryBuffers;
			st_int32									m_nMaxNumDrawCallsPerLod;

			// constant buffers
			TShaderConstantBufferClass					m_cBaseTreeConstantBuffer;
	mutable	SBaseTreeCBLayout							m_sBaseTreeConstantBufferLayout;
			TShaderConstantBufferClass					m_cWindDynamicsConstantBuffer;
	mutable	SWindDynamicsCBLayout						m_sWindDynamicsConstantBufferLayout;

			// billboards
			TGeometryBufferClass						m_tBillboardGeometryBuffer;

			// textures
			CArray<st_float32>							m_aModifiedBillboardTexCoords;		// copy of CTree::SGeometry::m_sVertBBs, but modified to include shader hints
			const CArray<CFixedString>*					m_pSearchPaths;

			// misc
			st_bool										m_bGraphicsInitialized;

			// LAVA++
			SCachedUserData								m_cachedUserData;
			// LAVA--

	private:
														CTreeRI(const CTreeRI& cRight);		// copying CTreeRI disabled
			// LAVA++
			mutable st_bool m_isDirty;
			// LAVA--
	};
	#define CTreeRI_t CTreeRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>


	///////////////////////////////////////////////////////////////////////  
	//  Structure SInstancedDrawStats

	struct ST_DLL_LINK SInstancedDrawStats
	{
										SInstancedDrawStats( );

			st_int32					m_nNumInstancesDrawn;
			st_int32					m_nNumDrawCalls;
			st_int32					m_nBatchSize;
	};


	///////////////////////////////////////////////////////////////////////
	//  Structure SBillboardVboBuffer

	struct SBillboardVboBuffer
	{
		st_int32					m_nNumInstances;
		SBillboardInstanceVertex*	m_pBuffer;
	};


	///////////////////////////////////////////////////////////////////////  
	//  Class CInstancingMgrRI

	#define CInstancingMgrRI_TemplateList template<class TInstanceMgrPolicy, class TGeometryBufferClass>

	#ifdef __CELLOS_LV2__
		const st_int32 c_nNumInstBuffers = 3;
	#elif defined(SPEEDTREE_OPENGL)
		const st_int32 c_nNumInstBuffers = 2;
	#else
		const st_int32 c_nNumInstBuffers = 1;	// LAVA: setting to 1, we don't need this.
	#endif

	CInstancingMgrRI_TemplateList
	class ST_DLL_LINK CInstancingMgrRI
	{
	public:

										CInstancingMgrRI( );
			virtual						~CInstancingMgrRI( );

			// init
			st_bool						Init3dTrees(st_int32 nNumLods, const CArray<TGeometryBufferClass>& aGeometryBuffers);
			st_bool						InitGrass(const CArray<TGeometryBufferClass>& aGeometryBuffers);
			st_bool						InitBillboards(const TGeometryBufferClass* pGeometryBuffer);
			void						ReleaseGfxResources(void);

			// update
			st_bool						PreUpdate3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods);
			st_bool						PostUpdate3dTreeInstanceBuffers(st_int32 nNumLods);
			st_bool						Update3dTreeInstanceBuffers(st_int32 nNumLods, const T3dTreeInstanceLodArray& aInstanceLods/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);
			st_bool						UpdateGrassInstanceBuffers(const TRowColCellPtrMap& mCells, const Vec4 * const frustumPlanes, st_float32 cullingRadius/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);

			// billboard update
			st_bool						UpdateBillboardInstanceBuffers(SBillboardVboBuffer& sBuffer, const CTree* pBaseTree, const TRowColCellPtrMap& mCells);
			st_bool						CopyInstancesToBillboardInstanceBuffer(SBillboardVboBuffer& sBuffer, const CTree* pBaseTree, const TRowColCellPtrMap& mCells);
			// LAVA++
			st_bool						LAVACopyInstancesToBillboardInstanceBufferDoneRight(const CTree* pBaseTree, const TRowColCellPtrMap& mCells, const Vec4 * const frustumPlanes, st_float32 cullingRadius, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);
			// LAVA--
			st_bool						CopyVboDataToGpu(const SBillboardVboBuffer& sBuffer/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);

			// render
			st_bool						Render3dTrees(st_int32 nGeometryBufferIndex, st_int32 nLod, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const;
			st_bool						RenderGrass(st_int32 nGeometryBufferIndex, SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const;
			st_bool						RenderBillboards(SInstancedDrawStats& sStats/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer) const;

			// simple queries
			st_bool						IsInitialized(void) const;
			st_int32					NumInstances(st_int32 nLod = 0) const;

	private:
			void						AdvanceMgrIndex(void);

			TInstanceMgrPolicy			m_atInstanceMgrPolicies[c_nNumInstBuffers];
			st_int32					m_nActiveMgrIndex;
			st_bool						m_bInitialized;
	};

#define CInstancingMgrRI_t CInstancingMgrRI<TInstanceMgrPolicy, TGeometryBufferClass>


	///////////////////////////////////////////////////////////////////////  
	//  Class CVisibleInstancesRI

	#define CVisibleInstancesRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TInstancingMgrClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CVisibleInstancesRI_TemplateList
	class ST_DLL_LINK CVisibleInstancesRI : public CVisibleInstances
	{
	public:
			// SForestInstancingData stores instance and billboard instancing 
			// information for a single base tree
			struct SForestInstancingData
			{
											SForestInstancingData( );
				
				const CTree*				m_pBaseTree;

				// mesh instancing support
				TInstancingMgrClass			m_t3dTreeInstancingMgr;
				TInstancingMgrClass			m_tBillboardInstancingMgr;
			};
			typedef CMap<const CTree*, SForestInstancingData*> TInstanceDataPtrMap;

											CVisibleInstancesRI(EPopulationType ePopulationType = POPULATION_TREES, st_bool bTrackNearestInsts = false/*LAVA++*/, Uint32 instanceRBSize = 0 );
											~CVisibleInstancesRI( );

			void							SetHeapReserves(const SHeapReserves& sHeapReserves);
			st_bool							InitGfx(const CArray<CTreeRI_t*>& aBaseTrees);
			void							Clear(void);
			void							ReleaseGfxResources(void);

			// update
			st_bool							PreUpdate3dTreeInstanceBuffers();
			st_bool							Update3dTreeInstanceBuffers(const CView& cView);
			void							PostUpdate3dTreeInstanceBuffers();
			st_bool							UpdateGrassInstanceBuffers(const CTreeRI_t* pBaseGrass, const Vec4 * const frustumPlanes, st_float32 cullingRadius /*LAVA++*/, CSpeedTreeInstanceRingBuffer& instanceRingBuffer);

			// billboard update
			st_bool							UpdateBillboardInstanceBuffers(SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree);
			st_bool							GetBaseTreeBillboardVboData(SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree);
			st_bool							CopyVboDataToGpu(const SBillboardVboBuffer& sBuffer, const CTreeRI_t* pBaseTree);
			st_int32						GetNumBillboardsCeiling();
			// LAVA++
			st_bool							LAVACopyBillboardInstanceDataToInstanceBufferDoneRight( const CTreeRI_t* pBaseTree, const Vec4 * const frustumPlanes, st_float32 cullingRadius );
			// LAVA--

			void							NotifyOfFrustumReset(void);
			void							NotifyOfPopulationChange(void);
			void							NotifyOfPopulationRemoval( st_int32 rowCol[4], const CTree* tree );
			void							NotifyOfPopulationAddition( st_int32 row, st_int32 col, const CTree* tree );

			const SForestInstancingData*	FindInstancingDataByBaseTree(const CTree* pTree) const;
			SForestInstancingData*			FindOrAddInstancingDataByBaseTree(const CTree* pTree);
			const TInstanceDataPtrMap&		GetPerBaseInstancingDataMap(void) const;
			// LAVA++
			CSpeedTreeInstanceRingBuffer&	GetInstanceRingBuffer() const { return m_instancingRingBuffer; }
			// LAVA--

	private:
			TInstanceDataPtrMap				m_mPerBaseInstancingDataMap;
			// LAVA++
			mutable CSpeedTreeInstanceRingBuffer	m_instancingRingBuffer;
			// LAVA--
	};

#define CVisibleInstancesRI_t CVisibleInstancesRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TInstancingMgrClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>


	///////////////////////////////////////////////////////////////////////  
	//  Class CForestRI

	const st_int32 c_nShadowSmoothingTableSize = 3;
	
	#define CForestRI_TemplateList template<class TStateBlockClass, class TTextureClass, class TGeometryBufferClass, class TInstancingMgrClass, class TShaderTechniqueClass, class TShaderConstantClass, class TShaderConstantBufferClass>

	CForestRI_TemplateList
	class ST_DLL_LINK CForestRI : public CForest
	{
	public:
											CForestRI( );
											~CForestRI( );
	
			// general graphics
			void							ReleaseGfxResources(void);
			void							SetRenderInfo(const SForestRenderInfo& sInfo);
			const SForestRenderInfo&		GetRenderInfo(void) const;
			st_bool							InitGfx(void);

			// main render functions
			st_bool							StartRender(void);
			st_bool							EndRender(void);
			st_bool							Render3dTrees(ERenderPass ePass, const CVisibleInstancesRI_t& cVisibleInstances, st_bool bRenderOpaqueMaterials = true) const;
			st_bool							RenderGrass(ERenderPass ePass, const CTreeRI_t* BaseGrass, const CVisibleInstancesRI_t& cVisibleInstances/*LAVA++*/, CSpeedTreeInstanceRingBuffer& instancingRingBuffer) const;
			st_bool							RenderBillboards(ERenderPass ePass, const CVisibleInstancesRI_t& sVisibleTrees) const;

			// constant buffers
			// LAVA: Removing "static" attribute
			TShaderConstantBufferClass&		GetFrameConstantBuffer(void); // todo: don't want to house these here
			SFrameCBLayout&					GetFrameConstantBufferContents(void); // todo: don't want to house these here

			SFogAndSkyCBLayout&				GetFogAndSkyBufferContents(void) const;
			st_bool							UpdateFrameConstantBuffer(const CView& cView, st_int32 nWindowWidth, st_int32 nWindowHeight);
			st_bool							UpdateFogAndSkyConstantBuffer(void);

	protected:
			// rendering
			SForestRenderInfo				m_sRenderInfo;
			Vec4							m_avShadowSmoothingTable[c_nShadowSmoothingTableSize];

	private:
											CForestRI(const CForestRI& cRight); // copying CForestRI disabled

			TTextureClass					m_tFizzleNoise;
			TTextureClass					m_tPerlinNoiseKernel;
			TTextureClass					m_tAmbientImageLighting;

			// todo: don't really want to house these here, do we? conflict of multiple forest objects together
			// we access needed in app classes
			// LAVA: Removing "static" attribute
			TShaderConstantBufferClass		m_cFrameConstantBuffer;
			SFrameCBLayout					m_sFrameConstantBufferLayout; // todo: rename to shadow or contents?
			TShaderConstantBufferClass		m_cFogAndSkyConstantBuffer;
	mutable	SFogAndSkyCBLayout				m_sFogAndSkyConstantBufferLayout;

			struct SDrawCallData
			{
				st_bool						operator<(const SDrawCallData& sRight) const;
				st_int64					GetHashKey(void) const;

				CTreeRI_t*					m_pBaseTree;
				st_int32					m_nLod;
				const CRenderStateRI_t*		m_pRenderState;
				const SDrawCall*			m_pDrawCall;
				const TGeometryBufferClass*	m_pGeometryBuffer;
				st_int32					m_nBufferOffset;
				const typename 
				CVisibleInstancesRI_t::
				SForestInstancingData*		m_pInstancingData;
				Float						m_closestInstance; // LAVA
			};
	mutable	CArray<SDrawCallData>			m_aSortedDrawCalls;
	};
	#define CForestRI_t CForestRI<TStateBlockClass, TTextureClass, TGeometryBufferClass, TInstancingMgrClass, TShaderTechniqueClass, TShaderConstantClass, TShaderConstantBufferClass>

	// include inline functions
	#include "ForestRI_inl.h"
	#include "RenderStateRI_inl.h"
	#include "TreeRI_inl.h"
	#include "MiscRI_inl.h"
	#include "RenderStats_inl.h"
	#include "InstancingManagerRI_inl.h"
	#include "VisibleInstancesRI_inl.h"

} // end namespace SpeedTree

#ifdef ST_SETS_PACKING_INTERNALLY
	#pragma pack(pop)
#endif

#include "Core/ExportEnd.h"
