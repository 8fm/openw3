/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/
#pragma  once

#include "behaviorGraphNode.h"

// class defining output node for given graph stage/element etc
class CBehaviorGraphParentInputNode : public CBehaviorGraphBaseNode
{
	DECLARE_BEHAVIOR_NODE_CLASS( CBehaviorGraphParentInputNode, CBehaviorGraphBaseNode, "Parent input", "Animation" );

protected:
	CName					m_parentSocket;

public:
	CBehaviorGraphParentInputNode();

#ifndef NO_EDITOR_GRAPH_SUPPORT
	virtual void OnRebuildSockets();
	virtual String GetCaption() const;
	virtual Color GetTitleColor() const;
#endif

public:
	void SetParentInputSocket( const CName &name );
	virtual void CacheConnections();
};

BEGIN_CLASS_RTTI( CBehaviorGraphParentInputNode );
	PARENT_CLASS( CBehaviorGraphBaseNode );
	PROPERTY_CUSTOM_EDIT( m_parentSocket, TXT("Name of parent input socket"), TXT("BehaviorParentInputSelection") );
END_CLASS_RTTI();
