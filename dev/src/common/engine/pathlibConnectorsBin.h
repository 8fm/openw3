/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlibGraph.h"

namespace PathLib
{

struct CMetalinkComputedData;
	
///////////////////////////////////////////////////////////////////////////
// Global collection of metalink based connectors between areas. Generation
// only data!
class CGlobalConnectorsBin
{
public:
	struct Connection
	{
		Uint32								m_category;
		AreaId								m_areaFrom;
		AreaId								m_areaTo;
		CPathNode::Id						m_idFrom;

		Bool operator<( const Connection& c ) const
		{
			return
				m_areaFrom < c.m_areaFrom
				? true
				: m_areaFrom > c.m_areaFrom
				? false
				: m_areaTo < c.m_areaTo
				? true
				: m_areaTo > c.m_areaTo
				? false
				: m_category < c.m_category
				? true
				: m_category > c.m_category
				? false
				: m_idFrom < c.m_idFrom;
		}
	};
	struct MetalinkId
	{
		AreaId								m_area;
		ExternalDepenentId					m_navModId;
		
		Bool operator==( const MetalinkId& c ) const						{ return m_area == c.m_area && m_navModId == c.m_navModId; }
		Uint32 CalcHash() const												{ return m_navModId ^ (Uint32( m_area ) << 16); }
	};
	typedef TDynArray< CMetalinkComputedData* > MetalinkDataList;

protected:
	typedef TSortedArray< Connection > ConnectionsArray;
	typedef THashMap< MetalinkId, CMetalinkComputedData* > MetalinkData;
	typedef Red::Threads::CMutex Mutex;
	typedef Red::Threads::CScopedLock< Mutex > Lock;

	ConnectionsArray					m_connectors;
	MetalinkData						m_metalinkData;
	Bool								m_areConnectorsSorted;
	Mutex								m_mutex;

public:
							CGlobalConnectorsBin();
							~CGlobalConnectorsBin();

	void					StoreConnection( Connection& connection );

	void					StoreMetalinkData( const MetalinkId& metalink, CMetalinkComputedData* data );
	CMetalinkComputedData*	GetMetalinkData( const MetalinkId& metalink );


	///////////////////////////////////////////////////////////////////////
	struct Iterator
	{
	protected:
		ConnectionsArray::iterator				m_it;
		ConnectionsArray::iterator				m_begin;
		ConnectionsArray::iterator				m_end;

	public:
							Iterator( CGlobalConnectorsBin& bin, Uint32 category, AreaId areaFrom, AreaId areaTo );						// all connection between 2 areas
							Iterator( CGlobalConnectorsBin& bin, AreaId areaFrom );														// all connections that goes of given area

		void				Reset()											{ m_it = m_begin; }

		operator			Bool() const									{ return m_it != m_end; }
		void				operator++()									{ ++m_it; }

		Connection&			operator*() const								{ return *m_it; }
		Connection*			operator->() const								{ return &(*m_it); }
	};

};

};			// namespace PathLib
