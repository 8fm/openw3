/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"
#include "behaviorGraphTransitionNode.h"

class CBehaviorGraphComparatorNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphComparatorNode, CBehaviorGraphValueNode, "Float", "Compare" );			

protected:
	Float			m_firstValue;
	Float			m_secondValue;
	Float			m_trueValue;
	Float			m_falseValue;
	ECompareFunc	m_operation;

protected:
	TInstanceVar< Bool >			i_conditionFulfilled;

protected:
	CBehaviorGraphValueNode*		m_cachedFirstInputNode;
	CBehaviorGraphValueNode*		m_cachedSecondInputNode;
	CBehaviorGraphValueNode*		m_cachedTrueInputNode;
	CBehaviorGraphValueNode*		m_cachedFalseInputNode;

public:
	CBehaviorGraphComparatorNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	virtual void UpdateCondition( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphComparatorNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_EDIT( m_operation, TXT("Operation type") );
	PROPERTY_EDIT( m_firstValue, TXT("Fist argument") );
	PROPERTY_EDIT( m_secondValue, TXT("Second argument") );
	PROPERTY_EDIT( m_trueValue, TXT("True value") );
	PROPERTY_EDIT( m_falseValue, TXT("False value") );
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
	PROPERTY( m_cachedTrueInputNode );
	PROPERTY( m_cachedFalseInputNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
class CBehaviorGraphEnumComparatorNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphEnumComparatorNode, CBehaviorGraphValueNode, "Float", "Compare enum" );			

protected:	
	CVariant					m_enumValue;
	ECompareFunc				m_operation;	

	TInstanceVar< Float >		i_enumValueVar;

protected:
	CBehaviorGraphValueNode*	m_cachedFirstInputNode;

public:
	CBehaviorGraphEnumComparatorNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	Int32 ReadVariantEnum( CBehaviorGraphInstance& instance, const CVariant& variant ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphEnumComparatorNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_CUSTOM_EDIT( m_enumValue, TXT("Enumerated value"), TXT("VariantEnumEditor") );
	PROPERTY_EDIT( m_operation, TXT("Operation type") );
	PROPERTY( m_cachedFirstInputNode );		
END_CLASS_RTTI();