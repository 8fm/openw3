/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphValueNode.h"

// class defining output node for given graph stage/element etc
class CBehaviorGraphParentValueInputNode : public CBehaviorGraphValueNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphParentValueInputNode, CBehaviorGraphValueNode, "Parent input", "Value" );

protected:
	CName							m_parentSocket;

protected:
	CBehaviorGraphValueNode*		m_cachedParentValueNode;

public:
	CBehaviorGraphParentValueInputNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
#endif

public:
	//! set name of parent input node
	void SetParentInputSocket( const CName &name );

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;

	virtual void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void CacheConnections();

	virtual Float GetValue( CBehaviorGraphInstance& instance ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphParentValueInputNode );
	PARENT_CLASS( CBehaviorGraphValueNode );
	PROPERTY_CUSTOM_EDIT( m_parentSocket, TXT("Name of parent input socket"), TXT("BehaviorParentValueInputSelection") );
	PROPERTY( m_cachedParentValueNode );
END_CLASS_RTTI();
