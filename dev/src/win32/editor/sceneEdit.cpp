/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../../common/game/actionAreaVertex.h"
#include "../../common/game/sceneAreaComponent.h"
#include "../../common/game/wayPointComponent.h"
#include "../../common/core/gatheredResource.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/bitmapTexture.h"

CGatheredResource resSceneIcon( TXT("engine\\textures\\icons\\sceneicon.xbm"), RGF_Startup );

/// Editor tool for editing scene triggers
class CEdSceneEdit : public IEditorTool
{
	DECLARE_ENGINE_CLASS( CEdSceneEdit, IEditorTool, 0 );

public:
	CWorld*										m_world;						//!< World shortcut
	CSceneAreaComponent*						m_sceneArea;					//!< Scene area we edit
	
	Bool										m_cursorWorldPositionIsValid;
	Vector										m_cursorWorldPosition;

public:
	CEdSceneEdit();
	virtual String GetCaption() const { return TXT("Scene area edit"); }
	virtual Bool Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* m_panelSizer, wxPanel* panel, const TDynArray< CComponent* >& selection );
	virtual void End() {}

	virtual Bool HandleSelection( const TDynArray< CHitProxyObject* >& objects );
	virtual Bool OnDelete();
	virtual Bool OnViewportTrack( const CMousePacket& packet );
	virtual Bool OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y );

	//virtual Bool UsableInActiveWorldOnly() const { return false; }
};

BEGIN_CLASS_RTTI( CEdSceneEdit );
	PARENT_CLASS( IEditorTool );
END_CLASS_RTTI();

IMPLEMENT_ENGINE_CLASS( CEdSceneEdit );

CEdSceneEdit::CEdSceneEdit()
	: m_world( NULL )
	, m_sceneArea( NULL )
{
}

Bool CEdSceneEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	// Remember world
	m_world = world;

	m_world->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Components );

	// Start editing
	m_sceneArea = NULL;
	for ( Uint32 i=0; i < selection.Size(); i++ )
	{
		m_sceneArea = Cast< CSceneAreaComponent >( selection[ i ] );
		if ( m_sceneArea != NULL )
		{
			// Ask for template detachment if we edit main world
			if ( m_world == GGame->GetActiveWorld() && m_sceneArea->GetEntity()->GetEntityTemplate() != NULL )
			{
				if ( wxYES ==  wxMessageBox( TXT("Selected component is not detached, detach it?"), TXT("Component needs detachment"), wxYES_NO | wxCENTRE, viewport ) )
				{
					m_sceneArea->GetEntity()->DetachTemplate();
				}
				else // Cannot edit entities attached to their templates
				{
					m_sceneArea = NULL;
				}
			}

			break;
		}
	}

	// No area components to edit
	if ( m_sceneArea == NULL )
	{
		WARN_EDITOR( TXT("No scene area components selected") );
		return false;
	}

	m_cursorWorldPositionIsValid = false;

	// Initialized
	return true;
}

Bool CEdSceneEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
	CSelectionManager::CSelectionTransaction transaction( *m_world->GetSelectionManager() );

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_world->GetSelectionManager()->DeselectAll();
	}

	// Select only scene waypoints
	for ( Uint32 i = 0; i < objects.Size(); ++i )
	{
		CWayPointComponent * node = Cast< CWayPointComponent >( objects[i]->GetHitObject() );
		if ( node != NULL && node->GetParent() == m_sceneArea->GetParent() )
		{
			m_world->GetSelectionManager()->Select( node );
			break;
		}
	}

	// Handled
	return true;
}


Bool CEdSceneEdit::OnDelete()
{
	// Enumerate vertices
	TDynArray< CNode* > nodes;
	m_world->GetSelectionManager()->GetSelectedNodes( nodes );
	// Deselect'em all
	m_world->GetSelectionManager()->DeselectAll();

	// Delete vertices
	for ( Uint32 i = 0; i < nodes.Size(); ++i )
	{
		CWayPointComponent * point = Cast< CWayPointComponent >( nodes[ i ] );
		if ( point && point->GetParent() == m_sceneArea->GetParent() )
		{
			point->Destroy();
		}
	}

	// Handled
	return true;
}

Bool CEdSceneEdit::OnViewportTrack( const CMousePacket& packet )
{
	SPhysicsContactInfo contactInfo;
	const Float rayDist = 100000.f;

	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetCollisionTypeBit( CNAME( Static ) ) | GPhysicEngine->GetCollisionTypeBit( CNAME( Terrain ) );
	CPhysicsWorld* physicsWorld = nullptr;
	m_world->GetPhysicsWorld( physicsWorld );
	m_cursorWorldPositionIsValid = physicsWorld->RayCastWithSingleResult( packet.m_rayOrigin, packet.m_rayOrigin + packet.m_rayDirection * rayDist, include, 0, contactInfo ) == TRV_Hit;
	if ( m_cursorWorldPositionIsValid )
	{
		m_cursorWorldPosition = contactInfo.m_position;
	}
	else
	{
		Plane projectionPlane( Vector::EZ, Vector::ZERO_3D_POINT );

		// Calculate intersection with plane
		Float intersectionDistance;
		m_cursorWorldPositionIsValid = projectionPlane.IntersectRay( packet.m_rayOrigin, packet.m_rayDirection, m_cursorWorldPosition, intersectionDistance );
	}

	return false;
}

Bool CEdSceneEdit::OnViewportClick( IViewport* view, Int32 button, Bool state, Int32 x, Int32 y )
{
	if ( button == 0 && state )
	{
		if ( RIM_IS_KEY_DOWN( IK_Alt ) && m_cursorWorldPositionIsValid )
		{
			CEntity * entity = m_sceneArea->GetEntity();
			if ( entity->MarkModified() )
			{
				Matrix worldToLocal;
				entity->GetWorldToLocal( worldToLocal );

				SComponentSpawnInfo spawnInfo;
				spawnInfo.m_spawnPosition = worldToLocal.TransformPoint( m_cursorWorldPosition + Vector::EZ * 0.25f );

				CWayPointComponent * component = Cast<CWayPointComponent>(
						entity->CreateComponent( CWayPointComponent::GetStaticClass(), spawnInfo ) );
				component->SetSpriteIcon( resSceneIcon.LoadAndGet< CBitmapTexture >() );

				//m_world->GetSelectionManager()->DeselectAll();
				m_world->GetSelectionManager()->Select( component );
			}

			return true;
		}
	}
	return false;
}
