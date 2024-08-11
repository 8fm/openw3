/**
* Copyright © 2011 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class CBehaviorGraphValueNode;

class CBehaviorGraphPoseConstraintNode	: public CBehaviorGraphBaseNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseConstraintNode, CBehaviorGraphBaseNode, "Pose Constraints", "Base" );

protected:
	TInstanceVar< Float >		i_dt;
	TInstanceVar< Float >		i_weight;

protected:
	CBehaviorGraphValueNode*	m_cachedControlValueNode;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
	Float GetTimeDelta( CBehaviorGraphInstance& instance ) const;
	Float GetWeight( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseConstraintNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedControlValueNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPoseConstraintWithTargetNode : public CBehaviorGraphPoseConstraintNode
{	
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPoseConstraintWithTargetNode, CBehaviorGraphPoseConstraintNode, "Pose Constraints", "Pose with Target" );

protected:
	TInstanceVar< Vector >			i_targetPos;
	TInstanceVar< EulerAngles >		i_targetRot;

protected:
	CBehaviorGraphVectorValueNode*	m_cachedTargetPosValueNode;
	CBehaviorGraphVectorValueNode*	m_cachedTargetRotValueNode;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

	virtual void CacheConnections();

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

protected:
	Bool GetTarget( CBehaviorGraphInstance& instance, Vector& pos, EulerAngles& rot ) const;
	const Vector& GetTargetPos( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPoseConstraintWithTargetNode );
	PARENT_CLASS( CBehaviorGraphPoseConstraintNode );
	PROPERTY( m_cachedTargetPosValueNode );
	PROPERTY( m_cachedTargetRotValueNode );
END_CLASS_RTTI();
