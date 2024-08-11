#pragma once
#include "../../common/game/behTreeNodeCondition.h"

////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeConditionIsEnemyAroundInstance;
class CBehTreeNodeConditionIsEnemyAroundDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsEnemyAroundDefinition, CBehTreeNodeConditionDefinition, CBehTreeConditionIsEnemyAroundInstance, ConditionIsEnemyAround );
protected:
	Float m_maxEnemyDistance;
	Float m_updateDelay;
	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;
public:
	CBehTreeNodeConditionIsEnemyAroundDefinition( )
		:  m_maxEnemyDistance( 20.0f )
		,  m_updateDelay( 0.5f )
	{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsEnemyAroundDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_maxEnemyDistance,		TXT("Any enemy above that distance will be ignored") );
	PROPERTY_EDIT( m_updateDelay,			TXT("(s) time before the node is re-evalued") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeConditionIsEnemyAroundInstance : public CBehTreeNodeConditionInstance
{
protected:
	typedef CBehTreeNodeConditionInstance Super;
	Float m_lastUpdateTime;
	Bool m_isEnnemyAround;

	Float m_maxEnemyDistance;
	Float m_updateDelay;


	Bool ConditionCheck() override;

public:
	typedef CBehTreeNodeConditionIsEnemyAroundDefinition Definition;

	CBehTreeConditionIsEnemyAroundInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};