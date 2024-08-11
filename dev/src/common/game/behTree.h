/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/

#pragma once

class CBehTreeInstance;
class IBehTreeNodeDefinition;
class CAIDefaults;

#define BEHREE_AUTOFIX_TREE_PARENTAGE

class CBehTree : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CBehTree, CResource, "w2behtree", "AI tree" );

private:
	THandle< IBehTreeNodeDefinition >				m_rootNode;
#ifdef EDITOR_AI_DEBUG
	THashMap< Int32, IBehTreeNodeDefinition * >		m_nodeMap;
	TDynArray< IBehTreeNodeDefinition* >			m_nodes;
#endif

#ifdef BEHREE_AUTOFIX_TREE_PARENTAGE
	void FixTreeParentage( IBehTreeNodeDefinition* node, CObject* parentObject );
#endif
public:

	CBehTree()
		: m_rootNode(NULL)
	{
	}

	~CBehTree()
	{
	}

	//! Get root node
	const IBehTreeNodeDefinition*	GetRootNode() const { return m_rootNode; }
	IBehTreeNodeDefinition* GetRootNode() { return m_rootNode; }
	
	//! Set root node
	void SetRootNode( IBehTreeNodeDefinition * rootNode );

#ifdef EDITOR_AI_DEBUG
	//! Loaded
	void OnPostLoad() override;
#endif

	//! On save
	void OnSave();

	//! On node save
	void OnNodeSave( IBehTreeNodeDefinition* node );

#ifdef BEHREE_AUTOFIX_TREE_PARENTAGE
	void FixTreeParentage();
#endif

#ifdef EDITOR_AI_DEBUG
	//! Retrieves a node by hash
	IBehTreeNodeDefinition* GetNodeByHash( Int32 hash ) const;
#endif

};

BEGIN_CLASS_RTTI( CBehTree );
	PARENT_CLASS( CResource );
	PROPERTY_RO( m_rootNode, TXT("Root node") );
#ifdef EDITOR_AI_DEBUG
	PROPERTY_NOT_COOKED( m_nodes );
#endif
END_CLASS_RTTI();
