#include "build.h"
#include "behTreeNodeSpecial.h"

IBehTreeNodeDefinition::eEditorNodeType IBehTreeNodeSpecialDefinition::GetEditorNodeType() const
{
	return NODETYPE_UNIQUE;
}