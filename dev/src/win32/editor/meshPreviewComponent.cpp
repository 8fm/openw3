/**
* Copyright © 2009 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "meshPreviewComponent.h"
#include "../../common/engine/mesh.h"
#include "../../common/engine/renderer.h"
#include "../../common/engine/renderFragment.h"
#include "../../common/engine/collisionShape.h"
#include "../../common/engine/collisionMesh.h"


IMPLEMENT_ENGINE_CLASS( CMeshPreviewComponent );



CMeshPreviewComponent::CMeshPreviewComponent()
	: m_showBoundingBox( false )
	, m_showCollision( false )
	, m_activeCollisionShape( -1 )
	, m_activeCollisionShapeDebugMesh( NULL )
{
	SetStreamed( false );
}

CMeshPreviewComponent::~CMeshPreviewComponent()
{
	SAFE_RELEASE( m_activeCollisionShapeDebugMesh );
}



void CMeshPreviewComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_VisualDebug );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Collision );

	// Need to listen for collision mesh changed events, so we can keep our active shape up-to-date.
	SEvents::GetInstance().RegisterListener( CNAME( OnCollisionMeshChanged ), this );
}

void CMeshPreviewComponent::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	SEvents::GetInstance().UnregisterListener( CNAME( OnCollisionMeshChanged ), this );

	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_VisualDebug );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Collision );

	SAFE_RELEASE( m_activeCollisionShapeDebugMesh );
}


void CMeshPreviewComponent::SetActiveCollisionShape( Int32 shapeIndex )
{
	m_activeCollisionShape = shapeIndex;

	SAFE_RELEASE( m_activeCollisionShapeDebugMesh );
}


void CMeshPreviewComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	// Pass to base class
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( flags == SHOW_VisualDebug )
	{
		// Generate the bounding box
		if ( m_showBoundingBox && TryGetMesh() )
		{
			frame->AddDebugBox( TryGetMesh()->GetBoundingBox(), Matrix::IDENTITY, Color::RED );
		}


		if ( m_showCollision )
		{
			// Draw collision shapes.
			const CCollisionMesh* mesh = TryGetMesh() ? TryGetMesh()->GetCollisionMesh() : NULL;
			if ( mesh )
			{
				// If we have a selected collision shape, draw it.
				if ( m_activeCollisionShape != -1 )
				{
					const TDynArray< ICollisionShape* >& shapes = mesh->GetShapes();
					if ( ( Uint32 )m_activeCollisionShape < shapes.Size() )
					{
						ICollisionShape* shape = shapes[m_activeCollisionShape];
						// Generate debug mesh if needed
						if ( shape && !m_activeCollisionShapeDebugMesh )
						{
							TDynArray< DebugVertex > vertices;
							TDynArray< Uint32 > indices;

							// Generate drawing data from shapes
							shape->GenerateDebugMesh( vertices, indices );

							// Upload to card
							if ( indices.Size() > 0 )
							{
								m_activeCollisionShapeDebugMesh = GRender->UploadDebugMesh( vertices, indices );
							}
						}

						// Generate rendering fragment. The collision shapes, drawn below, are plain wireframe. We make the active shape stand out
						// by drawing it filled in.
						if ( m_activeCollisionShapeDebugMesh )
						{
							new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_activeCollisionShapeDebugMesh, true );
							new ( frame ) CRenderFragmentDebugMesh( frame, GetLocalToWorld(), m_activeCollisionShapeDebugMesh, false, true );
						}
					}
				}

				// Setup rendering context
				CCollisionMesh::RenderContext renderContext;
				renderContext.m_hitProxyID = GetHitProxyID();
				renderContext.m_localToWorld = GetLocalToWorld();
				renderContext.m_selected = IsSelected();
				// For the mesh preview component, we only draw the selected shape as solid, so the collision mesh should be wire only.
				renderContext.m_solid = false;

				// Generate collision mesh fragments
				mesh->GenerateFragments( frame, renderContext );
			}
		}
	}

	// Draw skeleton
	/************************************************************************/
	/*																		*/
	/* LMF importer commented out.                                          */
	/* It was made to unify importing assets from 3ds max by RE exporter	*/
	/* (REXMesh). Right now asset with verts and indices and faces limits	*/
	/* per material chunk should be prepare by artist in 3ds max			*/
	/* instead of cutting it in engine. It was done for more control		*/
	/* in bounding boxes of chunks, for better occlusion and performance.	*/
	/*																		*/
	/************************************************************************/
	/*
	extern TDynArray< DebugVertex > GDebugSkeletonLines;
	frame->AddDebugLines( GDebugSkeletonLines.TypedData(), GDebugSkeletonLines.Size(), true );
	*/
}

void CMeshPreviewComponent::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( OnCollisionMeshChanged ) )
	{
		// Release our debug mesh, so it can be re-created.
		SAFE_RELEASE( m_activeCollisionShapeDebugMesh );
	}
}
