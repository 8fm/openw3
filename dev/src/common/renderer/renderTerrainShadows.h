/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once


// Terrain shadow culling is not used right now.
//
// NOTE : If you enable this, you'll need to do something about the texture formats. Shadows Texture is
// using 16bit unorm, but the culling job assumes 32bit floats.
#define USE_TERRAIN_SHADOW_CULLING		0

//-----------------------------------------------------------------------------

class CRenderTerrainShadows;
class CJobCopyTerrainShadowBuffer;
class CJobTestShadowOcclusion;
class CRenderProxy_Terrain;
class CRenderTerrainShadowsPatchDrawer;

//-----------------------------------------------------------------------------

///dex++: shadow culling helper
class CRenderProxyShadowCulling
{
	DECLARE_CLASS_MEMORY_POOL( MemoryPool_SmallObjects, MC_RenderProxy );

private:
	//! Owner
	CRenderTerrainShadows*	m_terrainShadows;

	//!< Age of the culling buffer on which the last test was performed on
	Uint32		m_age;				

	//!< Last known bounding box of the proxy
	Box			m_worldBox;			

	//!< We have valid result
	Uint32		m_validResult:1;

	//! Is the shadow culled by the terrain shadow ?
	Uint32		m_shadowCulledByTerrain:1;

	//! Testing job 
	CJobTestShadowOcclusion*	m_job;

public:
	CRenderProxyShadowCulling( CRenderTerrainShadows* terrainShadows, const Box& initialBox );
	~CRenderProxyShadowCulling();

	// Update the bounding box of the culling proxy
	void UpdateBoundingBox( const Box& newBox );

	// Call this to see if the proxy has culled shadow
	bool IsShadowCulledByTerrain();
};
//dex--

//-----------------------------------------------------------------------------


///dex++: terrain shadows system
/*
	Terrain shadow system uses similar approach to the clipmap system.
	There are as many terrain shadow map levels as there are clipmap levels and 
	they roughly move in the same manner.
*/
class CRenderTerrainShadows
{
public:
	struct Window
	{
		// True if we have rendered at least once...
		bool		m_isValid;

		// Direction for which the shadow mask was calculated
		Vector		m_lightDirection;

		// The world region from which the clipmap was generated, only X and Y are valid
		Box			m_worldRect;

		Vector		m_validTexels;
	};

	class CullingBuffer
	{
	public:
		enum QueryResult
		{
			NotReady,
			Outdated,
			Culled,
			NotCulled,
		};

	private:
		// Data buffer
		const void*			m_mem;
		const Float*		m_data;

		// Age marker
		Red::Threads::CAtomic< Int32 >	m_age;
		volatile bool					m_updating;

		// Size of the data region ( in pixels )
		Uint32				m_size;

		// Size of the mapped area
		Box					m_rect;

		// Offset and scale to help convert world space position into pixels
		Float				m_offsetX;
		Float				m_offsetY;
		Float				m_scaleX;
		Float				m_scaleY;

	public:
		CullingBuffer( Uint32 size );
		~CullingBuffer();

		//! Get buffer age
		inline Uint32 GetAge() const { return m_age.GetValue(); }

		//! Check if given bounding box is in the shadow frustum
		QueryResult TestBox( const CTask& jobToken, Uint32 age, const Box& box ) const;

		//! Push new data (called from job)
		void GrabData( const CTask& jobToken, const void* srcData, Uint32 srcPutch, const Box& worldRect );
	};

private:

	//! For drawing terrain patches into the shadow depth buffer.
	CRenderTerrainShadowsPatchDrawer*	m_patchDrawer;

	//! Terrain proxy we are bound to
	CRenderProxy_Terrain*	m_terrainProxy;

	//! Scene we are bound to
	CRenderSceneEx*			m_terrainScene;

	//! Resolution of the terrain height map (per window)
	Uint32					m_terrainHeightResolution;

	//! Resolution of the terrain shadow map (per window)
	Uint32					m_terrainShadowResolution;

	//! Number of clipmap levels
	Uint32					m_clipmapLevels;

	//! Cached terrain clipamp texture
	GpuApi::TextureRef		m_terrainClipmapTexture;

	//! Texture with terrain shadow maps, it has the same structure
	GpuApi::TextureRef		m_shadowTexture;

	//! Preallocated texture used as a shadow map ( depth buffer )
	GpuApi::TextureRef		m_shadowDepthBuffer;

	//! Shadowmap texture, in ESRAM on xbox. On other platforms, this is the same as m_shadowDepthBuffer
	GpuApi::TextureRef		m_shadowDepthBufferEsram;

	//! World space bounding box ( only X and Y are valid ) for each level of shadow map
	TDynArray< Window >		m_shadowTextureWindows;

	//! World space bounding box ( only X and Y are valid ) for each level of terrain clipamp
	TDynArray< Window >		m_terrainTextureWindows;

	//! Cached terrain decompression factors
	Vector					m_terrainTextureRange;

	//! Full update request to all levels ( dirty flag )
	Bool					m_fullUpdateRequest;

	//! Whether terrain shadows are visible or not
	Bool					m_isVisible;
#if USE_TERRAIN_SHADOW_CULLING
	//! Texture used to transfer data from GPU to CPU ( for culling buffer )
	GpuApi::TextureRef		m_copyBuffer;

	//! We need to schedule an update for culling buffer because it may be outdated
	Bool					m_cullingBufferUpdateRequested;

	//! Data was copied to the staging buffer
	Bool					m_dataCopiedToStagingBuffer;

	//! Proxy culling buffer
	CullingBuffer*			m_cullingBuffer;

	//! Job for updating the culling buffer
	CJobCopyTerrainShadowBuffer*	m_cullingBufferUpdateJob;
#endif

public:
	//! Get the depth buffer (general shared use by HiRes shadows)
	inline GpuApi::TextureRef GetShadowDepthBuffer() const { return m_shadowDepthBuffer; }

	//! Get the shadow atlas texture
	inline GpuApi::TextureRef GetShadowTexture() const { return m_shadowTexture; }

	//! Get the terrain clipmap texture
	inline GpuApi::TextureRef GetTerrainTexture() const { return m_terrainClipmapTexture; }

	//! Get terrain texture compression factors ( scale, bias )
	inline const Vector& GetTerrainTextureRange() const { return m_terrainTextureRange; }

	//! Get the texture windows
	inline const TDynArray< Window >& GetShadowTextureWindows() const { return m_shadowTextureWindows; }

	//! Get the terrain clipmap windows
	inline const TDynArray< Window >& GetTerrainTextureWindows() const { return m_terrainTextureWindows; }

#if USE_TERRAIN_SHADOW_CULLING
	//! Get the proxy culling buffer
	inline const CullingBuffer& GetCullingBuffer() const { return *m_cullingBuffer; }
#endif

	//! Get isVisible
	inline Bool IsVisible(){ return m_isVisible; }

	//! Set visible
	inline void SetVisible( Bool isVisible ){ m_isVisible = isVisible; }

public:
	CRenderTerrainShadows();
	~CRenderTerrainShadows();

	//! Request full update of terrain shadows ( due to for example terrain painting )
	void RequestFullUpdate();

	//! Do we have valid data for given scene
	bool IsValidForScene( const CRenderSceneEx* scene ) const;

	//! Prepare shadows for given camera position and light direction
	void PrepareShadows( const CRenderCollector& collector );

	//! Bind to given terrain proxy
	void BindToTerrainProxy( CRenderProxy_Terrain* proxy, CRenderSceneEx* scene );

	//! Create a shadow culling helper
	class CRenderProxyShadowCulling* CreateShadowCullingProxy( const Box& initialBoundingBox );

private:
	//! Calculate lighting camera for given light position and clipmap level
	void CalculateSunCamera( const CRenderFrameInfo& frameInfo, Uint32 level, CRenderCamera& outSunCamera ) const;

	//! Prepare shadowmap depth buffer for calculation in given clipmap level
	// Camera used to calculate depth map is returned
	void PrepareShadowMap( const CRenderCollector& collector, const CRenderFrameInfo& frameInfo, Uint32 level, CRenderCamera& outSunCamera ) const;

	//! Draw terrain level using best possible LOD for that level
	// Terrain is rendered in proper world space so a valid camera is needed.
	// Only given terrain clipmap level is rendered.
	void DrawTerrainLevel( const CRenderCamera& camera, Uint32 level, const Box* excludeBox ) const;

	//! Refresh the terrain shadow level
	void RefreshLevel( const CRenderCollector& collector, const CRenderFrameInfo& frameInfo, Uint16 level );
};

//-----------------------------------------------------------------------------

#if USE_TERRAIN_SHADOW_CULLING

// Resource copy job for the shadow system
class CJobCopyTerrainShadowBuffer : public CTask
{
private:
	CRenderTerrainShadows::CullingBuffer*	m_target;
	const void*								m_srcData;
	Uint32									m_srcPitch;
	Box										m_worldRect;

public:
	CJobCopyTerrainShadowBuffer( const void* srcData, Uint32 srcPitch, const Box& worldRect, CRenderTerrainShadows::CullingBuffer*	target );

private:
	~CJobCopyTerrainShadowBuffer();

	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT("CopyTerrainShadowBuffer"); };

	//! Get debug color
	virtual Uint32 GetDebugColor() const { return Color::DARK_BLUE.ToUint32(); }

	//! Process the job, is called from job thread
	virtual void Run();
};

//-----------------------------------------------------------------------------

// Job to test if a given bounding box is occluded by terrain/mesh shadow
class CJobTestShadowOcclusion : public CTask
{
private:
	const CRenderTerrainShadows::CullingBuffer*		m_target;
	Uint32											m_age;
	Box												m_box;
	bool											m_result; // occlusion result

public:
	//! Get the result
	inline bool GetResult() const { return m_result; }

	//! Get the age this job was submited for
	inline Uint32 GetAge() const { return m_age; }

public:
	CJobTestShadowOcclusion( const CRenderTerrainShadows::CullingBuffer* target, const Box& box );

private:
	virtual ~CJobTestShadowOcclusion();

	//! Get short debug info
	virtual const Char* GetDebugName() const { return TXT("TestShadowOcclusion"); };

	//! Get debug color
	virtual Uint32 GetDebugColor() const { return Color::LIGHT_MAGENTA.ToUint32(); }

	//! Process the job, is called from job thread
	virtual void Run();
};

#endif

//-----------------------------------------------------------------------------
