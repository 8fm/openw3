/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mergedMeshComponent.h"
#include "renderProxy.h"
#include "renderCommands.h"
#include "renderVisibilityExclusion.h"

IMPLEMENT_ENGINE_CLASS( CMergedMeshComponent );

CMergedMeshComponent::CMergedMeshComponent( const THandle< CMesh > mesh, const TDynArray< GlobalVisID >& objects, const Float streamingDistance, const Uint8 renderMask )
	: m_streamingDistance( streamingDistance )
	, m_renderingExclusionFilter( nullptr )
	, m_renderMask( renderMask )
{
	m_objects = objects;
	SetResource( mesh.Get() );
}

CMergedMeshComponent::~CMergedMeshComponent()
{
	RED_FATAL_ASSERT( m_renderingExclusionFilter == nullptr, "Component destroyed while still attached" );
}

Uint32 CMergedMeshComponent::GetMinimumStreamingDistance() const
{
	return Max< Uint32 >( 16, (Uint32) m_streamingDistance );
}

void CMergedMeshComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// create the exclusion filter
	if ( !m_objects.Empty() )
	{
		IRenderScene* scene = world->GetRenderSceneEx();
		if ( scene )
		{
			// create visibility exclusion list for the object that were source of this render mesh
			// when this list will be activated it will prevent those objects from being rendered to the specified render mask
			IRenderVisibilityExclusion* visibilityList = GRender->CreateVisibilityExclusion( m_objects.TypedData(), m_objects.Size(), m_renderMask, true );
			if ( visibilityList )
			{
				m_renderingExclusionFilter = visibilityList;
				(new CRenderCommand_AddRenderingExclusionToScene( scene, m_renderingExclusionFilter ))->Commit();
			}
		}
	}
}

void CMergedMeshComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	// remove the exclusion filter
	if ( m_renderingExclusionFilter != nullptr )
	{
		IRenderScene* scene = world->GetRenderSceneEx();
		if ( scene )
		{
			(new CRenderCommand_RemoveRenderingExclusionToScene( scene, m_renderingExclusionFilter ))->Commit();
		}

		m_renderingExclusionFilter->Release();
		m_renderingExclusionFilter = nullptr;
	}

	// OPTIMIZATION HACK: this component is the only user of the shadow mesh
	// release the rendering resource earlier in order to reduce memory usage
	THandle< CMesh > mesh = GetMeshNow();
	if ( mesh )
	{
		mesh->ReleaseRenderResource();
	}
}