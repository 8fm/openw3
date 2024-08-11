/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "wayPointsCollectionsSet.h"

#include "wayPointsCollection.h"

IMPLEMENT_ENGINE_CLASS( CWayPointsCollectionsSet )


CWayPointsCollectionsSet::CWayPointsCollectionsSet()
{

}
CWayPointsCollectionsSet::~CWayPointsCollectionsSet()
{

}

CWayPointsCollection* CWayPointsCollectionsSet::GetWayPointsCollection( const CGUID& guid ) const
{
	auto itFind = m_collections.Find( guid );
	if ( itFind == m_collections.End() )
	{
		return nullptr;
	}
	return itFind->m_second.Get();
}
void CWayPointsCollectionsSet::StoreWayPointsCollection( const CGUID& guid, CWayPointsCollection* waypoints )
{
	m_collections[ guid ] = waypoints;
}

void CWayPointsCollectionsSet::OnSerialize( IFile& file )
{
	TBaseClass::OnSerialize( file );

	file << m_collections;
}
