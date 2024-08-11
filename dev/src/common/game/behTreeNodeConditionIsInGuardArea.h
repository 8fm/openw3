/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"
#include "behTreeGuardAreaData.h"
#include "behTreeCustomMoveData.h"

class IBehTreeNodeConditionIsInGuardAreaInstance;
class CBehTreeNodeConditionIsActionTargetInGuardAreaInstance;
class CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance;
class CBehTreeNodeConditionIsThisActorInGuardAreaInstance;

///////////////////////////////////////////////////////////////////////////////
// Base is-in-guard-area condition class
// Its only missing code that defined 'target'
class IBehTreeNodeConditionIsInGuardAreaDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionIsInGuardAreaDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeConditionIsInGuardAreaInstance, ConditionIsInGuardArea );
};

BEGIN_ABSTRACT_CLASS_RTTI(IBehTreeNodeConditionIsInGuardAreaDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
END_CLASS_RTTI();

class IBehTreeNodeConditionIsInGuardAreaInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeGuardAreaDataPtr				m_guardAreaPtr;

	virtual Bool							GetTarget( Vector& outPosition )	= 0;
	Bool									ConditionCheck() override;
public:
	typedef IBehTreeNodeConditionIsInGuardAreaDefinition Definition;

	IBehTreeNodeConditionIsInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};
///////////////////////////////////////////////////////////////////////////////
// Action target implementation
class CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition : public IBehTreeNodeConditionIsInGuardAreaDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition, IBehTreeNodeConditionIsInGuardAreaDefinition, CBehTreeNodeConditionIsActionTargetInGuardAreaInstance, ConditionIsActionTargetInGuardArea )
protected:
	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition);
	PARENT_CLASS(IBehTreeNodeConditionIsInGuardAreaDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeConditionIsActionTargetInGuardAreaInstance : public IBehTreeNodeConditionIsInGuardAreaInstance
{
	typedef IBehTreeNodeConditionIsInGuardAreaInstance Super;
protected:
	Bool									GetTarget( Vector& outPosition ) override;
public:
	typedef CBehTreeNodeConditionIsActionTargetInGuardAreaDefinition Definition;

	CBehTreeNodeConditionIsActionTargetInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};	

///////////////////////////////////////////////////////////////////////////////
// "this actor" implementation
class CBehTreeNodeConditionIsThisActorInGuardAreaDefinition : public IBehTreeNodeConditionIsInGuardAreaDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsThisActorInGuardAreaDefinition, IBehTreeNodeConditionIsInGuardAreaDefinition, CBehTreeNodeConditionIsThisActorInGuardAreaInstance, ConditionIsThisActorInGuardArea )
protected:
	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsThisActorInGuardAreaDefinition );
PARENT_CLASS(IBehTreeNodeConditionIsInGuardAreaDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeConditionIsThisActorInGuardAreaInstance : public IBehTreeNodeConditionIsInGuardAreaInstance
{
	typedef IBehTreeNodeConditionIsInGuardAreaInstance Super;
protected:
	Bool									GetTarget( Vector& outPosition ) override;
public:
	typedef CBehTreeNodeConditionIsThisActorInGuardAreaDefinition Definition;

	CBehTreeNodeConditionIsThisActorInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};	

///////////////////////////////////////////////////////////////////////////////
// Custom target implementation
class CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition : public IBehTreeNodeConditionIsInGuardAreaDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition, IBehTreeNodeConditionIsInGuardAreaDefinition, CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance, ConditionIsCustomTargetInGuardArea )
protected:
	IBehTreeNodeDecoratorInstance*			SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition);
	PARENT_CLASS(IBehTreeNodeConditionIsInGuardAreaDefinition);	
END_CLASS_RTTI();

class CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance : public IBehTreeNodeConditionIsInGuardAreaInstance
{
	typedef IBehTreeNodeConditionIsInGuardAreaInstance Super;
protected:
	CBehTreeCustomMoveDataPtr				m_customTargetPtr;

	Bool									GetTarget( Vector& outPosition ) override;
public:
	typedef CBehTreeNodeConditionIsCustomTargetInGuardAreaDefinition Definition;

	CBehTreeNodeConditionIsCustomTargetInGuardAreaInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};	