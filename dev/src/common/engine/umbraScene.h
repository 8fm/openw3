#pragma once

#ifdef USE_UMBRA

#include "umbraJobs.h"
#include "umbraStructures.h"
#include "../core/resource.h"
#include "../engine/simplexTreeResource.h"

class IRenderObject;
class CClipMap;
class CDecalComponent;
class CMesh;
class CRenderCamera;
class CRenderFrame;
class CWayPointComponent;
class CStripeComponent;
class CSpotLightComponent;
class CPointLightComponent;

//////////////////////////////////////////////////////////////////////////
class CUmbraSmallestHoleOverrideComponent : public CBoundedComponent
{
	DECLARE_ENGINE_CLASS( CUmbraSmallestHoleOverrideComponent, CBoundedComponent, 0 )

private:
	static Box	m_unitBox;

public:
	CUmbraSmallestHoleOverrideComponent() : m_smallestHoleOverride( -1.0f ) { }

	RED_INLINE Float GetSmallestHoleOverride() const { return m_smallestHoleOverride; }

	// Update world space bounding box
	virtual void OnUpdateBounds();
	virtual void OnAttached( CWorld* world );
	virtual void OnDetached( CWorld* world );

	void OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag );

#ifdef USE_UMBRA
	virtual Bool ShouldBeCookedAsOcclusionData() const { return m_smallestHoleOverride > 0.0f; }
	virtual Bool OnAttachToUmbraScene( CUmbraScene* umbraScene, const VectorI& bounds );
#endif

private:
	Float		m_smallestHoleOverride;
};

BEGIN_CLASS_RTTI( CUmbraSmallestHoleOverrideComponent );
	PARENT_CLASS( CBoundedComponent );
	PROPERTY_EDIT( m_smallestHoleOverride, TXT("SmallestHoleOverride") );
END_CLASS_RTTI();
//////////////////////////////////////////////////////////////////////////

enum EComponentCollectionFlags
{
	CCF_TargetOnly						= FLAG( 0 ),	//!< This component should be inserted as target only ( eg. is on quest layer)
	CCF_Occluder						= FLAG( 1 ),	//!< This component should be inserted as occluder (if possible)
	CCF_Target							= FLAG( 2 ),	//!< This component should be inserted as target
	CCF_TwoSided						= FLAG( 3 ),	//!< This component is two sided
	CCF_VolumeOnly						= FLAG( 4 ),	//!< This component should be inserted as volume (use with care)
	CCF_Gate							= FLAG( 5 ),
	CCF_OverrideComputationParameters	= FLAG( 6 ),
};

#ifndef NO_UMBRA_DATA_GENERATION
struct UmbraMemoryStats 
{
	size_t	m_umbraPoolAllocatedMemory;
	size_t	m_tempBufferAllocatedMemoryPeak;
	size_t	m_tomeCollectionSize;
	size_t	m_tomeCollectionScratchAllocationPeak;
	Uint32	m_numberOfLoadedTiles;
	Vector	m_position;

	UmbraMemoryStats()
		: m_umbraPoolAllocatedMemory( 0 )
		, m_tempBufferAllocatedMemoryPeak( 0 )
		, m_tomeCollectionSize( 0 )
		, m_tomeCollectionScratchAllocationPeak( 0 )
		, m_numberOfLoadedTiles( 0 )
		, m_position( Vector::ZEROS )
	{}
};

struct ComponentWithUmbraData
{
	TDynArray< UmbraObjectInfo > umbraObjectInfo;
};

struct TileDependency
{
	VectorI src;
	VectorI dst;

	TDynArray< ComponentWithUmbraData > data;
};
#endif	// NO_UMBRA_DATA_GENERATION

struct TileSet
{
public:
	TileSet() 
		: m_tiles( nullptr )
	{ }

	RED_INLINE void Set( const TUmbraTileArray* tiles );
	RED_INLINE Bool IsLoading() const	{ return m_numLoading.GetValue() > 0;	}
	RED_INLINE void OnLoaded()			{ m_numLoading.Decrement();				}

	const TUmbraTileArray*				m_tiles;

private:
	Red::Threads::CAtomic< Int32 >		m_numLoading;
};

class CUmbraScene : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CUmbraScene, CResource, "w3occlusiondef", "Umbra Occlusion Scene" );

public:
	enum ETickState
	{
		TS_Idle,
		TS_StartLoadingTiles,
		TS_LoadTiles,
		TS_StartTomeCollectionGeneration,
		TS_GenerateTomeCollection,
	};

	enum ETileInsertResult
	{
		TIR_TileDetermined	= 0,
		TIR_OutsideGrid		= 1,
		TIR_OutsideBounds	= 2,
	};

public:
	static Float DEFAULT_UMBRA_DISTANCE_MULTIPLIER;

public:
	CUmbraScene();
	virtual ~CUmbraScene();

	void								Initialize( CWorld* world, Uint32 tilesCount, Float tileSize );
	void								Shutdown();

	static RED_INLINE Bool				IsUsingOcclusionCulling() { return g_useOcclusionCulling; }
	static RED_INLINE void				UseOcclusionCulling( Bool use ) { g_useOcclusionCulling = use; }

	virtual void						OnSerialize( IFile& file );

	RED_INLINE CUmbraTomeCollection*	GetTomeCollectionWrapper() const	{ return m_ping ? m_tomeCollectionPing : m_tomeCollection; }
	RED_INLINE TObjectCache&			GetObjectCache()					{ return m_objectCache; }
	RED_INLINE TOcclusionGrid&			GetGrid()							{ return m_grid; }
	RED_INLINE CUmbraTile*				GetTile( const VectorI& id ) const	{ const TOcclusionGrid::ElementList& elements = m_grid.GetElementsByCellIndex( id ); ASSERT( elements.Size() == 1 ); return elements[0].m_Data.Get(); }
	RED_INLINE Float					GetDistanceMultiplier() const		{ return m_distanceMultiplier; }
	RED_INLINE void						SetDistanceMultiplier( Float dm )	{ m_distanceMultiplier = dm; }
	RED_INLINE Bool						HasData() const						{ return m_hasData; }
	RED_INLINE Bool						IsRecreatingTomeCollection() const	{ if ( !m_tomeCollectionJob ) return false; else return !m_tomeCollectionJob->IsFinished(); }

	void								GenerateEditorFragments( CRenderFrame* frame );
	void								Tick( IRenderScene* renderScene );
	void								TickUntilStateOrIdle( IRenderScene* renderScene, ETickState state, Float maxTime );

	Int32								GetDifferenceInTileCount( const Vector& position ) const;
	Bool								GenerateTomeCollection( TDynArray< THandle< CUmbraTile > >& tomes );
	Bool								GenerateTomeCollection( const TOcclusionGrid::ElementList& elements );
	void								RecreateOcclusionDataInRenderScene( IRenderScene* renderScene, TVisibleChunksIndices& remapTable, TObjectIDToIndexMap& objectIDToIndexMap );
	void								RemoveUnusedTilesAndTomeCollection( Bool forceRemoveAll = false );
	void								MergeObjectCache( const TTileObjectCache& objectCache );
	void								RemoveFromObjectCache( const TTileObjectCache& objectCache );
	void								OnSerialize( IFile& file, const String& worldDirectory, const String& worldFileName );
	Bool								UpdateTomeCollection( const Vector& position, IRenderScene* renderScene );
	Bool								FindInCache( const TObjectCacheKeyType& key, TObjectIdType& objectId );
	Uint16								GetUniqueTileIdFromPosition( const Vector& position );
	void								CleanupActiveTomes( IRenderScene* renderScene );
	void								SetReferencePosition( const Vector& position );

protected:
	VectorI								GetTileIdFromPosition( const Vector& position ) const;

public:
	RED_INLINE const Float				GetTileSize() const								{ return m_tileSize; }
#ifndef NO_UMBRA_DATA_GENERATION
	RED_INLINE Box						GetBoundingBoxOfTile( const VectorI& id ) const { return m_grid.GetBoundingBoxOfTile( id ); } 
	RED_INLINE const Int32				GetTilesCount() const							{ return m_tilesCount; }
	RED_INLINE Bool						IsDuringSyncRecreation() const					{ return m_isDuringSyncRecreation; }
	RED_INLINE void						SetIsDuringSyncRecreation( Bool val )			{ m_isDuringSyncRecreation = val; }

	void								ClearTileData( const VectorI& tileId );
	void								ProcessDependencies( const VectorI& id );

#ifndef RED_FINAL_BUILD
public:
	Bool								GetMemoryStatsForPosition( const Vector& position, UmbraMemoryStats& stats );
#endif // RED_FINAL_BUILD

public:
	static Bool							ShouldAddComponent( const CComponent* component, Uint8* flags = nullptr );

#ifdef USE_UMBRA_COOKING
protected:
	void								InsertDependencies( const Box& worldSpaceBoundingBox, const Matrix& transform, const TDynArray< UmbraObjectInfo >& addedObjects );
	ETileInsertResult					DetermineTileToInsert( const Matrix& transform, const VectorI& bounds, THandle< CUmbraTile >& tile ) const;
	ETileInsertResult					DetermineTileToInsert( const Vector& position, const VectorI& bounds, THandle< CUmbraTile >& tile ) const;

public:
	Bool								AddMesh( const CMeshComponent* component, const VectorI& bounds, Uint8 flags );
	Bool								AddDecal( const CDecalComponent* component, const VectorI& bounds );
	Bool								AddTerrain( const VectorI& tileCoords, const CClipMap* clipmap );
	Bool								AddWaypoint( const Vector& position, const VectorI& bounds );
	Bool								AddDimmer( const CDimmerComponent* component, const VectorI& bounds );
	Bool								AddStripe( const CStripeComponent* component, const VectorI& bounds, const TDynArray< Vector >& vertices, const TDynArray< Uint16 >& indices, const Box& worldSpaceBBox );	
	Bool								AddPointLight( const CPointLightComponent* component, const VectorI& bounds );
	Bool								AddSpotLight( const CSpotLightComponent* component, const VectorI& bounds );
	Bool								AddSmallestHoleOverride( const CUmbraSmallestHoleOverrideComponent* component, const VectorI& bounds );

	Bool								GenerateTomeForTileSync( STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData );
	Bool								DumpTomeDataForTile( const STomeDataGenerationContext& context );
	Uint32								GetTileDensity( const VectorI& id );
	Bool								ShouldGenerateData( const VectorI& id ) const;
	Bool								InsertExistingObject( const VectorI& id, const UmbraObjectInfo& objectInfo );
#endif //USE_UMBRA_COOKING
	void								ClearTileDependencies() { m_tileDependencies.ClearFast(); }

protected:
	Bool								FindDependency( const VectorI& srcId, const VectorI& dstId, TileDependency*& dependency );

#endif	//!NO_UMBRA_DATA_GENERATION

	TOcclusionGrid						m_grid;
	TOcclusionGrid::ElementList			m_elements;
	Int32								m_tilesCount;
	Float								m_tileSize;
	CJobCreateTomeCollection*			m_tomeCollectionJob;
	Float								m_distanceMultiplier;
	THandle< CResourceSimplexTree >		m_localUmbraOccThresholdMul;
	IRenderObject*						m_renderOcclusionData;
	TObjectCache						m_objectCache;
	Bool								m_dataUploaded;
	Bool								m_hasData;
	Vector								m_referencePosition;
	Bool								m_referencePositionValid;
	ETickState							m_tickState;
	TileSet								m_tileSet;

	Bool								m_ping;
	CUmbraTomeCollection*				m_tomeCollection;
	TUmbraTileArray						m_tomesForTomeCollectionGeneration;

	CUmbraTomeCollection*				m_tomeCollectionPing;
	TUmbraTileArray						m_tomesForTomeCollectionGenerationPing;

#ifndef NO_UMBRA_DATA_GENERATION
	TDynArray< TileDependency >			m_tileDependencies;
	Bool								m_isDuringSyncRecreation;
#endif

public:
	static Red::Threads::CMutex			s_tomesNotificationAccessMutex;

private:
	static Bool							g_useOcclusionCulling;
};

BEGIN_CLASS_RTTI( CUmbraScene );
	PARENT_CLASS( CResource );
	PROPERTY_EDIT( m_distanceMultiplier,				TXT("The distance multiplier for umbra streaming distance") );
	PROPERTY_EDIT( m_localUmbraOccThresholdMul,		TXT("Umbra occlusion treshold multipliers areas.") );
END_CLASS_RTTI();

#endif // USE_UMBRA
