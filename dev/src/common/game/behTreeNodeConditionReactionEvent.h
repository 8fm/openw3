#pragma once

#include "behTreeNodeCondition.h"
#include "behTreeCounterData.h"
#include "behTreeWorkData.h"

class CBehTreeReactionEventData;
class CBehTreeNodeConditionReactionEventInstance;
////////////////////////////////////////////////////////////////////////
// Definition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionReactionEventDefinition : public CBehTreeNodeConditionDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeConditionReactionEventDefinition, CBehTreeNodeConditionDefinition, CBehTreeNodeConditionReactionEventInstance, ConditionReactionEvent );
protected:	
	CBehTreeValCName m_eventName;
	CBehTreeValFloat m_cooldownDistance;
	CBehTreeValFloat m_cooldownTimeout;
	CBehTreeValBool	 m_dontSetActionTargetEdit;
	
	IBehTreeNodeDecoratorInstance*	SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const override;	

public:
	CBehTreeNodeConditionReactionEventDefinition() : m_dontSetActionTargetEdit( false ){}
};

BEGIN_CLASS_RTTI(CBehTreeNodeConditionReactionEventDefinition);
	PARENT_CLASS(CBehTreeNodeConditionDefinition);	
	PROPERTY_EDIT( m_eventName, TXT( "Name of the condition event" ) );
	PROPERTY_EDIT( m_cooldownDistance, TXT("Distance between invoker and reactee needed to allow this reaction to be run again") );
	PROPERTY_EDIT( m_cooldownTimeout, TXT("Timeout needed to allow this reaction to be run again") );
	PROPERTY_EDIT( m_dontSetActionTargetEdit, TXT("If set to true, action target will not be set to invoker") );
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// Instance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeConditionReactionEventInstance : public CBehTreeNodeConditionInstance
{
	typedef  CBehTreeNodeConditionInstance Super;
protected:
	THandle<CBehTreeReactionEventData>	m_reactionData;
	CBehTreeCounterDataPtr				m_counterData;
	CName								m_eventName;	
	Float								m_cooldownDistanceSq;
	Float								m_cooldownTimeout;
	Float								m_nextActivationTime;

	Bool ConditionCheck() override;
	void Deactivate() override;

	Bool								m_setActionTarget;
	Bool								m_dontSetActionTargetEdit;
	Bool								m_reactionTreeComplated;
	Bool								m_counterTicket;
	CBehTreeWorkDataPtr					m_workData;

	//CName								m_debugEventName;
public:
	typedef CBehTreeNodeConditionReactionEventDefinition Definition;

	CBehTreeNodeConditionReactionEventInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL );
	Bool OnListenedEvent( CBehTreeEvent& e ) override;
	Bool Activate() override;
	Bool IsAvailable() override;
	Int32 Evaluate() override;
	void OnDestruction() override;
};