/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "selectionManager.h"
#include "../physics/physicsWrapper.h"
#include "clipMap.h"
#include "../physics/physicsWorldUtils.h"
#include "../physics/physicsWorld.h"
#include "entityGroup.h"
#include "entity.h"
#include "componentIterator.h"
#include "layer.h"
#include "world.h"
#include "component.h"
#include "worldIterators.h"
#include "layerInfo.h"
#include "tagManager.h"
#include "../core/feedback.h"
#include "nodeHelpers.h"

Uint32 CSelectionManager::CSelectionTransaction::sm_lastUsedId = 0;

CSelectionManager::CSelectionTransaction::CSelectionTransaction( CSelectionManager& manager )
	: m_manager( &manager ), m_id( ++sm_lastUsedId )
{
	++m_manager->m_transactionsInProgress;

	if ( m_id == 0 ) 
	{
		m_id = ++sm_lastUsedId; // id 0 is reserved as "no transaction"
	}
}

CSelectionManager::CSelectionTransaction::~CSelectionTransaction()
{
    --m_manager->m_transactionsInProgress;

	if ( m_manager->m_scheduleLayerSelectionChangedEvent )
	{
		m_manager->SendLayerSelectionChangedEvent( m_id );
	}

    if ( m_manager->m_scheduleNodeSelectionChangedEvent )
	{
        m_manager->SendNodeSelectionChangedEvent( m_id );
	}
}

CSelectionManager::CSelectionManager( CWorld* world )
	: m_world( world )
	, m_pivot( NULL )
	, m_activeLayer( nullptr )
	, m_granularity( SG_Entities )
	, m_selectMode( SM_MultiLayer )
	, m_layerSelection( NULL )
    , m_transactionsInProgress( 0 )
    , m_scheduleNodeSelectionChangedEvent( false )
    , m_scheduleLayerSelectionChangedEvent( false )
	, m_blockEvent( false )
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().RegisterListener( CNAME( Detached ), this );
#endif
}

CSelectionManager::~CSelectionManager()
{
#ifndef NO_EDITOR_EVENT_SYSTEM
	SEvents::GetInstance().UnregisterListener( this );
#endif
	DeselectAll();
}

void CSelectionManager::SendNodeSelectionChangedEvent( Uint32 transactionId /*= 0*/ )
{
	if ( m_blockEvent )
	{
		return;
	}

    if ( m_transactionsInProgress > 0 )
	{
        m_scheduleNodeSelectionChangedEvent = true;
	}
    else
    {
        m_scheduleNodeSelectionChangedEvent = false;
        
        EDITOR_DISPATCH_EVENT( CNAME( SelectionChanged ), CreateEventData( SSelectionEventData( m_world, transactionId ) ) );
    }
}

void CSelectionManager::SendLayerSelectionChangedEvent( Uint32 transactionId /*= 0*/ )
{
	if ( m_blockEvent )
	{
		return;
	}

    if ( m_transactionsInProgress > 0 )
	{
        m_scheduleLayerSelectionChangedEvent = true;
	}
    else
    {
        m_scheduleLayerSelectionChangedEvent = false;

		EDITOR_DISPATCH_EVENT( CNAME( LayerSelectionChanged ), CreateEventData( SSelectionEventData( m_world, transactionId ) ) );
    }
}

void CSelectionManager::SetSelectMode( ESelectMode selectMode )
{
	m_selectMode = selectMode;
	DeselectAll();
}

ESelectMode CSelectionManager::GetSelectMode() const
{
	return m_selectMode;
}

void CSelectionManager::SetGranularity( ESelectionGranularity granularity )
{
	// Cancel current selection
	DeselectAll();

	// Change selection granularity
	m_granularity = granularity;
}

void CSelectionManager::InvertSelection()
{
	// warning when user want select all objects on all layers
	if( GetSelectMode() == SM_MultiLayer )
	{
		if( GFeedback->AskYesNo( TXT("You want invert selection on all loaded layers. Do you want to continue this process?") ) == false )
		{
			return;
		}
	}

	CSelectionTransaction transaction(*this);

	// gather all nodes
	TDynArray< CNode* > nodeCollection;
	GatherAllNodes( nodeCollection );

	for( auto it=nodeCollection.Begin(); it!=nodeCollection.End(); ++it )
	{
		CNode* node = ( *it );
		Bool isSelected = node->IsSelected();
		SetSelection( node, !isSelected );
	}

	// Send event
	SendNodeSelectionChangedEvent();
}

void CSelectionManager::SelectAllWithTheSameEntityTemplate()
{
	CSelectionTransaction transaction(*this);

	// gather all entity templates
	TDynArray< CEntityTemplate* > entityTemplates;
	for( auto it=m_selection.Begin(); it!=m_selection.End(); ++it )
	{
		CNode* node = ( *it );
		CEntity* entity = Cast< CEntity >( node );
		if( entity == nullptr )
		{
			continue;
		}

		// get information abouut entity template
		CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
		if( entityTemplate != nullptr )
		{
			entityTemplates.PushBackUnique( entityTemplate );
		}
	}

	// Deselect all
	DeselectAll();

	// Select entities with the same entity template
	TDynArray< CNode* > nodeCollection;
	GatherAllNodes( nodeCollection );

	for( auto it=nodeCollection.Begin(); it!=nodeCollection.End(); ++it )
	{
		CNode* node = ( *it );
		CEntity* entity = Cast< CEntity >( node );
		if( entity == nullptr )
		{
			CComponent* component = Cast< CComponent >( node );
			if( component != nullptr )
			{
				entity = component->GetEntity();
				if( entity == nullptr )
				{
					continue;
				}
			}
		}

		for( auto itTemp = entityTemplates.Begin(); itTemp != entityTemplates.End(); ++itTemp )
		{
			CEntityTemplate* entTmp = ( *itTemp );
			if( entTmp == entity->GetEntityTemplate() )
			{
				Select( entity );
				break;
			}
		}
	}

	// Send event
	SendNodeSelectionChangedEvent();
}

void CSelectionManager::SelectByTags( const TagList& tagList )
{
	CTagManager* tagsManager = m_world->GetTagManager();
	if( tagsManager != nullptr )
	{
		TDynArray< CEntity* > entities;
		tagsManager->CollectTaggedEntities( tagList, entities, BCTO_MatchAll );

		ESelectMode selectMode = GetSelectMode();
		if( selectMode == SM_ActiveLayer )
		{
			if( m_activeLayer != nullptr )
			{
				CLayer* activeLayer = m_activeLayer->GetLayer();
				for( auto it = entities.Begin(); it!=entities.End(); ++it )
				{
					CEntity* entity = ( *it );
					if( entity->GetLayer() != activeLayer )
					{
						entities.EraseFast( it );
					}
				}
			}
		}

		//
		for( auto it = entities.Begin(); it!=entities.End(); ++it )
		{
			CEntity* entity = ( *it );
			if( entity != nullptr )
			{
				if( entity->IsSelected() == false )
				{
					Select( entity );
				}
			}
		}
	}
}

void CSelectionManager::DeselectOnLayer( CLayer *layer )
{
	TDynArray< CNode * > removed;

	for ( Uint32 i = 0; i < m_selection.Size(); i++ )
	{
		if ( m_selection[i]->GetLayer() == layer )
		{
			if( m_selection[i]->IsSelected() == false )
			{
				continue;
			}
			m_selection[i]->m_objectFlags &= ~NF_Selected;
			m_selection[i]->OnSelectionChanged();
			removed.PushBack( m_selection[i] );
		}
	}

	for ( Uint32 i = 0; i < removed.Size(); i++ )
	{
		m_selection.Remove( removed[i] );
		m_roots.Remove( removed[i] );
	}

	UpdatePivot();
	SendNodeSelectionChangedEvent();
}

void CSelectionManager::DestroySelectedNodes()
{
	// Copy selection
	TDynArray< CEntity* > entities;
	GetSelectedEntities( entities );

	// Mark all layers as modified
	for ( Uint32 i=0; i<entities.Size(); i++ )
	{
		CNode* node = entities[i];
		if ( !node->MarkModified() )
		{
			// Not able to modify this file
			return;
		}
	}

	// Deselect all objects
	DeselectAll();

	// Destroy entities
	for ( Uint32 i=0; i<entities.Size(); i++ )
	{
		entities[i]->Destroy();
	}	
}

TDynArray< CEntity* > CSelectionManager::DuplicateSelectedEntities()
{
	// Copy selection
	TDynArray< CEntity* > entities = GetSelectedEntities();

	if ( entities.Empty() ) 
	{
		return TDynArray< CEntity* >();
	}

	// Store initial position and offset
 	Vector spawnPosition = m_pivot->GetWorldPosition();
 	Vector savedOffset = m_pivotOffset;

	TDynArray< CEntity* > clones;

	// Clone map maps originals to their clones
	THashMap< CEntity*, CEntity* > cloneMap;

	// Skip entities placed on dynamic layers (like VertexEntities) and those that cannot be modified. 
	// Do this upfront by removing from the list, to not unnecessarily de-select them at the end.
	entities.Erase(
		RemoveIf( entities.Begin(), entities.End(), 
			[]( const CEntity* e ) {  
				return !e->GetLayer()->GetLayerInfo() || !e->GetLayer()->MarkModified();
			} ),
		entities.End()
		);

	// Push entity groups at the bottom of the entities list so they'll be processed last once all entity clones are known
	RemoveIf( entities.Begin(), entities.End(), []( CEntity* ent ) { return ent->IsA< CEntityGroup >(); } );

	for ( CEntity* entity : entities  )
	{
		// Update entity's streaming buffers in case there are modified parameters
#ifndef NO_EDITOR
		{
			Red::System::ScopedFlag< Bool > block( m_blockEvent = true, false );

			if ( entity->GetEntityTemplate() == nullptr )
			{
				entity->UpdateStreamedComponentDataBuffers();
			}
		}
#endif

		if ( CEntity* clone = Cast<CEntity>( entity->Clone( NULL ) ) ) 
		{
			cloneMap.Insert( entity, clone );

			// Clear entity group clones before attaching them
			if ( clone->IsA<CEntityGroup>() )
			{
				static_cast<CEntityGroup*>( clone )->DeleteAllEntities();
			}

#ifndef NO_EDITOR
			clone->SetLocalToWorldFromTransformDirectly();
#endif
			// apply an offset to cloned entity, so that the created proxies have unique positions - occlusion uses hash from position
			static const Vector cloneOffset( 0.01f, 0.01f, 0.0f );
  			clone->SetPosition( spawnPosition + clone->GetWorldPosition() - entities[0]->GetWorldPosition() + cloneOffset );
  			clone->SetRotation( entity->GetRotation() );
			clone->ForceUpdateTransformNodeAndCommitChanges();
			clone->ForceUpdateBoundsNode();
			clone->SetName( entity->GetLayer()->GenerateUniqueEntityName( entity->GetName() ) );
			clone->OnPasted( entity->GetLayer() );
			clone->ClearFlag( NF_Attached );
			CLayer* layer = entity->GetLayer();
			layer->AddEntity( clone );
#ifndef NO_EDITOR
			clone->EditorPostDuplication( entity );
			clone->CreateStreamedComponents( SWN_NotifyWorld );
#endif

			// Put the clones in a cloned entity group instead of the originals
			if ( CEntityGroup* eg = Cast< CEntityGroup >( clone ) )
			{
				TDynArray< CEntity* > originals = SafeCast< CEntityGroup >( entity )->GetEntities();
				for ( CEntity* original : originals )
				{
					if ( CEntity** clonedRef = cloneMap.FindPtr( original ) )
					{
						eg->AddEntity( *clonedRef );
					}
					else
					{
						HALT( "Selection to duplicate does not contain all entities of a selected entity group" );
					}
				}
			}

			clones.PushBack( clone ); 
		}
	}

	// Restore pivot offset
	m_pivotOffset = savedOffset;

	{
		CSelectionTransaction transaction( *this );
		Deselect( CastArray< CNode >( entities ) );
		Select( CastArray< CNode >( clones ) );
	}

	return clones;
}

Uint32 CSelectionManager::GetEntitiesSelectionCount() const
{
	Uint32 result = 0;
	
	TDynArray< CNode* >::const_iterator i;
	for ( i = m_selection.Begin(); i != m_selection.End(); ++i )
	{
		if ( (*i)->IsA< CEntity >() ) ++result;
	}

	return result;
}

void CSelectionManager::GetSelectedNodes( TDynArray< CNode* >& nodes ) const
{
	// Just copy selection list
	nodes.PushBack( m_selection );
}

TDynArray< CNode* > CSelectionManager::GetSelectedNodes() const
{
	return m_selection;
}

void CSelectionManager::GetSelectedRoots( TDynArray< CNode* >& nodes ) const
{
    nodes = m_roots;
}

TDynArray< CNode* > CSelectionManager::GetSelectedRoots() const
{
	return m_roots;
}

void CSelectionManager::FilterSelection( ISelectionFilter *filter )
{
	if ( !filter )
	{
		return;
	}

	TDynArray< CNode* > filteredSelection;
	for ( Uint32 i=0; i<m_selection.Size(); i++ )
	{
		if ( filter->FilterNode( m_selection[i] ) )
		{
			filteredSelection.PushBack( m_selection[i] );
		}
	}

    CSelectionTransaction transaction(*this);

	DeselectAll();

	for( Uint32 i=0; i<filteredSelection.Size(); ++i )
	{
		if ( !filteredSelection[i]->IsSelected() )
		{
			Select( filteredSelection[i] );
		}
	}
}

void CSelectionManager::GetSelectedEntities( TDynArray< CEntity* >& entities ) const
{
	// Just copy entity list
	for ( Uint32 i=0; i<m_selection.Size(); i++ )
	{
		CEntity* entity = Cast< CEntity >( m_selection[i] );
		if ( entity )
		{
			if( m_selection[i]->IsSelected() == false )
			{
				continue;
			}
			entities.PushBack( entity );
		}
	}
}

TDynArray< CEntity* > CSelectionManager::GetSelectedEntities() const
{
	TDynArray< CEntity* > selection;
	GetSelectedEntities( selection );
	return selection;
}

void CSelectionManager::GetSelectedComponentsFiltered( CClass* filterClass, TDynArray< CComponent* >& components ) const
{
	// Filter list
	for ( Uint32 i=0; i<m_selection.Size(); i++ )
	{
		CComponent* component = Cast< CComponent >( m_selection[i] );

		// Filter
		if ( component )
		{
			if( m_selection[i]->IsSelected() == false )
			{
				continue;
			}

			if ( component->IsA( filterClass ) )
			{
				components.PushBack( component );
			}
		}
	}
}

void CSelectionManager::SelectFromLayer( CLayer* layer )
{
	if ( layer )
	{
        CSelectionTransaction transaction(*this);

		DeselectAll();
		TDynArray< CEntity* > entities;
		layer->GetEntities( entities );
		for( TDynArray< CEntity* >::iterator it=entities.Begin(); it!=entities.End(); it++ )
			Select( *it );
	}
}

void CSelectionManager::UpdatePivot()
{
	m_isPivotUpdateScheduled = false;
	// Use first selected root as a pivot object
	if ( m_roots.Size() )
	{
		Bool hasScheduledTransformUpdate = false;
		// If any of root has scheduled transform update
		for ( Uint32 i=0; i<m_roots.Size(); ++i )
		{
			hasScheduledTransformUpdate = m_roots[i]->HasScheduledUpdateTransform();
			if( hasScheduledTransformUpdate == true )
			{
				m_isPivotUpdateScheduled = true;
			}
		}
		
		m_pivot = m_roots[0];

		Vector minPos, maxPos;
		for ( Uint32 i=0; i<m_roots.Size(); ++i )
		{
			Vector pos = m_roots[i]->GetWorldPosition();
			if ( i == 0 || pos.X < minPos.X ) minPos.X = pos.X;
			if ( i == 0 || pos.Y < minPos.Y ) minPos.Y = pos.Y;
			if ( i == 0 || pos.Z < minPos.Z ) minPos.Z = pos.Z;
			if ( i == 0 || pos.X > maxPos.X ) maxPos.X = pos.X;
			if ( i == 0 || pos.Y > maxPos.Y ) maxPos.Y = pos.Y;
			if ( i == 0 || pos.Z > maxPos.Z ) maxPos.Z = pos.Z;
		}

		SetPivotPosition( ( maxPos + minPos )*0.5f );
	}
	else
	{
		m_pivot = NULL;
		m_pivotOffset = Vector::ZEROS;
	}
}

CEntityGroup* CSelectionManager::CheckIfPartOfAnyGroup( CEntity* entity )
{
	// Get layer
	CLayer* layer = entity->GetLayer();
	ASSERT( layer != NULL );

	const LayerEntitiesArray& entities = layer->GetEntities();

	for( LayerEntitiesArray::const_iterator entityIter = entities.Begin();
		entityIter != entities.End(); ++entityIter )
	{
		CEntity* ent = *entityIter;
		if( ent == NULL )
		{
			// Shouldn't happen but whatever...
			continue;
		}

		if( ent->IsA< CEntityGroup >() )
		{
			CEntityGroup* group = Cast< CEntityGroup >( ent );

			// Check if this group is a part of other group
			Bool isEmbedded = false;
			for( LayerEntitiesArray::const_iterator groupIter = entities.Begin();
				groupIter != entities.End(); ++groupIter )
			{
				if( !*groupIter || *entityIter == *groupIter )
				{
					continue;
				}

				if( ( *groupIter )->IsA< CEntityGroup >() )
				{
					if( CheckIfPartOfGroup( group, Cast< CEntityGroup >( *groupIter ) ) )
					{
						isEmbedded = true;
						break;
					}
				}
			}

			if( isEmbedded )
			{
				continue;
			}

			if( CheckIfPartOfGroup( entity, group ) )
			{
				return group;
			}
		}
	}
	
	return NULL;
}

Bool CSelectionManager::CheckIfPartOfGroup( CEntity* entity, CEntityGroup* group )
{
	ASSERT( entity != NULL );
	ASSERT( group != NULL );

	for( TDynArray< CEntity* >::const_iterator entityIter = group->GetEntities().Begin();
		entityIter != group->GetEntities().End(); ++entityIter )
	{
		CEntity* ent = *entityIter;
		if ( ent )
		{
			if ( ent == entity )
			{
				return true;
			}
			else if( ent->IsA< CEntityGroup >() )
			{
				CEntityGroup* g = Cast< CEntityGroup >( ent );
				if( CheckIfPartOfGroup( entity, g ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}

void CSelectionManager::ExtractGroupNodes( CEntityGroup* group, TDynArray< CNode* >& nodes )
{
	ASSERT( group != NULL );

	nodes.PushBackUnique( group );

	for( TDynArray< CEntity* >::const_iterator entityIter = group->GetEntities().Begin();
		entityIter != group->GetEntities().End(); ++entityIter )
	{
		CEntity* ent = *entityIter;

		if( ent->IsA< CEntityGroup >() )
		{
			CEntityGroup* g = Cast< CEntityGroup >( ent );
			ExtractGroupNodes( g, nodes );
		}
		else
		{
			nodes.PushBackUnique( ent );
		}
	}
}

void CSelectionManager::GetNodeList( CNode* baseNode, TDynArray< CNode* >& nodes,
			TDynArray< CNode* >& roots, Bool ignoreGroups )
{
	// Support groups
	static TDynArray< CNode* > groupNodes;
	groupNodes.ClearFast();

	// We will now check if selected object is a part of any group
	// First extract entity to check
	CEntity* check = NULL;
	if ( baseNode->IsA< CEntity >() )
	{
		check = Cast< CEntity >( baseNode ); 
	}
	else if( baseNode->IsA< CComponent >() && m_granularity == SG_Entities )
	{
		CComponent* component = SafeCast< CComponent >( baseNode );
		check = component->GetEntity();
	}
	
	if ( check != NULL )
	{
		// Make sure the entity is still in a regular layer (f.e. not deleted
		// and lives in the undo stack)
		if ( !check->GetLayer() )
		{
			return;
		}

		CEntityGroup* group = CheckIfPartOfAnyGroup( check );
		if( group != NULL && ! ignoreGroups && group->IsSelected() == false && group->IsLocked() == true )
		{
			ExtractGroupNodes( group, groupNodes );
		}
		else
		{
			// Add base node
			groupNodes.PushBackUnique( check );
		}
	}
	else
	{
		// Add base node
		groupNodes.PushBackUnique( baseNode );
	}

	for( TDynArray< CNode* >::const_iterator nodeIter = groupNodes.Begin();
		nodeIter != groupNodes.End(); ++nodeIter )
	{
		CNode* node = *nodeIter;

		// Select by granularity
		if ( m_granularity == SG_Components )
		{
			// Get list of components to select
			if ( node->IsA< CEntity >() )
			{
				CEntity* entity = static_cast< CEntity* >( node );

				// Select all entity components
				for ( BaseComponentIterator it( entity ); it; ++it )
				{
					// Add to both list
					CComponent* component = *it;
					nodes.PushBackUnique( component );
					roots.PushBackUnique( component );
				}
			}
			else
			{
				// Just select single component
				nodes.PushBackUnique( node );
				roots.PushBackUnique( node );
			}
		}
		else if ( m_granularity == SG_Entities )
		{
			// Get entity
			CEntity* baseEntity = NULL;
			if ( node->IsA< CEntity >() )
			{
				// We have clicked an entity
				baseEntity = SafeCast< CEntity >( node );
			}
			else if ( node->IsA< CComponent >() )
			{
				// We have clicked an component, get entity from it
				CComponent* component = SafeCast< CComponent >( node );
				baseEntity = component->GetEntity();
				ASSERT( baseEntity );
			}

			// Get list of components to select
			if ( baseEntity )
			{
				// Components are not the roots but are selected
				for ( BaseComponentIterator it( baseEntity ); it; ++it )
				{
					nodes.PushBackUnique( *it );
				}

				// Entity is a root and gets selected
				roots.PushBackUnique( baseEntity );
				nodes.PushBackUnique( baseEntity );
			}
		}
	}
}

void CSelectionManager::SelectLayer( ISerializable* layer )
{
	m_layerSelection = layer;

	// Send event
    SendLayerSelectionChangedEvent();
}

void CSelectionManager::SelectAll()
{
	// warning when user want select all objects on all layers
	if( GetSelectMode() == SM_MultiLayer )
	{
		if( GFeedback->AskYesNo( TXT("You want select all entities on all loaded layers. Do you want to continue this process?") ) == false )
		{
			return;
		}
	}

	TDynArray< CNode* > nodes;
	GatherAllNodes( nodes );
	Select( nodes );
}

void CSelectionManager::Select( CNode* node, Bool ignoreGroups )
{
	ASSERT( node );
	if ( !node )
	{
		return;
	}

	TDynArray< CNode* > nodes;
	nodes.PushBack( node );
	Select( nodes, ignoreGroups );
}

void CSelectionManager::Select( TDynArray< CNode* > nodes, Bool ignoreGroups )
{
	// Check that we still have nodes to select
	if ( nodes.Empty() )
	{
		return;
	}

	// Get roots and nodes
	TDynArray< CNode* > allNodes, roots;
	for ( CNode* node : nodes )
	{
		GetNodeList( node, allNodes, roots, ignoreGroups );
	}

	// Select nodes
	for ( CNode* node : allNodes )
	{
		if( m_selection.Exist( node ) )
		{
			continue;
		}

		// Select
		m_selection.PushBackUnique( node );
		node->m_objectFlags |= NF_Selected;
		node->OnSelectionChanged();

		EDITOR_QUEUE_EVENT( CNAME( NodeSelected ), CreateEventData( node ) );
	}

	// Add roots
	for ( Uint32 i=0; i<roots.Size(); i++ )
	{
		CNode* node = roots[i];
		ASSERT( node->IsSelected() );	// Roots should be selected by now		

		// Select
		m_roots.PushBackUnique( node );
	}

	// Send event
	SendNodeSelectionChangedEvent();

	// Update pivot
	UpdatePivot();
}

void CSelectionManager::DeselectAll()
{
	if ( m_layerSelection )
	{
		m_layerSelection = NULL;
		SendLayerSelectionChangedEvent();
	}

	TDynArray< CNode* > nodes = m_selection;
	Deselect( nodes );
	ASSERT( m_selection.Empty() );
}

void CSelectionManager::Deselect( CNode* node )
{
	ASSERT( node );

	TDynArray< CNode* > nodes;
	nodes.PushBack( node );
	Deselect( nodes );
}

void CSelectionManager::Deselect( const TDynArray< CNode* >& nodes )
{
	if ( nodes.Empty() )
	{
		return;
	}

	// Get roots and nodes
	TDynArray< CNode* > allNodes, roots;
	for ( CNode* node : nodes )
	{
		GetNodeList( node, allNodes, roots, true );
	}

	// Deselect nodes
	for ( CNode* node : allNodes )
	{
		if( !m_selection.Exist( node ) )
		{
			continue;
		}

		// Deselect
		m_selection.Remove( node );
		node->m_objectFlags &= ~NF_Selected;
		node->OnSelectionChanged();

		EDITOR_QUEUE_EVENT( CNAME( NodeDeselected ), CreateEventData( node ) );
	}

	// Add roots
	for ( Uint32 i=0; i<roots.Size(); i++ )
	{
		CNode* node = roots[i];
		ASSERT( !node->IsSelected() );	// Roots should be deselected by now

		// Select
		m_roots.Remove( node );
	}

	// Update pivot
	UpdatePivot();

	// Send event
	SendNodeSelectionChangedEvent();
}

CNode* CSelectionManager::GetPivot() const
{
	return m_pivot;
}

Vector CSelectionManager::GetPivotPosition() const
{
	return m_pivot ? m_pivotPosition : Vector::ZEROS;
}

EulerAngles CSelectionManager::GetPivotRotation() const
{
	return GetPivotLocalToWorldMatrix().ToEulerAngles();
}

Vector CSelectionManager::GetPivotOffset() const
{
	return m_pivotOffset;
}

Matrix CSelectionManager::GetPivotLocalToWorldMatrix() const
{
	if ( m_pivot )
	{
		Matrix localWithOffset = m_pivot->GetLocalToWorld();
		localWithOffset.SetTranslation( localWithOffset.GetTranslation() + m_pivotOffset );

		return localWithOffset;
	}

	return Matrix::IDENTITY;
}

void CSelectionManager::SetPivotPosition( const Vector& position )
{
	ASSERT( !m_pivot || m_pivot->IsSelected() );
	m_pivotPosition = position;

	Matrix toWorld = GetNodeLocalToWorldMatrix( m_pivot );
	m_pivotOffset = m_pivotPosition - toWorld.TransformPoint( m_pivot->GetPosition() );
}

void CSelectionManager::SetPivotOffset( const Vector& offset )
{
	m_pivotOffset = offset;
}

Bool CSelectionManager::ModifySelection()
{
	for ( Uint32 i = 0; i < m_selection.Size(); i++ )
	{
		if ( !m_selection[i]->MarkModified() )
		{
			return false;
		}
	}
	return true;
}

#ifndef NO_EDITOR_EVENT_SYSTEM
void CSelectionManager::DispatchEditorEvent( const CName& name, IEdEventData* data )
{
	if ( name == CNAME( Detached ) )
	{
		CNode *node = GetEventData< CNode* >( data );

		if ( m_selection.Exist( node ) )
		{
			node->Select( false );
			node->m_objectFlags &= ~NF_Selected;
			m_selection.Remove( node );
			m_roots.Remove( node );

			// Update pivot
			UpdatePivot();

			// Send event
			SendNodeSelectionChangedEvent();
		}
	}
}
#endif // NO_EDITOR_EVENT_SYSTEM

void CSelectionManager::RefreshPivot()
{
	if( m_pivot != nullptr )
	{
		Matrix toWorld = GetNodeLocalToWorldMatrix( m_pivot );
		m_pivotPosition = toWorld.TransformPoint ( m_pivot->GetPosition() ) + m_pivotOffset;
	}
}

void CSelectionManager::SetActiveLayer( CLayerInfo* activeLayer )
{
	m_activeLayer = activeLayer;
}

CLayerInfo* CSelectionManager::GetActiveLayer() const
{
	return m_activeLayer;
}

void CSelectionManager::GatherAllNodes( TDynArray< CNode* >& nodeCollection )
{
	if( m_granularity == SG_Components )
	{
		for( WorldAttachedComponentsIterator it( m_world ); it; ++it )
		{
			nodeCollection.PushBack( *it );
		}
	}
	else if( m_granularity == SG_Entities )
	{
		for( WorldAttachedEntitiesIterator it( m_world ); it; ++it )
		{
			nodeCollection.PushBack( *it );
		}
	}

	// check nodes which are on active layer
	if( GetSelectMode() == SM_ActiveLayer )
	{
		if( m_activeLayer != nullptr )
		{
			CLayer* activeLayer = m_activeLayer->GetLayer();
			for( auto it = nodeCollection.Begin(); it!=nodeCollection.End(); ++it )
			{
				CNode* node = ( *it );
				if( node != nullptr )
				{
					if( node->GetLayer() != activeLayer )
					{
						nodeCollection.EraseFast( it );
					}
				}
			}
		}
		else
		{
			nodeCollection.ClearFast();
		}
	}
}

void CSelectionManager::SetSelection( CNode* node, Bool selected )
{
	if( selected == true )
	{
		Select( node );
	}
	else
	{
		Deselect( node );
	}
}
