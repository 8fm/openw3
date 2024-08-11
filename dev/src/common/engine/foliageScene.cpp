/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageScene.h"
#include "foliageBroker.h"
#include "foliageRenderCommandDispatcher.h"
#include "foliageResourceHandler.h"
#include "grassMask.h"
#include "foliageEditionController.h"
#include "grassOccurrenceMap.h"
#include "foliageDynamicInstanceService.h"
#include "renderer.h"
#include "world.h"
#include "../core/configVar.h"
#include "foliageCollisionHandler.h"
#include "renderSettings.h"

IMPLEMENT_ENGINE_CLASS( CFoliageScene );
IMPLEMENT_ENGINE_CLASS( SFoliageLODSetting );

const Int32 defaultVisibilityDepth = 4;
const Vector2 defaultWorldDimension = Vector2( 8192.0f, 8192.0f );
const Vector2 defaultCellDimension = Vector2( 64.0f, 64.0f );

namespace Config
{
	TConfigVar< Float, Validation::FloatRange< 0, 1000, 10 > >		cvFoliageStreamingStep( "Streaming/Foliage", "StreamingStep", 10.0f );
	TConfigVar< Bool >												cvFoliageStreamingEnabled( "Streaming/Foliage", "StreamingEnabled", true );
}

const Float defaultFoliageLodExtent[ FoliageLodCount ] = 
{
	0.0f,
	1.0f,
	2.0f,
	4.0f,
	6.0f,
	9.0f,
	12.0f
};

CFoliageScene::CFoliageScene()
	:	m_worldDimensions( defaultWorldDimension ),
		m_cellDimensions( defaultCellDimension ),
		m_visibilityDepth( defaultVisibilityDepth ),
		m_editorVisibilityDepth( defaultVisibilityDepth ),
		m_grassOccurrenceMap( nullptr ),
		m_forceUpdate( true )
{
	Red::System::MemoryCopy( m_lodSetting.m_minTreeExtentPerLod, defaultFoliageLodExtent, sizeof( m_lodSetting.m_minTreeExtentPerLod ) );
}

CFoliageScene::~CFoliageScene()
{}

void CFoliageScene::Initialize( IRenderScene * renderScene )
{
	m_renderCommandDispatcher = CreateFoliageRenderCommandDispatcher( renderScene );
	m_collisionHandler = CreateFoliageCollisionHandler( FindParent< CWorld >(), GRender );
	m_resourceHandler = CreateFoliageResourceHandler( m_collisionHandler.Get(), m_renderCommandDispatcher.Get() );

	m_resourceHandler->SetLodSetting( m_lodSetting );

	Int32 visibilityDepth = GIsEditor ? m_editorVisibilityDepth : m_visibilityDepth;
	visibilityDepth = Min( Config::cvFoliageMaxVisibilityDepth.Get(), visibilityDepth );

	SFoliageBrokerSetupParameters param = 
	{ 
		m_worldDimensions, 
		m_cellDimensions, 
		visibilityDepth, 
		m_resourceHandler.Get(), 
		m_renderCommandDispatcher.Get() 
	};

	m_foliageBroker = CreateFoliageBroker( param );

	InitializeGrassMask();

	m_forceUpdate = true;
}

void CFoliageScene::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );
	m_resourceHandler->SetLodSetting( m_lodSetting );
	m_resourceHandler->Invalidate();
}

void CFoliageScene::InitializeGrassMask()
{
	if( !m_grassMask )
	{
		m_grassMask = CreateObject< CGenericGrassMask >( this );
	}
}

void CFoliageScene::UploadGenericGrassData( IRenderProxy * terrainProxy ) const
{
	if( m_grassMask && terrainProxy )
	{
		m_renderCommandDispatcher->UpdateGenericGrassMaskCommand( terrainProxy, m_grassMask.Get() );
		m_renderCommandDispatcher->RefreshGenericGrassCommand();
	}
	if ( m_grassOccurrenceMap && !m_grassOccurrenceMap->GetCellMasks().Empty() )
	{
		m_renderCommandDispatcher->UploadGrassOccurrenceMasks( m_grassOccurrenceMap->GetCellMasks() );
	}
}

void CFoliageScene::Shutdown()
{
	m_foliageBroker.Reset();
	m_resourceHandler.Reset();
	m_renderCommandDispatcher.Reset();
}

void CFoliageScene::Update( const Vector & position )
{
	if ( Config::cvFoliageStreamingEnabled.Get() )
	{
		UpdateVisibilityDepth();
		const Float distanceTreshold = Config::cvFoliageStreamingStep.Get() * Config::cvFoliageStreamingStep.Get();
		if ( m_forceUpdate || Abs( m_currentPosition.DistanceSquaredTo2D( position ) ) > distanceTreshold )
		{
			m_currentPosition = position;
			m_foliageBroker->UpdateVisibileCells( position );
			m_resourceHandler->UpdateCurrentPosition( position );
			m_forceUpdate = false;
		}
	}
}

void CFoliageScene::UpdateVisibilityDepth()
{
	Int32 maxVisibilityDepth = Config::cvFoliageMaxVisibilityDepth.Get();
	if( m_visibilityDepth != maxVisibilityDepth )
	{
		m_foliageBroker->SetVisibilityDepth( maxVisibilityDepth );
		m_visibilityDepth = maxVisibilityDepth;
		m_forceUpdate = true;
	}
}

void CFoliageScene::Tick()
{
	m_foliageBroker->Tick();
	m_resourceHandler->Tick();
}

void CFoliageScene::PrefetchPositionSync( const Vector & position )
{
	if( Config::cvFoliageStreamingEnabled.Get() )
	{
		m_currentPosition = position;
		m_foliageBroker->PrefetchPosition( position );
		m_foliageBroker->Tick();
		m_resourceHandler->UpdateCurrentPosition( position );
		m_resourceHandler->Tick();
	}
}

void CFoliageScene::PrefetchArea( const Box * boxArray, Uint32 boxCount )
{
	m_foliageBroker->PrefetchArea( boxArray, boxCount );
}

Red::TUniquePtr< CFoliageEditionController > CFoliageScene::CreateEditionController()
{
	SFoliageEditionControllerSetupParameter param = 
	{
		m_worldDimensions,
		m_cellDimensions,
		this,
		m_renderCommandDispatcher.Get(),
		m_resourceHandler.Get(),
		m_collisionHandler.Get(),
		m_foliageBroker
	};

	return CreateFoliageEditionController( param );
}

CFoliageDynamicInstanceService CFoliageScene::CreateDynamicInstanceController()
{
	return CFoliageDynamicInstanceService( m_resourceHandler );
}

void CFoliageScene::SetWorldDimensions( const Vector2 & dimension )
{
	m_worldDimensions = dimension;
}

void CFoliageScene::SetCellDimensions( const Vector2 & cellDimension )
{
	m_cellDimensions = cellDimension;
}

void CFoliageScene::SetGenericGrassMask( CGenericGrassMask * grassMask )
{
	m_grassMask = grassMask;
}

CGenericGrassMask * CFoliageScene::GetInternalGrassMask()
{
	return m_grassMask.Get();
}

void CFoliageScene::SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center )
{
	m_renderCommandDispatcher->SetTreeFadingReferencePoints( left, right, center );
}

bool CFoliageScene::IsLoading() const
{
	return m_resourceHandler->InstancesPendingInsertion() || m_foliageBroker->IsLoading();
}

void CFoliageScene::SetInternalFoliageBroker( Red::TSharedPtr< CFoliageBroker > broker )
{
	m_foliageBroker = broker;
}

void CFoliageScene::SetInternalFoliageRenderCommandDispatcher( Red::TUniquePtr< IFoliageRenderCommandDispatcher > dispatcher )
{
	m_renderCommandDispatcher = std::move( dispatcher );
}

void CFoliageScene::SetInternalFoliageResourceHandler( Red::TUniquePtr< CFoliageResourceHandler > handler )
{
	m_resourceHandler = std::move( handler );
}

#ifndef NO_RESOURCE_COOKING
void CFoliageScene::OnCook( ICookerFramework& cooker )
{
	CWorld* world = SafeCast< CWorld >( GetParent() );
	const CClipMap * clipMap = world->GetTerrain();
	if( clipMap )
	{
		m_grassOccurrenceMap = CreateGrassOccurenceMap( clipMap ).Release();
		if( m_grassOccurrenceMap )
		{
			m_grassOccurrenceMap->SetParent( this );
		}
	}	
}
#endif

void CFoliageScene::SetInternalStreamingConfig( bool value )
{
	Config::cvFoliageStreamingEnabled.Set( value );
}

const Vector & CFoliageScene::GetCurrentPosition() const
{
	return m_currentPosition;
}
