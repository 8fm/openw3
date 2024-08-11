/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#pragma  once

#include "behaviorGraphValueNode.h"

class CBehaviorGraphGetCustomTrackNode : public CBehaviorGraphValueBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGetCustomTrackNode, CBehaviorGraphValueBaseNode, "Misc", "Custom track value" );		

protected:
	Int32			m_trackIndex;
	Float		m_currentValue;	
	Float		m_defaultValue;

protected:
	CBehaviorGraphNode*	m_cachedAnimInputNode;

public:
	CBehaviorGraphGetCustomTrackNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual void OnPropertyPostChange( IProperty *prop );
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
	virtual void Sample( SBehaviorSampleContext& context, CBehaviorGraphInstance& instance, SBehaviorGraphOutput &output ) const;
	
	virtual void OnReset( CBehaviorGraphInstance& instance ) const;	
	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual void OnDeactivated( CBehaviorGraphInstance& instance ) const;

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const;
	virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	void ProcessActivationAlpha( CBehaviorGraphInstance& instance, Float alpha ) const;

	virtual void CacheConnections();

public:
	virtual Float GetValueFromTrack( const SBehaviorGraphOutput& pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphGetCustomTrackNode );
	PARENT_CLASS( CBehaviorGraphValueBaseNode );
	PROPERTY_CUSTOM_EDIT( m_trackIndex, TXT("Custom float track to access"), TXT("BehaviorCustomTrackSelection") );
	PROPERTY_EDIT( m_defaultValue, TXT("Default value") );
	PROPERTY( m_cachedAnimInputNode );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphGetFloatTrackNode : public CBehaviorGraphGetCustomTrackNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphGetFloatTrackNode, CBehaviorGraphGetCustomTrackNode, "Misc", "Float track value" );		

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const;
	virtual void OnPropertyPostChange( IProperty *prop );
#endif

public:
	virtual Float GetValueFromTrack( const SBehaviorGraphOutput& pose ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphGetFloatTrackNode );
	PARENT_CLASS( CBehaviorGraphGetCustomTrackNode );
END_CLASS_RTTI();
