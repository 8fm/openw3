#pragma once

#include "pathlib.h"
#include "pathlibObstacleSpawnContext.h"
#include "pathlibNodeSetRequestsList.h"

class CPathLibWorld;


namespace PathLib
{

class IComponent;
class CObstacle;
class CObstacleProcessingJob;

////////////////////////////////////////////////////////////////////////////////

class CObstaclesMapper
{
	friend class CObstaclesLayerAsyncProcessing;
	friend class CObstacleProcessingJob;
	friend class IMetalinkComponent;
	friend class IObstacleComponent;
public:
	////////////////////////////////////////////////////////////////////////////
	// Obstacles mapping
	static const Uint32 OBSTACLE_MAX_AREAS = 4;
	struct ObstacleAreaInfo
	{
		ObstacleAreaInfo()														{}
		ObstacleAreaInfo( PathLib::AreaId areaId, ExternalDepenentId obstacleId )
			: m_areaId( areaId )
			, m_obstacleId( obstacleId )										{}
		PathLib::AreaId		m_areaId;
		ExternalDepenentId	m_obstacleId;

		Bool			operator==( const ObstacleAreaInfo& info  ) const		{ return m_areaId == info.m_areaId && m_obstacleId == info.m_obstacleId; }
	};
	struct ObstacleInfo
	{
		ObstacleInfo()
			: m_isRuntimeObstacleEnabled( false )								{}

		TDynArray< ObstacleAreaInfo, MC_PathLib >					m_areaInfo;
		Bool														m_isRuntimeObstacleEnabled;

	};
	typedef THashMap< SComponentMapping, ObstacleInfo >				ObstaclesMapping;

	////////////////////////////////////////////////////////////////////////////
	struct SActiveImmediateObstacle
	{
		THandle< CComponent >						m_component;
		SComponentMapping							m_componentMapping;
	};
	typedef TDynArray< CProcessingEvent >							EventList;
	typedef TArrayMap< SComponentMapping, THandle< CComponent > >	ActiveImmediateObstacleList;

protected:
	////////////////////////////////////////////////////////////////////////////
	CPathLibWorld&									m_pathlib;
	ObstaclesMapping								m_obstacles;					// Main obstacles map. Shows what obstacles are 'on' and what areas they are on currently

	Red::Threads::CMutex							m_eventListMutex;
	Bool											m_hasEvents;
	EventList										m_eventList;
	ActiveImmediateObstacleList						m_immediateObstacles;
	CComponentRuntimeProcessingContext				m_componentProcessingContext;	// Could also be created dynamically.
	////////////////////////////////////////////////////////////////////////////
	void						AddEvent( CProcessingEvent&& e );
	Bool						ProcessObstacleInternal( const CObstacleSpawnData& data, Bool obstacleIsUpdated, Bool checkNavmeshAreas, Bool markDirty, ObstacleInfo* prevInfo = NULL );
	void						ObstaclesMappingUpdated( const SComponentMapping& mapping, ObstacleInfo&& newInfo, ObstacleInfo* prevInfo, Bool markDirty );
	
public:

	CObstaclesMapper( CPathLibWorld& pathlib );
	~CObstaclesMapper();

	void						StealAllEvents( EventList& outEvents );

	void						RemoveImmediateObstacles();

	const ObstacleInfo*			GetMapping( const SComponentMapping& mapping ) const;
	ObstacleInfo*				GetMapping( const SComponentMapping& mapping );
	ObstacleInfo&				RequestMapping( const SComponentMapping& mapping );
	Bool						ForgetMapping( const SComponentMapping& mapping );

	Bool						ProcessObstacleOffline( const CObstacleSpawnData& data, Bool obstacleIsUpdated );
	Bool						ProcessObstacleRemovalOffline( const SComponentMapping& componentMapping );

	void						NotifyOfComponentAttached( IComponent* component );
	void						NotifyOfComponentUpdated( IComponent* component );
	void						NotifyOfComponentDetached( IComponent* component );
	void						NotifyOfComponentRemoved( IComponent* component );

	void						NotifyOfObstacleAttached( CComponent* component, EPathLibCollision collisionType );
	void						NotifyOfObstacleUpdated( CComponent* component, EPathLibCollision collisionType );
	void						NotifyOfObstacleDetached( CComponent* component, EPathLibCollision collisionType );
	void						NotifyOfObstacleRemoved( CComponent* component );
	void						NotifyOfObstacleRemoved( const SComponentMapping& mapping );				// was obstacle already in system
	
	void						ObstacleLoaded( const SComponentMapping& obstacleMapping, AreaId areaId, ExternalDepenentId obstacleId );
	//void						ObstacleDestroyed( CObstacle* obstacle, AreaId areaId );

	void						CheckForObsolateObstacles();
	CPathLibWorld&				GetPathLib()											{ return m_pathlib; }
	Bool						HasMapping( const SComponentMapping& mapping ) const;
	Bool						IsDynamicObstacleEnabled( const SComponentMapping& mapping ) const;
	Bool						IsDirty() const											{ return m_hasEvents; }

	CComponentRuntimeProcessingContext& GetComponentProcessingContext()					{ return m_componentProcessingContext; }

	static Bool					CheckIfObstacleMappingIsObsolate( CWorld* world, const SComponentMapping& mapping );
};

};				// namespace PathLib

//////////////////////////////////////////////////////////////////////////
