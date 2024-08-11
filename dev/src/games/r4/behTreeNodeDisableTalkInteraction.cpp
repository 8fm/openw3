/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeDisableTalkInteraction.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeDecoratorDisableTalkInteractionDefinition )

///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorDisableTalkInteractionInstance
///////////////////////////////////////////////////////////////////////////////
CInteractionComponent* CBehTreeNodeDecoratorDisableTalkInteractionInstance::GetInteractionComponent()
{
	{
		CInteractionComponent* interactionComponent = m_cachedInteraction.Get();
		if ( interactionComponent )
		{
			return interactionComponent;
		}
	}
	

	static const String COMPONENT_NAME( TXT("talk") );

	ComponentIterator< CInteractionComponent > it( m_owner->GetActor() );
	while ( it )
	{
		CInteractionComponent* interactionComponent = *it;
		if ( interactionComponent->GetName() == COMPONENT_NAME )
		{
			m_cachedInteraction = interactionComponent;
			return interactionComponent;
		}

		++it;
	}

	return nullptr;
}

Bool CBehTreeNodeDecoratorDisableTalkInteractionInstance::Activate()
{
	if ( Super::Activate() )
	{
		CInteractionComponent* interactionComponent = GetInteractionComponent();
		if ( interactionComponent )
		{
			interactionComponent->SetEnabled( false );
		}
		return true;
	}
	return false;
}
void CBehTreeNodeDecoratorDisableTalkInteractionInstance::Deactivate()
{
	CInteractionComponent* interactionComponent = GetInteractionComponent();
	if ( interactionComponent )
	{
		interactionComponent->SetEnabled( true );
	}

	Super::Deactivate();
}