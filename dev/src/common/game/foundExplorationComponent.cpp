/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "foundExplorationComponent.h"
#include "explorationFinder.h"
#include "../../common/core/gatheredResource.h"
#include "../engine/renderer.h"
#include "../engine/renderFragment.h"
#include "../engine/bitmapTexture.h"

IMPLEMENT_ENGINE_CLASS( CFoundExplorationComponent );

CGatheredResource foundExplorationIcon( TXT("engine\\textures\\icons\\waypointicon.xbm"), RGF_NotCooked );

IRenderResource*	CFoundExplorationComponent::m_markerIgnored;
IRenderResource*	CFoundExplorationComponent::m_markerInvalid;

CFoundExplorationComponent::CFoundExplorationComponent()
: m_ignore( false )
{
	m_icon = foundExplorationIcon.LoadAndGet< CBitmapTexture >();
	InitializeMarkers();
}

struct SFoundExplorationMesh
{
	TDynArray< DebugVertex > vertices;
	TDynArray< Uint32 > indices;
	Uint32 startingIndex;
	Color color;
	
	void Add(Vector2 const & at)
	{
		Float v = 0.5f;
		Float h = 0.4f;
		Float d = 0.2f;
		Uint32 currStart = vertices.Size();
		new ( vertices ) DebugVertex( Vector( at.X * v,  d, ( at.Y + 1.0f ) * h ), color );
		new ( vertices ) DebugVertex( Vector( at.X * v, -d, ( at.Y + 1.0f ) * h ), color );
		if ( currStart != startingIndex )
		{
			indices.PushBack( currStart - 2);
			indices.PushBack( currStart );
			indices.PushBack( currStart + 1 );
			indices.PushBack( currStart - 2 );
			indices.PushBack( currStart + 1 );
			indices.PushBack( currStart - 1 );
		}
	}

	void Side(Uint32 a, Uint32 b, Uint32 c)
	{
		indices.PushBack(startingIndex + a * 2 + 1);
		indices.PushBack(startingIndex + b * 2 + 1);
		indices.PushBack(startingIndex + c * 2 + 1);
		indices.PushBack(startingIndex + c * 2);
		indices.PushBack(startingIndex + b * 2);
		indices.PushBack(startingIndex + a * 2);
	}

	void Close()
	{
		Uint32 currStart = vertices.Size() - 2;
		indices.PushBack( startingIndex );
		indices.PushBack( currStart );
		indices.PushBack( currStart + 1 );
		indices.PushBack( startingIndex );
		indices.PushBack( currStart + 1 );
		indices.PushBack( startingIndex + 1 );
		startingIndex = vertices.Size();
	}

	SFoundExplorationMesh( const Color& withColor )
	: startingIndex( 0 )
	, color( withColor )
	{
		Add(Vector2(-2.0f, 6.0f));
		Add(Vector2(-2.0f, 7.0f));
		Add(Vector2(-1.0f, 8.0f));
		Add(Vector2( 1.0f, 8.0f));
		Add(Vector2( 2.0f, 7.0f));
		Add(Vector2( 2.0f, 5.2f));
		Add(Vector2( 1.0f, 3.2f));
		Add(Vector2( 1.0f, 2.0f));
		Add(Vector2( 0.0f, 2.0f));
		Add(Vector2( 0.0f, 4.0f));
		Add(Vector2( 1.0f, 6.0f));
		Add(Vector2( 1.0f, 7.0f));
		Add(Vector2(-1.0f, 7.0f));
		Add(Vector2(-1.0f, 6.0f));
		Side(0, 1, 13);
		Side(13, 1, 12);
		Side(1, 2, 12);
		Side(2, 3, 12);
		Side(3, 11, 12);
		Side(3, 4, 11);
		Side(4, 10, 11);
		Side(4, 5, 10);
		Side(5, 9, 10);
		Side(5, 6, 9);
		Side(6, 8, 9);
		Side(6, 7, 8);
		Close();
		Add(Vector2( 0.0f, 0.0f));
		Add(Vector2( 1.0f, 0.0f));
		Add(Vector2( 1.0f, 1.0f));
		Add(Vector2( 0.0f, 1.0f));
		Side(0, 1, 2);
		Side(0, 2, 3);
		Close();
	}
};

IRenderResource* CFoundExplorationComponent::CreateAgentMesh( const Color& color )
{
	// Upload mesh
	SFoundExplorationMesh mesh( color );
	return GRender->UploadDebugMesh( mesh.vertices, mesh.indices );
}

void CFoundExplorationComponent::InitializeMarkers()
{
	struct InitOnce
	{
		InitOnce( )
		{		
			Color red = Color::RED;
			Color gray = Color::GRAY;
			red.A = 80;
			gray.A = 80;
			CFoundExplorationComponent::m_markerIgnored = CreateAgentMesh( gray );
			CFoundExplorationComponent::m_markerInvalid = CreateAgentMesh( red );
		}
	};
	static InitOnce initOnce;
}

void CFoundExplorationComponent::OnAttached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CFoundExplorationComponent_OnAttached );

	// Register in the editor drawing system
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Exploration );

	// Report error if this component is used inside CActor
	if ( GetEntity()->QueryActorInterface() )
	{
		String path = GetEntity()->GetEntityTemplate() ? GetEntity()->GetEntityTemplate()->GetDepotPath() : TXT("<empty template>");
		ERR_GAME( TXT("Actor has got CFoundExplorationComponent! Actor: '%ls', resource: '%ls'"), GetEntity()->GetFriendlyName().AsChar(), path.AsChar() );
	}

	CEntity* parentEntity = GetEntity();
	if ( parentEntity )
	{
		parentEntity->ScheduleUpdateTransformNode();
	}
}

void CFoundExplorationComponent::OnDetached( CWorld* world )
{
	// Pass to base class
	TBaseClass::OnDetached( world );

	// Unregister from the editor drawing system
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Exploration );
}

Color CFoundExplorationComponent::CalcSpriteColor() const
{
	if ( m_ignore )
	{
		return IsSelected() ? Color::WHITE : Color::GRAY;
	}

	return IsSelected() ? Color::LIGHT_RED : Color::RED;
}

void CFoundExplorationComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
    // Pass to base class
    TBaseClass::OnGenerateEditorFragments( frame, flag );

#ifndef NO_EDITOR
	if ( flag == SHOW_Exploration && ! SExplorationFinder::GetInstance().IsHidden() )
	{
		Matrix mat = Matrix::IDENTITY;
		Vector position = GetWorldPosition();
		GetWorldRotation().ToMatrix(mat);
		mat.SetTranslation( position );

		// Hit proxy
		if ( frame->GetFrameInfo().m_renderingMode == RM_HitProxies )
		{
			new ( frame ) CRenderFragmentDebugMesh( frame, mat, m_ignore? CFoundExplorationComponent::m_markerIgnored : CFoundExplorationComponent::m_markerInvalid, GetHitProxyID() );
			return;
		}

		new ( frame ) CRenderFragmentDebugMesh( frame, mat, m_ignore? CFoundExplorationComponent::m_markerIgnored : CFoundExplorationComponent::m_markerInvalid );
	}
#endif
}
