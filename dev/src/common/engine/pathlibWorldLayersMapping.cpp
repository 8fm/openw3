/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibWorldLayersMapping.h"

#include "layerGroup.h"
#include "pathlibAreaDescription.h"
#include "pathlibObstaclesMap.h"
#include "pathlibTaskLayersVisibilityProcessing.h"
#include "pathlibWorld.h"

namespace PathLib
{



///////////////////////////////////////////////////////////////////////////////
// CWorldLayersMapping
///////////////////////////////////////////////////////////////////////////////

CWorldLayersMapping::CWorldLayersMapping()
	: m_isTaskProcessing( false )
{

}

CWorldLayersMapping::~CWorldLayersMapping()
{

}

void CWorldLayersMapping::Initialize( CWorld* world )
{
	struct Local
	{
		static void ProcessLayerRec( CLayerGroup* layerGroup, LayersMap& layersState )
		{
			{
				SLayerMapping mapping( layerGroup );
				auto itFind = layersState.Find( mapping );
				if ( itFind != layersState.End() )
				{
					itFind->m_second.m_enabled = layerGroup->IsVisible();
				}
			}
			
			const auto& subGroups = layerGroup->GetSubGroups();
			for ( CLayerGroup* subGroup : subGroups )
			{
				ProcessLayerRec( subGroup, layersState );
			}
		}
	};

	CLayerGroup* rootLayer = world->GetWorldLayers();
	if ( rootLayer )
	{
		Local::ProcessLayerRec( world->GetWorldLayers(), m_layers );
	}
}

void CWorldLayersMapping::SpawnTask( CPathLibWorld& pathlib )
{
	if ( !m_isTaskProcessing )
	{
		m_isTaskProcessing = true;
		CTaskManager* taskManager = pathlib.GetTaskManager();
		CTaskLayerVisibilityProcessing* layerTask = new CTaskLayerVisibilityProcessing( *taskManager, this );

		taskManager->AddTask( layerTask );
		layerTask->Release();
	}
}

void CWorldLayersMapping::AddAreaDependency( const SLayerMapping& mapping, AreaId areaId )
{
	auto itFind = m_layers.Find( mapping );
	if ( itFind == m_layers.End() )
	{
		LayerInfo info;
		info.m_enabled = false;
		info.m_areaList.PushBack( areaId );
		m_layers.Insert( mapping, Move( info ) );
		return;
	}

	itFind->m_second.m_areaList.PushBackUnique( areaId );
}

void CWorldLayersMapping::OnLayerEnabled( CPathLibWorld& pathlib, CLayerGroup* layerGroup, Bool enabled )
{
	SLayerMapping mapping( layerGroup );
	auto itFind = m_layers.Find( mapping );
	if ( itFind != m_layers.End() )
	{
		Bool wasEnabled = itFind->m_second.m_enabled;
		if ( wasEnabled != enabled )
		{
			itFind->m_second.m_enabled = enabled;

			m_pendingLayersList.PushBack( mapping );

			SpawnTask( pathlib );
		}
	}
}

void CWorldLayersMapping::LayerProcessingFinished( CPathLibWorld& pathlib )
{
	m_isTaskProcessing = false;

	if ( !m_pendingLayersList.Empty() )
	{
		SpawnTask( pathlib );
	}

}

Bool CWorldLayersMapping::IsLayerEnabled( const SLayerMapping& layer ) const
{
	auto itFind = m_layers.Find( layer );
	if ( itFind != m_layers.End() )
	{
		return itFind->m_second.m_enabled;
	}
	return false;
}

const CWorldLayersMapping::LayerInfo* CWorldLayersMapping::GetLayerInfo( const SLayerMapping& layer ) const
{
	auto itFind = m_layers.Find( layer );
	if ( itFind != m_layers.End() )
	{
		return &itFind->m_second;
	}
	return nullptr;
}

void CWorldLayersMapping::Generation_MapLayer( SLayerMapping& layer, AreaId areaId )
{
	auto itFind = m_layers.Find( layer );
	if ( itFind == m_layers.End() )
	{
		LayerInfo info;
		info.m_enabled = false;
		info.m_areaList.PushBack( areaId );
		m_layers.Insert( layer, Move( info ) );
	}
}

Bool CWorldLayersMapping::ReadFromBuffer( CSimpleBufferReader& reader )
{
	Uint16 size;
	if ( !reader.Get( size ) ) 
	{
		return false;
	}

	m_layers.Reserve( size );

	for ( Uint16 i = 0; i < size; ++i )
	{
		SLayerMapping mapping;
		if ( !reader.Get( mapping ) )
		{
			return false;
		}
		LayerInfo info;
		info.m_enabled = false;
		if ( !reader.SmartGet( info.m_areaList ) )
		{
			return false;
		}
		m_layers.Insert( mapping, Move( info ) );
	}
	return true;
}

void CWorldLayersMapping::WriteToBuffer( CSimpleBufferWriter& writer ) const
{
	Uint16 size = Uint16( m_layers.Size() );
	writer.Put( size );

	for ( const auto& pair : m_layers )
	{
		const SLayerMapping& mapping = pair.m_first;
		const AreaList& areaList = pair.m_second.m_areaList;
		writer.Put( mapping );
		writer.SmartPut( areaList );
	}
}

};			// namespace PathLib

