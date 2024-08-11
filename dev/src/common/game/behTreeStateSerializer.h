/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "behTreeNode.h"
#include "generalTreeInterface.h"

class IBehTreeNodeInstance;

class CBehTreeGeneralDescription : public IGeneralTreeDummyInterface
{
public:
	typedef				IBehTreeNodeInstance													Node;

protected:
	Node*				m_rootNode;

public:
	CBehTreeGeneralDescription( Node* node )
		: m_rootNode( node )																	{}

	Node*				GetRootNode() const														{ return m_rootNode; }

	static Uint32		GetChildNodesCount( Node* node )										{ return node->GetNumChildren(); }
	static Node*		GetChildNode( Node* node, Uint32 index )								{ return node->GetChild( index ); }

	// returns true if node state should be saved
	static Bool			IsNodeStateSaving( Node* node )											{ return node->IsSavingState(); }
	static void			SaveNodeState( Node* node, IGameSaver* writer )							{ node->SaveState( writer ); }
	static Bool			LoadNodeState( Node* node, IGameLoader* reader )						{ return node->LoadState( reader ); }

	static Uint32		GetPersistantChildsCount( Node* node )									{ return node->GetNumPersistantChildren(); }
	static Node*		GetPersistantChild( Node* node, Uint32 index )							{ return node->GetPersistantChild( index ); }
};

class CBehTreeStateSerializer
{
protected:
	CBehTreeGeneralDescription			m_desc;
public:
	static const Uint16 BINARY_VERSION				 = 1;

	CBehTreeStateSerializer( IBehTreeNodeInstance* node );

	Bool				IsSaving();
	Bool				Save( IGameSaver* writer );
	Bool				Load( IGameLoader* reader );
};