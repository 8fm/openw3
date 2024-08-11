
#include "build.h"
#include "interactionToComponentComponent.h"

IMPLEMENT_ENGINE_CLASS( CInteractionToComponentComponent );

void CInteractionToComponentComponent::OnActivate( CEntity* activator )
{
	if ( !NotifyActivation(activator) )
	{
		return;
	}

	// Report activation to script
	if ( m_reportToScript && CallEvent( CNAME( OnInteractionActivated ), GetName(), THandle<CEntity>(activator) ) != CR_EventSucceeded )
	{
		// HACK... needed because looting deletes the entity...
		if ( !activator->HasFlag( OF_Discarded ) )
		{
			CComponent* comp = GetListeningComponent();
			if ( comp )
			{
				THandle< CEntity > activatorHandle( activator );
				comp->CallEvent( CNAME( OnInteractionActivated ),  GetName(), activatorHandle );
			}
		}
	}
}

void CInteractionToComponentComponent::OnDeactivate( CEntity* activator )
{
	if ( !NotifyDeactivation( activator ) )
	{
		return;
	}

	// Report deactivation to script
	if ( m_reportToScript && CallEvent( CNAME( OnInteractionDeactivated ), GetName(), THandle<CEntity>(activator) ) != CR_EventSucceeded )
	{
		// HACK... needed because looting deletes the entity...
		if ( !activator->HasFlag( OF_Discarded ) )
		{
			CComponent* comp = GetListeningComponent();
			if ( comp )
			{
				THandle< CEntity > activatorHandle( activator );
				comp->CallEvent( CNAME( OnInteractionDeactivated ), GetName(), activatorHandle );
			}
		}
	}
}

void CInteractionToComponentComponent::OnExecute()
{
	ASSERT( !m_activatorsList.Empty() );

	// Report to script
	{
		THandle< CEntity > activator( m_activatorsList.Empty() ? NULL : m_activatorsList[0] );
		if( CallEvent( CNAME( OnInteraction ), m_actionName, activator ) != CR_EventSucceeded )
		{
			CComponent* comp = GetListeningComponent();
			if ( comp )
			{
				comp->CallEvent( CNAME( OnInteraction ), m_actionName, activator );
			}
		}
	}

	if ( !NotifyExecution() )
	{
		return;
	}
}

CComponent* CInteractionToComponentComponent::GetListeningComponent()
{
	if ( !m_listeningComponent.IsValid() )
	{
		FindListeningComponent();
	}
	return m_listeningComponent.Get();
}

void CInteractionToComponentComponent::FindListeningComponent()
{
	if ( CEntity* ent = GetEntity() )
	{
		const TDynArray<CComponent*>::const_iterator end = ent->GetComponents().End();
		TDynArray<CComponent*>::const_iterator it = ent->GetComponents().Begin();
		for ( ; it != end; ++it )
		{
			CComponent* comp = *it;
			if ( comp->GetName() == m_targetComponentName )
			{
				m_listeningComponent = THandle< CComponent >( comp );
				return;
			}
		}
	}
}
