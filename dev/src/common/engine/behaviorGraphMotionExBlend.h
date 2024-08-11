/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphMotionExBlendNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionExBlendNode, CBehaviorGraphNode, "Blends", "Blend motion ex" );

protected:
	Float						m_threshold;

protected:
	TInstanceVar< Float	>		i_control;
	TInstanceVar< Float	>		i_speed;
	TInstanceVar< Float	>		i_weight;

	CBehaviorGraphNode*			m_cachedFirstInputNode;
	CBehaviorGraphNode*			m_cachedSecondInputNode;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;
	CBehaviorGraphValueNode*	m_cachedSpeedVariableNode;

public:
	CBehaviorGraphMotionExBlendNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Motion ex blend"); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

protected:
	Float GetWeightFromSpeed( const SBehaviorGraphOutput& poseA, const SBehaviorGraphOutput& poseB, Float speed ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionExBlendNode );
	PARENT_CLASS( CBehaviorGraphNode );	
	PROPERTY( m_cachedFirstInputNode );
	PROPERTY( m_cachedSecondInputNode );
	PROPERTY( m_cachedControlVariableNode );
	PROPERTY( m_cachedSpeedVariableNode );
	PROPERTY_EDIT_RANGE( m_threshold, TXT(""), 0.f, FLT_MAX );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMotionExValueNode	: public CBehaviorGraphValueNode
										, public IBehaviorGraphProperty
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionExValueNode, CBehaviorGraphValueNode, "Float", "Motion ex" );		

protected:
	TInstanceVar< Float >		i_value;

protected:
	CName						m_animation;

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const {}
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	//! IBehaviorGraphProperty implementation
	virtual CBehaviorGraph* GetParentGraph();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionExValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_CUSTOM_EDIT( m_animation, TXT("Animation name"), TXT("BehaviorAnimSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMotionRotChangeValueNode : public CBehaviorGraphValueNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionRotChangeValueNode, CBehaviorGraphValueNode, "Misc", "Motion rot change" );

protected:
	Bool					m_analizeMotionEx;
	Bool					m_radOrDeg;

protected:
	TInstanceVar< Vector >	i_currMotion;
	TInstanceVar< Vector >	i_prevMotion;
	TInstanceVar< Float >	i_value;

protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
	CBehaviorGraphMotionRotChangeValueNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionRotChangeValueNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY( m_cachedInputNode );
	PROPERTY_EDIT( m_analizeMotionEx, TXT("true = motion extraction, false = actor's heading") );
	PROPERTY_EDIT( m_radOrDeg, TXT("true = rad, false = deg") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
