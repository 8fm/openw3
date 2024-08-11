#include "build.h"
#include "r4BehTreeNodePredefinedPath.h"

//////////////////////////////////////////////////////////////////////////
// CR4BehTreeNodePredefinedPathWithCompanionDefinition
//////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CR4BehTreeNodePredefinedPathWithCompanionDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

//////////////////////////////////////////////////////////////////////////
// CR4BehTreeNodePredefinedPathWithCompanionInstance
//////////////////////////////////////////////////////////////////////////
CR4BehTreeNodePredefinedPathWithCompanionInstance::CR4BehTreeNodePredefinedPathWithCompanionInstance( const Definition& def, CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent )
	: CBehTreeNodePredefinedPathWithCompanionInstance( def, owner, context, parent )
	, m_getRiderOrMountHelper( )
{
}

Bool CR4BehTreeNodePredefinedPathWithCompanionInstance::Activate()
{
	m_getRiderOrMountHelper.Initialise( m_companionTag );
	return CBehTreeNodePredefinedPathWithCompanionInstance::Activate();
}

CEntity *const CR4BehTreeNodePredefinedPathWithCompanionInstance::GetCompanion()const
{
	return m_getRiderOrMountHelper.GetEntity();
}