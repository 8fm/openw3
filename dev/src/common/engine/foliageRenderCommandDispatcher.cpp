/**
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "foliageRenderCommandDispatcher.h"
#include "foliageInstance.h"
#include "renderer.h"
#include "grassMask.h"
#include "renderCommands.h"
#include "baseTree.h"
#include "renderProxy.h"

// Implementation of the render command dispatcher for the game / editor.
class CFoliageRenderCommandDispatcher : public IFoliageRenderCommandDispatcher
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
public:
	CFoliageRenderCommandDispatcher();
	virtual ~CFoliageRenderCommandDispatcher();

	void Initialize( IRender * renderer, IRenderScene * renderScene );

	virtual void UpdateSpeedTreeInstancesCommand( SFoliageUpdateRequest & updateRequest ) const;
	virtual void CreateSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const;
	virtual void CreateSpeedTreeDynamicInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const;
	virtual void RemoveSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const Box& rect ) const;
	virtual void RemoveSpeedTreeInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const;
	virtual void RemoveSpeedTreeDynamicInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const;

	virtual void UpdateGenericGrassMaskCommand( IRenderProxy * terrainProxy, CGenericGrassMask * grassMask ) const;
	virtual void UploadGrassOccurrenceMasks( const TDynArray< CGrassCellMask >& cellMasks ) const;
	virtual void RefreshGenericGrassCommand() const;
	virtual void UpdateGrassSetupCommand( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData ) const;
	virtual void UpdateDynamicGrassCollisionCommand( const TDynArray< SDynamicCollider >& collisions ) const;

	virtual void SetDebugVisualisationModeCommand( EFoliageVisualisationMode mode ) const;

	// add ifndef no editor later
#ifndef NO_EDITOR
	virtual void UpdateFoliageRenderParams( const SFoliageRenderParams &params ) const;
#endif

	virtual void SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center );


private:
	RenderObjectHandle m_speedTreeProxy;
	TRenderObjectPtr< IRenderScene > m_renderScene;
	IRender * m_renderer;
};

// Implementation of the render command dispatcher for the cooker, or anything else with no rendering
class CNullFoliageRenderCommandDispatcher : public IFoliageRenderCommandDispatcher
{
	DECLARE_CLASS_MEMORY_ALLOCATOR( MC_Engine );
public:
	CNullFoliageRenderCommandDispatcher()	{ }
	virtual ~CNullFoliageRenderCommandDispatcher() { }

	virtual void UpdateSpeedTreeInstancesCommand( SFoliageUpdateRequest&  ) const {  }
	virtual void CreateSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const { RED_UNUSED( tree ); RED_UNUSED( instanceData ); RED_UNUSED( rect ); }
	virtual void CreateSpeedTreeDynamicInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const { RED_UNUSED( tree ); RED_UNUSED( instanceData ); RED_UNUSED( rect ); }
	virtual void RemoveSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const Box& rect ) const { RED_UNUSED( tree ); RED_UNUSED( rect ); }
	virtual void RemoveSpeedTreeDynamicInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const { RED_UNUSED( tree ); RED_UNUSED( position ); RED_UNUSED( radius ); }
	virtual void RemoveSpeedTreeInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const { RED_UNUSED( tree ); RED_UNUSED( position ); RED_UNUSED( radius ); }

	virtual void UpdateGenericGrassMaskCommand( IRenderProxy * terrainProxy, CGenericGrassMask * grassMask ) const { RED_UNUSED( terrainProxy ); RED_UNUSED( grassMask ); }
	virtual void UploadGrassOccurrenceMasks( const TDynArray< CGrassCellMask >& cellMasks ) const { RED_UNUSED( cellMasks ); }
	virtual void RefreshGenericGrassCommand() const	{ }
	virtual void UpdateGrassSetupCommand( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData ) const	{ RED_UNUSED( terrainProxy ); RED_UNUSED( renderUpdateData ); }
	virtual void UpdateDynamicGrassCollisionCommand( const TDynArray< SDynamicCollider >& collisions ) const { RED_UNUSED( collisions ); }

	virtual void SetDebugVisualisationModeCommand( EFoliageVisualisationMode mode ) const	{ RED_UNUSED( mode ); }

#ifndef NO_EDITOR
	virtual void UpdateFoliageRenderParams( const SFoliageRenderParams& params ) const { RED_UNUSED( params ); }
#endif // !NO_EDITOR

	virtual void SetTreeFadingReferencePoints( const Vector& /*left*/, const Vector& /*right*/, const Vector& /*center*/ ) {}
};

const Float MAX_GRASS_INSTANCES_PER_SQUARE_METER = 40.0f;
const Float MAX_TREE_INSTANCES_PER_SQUARE_METER = 2.0f;
const Float MAX_GRASS_LAYERS_PER_SQUARE_METER = 15.0f;

CFoliageRenderCommandDispatcher::CFoliageRenderCommandDispatcher()
	: m_renderer( nullptr )
{}

CFoliageRenderCommandDispatcher::~CFoliageRenderCommandDispatcher()
{
	if( m_speedTreeProxy )
	{
		( new CRenderCommand_RemoveSpeedTreeProxyFromScene( m_renderScene.Get(), m_speedTreeProxy.Get() ) )->Commit();
	}
}

void CFoliageRenderCommandDispatcher::Initialize( IRender * renderer, IRenderScene * renderScene )
{
	m_renderer = renderer;
	m_renderScene.ResetFromExternal( renderScene );

#ifdef USE_SPEED_TREE
	m_speedTreeProxy.Reset( m_renderer->CreateSpeedTreeProxy() );
#endif

	( new CRenderCommand_AddSpeedTreeProxyToScene( m_renderScene.Get(), m_speedTreeProxy.Get() ) )->Commit();

	( new CRenderCommand_UpdateFoliageBudgets( static_cast< IRenderProxy* >( m_speedTreeProxy.Get() ), 
		MAX_GRASS_INSTANCES_PER_SQUARE_METER, 
		MAX_TREE_INSTANCES_PER_SQUARE_METER, 
		MAX_GRASS_LAYERS_PER_SQUARE_METER ) 
	)->Commit();

	SetDebugVisualisationModeCommand( VISUALISE_NONE );
}

void CFoliageRenderCommandDispatcher::UpdateSpeedTreeInstancesCommand( SFoliageUpdateRequest & updateRequest ) const
{
	if( !updateRequest.addRequestContainer.Empty() || !updateRequest.removeRequestContainer.Empty() )
	{
		( new CRenderCommand_UpdateSpeedTreeInstances( m_speedTreeProxy, std::move( updateRequest ) ) )->Commit();
	}
}

void CFoliageRenderCommandDispatcher::CreateSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const
{
	( new CRenderCommand_CreateSpeedTreeInstances( m_speedTreeProxy, tree->AcquireRenderObject(), instanceData, rect ) )->Commit();
}

void CFoliageRenderCommandDispatcher::CreateSpeedTreeDynamicInstancesCommand( const CSRTBaseTree * tree, const FoliageInstanceContainer& instanceData, const Box& rect ) const
{
	( new CRenderCommand_CreateSpeedTreeDynamicInstances( m_speedTreeProxy, tree->AcquireRenderObject(), instanceData, rect ) )->Commit();
}

void CFoliageRenderCommandDispatcher::RemoveSpeedTreeInstancesCommand( const CSRTBaseTree * tree, const Box& rect ) const
{
	RenderObjectHandle renderObject = tree->GetRenderObject();
	if( renderObject )
	{
		( new CRenderCommand_RemoveSpeedTreeInstancesRect( m_speedTreeProxy, renderObject, rect ) )->Commit();
	}
}

void CFoliageRenderCommandDispatcher::RemoveSpeedTreeInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const
{
	RenderObjectHandle renderObject = tree->GetRenderObject();
	if( renderObject )
	{
		( new CRenderCommand_RemoveSpeedTreeInstancesRadius( m_speedTreeProxy, renderObject, position, radius ) )->Commit();
	}
}

void CFoliageRenderCommandDispatcher::RemoveSpeedTreeDynamicInstancesRadiusCommand( const CSRTBaseTree * tree, const Vector3& position, Float radius ) const
{
	RenderObjectHandle renderObject = tree->GetRenderObject();
	if( renderObject )
	{
		( new CRenderCommand_RemoveSpeedTreeDynamicInstancesRadius( m_speedTreeProxy, renderObject, position, radius ) )->Commit();
	}
}

void CFoliageRenderCommandDispatcher::UpdateGenericGrassMaskCommand( IRenderProxy * terrainProxy, CGenericGrassMask * grassMask ) const
{
	( new CRenderCommand_UpdateGenericGrassMask( terrainProxy, grassMask->GetGrassMask(), grassMask->GetGrassMaskRes() ) )->Commit();
}

void CFoliageRenderCommandDispatcher::UploadGrassOccurrenceMasks( const TDynArray< CGrassCellMask >& cellMasks ) const
{
	( new CRenderCommand_UploadGenericGrassOccurrenceMap( m_speedTreeProxy, cellMasks ) )->Commit();
}

void CFoliageRenderCommandDispatcher::RefreshGenericGrassCommand() const
{
	( new CRenderCommand_RefreshGenericGrass( m_speedTreeProxy ) )->Commit();
}

void CFoliageRenderCommandDispatcher::UpdateGrassSetupCommand( IRenderProxy * terrainProxy, IRenderObject * renderUpdateData ) const
{
	( new CRenderCommand_UpdateGrassSetup( terrainProxy, m_speedTreeProxy.Get(), renderUpdateData ) )->Commit();
}

void CFoliageRenderCommandDispatcher::SetDebugVisualisationModeCommand( EFoliageVisualisationMode mode ) const
{
	( new CRenderCommand_SetFoliageVisualisation( reinterpret_cast< IRenderProxy* >( m_speedTreeProxy.Get() ), mode ) )->Commit();
}

void CFoliageRenderCommandDispatcher::UpdateDynamicGrassCollisionCommand( const TDynArray< SDynamicCollider >& collisions ) const
{
	( new CRenderCommand_UpdateDynamicGrassColissions( m_speedTreeProxy, collisions ) )->Commit();
}

Red::TUniquePtr< IFoliageRenderCommandDispatcher > CreateFoliageRenderCommandDispatcher( IRenderScene * scene )
{
#ifdef USE_SPEED_TREE

	if( scene != nullptr )
	{
		Red::TUniquePtr< CFoliageRenderCommandDispatcher > factory( new CFoliageRenderCommandDispatcher );
		factory->Initialize( GRender, scene );
		return std::move( factory );
	}
	else
	{
		LOG_ENGINE( TXT( "Foliage system is using Null render command dispatcher" ) );
		return Red::TUniquePtr< IFoliageRenderCommandDispatcher >( new CNullFoliageRenderCommandDispatcher );
	}

#else

	return Red::TUniquePtr< IFoliageRenderCommandDispatcher >( new CNullFoliageRenderCommandDispatcher );

#endif
}

#ifndef NO_EDITOR
void CFoliageRenderCommandDispatcher::UpdateFoliageRenderParams( const SFoliageRenderParams &params ) const
{
	( new CRenderCommand_UpdateFoliageRenderParams( m_speedTreeProxy.Get(), params ) )->Commit();
}
#endif


void CFoliageRenderCommandDispatcher::SetTreeFadingReferencePoints( const Vector& left, const Vector& right, const Vector& center )
{
	( new CRenderCommand_SetTreeFadingReferencePoints( reinterpret_cast< IRenderProxy* >( m_speedTreeProxy.Get() ), left, right, center ) )->Commit();
}
