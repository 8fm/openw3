/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CWayPointCookingContext;
struct SWayPointCookingData;
struct SPartyCookingData;

RED_WARNING_PUSH()
RED_DISABLE_WARNING_MSC( 4200 ) // nonstandard extension used : zero-sized array in struct/union

struct SPartyWaypointData
{
	DECLARE_RTTI_STRUCT( SPartyWaypointData )

	SPartyWaypointData()
		: m_position( Vector::ZEROS )
		, m_rotation( 0.f )				{}
	~SPartyWaypointData()				{}
	
	Vector3								m_position;
	Float								m_rotation;
	CName								m_memberName;
};

BEGIN_CLASS_RTTI( SPartyWaypointData )
	PROPERTY( m_position )
	PROPERTY( m_rotation )
	PROPERTY( m_memberName )
END_CLASS_RTTI()

struct SPartySpawner
{
	DECLARE_RTTI_STRUCT( SPartySpawner )

	SPartySpawner()
		: m_firstIndex( 0 )
		, m_waypointsCount( 0 )
		, m_mappingIndex( 0 )			{}		
	~SPartySpawner()					{}

	Uint32								m_firstIndex;
	Uint32								m_waypointsCount;
	Uint32								m_mappingIndex;

	const SPartyWaypointData*			FindWaypoint( const CWayPointsCollection& collection, const CName& partyMemberName, TDynArray< Bool >& excludedWaypoints ) const;
};

BEGIN_CLASS_RTTI( SPartySpawner )
	PROPERTY( m_firstIndex )
	PROPERTY( m_waypointsCount )
	PROPERTY( m_mappingIndex )
END_CLASS_RTTI()

class CWayPointsCollection : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWayPointsCollection, CResource, "redwpc", "Cooked waypoints collection" );
public:
	typedef Uint32 GroupId;
	typedef Int16 GroupIndex;

	// mapping waypoint cooked data -> component
	struct SComponentMapping
	{
	public:
		CGUID								m_entityGuid;
		CGUID								m_componentGuid;

		Bool operator==( const SComponentMapping& m ) const															{ return m_entityGuid == m.m_entityGuid && m_componentGuid == m.m_componentGuid; }
		RED_FORCE_INLINE Uint32 CalcHash() const																	{ return m_entityGuid.CalcHash() ^ m_componentGuid.CalcHash(); }
	};

	// base waypoint cooked information
	struct SWayPointHandle
	{
	public:
		Vector3								m_position;
		Float								m_yaw;
		Int32								m_ownerMapping;
	};

	// waypoint collections
	struct SWayPointsGroup
	{
	public:
		Uint32								m_firstIndex;
		Uint32								m_waypointsCount;
		GroupId								m_groupId;
		
		Uint16*								GetIndexes( const CWayPointsCollection& c ) const						{ return &c.m_indexes[ m_firstIndex ]; }
	};

	struct Input
	{
		Input( CWayPointCookingContext& context )
			: m_context( context )													{}

		typedef THashSet< SComponentMapping > ActionPointsList;
		typedef TDynArray< SComponentMapping > APIdsList;

		struct Group
		{
			APIdsList							m_list;
			Uint32								m_groupId;

			Group()																	{}
			Group( Group&& g )
				: m_list( Move( g.m_list ) )
				, m_groupId( g.m_groupId )											{}

			Bool operator<( const Group& g ) const									{ return m_groupId < g.m_groupId; }
		};

		typedef TSortedArray< Group > GroupsCollection;

		CWayPointCookingContext&			m_context;
		ActionPointsList					m_aps;
		GroupsCollection					m_groups;
	};

protected:
	DataBuffer							m_dataBuffer;
	Uint16								m_version;
	Uint16								m_waypointsCount;
	Uint16								m_componentsMappingsCount;
	Uint16								m_waypointsGroupsCount;
	Uint32								m_indexesCount;

	SWayPointHandle*					m_waypoints;
	SComponentMapping*					m_componentMappings;
	SWayPointsGroup*					m_waypointsGroups;
	Uint16*								m_indexes;

	TDynArray< SPartySpawner >			m_parties;
	TDynArray< SPartyWaypointData >		m_partyWaypoints;
	
	void								CreateWaypoint( const SWayPointCookingData& wpInput, SWayPointHandle& outWp );
	void								CreateParty( const SComponentMapping& wpMapping, const CWayPointCookingContext& context, GroupId mappingIndex );

public:
	static const Uint16 CURRENT_BINARIES_VERSION = 1;

	CWayPointsCollection();
	~CWayPointsCollection();

	Bool Create( const Input& input );

	virtual void OnSerialize( IFile& file ) override;

	Bool RestoreDataPostSerialization();

	Uint16 GetWaypointsCount() const												{ return m_waypointsCount; }

	const SWayPointHandle& GetWaypoint( Uint16 wpId ) const							{ return m_waypoints[ wpId ]; }
	const SPartyWaypointData* GetPartyWaypoints( Uint32 firstIndex ) const			{ return &m_partyWaypoints[ firstIndex ]; }
	SComponentMapping* GetComponentMapping( const SWayPointHandle& wp ) const		{ if ( wp.m_ownerMapping < 0 ) { return nullptr; } return &m_componentMappings[ wp.m_ownerMapping ]; }
	const SWayPointsGroup& GetWPGroup( GroupIndex groupIndex ) const				{ return m_waypointsGroups[ groupIndex ]; }
	GroupIndex GetWPGroupIndexById( GroupId groupId ) const;
	const SPartySpawner* GetPartySpawner( GroupId mappingIndex ) const;
};

BEGIN_CLASS_RTTI( CWayPointsCollection )
	PARENT_CLASS( CResource )
	PROPERTY( m_version )
	PROPERTY( m_waypointsCount )
	PROPERTY( m_componentsMappingsCount )
	PROPERTY( m_waypointsGroupsCount )
	PROPERTY( m_indexesCount )
	PROPERTY( m_parties )
	PROPERTY( m_partyWaypoints )
END_CLASS_RTTI()

RED_WARNING_POP()