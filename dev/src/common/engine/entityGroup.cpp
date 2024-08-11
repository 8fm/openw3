/*
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "entityGroup.h"
#include "entity.h"
#include "renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CEntityGroup )

CEntityGroup::CEntityGroup()
	: CEntity()
	, m_entities()
	, m_locked( true )
	, m_boundingBox( Box::EMPTY )
{
}

CEntityGroup::~CEntityGroup()
{
	/* intentionally empty */
}

void CEntityGroup::AddEntity( CEntity* entity )
{
#ifndef NO_EDITOR
	entity->SetPartOfAGroup( true );
	entity->SetContainingGroup( this );
#endif
	m_entities.PushBackUnique( entity );

	CalcBoundingBox();
}

void CEntityGroup::AddEntities( const TDynArray< CEntity* >& entities )
{
#ifndef NO_EDITOR
	for ( auto it=entities.Begin(); it != entities.End(); ++it )
	{
		(*it)->SetPartOfAGroup( true );
		(*it)->SetContainingGroup( this );
	}
#endif
	m_entities.PushBackUnique( entities );

	CalcBoundingBox();
}

Bool CEntityGroup::DeleteEntity( const CEntity* entity )
{
	// Try remove from current group, otherwise go deep
	if( m_entities.Remove( const_cast< CEntity* >( entity ) ) )
	{
#ifndef NO_EDITOR
		const_cast< CEntity* >( entity )->SetPartOfAGroup( false );
		const_cast< CEntity* >( entity )->SetContainingGroup( nullptr );
#endif
		return true;
	}

	for( TDynArray< CEntity* >::iterator entityIter = m_entities.Begin();
		entityIter != m_entities.End(); ++entityIter )
	{
		if( ( *entityIter )->IsA< CEntityGroup >() )
		{
			CEntityGroup* group = Cast< CEntityGroup >( *entityIter );
			if( group->DeleteEntity( entity ) )
			{
				return true;
			}
		}
	}

	CalcBoundingBox();

	return false;
}

void CEntityGroup::DeleteAllEntities()
{
#ifndef NO_EDITOR
	for ( auto it=m_entities.Begin(); it != m_entities.End(); ++it )
	{
		(*it)->SetPartOfAGroup( false );
		(*it)->SetContainingGroup( nullptr );
	}
#endif
	m_entities.Clear();

	CalcBoundingBox();
}

const TDynArray< CEntity* >& CEntityGroup::GetEntities() const
{
	return m_entities;
}

void CEntityGroup::OnPostLoad()
{
	// Remove all NULL references
	for ( Int32 i = m_entities.Size() - 1; i >= 0; --i )
	{
		if ( m_entities[ i ] == nullptr )
		{
			m_entities.RemoveAtFast( i );
		}
#ifndef NO_EDITOR
		else
		{
			m_entities[ i ]->SetPartOfAGroup( true );
			m_entities[ i ]->SetContainingGroup( this );
		}
#endif
	}

	m_entities.Shrink();

	CalcBoundingBox();
}

void CEntityGroup::OnDestroyed( CLayer* layer )
{
	TBaseClass::OnDestroyed( layer );
	DeleteAllEntities();
}

void CEntityGroup::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if( flags == SHOW_Selection )
	{
		if( this->IsSelected() == true )
		{
			if( this->IsLocked() == true )
			{
				frame->AddDebugBrackets( m_boundingBox, GetLocalToWorld(), Color::GREEN, true, true );
			}
			else
			{
				frame->AddDebugBrackets( m_boundingBox, GetLocalToWorld(), Color::RED, true, true );
			}
		}
	}
}

void CEntityGroup::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	CalcBoundingBox();
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_Selection );
}

void CEntityGroup::OnDetached( CWorld* world )
{
	TBaseClass::OnDetached( world );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_Selection );
}

Box CEntityGroup::CalcBoundingBox()
{
	m_boundingBox = Box::EMPTY;

	for( auto it=m_entities.Begin(); it!=m_entities.End(); ++it )
	{
		CEntity* entity = ( *it );
		m_boundingBox.AddBox( entity->CalcBoundingBox() );
	}

	m_boundingBox -= this->GetWorldPosition();

	return m_boundingBox;
}

void CEntityGroup::OnPostUpdateTransform()
{
	CalcBoundingBox();
}
