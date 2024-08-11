/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "mergedShadowMeshComponent.h"
#include "mergedWorldGeometryEntity.h"
#include "renderFragment.h"

IMPLEMENT_ENGINE_CLASS( CMergedShadowMeshComponent );

CMergedShadowMeshComponent::CMergedShadowMeshComponent( const THandle< CMesh > mesh, const TDynArray< GlobalVisID >& objects, const Float streamingDistance, const Uint8 renderMask )
	: CMergedMeshComponent( mesh, objects, streamingDistance, renderMask )
	, m_compiledDebugMesh( nullptr )
{
}

CMergedShadowMeshComponent::~CMergedShadowMeshComponent()
{
	SAFE_RELEASE( m_compiledDebugMesh );
}

void CMergedShadowMeshComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

#ifndef NO_RESOURCE_IMPORT
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_ShadowMeshDebug );
#endif
}

void CMergedShadowMeshComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );

	SAFE_RELEASE( m_compiledDebugMesh );

#ifndef NO_RESOURCE_IMPORT
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_ShadowMeshDebug );
#endif
}

#ifndef NO_RESOURCE_IMPORT

void CMergedShadowMeshComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );

	if ( flag == SHOW_ShadowMeshDebug )
	{
		// compile the mesh
		if ( !m_compiledDebugMesh )
		{
			const auto mesh = GetMeshNow();
			if ( mesh )
			{
				const Uint8 renderMask = MCR_Cascade1 | MCR_Cascade2 | MCR_Cascade3 | MCR_Cascade4;
				m_compiledDebugMesh = mesh->CompileDebugMesh( MDS_FaceFakeLighting, 0, renderMask, Color(255,235,215,255) );
			}
		}

		// draw mesh
		if ( m_compiledDebugMesh )
		{
			new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_compiledDebugMesh );
		}

		// mesh stats
		THandle< CMergedWorldGeometryEntity > gridEntity = Cast< CMergedWorldGeometryEntity >( GetEntity() );
		if ( gridEntity )
		{
			const String txt = String::Printf( TXT("Grid [%d,%d]: %1.2f KB, %d vertices, %d triangles"),
				gridEntity->GetGridCoordinates().m_x, gridEntity->GetGridCoordinates().m_y,
				gridEntity->GetPayloadDataSize() / 1024.0f,
				gridEntity->GetPayloadVertexCount(),
				gridEntity->GetPayloadTriangleCount() );

			frame->AddDebugText( GetWorldPositionRef(), txt, 0, 0, true, Color::WHITE, Color::BLACK, nullptr, true );
		}
	}
}

#endif