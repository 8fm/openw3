/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "pivot.h"
#include "areaPivotEdit.h"
#include "undoVertex.h"
#include "../../common/engine/hitProxyObject.h"
#include "../../common/engine/dynamicLayer.h"
#include "../../common/engine/areaComponent.h"

IMPLEMENT_ENGINE_CLASS( CEdPivotEdit );

CEdPivotEdit::CEdPivotEdit()
	: m_world( NULL )
	, m_editedComponent( NULL )
	, m_pivotEntity( NULL )
{
}

String CEdPivotEdit::GetCaption() const
{
	return TXT( "Pivot edit" );
}

Bool CEdPivotEdit::Start( CEdRenderingPanel* viewport, CWorld* world, wxSizer* sizer, wxPanel* panel, const TDynArray< CComponent* >& selection )
{
	m_world = world;

	m_world->GetSelectionManager()->SetGranularity( CSelectionManager::SG_Entities );

	// Start editing
	for ( Uint32 i = 0; i < selection.Size(); i++ )
	{
		CComponent* component = selection[i];

		if( component->IsA< CAreaComponent >() )
		{
			m_editedComponent = Cast< CAreaComponent >( component );
		}
	}

	// No area components to edit
	if ( m_editedComponent == NULL )
	{
		WARN_EDITOR( TXT("No area components selected") );
		return false;
	}

	// Create virtual pivot entity
	CLayer* dynamicLayer = world->GetDynamicLayer();
	ASSERT( dynamicLayer != NULL );

	// Setup spawn info
	EntitySpawnInfo info;
	info.m_entityClass = CPivotEditorEntity::GetStaticClass();
	info.m_spawnPosition = m_editedComponent->GetEntity()->GetPosition();
	
	// Create pivot entity
	m_pivotEntity = Cast< CPivotEditorEntity >( dynamicLayer->CreateEntitySync( info ) );
	ASSERT( m_pivotEntity != NULL );

	// Create pivot component
	SComponentSpawnInfo spawnInfo;
	spawnInfo.m_name = TXT("Pivot");
	m_pivotEntity->CreateComponent( CPivotComponent::GetStaticClass(), spawnInfo );

	// Initialized
	return true;
}

void CEdPivotEdit::End()
{
	// Get offset
	Vector offset = m_pivotEntity->GetPosition() - m_editedComponent->GetEntity()->GetPosition();

	// Set up new pivot point
	m_editedComponent->GetEntity()->SetPosition( m_pivotEntity->GetPosition() );

	// Update vertices
	CAreaComponent::TAreaPoints points = m_editedComponent->GetLocalPoints();
	for( CAreaComponent::TAreaPoints::iterator pointIter = points.Begin();
		pointIter != points.End(); ++pointIter )
	{
		Vector& point = *pointIter;
		point -= offset;
	}
	m_editedComponent->SetLocalPoints( points );

	// Destroy pivot entity
	if( ! m_pivotEntity->IsDestroyed() )
	{
		m_pivotEntity->Destroy();
	}

	m_pivotEntity = NULL;
	m_editedComponent = NULL;
}

Bool CEdPivotEdit::HandleSelection( const TDynArray< CHitProxyObject* >& objects )
{
    CSelectionManager::CSelectionTransaction transaction(*m_world->GetSelectionManager());

	// Deselect all selected object
	if ( !RIM_IS_KEY_DOWN( IK_Shift ) )
	{
		m_world->GetSelectionManager()->DeselectAll();
	}

	// Select only edited vertices
	for ( Uint32 i = 0; i < objects.Size(); i++ )
	{
		CPivotEditorEntity * pivot = Cast< CPivotEditorEntity >( objects[i]->GetHitObject()->GetParent() );
		if ( pivot == NULL || pivot->IsSelected() )
		{
			continue;
		}

		m_world->GetSelectionManager()->Select( pivot );
	}
	
	// Handled
	return true;
}
