/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphAnimationNode.h"
#include "behaviorGraphMimicsAnimationNode.h"
#include "animationEvent.h"

class CBehaviorGraphInstance;

class CBehaviorGraphCutsceneControllerNode : public CBehaviorGraphMimicsAnimationNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphCutsceneControllerNode, CBehaviorGraphAnimationNode, "Misc", "Cutscene controller" );

protected:
	Bool				m_mimicControl;

protected:
	TInstanceVar< TDynArray< CSkeletalAnimationSetEntry* > >	i_blendingAnimations;
	TInstanceVar< Float >										i_blendingFactor;
	TInstanceVar< Bool >										i_gameplayMode;
	TInstanceVar< Matrix >										i_gameplayRefPosition;
	TInstanceVar< Float >										i_gameplayBlendTime;

protected:
	CBehaviorGraphNode*	m_cachedBaseInputNode;

public:
	CBehaviorGraphCutsceneControllerNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual void OnPropertyPostChange( IProperty *prop );

	virtual String GetCaption() const { return TXT("Cutscene controller"); }
	virtual Color GetTitleColor() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void CollectEvents( CBehaviorGraphInstance& instance, const CSyncInfo &info, TDynArray< CAnimationEventFired >& eventsFired ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Bool IsMimic() const { return m_mimicControl; }

public:
	Bool AddCutsceneAnimation( CBehaviorGraphInstance& instance, const CName &name ) const;
	void ResetRuntimeAnimation( CBehaviorGraphInstance& instance ) const;

	void SetBlendFactor( CBehaviorGraphInstance& instance, Float factor ) const;

	void SetGameplayMode( CBehaviorGraphInstance& instance, Bool flag, Float blendTime, const Matrix& refPosition ) const;

public:
	Bool CanWork( CBehaviorGraphInstance& instance ) const;

protected:
	void SampleBlendingAnim( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output, Float blendingFactor, const TDynArray< CSkeletalAnimationSetEntry* >& anims ) const;

public:
	virtual void OnLoadedSnapshot( CBehaviorGraphInstance& instance, const InstanceBuffer& previousData ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphCutsceneControllerNode );
	PARENT_CLASS( CBehaviorGraphAnimationNode );
	PROPERTY( m_cachedBaseInputNode );
	PROPERTY_EDIT( m_mimicControl, TXT("") );
END_CLASS_RTTI();
