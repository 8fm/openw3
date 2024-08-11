#pragma once

#include "behTreeMetanode.h"

class CBehTreeCommentDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( CBehTreeCommentDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, Comment );
protected:
	IBehTreeNodeDefinition*		m_child;
#ifndef RED_FINAL_BUILD
	String						m_commentDescription;
#endif
public:
	CBehTreeCommentDefinition()
		: m_child ( nullptr )
#ifndef RED_FINAL_BUILD
		, m_commentDescription( TXT("Comment here") )										
#endif
	{}

	IBehTreeNodeInstance*			SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool							OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const;

	// editor support
	Bool							IsTerminal() const override;
	Bool							CanAddChild() const override;
	Int32							GetNumChildren() const override;
	IBehTreeNodeDefinition*			GetChild( Int32 index ) const override;
	void							AddChild( IBehTreeNodeDefinition* node ) override;
	void							RemoveChild( IBehTreeNodeDefinition* node ) override;
	Bool							IsValid() const override;
	void							CollectNodes( TDynArray< IBehTreeNodeDefinition* >& nodes ) const override;
#ifndef NO_EDITOR_GRAPH_SUPPORT
	void							OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif // NO_EDITOR_GRAPH_SUPPORT

	String							GetNodeCaption() const override;
	eEditorNodeType					GetEditorNodeType() const override;
};

BEGIN_CLASS_RTTI( CBehTreeCommentDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
	PROPERTY_RO( m_child, TXT("Child") );
#ifndef RED_FINAL_BUILD
	PROPERTY_EDIT_NOT_COOKED( m_commentDescription, TXT( "Comment" ) );
#endif
END_CLASS_RTTI();