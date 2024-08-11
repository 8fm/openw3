/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphRotationControllerNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRotationControllerNode, CBehaviorGraphBaseNode, "Controllers", "Rotation" );

	static const Float				ROTATION_THRESHOLD;

protected:
	CName						m_eventAllowRot;
	Bool						m_continueUpdate;

protected:
	TInstanceVar< Bool >		i_nodeActive;
	TInstanceVar< Bool >		i_allowRot;

	TInstanceVar< Float >		i_destAngle;
	TInstanceVar< Float	>		i_accRot;

	TInstanceVar< Float	>		i_timer;
	TInstanceVar< Float	>		i_rotDuration;
	TInstanceVar< Float	>		i_timeDelta;

protected:
	CBehaviorGraphValueNode*	m_cachedAngleVariableNode;
	CBehaviorGraphValueNode*	m_cachedWeightVariableNode;

public:
	CBehaviorGraphRotationControllerNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Rotation controller") ); }
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
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

protected:
	Bool IsEventOccurred( const CName& eventName, const SBehaviorGraphOutput &output, Float& outEventDuration, CBehaviorGraphInstance& instance ) const;
	void Rotate( Float angle, SBehaviorGraphOutput &output ) const;

	void ResetController( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphRotationControllerNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_eventAllowRot, TXT("") );
	PROPERTY_EDIT( m_continueUpdate, TXT("") );
	PROPERTY( m_cachedAngleVariableNode );
	PROPERTY( m_cachedWeightVariableNode );
END_CLASS_RTTI();
