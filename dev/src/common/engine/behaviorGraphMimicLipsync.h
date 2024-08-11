
#pragma once

#include "behaviorGraphMimicNodes.h"

class CBehaviorGraphValueNode;

class CBehaviorGraphMimicLipsyncFilterNode : public CBehaviorGraphBaseMimicNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMimicLipsyncFilterNode, CBehaviorGraphBaseMimicNode, "Mimic", "Lipsync filter" );

protected:
	Int32		m_lipsyncControlTrack;

	Int32		m_lipsyncTrackBeginA;
	Int32		m_lipsyncTrackEndA;

	Int32		m_lipsyncTrackBeginB;
	Int32		m_lipsyncTrackEndB;

	Int32		m_lipsyncTrackBeginC;
	Int32		m_lipsyncTrackEndC;

protected:
	TInstanceVar< Bool > i_valid;

protected:
	CBehaviorGraphNode*			m_cachedFilterInputNode;
	CBehaviorGraphValueNode*	m_cachedWeightValueNode;

public:
	CBehaviorGraphMimicLipsyncFilterNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override { return String( TXT("Lipsync filter") ); }
	virtual void OnRebuildSockets() override;
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const override;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const override;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const override;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const override;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const override;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const override;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const override;

	virtual void CacheConnections() override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphMimicLipsyncFilterNode );
	PARENT_CLASS( CBehaviorGraphBaseMimicNode );
	PROPERTY_EDIT( m_lipsyncControlTrack, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackBeginA, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackEndA, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackBeginB, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackEndB, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackBeginC, TXT("") );
	PROPERTY_EDIT( m_lipsyncTrackEndC, TXT("") );
	PROPERTY( m_cachedFilterInputNode );
	PROPERTY( m_cachedWeightValueNode );
END_CLASS_RTTI();
