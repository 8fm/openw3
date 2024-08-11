/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorIncludes.h"

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAlignToGroundNode;

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphAlignToGroundNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphAlignToGroundNode, CBehaviorGraphBaseNode, "Ragdoll", "Align to ground" );

protected:
	Float m_groundNormalBlendTime;
	Float m_additionalOffset;
	Bool m_eatEvent;

protected:
	TInstanceVar< Float > i_timeDelta;
	TInstanceVar< Vector > i_groundNormalWS;
	TInstanceVar< Vector > i_groundNormalTargetWS;
	TInstanceVar< Vector > i_groundOffsetWS;
	TInstanceVar< Vector > i_groundOffsetTargetWS;
	TInstanceVar< Float > i_alignToGroundWeight;
	TInstanceVar< Vector > i_lastGroundNormalCheckLocationWS;

public:
	CBehaviorGraphAlignToGroundNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return TXT("Align to ground"); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;

	virtual void OnGenerateFragments( CBehaviorGraphInstance& instance, CRenderFrame* frame ) const;

private:
	void FindGroundNormal( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphAlignToGroundNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_groundNormalBlendTime, TXT("") );
	PROPERTY_EDIT( m_additionalOffset, TXT("") );
	PROPERTY_EDIT( m_eatEvent, TXT("") );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
