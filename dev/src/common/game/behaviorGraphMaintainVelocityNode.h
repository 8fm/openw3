/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

class CBehaviorGraphValueNode;

#include "../engine/behaviorGraphNode.h"

class CBehaviorGraphMaintainVelocityNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMaintainVelocityNode, CBehaviorGraphBaseNode, "Motion", "Maintain velocity" );

protected:
	Float m_blendTime;
	Bool m_stop;

protected:
	TInstanceVar< Vector > i_velocityWS;
	TInstanceVar< Float > i_timeDelta;

public:
	CBehaviorGraphMaintainVelocityNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Maintain velocity") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

protected:
};

BEGIN_CLASS_RTTI( CBehaviorGraphMaintainVelocityNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_blendTime, TXT("") );
	PROPERTY_EDIT( m_stop, TXT("") );
END_CLASS_RTTI();
