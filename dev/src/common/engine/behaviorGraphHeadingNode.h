/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"
#include "behaviorGraphValueNode.h"

class CBehaviorGraphHeadingNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphHeadingNode, CBehaviorGraphBaseNode, "Motion", "Heading" );

protected:
	Float	m_heading;

public:
	CBehaviorGraphHeadingNode();

	virtual String GetCaption() const;

	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphHeadingNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_heading, TXT("Character heading") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMotionExRotAngleNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionExRotAngleNode, CBehaviorGraphValueBaseNode, "Motion", "Motion Ex Rot Angle" );		

protected:
	Bool				m_worldSpace;

protected:
	CBehaviorGraphNode*	m_cachedAnimInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Motion Ex Rot Angle"); }
#endif

public:
	CBehaviorGraphMotionExRotAngleNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;	
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionExRotAngleNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY( m_cachedAnimInputNode );
	PROPERTY_EDIT( m_worldSpace, String::EMPTY );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphMotionExToAngleNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphMotionExToAngleNode, CBehaviorGraphValueBaseNode, "Motion", "Motion Ex To Angle" );		

protected:
	Bool				m_worldSpace;

protected:
	CBehaviorGraphNode*	m_cachedAnimInputNode;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const { return TXT("Motion Ex To Angle"); }
#endif

public:
	CBehaviorGraphMotionExToAngleNode();

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void OnReset( CBehaviorGraphInstance& instance ) const;	
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphMotionExToAngleNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY( m_cachedAnimInputNode );
	PROPERTY_EDIT( m_worldSpace, String::EMPTY );
END_CLASS_RTTI();
