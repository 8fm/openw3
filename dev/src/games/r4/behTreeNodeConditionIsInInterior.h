/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "../../common/game/behTreeNodeCondition.h"
#include "../../common/game/behTreeWorkData.h"


class IBehTreeNodeConditionIsInInteriorBaseInstance;
class CBehTreeNodeConditionAmIInInteriorInstance;
class CBehTreeNodeConditionIsPlayerInInteriorInstance;

///////////////////////////////////////////////////////////////////////////////
// Base abstract class for is-in-interior tests
///////////////////////////////////////////////////////////////////////////////
class IBehTreeNodeConditionIsInInteriorBaseDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeConditionIsInInteriorBaseDefinition, CBehTreeNodeConditionDefinition, IBehTreeNodeConditionIsInInteriorBaseInstance, ConditionIsInInterior )
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeConditionIsInInteriorBaseDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )
END_CLASS_RTTI()

class IBehTreeNodeConditionIsInInteriorBaseInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
public:
	typedef IBehTreeNodeConditionIsInInteriorBaseDefinition Definition;

	IBehTreeNodeConditionIsInInteriorBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};


///////////////////////////////////////////////////////////////////////////////
// Is NPC in interior
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionAmIInInteriorDefinition : public IBehTreeNodeConditionIsInInteriorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionAmIInInteriorDefinition, IBehTreeNodeConditionIsInInteriorBaseDefinition, CBehTreeNodeConditionAmIInInteriorInstance, ConditionAmIInInterior )
protected:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionAmIInInteriorDefinition )
	PARENT_CLASS( IBehTreeNodeConditionIsInInteriorBaseDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionAmIInInteriorInstance : public IBehTreeNodeConditionIsInInteriorBaseInstance
{
	typedef IBehTreeNodeConditionIsInInteriorBaseInstance Super;
protected:
		Bool					ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionAmIInInteriorDefinition Definition;

	CBehTreeNodeConditionAmIInInteriorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};

///////////////////////////////////////////////////////////////////////////////
// Is Player in interior
///////////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsPlayerInInteriorDefinition : public IBehTreeNodeConditionIsInInteriorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsPlayerInInteriorDefinition, IBehTreeNodeConditionIsInInteriorBaseDefinition, CBehTreeNodeConditionIsPlayerInInteriorInstance, ConditionIsPlayerInInterior )
protected:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsPlayerInInteriorDefinition )
	PARENT_CLASS( IBehTreeNodeConditionIsInInteriorBaseDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionIsPlayerInInteriorInstance : public IBehTreeNodeConditionIsInInteriorBaseInstance
{
	typedef IBehTreeNodeConditionIsInInteriorBaseInstance Super;
protected:
	Bool					ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsPlayerInInteriorDefinition Definition;

	CBehTreeNodeConditionIsPlayerInInteriorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}
};

class CBehTreeNodeConditionAmIOrAPInInteriorDefinition : public IBehTreeNodeConditionIsInInteriorBaseDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionAmIOrAPInInteriorDefinition, IBehTreeNodeConditionIsInInteriorBaseDefinition, class CBehTreeNodeConditionAmIOrAPInInteriorInstance, ConditionalAmIOrAPInInterior );
protected:
	IBehTreeNodeDecoratorInstance* SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionAmIOrAPInInteriorDefinition )
	PARENT_CLASS( IBehTreeNodeConditionIsInInteriorBaseDefinition )
END_CLASS_RTTI()

class CBehTreeNodeConditionAmIOrAPInInteriorInstance : public IBehTreeNodeConditionIsInInteriorBaseInstance
{
	typedef IBehTreeNodeConditionIsInInteriorBaseInstance Super;
protected:
	CBehTreeWorkDataPtr	 m_workData;
protected:
	Bool					ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionAmIOrAPInInteriorDefinition Definition;

	CBehTreeNodeConditionAmIOrAPInInteriorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};

class CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition : public IBehTreeNodeDecoratorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition, IBehTreeNodeDecoratorDefinition, class CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance, IgnoreInteriorsDuringPathfinding );
protected:
	virtual IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const  override;
};

BEGIN_CLASS_RTTI( CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition )
	PARENT_CLASS( IBehTreeNodeDecoratorDefinition )
END_CLASS_RTTI()

class CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance : public IBehTreeNodeDecoratorInstance
{
	typedef IBehTreeNodeDecoratorInstance Super;
protected:
	virtual Bool Activate() override;
	virtual void Deactivate() override;

public:
	typedef CBehTreeNodeIgnoreInteriorsDuringPathfindingDefinition Definition;

	CBehTreeNodeIgnoreInteriorsDuringPathfindingInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
		: Super( def, owner, context, parent )									{}

private:
	class CPathAgent* GetPathAgent() const;
};