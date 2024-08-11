#include "build.h"

#include "undoCreate.h"
#include "undoManager.h"
#include "../../common/engine/entityGroup.h"

IMPLEMENT_ENGINE_CLASS( CUndoCreateDestroy );

//#define DISABLE_UNDO_CREATE_DESTROY


String CUndoCreateDestroy::GetName()
{
	if ( !m_stepName.Empty() )
	{
		return m_stepName;
	}
	else
	{
		Bool createdEntities     = !m_createdData.m_entities.Empty();
		Bool createdComponents   = !m_createdData.m_components.Empty();
		Bool createdAttachements = !m_createdData.m_attachments.Empty();

		Bool sthCreated = createdEntities || createdComponents || createdAttachements;

		Bool deletedEntities     = !m_removedData.m_entities.Empty();
		Bool deletedComponents   = !m_removedData.m_components.Empty();
		Bool deletedAttachements = !m_removedData.m_attachments.Empty();

		Bool sthDeleted = deletedEntities || deletedComponents || deletedAttachements;

		if ( sthCreated )
		{
			if ( sthDeleted )
			{
				return TXT("creation / destruction");
			}

			return TXT("creation");
		}

		if ( sthDeleted )
		{
			return TXT("destruction");
		}
	}

	// nothing created nor removed - shouldn't get here
	return TXT("???");
}

void CUndoCreateDestroy::BuildTargeStringOn( Data& data )
{
	// Target entities
	for ( Uint32 i=0; i < data.m_entities.Size(); ++i )
	{
		if ( CObject* originalParent = data.m_entities[i].m_layer.Get() )
		{
			m_targetString += originalParent->GetFriendlyName() + TXT("::") + data.m_entities[i].m_entity->GetName();
		}
		else
		{
			m_targetString += data.m_entities[i].m_entity->GetFriendlyName();
		}

		if ( i + 1 < data.m_entities.Size() ) m_targetString += TXT(", ");
	}

	// Target components
	for ( Uint32 i=0; i < data.m_components.Size(); ++i )
	{
		if ( CObject* originalParent = data.m_components[i].m_entity.Get() )
		{
			m_targetString += originalParent->GetFriendlyName() + TXT("::");
		}

		m_targetString += data.m_components[i].m_component->GetName();

		if ( i + 1 < data.m_components.Size() ) m_targetString += TXT(", ");
	}

	// Target attachments
	for ( Uint32 i=0; i < data.m_attachments.Size(); ++i )
	{
		if ( CEntity* entity = data.m_attachments[i].m_entity.Get() )
		{
			m_targetString += entity->GetFriendlyName() + TXT(" ");
		}

		m_targetString += String::Printf( TXT("from %s to %s"), data.m_attachments[i].m_parentComponent.AsChar(), data.m_attachments[i].m_childComponent.AsChar() );

		if ( i + 1 < data.m_components.Size() ) m_targetString += TXT(", ");
	}
}

String CUndoCreateDestroy::GetTarget()
{
	if ( m_targetString.Empty() )
	{
		BuildTargeStringOn( m_createdData );
		BuildTargeStringOn( m_removedData );
	}

	return m_targetString;
}

CUndoCreateDestroy::Data& CUndoCreateDestroy::PrepareStep( CEdUndoManager* undoManager, CObject* object, Bool undoCreation )
{
	CUndoCreateDestroy *stepToAdd = undoManager->SafeGetStepToAdd< CUndoCreateDestroy >();

	if ( !stepToAdd )
	{
		stepToAdd = new CUndoCreateDestroy( *undoManager, undoCreation );
		undoManager->SetStepToAdd( stepToAdd );
	}

	if ( !undoCreation )
	{
		// inform the manager that we're taking care of this object
		undoManager->NotifyObjectTrackedForRemoval( object );
	}

	return undoCreation ? stepToAdd->m_createdData : stepToAdd->m_removedData;
}

void CUndoCreateDestroy::CreateStep( CEdUndoManager* undoManager, CEntity * entity, Bool undoCreation )
{
#ifdef DISABLE_UNDO_CREATE_DESTROY
	if ( !undoCreation )
	{
		// Just delete entity
		CLayer* layer = entity->GetLayer();
		if ( layer )
		{
			entity->EditorPreDeletion();
			layer->RemoveEntity( entity );
		}
	}
#else
	Data& data = PrepareStep( undoManager, entity, undoCreation );

	data.m_entities.PushBack( EntityInfo() );
	EntityInfo& ei = data.m_entities.Back();
	ei.m_layer = entity->GetLayer();
	
	if ( undoCreation )
	{
		ei.m_entity = entity;
		ei.m_parentGroup = entity->GetContainingGroup();
	}
	else
	{
		ei.m_entity = entity;
		ei.m_parentGroup = entity->GetContainingGroup();

		if( ei.m_parentGroup != nullptr )
		{
			ei.m_parentGroup->DeleteEntity( entity );
		}

		for ( CComponent* comp : entity->GetComponents() )
		{
			undoManager->NotifyObjectTrackedForRemoval( comp );
		}

		entity->EditorPreDeletion();
		// Remove original entity
		entity->CreateStreamedComponents( SWN_NotifyWorld );
		ei.m_layer.Get()->RemoveEntity( entity );
		undoManager->GetWorld()->DelayedActions();
	}
#endif
}

void CUndoCreateDestroy::CreateStep( CEdUndoManager* undoManager, CComponent * component, Bool undoCreation, Bool updateStreaming )
{
#ifdef DISABLE_UNDO_CREATE_DESTROY
	if ( !undoCreation )
	{
		// Just delete component
		CEntity* entity = component->GetEntity();
		if ( entity )
		{
			component->EditorPreDeletion();
			entity->RemoveComponent( component );
		}
	}
#else
	Data& data = PrepareStep( undoManager, component, undoCreation );

	data.m_components.PushBack( ComponentInfo() );
	ComponentInfo& ci = data.m_components.Back();
	ci.m_entity = component->GetEntity();

	if ( undoCreation )
	{
		ci.m_component = component;
	}
	else
	{
		for ( IAttachment* atachment : component->GetChildAttachments() )
		{
			CreateStep( undoManager, atachment, undoCreation );
		}

		for ( IAttachment* atachment : component->GetParentAttachments() )
		{
			CreateStep( undoManager, atachment, undoCreation );
		}

		ci.m_component = component;

		component->EditorPreDeletion();
		// Remove original entity
		ci.m_entity.Get()->RemoveComponent( component );
		if ( updateStreaming )
		{
			ci.m_entity.Get()->UpdateStreamedComponentDataBuffers();
			ci.m_entity.Get()->UpdateStreamingDistance();
		}
		undoManager->GetWorld()->DelayedActions();
	}
#endif
}

void CUndoCreateDestroy::CreateStep( CEdUndoManager* undoManager, IAttachment * attachment, Bool undoCreation )
{
#ifdef DISABLE_UNDO_CREATE_DESTROY
	if ( !undoCreation )
	{
		attachment->Break();
	}
#else
	Data& data = PrepareStep( undoManager, attachment, undoCreation );

	CComponent* parentComponent = Cast< CComponent >( attachment->GetParent() );
	CComponent* childComponent = Cast< CComponent >( attachment->GetChild() );
	if ( parentComponent && childComponent )
	{
		data.m_attachments.PushBack( AttachmentInfo() );
		AttachmentInfo& ai = data.m_attachments.Back();

		ai.m_childComponent = childComponent->GetName();
		ai.m_parentComponent = parentComponent->GetName();
		ASSERT( ai.m_childComponent != ai.m_parentComponent );

		ai.m_entity = parentComponent->GetEntity();
		// We support messing with components and attachments only inside single entity during one undo session
		ASSERT( parentComponent->GetEntity() == childComponent->GetEntity() && "Unsupported usage of the UndoCreateDestroy functionality" );

		if ( undoCreation )
		{
			ai.m_attachment = attachment;
		}
		else
		{
			ai.m_attachment = attachment;
			attachment->Break();
		}
	}
#endif
}

void CUndoCreateDestroy::FinishStep( CEdUndoManager* undoManager, const String& stepName )
{
#ifndef DISABLE_UNDO_CREATE_DESTROY
	ASSERT( undoManager );

	if ( CUndoCreateDestroy *stepToAdd = undoManager->SafeGetStepToAdd< CUndoCreateDestroy >() )
	{
		stepToAdd->m_stepName = stepName;

		// re-parent removed data
		Data& data = stepToAdd->m_removedData;

		for ( EntityInfo& info : data.m_entities )
		{
			if ( info.m_layer.Get() )
			{
				info.m_entity->CObject::SetParent( stepToAdd );
			}
		}

		for ( ComponentInfo& info : data.m_components )
		{
			if ( info.m_entity.Get() )
			{
				info.m_component->CObject::SetParent( stepToAdd );
			}
		}

		for ( AttachmentInfo& info : data.m_attachments )
		{
			if ( info.m_entity.Get() )
			{
				info.m_attachment->CObject::SetParent( stepToAdd );
			}
		}

		stepToAdd->PushStep();
	}
#endif
}

void CUndoCreateDestroy::DoRemoveObjectOn( Data& data, CObject *object )
{
	for ( const EntityInfo& info : data.m_entities )
	{
		if ( info.m_entity == object || info.m_layer.Get() == object )
		{
			PopStep();
			return;
		}
	}

	for ( const ComponentInfo& info : data.m_components  )
	{
		if ( info.m_entity.Get() == object || info.m_component == object )
		{
			PopStep();
			return;
		}
	}

	for ( const AttachmentInfo& info : data.m_attachments )
	{
		CComponent* component = Cast< CComponent >( object );
		if ( component && component->GetEntity() && component->GetEntity() == info.m_entity.Get() )
		{
			if ( info.m_childComponent == component->GetName() || info.m_parentComponent == component->GetName() )
			{
				PopStep();
				return;
			}
		}
		
	}
}

void CUndoCreateDestroy::OnObjectRemoved( CObject *object )
{
#ifndef DISABLE_UNDO_CREATE_DESTROY
	DoRemoveObjectOn( m_createdData, object );
	DoRemoveObjectOn( m_removedData, object );
#endif
}

void CUndoCreateDestroy::DoCreationOn( Data& data )
{
	for ( EntityInfo& info : data.m_entities )
	{
		if ( CLayer* layer = info.m_layer.Get() )
		{
			// Add the cloned destroyed entity
			layer->AddEntity( info.m_entity );

			// if entity is in group
			if( info.m_parentGroup != nullptr )
			{
				info.m_parentGroup->AddEntity( info.m_entity );
			}

			m_undoManager->GetWorld()->DelayedActions();
			info.m_entity->ScheduleUpdateTransformNode();
		}
	}

	for ( ComponentInfo& info : data.m_components )
	{
		if ( CEntity* entity = info.m_entity.Get() )
		{
			entity->AddComponent( info.m_component );
			entity->UpdateStreamedComponentDataBuffers();
			entity->UpdateStreamingDistance();
			m_undoManager->GetWorld()->DelayedActions();
			entity->ScheduleUpdateTransformNode();
		}
	}

	for ( AttachmentInfo& info : data.m_attachments )
	{
		if ( CEntity* entity = info.m_entity.Get() )
		{
			CComponent* parentComponent = entity->FindComponent( info.m_parentComponent );
			CComponent* childComponent = entity->FindComponent( info.m_childComponent );
			if ( parentComponent && childComponent )
			{
				info.m_attachment->Init( parentComponent, childComponent, NULL );
				info.m_attachment->SetParent( parentComponent );
			}
		}
	}
}

void CUndoCreateDestroy::DoDeletionOn( Data& data )
{
	for ( const AttachmentInfo& info : data.m_attachments )
	{
		if ( info.m_entity.Get() != NULL )
		{
			IAttachment* currentAttachment = info.m_attachment;
			m_undoManager->NotifyObjectTrackedForRemoval( currentAttachment );
			
			// Do the breaking
			currentAttachment->Break();

			currentAttachment->CObject::SetParent( this );
		}
	}

	for ( const ComponentInfo& info : data.m_components )
	{
		// This code assumes, that we don't add/remove entities, in the undo manager session in which we add/remove components.
		// If such functionality will be needed, the ComponentInfo will need to rely on entity name, instead of entity handle
		if ( CEntity* entity = info.m_entity.Get() )
		{
			CComponent* currentComponent = info.m_component;
			m_undoManager->NotifyObjectTrackedForRemoval( currentComponent );

			currentComponent->EditorPreDeletion();
			
			// Remove currently attached component
			entity->RemoveComponent( currentComponent );
			entity->UpdateStreamedComponentDataBuffers();
			entity->UpdateStreamingDistance();

			currentComponent->CObject::SetParent( this );
		}
	}

	for ( const EntityInfo& info : data.m_entities )
	{
		if ( CLayer* layer = info.m_layer.Get() )
		{
			CEntity* currentEntity = info.m_entity;
			m_undoManager->NotifyObjectTrackedForRemoval( currentEntity );

			for ( CComponent* comp : currentEntity->GetComponents() )
			{
				m_undoManager->NotifyObjectTrackedForRemoval( comp );
			}
			
			// if entity is in group
			if ( info.m_parentGroup != nullptr )
			{
				info.m_parentGroup->DeleteEntity( currentEntity );
			}

			currentEntity->EditorPreDeletion();
			// Remove currently attached entity
			layer->RemoveEntity( currentEntity );

			currentEntity->CObject::SetParent( this );
		}
	}

	m_undoManager->GetWorld()->DelayedActions();
}

void CUndoCreateDestroy::DoUndo()
{
#ifndef DISABLE_UNDO_CREATE_DESTROY
	DoDeletionOn( m_createdData );
	DoCreationOn( m_removedData );
#endif
}

void CUndoCreateDestroy::DoRedo()
{
#ifndef DISABLE_UNDO_CREATE_DESTROY
	DoDeletionOn( m_removedData );
	DoCreationOn( m_createdData );
#endif
}

void CUndoCreateDestroy::OnSerialize( IFile& file )
{
#ifndef DISABLE_UNDO_CREATE_DESTROY
	TBaseClass::OnSerialize( file );

	if ( file.IsGarbageCollector() )
	{
		for ( EntityInfo& info : m_removedData.m_entities )
		{
			file << info.m_entity;
		}

		for ( ComponentInfo& info : m_removedData.m_components )
		{
			file << info.m_component;
		}

		for ( AttachmentInfo& info : m_removedData.m_attachments )
		{
			file << info.m_attachment;
		}

		for ( EntityInfo& info : m_createdData.m_entities )
		{
			file << info.m_entity;
		}

		for ( ComponentInfo& info : m_createdData.m_components )
		{
			file << info.m_component;
		}

		for ( AttachmentInfo& info : m_createdData.m_attachments )
		{
			file << info.m_attachment;
		}
	}
#endif
}
