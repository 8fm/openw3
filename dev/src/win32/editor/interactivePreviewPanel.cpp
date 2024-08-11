#include "build.h"
#include "interactivePreviewPanel.h"
#include "../../common/engine/clothComponent.h"
#include "../../common/engine/apexDestructionWrapper.h"
#include "../../common/engine/destructionSystemComponent.h"
#include "../../common/engine/rigidMeshComponent.h"
#include "../../common/engine/viewport.h"
#include "../../common/engine/worldIterators.h"
#include "../../common/engine/selectionManager.h"
#include "../../common/engine/destructionComponent.h"


CEdInteractivePreviewPanel::CEdInteractivePreviewPanel( wxWindow* parent, Bool allowRenderOptionsChange )
	: CEdPreviewPanel( parent, allowRenderOptionsChange )
	, m_draggingCloth( nullptr )
{
}


CEdInteractivePreviewPanel::~CEdInteractivePreviewPanel()
{
}


Bool CEdInteractivePreviewPanel::OnViewportTrack( const CMousePacket& packet )
{
	m_mouseX = packet.m_x;
	m_mouseY = packet.m_y;

	if ( m_draggingCloth )
	{
		UpdateDraggedCloth( packet.m_viewport );

		return true;
	}

	return CEdPreviewPanel::OnViewportTrack( packet );
}

Bool CEdInteractivePreviewPanel::OnViewportInput( IViewport* view, enum EInputKey key, enum EInputAction action, Float data )
{
	// freezing simulation 
	if ( key == IK_F /*both press and release*/)
	{
		if( m_previewWorld )
		{
			FreezeSimulation( action != IACT_Press );
			return true;
		}
	}

	if ( key == IK_I )
	{
		if ( action == IACT_Press )
		{
			BeginInteraction( view, m_mouseX, m_mouseY );
		}
		else if ( action == IACT_Release )
		{
			EndInteraction( view, m_mouseX, m_mouseY );
		}
	}
	if ( m_draggingCloth )
	{
		return true;
	}

	return CEdPreviewPanel::OnViewportInput( view, key, action, data );
}


void CEdInteractivePreviewPanel::BeginInteraction( IViewport* view, Int32 x, Int32 y )
{
	Vector worldCoords, worldNormal;

	Vector origin, dir;
	view->CalcRay( x, y, origin, dir );

	// Trace in physics world, see if our ray intersects any physical shapes.
	SPhysicsContactInfo cinfo;
	const Float RAY_DISTANCE = 1000.f;
	// For purposes of interaction, we'll just raycast against everything.
	STATIC_GAME_ONLY CPhysicsEngine::CollisionMask include = GPhysicEngine->GetWithAllCollisionMask();
	CPhysicsWorld* physicsWorld = nullptr;
	GetPreviewWorld()->GetPhysicsWorld( physicsWorld );
	Bool didHitObject = physicsWorld->RayCastWithSingleResult( origin, origin + dir * RAY_DISTANCE, include, 0, cinfo ) == TRV_Hit;
	if ( didHitObject )
	{
		worldCoords = cinfo.m_position;
		worldNormal = cinfo.m_normal;
	}


	TDynArray< CClothComponent* > clothComponents;
	GetPreviewWorld()->GetAttachedComponentsOfClass< CClothComponent >( clothComponents );

	// Find the closest cloth. Use the intersection position found above as the maximum distance, since we just want the closest thing.
	CClothComponent* closestCloth = nullptr;
	Float closestClothDist = didHitObject ? origin.DistanceTo( worldCoords ) : FLT_MAX;
	Uint32 closestVertex;

	for ( Uint32 i = 0; i < clothComponents.Size(); ++i )
	{
		CClothComponent* cloth = clothComponents[i];
		Vector hitPosition;
		Uint32 v = cloth->SelectVertex( origin, dir, hitPosition );
		if ( v != 0 )
		{
			Float dist = origin.DistanceTo( hitPosition );
			if ( dist < closestClothDist )
			{
				closestClothDist = dist;
				closestCloth = cloth;
				closestVertex = v;
			}
		}
	}

	// If we hit a cloth, start dragging it.
	if ( closestCloth )
	{
		m_draggingCloth = closestCloth;
		m_selectedClothVertex = closestVertex;
		m_clothHitDistance = closestClothDist;

		UpdateDraggedCloth( view );
	}
	// No cloth, so we'll destroy stuff.
	else
	{
		// Apply force to all selected destructible objects.
		if ( didHitObject )
		{
			TDynArray< CDestructionSystemComponent* > destructionSystemComponents;
			GetPreviewWorld()->GetAttachedComponentsOfClass< CDestructionSystemComponent >( destructionSystemComponents );

			for ( Uint32 i = 0; i < destructionSystemComponents.Size(); ++i )
			{
#ifdef USE_APEX
				CApexDestructionWrapper* wrapper = destructionSystemComponents[i]->GetDestructionBodyWrapper();
				if( !wrapper ) continue;
				wrapper->ApplyFracture();
#endif
			}

			TDynArray< CDestructionComponent* > destructionComponents;
			GetPreviewWorld()->GetAttachedComponentsOfClass< CDestructionComponent >( destructionComponents );
			for ( Uint32 i = 0; i < destructionComponents.Size(); ++i )
			{
				CPhysicsDestructionWrapper* wrapper = destructionComponents[i]->GetDestructionBodyWrapper();
				if( !wrapper ) continue;
				wrapper->ApplyFracture();
				
			}
		}
	}
}

void CEdInteractivePreviewPanel::EndInteraction( IViewport* view, Int32 x, Int32 y )
{
	// If we were dragging a cloth, stop.
	if ( m_draggingCloth )
	{
		m_draggingCloth->FreeVertex( m_selectedClothVertex );
		m_draggingCloth = nullptr;
	}
}

void CEdInteractivePreviewPanel::UpdateDraggedCloth( IViewport* view )
{
	Vector origin, dir;
	view->CalcRay( m_mouseX, m_mouseY, origin, dir );

	// Keep the dragged vertex at the same distance to the camera.
	Vector newPos = origin + dir * m_clothHitDistance;

	m_draggingCloth->MoveVertex( m_selectedClothVertex, newPos );
}

void CEdInteractivePreviewPanel::FreezeSimulation( Bool state )
{
	TDynArray< CRigidMeshComponent* > rigidComponents;
	GetPreviewWorld()->GetAttachedComponentsOfClass< CRigidMeshComponent >( rigidComponents );

	for ( CRigidMeshComponent* comp : rigidComponents )
	{
		comp->SetEnabled( state );
		comp->ScheduleUpdateTransformNode();
	}
}
