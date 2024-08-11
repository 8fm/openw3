#pragma once

#include "encounterTypes.h"
#include "encounter.h"
#include "spawnTreeNode.h"

class ISpawnTreeLeafNode : public ISpawnTreeBaseNode
{
	DECLARE_ENGINE_ABSTRACT_CLASS( ISpawnTreeLeafNode, ISpawnTreeBaseNode );
};

BEGIN_ABSTRACT_CLASS_RTTI( ISpawnTreeLeafNode )
	PARENT_CLASS( ISpawnTreeBaseNode )
END_CLASS_RTTI()