/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

class CBehaviorGraphValueNode;

#include "behaviorGraphNode.h"

class CBehaviorGraphPositionControllerBaseNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPositionControllerBaseNode, CBehaviorGraphBaseNode, "Motion", "Position base" );

	static const Float				TRANSLATION_THRESHOLD;

	Bool							m_useHeading;

protected:
	TInstanceVar< Bool >			i_nodeActive;
	TInstanceVar< Vector >			i_shift;

protected:
	CBehaviorGraphValueNode*		m_cachedWeightVariableNode;
	CBehaviorGraphVectorValueNode*	m_cachedShiftVariableNode;

public:
	CBehaviorGraphPositionControllerBaseNode();

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return String( TXT("Position controller base") ); }
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
	void Translate( const Vector& shift, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

protected:
	virtual void UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ResetController( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPositionControllerBaseNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_useHeading , TXT("") );
	PROPERTY( m_cachedWeightVariableNode );
	PROPERTY( m_cachedShiftVariableNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPositionControllerNode : public CBehaviorGraphPositionControllerBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPositionControllerNode, CBehaviorGraphPositionControllerBaseNode, "Motion", "Position" );

	Bool							m_continueUpdate;
	Bool							m_useAnimEvent;
	CName							m_eventAllowTrans;

protected:
	TInstanceVar< Bool >			i_allowTrans;

	TInstanceVar< Vector >			i_accShift;

	TInstanceVar< Float	>			i_timer;
	TInstanceVar< Float	>			i_transDuration;
	TInstanceVar< Float	>			i_timeDelta;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Position controller") ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

protected:
	Bool IsEventOccurred( const CName& eventName, const SBehaviorGraphOutput &output, Float& outEventDuration, CBehaviorGraphInstance& instance ) const;

protected:
	virtual void UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ResetController( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphPositionControllerNode );
	PARENT_CLASS( CBehaviorGraphPositionControllerBaseNode );
	PROPERTY_EDIT( m_continueUpdate , TXT("") );
	PROPERTY_EDIT( m_useAnimEvent, TXT("") );
	PROPERTY_EDIT( m_eventAllowTrans, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphPositionControllerWithDampNode : public CBehaviorGraphPositionControllerBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphPositionControllerWithDampNode, CBehaviorGraphPositionControllerBaseNode, "Motion", "Position with damp" );

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnBuildInstanceProperites( InstanceBuffer& instance, CInstancePropertiesBuilder& builder ) const;

protected:
	virtual void UpdateController( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void SampleController( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ResetController( CBehaviorGraphInstance& instance ) const;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String( TXT("Position controller with damp") ); }
#endif
};

BEGIN_CLASS_RTTI( CBehaviorGraphPositionControllerWithDampNode );
	PARENT_CLASS( CBehaviorGraphPositionControllerBaseNode );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMotionExFilterNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionExFilterNode, CBehaviorGraphBaseNode, "Motion", "Filter motion ex" );

protected:
	TInstanceVar< Bool > i_control;

protected:
	CBehaviorGraphValueNode*	m_cachedControlVariableNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Motion Ex Filter"); }
#endif

	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionExFilterNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY( m_cachedControlVariableNode );
END_CLASS_RTTI();
