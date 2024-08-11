/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibConnectorsBin.h"

#include "pathlibObstacleSpawnContext.h"


namespace PathLib
{

///////////////////////////////////////////////////////////////////////////////
// CGlobalConnectorsBin
///////////////////////////////////////////////////////////////////////////////
CGlobalConnectorsBin::CGlobalConnectorsBin()
	: m_areConnectorsSorted( true )
{

}

CGlobalConnectorsBin::~CGlobalConnectorsBin()
{
	for ( auto pair : m_metalinkData )
	{
		delete pair.m_second;
	}
}

void CGlobalConnectorsBin::StoreConnection( Connection& connection )
{
	Lock lock( m_mutex );

	ASSERT( connection.m_areaFrom != connection.m_areaTo );

	m_connectors.PushBack( connection );
	m_areConnectorsSorted = false;
}

void CGlobalConnectorsBin::StoreMetalinkData( const MetalinkId& metalink, CMetalinkComputedData* data )
{
	Lock lock( m_mutex );

	ASSERT( m_metalinkData.Find( metalink ) == m_metalinkData.End() );
	m_metalinkData.Insert( metalink, data );
}
CMetalinkComputedData* CGlobalConnectorsBin::GetMetalinkData( const MetalinkId& metalink )
{
	// NOTICE: we never 'get' in same processing layer as 'store' so we dont use locking here

	auto itFind = m_metalinkData.Find( metalink );
	if ( itFind == m_metalinkData.End() )
	{
		return nullptr;
	}
	return itFind->m_second;
}

///////////////////////////////////////////////////////////////////////////////
// CGlobalConnectorsBin::Iterator
///////////////////////////////////////////////////////////////////////////////

CGlobalConnectorsBin::Iterator::Iterator( CGlobalConnectorsBin& bin, Uint32 category, AreaId areaFrom, AreaId areaTo )
{
	if ( !bin.m_areConnectorsSorted )
	{
		bin.m_areConnectorsSorted = true;
		bin.m_connectors.Sort();
	}
	Connection data;

	data.m_category = category;
	data.m_areaFrom = areaFrom;
	data.m_areaTo = areaTo;

	ConnectionsArray::SPred pred;

	data.m_idFrom = CPathNode::Id::VALUE_MIN;

	auto& connectors = bin.m_connectors;

	m_begin = ::LowerBound( connectors.Begin(), connectors.End(), data, pred );

	data.m_idFrom = CPathNode::Id::VALUE_MAX;

	m_end = ::UpperBound( connectors.Begin(), connectors.End(), data, pred );

	m_it = m_begin;
}

CGlobalConnectorsBin::Iterator::Iterator( CGlobalConnectorsBin& bin, AreaId areaFrom )
{
	if ( !bin.m_areConnectorsSorted )
	{
		bin.m_areConnectorsSorted = true;
		bin.m_connectors.Sort();
	}
	Connection data;

	data.m_category = 0;
	data.m_areaFrom = areaFrom;
	data.m_areaTo = NumericLimits< AreaId >::Min();

	ConnectionsArray::SPred pred;

	data.m_idFrom = CPathNode::Id::VALUE_MIN;

	auto& connectors = bin.m_connectors;

	m_begin = ::LowerBound( connectors.Begin(), connectors.End(), data, pred );

	data.m_category = 0xffff;
	data.m_areaTo = NumericLimits< AreaId >::Max();
	data.m_idFrom = CPathNode::Id::VALUE_MAX;

	m_end = ::UpperBound( connectors.Begin(), connectors.End(), data, pred );

	m_it = m_begin;
}


};			// namespace PathLib


