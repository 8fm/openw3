/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "pathlib.h"
#include "pathlibAreaRes.h"
#include "pathlibNodeSet.h"
#include "pathlibNavgraph.h"
#include "pathlibObstacleSpawnContext.h"
#include "pathlibMetalinkComponent.h"


class CSimpleBufferWriter;
class CSimpleBufferReader;

namespace PathLib
{

class CComponentRuntimeProcessingContext;
class IMetalinkSetup;
class IMetalinkComponent;

////////////////////////////////////////////////////////////////////////////
// Metalink. A simple node set.
class CNavModyfication : public INodeSetPack
{
	typedef INodeSetPack Super;
public:
	typedef ExternalDepenentId Id;
	static const Id INVALID_ID			= EXTERNAL_INVALID_ID;
	static const Id MASK				= EXTERNAL_MASK_METACONNECTION;
	static const Id MASK_INDEX			= EXTERNAL_MASK_INDEX;

	struct Order
	{
		static Bool			Less( const CNavModyfication* m, Id id )		{ return m->m_id < id; }
		static Bool			Less( Id id, const CNavModyfication* m )		{ return id < m->m_id; }
		static Bool			Less( const CNavModyfication* m1, const CNavModyfication* m2 )	{ return m1->m_id < m2->m_id; }
	};

protected:
	Bool					m_isAttached;
	Bool					m_wasApplied;
	Id						m_id;
	SComponentMapping		m_mapping;
	IMetalinkSetup::Ptr		m_metalink;
public:

	CNavModyfication()
		: m_isAttached( false )
		, m_wasApplied( false )												{}
	CNavModyfication( Id id, CComponent* component );								// this constructor does all initialization stuff
	

	~CNavModyfication();

	Bool					Compute( IMetalinkComponent* component, CMetalinkConfiguration& configuration, CAreaDescription* area );
	Bool					NavgraphInjection( CNavGraph* navgraph );
	Bool					OnGraphGeneration( CNavGraph* navgraph );
	void					OnPostLoad( CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void					OnPreUnload( CAreaDescription* area );
	void					OnRemoval( CAreaDescription* area );

	Id						GetId() const									{ return m_id; }
	Bool					WasApplied() const								{ return m_wasApplied; }
	IMetalinkSetup*			GetSetup() const								{ return m_metalink.Get(); }
	const SComponentMapping& GetMapping() const								{ return m_mapping; }

	void					WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool					ReadFromBuffer( CSimpleBufferReader& reader );
	static CNavModyfication* NewFromBuffer( CSimpleBufferReader& reader );

	void					Enable( CAreaDescription* area, CComponentRuntimeProcessingContext& context );
	void					Disable( CAreaDescription* area, CComponentRuntimeProcessingContext& context );

	Bool					SimplifyGraphInput( CMetalinkConfigurationCommon& data, CAreaDescription* area, Float personalSpace, Bool initialSimplification ) const;

	CNavModyfication*		AsNavModyfication() override;
};


////////////////////////////////////////////////////////////////////////////
// Set of all metalinks on area
class CNavModyficationMap : public CVersionTracking
{
protected:
	// NOTICE: Current we hold metalinks 'by value', but its veerryy possibly subject of a change if we only got new use cases for the system.
	typedef TSortedArray< CNavModyfication*, CNavModyfication::Order > Metalinks;

	CAreaDescription*		m_area;
	Metalinks				m_metalinks;
	CNavModyfication::Id	m_prevMetalinkId;

	void					OnPreLoad( CAreaDescription* area );
	void					OnPostLoad( CComponentRuntimeProcessingContext& context );

	void					OnPreUnload( CAreaDescription* area );

	void					WriteToBuffer( CSimpleBufferWriter& writer ) const;
	Bool					ReadFromBuffer( CSimpleBufferReader& reader );

public:
	CNavModyficationMap();
	~CNavModyficationMap();

	Bool					CreateMetalink( CNavModyfication::Id id, IMetalinkComponent* component, CMetalinkConfiguration& configuration );
	void					RemoveMetalink( CNavModyfication::Id id );

	void					RemoveNotAppliedModyfications();

	void					RuntimeEnableMetalink( CNavModyfication::Id id, CComponentRuntimeProcessingContext& context );
	void					RuntimeDisableMetalink( CNavModyfication::Id id, CComponentRuntimeProcessingContext& context );

	void					NavgraphInjection( CNavGraph* navgraph );
	void					OnGraphGeneration( CNavGraph* navgraph );

	CAreaDescription*		GetArea() const										{ return m_area; }
	CNavModyfication::Id	GetUniqueMetalinkId()								{ return (++m_prevMetalinkId & CNavModyfication::MASK_INDEX) | CNavModyfication::MASK; }
};



};			// namespace PathLib

