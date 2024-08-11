#pragma once

#include "pathlib.h"
#include "pathlibTerrainInfo.h"
#include "pathlibSearchEngine.h"
#include "pathlibSettings.h"
#include "pathlibSpatialQuery.h"
#include "pathlibWorldLayersMapping.h"
#include "pathlibInstanceMap.h"

namespace PathLib
{
	class IComponent;
	class CAreaDescription;
	class CCollectCollisionPointsInCircleProxy;
	class CGenerationManager;
	class CNavmeshAreaDescription;
	class CObstaclesMapper;
	class CRuntimeTaskManager;
	class CSearchData;
	class CStreamingManager;
	class CTaskManager;
	class CTerrainAreaDescription;
	class CWorldTextFile;
	class CVisualizer;

	template < class T > struct TMultiAreaSpatialQuery;
};		// namespace PathLib

class CPathLibCooker;
class CDeniedAreaComponent;
class CRenderFrame;
class CDirectory;
class CDiskFile;
class CNavigationCookingContext;

////////////////////////////////////////////////////////////////////////////
// This CObject represents most PathLib data and functionalities that are
// visible from outside
class CPathLibWorld : public CObject
{
	DECLARE_ENGINE_CLASS( CPathLibWorld, CObject, 0 )
	friend class PathLib::CInstanceMap;										// direct access to internal instances list
	friend class PathLib::CGenerationManager;										// basically task manager is integral part of CPathLibWorld
	friend class CPathLibCooker;
protected:
	
	////////////////////////////////////////////////////////////////////////
	typedef TArrayMap< CGUID, PathLib::AreaId > GuidMapping;
	struct SInstanceAreaDesc
	{
		PathLib::AreaId									m_id;
		PathLib::CNavmeshAreaDescription*				m_area;
		Bool operator<( const SInstanceAreaDesc& c ) const					{ return m_id < c.m_id; }
	};
	typedef TSortedArray< SInstanceAreaDesc >								InstanceAreas;
	typedef TDynArray< PathLib::CTerrainAreaDescription*, MC_PathLib >		TerrainAreas;
	PathLib::CSearchEngine							m_searchEngine;
	CPathLibSettings								m_globalSettings;
	PathLib::CTerrainInfo							m_terrainInfo;
	PathLib::CObstaclesMapper*						m_mapper;
	PathLib::CStreamingManager*						m_streamingManager;
	PathLib::CWorldLayersMapping					m_worldLayers;
	////////////////////////////////////////////////////////////////////////
	// area descriptions storage
	GuidMapping													m_instanceGuidMapping;	// navmesh component to instance guid mapping
	InstanceAreas												m_instanceAreas;		// instance graphs (by id)
	TDynArray< PathLib::CTerrainAreaDescription*, MC_PathLib >	m_terrainAreas;			// terrain graphs (cel map)
	PathLib::CInstanceMap										m_instanceMap;			// areas spatial search structure
	////////////////////////////////////////////////////////////////////////
	struct SObstacleRuntimeEvent
	{
		THandle< CComponent >							m_component;
		Bool											m_attach;
		Bool											m_immediate;
	};
	TDynArray< SObstacleRuntimeEvent >				m_obstacleEvents;

	Bool											m_isGameRunning;
	Bool											m_useLocalFolder;
	Bool											m_isCookerMode;
	Bool											m_isProcessingObstacles;

	CNavigationCookingContext*						m_cookingContext;

	CDirectory*										m_cookedDir;
	CDirectory*										m_localDir;
	CDirectory*										m_sourceDir;
	
#ifndef NO_EDITOR_PATHLIB_SUPPORT
	PathLib::CGenerationManager*					m_generationManager;
#endif
	PathLib::CTaskManager*							m_taskManager;
	PathLib::CVisualizer*							m_visualizer;

	////////////////////////////////////////////////////////////////////////
	template < class TQuery >
	Bool						SpatialQuery( PathLib::CAreaDescription* area, TQuery& query ) const;
	template < class TQuery >
	Bool						SpatialQuery( PathLib::CAreaDescription* area, PathLib::TMultiAreaSpatialQuery< TQuery >& query ) const;

	template < class TQuery >
	Bool						SpatialQuery( PathLib::TMultiAreaSpatialQuery< TQuery >& query ) const;
	template < class TQuery >
	Bool						SpatialQuery( PathLib::TMultiAreaSpatialQuery< TQuery >& query, PathLib::AreaId& areaId ) const;

	template < class TQuery >
	Bool						SpatialQuery( TQuery& query ) const;
	template < class TQuery >
	Bool						SpatialQuery( TQuery& query, PathLib::AreaId& areaId ) const;
	////////////////////////////////////////////////////////////////////////
	void						ClearAreas();

	void						AddInstanceArea( PathLib::CNavmeshAreaDescription* area );
	void						RemoveInstanceArea( PathLib::AreaId areaId );
	Bool						DeleteAreaFiles( CDirectory* dir, PathLib::AreaId areaId );

	CDirectory*					InternalGetDirectory( const String& dirName, Bool createIfMissing ) const;

public:
	CPathLibWorld();
	~CPathLibWorld();

	////////////////////////////////////////////////////////////////////////
	// queries
	Bool						FindRandomPositionInRadius( PathLib::AreaId areaId, const Vector3& pos, Float searchRadius, Float personalSpace, Uint32 agentCategory, Vector3& posOut );

	////////////////////////////////////////////////////////////////////////
	void						Tick();
	void						SetReferencePosition( const Vector& position );

	CWorld*						GetWorld() const;
	void						SetGameRunning( Bool isGameRunning );
	Bool						IsGameRunning() const						{ return m_isGameRunning; }

	void						Initialize();
	void						Shutdown();
	void						Reload();

	Bool						HasJobOrTasks() const;

	////////////////////////////////////////////////////////////////////////
	Bool						ReinitializeSystem();
	void						RecalculateAllNavigationData();
	
	CDirectory*					ForceGetLocalDataDirectory();
	CDirectory*					GetCookedDataDirectory() const				{ return m_cookedDir; }
	CDirectory*					GetLocalDataDirectory()	const				{ return m_localDir; }
	CDirectory*					GetSourceDataDirectory() const				{ return m_sourceDir; }

	CDirectory*					GetSaveDirectory();
	CDiskFile*					GetFile4Load( const String& fileName ) const;
	CDiskFile*					GetFile4Save( const String& fileName );

	void						GetGenericFileName( PathLib::AreaId areaId, String& outFilename, const Char* extension );
	void						GetNavgraphFileName( PathLib::AreaId areaId, String& outFilename );
	void						GetTerrainFileName( PathLib::AreaId areaId, String& outFilename );
	void						GetObstaclesFileName( PathLib::AreaId areaId, String& outFilename );
	static PathLib::AreaId		GetInstanceAreaIdFromFileName( const String& filename );

	
	Bool						InitializeTerrain();
	void						MarkTileSurfaceModified( Uint32 x, Uint32 y );
	void						MarkTileCollisionsModified( Uint32 x, Uint32 y );
	void						MarkTileNavigationModified( Uint32 x, Uint32 y );
	Bool						DestroyTerrainData();
	const Box&					GetStreamedWorldBox();

	// instances external modyfications (navmesh)
	void						NotifyOfInstanceAttached( CNavmeshComponent* instanceComponent );
	void						NotifyOfInstanceUpdated( CNavmeshComponent* instanceComponent, Bool areaChanged, Uint32 areaDirtyFlags = 0 );
	void						NotifyOfInstanceRemoved( CNavmeshComponent* instanceComponent );
	void						NotifyOfInstanceRemoved( PathLib::AreaId areaId );
	PathLib::AreaId				GetEngineComponentInstanceAreaMapping( CNavmeshComponent* instanceComponent );

	// instance external systems processing (like navmesh modyfication)
	Bool						StartInstanceProcessing( CNavmeshComponent* instanceComponent );
	Bool						IsInstanceProcessing( CNavmeshComponent* instanceComponent );
	void						EndInstanceProcessing( CNavmeshComponent* instanceComponent );

	void						NotifyOfComponentAttached( PathLib::IComponent* component );
	void						NotifyOfComponentUpdated( PathLib::IComponent* component, Bool force = false );
	void						NotifyOfComponentDetached( PathLib::IComponent* component );
	void						NotifyOfComponentRemoved( PathLib::IComponent* component );

	void						OnLayerEnabled( CLayerGroup* layerGroup, Bool isEnabled );
	
	void						UpdateObstacles( Box& bbox );
	Bool						UpdateInstancesMarking( const Vector2& worldMin, const Vector2& worldMax );

	void						ReadConfiguration();

	static Bool					IsLocalNavdataFolderForced();
	static void					ForceLocalNavdataFolder( Bool b );

#ifndef NO_EDITOR_PATHLIB_SUPPORT
	Bool						ForceSynchronousAreaProcessing( PathLib::AreaId areaId );

	static Bool					IsTaskManagerEnabled();
	static Bool					IsObstacleGenerationEnabled();
	static void					EnableTaskManager( Bool on );
	static void					EnableObstacleGeneration( Bool on );
	PathLib::CGenerationManager* GetGenerationManager()						{ return m_generationManager; }
	PathLib::CTaskManager*		GetTaskManager()							{ return m_taskManager; }
	void						EnableObstaclesProcessing( Bool enable )	{ m_isProcessingObstacles = enable; }
	Bool						IsObstacleGenerationFlagEnabled() const		{ return m_isProcessingObstacles; }
	CNavigationCookingContext*	GetCookingContext() const					{ return m_cookingContext; }
	Bool						IsCooking() const							{ return m_isCookerMode; }
	void						SetCookerMode( CNavigationCookingContext* cookContext )	{ m_isCookerMode = true; m_useLocalFolder = false; m_cookingContext = cookContext; }
	Bool						SaveSystemConfiguration();
#endif
	PathLib::AreaId				GetFreeInstanceId();
	////////////////////////////////////////////////////////////////////////

	PathLib::CNavmeshAreaDescription*	GetInstanceAreaDescription(PathLib::AreaId areaId) const;
	PathLib::CTerrainAreaDescription*	GetTerrainAreaDescription(PathLib::AreaId areaId) const;
	Bool								IsLocationLoaded( const Vector3& v );
	Bool								IsLocationLoaded( const Vector3& v, PathLib::AreaId& outAreaId );
	PathLib::AreaId						GetReadyAreaAtPosition( const Vector3& v );
	PathLib::CAreaDescription*			GetAreaDescription(PathLib::AreaId areaId) const;
	PathLib::CTerrainAreaDescription*	GetTerrainAreaAtPosition( const Vector3& v ) const;
	PathLib::CAreaDescription*			GetAreaAtPosition( const Vector3& v ) const;
	PathLib::CAreaDescription*			GetAreaAtPosition( const Vector3& v, PathLib::AreaId& hint ) const;
	PathLib::CNavmeshAreaDescription*	GetClosestInstanceArea( const Vector3& v );
	Uint32								CollectAreasAt( const Box& bbox, PathLib::CAreaDescription** areaList, Uint32 maxOutputSize, Bool getNavmeshAreas = true ) const;

	PathLib::CVisualizer*				GetVisualizer() const				{ return m_visualizer; }
	PathLib::CObstaclesMapper*			GetObstaclesMapper() const			{ return m_mapper; }
	const CPathLibSettings&				GetGlobalSettings() const			{ return m_globalSettings; }
	const PathLib::CTerrainInfo&		GetTerrainInfo() const				{ return m_terrainInfo; }
	const PathLib::CInstanceMap*		GetInstanceMap() const				{ return &m_instanceMap; }
	PathLib::CStreamingManager*			GetStreamingManager() const			{ return m_streamingManager; }
	const GuidMapping&					GetGuidMapping() const				{ return m_instanceGuidMapping; }
	PathLib::CSearchEngine&				GetSearchEngine()					{ return m_searchEngine; }
	const PathLib::CWorldLayersMapping&	GetWorldLayers() const				{ return m_worldLayers; }
	PathLib::CWorldLayersMapping&		GetWorldLayers()					{ return m_worldLayers; }

	////////////////////////////////////////////////////////////////////////
	void						GenerateEditorFragments( CRenderFrame* frame );

	struct CustomPositionFilter
	{
		virtual Bool			TestPosition( const Vector3& pos ) = 0;
	};

	Bool						FindClosestWalkableSpotInArea( const Box& bbox, const Vector3& closestSpot, Float personalSpace, Vector3& outPosition, PathLib::AreaId& outAreaId, CustomPositionFilter* filter = NULL ) const;
	////////////////////////////////////////////////////////////////////////
	// Collision tests
	Bool						CustomCollisionTest( const Vector3& basePos, PathLib::SCustomCollisionTester& collisionTester ) const;
	Bool						ConvexHullCollisionTest( const TDynArray< Vector > & convexHull, const Box& convexHullBBox, const Vector & basePosition ) const;

	Bool						ComputeHeight( const Vector3& v, Float& zOut, Bool smooth = false ) const;
	Bool						ComputeHeight( const Vector2& v, Float zMin, Float zMax, Float& zOut, PathLib::AreaId& outAreaId, Bool smooth = false ) const;
	Bool						ComputeHeightTop( const Vector2& v, Float zMin, Float zMax, Float& zOut, PathLib::AreaId& outAreaId ) const;		// fuck. This should be just ComputeHeight. But.. We are using ComputeHeight( min, max ) and we don't want to modify its behavior to get 'highest hit' by default as it would be pandora box. Insteady we duplicate this function with subtle changes.
	Bool						ComputeHeightFrom( const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) const;
	Bool						TestLine( const Vector3& v1, const Vector3& v2, Uint32 collisionFlags ) const;
	Bool						TestLine( const Vector3& v1, const Vector3& v2, Float personalSpace, Uint32 collisionFlags ) const;
	Bool						TestLocation( const Vector3& v1, Float personalSpace, Uint32 collisionFlags ) const;
	Bool						TestLocation( const Vector3& v1, Uint32 collisionFlags ) const;
	Float						GetClosestObstacle( const Vector3& v, Float personalSpace, Vector3& outCollisionPoint, Uint32 collisionFlags = PathLib::CT_DEFAULT ) const;
	Float						GetClosestObstacle( const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outCollisionPoint, Vector3& outPointOnLine, Uint32 collisionFlags = PathLib::CT_DEFAULT ) const;
	Bool						CollectGeometryAtLocation( const Vector3& v1, Float personalSpace, Uint32 collisionFlags, TDynArray< Vector >& output ) const;

	Bool						MoveAwayFromWall( PathLib::AreaId areaId, const Vector3& pos, Float personalSpace, Vector3& outPos, PathLib::CollisionFlags flags = PathLib::CT_DEFAULT ) const;
	Bool						FindSafeSpot( PathLib::AreaId areaId, const Vector3& pos, Float radius, Float personalSpace, Vector3& outPos, Float* minZptr = nullptr, Float* maxZptr = nullptr, PathLib::CollisionFlags flags = PathLib::CT_DEFAULT, const Vector2* optionalPreferedDir = nullptr ) const;
	Bool						ComputeHeight( PathLib::AreaId& areaId, const Vector3& v, Float& zOut, Bool smooth = false ) const;
	Bool						ComputeHeightFrom( PathLib::AreaId& areaId, const Vector2& pos, const Vector3& posFrom, Float& outHeight, Bool smooth = false ) const;
	Bool						TestLine( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Uint32 collisionFlags ) const;
	Bool						TestLine( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Uint32 collisionFlags ) const;
	Bool						TestLocation( PathLib::AreaId& areaId, const Vector3& v1, Float personalSpace, Uint32 collisionFlags ) const;
	Bool						TestLocation( PathLib::AreaId& areaId, const Vector3& v1, Uint32 collisionFlags ) const;
	Float						GetClosestObstacle( PathLib::AreaId& areaId, const Vector3& v, Float personalSpace, Vector3& outCollisionPoint, Uint32 collisionFlags = PathLib::CT_DEFAULT ) const;
	Float						GetClosestObstacle( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outCollisionPoint, Vector3& outPointOnLine, Uint32 collisionFlags = PathLib::CT_DEFAULT ) const;
	PathLib::EClearLineTestResult	GetClearLineInDirection( PathLib::AreaId& areaId, const Vector3& v1, const Vector3& v2, Float personalSpace, Vector3& outPos, Uint32 collisionFlags = PathLib::CT_DEFAULT ) const;
	Bool						CollectCollisionPoints( PathLib::AreaId& areaId, const Vector3& v, Float personalSpace, PathLib::CCollectCollisionPointsInCircleProxy& proxy, PathLib::CollisionFlags = PathLib::CT_DEFAULT ) const;
	//Bool						GetClosestSafeSpot( const Vector3& v, Float fPersonalSpace, Float range, Vec3& outSafeSpot, Uint32 nCollisionFlags = PathLib::CT_DEFAULT);

	struct TerrainAreasIterator
	{
	protected:
		TerrainAreas::iterator			m_it;
		TerrainAreas::iterator			m_end;
	public:
		TerrainAreasIterator( CPathLibWorld& pathlib )
			: m_it( pathlib.m_terrainAreas.Begin() )
			, m_end( pathlib.m_terrainAreas.End() )							{}

		operator				Bool() const								{ return m_it != m_end; }
		void					operator++()								{ ++m_it; }

		PathLib::CTerrainAreaDescription* 	operator*() const				{ return *m_it; }
	};

	struct InstanceAreasIterator
	{
	protected:
		InstanceAreas::iterator			m_it;
		InstanceAreas::iterator			m_end;
	public:
		InstanceAreasIterator( CPathLibWorld& pathlib )
			: m_it( pathlib.m_instanceAreas.Begin() )
			, m_end( pathlib.m_instanceAreas.End() )						{}

		operator				Bool() const								{ return m_it != m_end; }
		void					operator++()								{ ++m_it; }

		PathLib::CNavmeshAreaDescription* 	operator*() const				{ return m_it->m_area; }
	};
};

BEGIN_CLASS_RTTI( CPathLibWorld )
	PARENT_CLASS( CObject );
	PROPERTY_INLINED( m_globalSettings, TXT("Settings") );
END_CLASS_RTTI();



