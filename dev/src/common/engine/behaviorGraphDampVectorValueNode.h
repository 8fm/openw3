/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphDampVectorValueNode : public CBehaviorGraphVectorValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphDampVectorValueNode, CBehaviorGraphVectorValueNode, "Vector", "Damp vector value" );		

protected:	
	Vector					m_increaseSpeed;
	Vector					m_decreaseSpeed;
	Bool					m_absolute;

protected:
	TInstanceVar< Vector > i_vecValue;
	TInstanceVar< Vector > i_increaseSpeed;
	TInstanceVar< Vector > i_decreaseSpeed;

protected:
	CBehaviorGraphVectorValueNode*		m_cachedInputNode;
	CBehaviorGraphVectorValueNode*		m_cachedIncSpeedNode;
	CBehaviorGraphVectorValueNode*		m_cachedDecSpeedNode;

public:
	CBehaviorGraphDampVectorValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Damp vector value") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const { return 0.f; }
	virtual Vector GetVectorValue( CBehaviorGraphInstance& instance ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	void DampValue( CBehaviorGraphInstance& instance, Float value, Int32 num, Float timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphDampVectorValueNode );
	PARENT_CLASS( CBehaviorGraphVectorValueNode );
	PROPERTY_EDIT( m_increaseSpeed, TXT("Speed of variable change in positive direction") );
	PROPERTY_EDIT( m_decreaseSpeed, TXT("Speed of variable change in negative direction") );
	PROPERTY_EDIT( m_absolute, TXT("True if we consider Abs(value) of variable change when choosing between increase or decrease speed") ); 
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedIncSpeedNode );
	PROPERTY( m_cachedDecSpeedNode );
END_CLASS_RTTI();
