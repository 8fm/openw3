#include "build.h"
#include "behTreeNodeVitalSpotActive.h"

IMPLEMENT_ENGINE_CLASS( SActiveVitalSpotEventData );

RED_DEFINE_NAME( VitalSpotActivator );

RED_DEFINE_STATIC_NAME( FilterVitalSpots );

Bool CBehTreeNodeVitalSpotActiveInstance::OnEvent( CBehTreeEvent& e )
{
	if ( e.m_eventName == CNAME(FilterVitalSpots) && e.m_eventType == BTET_GameplayEvent && e.m_gameplayEventData.m_customDataType && e.m_gameplayEventData.m_customDataType->GetType() == RT_Class )
	{
		CClass* classId = (CClass*)e.m_gameplayEventData.m_customDataType;
		SActiveVitalSpotEventData* data = Cast < SActiveVitalSpotEventData > ( classId, e.m_gameplayEventData.m_customData );
		
		if ( data && data->m_VSActivatorNodeName == m_VSActivatorNodeName )
		{
			data->m_result = m_activateVitalSpot;
		}
	}

	return Super::OnEvent( e );
}

IBehTreeNodeDecoratorInstance* CBehTreeNodeVitalSpotActiveDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const 
{
	return new Instance( *this, owner, context, parent );
}
