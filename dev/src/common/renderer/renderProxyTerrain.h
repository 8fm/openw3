/**
* Copyright © 2012 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/terrainTypes.h"
#include "../engine/terrainStructs.h"
#include "../engine/renderProxy.h"

#define TERRAIN_USE_TESSELATED_QUAD_DOMAIN

#define MAX_CLIPMAP_COUNT 16

#ifndef NO_EDITOR
	#ifdef USE_SPEED_TREE
		// Use this to enable / disable the world metrics collection
		#define ENABLE_SPEEDTREE_DENSITY_VISUALISER
	#endif
#endif

class CRenderSceneEx;
class RenderingContext;
class CRenderFrameInfo;
class CRenderQuery;
class CBuildQuadTreeTask;

class CTerrainTemplateMesh
{
public:
	TDynArray< GpuApi::SystemVertex_Terrain >		m_vertices;
	TDynArray< Uint16 >								m_indices;
	Uint32											m_numFaces;
	Uint32											m_vertexResolution;

	GpuApi::BufferRef								m_vertexBuffer;

	CTerrainTemplateMesh( Uint32 res, Float size );
	~CTerrainTemplateMesh();
};

class CSkirtTemplateMesh
{
public:
	TDynArray< GpuApi::SystemVertex_Terrain >	m_vertices;
	TDynArray< Uint16 >							m_indices;

	GpuApi::BufferRef							m_vertexBuffer;
	GpuApi::BufferRef							m_indexBuffer;

	CSkirtTemplateMesh( );
	~CSkirtTemplateMesh();
};

//////////////////////////////////////////////////////////////////////////

#define NUM_TERRAIN_TEXTURES_AVAILABLE		31	// Remember to set the corresponding flag on the engine side
#define MAX_AUTOMATIC_GRASS_TYPES			10

struct SClipmapWindow
{
	Box					m_worldRect;			//!< Space covered by this window
	Vector				m_validTexels;			//!< minXY, maxXY
	Bool				m_generateIntermediate;	//!< true if intermediate maps need regeneration

	SClipmapWindow()
		: m_worldRect( Vector::ZEROS, Vector::ZEROS )
		, m_validTexels( 0, 0, 0, 0 )
		, m_generateIntermediate( false )
	{}

	void SetValidTexels( const Rect& rect, Uint32 clipSize );
	Bool IsAllValid() const;
	Bool NeedsIntermediateMapsRegeneration() const { return m_generateIntermediate; }
	void MarkIntermediateMapsRegeneration( Bool flag ) { m_generateIntermediate = flag; }
};

struct STerrainPatchInstanceData
{
	Float	m_patchSize;
	Vector3	m_patchBias;
};

struct SSkirtPatchInstanceData
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );
	Vector	m_patchBias;
};

struct SQuadTreeNode
{
	Vector2 bias; //xy - bias
	Int32 level;
};

/* Encapsulating constant and texture buffers that describe the terrain clipmap allows easy reusing the same CBs and TBs in other systems (like water) */
class CRenderProxy_Terrain;

class CClipMapShaderData
{
	DECLARE_STRUCT_MEMORY_ALLOCATOR( MC_Engine );
public:
	struct STerrainParams	// Matches a "TerrainParams" constant buffer 
	{
		// float4
		Float	m_lowestElevation;
		Float	m_highestElevation;
		Float	m_mipmapLevelRes;
		Float	m_numLevels;
		// float4
		Float	m_terrainEdgeLength;
		Float	m_fullResolution;
		Float	m_interVertexSpace;
		Float	m_colormapStartingIndex;
		// float4
		Float	m_screenSpaceErrorThresholdNear;
		Float	m_screenSpaceErrorThresholdFar;
		Float	m_quadTreeRootNodeSize;
		Float	m_padding_for_W;
	};

	/* Size of the constant buffer describing textures */
	static const Int32	s_textureParamsCBSize = NUM_TERRAIN_TEXTURES_AVAILABLE * 2 * sizeof( Vector );	

protected:
	/* A constant buffer containing most important parameters describing the clipmap */
	STerrainParams		m_params;
	GpuApi::BufferRef	m_paramsCB;
	mutable Bool		m_paramsChanged;

	/* A texture buffer containing two arrays of vectors. First one specificifies world-space areas covered by clipwindows. Second one describes valid areas (in context of data streaming) */
	Vector				m_rectangles[ MAX_CLIPMAP_COUNT ];
	Vector				m_validTexels[ MAX_CLIPMAP_COUNT ];
	GpuApi::TextureRef	m_clipWindowsTB;
	mutable Bool		m_clipWindowschanged;
	
	/* Pairs (horizontal and vertical) of texture parameters (blend sharpness, slope based damp, fallof, etc.) 
	A constant buffer is used to send those to the GPU */
	Vector				m_textureParams[NUM_TERRAIN_TEXTURES_AVAILABLE][2];
	GpuApi::BufferRef	m_textureParamsCB;
	mutable Bool		m_textureParamsChanged;

public:
	CClipMapShaderData();
	~CClipMapShaderData();

	void Init( Uint32 numClipWindows );
	Bool WasInit() const;
	void Shutdown();

	/* Getters for editing */
	RED_FORCE_INLINE Vector&				GetRectangle( Int32 clipmapLevel )						{ return m_rectangles[ clipmapLevel ]; }
	RED_FORCE_INLINE Vector&				GetValidTexels( Int32 clipmapLevel )					{ return m_validTexels[ clipmapLevel ]; }
	RED_FORCE_INLINE Vector&				GetTextureParams( Int32 texIndex, Int32 paramIndex )	{ return m_textureParams[texIndex][paramIndex]; }
	RED_FORCE_INLINE STerrainParams&		GetTerrainParams()										{ return m_params; }

	//! Update m_params
	void UpdateTerrainParams( const CRenderProxy_Terrain* terrain );

	//! Update m_rectangles and m_validTexels
	void UpdateClipWindows( const CRenderProxy_Terrain* terrain );

	//! Update m_textureParams
	void UpdateTextureParams( const Vector* params );

	//! Bind cb/tb
	void BindTerrainParams( Int32 cbReg ) const;
	void BindTextureParams( Int32 cbReg ) const;
	void BindClipWindows( Int32 texReg, GpuApi::eShaderType shaderType ) const;

private:
	//! Update cb's and tb's with current data
	void UploadTerrainParams() const;
	void UploadTextureParams() const;
	void UploadClipWindows() const;
};

struct SPigmentDataConstants
{
	Vector pigmentWorldAreaScaleBias;
};

/// Terrain pigment data
struct STerrainPigmentData
{
	struct SState
	{
		SState ()
		{
			Reset();
		}

		void Reset()
		{
			m_areaCenter.Set4( 0.f, 0.f, 0.f, 0.f );
			m_areaSize.Set4( 0.f, 0.f, 0.f, 0.f );
			m_clipmapIndex = 0;
			m_colorClipmapIndex = 0;
		}

		Vector m_areaCenter;
		Vector m_areaSize;
		Uint32 m_clipmapIndex;
		Uint32 m_colorClipmapIndex;
	};

	STerrainPigmentData ();
	~STerrainPigmentData ();

	Bool IsInit();
	void Init( Uint32 resolution );
	void Release();
	void UpdateConstant();

	GpuApi::TextureRef		m_texture;
	SPigmentDataConstants	m_constants;
	SState m_state;
};

class CRenderTerrainUpdateData;
class CRenderGrassUpdateData;
struct SClipmapParameters;

/// Terrain chunk proxy
class CRenderProxy_Terrain : public IRenderProxy
{
public:
	struct SCustomDebugOverlay
	{
		GpuApi::TextureRef		ovarlayBitmap;	//!< Bitmap data
		Uint32					bitmaskWidth;	//!< Resolution
		Uint32					bitmaskHeight;	//!< Resolution
	};

protected:
	static const Bool					s_pregenNormals;				//!< Should precompute normal maps?

	GpuApi::TextureRef					m_heightmapArray;				//!< Clipmap stack of height data
	GpuApi::TextureRef					m_controlMapArray;				//!< Clipmap stack of mask data

#ifndef NO_HEIGHTMAP_EDIT
	GpuApi::TextureRef					m_heightmapStamp;				//!< Stamp heightmap
	GpuApi::TextureRef					m_colorStamp;					//!< Stamp color
	GpuApi::TextureRef					m_controlStamp;					//!< Stamp control
	GpuApi::TextureRef					m_grassMaskPreview;				//!< Preview texture for grass mask
#endif

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	GpuApi::TextureRef					m_grassDensityDebug;		//!< Grass density debug view
	Vector								m_grassDensityRect;			//!< World space rectangle covered by grass density debug texture map
	Bool								m_grassVisualiserEnable;	//!< Grass density debug view switch
#endif

	GpuApi::TextureRef					m_colorMapArray;				//!< Color map
	GpuApi::TextureRef					m_tesselationBlockErrorsArray;	//!< A map of tesselation block errors per clipmap level
	GpuApi::TextureRef					m_normalMapsArray;				//!< Precomputed vertex normals
	GpuApi::TextureRef					m_grassMap;						//!< A generic grass speedup map, saving the CPU from doing lots of computations
	GpuApi::TextureRef					m_grassMap_Staging;				//!< A staging texture for generic grass speedup map
	Uint32								m_windowResolution;				//!< Resolution of the clipmap window (clipsize)
	Uint16								m_numLevels;					//!< Number of clipmap levels

	TDynArray< SClipmapWindow >			m_clipmapWindows;				//!< A Clip map window per level
	TDynArray< SQuadTreeNode >			m_quadTreeStack;				//!< Quad tree nodes

	CTerrainTemplateMesh*				m_templateMesh;					//!< Template mesh, progressively tesselated and displaced by the gpu
	CSkirtTemplateMesh*					m_skirtTemplateMesh;			//!< Template mesh for skirt

	GpuApi::BufferRef					m_instanceData;					//!< Patch instancing buffer
	Int8* RESTRICT						m_instancedPtr;					//!< Instance buffer data allocation

	TDynArray< STerrainPatchInstanceData >	m_instances;				//!< Array of instances generated through quad tree transition
	TDynArray< SSkirtPatchInstanceData >	m_skirtInstances;			//!< Array of instances for skirt generated through quad tree transition

	STerrainPigmentData					m_pigmentData;

	Float								m_quadTreeRootNodeSize;			//!< Scale of a quad tree node for 0-clipmap level
	Float								m_lowestElevation;				//!< Value corresponding to 0 in the heightmap
	Float								m_highestElevation;				//!< Value corresponding to 65535 in the heightmap
	Float								m_elevationRange;				//!< Difference between lowest and highest elevation
	Float								m_screenSpaceErrorThresholdOverride;	//!< If >= 0, override the default near/far error thresholds with this value.
	Uint32								m_mipmapLevelRes;				//!< Resolution of the mipmap stack texture
	Float								m_terrainEdgeLength;			//!< Length (in meters) of the terrain edge
	Float								m_terrainEdgeLengthPowerOf2;	//!< Length (in meters) of the terrain edge rounded to be power of 2
	Vector								m_terrainCorner;				//!< Corner position of the terrain
	Int32								m_fullResolution;				//!< Resolution of the whole clipmap
	Float								m_interVertexSpace;				//!< Spacing between vertices on the highest detail level
	Int32								m_maxQuadTreeLevel;				//!< Maximum depth of the quad tree (precomputed based on the clipmap setup)
	Uint32								m_colormapStartingIndex;		//!< Index in the colormap array, where the data actually starts (index of the highest colormap mipmap)
	Int64								m_lastGrassMapStagingCopyIssuanceFrame;	//!< Last frame when a staging copy of the grass map was issued
	Bool								m_grassMapStagingCopyInProgress;		//!< Are we waiting for a grass map staging copy?

	Uint32								m_tilesPerEdge;					//!< Number of tiles in each edge of the terrain
	TDynArray< Vector2 >				m_tileHeightRange;				//!< Min/max terrain height for each tile (m_tilesPerEdge*m_tilesPerEdge)
	TDynArray< Float >					m_minWaterLevels;				//!< Min water level for each tile (m_tilesPerEdge*m_tilesPerEdge)

#ifndef NO_HEIGHTMAP_EDIT
	Vector								m_stampCenterAxis;			//!< World-space center and U axis of stamp. Assume that V axis is same length and perpendicular.
	//! Domain Shader settings for stamp.
	//!   X: stamp height scale
	//!   Y: stamp height offset
	//!   Z: stamp mode (0=replace, 1=add)
	Vector								m_stampVertexSettings;
	//! Pixel Shader settings for stamp.
	//!   X: whether color stamp is available
	//!   Y: whether control stamp is available
	//!   Z: size of original (unscaled) stamp data
	//!   W: size of stamp in world
	Vector								m_stampPixelSettings;
	Bool								m_renderStampVisualization; //!< On/off stamp rendering
#endif
#ifndef NO_EDITOR
	Bool								m_isEditing;				//!< Is terrain being edited? Disables certain updates.
#endif

	// ---
	// Constant buffers and texture buffers describing the clipmap itself. Used by both terrain and water rendering (as water is highly dependant on terrain)
	CClipMapShaderData					m_clipMapShaderData;
	// ---

	CRenderMaterial*					m_material;					//!< Material of the terrain

	Uint16*								m_level0Heightmap;			//!< Cached height data for 0th clipmap level
	TControlMapType*					m_level0ControlMap;			//!< Cached control data for 0th clipmap level
	TDynArray< SAutomaticGrassDesc >	m_grassSetup[ NUM_TERRAIN_TEXTURES_AVAILABLE ];
	Uint8*								m_grassMask;
	Uint32								m_grassMaskRes;

	struct SLWGrassSettings
	{
		Float			m_radiusScale;
		Float			m_radiusScaleSquare;
		Float			m_isOn;	// Bleh...

		SLWGrassSettings() 
			: m_radiusScale(1.0f)
			, m_radiusScaleSquare(1.0f)
			, m_isOn(0.0f) 
		{}

	}									m_lwGrassSetups[ MAX_AUTOMATIC_GRASS_TYPES * NUM_TERRAIN_TEXTURES_AVAILABLE ];

	struct SGrassIndexPair
	{
		RenderObjectHandle	m_grassResourceHandle;
		Uint32				m_indexAssigned;

		SGrassIndexPair()
			: m_indexAssigned( -1 )
		{}
	}									m_indexToGrassAssignments[ MAX_AUTOMATIC_GRASS_TYPES ];

	Uint8*								m_grassMapData;				//!< A grass map copied to the CPU mapped memory area
	
	Bool								m_requestIntermediateMapsUpdate;		//!< Set during render command execution. Fulfilled during rendering.
	Bool								m_requestGrassMapUpdate;
	Bool								m_requestPigmentMapUpdate;				//!< Set during render command execution. Fulfilled during rendering.

	Bool								m_requestGrassMapUpdateWhenDataExists;
	Bool								m_requestPigmentMapUpdateWhenDataExists;

	Uint32								m_pigmentMapUpdateClipmapIndex;			//!< Set during render command execution. Fulfilled during rendering.
	Uint32								m_pigmentMapUpdateColorClipmapIndex;	//!< Set during render command execution. Fulfilled during rendering.
	Uint32								m_pigmentMapNormalsClipmapIndex;
	Box									m_pigmentMapNormalsWindow;

#ifndef RED_FINAL_BUILD
	CRenderQuery*						m_pipelineStatsQuery;
#endif

	SCustomDebugOverlay*				m_customOverlay;			//!< DEBUG: A custom overlay to texture the terrain with. Allows easy preview for arbitrary masks covering the game world.

	Bool								m_visible;
	Bool								m_isColorMapCompressed;
	TQueue< CRenderTerrainUpdateData* >	m_updateDataPending;

	friend class CBuildQuadTreeTask;
	CBuildQuadTreeTask*					m_buildQuadTreeTask;

public:
	CRenderProxy_Terrain( const CRenderTerrainUpdateData* initData, const SClipmapParameters& clipmapParams );
	virtual ~CRenderProxy_Terrain();

	//! Initialize dx objects. Don't call from constructor (which is called from engine thread)
	void Init();

	RED_INLINE virtual void SetVisible( Bool visible ) { m_visible = visible; }
	RED_INLINE Bool			IsVisible(){ return m_visible; }

	void FlushTerrainUpdates();

	void Update( CRenderTerrainUpdateData* updateData );
	void Update( const CRenderGrassUpdateData* updateData );
	void UpdateMinimumWaterLevels( const TDynArray< Float >& minWaterLevels );
	void UpdateTileHeightRanges( const TDynArray< Vector2 >& tileHeightRanges );

	void Render( const RenderingContext& context, const CRenderFrameInfo& frameInfo, CRenderSceneEx* scene, MeshDrawingStats& stats );

	void StartBuildingQuadTree( const CRenderSceneEx* scene, const CRenderFrameInfo& frameInfo );
	void FinishBuildingQuadTree();

#ifndef NO_EDITOR
	void SetIsEditing( Bool isEditing ) { m_isEditing = isEditing; }
#endif

	void GetHeightRangeInArea( const Vector2& min, const Vector2& max, Float& outMinHeight, Float& outMaxHeight );

	RED_INLINE CRenderMaterial* GetMaterial() { return m_material; }

	// Get current material, or material from a pending update if there is one.
	CRenderMaterial* GetNewestMaterial();

	// global ocean shader access hack
	RED_INLINE	GpuApi::TextureRef					GetHeightMapArray( void ) const { return m_heightmapArray; }
	RED_INLINE	GpuApi::TextureRef					GetNormalMapsArray( void ) const { return m_normalMapsArray; }
	RED_INLINE	const CClipMapShaderData&			GetClipMapData( void ) const { return m_clipMapShaderData; };

	RED_INLINE const TDynArray< SClipmapWindow >& GetClipmapWindows() const { return m_clipmapWindows; }
	RED_INLINE Float		GetLowestElevation()			const { return m_lowestElevation; }
	RED_INLINE Float		GetHighestElevation()			const { return m_highestElevation; }
	RED_INLINE Float		GetTerrainEdgeLength()			const { return m_terrainEdgeLength; }
	RED_INLINE Uint32		GetMipmapLevelRes()				const { return m_mipmapLevelRes; }
	RED_INLINE Uint32		GetNumLevels()					const { return m_numLevels; }
	RED_INLINE Int32		GetFullResolution()				const { return m_fullResolution; }
	RED_INLINE Float		GetInterVertexSpace()			const { return m_interVertexSpace; }
	RED_INLINE Uint32		GetColormapStartingIndex()		const { return m_colormapStartingIndex; }
	RED_INLINE Float		GetQuadTreeRootNodeSize()		const { return m_quadTreeRootNodeSize; }
	Float					GetScreenspaceErrorThresholdNear()	const;
	Float					GetScreenspaceErrorThresholdFar()	const;

	RED_INLINE const STerrainPigmentData& GetPigmentData() const { return m_pigmentData; }
	RED_INLINE Uint32 GetPigmentMapClipmapIndex() const { return m_pigmentMapUpdateClipmapIndex; }
	RED_INLINE Uint32 GetPigmentMapNormalsClipmapIndex() const { return m_pigmentMapNormalsClipmapIndex; }
	RED_INLINE const Box& GetPigmentMapNormalsWindow() const { return m_pigmentMapNormalsWindow; }

	RED_INLINE void SetScreenSpaceErrorThresholdOverride( Float value ) { m_screenSpaceErrorThresholdOverride = value; }

	void BindConstantsAndSamplers( const RenderingContext& context );
	Bool BindMaterialForHolesSkippingPass( const RenderingContext& context, const CRenderFrameInfo& frameInfo, EMaterialVertexFactory vertexFactory, Bool lowQuality );
	Bool BindMaterialForHolesOnlyPass( const RenderingContext& context, const CRenderFrameInfo& frameInfo );

	void RenderIntermediateMaps( CRenderSceneEx* scene );

	void GetClipmap0Params( Box& worldRect, Uint32& windowResolution, Float& minElevation, Float& maxElevation, Float& interVertexSpace );

	Float GetUnfilteredHeightAtPos( const Vector2& uv );

	// Check whether the data is complety for automatic grass cover generation
	Bool IsDataReadyForGrassGeneration( Int64 frameIndex );

	Int32 GetIndexForGrassType( IRenderObject* object );

	RED_FORCE_INLINE Bool GetHeightParamsAtPos( const Vector2& uv, Uint32 grassResIndex, Float& outHeight, Float& outScale, Float& numTriesConsumed )
	{
		const Vector& validTexels = m_clipmapWindows[0].m_validTexels;
		const Vector2 validatedUV = ( uv * ( (*(const Vector2*)(((const Float*)&validTexels)+2)) - validTexels.AsVector2()  ) + validTexels.AsVector2() );

		// Compute clipmap window texture coords
		const Float xCoordF = validatedUV.X * m_windowResolution;
		const Float yCoordF = validatedUV.Y * m_windowResolution;

		const Int32 xCoord = (Int32)xCoordF;
		const Int32 yCoord = (Int32)yCoordF;

		// Compute height with bilinear interpolation
		const Int32 xCoord_right = Min( xCoord + 1, static_cast< Int32 >( m_windowResolution ) - 1 );
		const Int32 yCoord_down = Min( yCoord + 1, static_cast< Int32 >( m_windowResolution ) - 1 );

		// Sample control map value
		const TControlMapType controlMapVal = m_level0ControlMap[ yCoord * m_windowResolution + xCoord ];

		// Fetch horizontal texture index
		const Uint16 actualHorizontalTexIndex = ( controlMapVal & 31 ) - 1;
		const Uint16 actualVerticalTexIndex = ( ( controlMapVal >> 5 ) & 31 ) - 1;

		// If control map was 0 here, this value will blow up (uint wrap). Making a check like this saves another if for checking against invalid data.
		if ( actualHorizontalTexIndex + actualVerticalTexIndex > 0xff )
		{
			// Terrain hole here, early quit
			numTriesConsumed += 1.0f;
			return false;
		}

		const SLWGrassSettings& hDescriptor = m_lwGrassSetups[ grassResIndex * NUM_TERRAIN_TEXTURES_AVAILABLE + actualHorizontalTexIndex ];
		const SLWGrassSettings& vDescriptor = m_lwGrassSetups[ grassResIndex * NUM_TERRAIN_TEXTURES_AVAILABLE + actualVerticalTexIndex ];

		const Float fade = m_grassMapData[ yCoord * m_windowResolution + xCoord ] / 256.0f;
		const Float bothDescriptorsPresent = hDescriptor.m_isOn * vDescriptor.m_isOn;

		outScale = hDescriptor.m_isOn * ( 1.0f - fade ) + vDescriptor.m_isOn * fade;

		numTriesConsumed += ( 1.0f - bothDescriptorsPresent )	* hDescriptor.m_radiusScaleSquare * vDescriptor.m_radiusScaleSquare
									+ bothDescriptorsPresent	* hDescriptor.m_radiusScale * vDescriptor.m_radiusScale;

		if ( outScale >= 0.2f )
		{
			// Calculate height of the instance
			const Float xFrac = xCoordF - xCoord;
			const Float yFrac = yCoordF - yCoord;

			// Calculate heights
			const Uint16 heightmapValLeftTop = m_level0Heightmap[ yCoord * m_windowResolution + xCoord ];
			const Uint16 heightmapValLeftBottom = m_level0Heightmap[ yCoord_down * m_windowResolution + xCoord ];
			const Uint16 heightmapValRightTop = m_level0Heightmap[ yCoord * m_windowResolution + xCoord_right ];
			const Uint16 heightmapValRightBottom = m_level0Heightmap[ yCoord_down * m_windowResolution + xCoord_right ];

			const Float heightLeftTop = m_lowestElevation + m_elevationRange * ( (Float)heightmapValLeftTop / 65536.f );
			const Float heightLeftBottom = m_lowestElevation + m_elevationRange * ( (Float)heightmapValLeftBottom / 65536.f );
			const Float heightRightTop = m_lowestElevation + m_elevationRange * ( (Float)heightmapValRightTop / 65536.f );
			const Float heightRightBottom = m_lowestElevation + m_elevationRange * ( (Float)heightmapValRightBottom / 65536.f );

			const Float xTop = heightLeftTop + (heightRightTop - heightLeftTop) * xFrac;
			const Float xBottom = heightLeftBottom + (heightRightBottom - heightLeftBottom) * xFrac;
			outHeight = xTop + (xBottom - xTop) * yFrac;

			return true;
		}
		else
		{
			// No instance here
			return false;
		}
	}


	const SAutomaticGrassDesc* GetGrassDescriptor( Int32 texIndex, IRenderObject* object ) const;

	void UpdateGrassMask( Uint8* grassMaskUpdate, Uint32 grassMaskResUpdate );
	RED_INLINE Bool IsGrassMaskedOut( Float worldX, Float worldY )
	{
		if ( m_grassMask == NULL ) return false;

		const Float xNorm = ( worldX - m_terrainCorner.X ) / m_terrainEdgeLength;
		const Float yNorm = ( worldY - m_terrainCorner.Y ) / m_terrainEdgeLength; 

		const Int32 maskCol = Int32( xNorm * m_grassMaskRes );
		const Int32 maskRow = Int32( yNorm * m_grassMaskRes );

		const Int32 bitIndex = ( maskRow * m_grassMaskRes + maskCol );

		return ( ( m_grassMask[ bitIndex / 8 ] & ( 1 << ( (bitIndex) % 8 ) ) ) == 0 );
	}

#ifdef ENABLE_SPEEDTREE_DENSITY_VISUALISER
	void SetGrassDensityVisualisationTexture( GpuApi::TextureRef tex, const Vector& rect, Bool visualisationEnabled );
#endif

	//! Set custom color overlay for the terrain to be rendered with. RGBA data, when A == 0 then terrain will have
	//! it's normal color, otherwise will be colored according to the overlay. The overlay covers the full area of the
	//! final clipmap window, with no filtering.
	void SetCustomBitmaskOverlay( Uint32 width, Uint32 height, Uint32* data );

private:
	void RenderPigmentMap( Uint32 clipmapIndex, Uint32 colorClipmapIndex );
	void RenderGrassMap( CRenderSceneEx* scene );
	void ReadBackGrassMapMemoryFromStaging();
	void OptimizeGrassSetup();
	Int32 AssignIndexToGrassType( IRenderObject* grassType );
	void PerformTerrainUpdate( CRenderTerrainUpdateData* updateData );

	void PerformTerrainUpdateTileComplete( SClipmapLevelUpdate* update );
	void PerformTerrainUpdateTileIncomplete( SClipmapLevelUpdate* update );
	void PerformTerrainUpdateColorTileComplete( SClipmapLevelUpdate* update );
	void PerformTerrainUpdateColorTileIncomplete( SClipmapLevelUpdate* update );
};

class CBuildQuadTreeTask : public CTask
{
public:
	CBuildQuadTreeTask( CRenderProxy_Terrain* renderProxy, 
#ifdef USE_UMBRA
		const CRenderOcclusionData* occlusionData, 
#endif // USE_UMBRA
		const CRenderFrameInfo& frameInfo, Bool shouldCollectWithUmbra );
	virtual ~CBuildQuadTreeTask();

#ifndef NO_DEBUG_PAGES
	virtual const Char* GetDebugName() const override { return TXT("CBuildQuadTreeTask"); }
	virtual Uint32 GetDebugColor() const override { return COLOR_UINT32( 23, 233, 250 ); }
#endif

	void BuildQuadTree();
	virtual void Run() override;
	
	Vector							m_cameraPosition;

	Bool							m_cullTerrainWithUmbra;
	Bool							m_showSkirt;
	Bool							m_renderDeepTerrain;
	Bool							m_cullFullHeight;

	CRenderProxy_Terrain*			m_renderProxy;
#ifdef USE_UMBRA
	const CRenderOcclusionData*		m_occlusionData;
#endif // USE_UMBRA
	CFrustum*						m_frustum;
};