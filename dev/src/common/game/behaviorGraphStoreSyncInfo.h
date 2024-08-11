/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#pragma  once

#include "../engine/behaviorGraphNode.h"

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphStoreSyncInfoNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphStoreSyncInfoNode, CBehaviorGraphBaseNode, "Utils", "Store sync info" );

protected:
	CName m_storeName;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const { return String::Printf(TXT( "Store sync info as '%ls'" ), m_storeName.AsChar() ); }
#endif

public:
	virtual void OnUpdate( SBehaviorUpdateContext &context, CBehaviorGraphInstance& instance, Float timeDelta ) const;
};

BEGIN_CLASS_RTTI( CBehaviorGraphStoreSyncInfoNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_storeName, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////

class CBehaviorGraphConvertSyncInfoIntoCyclesNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphConvertSyncInfoIntoCyclesNode, CBehaviorGraphBaseNode, "Utils", "Convert sync info into cycles" );

protected:
	Int32 m_numCycles;

public:
#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual String GetCaption() const override { return String::Printf(TXT( "Convert sync info into [%d] cycles" ), m_numCycles ); }
#endif

public:
	CBehaviorGraphConvertSyncInfoIntoCyclesNode();

	virtual void GetSyncInfo( CBehaviorGraphInstance& instance, CSyncInfo &info ) const override;
	//virtual void SynchronizeTo( CBehaviorGraphInstance& instance, const CSyncInfo &info ) const override;
};

BEGIN_CLASS_RTTI( CBehaviorGraphConvertSyncInfoIntoCyclesNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_EDIT( m_numCycles, TXT("") );
END_CLASS_RTTI();

//////////////////////////////////////////////////////////////////////////
