/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "generalTreeInterface.h"
#include "spawnTreeNode.h"

class CSpawnTreeGeneralIterator;

class CSpawnTreeGeneralDescription : public IGeneralTreeDummyInterface
{
protected:
	ISpawnTreeBaseNode*						m_rootNode;
	CSpawnTreeInstance*						m_instanceBuffer;
public:
	typedef				ISpawnTreeBaseNode														Node;

	CSpawnTreeGeneralDescription( CEncounter* encounter );

	Node*				GetRootNode() const														{ return m_rootNode; }
	CSpawnTreeInstance*	GetRootInstanceBuffer() const											{ return m_instanceBuffer; }

	static Uint32		GetChildNodesCount( Node* node )										{ return node->GetTransientChildMembersCount(); }
	static Node*		GetChildNode( Node* node, Uint32 index )								{ return node->GetTransientChildMember( index ); }

	// returns true if node state should be saved
	static Bool			IsNodeStateSaving( CSpawnTreeGeneralIterator& iterator );
	static void			SaveNodeState( CSpawnTreeGeneralIterator& iterator, IGameSaver* writer );
	static Bool			LoadNodeState( CSpawnTreeGeneralIterator& iterator, IGameLoader* reader );

	static Uint32		GetPersistantChildsCount( Node* node )									{ return GetChildNodesCount( node ); }
	static Node*		GetPersistantChild( Node* node, Uint32 index )							{ return GetChildNode( node, index ); }
};

class CSpawnTreeStateSerializer
{
protected:
	CSpawnTreeGeneralDescription			m_desc;
	Uint32									m_serializedNodes;
public:
	CSpawnTreeStateSerializer( CEncounter* encounter );

	void				Save( IGameSaver* writer );
	Bool				Load( IGameLoader* reader );

	Uint32				GetSerializedNodesCount() const											{ return m_serializedNodes; }
};