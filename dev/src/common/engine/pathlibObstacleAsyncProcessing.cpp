#include "build.h"
#include "pathlibObstacleAsyncProcessing.h"

#include "component.h"
#include "componentIterator.h"
#include "game.h"
#include "layer.h"
#include "layerGroup.h"
#include "layerInfo.h"
#include "pathlibComponent.h"
#include "pathlibNavmeshArea.h"
#include "pathlibTaskManager.h"
#include "pathlibTerrain.h"


namespace PathLib
{

#ifndef NO_EDITOR_PATHLIB_SUPPORT

CObstaclesLayerAsyncProcessing::CObstaclesLayerAsyncProcessing( CObstaclesMapper* obstaclesMapper, CLayer* layer )
	: Super( "PathLib-Obstacles processing" )
	, m_obstaclesMapper( obstaclesMapper )
	, m_layer( layer )	
{

}

void CObstaclesLayerAsyncProcessing::DescribeTask( String& task )
{
	static const String STR( TXT("Processing obstacles") );
	task = STR;
}

CLayerGroup* CObstaclesLayerAsyncProcessing::GetProcessedLayerGroup()
{
	CLayer* layer = m_layer.Get();
	return layer
		? layer->GetLayerInfo()->GetLayerGroup()
		: nullptr;
}

Bool CObstaclesLayerAsyncProcessing::ProcessEventList()
{
	// do stuff
	for ( Uint32 i = 0, n = m_eventList.Size(); i != n; ++i )
	{
		const CProcessingEvent& e = m_eventList[ i ];
		m_taskProgress = Float( i ) / Float( n );
		if ( e.m_generalProcessingImplementation )
		{
			CSynchronousSection section( this, false );
			e.m_generalProcessingImplementation->ToolAsyncProcessing( this, e, section );
		}
		else
		{
			if ( e.m_type == CProcessingEvent::TYPE_ATTACHED )
			{
				if ( m_obstaclesMapper->HasMapping( e.m_componentMapping ) )
				{
					continue;
				}
			}
			CSynchronousSection section( this, true );
			if ( section.FailedToStartSection() )
			{
				return false;
			}

			CComponent* component = e.m_component.Get();
			if ( component )
			{
				component->AsPathLibComponent()->ToolAsyncProcessing( this, e, section );
			}
		}
	}

	m_eventList.ClearFast();

	return true;
}

Bool CObstaclesLayerAsyncProcessing::PreProcessingSync()
{
	CLayer* layer = m_layer.Get();
	if ( !layer )
	{
		return false;
	}

	for ( CEntity* entity : layer->GetEntities() )
	{
		for ( CComponent* component : entity->GetComponents() )
		{
			PathLib::IComponent* pathlibComponent = component->AsPathLibComponent();
			if ( !pathlibComponent )
			{
				continue;
			}
			if ( pathlibComponent->IsNoticableInEditor( CProcessingEvent::TYPE_ATTACHED ) )
			{
				CProcessingEvent e;
				pathlibComponent->ProcessingEventAttach( e );
				m_eventList.PushBack( e );
			}
		}
	}
	return !m_eventList.Empty();
}
Bool CObstaclesLayerAsyncProcessing::ProcessPathLibTask()
{
	return ProcessEventList();
}
IGenerationManagerBase::CAsyncTask* CObstaclesLayerAsyncProcessing::PostProcessingSync()
{
	return NULL;
}

#endif		// NO_EDITOR_PATHLIB_SUPPORT

};			// namespace PathLib

