#pragma once

#include "behTreeNode.h"

class IBehTreeNodeSpecialDefinition : public IBehTreeNodeDefinition
{
	DECLARE_BEHTREE_ABSTRACT_NODE( IBehTreeNodeSpecialDefinition, IBehTreeNodeDefinition, IBehTreeNodeInstance, Special );

public:
	eEditorNodeType			GetEditorNodeType() const override;

};

BEGIN_ABSTRACT_CLASS_RTTI( IBehTreeNodeSpecialDefinition );
	PARENT_CLASS( IBehTreeNodeDefinition );
END_CLASS_RTTI();
