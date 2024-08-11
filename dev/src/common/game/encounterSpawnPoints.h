/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "wayPointsCollection.h"

class CEncounterSpawnPoints;

struct SEncounterSpawnPointData
{
public:
	typedef TDynArray< SEncounterSpawnPointData > List;

	SEncounterSpawnPointData()											{}

	CWayPointComponent*						GetWaypoint( CWorld* world, const CWayPointsCollection& collection, const CWayPointsCollection::SWayPointHandle& baseData );

	EngineTime								m_timeout;
	THandle< CWayPointComponent >			m_waypoint;
};


struct SEncounterSpawnPoint
{
	typedef Uint16 Id;

	const CWayPointsCollection::SWayPointHandle*	m_baseData;
	SEncounterSpawnPointData*						m_runtimeData;
	CWayPointsCollection*							m_collection;

	SEncounterSpawnPoint()												{} // NOTICE: empty default constructor
	SEncounterSpawnPoint( const CWayPointsCollection::SWayPointHandle* wp, SEncounterSpawnPointData* runtimeData, CWayPointsCollection* collection )
		: m_baseData( wp )
		, m_runtimeData( runtimeData )
		, m_collection( collection )									{}
};


struct SEncounterSpawnGroupIterator : public Red::System::NonCopyable
{
protected:
	const CWayPointsCollection&				m_collection;
	SEncounterSpawnPointData::List&			m_runtimeData;

	Uint16*									m_wp;
	Uint32									m_waypointsLeft;

	Uint16*									m_baseWP;
	Uint32									m_waypointsCount;
public:
	typedef CWayPointsCollection::SComponentMapping SComponentMapping;
	typedef CWayPointsCollection::SWayPointHandle SWayPointHandle;

	SEncounterSpawnGroupIterator( CEncounterSpawnPoints& sp, CWayPointsCollection::GroupIndex groupIndex );

	operator								Bool() const											{ return m_waypointsLeft != 0; }
	void									operator++()											{ ++m_wp; --m_waypointsLeft; }

	void									Reset();

	Uint16									GetIndex() const										{ return *m_wp; }
	SEncounterSpawnPointData&				RuntimeData() const										{ return m_runtimeData[ *m_wp ]; }
	const SWayPointHandle&					GetBaseData() const										{ return m_collection.GetWaypoint( *m_wp ); }
	const SComponentMapping*				GetComponentMapping() const								{ return m_collection.GetComponentMapping( m_collection.GetWaypoint( *m_wp ) ); }

	const CWayPointsCollection&				GetWPCollection() const									{ return m_collection; }
};

struct SRandomizedEncounterSpawnGroupIterator : public SEncounterSpawnGroupIterator
{
protected:
	Uint32									m_randomizedPoint;
	Bool									m_isLooped;
public:
	SRandomizedEncounterSpawnGroupIterator( CEncounterSpawnPoints& sp, CWayPointsCollection::GroupIndex groupIndex );

	void									Reset();

	void									operator++()
	{
		++m_wp; --m_waypointsLeft;
		if ( m_waypointsLeft == 0 && !m_isLooped )
		{
			m_wp = m_baseWP;
			m_waypointsLeft = m_randomizedPoint;
			m_isLooped = true;
		} 
	}
};

class CEncounterSpawnPoints
{
protected:
	THandle< CWayPointsCollection >						m_collection;
	SEncounterSpawnPointData::List						m_runtimeData;

public:
	typedef CWayPointsCollection::GroupId GroupId;
	typedef CWayPointsCollection::GroupIndex GroupRuntimeIndex;

	CEncounterSpawnPoints()																			{}
	~CEncounterSpawnPoints()																		{}

	Bool									Initialize( CGameWorld* world, CEncounter* encounter );

	Bool									IsInitialized() const									{ return m_collection.IsValid(); }
	const CWayPointsCollection*				GetCollection()	const									{ return m_collection.Get(); }
	SEncounterSpawnPointData::List&			GetRuntimeData()										{ return m_runtimeData; }
	GroupRuntimeIndex						GetGroupRuntimeIndex( GroupId groupId );

	//Bool									Request();
	//void									Release();

	SEncounterSpawnPointData&				GetRuntimeWP( Uint32 index )							{ return m_runtimeData[ index ]; }

	SEncounterSpawnPoint					GetSpawnPoint( Uint32 sp )								{ CWayPointsCollection* collection = m_collection.Get(); return SEncounterSpawnPoint( &collection->GetWaypoint( sp ), &m_runtimeData[ sp ], collection );}
	SEncounterSpawnPoint					GetRandomSpawnPoint();

	void									OnSerialize( IFile& file );

	static void								ComputeResourceFileName( CEncounter* encounter, String& outPath );
	static void								ComputeResourceDepotPath( CWorld* world, CEncounter* encounter, String& outPath );
};



