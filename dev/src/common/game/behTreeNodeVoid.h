#pragma once

#include "behTreeMetanode.h"

class IBehTreeVoidDefinition : public IBehTreeMetanodeDefinition
{
	DECLARE_BEHTREE_NODE( IBehTreeVoidDefinition, IBehTreeMetanodeDefinition, IBehTreeNodeInstance, Void );
protected:
#ifndef RED_FINAL_BUILD
	TDynArray< IBehTreeNodeDefinition* >		m_voidNodes;
#endif

	Bool RemoveNullChildren();
public:
	IBehTreeNodeInstance*	SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent = NULL ) const override;
	Bool					OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;


	// Node list support
	Bool					IsTerminal() const override;
	Bool					IsValid() const override;
	Bool					CanAddChild() const override;
	void					RemoveChild( IBehTreeNodeDefinition* node ) override;
	Int32					GetNumChildren() const override;
	IBehTreeNodeDefinition*	GetChild( Int32 index ) const override;
	void					AddChild( IBehTreeNodeDefinition* node ) override;


#ifndef NO_EDITOR_GRAPH_SUPPORT
	void					OffsetNodesPosition( Int32 offsetX, Int32 offsetY ) override;
#endif

	void					OnPostLoad() override;
};

BEGIN_CLASS_RTTI( IBehTreeVoidDefinition );
	PARENT_CLASS( IBehTreeMetanodeDefinition );
#ifndef RED_FINAL_BUILD
	PROPERTY_RO_NOT_COOKED( m_voidNodes, TXT("Void nodes that WONT spawn.") );
#endif
END_CLASS_RTTI();