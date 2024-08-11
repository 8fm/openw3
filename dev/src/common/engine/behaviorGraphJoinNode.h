
#pragma once

#include "behaviorGraphNode.h"
#include "allocatedBehaviorGraphOutput.h"
#include "behaviorGraphValueNode.h"

class CBehaviorGraphJoinNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphJoinNode, CBehaviorGraphNode, "Blends", "Join" );	

protected:
	TInstanceVar< CAllocatedBehaviorGraphOutput >	i_pose;
	TInstanceVar< Uint32 >							i_sampleMarker;
	
protected:
	CBehaviorGraphNode*		m_cachedInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool PreloadAnimations( CBehaviorGraphInstance& instance ) const;

protected:
	void CreatePose( CBehaviorGraphInstance& instance ) const;
	void DestroyPose( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphJoinNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY( m_cachedInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphInjectorNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphInjectorNode, CBehaviorGraphBaseNode, "Blends", "Injector" );

protected:
	TInstanceVar< Float >		i_weight;

protected:
	CBehaviorGraphNode*			m_cachedInjectorNode;
	CBehaviorGraphValueNode*	m_cachedControlNode;

public:

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Injector"); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphInjectorNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedInjectorNode );
	PROPERTY( m_cachedControlNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class IBehaviorGraphStaticCondition : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IBehaviorGraphStaticCondition, CObject );

public:
	virtual Bool Check( const CBehaviorGraphInstance& instance ) const = 0;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const = 0;
#endif
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehaviorGraphStaticCondition );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

class CBehaviorGraphStaticConditionNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStaticConditionNode, CBehaviorGraphNode, "Blends", "Static Condition" );

private:
	IBehaviorGraphStaticCondition* m_condition;

private:
	TInstanceVar< Bool >	i_conditionValue;

protected:
	CBehaviorGraphNode*		m_cachedInputANode;
	CBehaviorGraphNode*		m_cachedInputBNode;

public:
	CBehaviorGraphStaticConditionNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets() override;
	virtual String GetCaption() const override;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const override;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const override;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void CacheConnections() override;

private:
	Bool CheckStaticCondition( const CBehaviorGraphInstance& instance ) const;
	const CBehaviorGraphNode* GetNode( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStaticConditionNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_INLINED( m_condition, TXT("Condition") );
	PROPERTY( m_cachedInputANode );
	PROPERTY( m_cachedInputBNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStaticCondition_AnimTag : public IBehaviorGraphStaticCondition
{
	DECLARE_ENGINE_CLASS( CBehaviorGraphStaticCondition_AnimTag, IBehaviorGraphStaticCondition, 0 );

protected:
	CName	m_animTag;

public:
	virtual Bool Check( const CBehaviorGraphInstance& instance ) const override;

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override;
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphStaticCondition_AnimTag );
	PARENT_CLASS( IBehaviorGraphStaticCondition );
	PROPERTY_EDIT( m_animTag, TXT("What is your character 'AnimTag'?") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
