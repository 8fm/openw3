/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../engine/pathlibCookerData.h"

#include "wayPointsCollection.h"

struct SWayPointCookingData
{
public:
	typedef CWayPointsCollection::SComponentMapping SComponentMapping;

	SComponentMapping			m_id;
	Vector						m_position;
	EulerAngles					m_orientation;
	CClass*						m_waypointClass;
	TagList						m_tagList;
	IdTag						m_idTag;
	Int32						m_actionPointDataIndex;
	Uint32						m_componentNameHash;

	Bool operator==( const SWayPointCookingData& m ) const															{ return m_id == m.m_id; }
	RED_FORCE_INLINE Uint32 CalcHash() const																		{ return m_id.CalcHash(); }

	Bool operator==( const SComponentMapping& m ) const																{ return m_id == m; }
};

struct SActionPointCookingData
{
	TDynArray< TAPCategory >	m_categories;
};

enum ESpawnPointHandleType
{
	HANDLE_NONE,
	HANDLE_GUID,
	HANDLE_IDTAG
};

struct SPartySpawnPointData
{
	SPartySpawnPointData() 
		: m_componentNameHash( 0 )
		, m_handleType( HANDLE_NONE )
	{
	}

	IdTag m_idTag;
	CGUID m_guid;
	Uint32 m_componentNameHash;
	CName m_memberName;
	ESpawnPointHandleType m_handleType;
};

struct SPartyCookingData
{
	SWayPointCookingData::SComponentMapping m_spawnerMapping;
	TDynArray< SPartySpawnPointData > m_partySpawnPoints;
};

class CWayPointCookingContext : public INavigationCookingSystem
{
public:
	typedef SWayPointCookingData::SComponentMapping SComponentMapping;
	typedef TDynArray< SComponentMapping > WaypointsMappingList;

protected:
	struct WaypointsCollectionHashFunc
	{
		static RED_FORCE_INLINE Uint32 GetHash( const SWayPointCookingData& owner )									{ return owner.m_id.CalcHash(); }
		static RED_FORCE_INLINE Uint32 GetHash( const SComponentMapping& tag )										{ return tag.CalcHash(); }
	};
	struct WaypointsCollectionEqualFunc
	{
		static RED_FORCE_INLINE Bool Equal( const SWayPointCookingData& a, const SWayPointCookingData& b )			{ return a.m_id == b.m_id; }
		static RED_FORCE_INLINE Bool Equal( const SWayPointCookingData& a, const SComponentMapping& m )				{ return a.m_id == m; }
	};
	struct SCollections2Save
	{
		CGUID										m_guid;
		CWayPointsCollection::Input*				m_collectionInput;
	};

	typedef THashSet< SWayPointCookingData, WaypointsCollectionHashFunc, WaypointsCollectionEqualFunc > WaypointsCollection;
	typedef TDynArray< SActionPointCookingData > ActionPointsCollection;
	typedef TDynArray< SPartyCookingData > PartiesCollection;
	typedef TDynArray< SCollections2Save > SavedCollections;
	typedef THashMap< CName, WaypointsMappingList > TagMapping;

	
	WaypointsCollection			m_allWaypoints;
	ActionPointsCollection		m_actionPoints;
	PartiesCollection			m_parties;
	SavedCollections			m_collections2Save;
	mutable TagMapping			m_tagMapping;																		// lazy initialization
	mutable Bool				m_queryStructuresInitialized;														// lazy initialization

	void						LazyInitializeQueryStructures() const;

public:
	struct Handler
	{
		virtual void Handle( const CWayPointCookingContext* context, const SWayPointCookingData& waypoint )			= 0;
	};

	static const Uint32 COOKING_SYSTEM_ID = 0xbaad;

	CWayPointCookingContext();
	~CWayPointCookingContext();

	void						RegisterWaypoint( CWayPointComponent* waypoint );

	void						HandleWaypoints( Handler& handler, const Box& bbox );
	void						ScheduleCollectionToSave( const CGUID& guid, CWayPointsCollection::Input* collection );

	void						Clear();

	const SWayPointCookingData*	GetWaypoint( const SComponentMapping& id ) const;
	const SWayPointCookingData*	GetWaypoint( const IdTag& idTag ) const;
	const SPartyCookingData*	GetParty( const SComponentMapping& id ) const;
	const SWayPointCookingData*	GetWaypoint( const CGUID& entityGUID, Uint32 componentNameHash ) const;
	const WaypointsMappingList*	GetWaypointsByTag( CName tag ) const;
	const SActionPointCookingData& GetActionPointData( Uint32 actionPointDataIndex ) const							{ return m_actionPoints[ actionPointDataIndex ]; }

#ifndef NO_RESOURCE_COOKING
	virtual Bool				CommitOutput() override;
#endif
};

