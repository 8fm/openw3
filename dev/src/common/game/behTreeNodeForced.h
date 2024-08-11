#pragma once

#include "behTreeDynamicNode.h"

class CBehTreeNodeBaseForcedBehaviorInstance;


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseForcedBehaviorDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeBaseForcedBehaviorDefinition : public IBehTreeDynamicNodeBaseDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( CBehTreeNodeBaseForcedBehaviorDefinition, IBehTreeDynamicNodeBaseDefinition, CBehTreeNodeBaseForcedBehaviorInstance, BaseForcedBehavior );
public:
	CBehTreeNodeBaseForcedBehaviorDefinition(){}

	IAITree* GetBaseDefinition( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	virtual CName GetCancelEventName() const = 0;
};


BEGIN_ABSTRACT_CLASS_RTTI( CBehTreeNodeBaseForcedBehaviorDefinition );
	PARENT_CLASS( IBehTreeDynamicNodeBaseDefinition );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeBaseForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeBaseForcedBehaviorInstance : public CBehTreeDynamicNodeInstance
{
private:
	typedef CBehTreeDynamicNodeInstance Super;
protected:
	Priority							m_overridenPriority;
	Bool								m_cancelBehaviorRequested;
	Int16								m_currentUniqueBehaviorId;
	EAIForcedBehaviorInterruptionLevel	m_interruptionLevel;
	CName								m_cancelEventName;
public:
	typedef CBehTreeNodeBaseForcedBehaviorDefinition Definition;

	CBehTreeNodeBaseForcedBehaviorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );

	void					OnDestruction() override;

	void					Update() override;

	////////////////////////////////////////////////////////////////////
	//! Event handling
	Bool					OnEvent( CBehTreeEvent& e ) override;
	Bool					OnListenedEvent( CBehTreeEvent& e ) override;

	////////////////////////////////////////////////////////////////////
	//! Evaluation
	Bool					IsAvailable() override;
	Int32					Evaluate() override;
	void					OnSubgoalCompleted( eTaskOutcome outcome ) override;
};



////////////////////////////////////////////////////////////////////////
// CBehTreeNodeForcedBehaviorDefinition
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeForcedBehaviorInstance;
class CBehTreeNodeForcedBehaviorDefinition : public CBehTreeNodeBaseForcedBehaviorDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeNodeForcedBehaviorDefinition, CBehTreeNodeBaseForcedBehaviorDefinition, CBehTreeNodeForcedBehaviorInstance, ForcedBehavior );
public:
	CBehTreeNodeForcedBehaviorDefinition(){}

	CName GetEventName( CBehTreeInstance* owner, CBehTreeSpawnContext& context ) const override;
	IBehTreeNodeInstance* SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
private:
	CName GetCancelEventName( ) const override;
};


BEGIN_CLASS_RTTI( CBehTreeNodeForcedBehaviorDefinition );
	PARENT_CLASS( CBehTreeNodeBaseForcedBehaviorDefinition );
	BEHTREE_HIDE_PRIORITY;
END_CLASS_RTTI();


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeForcedBehaviorInstance
////////////////////////////////////////////////////////////////////////
class CBehTreeNodeForcedBehaviorInstance : public CBehTreeNodeBaseForcedBehaviorInstance
{
private:
	typedef CBehTreeNodeBaseForcedBehaviorInstance Super;
protected:
	
public:
	typedef CBehTreeNodeForcedBehaviorDefinition Definition;

	CBehTreeNodeForcedBehaviorInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent );
};