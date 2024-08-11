#pragma once

#include "behTreeNodeAtomicAction.h"
#include "aiPositioning.h"

class CBehTreeNodeAtomicTeleportInstance;
class CBehTreeNodeTeleportToActionTargetInstance;
class CBehTreeNodeTeleportToActionTargetCheckPositionInstance;


////////////////////////////////////////////////////////////////////////
// Generic teleportation goal
class CBehTreeNodeAtomicTeleportDefinition : public CBehTreeNodeAtomicActionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeAtomicTeleportDefinition, CBehTreeNodeAtomicActionDefinition, CBehTreeNodeAtomicTeleportInstance, AtomicTeleport );
};

BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeAtomicTeleportDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicActionDefinition );
END_CLASS_RTTI();

class CBehTreeNodeAtomicTeleportInstance : public CBehTreeNodeAtomicActionInstance
{
	typedef CBehTreeNodeAtomicActionInstance Super;
protected:
	virtual Bool ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) = 0;
public:
	typedef CBehTreeNodeAtomicTeleportDefinition Definition;

	CBehTreeNodeAtomicTeleportInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}

	void Update() override;
};

////////////////////////////////////////////////////////////////////////
// Generic teleport to action target
class CBehTreeNodeTeleportToActionTargetDefinition : public CBehTreeNodeAtomicTeleportDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeTeleportToActionTargetDefinition, CBehTreeNodeAtomicTeleportDefinition, CBehTreeNodeTeleportToActionTargetInstance, TeleportToActionTarget )
public:
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeTeleportToActionTargetDefinition );
	PARENT_CLASS( CBehTreeNodeAtomicTeleportDefinition );
END_CLASS_RTTI();


class CBehTreeNodeTeleportToActionTargetInstance : public CBehTreeNodeAtomicTeleportInstance
{
	typedef CBehTreeNodeAtomicTeleportInstance Super;
protected:
	Bool ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
public:
	typedef CBehTreeNodeTeleportToActionTargetDefinition Definition;

	CBehTreeNodeTeleportToActionTargetInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// Teleport to action target, but keep some base positioning filters
class CBehTreeNodeTeleportToActionTargetCheckPositionDefinition : public CBehTreeNodeTeleportToActionTargetDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeTeleportToActionTargetCheckPositionDefinition, CBehTreeNodeTeleportToActionTargetDefinition, CBehTreeNodeTeleportToActionTargetCheckPositionInstance, TeleportToActionTargetAndCheckPosition )
protected:
	CBehTreeValFloat					m_queryDelay;
	SPositioningFilter					m_filter;
public:
	CBehTreeNodeTeleportToActionTargetCheckPositionDefinition()
		: m_filter()
		, m_queryDelay( 5.f )											{}

	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeTeleportToActionTargetCheckPositionDefinition )
	PARENT_CLASS( CBehTreeNodeTeleportToActionTargetDefinition )
	PROPERTY_EDIT( m_queryDelay, TXT("Min delay between queries (performance reasons)") )
	PROPERTY_EDIT( m_filter, TXT("Position filter") )
END_CLASS_RTTI()

class CBehTreeNodeTeleportToActionTargetCheckPositionInstance : public CBehTreeNodeTeleportToActionTargetInstance
{
	typedef CBehTreeNodeTeleportToActionTargetInstance Super;
protected:
	CPositioningFilterRequest::Ptr		m_queryRequest;
	Float								m_queryRequestValidTimeout;
	Float								m_queryLockTimeout;
	Float								m_queryDelay;
	SPositioningFilter					m_filter;
	
	void LazySpawnQueryRequest();
	Bool FindSpot();

	Bool ComputeTargetAndHeading( Vector& outTarget, Float& outHeading ) override;
public:
	typedef CBehTreeNodeTeleportToActionTargetCheckPositionDefinition Definition;

	CBehTreeNodeTeleportToActionTargetCheckPositionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_queryRequestValidTimeout( 0.f )
		, m_queryLockTimeout( 0.f )
		, m_queryDelay( def.m_queryDelay.GetVal( context ) )
		, m_filter( def.m_filter )										{}

	Bool IsAvailable() override;
	Int32 Evaluate() override;

	void Deactivate() override;
};