/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "pathlibMetalinkComponent.h"

#include "engineMetalinkTypes.h"
#include "pathlibAgent.h"
#include "pathlibAreaDescription.h"
#include "pathlibNavmeshArea.h"
#include "pathlibObstacleAsyncProcessing.h"
#include "pathlibObstacleMapper.h"
#include "pathlibTerrain.h"
#include "pathlibWorld.h"

extern PathLib::CMetalinkSetupFactory g_metalinkSetupFactory;

namespace PathLib
{

////////////////////////////////////////////////////////////////////////////
// IMetalinkSetup::RuntimeData
////////////////////////////////////////////////////////////////////////////

IMetalinkComponent* IMetalinkSetup::RuntimeData::GetComponent( CAgent* pathAgent )
{
	IComponent* c = m_component.Get();
	if ( c )
	{
		return c->AsMetalinkComponent();
	}
	c = m_mapping.GetComponent( pathAgent->GetPathLib()->GetWorld() );
	if ( c )
	{
		m_component = c;
		return c->AsMetalinkComponent();
	}
	return nullptr;
}

CComponent* IMetalinkSetup::RuntimeData::GetEngineComponent( CAgent* pathAgent )
{
	CComponent* engComp = m_component.GetEngineComponent();
	if ( engComp )
	{
		return engComp;
	}
	IComponent* c = m_mapping.GetComponent( pathAgent->GetPathLib()->GetWorld() );
	if ( c )
	{
		m_component = c;
		return m_component.GetEngineComponent();
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////
// IMetalinkSetup
////////////////////////////////////////////////////////////////////////////

Bool IMetalinkSetup::AgentPathfollowUpdate( RuntimeData& r, CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint )
{
	return true;
}
Bool IMetalinkSetup::AgentPathfollowOver( RuntimeData& r, CAgent* pathAgent, const Vector3& interactionPoint, const Vector3& destinationPoint )
{
	return true;
}
Bool IMetalinkSetup::AgentPathfollowIgnore( CAgent* pathAgent )
{
	return false;
}

Uint32 IMetalinkSetup::GetMetalinkPathfollowFlags() const
{
	return METALINK_PATHFOLLOW_DEFUALT;
}

void IMetalinkSetup::AddRef()
{

}

void IMetalinkSetup::Release()
{

}

Bool IMetalinkSetup::ReadFromBuffer( CSimpleBufferReader& reader )
{
	return true;
}
void IMetalinkSetup::WriteToBuffer( CSimpleBufferWriter& writer ) const
{

}

////////////////////////////////////////////////////////////////////////////
// CGenericMetalinkSetup
////////////////////////////////////////////////////////////////////////////
MetalinkClassId CGenericMetalinkSetup::GetClassId() const
{
	return PathLib::MetalinkClassId( EEngineMetalinkType::T_GENERIC );
}

IMetalinkSetup::Ptr CGenericMetalinkSetup::Create()
{
	return PathLib::CMetalinkSetupFactory::GetInstance().GetClassFactory( PathLib::MetalinkClassId( EEngineMetalinkType::T_GENERIC ) )->Request();
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkSetupFactory::IClassFactory
////////////////////////////////////////////////////////////////////////////
CMetalinkSetupFactory::IClassFactory::IClassFactory( ClassFactoryId id, ClassFactoryId knownIdLimit )
{
	CMetalinkSetupFactory::GetInstance().RegisterClassFactory( this, id, knownIdLimit );
}

////////////////////////////////////////////////////////////////////////////
// CMetalinkSetupFactory
////////////////////////////////////////////////////////////////////////////
CMetalinkSetupFactory::CMetalinkSetupFactory()
{
}
CMetalinkSetupFactory::~CMetalinkSetupFactory()
{

}

CMetalinkSetupFactory::IClassFactory* CMetalinkSetupFactory::GetClassFactory( ClassFactoryId classId )
{
	if ( classId >= m_classes.Size() )
	{
		return nullptr;
	}
	return m_classes[ classId ];
}

void CMetalinkSetupFactory::RegisterClassFactory( IClassFactory* classFactory, ClassFactoryId classId, ClassFactoryId classIdLimit )
{
	// handle possible limit expansion
	if ( m_classes.Size() < classIdLimit )
	{
		Uint32 oldSize = m_classes.Size();
		m_classes.Resize( classIdLimit );
		// fill new spots with empty ptr
		for ( Uint32 i = oldSize; i < classIdLimit; ++i )
		{
			m_classes[ i ] = nullptr;
		}
	}
	ASSERT( classId < m_classes.Size() );
	ASSERT( m_classes[ classId ] == nullptr );
	m_classes[ classId ] = classFactory;
}

IMetalinkSetup* CMetalinkSetupFactory::NewFromBuffer( CSimpleBufferReader& reader )
{
	ClassFactoryId id;
	if ( !reader.Get( id ) )
	{
		return nullptr;
	}

	IClassFactory* factory = GetClassFactory( id );
	if ( !factory )
	{
		return nullptr;
	}

	IMetalinkSetup* metalinkSetup = factory->Request();
	if ( !metalinkSetup->ReadFromBuffer( reader ) )
	{
		factory->Release( metalinkSetup );
		return nullptr;
	}
	return metalinkSetup;
}
void CMetalinkSetupFactory::SaveToBuffer( CSimpleBufferWriter& writer, IMetalinkSetup* metalinSetup )
{
	ClassFactoryId id = metalinSetup->GetClassId();

	writer.Put( id );

	metalinSetup->WriteToBuffer( writer );
}

CMetalinkSetupFactory& CMetalinkSetupFactory::GetInstance()
{
	return g_metalinkSetupFactory;
}


////////////////////////////////////////////////////////////////////////////
// IMetalinkComponent
////////////////////////////////////////////////////////////////////////////

Bool IMetalinkComponent::RuntimeProcessing( CComponentRuntimeProcessingContext& context, const CProcessingEvent& e )
{
	CObstaclesMapper* mapper = context.GetMapper();
	CPathLibWorld& pathlib = mapper->GetPathLib();
	CObstaclesMapper::ObstacleInfo& mapping = mapper->RequestMapping( e.m_componentMapping );
	Bool enabledState = (e.m_type == CProcessingEvent::TYPE_ATTACHED);
	if ( mapping.m_isRuntimeObstacleEnabled != enabledState )
	{
		mapping.m_isRuntimeObstacleEnabled = enabledState;
		const auto& areaList = mapping.m_areaInfo;
		for ( auto it = areaList.Begin(), end = areaList.End(); it != end; ++it )
		{
			CAreaDescription* area = pathlib.GetAreaDescription( it->m_areaId );
			CNavModyficationMap* metalinks = area->GetMetalinks();
			if ( metalinks )
			{
				if ( e.m_type == CProcessingEvent::TYPE_ATTACHED )
				{
					metalinks->RuntimeEnableMetalink( it->m_obstacleId, context );
				}
				else if ( e.m_type == CProcessingEvent::TYPE_DETACHED )
				{
					metalinks->RuntimeDisableMetalink( it->m_obstacleId, context );
				}
			}
		}
	}

	return true;
}
Bool IMetalinkComponent::ToolAsyncProcessing( CObstaclesLayerAsyncProcessing* processingJob, const CProcessingEvent& e, CGenerationManager::CAsyncTask::CSynchronousSection& section )
{
	struct AreaProcessing : public CInstanceMap::CInstanceFunctor
	{
		AreaProcessing( IMetalinkComponent* me, CObstaclesMapper::ObstacleInfo* prevMapping, CObstaclesMapper::ObstacleInfo& newMapping, CMetalinkConfiguration& configuration )
			: m_metalinkComponent( me )
			, m_prevMapping( prevMapping )
			, m_newMapping( newMapping )
			, m_configuration( configuration )
		{}

		void HandleArea( CAreaDescription* area )
		{
			PathLib::AreaId areaId = area->GetId();
			CNavModyfication::Id id = CNavModyfication::INVALID_ID;
			if ( m_prevMapping )
			{
				for ( auto it = m_prevMapping->m_areaInfo.Begin(), end = m_prevMapping->m_areaInfo.End(); it != end; ++it )
				{
					if ( it->m_areaId == areaId )
					{
						id = it->m_obstacleId;
						break;
					}
				}
			}

			CNavModyficationMap* metalinksSet = area->LazyInitializeModyficationSet();

			if ( id == CNavModyfication::INVALID_ID )
			{
				id = metalinksSet->GetUniqueMetalinkId();
			}

			if ( metalinksSet->CreateMetalink( id, m_metalinkComponent, m_configuration ) )
			{
				CObstaclesMapper::ObstacleAreaInfo areaInfo;
				areaInfo.m_areaId = areaId;
				areaInfo.m_obstacleId = id;
				m_newMapping.m_areaInfo.PushBack( areaInfo );
			}
		}
		// CInstanceMap::CInstanceFunctor interface
		Bool Handle( CNavmeshAreaDescription* naviArea ) override
		{
			HandleArea( naviArea );
			return false;
		}

		IMetalinkComponent*							m_metalinkComponent;
		CObstaclesMapper::ObstacleInfo*				m_prevMapping;
		CObstaclesMapper::ObstacleInfo&				m_newMapping;
		CMetalinkConfiguration&						m_configuration;

	};

	CObstaclesMapper* mapper = processingJob->GetObstacleMapper();
	CPathLibWorld& pathlib = mapper->GetPathLib();
	CObstaclesMapper::ObstacleInfo* prevMapping = mapper->GetMapping( e.m_componentMapping );
	CObstaclesMapper::ObstacleInfo newMapping;

	section.Begin();

	CComponent* engineComponent = e.m_component.Get();
	IMetalinkComponent* component = static_cast< IMetalinkComponent* >( engineComponent->AsPathLibComponent() );
	if ( !component )
	{
		return false;
	}
	CMetalinkConfiguration spawnData;
	Bool isUseable = component->ConfigureGraph( spawnData, pathlib );

	section.End();

	if ( !isUseable )
	{
		return false;
	}

	AreaProcessing functor( this, prevMapping, newMapping, spawnData );

	// iterate areas around
	pathlib.GetInstanceMap()->IterateAreasAt( spawnData.m_bbox, &functor );

	const PathLib::CTerrainInfo& terrainInfo = pathlib.GetTerrainInfo();
	Int32 tileXMin, tileYMin, tileXMax, tileYMax;
	pathlib.GetTerrainInfo().GetTileCoordsAtPosition( spawnData.m_bbox.Min.AsVector2(), tileXMin, tileYMin );
	pathlib.GetTerrainInfo().GetTileCoordsAtPosition( spawnData.m_bbox.Max.AsVector2(), tileXMax, tileYMax );
	for ( Int32 y = tileYMin; y <= tileYMax; ++y )
	{
		for ( Int32 x = tileXMin; x <= tileXMax; ++x )
		{
			CTerrainAreaDescription* terrainArea = pathlib.GetTerrainAreaDescription( terrainInfo.GetTileIdFromCoords( x, y ) );
			if ( terrainArea )
			{
				functor.HandleArea( terrainArea );
			}
		}
	}

	mapper->ObstaclesMappingUpdated( e.m_componentMapping, Move( newMapping ), prevMapping, true );

	return true;
}

Bool IMetalinkComponent::IsNoticableInGame( CProcessingEvent::EType eventType ) const
{
	switch ( eventType )
	{
	case CProcessingEvent::TYPE_ATTACHED:
	case CProcessingEvent::TYPE_DETACHED:
		return true;
	case CProcessingEvent::TYPE_REMOVED:
	case CProcessingEvent::TYPE_UPDATED:
		return false;
	default:
		ASSUME( false );
	}
}
Bool IMetalinkComponent::IsNoticableInEditor( CProcessingEvent::EType eventType ) const
{
	switch ( eventType )
	{
	case CProcessingEvent::TYPE_ATTACHED:
	case CProcessingEvent::TYPE_REMOVED:
	case CProcessingEvent::TYPE_UPDATED:
		return true;
	case CProcessingEvent::TYPE_DETACHED:
		return false;
	default:
		ASSUME( false );
	}
}
IMetalinkComponent* IMetalinkComponent::AsMetalinkComponent()
{
	return this;
}

IAIQueueMetalinkInterface* IMetalinkComponent::GetAIQueueInterface()
{
	return NULL;
}



};				// namespace PathLib

