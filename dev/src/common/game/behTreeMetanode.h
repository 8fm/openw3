#pragma once

#include "behTreeNode.h"

class IBehTreeMetanodeDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeMetanodeDefinition, IBehTreeNodeDefinition, IBehTreeNodeInstance, Metanode );
public:
	eEditorNodeType GetEditorNodeType() const override;

	Bool OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const override;
};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeMetanodeDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
END_CLASS_RTTI();