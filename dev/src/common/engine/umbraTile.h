#pragma once
#include "../core/latentDataBuffer.h"
#include "../core/deferredDataBuffer.h"
#include "../core/resource.h"
#include "umbraStructures.h"

#ifdef USE_UMBRA

class CUmbraScene;
class CMesh;
class CJobCreateTomeForTile;

enum EUmbraTileDataStatus : Int32
{
	EUTDS_Unknown,
	EUTDS_NoData,
	EUTDS_Invalid,
	EUTDS_Valid,
	EUTDS_ComputationInProgress,
};

BEGIN_ENUM_RTTI( EUmbraTileDataStatus );
	ENUM_OPTION( EUTDS_Unknown );
	ENUM_OPTION( EUTDS_NoData );
	ENUM_OPTION( EUTDS_Invalid );
	ENUM_OPTION( EUTDS_Valid );
	ENUM_OPTION( EUTDS_ComputationInProgress );
END_ENUM_RTTI();

class CUmbraTile : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CUmbraTile, CResource, "w3occlusion", "Occlusion Data Tile" );

public:
#ifndef NO_UMBRA_DATA_GENERATION
#ifdef USE_UMBRA_COOKING
	struct VolumeParameterOverride
	{
		Uint32	m_volumeId;
		Float	m_smallestHoleValue;
	};

	struct RegenerationContext
	{
		VectorI					m_id;
		DeferredDataBuffer*		m_data;
		Umbra::Scene*			m_scene;
		Float					m_smallestHole;
		Float					m_smallestOccluder;
		Uint32					m_backFaceLimit;
		Float					m_tileSize;
		Vector					m_tileCorner;
		Int32					m_tilesPerEdgeHorizontally;
		Int32					m_tilesPerEdgeVertically;
		Box*					m_sceneBoundingBox;
		String					m_intermediateDir;
		Bool					m_forceRegenerate;
		TDynArray< VolumeParameterOverride >	m_volumeParameterOverrides;
	};

	struct ObjectInfo
	{
		Uint32	m_id;
		Matrix	m_transform;
		Box		m_boundingBox;			// world space
		Vector	m_boundingBoxCenter;
		Float	m_autoHideDistance;
		Uint8	m_flags;				// additional flags
	};
#endif // USE_UMBRA_COOKING
#endif // NO_UMBRA_DATA_GENERATION

public:
	CUmbraTile();
	~CUmbraTile();

	void									Initialize( CUmbraScene* parentScene, const VectorI& coordinates, Uint16 tileId );

public:
#ifndef NO_UMBRA_DATA_GENERATION
	RED_INLINE SComputationParameters		GetComputationParameters() const { return m_computationParameters; }
#endif //NO_UMBRA_DATA_GENERATION

	RED_INLINE void							SetStatus( EUmbraTileDataStatus status ) { m_dataStatus = status; }
	RED_INLINE EUmbraTileDataStatus			GetStatus() const { return m_dataStatus; }

	RED_INLINE void							SetParentScene( CUmbraScene* scene ) { m_parentScene = scene; }
	RED_INLINE Uint16						GetId() const { return m_tileId; }

	RED_INLINE void*						GetOcclusionData() const { return m_handle->GetData(); }
	RED_INLINE Uint32						GetOcclusionDataSize() const { return m_dataSize; }
	RED_INLINE void							OnFinishedLoading() { m_asyncDataHandle.Reset(); }

	RED_INLINE TTileObjectCache&			GetObjectCache() { return m_objectCache; }

	RED_INLINE const VectorI&				GetCoordinates() const { return m_coordinates; }

	RED_INLINE const Umbra::Tome*			GetTome() const { return m_tome; }
	RED_INLINE void							SetTome( const Umbra::Tome* tome, BufferHandle handle ) { m_tome = tome; m_handle = handle; }

	RED_INLINE Bool							IsInTomeCollectionJob() const { return m_isInTomeCollectionJob; }
	RED_INLINE void							SetInTomeCollectionJob( Bool isInJob ) { m_isInTomeCollectionJob = isInJob; }

	RED_INLINE Int32						GetNumberOfTomeCollections() const { return m_tomeRefCount.GetValue(); }

public:
	virtual void							OnSerialize( IFile& file );

	void									AddRefTome();
	void									ReleaseRefTome();

	void									RequestAsyncLoad( const BufferAsyncCallback& callback );

#ifndef NO_UMBRA_DATA_GENERATION
public:
#ifdef USE_UMBRA_COOKING
	Bool									AddMesh( const ObjectInfo& objectInfo, const CMesh* mesh, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddDecal( const ObjectInfo& objectInfo, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddDimmer( const ObjectInfo& objectInfo, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddStripe( const ObjectInfo& objectInfo, const TDynArray< Vector >& vertices, const TDynArray< Uint16 >& indices, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddPointLight( const ObjectInfo& objectInfo, const Float radius, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddSpotLight( const ObjectInfo& objectInfo, const Float radius, const Float innerAngle, const Float outerAngle, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddTerrain( const CClipMap* clipmap );
	Bool									AddObject( const Uint64 modelCacheId, const Matrix& transform, const Vector& bbRefPosition, TDynArray< UmbraObjectInfo >& addedObjects );
	Bool									AddWaypoint( const Vector& position );
	Bool									AddSmallestHoleOverride( const Box& boundingBox, const Float smallestHoleOverride );

private:
	void									UpdateBounds( const Box& worldSpaceBoundingBox );
#endif //USE_UMBRA_COOKING
public:	
	void									Clear();
	
#ifdef USE_UMBRA_COOKING
	Bool									GenerateTomeDataSync( STomeDataGenerationContext& context, Bool dumpScene, Bool dumpRawTomeData );
	Bool									DumpScene( const STomeDataGenerationContext& context );
	Bool									DumpTomeData( const STomeDataGenerationContext& context );
#endif //USE_UMBRA_COOKING

#ifdef USE_UMBRA_COOKING
	static Bool								Regenerate( const RegenerationContext& context );
#endif //USE_UMBRA_COOKING

	void									SaveGeneratedData();

#ifdef USE_UMBRA_COOKING
	Bool									InsertExistingObject( const UmbraObjectInfo& objectInfo );

	Bool									ShouldGenerateData() const;
	Uint32									GetTileDensity() const;
#endif //USE_UMBRA_COOKING

private:
	Bool									CalculateSceneBounds( STomeDataGenerationContext& context, Umbra::Vector3& volumeMin, Umbra::Vector3& volumeMax );
#endif //NO_UMBRA_DATA_GENERATION

public:
	Bool									HasData( Uint32* dataSize = nullptr ) const;

protected:
	Uint16									m_tileId;			// 14 bits are used
	Uint32									m_objectId;			// 18 bits are used
	VectorI									m_coordinates;
	CUmbraScene*							m_parentScene;
	Uint32									m_dataSize;

	DeferredDataBuffer						m_data;
	BufferHandle							m_handle;
	BufferAsyncDataHandle					m_asyncDataHandle;

	TTileObjectCache						m_objectCache;
	EUmbraTileDataStatus					m_dataStatus;

	const Umbra::Tome*						m_tome;

	Red::Threads::CAtomic< Int32 >			m_tomeRefCount;

	Bool									m_isInTomeCollectionJob;

#ifndef NO_UMBRA_DATA_GENERATION
#ifdef USE_UMBRA_COOKING
	Umbra::Scene*							m_scene;
	TDynArray< VolumeParameterOverride >	m_volumeParameterOverrides;
	Uint32									m_overriddenVolumeId;
#endif //USE_UMBRA_COOKING
	TModelCache								m_modelCache;
	
	Vector									m_bbMin;
	Vector									m_bbMax;
	Float									m_minZ;
	Float									m_maxZ;	
	
	SComputationParameters					m_computationParameters;
#endif //NO_UMBRA_DATA_GENERATION
};

BEGIN_CLASS_RTTI( CUmbraTile );
	PARENT_CLASS( CResource );
	PROPERTY( m_dataStatus );
	PROPERTY( m_data );
END_CLASS_RTTI();

#endif //USE_UMBRA
