/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNodeCondition.h"
#include "behTreeWorkData.h"

class IBehTreeNodeWorkRelatedConditionInstance;
class CBehTreeNodeIsAtWorkConditionInstance;
class CBehTreeNodeCanUseChatSceneConditionInstance;
class CBehTreeNodeIsSittingInInteriorConditionInstance;

////////////////////////////////////////////////////////////////////////
// Abstract base class for all work-related condition nodes
////////////////////////////////////////////////////////////////////////
class IBehTreeNodeWorkRelatedConditionDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeWorkRelatedConditionDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeWorkRelatedConditionInstance, ConditionWorkRelated )
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeWorkRelatedConditionDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()

class IBehTreeNodeWorkRelatedConditionInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeWorkDataPtr			m_workDataPtr;

public:
	typedef IBehTreeNodeWorkRelatedConditionDefinition Definition;

	IBehTreeNodeWorkRelatedConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )
		, m_workDataPtr( owner )										{}
};

////////////////////////////////////////////////////////////////////////
// IsAtWork condition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeIsAtWorkConditionDefinition : public IBehTreeNodeWorkRelatedConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeIsAtWorkConditionDefinition, IBehTreeNodeWorkRelatedConditionDefinition, CBehTreeNodeIsAtWorkConditionInstance, ConditionIsAtWork )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeIsAtWorkConditionDefinition )
	PARENT_CLASS( IBehTreeNodeWorkRelatedConditionDefinition )
END_CLASS_RTTI()

class CBehTreeNodeIsAtWorkConditionInstance : public IBehTreeNodeWorkRelatedConditionInstance
{
	typedef IBehTreeNodeWorkRelatedConditionInstance Super;
protected:
	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeIsAtWorkConditionDefinition Definition;

	CBehTreeNodeIsAtWorkConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};


////////////////////////////////////////////////////////////////////////
// CanUseChatScene condition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeCanUseChatSceneConditionDefinition : public IBehTreeNodeWorkRelatedConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeCanUseChatSceneConditionDefinition, IBehTreeNodeWorkRelatedConditionDefinition, CBehTreeNodeCanUseChatSceneConditionInstance, ConditionCanUseChatSceneInAP )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeCanUseChatSceneConditionDefinition )
	PARENT_CLASS( IBehTreeNodeWorkRelatedConditionDefinition )
END_CLASS_RTTI()

class CBehTreeNodeCanUseChatSceneConditionInstance : public IBehTreeNodeWorkRelatedConditionInstance
{
	typedef IBehTreeNodeWorkRelatedConditionInstance Super;
protected:
	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeCanUseChatSceneConditionDefinition Definition;

	CBehTreeNodeCanUseChatSceneConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};

////////////////////////////////////////////////////////////////////////
// IsSittingInInterior condition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeIsSittingInInteriorConditionDefinition : public IBehTreeNodeWorkRelatedConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeIsSittingInInteriorConditionDefinition, IBehTreeNodeWorkRelatedConditionDefinition, CBehTreeNodeIsSittingInInteriorConditionInstance, ConditionIsSittingInInterior )
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeIsSittingInInteriorConditionDefinition )
	PARENT_CLASS( IBehTreeNodeWorkRelatedConditionDefinition )
	END_CLASS_RTTI()

class CBehTreeNodeIsSittingInInteriorConditionInstance : public IBehTreeNodeWorkRelatedConditionInstance
{
	typedef IBehTreeNodeWorkRelatedConditionInstance Super;
protected:
	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeIsSittingInInteriorConditionDefinition Definition;

	CBehTreeNodeIsSittingInInteriorConditionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )							{}
};
