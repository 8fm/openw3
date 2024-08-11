#pragma once

#include "../engine/occlusionSystem.h"

#include "encounterTypes.h"
#include "spawnTree.h"
#include "spawnTreeSpawnStrategy.h"
#include "wayPointsCollection.h"

enum ESpawnTreeSpawnVisibility
{ 
	STSV_SPAWN_HIDEN,
	STSV_SPAWN_ALWAYS,
	STSV_SPAWN_ONLY_VISIBLE
};

BEGIN_ENUM_RTTI( ESpawnTreeSpawnVisibility );
	ENUM_OPTION( STSV_SPAWN_HIDEN );
	ENUM_OPTION( STSV_SPAWN_ALWAYS );
	ENUM_OPTION( STSV_SPAWN_ONLY_VISIBLE );
END_ENUM_RTTI();

#ifdef USE_UMBRA
struct SOcclusionSPQuery
{
	DECLARE_RTTI_STRUCT( SOcclusionSPQuery );

	Uint32					m_spawnPointIndex;
	COcclusionQueryPtr		m_query;

	SOcclusionSPQuery() {}

	SOcclusionSPQuery( Uint32 spawnPointIndex, COcclusionQueryPtr&& query )
		: m_spawnPointIndex( spawnPointIndex )
		, m_query( Move( query ) ) {}

	SOcclusionSPQuery( SOcclusionSPQuery&& query )
		: m_spawnPointIndex( query.m_spawnPointIndex )
		, m_query( Move( query.m_query ) ) {}

	SOcclusionSPQuery( const SOcclusionSPQuery& query )
		: m_spawnPointIndex( query.m_spawnPointIndex )
		, m_query( query.m_query ) {}

	SOcclusionSPQuery& operator=( const SOcclusionSPQuery& query )
	{
		m_spawnPointIndex = query.m_spawnPointIndex;
		m_query = query.m_query;
		return *this;
	}
};

BEGIN_NODEFAULT_CLASS_RTTI( SOcclusionSPQuery )
END_CLASS_RTTI()

#define MAX_QUERIES 5
#endif

class ISpawnTreeBaseSpawner
{
protected:
	typedef ISpawnTreeSpawnStrategy::ESpawnPointTestResult EVisibilityResult;

#ifdef USE_UMBRA
	typedef TStaticArray< SOcclusionSPQuery,MAX_QUERIES > QueriesList;
	TInstanceVar< QueriesList >									i_occlusionQuerries;
#endif
	TInstanceVar< CWayPointsCollection::GroupIndex >			i_spawnPointListIndex;

	ESpawnTreeSpawnVisibility									m_visibility;
	Float														m_spawnpointDelay;

	virtual Bool					AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const;
	virtual Bool					IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const;
	
public:
									ISpawnTreeBaseSpawner();

	virtual void					CollectTags( TagList& tagList ) const;
	EFindSpawnResult				FindSpawnPoint( CSpawnTreeInstance& instance, const SCompiledSpawnStrategyInitializer& strategy, SSpawnTreeUpdateSpawnContext& context, Vector3& outPos, Float& outYaw, Uint32& outSP ) const;
	EFindSpawnResult				FindClosestSpawnPoint( CSpawnTreeInstance& instance, SSpawnTreeUpdateSpawnContext& context, Vector3& outPosition, Float& outYaw, Uint32& outSP, Float& cloesestDistSq ) const;

#ifndef NO_EDITOR
	virtual void					CollectWaypoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput, TDynArray< CWayPointsCollection::SComponentMapping >& outWaypointsList ) const;
#endif

	void							OnSpawnTreeDeactivation( CSpawnTreeInstance& instance ) const;

	// Instance buffer interface - NOTICE: should be marked virtual if required
	void							OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	void							OnInitData( CSpawnTreeInstance& instance ) const;
	void							OnDeinitData( CSpawnTreeInstance& instance ) const;

	void							SetSpawnPointDelay( Float delay )				{ m_spawnpointDelay = delay; }
	Float							GetSpawnPointDelay( ) const						{ return m_spawnpointDelay; }

	void							SetSpawnPointListIndex( CSpawnTreeInstance& instance, CWayPointsCollection::GroupIndex listIndex ) const	{ instance[ i_spawnPointListIndex ] = listIndex; }

};

class CSpawnTreeWaypointSpawner : public ISpawnTreeBaseSpawner
{
	typedef ISpawnTreeBaseSpawner Super;

	DECLARE_RTTI_SIMPLE_CLASS( CSpawnTreeWaypointSpawner );
protected:
	TagList						m_tags;
	Bool						m_useLocationTest;

	Bool						AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const override;
	Bool						IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const override;
public:
								CSpawnTreeWaypointSpawner();

#ifndef NO_EDITOR
	void						CollectWaypoints( CSpawnTreeInstance& instance, CWayPointsCollection::Input& waypointsInput, TDynArray< CWayPointsCollection::SComponentMapping >& outWaypointsList ) const override;
#endif
	void						CollectTags( TagList& tagList ) const override;

	Bool						IsValid() const											{ return !m_tags.Empty(); }
	const TagList&				GetTags() const											{ return m_tags; }
};

BEGIN_NODEFAULT_CLASS_RTTI( CSpawnTreeWaypointSpawner )
	PROPERTY_EDIT( m_visibility, TXT("Visibility settings") )
	PROPERTY_EDIT( m_spawnpointDelay, TXT("Spawnpoint minimal usage delay") )
	PROPERTY_EDIT( m_tags, TXT("Waypoint tags required") )
	PROPERTY_EDIT( m_useLocationTest, TXT("Shoud location test be made") )
END_CLASS_RTTI()

class CSpawnTreeActionPointSpawner : public CSpawnTreeWaypointSpawner
{
	typedef CSpawnTreeWaypointSpawner Super;
	DECLARE_RTTI_SIMPLE_CLASS( CSpawnTreeActionPointSpawner );

protected:
	TDynArray< CName >			m_categories;

	Bool						AcceptWaypoint( const SWayPointCookingData& sp, const CWayPointCookingContext& context ) const override;
	Bool						IsSpawnPointValid( CSpawnTreeInstance& instance, const SEncounterSpawnGroupIterator& sp ) const override;
public:
								CSpawnTreeActionPointSpawner()							{}

	const TDynArray< CName >&	GetCategories() const									{ return m_categories; }
};

BEGIN_NODEFAULT_CLASS_RTTI( CSpawnTreeActionPointSpawner )
	PROPERTY_EDIT( m_visibility, TXT("Visibility settings") )
	PROPERTY_EDIT( m_spawnpointDelay, TXT("Spawnpoint minimal usage delay") )
	PROPERTY_EDIT( m_tags, TXT("Waypoint tags required") )
	PROPERTY_CUSTOM_EDIT_ARRAY( m_categories, TXT("Categories filter"), TXT("2daValueSelection") );
END_CLASS_RTTI()
