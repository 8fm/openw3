/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "spawnPointComponent.h"

struct SPartyWaypointHandle
{
	DECLARE_RTTI_STRUCT( SPartyWaypointHandle )

	SPartyWaypointHandle()																			{}
	~SPartyWaypointHandle()																			{}

	CName						m_partyMemberName;

	EntityHandle				m_entityHandle;
	String						m_componentName;

	CWayPointComponent*			Get( CComponent* owner );

	THandle< CWayPointComponent > m_cachedWaypoint;
};

BEGIN_NODEFAULT_CLASS_RTTI( SPartyWaypointHandle )
	PROPERTY_EDIT( m_partyMemberName, TXT("Name, as referenced by party") )
	PROPERTY_EDIT( m_entityHandle, TXT("Handle to entity with waypoint. LEAVE EMPTY FOR 'MY ENTITY'.") )
	PROPERTY_EDIT( m_componentName, TXT("Name of waypoint component. LEAVE EMPTY IF ENTITY HAS ONLY ONE.") )
END_CLASS_RTTI()

class CPartySpawnPointComponent : public CSpawnPointComponent
{
	DECLARE_ENGINE_CLASS( CPartySpawnPointComponent, CSpawnPointComponent, 0 )
protected:
	static const Uint32 MAX_WAYPOINTS = 256;
	TDynArray< SPartyWaypointHandle >		m_partyWaypoints;

	virtual void WaypointGenerateEditorFragments( CRenderFrame* frame ) override;
	// TODO: local bbox to be able to do full visibility test
public:
	struct ExcludedWaypoints
	{
		TStaticArray< Bool, MAX_WAYPOINTS >	m_excluded;
	
		ExcludedWaypoints( CPartySpawnPointComponent* partySpawnPoint );
	};

#ifndef RED_FINAL_BUILD
	CPartySpawnPointComponent()							{ m_notUsedByCommunity = true; }
#else
	CPartySpawnPointComponent()							{}
#endif

	CWayPointComponent* FindWaypoint( CName partyMember, ExcludedWaypoints* excludedWaypoints = NULL );			// No-const method because of access to both entityHandle and waypoint caching.
	
	void FillWaypointsData( TDynArray< struct SPartySpawnPointData >& output );
};

BEGIN_CLASS_RTTI( CPartySpawnPointComponent )
	PARENT_CLASS( CSpawnPointComponent )
	PROPERTY_INLINED( m_partyWaypoints, TXT("List of dependent waypoints. NOTICE: max list length is 256") )
END_CLASS_RTTI()
