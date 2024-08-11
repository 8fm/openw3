#include "build.h"
#include "behTreeNodeSelectTargetOrMountByTag.h"
#include "ridingAiStorage.h"
#include "../../common/engine/tagManager.h"

////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition
IBehTreeNodeDecoratorInstance*	CBehTreeNodeSelectTargetOrMountByTagDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance

Bool CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance::Activate()
{
	m_getRiderOrMountHelper.Initialise( m_tag );
	CEntity *const targetEntity = m_getRiderOrMountHelper.GetEntity();
	if ( targetEntity )
	{
		m_owner->SetActionTarget( targetEntity );
			// Set target BEFORE children activation
		if ( !Super::Activate() )
		{
			// reset target
			m_owner->SetActionTarget( nullptr );
			DebugNotifyActivationFail();
			return false;
		}

		return true;
	}

	if ( m_allowActivationWhenNoTarget )
	{
		m_owner->SetActionTarget( nullptr );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance::Deactivate()
{
	Super::Deactivate();
	// reset target AFTER children deactivation
	m_owner->SetActionTarget( NULL );
}

void CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	Super::OnGenerateDebugFragments( frame );
}

void CBehTreeNodeSelectTargetOrMountByTagDecoratorInstance::Update()
{
	Super::Update();
	CEntity *const targetEntity =  m_getRiderOrMountHelper.GetEntity();
	m_owner->SetActionTarget( targetEntity );
}