#include "build.h"

#include "eventRouterComponent.h"
#include "../../common/game/scriptedComponent.h"

IMPLEMENT_ENGINE_CLASS( CEventRouterComponent );

SRouterEntry::~SRouterEntry()
{
	 //m_awareComponents.Clear();	
}

CEventRouterComponent::~CEventRouterComponent()
{
	//m_entries.Clear();		
}

void CEventRouterComponent::RouteEvent( CName eventName )
{	
	for( Uint32 i=0; i<m_entries.Size(); ++i )
	{
		SRouterEntry& entry = m_entries[i];
		if( entry.m_eventName == eventName )
		{			
			for( Uint32 j=0; j<entry.m_awareComponents.Size(); ++j )
			{
				entry.m_awareComponents[j]->ProcessEvent( eventName, NULL );
			}			
		}
	}
}

void CEventRouterComponent::OnAttachFinished( CWorld* world )
{
	TBaseClass::OnAttachFinished( world );

	const TDynArray< CComponent* >& allComponents = GetEntity()->GetComponents();
	
	TDynArray< CName > eventsNames;

	for( Uint32 i=0; i<allComponents.Size(); ++i )
	{
		if( allComponents[i]->IsA< CScriptedComponent >() )
		{					
			CScriptedComponent* r6Cmp =  static_cast< CScriptedComponent* >( allComponents[i] );
			const TDynArray< CScriptedComponent::SIntrestingEventEntry >& interestingEvents = r6Cmp->GetInterestingEvents();

			for( Uint32 j=0; j<interestingEvents.Size(); ++j )
			{
				const CScriptedComponent::SIntrestingEventEntry& interestingEvent = interestingEvents[j];

				if( eventsNames.PushBackUnique( interestingEvent.m_eventName ) )
				{
					SRouterEntry entry;
					entry.m_eventName = interestingEvent.m_eventName;
					entry.m_awareComponents.PushBack( r6Cmp );
					m_entries.PushBack( entry );
				}
				else
				{
					Int32 index = (Int32) eventsNames.GetIndex( interestingEvent.m_eventName );
					if( index != -1 )
					{
						m_entries[ index ].m_awareComponents.PushBack( r6Cmp );
					}				
				}
			}
		}		
	}
}

void CEventRouterComponent::funcRouteEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, nameOfEvent, CName::NONE );
	FINISH_PARAMETERS;
	
	if( nameOfEvent )
	{
		RouteEvent( nameOfEvent );
	}
}
