/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

class CBehaviorGraphBlendAdditiveNode : public CBehaviorGraphNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphBlendAdditiveNode, CBehaviorGraphNode, "Blends", "Blend additive" );

protected:
	Float						m_biasValue;
	Float						m_scaleValue;
	Bool						m_synchronize;
	IBehaviorSyncMethod*		m_syncMethod;
	EAdditiveType				m_type;

protected:
	TInstanceVar< Float	>		i_controlValue;
	TInstanceVar< Float >		i_prevControlValue;

protected:
	CBehaviorGraphNode*			m_cachedInputNode;
	CBehaviorGraphNode*			m_cachedAddedInputNode;
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;

public:
	CBehaviorGraphBlendAdditiveNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Blend additive"); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void Synchronize( CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;
	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;	

	virtual void CacheConnections();

protected:
	Float GetAlphaValue( Float varValue ) const;
	Bool IsAddedInputActive( Float var ) const;

	void ProcessActivations( CBehaviorGraphInstance& instance ) const;
	void UpdateControlValue( CBehaviorGraphInstance& instance ) const;

	static const Float ACTIVATION_THRESHOLD;
};

BEGIN_CLASS_RTTI( CBehaviorGraphBlendAdditiveNode );
	PARENT_CLASS( CBehaviorGraphNode );
	PROPERTY_EDIT( m_type, TXT("") );
	PROPERTY_EDIT( m_synchronize, TXT("Synchronize child playback") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_biasValue, TXT("Bias applied to control value before multipling second anim") );
	PROPERTY_EDIT( m_scaleValue, TXT("Scale applied to control value before multipling second anim") );		
	PROPERTY( m_cachedInputNode );
	PROPERTY( m_cachedAddedInputNode );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();


