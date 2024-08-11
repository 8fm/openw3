/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphSpeedModulationNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphSpeedModulationNode, CBehaviorGraphValueBaseNode, "Misc", "Modulate speed" );		

protected:
	Float						m_speedThreshold;
	Float						m_halfAngle;

protected:
	CBehaviorGraphValueNode*	m_cachedSpeedNode;
	CBehaviorGraphValueNode*	m_cachedDirectionNode;
	CBehaviorGraphValueNode*	m_cachedThresholdNode;

public:
	CBehaviorGraphSpeedModulationNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Modulate speed") ); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const {}

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphSpeedModulationNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_EDIT( m_speedThreshold, TXT( "Speed threshold") );
	PROPERTY_EDIT( m_halfAngle, TXT( "Half-angle in which the speed should not be modulated") );
	PROPERTY( m_cachedSpeedNode );
	PROPERTY( m_cachedDirectionNode );
	PROPERTY( m_cachedThresholdNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
