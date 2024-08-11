#include "build.h"

#include "behTreeNodeSelectTargetByTag.h"
#include "behTreeInstance.h"
#include "../engine/tagManager.h"
#include "../engine/renderFrame.h"

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagDecoratorDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeSelectTargetByTagDecoratorDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagDecoratorInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSelectTargetByTagDecoratorInstance::Activate()
{
	CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
	CNode* target = tagMgr->GetTaggedNode( m_tag );
	if ( target )
	{
		m_owner->SetActionTarget( target );
		// set target BEFORE children activation
		if ( !Super::Activate() )
		{
			// reset target
			m_owner->SetActionTarget( NULL );
			DebugNotifyActivationFail();
			return false;
		}

		return true;
	}

	if ( m_allowActivationWhenNoTarget )
	{
		m_owner->SetActionTarget( NULL );
		return true;
	}

	DebugNotifyActivationFail();
	return false;
}
void CBehTreeNodeSelectTargetByTagDecoratorInstance::Deactivate()
{
	Super::Deactivate();
	// reset target AFTER children deactivation
	m_owner->SetActionTarget( NULL );
}

void CBehTreeNodeSelectTargetByTagDecoratorInstance::OnGenerateDebugFragments( CRenderFrame* frame )
{
	CNode* node = m_owner->GetActionTarget().Get();
	String text = String::Printf(
		TXT("Selected action target is %s by tag %s"),
		node ? node->GetFriendlyName().AsChar() : TXT("NULL"),
		m_tag.AsChar() );

	CActor* actor = m_owner->GetActor();
	Vector pos = actor->GetWorldPositionRef();
	pos.Z += 2.0f;
	frame->AddDebugText( pos, text, 0, 0, false, Color::DARK_GREEN );
}


////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagDefinition
////////////////////////////////////////////////////////////////////////
IBehTreeNodeInstance* CBehTreeNodeSelectTargetByTagDefinition::SpawnInstance( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	return new Instance( *this, owner, context, parent );
}

////////////////////////////////////////////////////////////////////////
// CBehTreeNodeSelectTargetByTagInstance
////////////////////////////////////////////////////////////////////////
Bool CBehTreeNodeSelectTargetByTagInstance::Activate()
{
	if ( !m_tag.Empty() )
	{
		CTagManager* tagMgr = GGame->GetActiveWorld()->GetTagManager();
		CNode* target = tagMgr->GetTaggedNode( m_tag );
		if ( !target )
		{
			DebugNotifyActivationFail();
			return false;
		}

		m_owner->SetActionTarget( target );
	}
	else
	{
		m_owner->SetActionTarget( NULL );
	}

	return Super::Activate();
}
void CBehTreeNodeSelectTargetByTagInstance::Update()
{
	Complete( BTTO_SUCCESS );
}