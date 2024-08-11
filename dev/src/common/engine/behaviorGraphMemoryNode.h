/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphMemoryNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMemoryNode, CBehaviorGraphValueBaseNode, "Float", "Memory" );		

	Bool					m_resetOnActivation;

protected:
	TInstanceVar< Float	>	i_currValue;
	TInstanceVar< Float	>	i_prevValue;

public:
	CBehaviorGraphMemoryNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual String GetCaption() const;
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMemoryNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_resetOnActivation, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphValueAccNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphValueAccNode, CBehaviorGraphValueBaseNode, "Float", "Acc" );		

	Bool					m_resetOnActivation;
	Float					m_initValue;
	Bool					m_wrapValue;
	Float					m_wrapValueThrMax;
	Float					m_wrapValueThrMin;

protected:
	TInstanceVar< Float	> i_accValue;

public:
	CBehaviorGraphValueAccNode();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual String GetCaption() const;
	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphValueAccNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_resetOnActivation, String::EMPTY );
	PROPERTY_EDIT( m_initValue, String::EMPTY );
	PROPERTY_RO( m_wrapValue, String::EMPTY );
	PROPERTY_RO( m_wrapValueThrMax, String::EMPTY );
	PROPERTY_RO( m_wrapValueThrMin, String::EMPTY );
END_CLASS_RTTI();
