#pragma once

#include "behTreeNodeConditionCheckRotationToTarget.h"

class CBehTreeNodeConditionIsInAttackRangeInstance;
////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionIsInAttackRangeDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionIsInAttackRangeDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionIsInAttackRangeInstance, ConditionIsInAttackRange );
protected:	

	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	
	CBehTreeValCName				m_attackRangeName;
	CBehTreeValFloat				m_predictPositionInTime;
public:
	CBehTreeNodeConditionIsInAttackRangeDefinition()
		: m_predictPositionInTime( -1.f )								{}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionIsInAttackRangeDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_attackRangeName, TXT("Attack range name") )
	PROPERTY_EDIT( m_predictPositionInTime, TXT("Predict target position in time (< 0 for no prediction)") )
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
class CBehTreeNodeConditionIsInAttackRangeInstance : public CBehTreeNodeConditionInstance
{
protected:
	CName						m_attackRangeName;
	THandle< CAIAttackRange >	m_attackRange;
	Float						m_predictPositionInTime;

	Bool		ConditionCheck() override;
public:
	typedef CBehTreeNodeConditionIsInAttackRangeDefinition Definition;

	CBehTreeNodeConditionIsInAttackRangeInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
};