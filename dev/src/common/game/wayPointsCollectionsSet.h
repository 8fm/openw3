/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CWayPointsCollectionsSet : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CWayPointsCollectionsSet, CResource, "redwpset", "Cooked waypoints collections collection" );
protected:
	THashMap< CGUID, THandle< CWayPointsCollection > >		m_collections;

public:
	CWayPointsCollectionsSet();
	~CWayPointsCollectionsSet();

	CWayPointsCollection*		GetWayPointsCollection( const CGUID& guid ) const;
	void						StoreWayPointsCollection( const CGUID& guid, CWayPointsCollection* waypoints );

	virtual void				OnSerialize( IFile& file ) override;
};


BEGIN_CLASS_RTTI( CWayPointsCollectionsSet )
	PARENT_CLASS( CResource )
END_CLASS_RTTI()