#pragma once

#include "pathlib.h"
#include "pathlibAreaNavgraphs.h"
#include "pathlibCollectCollisionPointsSpatialQuery.h"
#include "pathlibResPtr.h"
#include "pathlibSpatialQuery.h"
#include "pathlibStreamingItem.h"
#include "pathlibGenerationManagerBase.h"


class CPathLibNavGraphRes;
class CSimpleBufferWriter;
class CSimpleBufferReader;

namespace PathLib
{

class CNavGraph;
class CTerrainMap;
class CNavmeshAreaDescription;
class CNavmeshTransformedAreaDescription;
class CTerrainAreaDescription;
class CAreaGenerationJob;
struct CObstacleSpawnContext;
class CNavModyficationMap;

////////////////////////////////////////////////////////////////////////////
class CAreaDescription : public IStreamingItem
{
	friend class CPathLibNavGraphRes;										// navgraph resource has direct access to area description internals
	friend class CAreaStreamingRequest;
public:
	static const Uint16 RES_VERSION = 5;

	enum EType
	{
		TYPE_TERRAIN,
		TYPE_INSTANCE_INWORLD,
		TYPE_INSTANCE_TRANSFORMED
	};

	enum EFlags
	{
		FLAG_NoObstaclesMap						= FLAG(0),
		FLAG_Blocked							= FLAG(1),
		FLAG_UseExternalNavmesh					= FLAG(2),
		FLAG_IsOriginallyCooked					= FLAG(3),

		FLAGS_Default							= FLAG_NoObstaclesMap,
		FLAGS_Saveable							= FLAG_NoObstaclesMap | FLAG_Blocked | FLAG_UseExternalNavmesh | FLAG_IsOriginallyCooked,
	};

	enum EDirtyFlags
	{
		////////////////////////////////////////////////////////////////////
		// Internal tasks
		DIRTY_TASK_SURFACE						= FLAG(0),					// surface was modified - recheck if it modified collision data
		DIRTY_TASK_CHECK_CONSISTENCY			= FLAG(1),					// recheck consistency (is data valid)
		DIRTY_TASK_CONNECT_WITH_NEIGHBOURS		= FLAG(2),					// area connection to neighbours
		DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION	= FLAG(3),					// reset neighbours connection
		DIRTY_TASK_GENERATE						= FLAG(4),					// navgraph generation it requires to reconnect it with neighbours
		DIRTY_TASK_COLLISIONDATA				= FLAG(5),					// collision data was directly modified check if it effected graph
		DIRTY_TASK_SAVE							= FLAG(6),					// save navgraph
		DIRTY_TASK_CALCULATE_NEIGHBOURS			= FLAG(7),					// neighbour list recalculation
		DIRTY_TASK_RECALCULATE_OBSTACLES		= FLAG(8),
		DIRTY_TASK_REMARK_INSTANCES				= FLAG(9),

		////////////////////////////////////////////////////////////////////
		// External modification flags
		DIRTY_ALL								= DIRTY_TASK_SURFACE | DIRTY_TASK_COLLISIONDATA | DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION | DIRTY_TASK_CONNECT_WITH_NEIGHBOURS | DIRTY_TASK_GENERATE | DIRTY_TASK_RECALCULATE_OBSTACLES | DIRTY_TASK_REMARK_INSTANCES,
		DIRTY_SURFACE							= DIRTY_TASK_SURFACE,									
		DIRTY_CHECK_CONSISTENCY					= DIRTY_TASK_CHECK_CONSISTENCY | DIRTY_SURFACE,									
		DIRTY_CONNECT_WITH_NEIGHBOURS			= DIRTY_TASK_CONNECT_WITH_NEIGHBOURS,									
		DIRTY_CLEAR_NEIGBOURS_CONNECTION		= DIRTY_TASK_CLEAR_NEIGHBOURS_CONNECTION | DIRTY_CONNECT_WITH_NEIGHBOURS,	
		DIRTY_GENERATE							= DIRTY_TASK_GENERATE | DIRTY_CLEAR_NEIGBOURS_CONNECTION | DIRTY_TASK_CALCULATE_NEIGHBOURS,	
		DIRTY_COLLISIONDATA						= DIRTY_TASK_COLLISIONDATA,
		DIRTY_SAVE								= DIRTY_TASK_SAVE,
		DIRTY_CALCULATE_NEIGHBOURS				= DIRTY_TASK_CALCULATE_NEIGHBOURS | DIRTY_TASK_CONNECT_WITH_NEIGHBOURS,
		DIRTY_RECALCULATE_OBSTACLES				= DIRTY_TASK_RECALCULATE_OBSTACLES | DIRTY_GENERATE,
		DIRTY_REMARK_INSTANCES					= DIRTY_TASK_REMARK_INSTANCES,
		////////////////////////////////////////////////////////////////////
		// Utility flags
		DIRTY_MASK_SERIALIZABLE					= DIRTY_TASK_SURFACE | DIRTY_TASK_COLLISIONDATA | DIRTY_TASK_GENERATE | DIRTY_TASK_REMARK_INSTANCES,
		DIRTY_MASK_BLOCKSTREAMOUT				= DIRTY_MASK_SERIALIZABLE | DIRTY_SAVE,
	};
	enum EInfo
	{
		SPATIALTESTS_IN_LOCAL_SPACE				= false
	};

	typedef AreaId Id;
	typedef Uint8 AreaFlags;
	static const Id INVALID_ID = INVALID_AREA_ID;
	static const Id ID_MASK_TERRAIN = AREA_ID_MASK_TERRAIN;
	static const Id ID_MASK_INDEX = 0x7fff;

	// pre-loading constructor
	CAreaDescription();
	// runtime constructor
	CAreaDescription( CPathLibWorld& pathlib, Id id );
	virtual ~CAreaDescription();

	CPathLibWorld&		GetPathLib() const									{ return *m_pathlib; }
	Id					GetId() const										{ return m_id; }
	virtual void		SetPathLib( CPathLibWorld* pathlib );

	void				LocalToWorld( Box& v ) const						{ }
	void				WorldToLocal( Box& v ) const						{ }
	void				LocalToWorld( Vector3& v ) const					{ }
	void				WorldToLocal( Vector3& v ) const					{ }
	void				LocalToWorld( Vector2& v ) const					{ }
	void				WorldToLocal( Vector2& v ) const					{ }
	Float				LocalToWorldZ( Float z ) const						{ return z; }
	Float				WorldToLocalZ( Float z ) const						{ return z; }

	virtual void		VLocalToWorld( Box& v ) const;						// no transformation by default
	virtual void		VWorldToLocal( Box& v ) const;
	virtual void		VLocalToWorld( Vector3& v ) const;
	virtual void		VWorldToLocal( Vector3& v ) const;
	virtual void		VLocalToWorld( Vector2& v ) const;
	virtual void		VWorldToLocal( Vector2& v ) const;
	virtual Float		VLocalToWorldZ( Float z ) const;
	virtual Float		VWorldToLocalZ( Float z ) const;

	virtual Bool		VSpatialQuery( CCircleQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CClosestObstacleCircleQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CCollectCollisionPointsInCircleQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CLineQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CWideLineQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CClosestObstacleWideLineQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CClearWideLineInDirectionQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CRectangleQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CCustomTestQueryData& query ) const = 0;
	virtual Bool		VSpatialQuery( CCollectGeometryInCirceQueryData& query ) const = 0;

	template < class TQuery >
	Bool				VLocalSpaceSpatialQuery( TQuery& query ) const;
	template < class TQuery >
	Bool				TMultiAreaQuery( TQuery& query ) const;
	template < class TArea, class TQuery >
	Bool				TSpatialQuery( TQuery& query ) const;

	virtual Bool		VContainsPoint( const Vector3& v ) const = 0;
	virtual Bool		VTestLocation( const Vector3& v1, Uint32 collisionFlags ) = 0;
	virtual Bool		VComputeHeight( const Vector3& v, Float& z, Bool smooth = false ) = 0;
	virtual Bool		VComputeHeight( const Vector2& v, Float minZ, Float maxZ, Float& z, Bool smooth = false ) = 0;
	virtual Bool		VComputeHeightFrom(const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) = 0;
	virtual Bool		VComputeAverageHeight( const Box& bbox, Float& zAverage, Float& zMin, Float& zMax ) = 0;
	

	// IStreamingItem interface - loading
	void				PreLoad() override;
	void				Load() override;
	void				PostLoad() override;
	void				PostLoadInterconnection() override;
	void				Attach( CStreamingManager* manager ) override;
	// IStreamingItem interface - unloading
	void				PreUnload() override;
	void				Unload() override;
	void				Detach( CStreamingManager* manager ) override;

	// custom load/unload code
	virtual void		OnPostLoad();
	virtual void		OnPreUnload();


	Bool				Save( CDirectory* dir, Bool onlyChanged = false );

	void				SetUsedCategories( CategoriesBitMask c )			{ m_usedCategories = c; }
	static Bool			IsUsingCategory( Uint32 category, CategoriesBitMask bitMask ) { return (( 1 << category ) & bitMask) != 0; }
	Bool				IsUsingCategory( Uint32 category ) const			{ return IsUsingCategory( category, m_usedCategories ); }
	Uint16				IsDirty() const										{ return m_isDirty; }
	Bool				IsProcessing() const								{ return m_isProcessed.GetValue(); }
	Bool				IsReady() const										{ return IsLoaded() && !m_isProcessed.GetValue(); }
	void				StartProcessing()									{ m_isProcessed.SetValue( true ); }			// dirty flag is cleared synchronously in pre-processing
	void				EndProcessing()										{ m_isProcessed.SetValue( false ); }		// when processing task is finished and area description is again usable
	Bool				CanModifyData()										{ return !m_isProcessed.GetValue(); }
	Bool				IsFullyCooked() const								{ return (m_areaFlags & FLAG_IsOriginallyCooked) != 0; }
	void				MarkNotFullyCooked()								{ m_areaFlags &= ~FLAG_IsOriginallyCooked; }
	void				MarkDirty( Uint32 flags, Float generationDelay = -1.f );

	EngineTime			GetProcessingDelay() const							{ return m_generationDelay; }
	virtual void		VPrecomputeObstacleSpawnData( CObstacleSpawnContext& context );

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	Bool				SyncProcessing( IGenerationManagerBase::CAsyncTask** outJob = NULL, Bool forceSynchronous = false );
	Bool				AsyncProcessing( CAreaGenerationJob* job );
	void				DescribeProcessingTasks( String& description )		{ DescribeProcessingTasks( description, Uint32( m_isDirty ) ); }
	void				DescribeProcessingTasks( String& description, Uint32 flags );
	virtual void		Describe( String& description ) = 0;

	void				GenerateAsync( CAreaGenerationJob* job, CategoriesBitMask bitMask )			{ m_usedCategories = bitMask; GenerateAsync( job ); }
	virtual void		GenerateAsync( CAreaGenerationJob* job ) = 0;
	virtual void		GenerateSync() = 0;
	virtual Bool		PreGenerateSync();
	virtual void		PostGenerationSyncProcess();
	virtual Bool		CorrectNeighbourList();
	virtual void		ClearNeighboursConnection();

	void				PreCooking();
	void				PostCooking();
#endif

	CNavGraph*			GetNavigationGraph( Uint32 i )						{ CAreaNavgraphsRes* res = m_graphs.Get(); return res ? res->GetGraph( i ) : NULL; }


	virtual void		ConnectWithNeighbours();
	void				DetachFromNeighbours();
	virtual void		Initialize();
	virtual void		OnRemoval();

	Bool				IsNavgraphsModified() const;

	CNavModyficationMap* LazyInitializeModyficationSet();
	CObstaclesMap*		LazyInitializeObstaclesMap();
	Bool				SaveObstaclesMap();
	Bool				LoadObstaclesMap( Bool async );

	Bool				IsNavmeshArea() const								{ return (m_id & ID_MASK_TERRAIN) == 0; }
	Bool				IsTerrainArea() const								{ return (m_id & ID_MASK_TERRAIN) != 0; }

	EType							GetType() const;
	const Box&						GetBBox() const							{ return m_bbox; }
	CAreaNavgraphsRes*				GetNavgraphs() const					{ return m_graphs.Get(); }
	CObstaclesMap*					GetObstaclesMap() const;
	CNavModyficationMap*			GetMetalinks() const;
	virtual Float					GetMaxNodesDistance() const				= 0;

	RED_INLINE CNavmeshTransformedAreaDescription* AsTransformedNavmeshArea();
	RED_INLINE CNavmeshAreaDescription* AsNavmeshArea();
	RED_INLINE CTerrainAreaDescription* AsTerrainArea();

	RED_INLINE const CNavmeshTransformedAreaDescription* AsTransformedNavmeshArea() const;
	RED_INLINE const CNavmeshAreaDescription* AsNavmeshArea() const;
	RED_INLINE const CTerrainAreaDescription* AsTerrainArea() const;


	static Bool			ReadIdFromBuffer( CSimpleBufferReader& reader, EType& areaType, Id& areaId );
	static CAreaDescription*	NewFromBuffer( CSimpleBufferReader& reader );
	virtual void		WriteToBuffer( CSimpleBufferWriter& writer );
	virtual	Bool		ReadFromBuffer( CSimpleBufferReader& reader );


	// temporary stuff
	static CAreaDescription*	NewFromBuffer_Legacy( CSimpleBufferReader& reader );

	

	// Load & Unload shouldn't be accessed directly (only via streamer)
	virtual void		GetAreaFileList( TDynArray< CDiskFile* >& fileList, Bool onlyChanged = false );

	struct ResourceFunctor : public Red::System::NonCopyable
	{
		virtual Bool	Handle( CResPtr& ptr, ENavResType resType ) = 0;
	};
	virtual Bool		IterateAreaResources( ResourceFunctor& functor );

protected:
	template < class TArea, class TQuery >
	static Bool			TMultiAreaQueryInternal( const TArea& area, TQuery& query );

	template < class SaverFunctor >
	RED_INLINE Bool	SaveResource( SaverFunctor& functor );
	template < class LoaderFunctor >
	RED_INLINE Bool	LoadResource( LoaderFunctor& functor, Bool async );


#ifndef NO_EDITOR_PATHLIB_SUPPORT
	virtual Bool		ProcessDirtySurface( IGenerationManagerBase::CAsyncTask** outJob );
	virtual Bool		ProcessDirtyCollisionData();
	virtual Bool		RemarkInstances();
#endif
	
	void				GatherNeighbourAreas( TSortedArray< AreaId >& outAreas ) const;

	virtual void		Clear();

	typedef Red::Threads::CAtomic< Bool > AtomicBool;
	typedef Red::Threads::CAtomic< Int32 > AtomicInt;

	CAreaNavgraphsResPtr	m_graphs;
	CPathLibWorld*			m_pathlib;
	Id						m_id;
	AreaFlags				m_areaFlags;
	CategoriesBitMask		m_usedCategories;

	Uint16					m_isDirty;
	AtomicBool				m_isProcessed;

	CObstaclesMapResPtr		m_obstaclesMap;

	EngineTime				m_generationDelay;

	Box						m_bbox;
};

};		// namespace PathLib

