/*
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma once

#include "../engine/world.h"
#include "movingPhysicalAgentComponent.h"



class IEntityStateChangeRequest;
class CSpawnPointComponent;
class CInterestPointComponent;
class CGameplayEntity;
class CWayPointCookingContext;
class ExpManager;

struct StateChangeRequestsDesc
{
	String			m_entityTag;
	String			m_requestDetails;

	StateChangeRequestsDesc( const String& entityTag = TXT( "" ) ) 
		: m_entityTag( entityTag )
	{}
};

/// Special world version for Wither2 game
class CGameWorld : public CWorld
{
	DECLARE_ENGINE_RESOURCE_CLASS( CGameWorld, CWorld, "w2w", "Game World" );

protected:
	TDynArray< CSpawnPointComponent* >							m_spawnPoints;
	TDynArray< CInterestPointComponent* >						m_intrestPoints;
	TDynArray< CMovingPhysicalAgentComponent* >					m_agentsThatNeedSeparation;
	TDynArray< CMovingPhysicalAgentComponent* >					m_agentsThatDontNeedSeparation;
	TDynArray< CMovingPhysicalAgentComponent* >					m_agentsThatAreInvisible;
	CMovingPhysicalAgentComponent::ResolveSeparationContext		m_resolveSeparationContext;
	Float m_invisibleAgentsAccum;

	typedef THashMap< CName, IEntityStateChangeRequest* >		StateChangeRequests;
	StateChangeRequests											m_requests;

	ExpManager*													m_expManager;					//!< Exploration manager
	THandle< CCookedExplorations >								m_cookedExplorations;
	THandle< CWayPointsCollectionsSet >							m_cookedWaypoints;
#ifndef NO_RUNTIME_WAYPOINT_COOKING
	CWayPointCookingContext*									m_wayPointCookingContext;
#endif

public:

	CGameWorld();
	~CGameWorld();

	virtual void OnSerialize( IFile& file );

	// Initialize dynamic world shit
	virtual void Init( const WorldInitInfo& initInfo ) override;

	// Shut down dynamic world
	virtual void Shutdown() override;

	//! Called when the game shuts down
	void OnShutdownAtGameEnd();

	virtual void OnLayerAttached( CLayer* layer );
	virtual void OnLayerDetached( CLayer* layer );

#ifndef NO_RESOURCE_COOKING
	void OnCook( class ICookerFramework& cooker );
#endif

public:
	// Adds spawn point component to the area tree (CAreaTree) for fast searching
	void AddSpawnPointComponent( CSpawnPointComponent* spawnPoint );

	// Remove spawn point component from the area tree (CAreaTree) for fast searching
	void RemoveSpawnPointComponent( CSpawnPointComponent* spawnPoint );

	// Finds all spawn point components that are in a bounding box and puts them into an array
	void FindSpawnPointComponents( const Box &boundingBox, TDynArray< CSpawnPointComponent* > &spawnPoints );

public:
	// Adds an interest point to the area tree (CAreaTree) for fast searching
	void AddInterestPoint( CInterestPointComponent *interestPoint );

	// Remove an interest point from the area tree (CAreaTree) for fast searching
	void RemoveInterestPoint( CInterestPointComponent *interestPoint );

	// Finds all interest points that are in a bounding box and puts them into an array
	void FindInterestPoint( const Box &boundingBox, TDynArray< CInterestPointComponent* > &interestPoints );

public:
	//! Registers a new state change request
	void RegisterStateChangeRequest( CName tag, IEntityStateChangeRequest* request );

	//! Removes a state change request registered for the specified tag
	void ResetStateChangeRequest( CName tag );

	//! Updates the state of an entity
	void UpdateEntityState( CGameplayEntity* entity );

	//! Removes all entity state change requests
	void ClearChangeRequests();

	//! Generates debug description of all registered state change requests
	void OnStateChangeRequestsDebugPage( TDynArray< StateChangeRequestsDesc >& outDescriptions ) const;

	virtual void OnTerrainCollisionDataBoundingUpdated( const Box& bbox ) override;

	// Wind
	virtual Vector GetWindAtPointForVisuals( const Vector& point, Bool withTurbulence, Bool withForcefield = true ) const;
	virtual Vector GetWindAtPoint( const Vector& point ) const;
	virtual void GetWindAtPoint( Uint32 elementsCount, void* inputPos, void* outputPos, size_t stride ) const;

	Vector GetGlobalWind() const;

	virtual void FinalizeMovement( Float timeDelta );

public:
	virtual void RefreshAllWaypoints( const Box& bbox ) override;

#ifndef NO_EDITOR_FRAGMENTS
public:
	// Generate editor fragments
	virtual void GenerateEditorFragments( CRenderFrame* frame ) override;
#endif

public:
	// ------------------------------------------------------------------------
	// iterate over pre-collected entities

	struct SpawnPointInterator : public Red::System::NonCopyable
	{
		TDynArray< CSpawnPointComponent* >::const_iterator			m_it;
		TDynArray< CSpawnPointComponent* >::const_iterator			m_end;

		SpawnPointInterator( const CGameWorld& world )
			: m_it( world.m_spawnPoints.Begin() )
			, m_end( world.m_spawnPoints.End() )								{}

		CSpawnPointComponent*	operator*() const								{ return *m_it; }

		operator Bool() const													{ return m_it != m_end; }
		SpawnPointInterator& operator++()										{ ++m_it; return *this; }
	};

	// ------------------------------------------------------------------------
	// Save game
	// ------------------------------------------------------------------------
	// Saves the world state
	virtual void SaveState( IGameSaver* saver );

	// Restores the world state
	virtual void RestoreState( IGameLoader* loader );

#ifndef NO_DEBUG_PAGES
public:
	Uint32 GetSpawnPointComponentNum() const;
	Uint32 GetInterestPointComponentNum() const;
#endif

	//! Get exploration manager
	RED_INLINE ExpManager* GetExpManager() const { return m_expManager; }

	CWayPointsCollectionsSet* GetCookedWaypoints() const { return m_cookedWaypoints; }

#ifndef NO_RUNTIME_WAYPOINT_COOKING
	CWayPointCookingContext* GetWaypointCookingContext() const { return m_wayPointCookingContext; }
#endif

};

BEGIN_CLASS_RTTI( CGameWorld );
	PARENT_CLASS( CWorld );
	PROPERTY( m_cookedExplorations );
	PROPERTY( m_cookedWaypoints );
END_CLASS_RTTI();