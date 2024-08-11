#pragma once

#include "aiStorage.h"
#include "behTreeVars.h"

class CBehTreeSteeringGraphCommonDef;
class CBehTreeSteeringGraphCommonInstance;

////////////////////////////////////////////////////////////////////////////
// Common stuff
////////////////////////////////////////////////////////////////////////////
// CAISteeringGraphData - ai storage data object that keeps reference to
// steering graph (and its instance buffer).
struct CAISteeringGraphData : public CAIStorageItemVirtualInterface
{
	friend class CAISteeringGraphDataPtr;
	DECLARE_RTTI_STRUCT( CAISteeringGraphData );

public:
	typedef Uint32 GraphIndex;
	typedef Uint32 OwnerId;
	static const GraphIndex INVALID_INDEX = 0xffffffff;

	struct GraphData
	{
		CMoveSteeringBehavior*					m_steeringGraph;
		InstanceBuffer*							m_instanceBuffer;
	};
	struct ActivationStackEntry
	{
		GraphIndex								m_graphIndex;
		OwnerId									m_holder;
	};
protected:
	TDynArray< GraphData >				m_graphs;
	CMovingAgentComponent*				m_mac;
	TDynArray< ActivationStackEntry >	m_activationsStack;
	OwnerId								m_nextUniqueOwnersId;

public:
	CAISteeringGraphData();
	~CAISteeringGraphData();

	GraphIndex	Add( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner );
	GraphData&	Get( GraphIndex i )											{ return m_graphs[ i ]; }
	OwnerId		GetUniqueOwnerId()											{ return ++m_nextUniqueOwnersId; }

	void		ActivateGraph( CMovingAgentComponent* mac, GraphIndex idx, OwnerId ownerId );
	void		DeactivateGraph( CMovingAgentComponent* mac, OwnerId ownerId );
	void		CustomSerialize( IFile& file ) override;

	class CInitializer : public CAIStorageItemVirtualInterface::CInitializer
	{
		typedef CAIStorageItemVirtualInterface::CInitializer Super;
	public:
		CName GetItemName() const override;
		void InitializeItem( CAIStorageItem& item ) const override;
		IRTTIType* GetItemType() const override;
	};
};

BEGIN_NODEFAULT_CLASS_RTTI( CAISteeringGraphData );
END_CLASS_RTTI();

////////////////////////////////////////////////////////////////////////////
// CBehTreeSteeringGraphCommonDef - common part of each beh tree node
// definition to use steering graph functionality (there will be some).
class CBehTreeSteeringGraphCommonDef
{
	friend class CBehTreeSteeringGraphCommonInstance;
public:
	CBehTreeSteeringGraphCommonDef()
		: m_steeringGraph()													{}

protected:
	CBehTreeValSteeringGraph			m_steeringGraph;
};

////////////////////////////////////////////////////////////////////////////
// CAISteeringGraphDataPtr - ai storage smart pointer to
// CAISteeringGraphData.
class CAISteeringGraphDataPtr : public TAIStoragePtr< CAISteeringGraphData >
{
	typedef TAIStoragePtr< CAISteeringGraphData > Super;
protected:
	CAISteeringGraphData::GraphIndex			m_index;
	CAISteeringGraphData::OwnerId				m_uniqueId;
public:
	CAISteeringGraphDataPtr( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner );

	CAISteeringGraphDataPtr()
		: Super()
		, m_index( CAISteeringGraphData::INVALID_INDEX )
		, m_uniqueId( 0xffffffff )											{}
	CAISteeringGraphDataPtr( const CAISteeringGraphDataPtr& p )
		: Super( p )
		, m_index( p.m_index )
		, m_uniqueId( p.m_uniqueId )										{}

	CAISteeringGraphDataPtr( CAISteeringGraphDataPtr&& p )
		: Super( Move( p ) )
		, m_index( p.m_index )
		, m_uniqueId( p.m_uniqueId )										{}

	CAISteeringGraphDataPtr& operator=( const CAISteeringGraphDataPtr& rhs )
	{
		Super::operator=( rhs );

		// Clobber regardless of self-assignment
		m_index = rhs.m_index;
		m_uniqueId = rhs.m_uniqueId;
		
		return *this;
	}

	CAISteeringGraphData::GraphIndex	GetIndex() const					{ return m_index; }
	CAISteeringGraphData::OwnerId		GetUid() const						{ return m_uniqueId; }
};

////////////////////////////////////////////////////////////////////////////
// CBehTreeSteeringInstance - common part of each beh tree node
// instance to use steering graph functionality.
class CBehTreeSteeringGraphCommonInstance
{
protected:
	CAISteeringGraphDataPtr				m_data;

	void		ActivateSteering( CBehTreeInstance* owner );
	void		DeactivateSteering( CBehTreeInstance* owner );

	Bool		IsSteeringGraphInitialized()								{ return m_data; }
	void		InitializeSteeringGraph( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner );
public:
	CBehTreeSteeringGraphCommonInstance()									{}

	CBehTreeSteeringGraphCommonInstance( const CBehTreeSteeringGraphCommonDef& def, CBehTreeInstance* owner, const CBehTreeSpawnContext& context );
	CBehTreeSteeringGraphCommonInstance( CMoveSteeringBehavior* steeringGraph, CBehTreeInstance* owner );
};


////////////////////////////////////////////////////////////////////////////
// Decorator