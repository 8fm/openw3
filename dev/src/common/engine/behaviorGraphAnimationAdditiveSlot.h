
#pragma  once

#include "behaviorGraphAnimationBlendSlotNode.h"

class CBehaviorGraphAnimationAdditiveSlotNode : public CBehaviorGraphAnimationSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationAdditiveSlotNode, CBehaviorGraphAnimationSlotNode, "Slots.Animation", "Additive slot" );

protected:
	EAdditiveType			m_additiveType;

protected:
	CBehaviorGraphNode*		m_cachedBaseAnimInputNode;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
#endif

public:
	CBehaviorGraphAnimationAdditiveSlotNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationAdditiveSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationSlotNode );
	PROPERTY_EDIT( m_additiveType, TXT("") );
	PROPERTY( m_cachedBaseAnimInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAnimationSelAdditiveSlotNode : public CBehaviorGraphAnimationAdditiveSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAnimationSelAdditiveSlotNode, CBehaviorGraphAnimationAdditiveSlotNode, "Slots.Animation", "Additive sel slot" );

protected:
	String				m_firstBone;
	String				m_lastBone;

protected:
	TInstanceVar< Int32 > i_firstBoneIndex;
	TInstanceVar< Int32 > i_lastBoneIndex;

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

protected:
	virtual void AdditiveBlend( CBehaviorGraphInstance& instance, SBehaviorGraphOutput& output, SBehaviorGraphOutput& pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAnimationSelAdditiveSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationAdditiveSlotNode );
	PROPERTY_CUSTOM_EDIT( m_firstBone, TXT(""), TXT("BehaviorBoneSelection") );
	PROPERTY_CUSTOM_EDIT( m_lastBone, TXT(""), TXT("BehaviorBoneSelection") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMimicAdditiveSlotNode : public CBehaviorGraphAnimationAdditiveSlotNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicAdditiveSlotNode, CBehaviorGraphAnimationAdditiveSlotNode, "Slots.Mimic", "Additive slot" );

#ifndef NO_EDITOR_GRAPH_SUPPORT
public:
	virtual String GetCaption() const;
	virtual void OnRebuildSockets();
	virtual Color GetTitleColor() const { return Color( 128, 0, 128 ); }
#endif

public:
	virtual void CacheConnections();

	virtual Bool IsMimic() const { return true; }

protected:
	virtual Bool IsSlotPoseMimic() const;
	virtual Bool IsValid( CBehaviorGraphInstance& instance ) const;
	virtual void SampleSlotAnimation( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicAdditiveSlotNode );
	PARENT_CLASS( CBehaviorGraphAnimationAdditiveSlotNode );
END_CLASS_RTTI();
