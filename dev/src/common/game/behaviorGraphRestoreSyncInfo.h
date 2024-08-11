/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphRestoreSyncInfoNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphRestoreSyncInfoNode, CBehaviorGraphBaseNode, "Utils", "Restore sync info" );

protected:
	CName m_storeName;
	IBehaviorSyncMethod* m_syncMethod;
	Bool m_restoreOnActivation;
	Bool m_restoreEveryFrame;
	CName m_restoreOnEvent;

protected:
	TInstanceVar< Uint32 > i_restoreOnEventID;
	TInstanceVar< Bool > i_restoreActivationAfterUpdate;

public:
	CBehaviorGraphRestoreSyncInfoNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf(TXT( "Restore '%ls' sync info" ), m_storeName.AsChar() ); }
#endif

public:
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );
	virtual void OnInitInstance( CBehaviorGraphInstance& instance ) const;

	virtual void OnActivated( CBehaviorGraphInstance& instance ) const;
	virtual Bool ProcessEvent( CBehaviorGraphInstance& instance, const CBehaviorEvent &event ) const;

	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;

protected:
	void RestoreSync( CBehaviorGraphInstance& instance ) const;

};

BEGIN_CLASS_RTTI( CBehaviorGraphRestoreSyncInfoNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_storeName, TXT("") );
	PROPERTY_INLINED( m_syncMethod, TXT("Synchronization method") );
	PROPERTY_EDIT( m_restoreOnActivation, TXT("") );
	PROPERTY_EDIT( m_restoreEveryFrame, TXT("") );
	PROPERTY_CUSTOM_EDIT( m_restoreOnEvent, TXT(""), TXT("BehaviorEventEdition") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
