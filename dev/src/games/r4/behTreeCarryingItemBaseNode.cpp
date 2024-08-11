#include "build.h"
#include "behTreeCarryingItemBaseNode.h"


CBehTreeNodeCarryingItemBaseInstance::CBehTreeNodeCarryingItemBaseInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: IBehTreeNodeInstance( def, owner, context, parent )
	, m_carryingItemsData( owner )
{

}
