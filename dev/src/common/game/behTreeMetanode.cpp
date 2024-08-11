#include "build.h"
#include "behTreeMetanode.h"


////////////////////////////////////////////////////////////////////////
// IBehTreeMetanodeDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeMetanodeDefinition::eEditorNodeType IBehTreeMetanodeDefinition::GetEditorNodeType() const
{
	return NODETYPE_METANODE;
}

Bool IBehTreeMetanodeDefinition::OnSpawn( IBehTreeNodeInstance* node, CBehTreeSpawnContext& context ) const
{
	ASSERT( false, TXT("IBehTreeMetanodeDefinition::OnSpawn must be overriden!") )
	return false;
}