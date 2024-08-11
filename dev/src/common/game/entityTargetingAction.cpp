/*
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entityTargetingAction.h"


IMPLEMENT_RTTI_ENUM( EEntityTargetingActionMechanism )

IMPLEMENT_ENGINE_CLASS( IEntityTargetingAction )
IMPLEMENT_ENGINE_CLASS( IComponentTargetingAction )


////////////////////////////////////////////////////////////////////////
// IEntityTargetingAction
////////////////////////////////////////////////////////////////////////
CEntity* IEntityTargetingAction::GetTarget( CEntity* owner, CNode* activator )
{
	switch( m_entitySelectionType )
	{
	case ETAM_ByHandle:
		{
			CEntity* entity = m_entityHandle.Get();
			if ( entity )
			{
				return entity;
			}
		}
		// NOTICE: no break
	case ETAM_Self:
		return owner;
	case ETAM_Activator:
		return Cast< CEntity>( activator );
	default:
		RED_LOG_WARNING( RED_LOG_CHANNEL( AI ), TXT("IEntityTargetingAction: Unexpected m_entitySelectionType value.") );
		return NULL;
	}
}

void IEntityTargetingAction::Perform( CEntity* parent )
{
	CEntity* entity = GetTarget( parent );
	if ( entity )
	{
		PerformOnEntity( entity );
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( AI ), TXT("IEntityTargetingAction: Cannot find targeted entity.") );
	}

}
void IEntityTargetingAction::Perform( CEntity* parent, CNode* node )
{
	CEntity* entity = GetTarget( parent, node );
	if ( entity )
	{
		PerformOnEntity( entity );
	}
	else
	{
		RED_LOG_WARNING( RED_LOG_CHANNEL( AI ), TXT("IEntityTargetingAction: Cannot find targeted entity.") );
	}
}

////////////////////////////////////////////////////////////////////////
// IComponentTargetingAction
////////////////////////////////////////////////////////////////////////
CClass* IComponentTargetingAction::SupportedComponentClass()
{
	return CComponent::GetStaticClass();
}
void IComponentTargetingAction::PerformOnEntity( CEntity* entity )
{
	CClass* componentClass = SupportedComponentClass();
	const auto& componentsList = entity->GetComponents();
	for ( auto it = componentsList.Begin(), end = componentsList.End(); it != end; ++it )
	{
		CComponent* component = *it;
		if ( component && component->IsA( componentClass ) && (m_componentName.Empty() || m_componentName == component->GetName()) )
		{
			PerformOnComponent( component );
		}
	}
}