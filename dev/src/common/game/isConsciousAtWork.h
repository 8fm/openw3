#pragma once

#include "behTreeNodeCondition.h"

#include "behTreeWorkData.h"

class CBehTreeNodeConditionIsConsciousAtWorkInstance;
class CBehTreeNodeConditionIsWorkingInstance;

////////////////////////////////////////////////////////////////////////
// Base class definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsConsciousAtWorkDefinition : public CBehTreeNodeConditionDefinition
{	
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsConsciousAtWorkDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsConsciousAtWorkInstance, ConditionIsConcsiousAtWork );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionIsConsciousAtWorkDefinition()
	{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsConsciousAtWorkDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )	
END_CLASS_RTTI()

class CBehTreeNodeConditionIsConsciousAtWorkInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeWorkDataPtr			m_workData;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsConsciousAtWorkDefinition Definition;

	CBehTreeNodeConditionIsConsciousAtWorkInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_workData( owner ){}
};



////////////////////////////////////////////////////////////////////////
// Is working definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsWorkingDefinition : public CBehTreeNodeConditionDefinition
{	
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsWorkingDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsWorkingInstance, ConditionIsWorking );
protected:
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;

public:
	CBehTreeNodeConditionIsWorkingDefinition()
	{}
};

BEGIN_CLASS_RTTI( CBehTreeNodeConditionIsWorkingDefinition )
	PARENT_CLASS( CBehTreeNodeConditionDefinition )	
	END_CLASS_RTTI()

class CBehTreeNodeConditionIsWorkingInstance : public CBehTreeNodeConditionInstance
{
	typedef CBehTreeNodeConditionInstance Super;
protected:
	CBehTreeWorkDataPtr			m_workData;

	Bool ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsWorkingDefinition Definition;

	CBehTreeNodeConditionIsWorkingInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL )
		: Super( def, owner, context, parent )
		, m_workData( owner ){}
};