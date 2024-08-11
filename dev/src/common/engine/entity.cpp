/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "entity.h"
#include "appearanceComponent.h"
#include "behaviorGraphStack.h"
#include "fxTrackItemParameterFloat.h"
#include "../physics/PhysicsRagdollWrapper.h"
#include "renderCommands.h"
#include "soundEntityParam.h"
#include "animDangleComponent.h"
#include "mesh.h"
#include "streamingSectorData.h"
#include "meshSkinningAttachment.h"
#include "animatedComponent.h"
#include "skeletalAnimationContainer.h"
#include "skeletalAnimationEntry.h"
#include "clothComponent.h"
#include "phantomComponent.h"
#include "destructionSystemComponent.h"
#include "meshComponent.h"
#include "staticMeshComponent.h"
#include "rigidMeshComponent.h"
#include "effectDummyComponent.h"
#include "areaComponent.h"
#include "skeleton.h"
#include "pathComponent.h"
#include "triggerAreaComponent.h"
#include "componentIterator.h"
#include "pointLightComponent.h"
#include "spotLightComponent.h"
#include "furComponent.h"
#include "flareComponent.h"
#include "dynamicFoliageComponent.h"
#include "switchableFoliageComponent.h"
#include "overrideStreamingDistanceComponent.h"
#include "entityOnLayerReference.h"
#include "renderProxyIterator.h"
#include "layer.h"
#include "layerInfo.h"
#include "tickManager.h"
#include "tagManager.h"
#include "world.h"
#include "soundEmitter.h"
#include "decalComponent.h"
#include "dimmerComponent.h"
#include "animDangleComponent.h"
#include "material.h"
#include "selectionManager.h"
#include "../core/scriptStackFrame.h"
#include "../core/dependencySaver.h"
#include "../core/dependencyLoader.h"
#include "../core/scriptingSystem.h"
#include "../core/feedback.h"
#include "../core/dataError.h"
#include "../core/cooker.h"
#include "../core/fileSkipableBlock.h"
#include "../core/resourceUsage.h"
#include "entityGroup.h"
#include "mimicComponent.h"
#include "utils.h"
#include "swarmRenderComponent.h"
#include "../core/profilerTypes.h"
#include "../core/debugPageHTMLDoc.h"
#include "entityExternalAppearance.h"

//#pragma optimize ("",off)

IMPLEMENT_RTTI_ENUM( ETickGroup );
IMPLEMENT_RTTI_ENUM( EMoveType );
IMPLEMENT_RTTI_ENUM( EMoveFailureAction );
IMPLEMENT_ENGINE_CLASS( SEntitySpawnData );
IMPLEMENT_ENGINE_CLASS( CEntity );
IMPLEMENT_ENGINE_CLASS( CEntityOnLayerReference );
IMPLEMENT_RTTI_BITFIELD( EEntityStaticFlags );

#if !defined(RED_FINAL_BUILD) && defined(RED_PLATFORM_WINPC)
	#define DEBUG_COMPONENT_ISSUES
#endif

#ifndef NO_EDITOR
RED_DEFINE_STATIC_NAME( entityStaticFlags );
#endif

IEntityListener::~IEntityListener()
{
#ifndef RED_FINAL_BUILD
	if ( !m_listenedEntities.Empty() )
	{
		TDynArray< THandle< CEntity > > entities = m_listenedEntities;
		for ( Uint32 i = 0; i < entities.Size(); ++i )
		{
			CEntity* entity = entities[ i ].Get();
			ASSERT( !entity, TXT("IEntityListener was not fully unregistered!") );
			if ( entity )
			{
				entity->UnregisterEntityListener( this );
			}
			else
			{
				m_listenedEntities.RemoveFast( entities[ i ] );
			}
		}

		ASSERT( m_listenedEntities.Empty() );
		m_listenedEntities.ClearFast();
	}
#endif
}

const Float CEntity::HEADING_ANY = FLT_MAX;

CEntity::CEntity()
	: m_entityFlags( 0 )
	, m_materialReplacementInfo( NULL )
	, m_entityStaticFlags( ESF_Streamed )
	, m_entityDynamicFlags( 0 )
	, m_streamingProxy( nullptr )
//	, m_streamingDataBuffer( TDataBufferAllocator< MC_EntityStreaming >::GetInstance() ) - no memory class in W3 in the shared buffers
	, m_streamingDistance( 255 )
#ifndef NO_EDITOR
	, m_partOfAGroup( false )
	, m_containingGroup( nullptr )
	, m_forceAutoHideDistance( 0 )
#endif
{
}

CEntity::~CEntity()
{
}

void CEntity::PrepareEntityForTemplateSaving()
{
	// When saving OLD entities we need to restore the missing GUID
	RestoreGUIDIfNeeded();
}

void CEntity::OnGUIDChanged( const CGUID& oldGuid )
{
	if ( IsAttached() )
	{
		GetLayer()->GetWorld()->OnEntityGUIDChanged( this, oldGuid );
	}
}

void CEntity::SetDynamicFlag( const EEntityDynamicFlags flag, const Bool val /*= true*/ )
{
	if ( val )
	{
		m_entityDynamicFlags |= flag;
	}
	else
	{
		m_entityDynamicFlags &= ~flag;
	}
}

void CEntity::ClearDynamicFlag( const EEntityDynamicFlags flag )
{
	m_entityDynamicFlags &= ~flag;
}

void CEntity::SetStaticFlag( const EEntityStaticFlags flag, const Bool val /*= true*/ )
{
	if ( val )
	{
		m_entityStaticFlags |= flag;
	}
	else
	{
		m_entityStaticFlags &= ~flag;
	}
}

void CEntity::ClearStaticFlag( const EEntityStaticFlags flag )
{
	m_entityStaticFlags &= ~flag;
}

void CEntity::RestoreGUIDIfNeeded()
{
	const Bool isInTemplate = GetParent() && GetParent()->IsA< CEntityTemplate >();
	if ( GetGUID().IsZero() && !isInTemplate )
	{
		SetGUID( CGUID::Create() );
	}
}

void CEntity::LoadPerComponentData( IFile& file )
{
	for ( ;; )
	{
		// Component class
		CName componentClassName;
		file << componentClassName;

		// End of data
		if ( componentClassName.Empty() )
		{
			break;
		}

		// Component GUID
		CGUID componentID;
		file << componentID;

		// Find component by ID
		CComponent* targetComponent = NULL;
		if ( m_template.IsValid() )
		{
			targetComponent = FindComponent( componentID );
		}

		{
			CFileSkipableBlock block( file );

			// Load data only if the component was found and it's class is the same
			if ( targetComponent != NULL && targetComponent->GetClass()->GetName() == componentClassName && targetComponent->SupportsAdditionalInstanceData() )
			{
				targetComponent->OnSerializeAdditionalData( file );
			}

			// Sync file offset
			block.Skip();
		}
	}
}

void CEntity::SavePerComponentData( IFile& file )
{
	// Only save in a templated case - a non templated entity saves the data automatically
	if ( nullptr != m_template.Get() )
	{
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];

			// skip components that do not require additional data to be saved
			if ( !component || !component->SupportsAdditionalInstanceData() )
			{
				continue;
			}

			// save component name and ID
			CName className = component->GetClass()->GetName();
			file << className;				
			CGUID id = component->GetGUID();
			file << id;

			// save data
			{
				CFileSkipableBlock block( file );
				component->OnSerializeAdditionalData( file );
			}
		}
	}

	// save the end of data marker (always)
	CName emptyName = CName::NONE;
	file << emptyName;
}

void CEntity::LoadPerComponentInstanceProperties( IFile& file )
{
	// This is only for templated entities
	if ( m_template != nullptr )
	{
		// Load the number of gathered components
		Uint32 componentCount = 0;
		file << componentCount;

		// Load the component properties
		for ( Uint32 i=0; i <componentCount; ++i )
		{
			CFileSkipableBlock block( file );

			// Load the component's name
			CName componentName;
			file << componentName;

			// Find the component
			CComponent* component = nullptr;
			if ( !componentName.Empty() )
			{
				component = FindComponent( componentName );
			}

			// If we have a component, ask it to load its instance properties
			if ( component != nullptr )
			{
				component->LoadInstanceProperties( file );
			}

			// Make sure we're at the proper file position
			block.Skip();
		}
	}
}


void CEntity::SavePerComponentInstanceProperties( IFile& file )
{
	// This is only for templated entities
	if ( m_template != nullptr )
	{
		// Gather the components with local properties
		TDynArray< CComponent* > componentsWithInstanceProperties;
		for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
		{
			CComponent* component = *it;
			if ( component->HasInstanceProperties() )
			{
				componentsWithInstanceProperties.PushBack( component );
			}
		}

		// Save the number of gathered components
		Uint32 componentCount = componentsWithInstanceProperties.Size();
		file << componentCount;

		// Ask the components for their instance properties
		for ( auto it=componentsWithInstanceProperties.Begin(); it != componentsWithInstanceProperties.End(); ++it )
		{
			CFileSkipableBlock block( file );
			CComponent* component = *it;

			// Save the component's name
			CName componentName( component->GetName() );
			file << componentName;

			// Save the component's instance properties
			component->SaveInstanceProperties( file );
		}
	}
}

#ifndef NO_RESOURCE_COOKING

namespace 
{
	static void GatherDependencies( const void* data, size_t size, class ICookerFramework& cooker )
	{
		if ( size > 0 )
		{
			CMemoryFileReader reader( (const Uint8*) data, size, 0 );

			// load the import/export tables
			DependencyLoadingContext loadingContext;
			CDependencyLoader loader( reader, NULL );
			TDynArray< FileDependency > dependencies;
			if ( loader.LoadDependencies( dependencies, true ) )
			{
				// pass the dependencies to the dependency tracker
				for ( const FileDependency& dep : dependencies )
				{
					if ( dep.m_isSoftDepdencency )
						cooker.ReportSoftDependency( dep.m_depotPath );
					else
						cooker.ReportHardDependency( dep.m_depotPath );
				}
			}
		}
	}
}


void CEntity::OnCook( class ICookerFramework& cooker )
{
	TBaseClass::OnCook( cooker );

	// capture additional dependencies hidden in the streaming buffers
	GatherDependencies( GetLocalStreamedComponentDataBuffer().GetData(), GetLocalStreamedComponentDataBuffer().GetSize(), cooker );
}

#endif // NO_RESOURCE_COOKING

#ifndef NO_EDITOR
namespace Helper
{
	extern void ForceUpdateTransformForEntity( CNode* node );
};

void CEntity::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{
	// handle appearances
	CAppearanceComponent* appearance = CAppearanceComponent::GetAppearanceComponent( this );
	if ( appearance )
	{
		CName apperanceName = appearance->GetForcedAppearance();
		if ( apperanceName.Empty() )
		{
			CEntityTemplate* entityTemplate = m_template.Get();
			if ( entityTemplate )
			{
				const TDynArray<CName>& appearanceList = entityTemplate->GetEnabledAppearancesNames();
				if ( !appearanceList.Empty() )
				{
					apperanceName = appearanceList[ 0 ];
				}
			}
		}
		
		if ( !apperanceName.Empty() )
		{
			appearance->ApplyAppearance( apperanceName );
		}
	}

	// synchronously stream-in all components
	CreateStreamedComponents( SWN_DoNotNotifyWorld );

	Helper::ForceUpdateTransformForEntity( this );

	// do component specyfic code
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->OnNavigationCook( world, context );
	}
}
void CEntity::OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context )
{
	Helper::ForceUpdateTransformForEntity( this );
	// do component specyfic code
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->OnNavigationCookerInitialization( world, context );
	}
}
void CEntity::PostNavigationCook( CWorld* world )
{
	// do component specyfic code
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->PostNavigationCook( world );
	}
}
#endif

void CEntity::OnSerialize( IFile& file )
{
	// Serialize base class
	TBaseClass::OnSerialize( file );

	// Handle specific serialization issues
	if ( file.IsReader()  )
	{
		// When loading old entities we need to restore the missing GUID
		RestoreGUIDIfNeeded();

		// Update components
		if ( nullptr == m_template.Get() )
		{
			// Load components only for non templated entities
			file << m_components;
		}
		else
		{
			// Grab streaming settings from entity template
			ASSERT( m_template->GetEntityObject(), TXT("No entity object in template!") );
			if ( m_template->GetEntityObject() != nullptr )
			{
				SetStreamed( m_template->GetEntityObject()->ShouldBeStreamed() );
				m_streamingDistance = m_template->GetEntityObject()->m_streamingDistance;
			}
		}

		// Remove NULL pointers from component array (for safety)
		RemoveEmptyPointers( m_components );

		// Load additional per-component data
		if ( file.GetVersion() >= VER_ADDITIONAL_COMPONENT_DATA)
		{
			LoadPerComponentData( file );

			// Load instance properties
			if ( file.GetVersion() >= VER_TEMPLATE_INSTANCE_PARAMETERS )
			{
				LoadPerComponentInstanceProperties( file );
			}
		}
	}
	else if ( file.IsGarbageCollector() || file.IsResourceCollector() )	// Collector writer => send all components
	{
		file << m_components;
	}
	else if ( !m_template.IsValid() )									// Regular writer => save local non-streamed components
	{
		// Collect all the components to be written to disk
		TDynArray< CComponent* > componentsToWrite;
		for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
		{
			CComponent* component = *it;
			
			// Do not write streamed components
			if ( ShouldBeStreamed() && component->IsStreamed() )
			{
				continue;
			}

			// Ask the component if we should write it to disk
			if ( component->ShouldWriteToDisk() )
			{
				componentsToWrite.PushBack( component );
			}
		}

		// Save the array
		file << componentsToWrite;
	}

	if ( file.IsGarbageCollector() && m_materialReplacementInfo && m_materialReplacementInfo->material )
	{
		file << m_materialReplacementInfo->material;
	}

	// In a templated entity there is still a limited way to save special data
	if ( file.GetVersion() >= VER_ADDITIONAL_COMPONENT_DATA && file.IsWriter() && !file.IsGarbageCollector() )
	{
		SavePerComponentData( file );
		SavePerComponentInstanceProperties( file );
	}
}

void CEntity::OnFinalize()
{
	RED_ASSERT( !IsAttached() );
	RED_ASSERT( !IsAttaching() );
	RED_ASSERT( !IsDetaching() );
	//RED_ASSERT( !IsUninitializing() );
	RED_ASSERT( !IsInitialized() );

	TBaseClass::OnFinalize();
}

void CEntity::Destroy()
{
	RED_FATAL_ASSERT( SIsMainThread(), "Can only call Destroy from main thread." );

	if( CheckDynamicFlag( EDF_DuringUpdateTransform ) == false )
	{
		ExecuteDestroy();
	}
	else
	{
		SetDynamicFlag( EDF_DestroyAfterTransformUpdate, true );
	}
}

void CEntity::ProcessPendingDestroyRequest()
{
	RED_FATAL_ASSERT( SIsMainThread(), "Can only call ProcessPendingDestroyRequest from main thread." );

	ResetDuringUpdateTransformFlag();
	if( CheckDynamicFlag( EDF_DestroyAfterTransformUpdate ) == true )
	{
		ExecuteDestroy();
	}
}

void CEntity::ExecuteDestroy()
{
	RED_FATAL_ASSERT( SIsMainThread(), "Can only call ExecuteDestroy from main thread." );

	// Ask layer to destroy this entity
	// We need to ask the layer because it's our owner
	RED_FATAL_ASSERT( CheckDynamicFlag( EDF_DuringUpdateTransform ) == false, "Can't destroy entity when it's being updated by Update Transform Manager." );
	GetLayer()->DestroyEntity( this );
}

void RecursiveImmediateTransformUpdate( CNode* node )
{
	if ( CEntity* e = node->AsEntity() )
	{
		e->ForceUpdateTransformNodeAndCommitChanges();
	}
	else if ( CComponent* c = node->AsComponent() )
	{
		c->ForceUpdateTransformNodeAndCommitChanges();
	}
	else
	{
		node->ForceUpdateTransformNodeAndCommitChanges();
	}
}

Bool CEntity::Teleport( const Vector& position, const EulerAngles& rotation )
{
	SetPosition( position );
	SetRotation( rotation );
	return true;
}

Bool CEntity::Teleport( CNode* node, Bool applyRotation /*= true*/ )
{
	ASSERT( node );
	SetPosition( node->GetWorldPosition() );
	if( applyRotation )
	{
		SetRotation( node->GetWorldRotation() );
	}

	return true;
}

// Generate unique name, if name is changed return true
Bool CEntity::GenerateUniqueComponentName( CComponent* component ) const
{
	// Name already used ?
    if ( !component->GetName().Empty() )
    {
        CComponent *found = FindComponent( component->GetName() );
	    if ( !found || found == component )
	    {
		    return false;
	    }
    }

	// If name is not given generate one
	// TODO: make this more efficient :)
	for ( Uint32 i=0; ; i++ )
	{
		// Assemble name
		Char testName[ 256 ];

RED_MESSAGE(  "CEntity::GenerateUniqueComponentName - this presents a problem for removing strings from cnames at release" )
		Red::System::SNPrintF( testName, ARRAY_COUNT(testName), TXT("%s%i"), component->GetClass()->GetName().AsString().AsChar(), i );

		// If not used take it
		if ( !FindComponent( testName ) )
		{
			//WARN_ENGINE( TXT("Renaming component '%ls' to '%ls' in entity '%ls'. Renaming..."), component->GetName().AsChar(), testName, GetName().AsChar() );
			component->SetName( testName );
			return true;
		}
	}
}

CComponent* CEntity::CreateComponent( CClass* componentClass, const SComponentSpawnInfo& spawnInfo )
{
	ASSERT( componentClass );
	ASSERT( !componentClass->IsAbstract() );
	ASSERT( componentClass->IsBasedOn( ClassID< CComponent >() ) );
	ASSERT( !IsDestroyed() );

	CComponent* defaultComponent = componentClass->GetDefaultObject< CComponent >();
	if( defaultComponent )
	{
		if( defaultComponent->IsIndividual() )
		{
			for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
			{
				if (  (*it)->GetClass()->GetName() == componentClass->GetName() )
				{
					DestroyComponent( *it );
					break;
				}
			}
		}
	}

	// Create component
	CComponent* comp = CreateObject< CComponent >( componentClass, this );
	if ( !comp )
	{
		WARN_ENGINE( TXT("Unable to create component of class '%ls'"), componentClass->GetName().AsString().AsChar() );
		return NULL;
	}

	// Create GUID for new element
	comp->SetGUID( CGUID::Create() );

	// No name
	if ( spawnInfo.m_name == String::EMPTY )
	{
		// Make sure component name is valid
		GenerateUniqueComponentName( comp );
	}
	else
	{
		// General component name
		comp->SetName( spawnInfo.m_name );
	}

	// Register in component list
	// Hack: push animated components to front
	if ( comp->IsA< CAnimatedComponent > () )
	{
		Uint32 pos;
		for ( pos = 0; pos < m_components.Size(); ++pos )
		{
			if ( !m_components[ pos ]->IsA< CAnimatedComponent > () )
			{
				break;
			}
		}

		m_components.Insert( pos, comp );
	}
	else
	{
		m_components.PushBack( comp );
	}

	// Inform component that it has been spawned
	comp->OnSpawned( spawnInfo );

	// When adding new component to initialized entity we need to initialize the component
	if ( IsInitialized() )
	{
		comp->PerformInitialization();
	}

	// Attach to world if entity is attached
	if ( IsAttached() )
	{
		CWorld* world = GetLayer()->GetWorld();
		comp->PerformAttachment( world );
	}

	// Update transform and bounds
	RecursiveImmediateTransformUpdate( comp );

	SendNotifyComponentAdded( comp );

	// Return created component
	return comp;
}

void CEntity::DestroyComponent( CComponent* component )
{
	ASSERT( component );
	ASSERT( component->GetEntity() == this );

	// Do not destroy already destroyed components
	if ( !component->IsDestroyed() )
	{
		// Mark entity as destroyed
		component->m_objectFlags |= NF_Destroyed;

		// Remove all attachments
		component->BreakAllAttachments();

		// Detach component
		if ( IsAttached() || component->IsAttached() )
		{
			CWorld* world = GetLayer()->GetWorld();

			// Do not tick this component anymore
			world->GetTickManager()->Remove( component );

			// Detach component
			component->PerformDetachment( world );
		}
	
		// Deinitialize component
		component->PerformUninitialization();

		// Destroy component
		component->OnDestroyed();

		// Remove the object from the list of components
		m_components.Remove( component );

		SendNotifyComponentRemoved( component );

		// Discard component object
		component->Discard();
	}
}

void CEntity::DestroyAllComponents()
{
	// HACK: Remove area components first (they create some additional components which should be destroyed by them, and not by the entity)
	TDynArray< CAreaComponent* > areas;
	for ( ComponentIterator< CAreaComponent > it( this ); it; ++it )
	{
		areas.PushBack( *it );
	}

	for ( Uint32 i = 0; i < areas.Size(); ++i )
	{
		CAreaComponent* comp = areas[ i ];
		comp->Destroy();
	}

	// Remove components
	while ( m_components.Size() )
	{
		CComponent* comp = m_components[ m_components.Size() - 1 ];
		RED_FATAL_ASSERT( comp->GetParent() == this, "Component is not from this entity" );
		ASSERT( !comp->IsDestroyed() );
		comp->Destroy();
	}
}

void CEntity::OnDestroyed( CLayer* layer )
{
	RED_FATAL_ASSERT( !IsDestroyed(), "Trying to call OnDestroyed twice on the same entity '%ls'", GetName().AsChar() );
	RED_FATAL_ASSERT( layer != nullptr, "Trying to call OnDestroyed on entity %ls'  that does not come from an layer", GetName().AsChar() );
	RED_FATAL_ASSERT( layer == GetLayer(), "Trying to call OnDestroyed on entity %ls' with invalid layer", GetName().AsChar() );
	RED_FATAL_ASSERT( !layer->IsAttached() || layer->GetWorld() != nullptr, "Trying to call OnDestroyed on entity '%ls' within a layer that is not part of the world", GetName().AsChar() );
	//RED_FATAL_ASSERT( !IsAttached(), "Destroying an entity '%ls' that is still attached", GetName().AsChar() );
	//RED_FATAL_ASSERT( !IsInitialized(), "Destroying an entity '%ls' that is still initialized", GetName().AsChar() );
	//RED_FATAL_ASSERT( !IsInitializing(), "Destroying an entity '%ls' while it's being initialized", GetName().AsChar() );
	//RED_FATAL_ASSERT( !IsUninitializing(), "Destroying an entity '%ls' while it's being uninitialized", GetName().AsChar() );
	RED_FATAL_ASSERT( !IsAttaching(), "Destroying an entity '%ls' while it's being attached", GetName().AsChar() );
	RED_FATAL_ASSERT( !IsDetaching(), "Destroying an entity '%ls' while it's being deattached", GetName().AsChar() );

	// Remove entity from selection system
#if !defined(RED_FINAL_BUILD) && defined(RED_PLATFORM_WINPC)
	if ( IsSelected() )
	{
		CWorld* world = layer->GetWorld();
		world->GetSelectionManager()->Deselect( this );
	}
#endif

	// set the destroyed flag
	SetFlag( NF_Destroyed );
}

void CEntity::OnLayerSavedInEditor()
{
#ifndef NO_EDITOR
	RED_VERIFY( UpdateStreamedComponentDataBuffers(), TXT("Faild to update streamed component data buffers") );
	if ( GetEntityTemplate() == nullptr )
	{
		UpdateStreamingDistance();
	}
#endif
}

void CEntity::OnPostLoad()
{
	TBaseClass::OnPostLoad();

#ifndef RED_FINAL_BUILD
	// When loading a non-streamed entity make sure there is no streamed data to be found
	if ( !ShouldBeStreamed() )
	{
		m_streamingDataBuffer.Clear();
	}
#endif

	// Call OnPostLoad for components loaded from the template
	if ( m_template ) 
	{
		for ( Uint32 i = 0; i < m_components.Size(); ++i )
		{
			m_components[i]->OnPostLoad();
		}
	}

	//HACK climb tag transform. to be removed after resave
	if ( !IsCooked() )
	{
		if ( m_tags.HasTag( CNAME( n ) ) ) 
		{ 
			m_tags.SubtractTag( CNAME( n ) ); 
			m_tags.AddTag( CNAME( no_climb ) ); 
		}
	}

	// resave streaming buffer
/*	{
		DependencySavingContext context(this);
		//context.m_hashPaths = true;
		ResaveStreamingBuffer( context );
	}*/
}

void CEntity::OnPostInstanced()
{
	TBaseClass::OnPostInstanced();

	for ( CComponent* component : m_components )
	{
		component->OnPostInstanced();
	}
}

void CEntity::OnIncludesFinished()
{
	FixRootAnimatedComponentPosition();
}

void CEntity::OnPasted( CLayer* layer )
{
	// Recreate GUID for entity
	SetGUID( CGUID::Create() );

	// Recreate GUID for components
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CComponent* component = m_components[i];
		if ( component )
		{
			// Create new GUID for entity when pasted
			component->SetGUID( CGUID::Create() );
		}
	}
}

void CEntity::DestroyEntityInternals()
{
	// Destroy script state
	KillCurrentState();

	// Kill all thread
	KillThread();

	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		static CName functionName( TXT("OnDestroyed") );
		CallEvent( functionName );
	}
}

void CEntity::Initialize()
{
	PC_SCOPE_PIX( InitializeEntity );

	ASSERT( !IsInitialized() );
	ASSERT( !IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );
	ASSERT( !IsDestroyed() );

	// Make sure entity transform is up to date
	ForceUpdateTransformNodeAndCommitChanges();

	// Internal initialization
	//SetFlag( NF_Initializing );
	OnInitialized();
	RED_ASSERT( IsInitialized(), TXT("OnInitialized not propagated correctly in entity '%ls'"), GetName().AsChar() );
	//RED_ASSERT( IsInitializing(), TXT("Flags broken by OnInitialized in entity '%ls'"), GetName().AsChar() );
	//ClearFlag( NF_Initializing );

	{
		PC_SCOPE_PIX( InitializeComponentsFirst );
		// Some components needs to be initialized first
		for ( Uint32 i = 0; i < m_components.Size(); ++i )
		{
			CComponent* component = m_components[ i ];
			if ( component->ShouldInitializeBeforeOtherComponents() )
			{
				component->PerformInitialization();
			}
		}
	}

	{
		PC_SCOPE_PIX( InitializeComponentsSecond );
		// Propagate to components
	#ifdef DEBUG_COMPONENT_ISSUES
		Uint32 originalComponentCount = m_components.Size();
	#endif
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];
			if ( !component )
				continue;

			// skip components that were already initialized
			if ( component->ShouldInitializeBeforeOtherComponents() )
				continue;

			// initialize component
			component->PerformInitialization();

			// components created ?
	#ifdef DEBUG_COMPONENT_ISSUES
			if ( originalComponentCount != m_components.Size() )
			{
				RED_ASSERT( originalComponentCount == m_components.Size(), TXT("Initialization of component '%ls' changed the component count (%d->%d) in entity '%ls'"), 
					component->GetFriendlyName().AsChar(), 
					originalComponentCount, m_components.Size(),
					GetFriendlyName().AsChar() );


				originalComponentCount = m_components.Size();
			}
	#endif
		}
	}
}

void CEntity::Uninitialize()
{
	ASSERT( IsInitialized() );
	ASSERT( !IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );

	// Propagate to components
#ifdef DEBUG_COMPONENT_ISSUES
	Uint32 originalComponentCount = m_components.Size();
#endif
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		CComponent* component = m_components[i];
		if ( !component )
			continue;

		// skip components that were already initialized
		if ( component->ShouldInitializeBeforeOtherComponents() )
			continue;

		// uninitialize component
		component->PerformUninitialization();

		// components created ?
#ifdef DEBUG_COMPONENT_ISSUES
		if ( originalComponentCount != m_components.Size() )
		{
			RED_ASSERT( originalComponentCount == m_components.Size(), TXT("Uninitialization of component '%ls' changed the component count (%d->%d) in entity '%ls'"), 
				component->GetFriendlyName().AsChar(), 
				originalComponentCount, m_components.Size(),
				GetFriendlyName().AsChar() );


			originalComponentCount = m_components.Size();
		}
#endif
	}

	// Uninitialize components that should go last (CAppearanceComponent)
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		CComponent* component = m_components[i];
		if ( component && component->ShouldInitializeBeforeOtherComponents() )
		{
			component->PerformUninitialization();
		}
	}

	// Internal event
	//SetFlag( NF_Uninitializing );
	OnUninitialized();
	RED_ASSERT( !IsInitialized(), TXT("OnUninitialized not propagated correctly in entity '%ls'"), GetName().AsChar() );
	//RED_ASSERT( IsUninitializing(), TXT("Flags broken by OnUninitialized in entity '%ls'"), GetName().AsChar() );
	//ClearFlag( NF_Uninitializing );
}

void CEntity::AttachToWorld( CWorld* world )
{
	PC_SCOPE( AttachEntityToWorld );

	ASSERT( IsInitialized() );
	ASSERT( !IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );
	ASSERT( !IsDestroyed() );

	// Create the visibility query for the whole entity
	if ( GRender && world->GetRenderSceneEx() && !CheckStaticFlag( ESF_NoVisibilityQuery ) )
	{
		m_visibilityQuery = world->GetRenderSceneEx()->CreateVisibilityQuery();
	}

	// Inform entity
	m_objectFlags |= NF_Attaching;
	OnAttached( world );
	ASSERT( IsAttached() );
	ASSERT( IsAttaching() );
	m_objectFlags &= ~NF_Attaching;

	// Extra checks - please do not ignore them
#ifdef DEBUG_COMPONENT_ISSUES
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CComponent* component = m_components[i];
		if ( component != nullptr )
		{			
			RED_ASSERT( component->IsInitialized(), TXT("DO NOT IGNORE: Component '%ls' is not initialized and the AttachToWorld() was called for entity '%ls'."), 
				component->GetFriendlyName().AsChar(), GetFriendlyName().AsChar() );

			RED_ASSERT( !component->IsAttached(), TXT("DO NOT IGNORE: Component '%ls' is already attached and the AttachToWorld() was called for entity '%ls'"), 
				component->GetFriendlyName().AsChar(), GetFriendlyName().AsChar() );
		}
	}
#endif

#ifdef DEBUG_COMPONENT_ISSUES
	Uint32 originalComponentCount = m_components.Size();
#endif
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CComponent* component = m_components[i];
		if ( component != nullptr )
		{
			// update localToWorld for non-streamable drawable components before creating render proxies
			if ( CDrawableComponent* drawableComponent = Cast< CDrawableComponent >( component ) )
			{
#ifndef NO_EDITOR
				if( drawableComponent->UseWithSimplygonOnly() )
				{
					drawableComponent->Uninitialize();
					continue;
				}
#endif

				component->UpdateLocalToWorld();
			}			
			component->PerformAttachment( world );

			// components created ?
#ifdef DEBUG_COMPONENT_ISSUES
			if ( originalComponentCount != m_components.Size() )
			{
				RED_ASSERT( originalComponentCount == m_components.Size(), TXT("Attaching component '%ls' changed the component count (%d->%d) in entity '%ls'"), 
					component->GetFriendlyName().AsChar(), 
					originalComponentCount, m_components.Size(),
					GetFriendlyName().AsChar() );

				originalComponentCount = m_components.Size();
			}
#endif
		}
	}

	// extra checks - make sure that all components are initialized
#ifdef DEBUG_COMPONENT_ISSUES
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CComponent* component = m_components[i];
		if ( component != nullptr )
		{
			if ( CDrawableComponent* drawableComponent = Cast< CDrawableComponent >( component ) )
			{
				if( drawableComponent->UseWithSimplygonOnly() )
				{
					RED_ASSERT( !component->IsAttached(), TXT("Component '%ls' is for simplygon use only and should not be attached in entity '%ls'"), component->GetFriendlyName().AsChar(), GetFriendlyName().AsChar() );
					continue;
				}
			}

			RED_ASSERT( component->IsAttached(), TXT("Component '%ls' was not attached properly in Attach() in entity '%ls'"), component->GetFriendlyName().AsChar(), GetFriendlyName().AsChar() );
		}
	}
#endif

	// Register to the world streaming
	if ( ShouldBeStreamed() && world->GetStreamingSectorData() )
	{
		UpdateLocalToWorld();
		RegisterInStreamingGrid( world );
	}

	// Attach virtual NULL component, causing OnAttachFinished call
	NotifyComponentsAttached();

	// Inform listeners
	EDITOR_DISPATCH_EVENT( CNAME( Attached ), CreateEventData( this ) );
}

void CEntity::DetachFromWorld( CWorld* world )
{
	ASSERT( IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );

	RED_FATAL_ASSERT( CheckDynamicFlag( EDF_DuringUpdateTransform ) == false, "Can't detach entity from world when it's being updated by Update Transform Manager." );

	// Unregister from the world streaming
	UnregisterFromStreamingGrid( world );

	// Stop all internal threads
	KillThread();

	// Remove entity from tick manager
	world->GetTickManager()->RemoveEntity( this );

	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		static CName functionName( TXT("OnDetaching") );
		CallEvent( functionName );
	}

	// Stop all timers
	RemoveTimers();

	// Stop all effects
	DestroyAllEffects();

	// Propagate to components
#ifdef DEBUG_COMPONENT_ISSUES
	Uint32 originalComponentCount = m_components.Size();
#endif
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CComponent* component = m_components[i];
		if ( component )
		{
			// Kill timers
			world->GetTickManager()->Remove( component );

			// Detach
			component->PerformDetachment( world );

			// components created ?
#ifdef DEBUG_COMPONENT_ISSUES
			if ( originalComponentCount != m_components.Size() )
			{
				RED_ASSERT( originalComponentCount == m_components.Size(), TXT("Detaching component '%ls' changed the component count (%d->%d) in entity '%ls'"), 
					component->GetFriendlyName().AsChar(), 
					originalComponentCount, m_components.Size(),
					GetFriendlyName().AsChar() );


				originalComponentCount = m_components.Size();
			}
#endif
		}
	}

	// Break the parent transform attachment on the entity itself
	// This is necessary since attachments work for entities
	CHardAttachment* transformParent = GetTransformParent();
	if ( transformParent )
	{
		transformParent->Break();
		ASSERT( !GetTransformParent() );
	}

	// Remove from tag manager
	world->GetTagManager()->RemoveNode( this, m_tags );

	// Inform entity
	ASSERT( IsAttached() );
	m_objectFlags |= NF_Detaching;
	OnDetached( world );
	ASSERT( !IsAttached() );
	m_objectFlags &= ~NF_Detaching;

	// Release visibility query
	if ( m_visibilityQuery )
	{
		world->GetRenderSceneEx()->ReleaseVisibilityQuery( m_visibilityQuery );
		m_visibilityQuery = 0;
	}

	// Unlink from handle system
	DiscardHandles();

	// Inform listeners
	EDITOR_DISPATCH_EVENT( CNAME( Detached ), CreateEventData( this ) );
}

void CEntity::OnCreatedAsync( const EntitySpawnInfo& info )
{
	// We have to set m_localToWorld and m_transform before ApplyInitialAppearance will be set (see ApplyInitialAppearance code)

	// Set initial entity transform
	m_transform.Init( info.m_spawnPosition, info.m_spawnRotation, info.m_spawnScale );

	// Force only localToWorld transform update
	HACK_UpdateLocalToWorld();
}

void CEntity::PostComponentsInitializedAsync()
{
	OnPostComponentsInitializedAsync();

	for ( CComponent* c : m_components )
	{
		c->OnPostComponentInitializedAsync();
	}
}

void CEntity::OnPostComponentsInitializedAsync()
{
	
}

void CEntity::OnCreated( CLayer* layer, const EntitySpawnInfo& info )
{
	PC_SCOPE( EntityOnCreated );

	// Set initial entity params
	m_transform.Init( info.m_spawnPosition, info.m_spawnRotation, info.m_spawnScale );

	// For every spawned entity during the game create a unique GUID
	if ( !info.m_guid.IsZero() )
	{
		SetGUID( info.m_guid );
	}
	else
	{
		SetGUID( CGUID::Create() );
	}

	// Override tags
	if ( !info.m_tags.Empty() )
	{
		m_tags.AddTags( info.m_tags );
	}

	// Set entity flags
	COMPILE_ASSERT( sizeof( m_entityFlags ) == sizeof( info.m_entityFlags ) );
	m_entityFlags = info.m_entityFlags;

	// Force transform update
	ForceUpdateTransformNodeAndCommitChanges();

	// Force bounds update
	ForceUpdateBoundsNode();
}

void CEntity::OnRestoredFromPoolAsync( const EntitySpawnInfo& spawnInfo )
{
	const CNode* nodeTemplate = Cast< CNode >( spawnInfo.m_template->GetTemplateInstance() );
	ASSERT( nodeTemplate );

	if ( !spawnInfo.m_tags.Empty() )
	{
		m_tags = nodeTemplate->GetTags();
		m_tags.AddTags( spawnInfo.m_tags );
	}
	else if ( m_tags != nodeTemplate->GetTags() )
	{
		m_tags = nodeTemplate->GetTags();
	}

	// We have to set m_localToWorld and m_transform before ApplyInitialAppearance will be set (see ApplyInitialAppearance code)

	// Set initial entity transform
	m_transform.Init( spawnInfo.m_spawnPosition, spawnInfo.m_spawnRotation, spawnInfo.m_spawnScale );

	// Force only localToWorld transform update
	HACK_UpdateLocalToWorld();
}

void CEntity::OnRestoredFromPool( CLayer* layer, const EntitySpawnInfo& spawnInfo )
{
	SetRawPlacement( &spawnInfo.m_spawnPosition, &spawnInfo.m_spawnRotation, NULL );
	ForceUpdateTransformNodeAndCommitChanges();
}

void CEntity::OnLoaded()
{
	for ( CComponent* component : m_components )
	{
		component->OnEntityLoaded();
	}
}

void CEntity::OnInitialized()
{
	//ASSERT( IsInitializing() );

    // Also its important to have root animated component that has no parent attachments and is first in component list
    FixRootAnimatedComponentPosition();

	// Mark as initialized
	SetFlag( NF_Initialized );
}

void CEntity::OnUninitialized()
{
	ASSERT( IsInitialized() );
	//ASSERT( IsUninitializing() );

	ClearFlag( NF_Initialized );
}

void CEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	// Mark as attached
	ASSERT( IsAttaching() );
	m_objectFlags |= NF_Attached;

	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_LodInfo );
	world->GetEditorFragmentsFilter().RegisterEditorFragment( this, SHOW_EntityVisibility );	

	// Auto play effect on attach
	if ( m_autoPlayEffectName != CName::NONE )
	{
		PlayEffect( m_autoPlayEffectName );
	}

	// Apply coloring
	ApplyMeshComponentColoring();

	world->RegisterEntity( this );
}

void CEntity::OnDetached( CWorld* world )
{
	world->UnregisterEntity( this );

	TBaseClass::OnDetached( world );

	ASSERT( IsAttached() );
	ASSERT( IsDetaching() );

	DestroyAllEffects();

	// If not unscheduled, will cause crashes
	if ( HasFlag( NF_ScheduledUpdateTransform ) || HasFlag( NF_MarkUpdateTransform ) )
	{
		ClearFlag( NF_ScheduledUpdateTransform );
		ClearFlag( NF_MarkUpdateTransform );

		world->GetUpdateTransformManager().UnscheduleEntity( this );
	}

	// Mark as detached
	m_objectFlags &= ~NF_Attached;
	m_objectFlags &= ~NF_PostAttachSpawnCalled;
	
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_EntityVisibility );
	world->GetEditorFragmentsFilter().UnregisterEditorFragment( this, SHOW_LodInfo );
}

void CEntity::OnAttachFinishedEditor( CWorld* world )
{
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		m_components[i]->OnAttachFinishedEditor( world );	
	}
}

void CEntity::OnAttachFinished( CWorld* world )
{
	PC_SCOPE_PIX( OnAttachFinished );
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		m_components[i]->OnAttachFinished( world );	
	}
}

void CEntity::NotifyComponentsAttached()
{
	if ( !HasFlag( NF_PostAttachSpawnCalled ) )
	{
		PC_SCOPE_PIX( NotifyComponentsAttached );

		// All components has been attached
		SetFlag( NF_PostAttachSpawnCalled );

		// Get the world we are attached to
		CWorld* world = GetLayer()->GetWorld();
		if ( world )
		{
			// Inform C++ side of the crap	
			if ( !GGame->IsActive() )
			{
				OnAttachFinishedEditor( world );

				// Call the post attach spawn event
				{
					// Pass to script
					SEntitySpawnData spawnData;
					spawnData.m_restored = CheckDynamicFlag( EDF_RestoredFromLayerStorage );
					CallEvent( CNAME( OnSpawnedEditor ), spawnData );
				}
			}
			// This crap is needed in game only
			else if ( GGame->IsActive() )
			{
				OnAttachFinished( world );

				// Call the post attach spawn event
				{
					PC_SCOPE_PIX( ScriptCall_OnSpawned );
					// Pass to script
					SEntitySpawnData spawnData;
					spawnData.m_restored = CheckDynamicFlag( EDF_RestoredFromLayerStorage );
					CallEvent( CNAME( OnSpawned ), spawnData );
				}
			}

			// Register in tag manager
			world->GetTagManager()->AddNode( this, m_tags );
		}
	}
}

void CEntity::SetRawPlacement( const Vector* position, const EulerAngles* rotation, const Vector* scale )
{
	// Change position
	if ( position )
	{
		m_transform.SetPosition( *position );
	}

	// Change rotation
	if ( rotation )
	{
		m_transform.SetRotation( *rotation );
	}

	// Change scale
	if ( scale )
	{
		m_transform.SetScale( *scale );
	}

	// Schedule transform update
	ScheduleUpdateTransformNode();
}

void CEntity::SetRawPlacementNoScale( const Matrix& newPlacement )
{
	const Vector& pos = newPlacement.GetTranslationRef();
	const EulerAngles rot = newPlacement.ToEulerAngles();
	SetRawPlacement( &pos, &rot, nullptr );
}

void CEntity::SetHideInGame( Bool hideInGame, Bool immediate /*= false */, EHideReason )
{
	// Check previous state
	const Bool wasHidden = IsHiddenInGame();

	// Apply flag
	if ( hideInGame )
	{
		m_objectFlags |= NF_HideInGame;
	}
	else
	{
		m_objectFlags &= ~NF_HideInGame;
	}

	// Refresh visibility if flag changed
	if ( IsAttached() && ( wasHidden != IsHiddenInGame() ) )
	{
		RefreshChildrenVisibility( immediate );
	}
}

Uint32 CEntity::GetHideReason() const
{
	// Entity doesn't know about hide reasons, so just return all reasons if hidden

	return IsHiddenInGame() ? HR_All : ( Uint32 ) 0;
}

void CEntity::RefreshChildrenVisibility( NodeProcessingContext& context, Bool force )
{
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		CComponent* component = *it;
		if ( context.Find( component ) == context.End() )
		{
			context.Insert( component );

			component->RefreshVisibilityFlag( context, force );
		}
	}
	for ( TList< IAttachment* >::iterator attachmentIter = m_childAttachments.Begin(), attachmentEnd = m_childAttachments.End();
		attachmentIter != m_childAttachments.End();
		++attachmentIter )
	{
		CNode* child = ( *attachmentIter)->GetChild();

		if ( context.Find( child ) == context.End() )
		{
			context.Insert( child );

			child->RefreshNodeVisibilityFlag( context, force );
		}
	}
}

void CEntity::RefreshChildrenVisibility( Bool force )
{
	NodeProcessingContext context;
	context.Insert( this );
	RefreshChildrenVisibility( context, force );
}

void CEntity::RefreshNodeVisibilityFlag( NodeProcessingContext& context, Bool force )
{
	RefreshChildrenVisibility( context, force );
}

void CEntity::SuspendRendering( Bool hideInGame )
{
	// Check previous state
	const Bool wasHidden = IsRenderingSuspended();

	// Apply flag
	if ( hideInGame )
	{
		m_objectFlags |= NF_SuspendRendering;
	}
	else
	{
		m_objectFlags &= ~NF_SuspendRendering;
	}

	// Refresh visibility of all attached components
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		CComponent* comp = m_components[i];
		comp->ToggleVisibility( !hideInGame );
	}
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",off)
#endif

void CEntity::UpdateTransformEntity( SUpdateTransformContext& context, Bool parentScheduledUpdateTransform )
{
	PC_SCOPE( CEntity_OnUpdateTransform );

	const Bool scheduledUpdateTransform = parentScheduledUpdateTransform || HasFlag( NF_ScheduledUpdateTransform );

	UpdateTransformNode( context, scheduledUpdateTransform );

	ClearFlag( NF_MarkUpdateTransform );

	RED_ASSERT( !HasFlag( NF_ScheduledUpdateTransform ) );

	for ( CComponent* c : m_components )
	{
		if ( c && c->IsNotDestroyed() && ( ( scheduledUpdateTransform && c->UsesAutoUpdateTransform() ) || ( c->HasFlag( NF_MarkUpdateTransform ) || c->HasFlag( NF_ScheduledUpdateTransform ) ) ) && c->IsRootComponent() ) // c->IsRootComponent() - Because of item atachment logic :(
		{
			c->ClearFlag( NF_MarkUpdateTransform );

			c->UpdateTransformNode( context, scheduledUpdateTransform );

			RED_ASSERT( !c->HasFlag( NF_ScheduledUpdateTransform ) );
		}
	}

#ifdef RED_ASSERTS_ENABLED
	/*for ( CComponent* c : m_components )
	{
		// Filter out items
		const CEntity* parentEntity = c && c->GetTransformParent() ? c->GetTransformParent()->GetParent()->FindParent< CEntity>() : nullptr;
		if ( parentEntity == this )
		{
			RED_ASSERT( !c->HasFlag( NF_ScheduledUpdateTransform ) );
			RED_ASSERT( !c->HasFlag( NF_MarkUpdateTransform ) );
		}
	}*/
#endif

	OnUpdateTransformEntity();
}

#ifdef DEBUG_TRANS_MGR
#pragma optimize("",on)
#endif

void CEntity::OnUpdateTransformEntity()
{
	// Inform world streaming about the new position
	if ( m_streamingProxy != nullptr )
	{
		CLayer* layer = GetLayer();
		if ( layer )
		{
			CWorld* world = layer->GetWorld();
			if ( world )
			{
				UpdateInStreamingGrid( world );
			}
		}
	}

#ifndef NO_EDITOR
	if( GetPartOfAGroup() == true )
	{
		CEntityGroup* group = GetContainingGroup();
		if( group->IsLocked() == false )
		{
			group->OnPostUpdateTransform();
		}
	}
#endif	// NO_EDITOR
}

void CEntity::OnTimer( const CName name, Uint32 id, Float timeDelta )
{
	// Entity is discarded - no timer should be called
	if ( HasFlag( OF_Discarded | OF_Finalized ) )
	{
		HALT( "Calling timer '%ls' on discarded entity '%ls'.", name.AsChar(), GetFriendlyName().AsChar() );
		return;
	}

	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		PC_SCOPE_PIX( EntityCallFuncOnTimer );

#ifndef NO_DEBUG_PAGES
		// Measure native function call
		CTimeCounter timer;
#endif

		CallFunction( this, name, timeDelta, id );

#ifndef NO_DEBUG_PAGES
		// Was it slow ?
		const Float timeElapsed = timer.GetTimePeriod();
		if ( timeElapsed > 0.1f )
		{
			GScreenLog->PerfWarning( timeElapsed, TXT("SCRIPT"), TXT("Slow call OnTimer '%ls'"), name.AsString().AsChar() );
		}
#endif
	}
}

void CEntity::OnTimer( const CName name, Uint32 id, GameTime timeDelta )
{
	PC_SCOPE( CEntity_OnTimer2 );

	// Entity is discarded - no timer should be called
	if ( HasFlag( OF_Discarded | OF_Finalized ) )
	{
		HALT( "Calling timer '%ls' on discarded entity '%ls'.", name.AsChar(), GetFriendlyName().AsChar() );
		return;
	}

	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		PC_SCOPE_PIX( EntityCallFuncOnTimer );

#ifndef NO_DEBUG_PAGES
		// Measure native function call
		CTimeCounter timer;
#endif

		CallFunction( this, name, timeDelta, id );

#ifndef NO_DEBUG_PAGES
		// Was it slow ?
		const Float timeElapsed = timer.GetTimePeriod();
		{
			GScreenLog->PerfWarning( timeElapsed, TXT("SCRIPT"), TXT("Slow call OnTimer (game-time) '%ls'"), name.AsString().AsChar() );
		}
#endif
	}
}

void CEntity::OnTick( Float timeDelta )
{
}

void CEntity::OnAreaEnter( CTriggerAreaComponent* area, CComponent* activator )
{
	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		static CName functionName( TXT("OnAreaEnter") );
		THandle< CTriggerAreaComponent > areaHandle( area );
		THandle< CComponent > activatorHandle( activator );
		CallEvent( functionName, areaHandle, activatorHandle );
	}
}

void CEntity::OnAreaExit( CTriggerAreaComponent* area, CComponent* activator )
{
	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		static CName functionName( TXT("OnAreaExit") );
		THandle< CTriggerAreaComponent > areaHandle( area );
		THandle< CComponent > activatorHandle( activator );
		CallEvent( functionName, areaHandle, activatorHandle );
	}
}

void CEntity::OnAreaActivated( CTriggerAreaComponent* area, Bool activated )
{
	// Pass to script
	if ( GGame && GGame->IsActive() )
	{
		static CName functionName( TXT("OnAreaActivated") );
		THandle< CTriggerAreaComponent > areaHandle( area );
		CallEvent( functionName, areaHandle, activated );
	}
}

void CEntity::OnScriptPreCaptureSnapshot()
{
	// Pass to script
	if ( IsAttached() && GGame->IsActive() )
	{
		static CName eventName( TXT("OnScriptReloading") );
		CallEvent( eventName );
	}

	TBaseClass::OnScriptPreCaptureSnapshot();
}

void CEntity::OnScriptReloaded()
{
	TBaseClass::OnScriptReloaded();

	// Pass to script
	if ( IsAttached() && GGame->IsActive() )
	{
		static CName eventName( TXT("OnScriptReloaded") );
		CallEvent( eventName );
	}
}

void CEntity::OnMoveSpeedChanged( CAnimatedComponent* component, Float newSpeed )
{
	// Pass to script
	{
		static CName eventName( TXT("OnSpeedChanged") );
		THandle< CComponent > componentHandle( component );

		CallEvent( eventName, componentHandle, newSpeed );
	}
}

Uint32 CEntity::AddTimer( const CName& name, Float period, Bool repeats /*=false*/, Bool scatter/*=false*/, ETickGroup group /*=TICK_Main*/, Bool savable /*=false*/, Bool overrideExisting )
{
	PC_SCOPE( CEntity_AddTimer );

	RED_ASSERT( IsAttached(), TXT( "Trying to add timer '%ls' to not attached entity. Probably AddTimer is being called from the scripts."), name.AsChar() );

	if ( ! IsDetaching() && IsAttached() ) // If AddTimer is called from scripts then sometimes CEntity is already detached
	{
		// Add entity timer
		return GetLayer()->GetWorld()->GetTickManager()->GetTimerManager().AddTimer( this, group, name, period, repeats, scatter, savable, overrideExisting );
	}

	return 0;
}

Uint32 CEntity::AddTimer( const CName& name, GameTime period, Bool repeats /*=false*/, Bool scatter/*=false*/, ETickGroup group /*=TICK_Main*/, Bool savable /*=false*/, Bool overrideExisting )
{
	PC_SCOPE( CEntity_AddTimer2 );

	RED_ASSERT( IsAttached(), TXT( "Trying to add timer '%ls' to not attached entity. Probably AddTimer is being called from the scripts."), name.AsChar() );

	if ( ! IsDetaching() && IsAttached() ) // If AddTimer is called from scripts then sometimes CEntity is already detached
	{
		// Add entity timer
		return GetLayer()->GetWorld()->GetTickManager()->GetTimerManager().AddTimer( this, group, name, period, repeats, scatter, savable, overrideExisting );
	}

	return 0;
}

void CEntity::RemoveTimer( const CName& name, ETickGroup group /*=TICK_Main*/ )
{
	PC_SCOPE( CEntity_RemoveTimer );

	if ( IsAttached() )
	{
		// Remove all entity timers
		GetLayer()->GetWorld()->GetTickManager()->GetTimerManager().RemoveTimer( this, group, name );
	}
}

void CEntity::RemoveTimer( Uint32 id, ETickGroup group /*=TICK_Main*/ )
{
	PC_SCOPE( CEntity_RemoveTimer2 );

	if ( IsAttached() )
	{
		// Remove all entity timers
		GetLayer()->GetWorld()->GetTickManager()->GetTimerManager().RemoveTimer( this, group, id );
	}
}

void CEntity::RemoveTimers()
{
	if ( IsAttached() )
	{
		// Remove all entity timers
		GetLayer()->GetWorld()->GetTickManager()->GetTimerManager().RemoveTimers( this );
	}
}

CSoundEmitterComponent* CEntity::GetSoundEmitterComponent( Bool createIfDoesntExist )
{
	PC_SCOPE( CEntity_GetSoundEmitterComponent );

	// Find component with given name
	for ( Int32 i=m_components.Size()-1; i>=0; i-- )
	{
		CComponent* cur = m_components[ i ];
		if ( CSoundEmitterComponent* soundEmitterComponent = Cast< CSoundEmitterComponent >( cur ) )
		{
			if( i != m_components.Size()-1 )
			{
				m_components.RemoveAtFast( i );
				m_components.PushBack( soundEmitterComponent );
			}
			return soundEmitterComponent;
		}
	}

	if( !createIfDoesntExist ) return 0;

	// Do not create sound components for entities that are not attached yet
	if ( !IsAttached() || !IsInitialized() )
		return nullptr;

	if( IsDestroyed() ) return 0;

	CSoundEmitterComponent* component = 0;
	if( m_template )
	{
		for( auto i = m_template->GetIncludes().Begin(), iEnd = m_template->GetIncludes().End(); i != iEnd && !component ; ++i )
		{
			CEntityTemplate* entityTemplate = i->Get();
			CEntity* entity = entityTemplate ? entityTemplate->GetEntityObject() : nullptr;
			if ( !entity )
			{
				continue;				// It actually crashed in-game on entityTemplate being NULL
			}
			ComponentIterator< CSoundEmitterComponent > it( entity );
			while ( it )
			{
				component = Cast< CSoundEmitterComponent >( (*it)->Clone( this ) );
				if ( component )
				{
					m_components.PushBack( component );
					break;
				}
				++it;
			}
		}
	}
	if( !component )
	{
		SComponentSpawnInfo spawnInfo;
		spawnInfo.m_name = TXT("CSoundEmitterComponent");
		component = SafeCast< CSoundEmitterComponent >( CreateComponent( ClassID< CSoundEmitterComponent >(), spawnInfo ) );
	}

	return component;
}

CComponent* CEntity::FindComponent( const String& name, Bool caseSensitive /*= true*/ ) const
{
	PC_SCOPE( CEntity_FindComponent );

	if ( !caseSensitive )
	{
		const String & lowerName = name.ToLower();

		// Find component with given name
		for ( Uint32 i=0; i<m_components.Size(); i++ )
		{
			if ( m_components[i]->GetName().ToLower() == lowerName )
			{
				return m_components[i];
			}
		}
	}
	else
	{
		// Find component with given name
		for ( Uint32 i=0; i<m_components.Size(); i++ )
		{
			if ( m_components[i]->GetName() == name )
			{
				return m_components[i];
			}
		}
	}

	// Not found
	return NULL;
}

CComponent* CEntity::FindComponent( CName name ) const
{
	PC_SCOPE( CEntity_FindComponent2 );

	// Find component with given name
	const String charName = name.AsString();
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		if ( m_components[i]->GetName() == charName )
		{
			return m_components[i];
		}
	}

	// Not found
	return NULL;
}

CComponent* CEntity::FindComponent( CName componentName, const CName& componentClass ) const
{
	PC_SCOPE( CEntity_FindComponent3 );

	const String charName = componentName.AsString();
	for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
	{
		if ( (*it)->GetName() == charName && (*it)->GetClass()->GetName() == componentClass )
		{
			return *it;
		}
	}
	return NULL;
}

CComponent* CEntity::FindComponent( const CGUID& guid ) const
{
	PC_SCOPE( CEntity_FindComponent4 );

	TDynArray<CComponent*>::const_iterator	cmpCurr = m_components.Begin(),
											cmpLast = m_components.End();
	for ( ; cmpCurr != cmpLast; ++cmpCurr )
		if ( (**cmpCurr).GetGUID() == guid )
			return *cmpCurr;

	return NULL;
}

Box CEntity::CalcBoundingBox() const
{
	PC_SCOPE( CEntity_CalcBoundingBox );

	Box box;
	box.Clear();

	Bool hasDrawableComponent = false;

	// Add bounding boxed of drawable components
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CDrawableComponent* dc = Cast< CDrawableComponent >( m_components[i] );
		if ( dc && dc->IsVisible() && dc->IsAttached() )
		{
			hasDrawableComponent = true;
			box.AddBox( dc->GetBoundingBox() );
		}
	}

	// For non-drawable en tities return small bounding box
	if( ! hasDrawableComponent )
	{
		return Box( GetWorldPosition(), 0.1f );
	}

	return box;
}

CObject* CEntity::GetTemplate() const
{
	return m_template.Get();
}

void CEntity::DetachTemplate( Bool markComponentsAsNonIncluded /* = false */ )
{
	for ( auto* ptr : m_components ) 
	{
		ptr->OnDetachFromEntityTemplate();
		if ( markComponentsAsNonIncluded )
		{
			ptr->ClearFlag( NF_IncludedFromTemplate );
		}
	}
	m_template = NULL;
}

#ifndef NO_ERROR_STATE

void CEntity::SetErrorState( const String & description ) const
{
	// Not used
}

#endif // NO_ERROR_STATE

Bool CEntity::GetScriptDebuggerName( String& debugName ) const
{
	debugName = GetName();
	return true;
}

CLayer* CEntity::GetLayer() const
{
	// Entity is always in a layer
	CObject * parent = GetParent();
	return parent && parent->IsA< CLayer >() ? static_cast< CLayer* >( parent ) : nullptr;
}

String CEntity::GetFriendlyName() const
{
	PC_SCOPE( CEntity_GetFriendlyName );

	if ( GetName().Empty() && GetEntityTemplate() )
	{
		if ( GetParent() )
		{
			return GetParent()->GetFriendlyName() + TXT("::") + GetEntityTemplate()->GetDepotPath().AsChar();
		}
		else
		{
			return GetName() + TXT("::") + GetEntityTemplate()->GetDepotPath().AsChar();
		}
	}
	else if ( GetParent() )
	{
		return GetParent()->GetFriendlyName() + TXT("::") + GetName();
	}

	return GetName();
}

Bool CEntity::CanExtractComponents( const Bool isOnStaticLayer ) const
{
	return isOnStaticLayer || IsExactlyA< CEntity >();
}

void CEntity::ForceFinishAsyncResourceLoads()
{
	CMeshComponent* meshComponent;

	for ( auto it=m_components.Begin(); it != m_components.End(); ++it )
	{
		// Try mesh components
		if ( ( meshComponent = Cast< CMeshComponent >( *it ) ) != nullptr )
		{
			meshComponent->GetMeshNow();
		}
	}
}

String CEntity::GetDisplayName() const
{
#ifndef NO_EDITOR
	return m_name;
#else
	return String::EMPTY;
#endif
}

const String& CEntity::GetErrorState() const
{
	return String::EMPTY;
}

void CEntity::OnPropertyPreChange( IProperty* property )
{
	TBaseClass::OnPropertyPreChange( property );

	// Node name changed
	if ( property->GetName() == CNAME( tags ) )
	{
		if ( CLayer* layer = GetLayer() )
		{
			layer->GetWorld()->GetTagManager()->RemoveNode( this, m_tags );
		}
	}
}

void CEntity::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME( tags ) )
	{
		EDITOR_QUEUE_EVENT( CNAME( NodeTagsChanged ), CreateEventData( this ) );

		if ( CLayer* layer = GetLayer() )
		{
			// Update node in the tag system
			layer->GetWorld()->GetTagManager()->AddNode( this, m_tags );
		}
	}

	
#ifndef NO_EDITOR
	if ( property->GetName() == CNAME( entityStaticFlags ) )
	{
		StreamedFlagChanged();
	}

	if ( property->GetName() == CNAME( forceAutoHideDistance ) )
	{
		ForceAutoHideDistanceChanged();
	}
#endif
		
}

Bool CEntity::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{	
	// handle the flag conversion
	if ( propertyName == TXT("streamed") && readValue.GetType() == GetTypeName< Bool >() )
	{
		Bool value = true;
		readValue.AsType< Bool >( value );
		SetStaticFlag( ESF_Streamed, value );
		return true;
	}

#ifndef RED_FINAL_BUILD	
	// Handle streaming buffer conversion
	if ( propertyName == TXT("streamingDataBuffer0") )
	{
		readValue.AsType< TDynArray< Uint8 > >( m_oldBuffer[0] );
		return true;
	}
	if ( propertyName == TXT("streamingDataBuffer1") )
	{
		readValue.AsType< TDynArray< Uint8 > >( m_oldBuffer[1] );
		return true;
	}
	if ( propertyName == TXT("streamingDataBuffer2") )
	{
		readValue.AsType< TDynArray< Uint8 > >( m_oldBuffer[2] );
		return true;
	}
#endif

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

Int32 CEntity::GetSubObjectCount() const
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
		if ( provider )
		{
			return provider->GetBonesNum();
		}
	}
	return 0;
}

Int32 CEntity::GetSoundSubObjectCount()const
{
	return GetSubObjectCount();
}

Bool CEntity::GetSubObjectWorldMatrix( Uint32 index, Matrix& matrix ) const
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
		if ( provider )
		{
			matrix = provider->GetBoneMatrixWorldSpace( index );
			return true;
		}
		else
		{
			matrix = Matrix::IDENTITY;
		}
	}
	else
	{
		matrix = Matrix::IDENTITY;
	}
	return false;
}

Matrix CEntity::GetBoneReferenceMatrixMS( Uint32 index ) const
{
	Matrix matrix( Matrix::IDENTITY );

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac != nullptr && ac->GetSkeleton() != nullptr )
	{
		RedMatrix4x4 m = ac->GetSkeleton()->GetBoneMS( index ).ConvertToMatrix();
		matrix = reinterpret_cast< const Matrix& >( m );
	}
	return matrix;
}

void CEntity::ConvertAllStaticMeshesToMeshes()
{
	CComponent* oldComponent;
	Uint32 numComponents = m_components.Size();
	TDynArray< CComponent* > componentsToRemove;

	for ( Uint32 i = 0; i < numComponents; ++i )
	{
		oldComponent = m_components[ i ];
		if ( oldComponent->IsA< CStaticMeshComponent >() )
		{
			componentsToRemove.PushBack( oldComponent ); 

			SComponentSpawnInfo componentSpawnInfo;
			componentSpawnInfo.m_spawnPosition = oldComponent->GetPosition();
			componentSpawnInfo.m_spawnRotation = oldComponent->GetRotation();
			CMeshComponent* component = Cast< CMeshComponent > (
				CreateComponent( CMeshComponent::GetStaticClass(), componentSpawnInfo ) );
			component->SetAsCloneOf( Cast< CMeshComponent > ( oldComponent ) );
		}
	}

	numComponents = componentsToRemove.Size();
	for ( Uint32 i = 0; i < numComponents; ++i )
	{
		DestroyComponent( componentsToRemove[ i ] );
	}
}

void CEntity::ConvertAllMeshesToStatic()
{
	CComponent* oldComponent;
	Uint32 numComponents = m_components.Size();
	TDynArray< CComponent* > componentsToRemove;

	for ( Uint32 i = 0; i < numComponents; ++i )
	{
		oldComponent = m_components[ i ];
		if ( oldComponent->GetClass() == ClassID< CMeshComponent >() )
		{
			componentsToRemove.PushBack( oldComponent ); 

			SComponentSpawnInfo componentSpawnInfo;
			componentSpawnInfo.m_spawnPosition = oldComponent->GetPosition();
			componentSpawnInfo.m_spawnRotation = oldComponent->GetRotation();
			CStaticMeshComponent* component = Cast< CStaticMeshComponent > (
				CreateComponent( CStaticMeshComponent::GetStaticClass(), componentSpawnInfo ) );
			component->SetAsCloneOf( Cast< CMeshComponent > ( oldComponent ) );
		}
	}

	numComponents = componentsToRemove.Size();
	for ( Uint32 i = 0; i < numComponents; ++i )
		DestroyComponent( componentsToRemove[ i ] );
}

void CEntity::ConvertAllMeshesToRigidMeshes()
{
	CComponent* oldComponent;
	Uint32 numComponents = m_components.Size();
	TDynArray< CComponent* > componentsToRemove;

	for ( Uint32 i = 0; i < numComponents; ++i )
	{
		oldComponent = m_components[ i ];
		if ( oldComponent->GetClass() == ClassID< CMeshComponent >() )
		{
			componentsToRemove.PushBack( oldComponent ); 

			SComponentSpawnInfo componentSpawnInfo;
			componentSpawnInfo.m_spawnPosition = oldComponent->GetPosition();
			componentSpawnInfo.m_spawnRotation = oldComponent->GetRotation();
			CRigidMeshComponent* component = Cast< CRigidMeshComponent > (
				CreateComponent( CRigidMeshComponent::GetStaticClass(), componentSpawnInfo ) );
			component->SetAsCloneOf( Cast< CMeshComponent > ( oldComponent ) );
		}
	}

	numComponents = componentsToRemove.Size();
	for ( Uint32 i = 0; i < numComponents; ++i )
		DestroyComponent( componentsToRemove[ i ] );
}

void CEntity::AddComponent( CComponent* component )
{
	if ( component ) 
	{
		// Make sure component name is valid
		if ( GenerateUniqueComponentName( component ) )
		{
			LOG_ENGINE( TXT("Add component: generate component unique name") );
		}

		// Register in component list
		m_components.PushBack( component );

		// Change parent
		component->ChangeParent( this );

		// Update transform and bounds
		RecursiveImmediateTransformUpdate( component );

		// Initialize if entity is initialized
		if ( IsInitialized() )
		{
			component->PerformInitialization();
		}

		// Attach to world if entity is attached
		if ( IsAttached() )
		{
			CWorld* world = GetLayer()->GetWorld();	  
			component->PerformAttachment( world );
		}

		SendNotifyComponentAdded( component );
	}
}

void CEntity::RemoveComponent( CComponent* component, bool leaveParentAttachments )
{
	ASSERT( component );
	ASSERT( component->GetEntity() == this );

	// Do not destroy already destroyed components
	if ( !component->IsDestroyed() )
	{
		// Detach component
		if ( IsAttached() || component->IsAttached() )
		{
			CWorld* world = GetLayer()->GetWorld();

			// Do not tick this component anymore
			world->GetTickManager()->Remove( component );

			// Detach and destroy
			component->PerformDetachment( world );
		}
		else
		{
			// Inform component
			component->OnDestroyed();
		}

		{ // Break child attachment
			TList< IAttachment* > attachments = component->GetChildAttachments();
			TList< IAttachment* >::iterator
				currAtt = attachments.Begin(),
				lastAtt = attachments.End();
			for ( ; currAtt != lastAtt; ++currAtt )
			{
				(*currAtt)->Break();
			}
		}

		if ( !leaveParentAttachments )
		{
			{ // Break parent attachment
				TList< IAttachment* > attachments = component->GetParentAttachments();
				TList< IAttachment* >::iterator
					currAtt = attachments.Begin(),
					lastAtt = attachments.End();
				for ( ; currAtt != lastAtt; ++currAtt )
				{
					(*currAtt)->Break();
				}
			}
		}

		// Remove the object from the list of components
		m_components.Remove( component );
		
		// Remove the object from the list of streamed components
		m_streamingComponents.Remove( component );

		SendNotifyComponentRemoved( component );
	}
}

void CEntity::MoveComponent( CComponent* component )
{
	CEntity* otherEntity = component->GetEntity();

	// We can move components only within a single world
	ASSERT( GetLayer()->GetWorld() == otherEntity->GetLayer()->GetWorld() );

	component->BreakAllAttachments();

	// Remove from other entity's components list
	otherEntity->m_components.Remove( component );

	// Make sure component name is valid
	if ( GenerateUniqueComponentName( component ) )
	{
		LOG_ENGINE( TXT("Add component: generate component unique name") );
	}

	// Register in component list
	m_components.PushBack( component );

	// Change parent
	component->SetParent( this );
}

Bool CEntity::IsInGame() const
{
	return GGame && GGame->IsActive() && ( GetLayer() != NULL ) && ( GGame->GetActiveWorld() == GetLayer()->GetWorld() );
}

Bool CEntity::RaiseBehaviorEventForAll( const CName& eventName )
{
	Bool processed = false;
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		CAnimatedComponent* ac = *it;
		if( ac->GetBehaviorStack() )
		{
			processed |= ac->GetBehaviorStack()->GenerateBehaviorEvent( eventName );
		}
	}

	return processed;
}

Bool CEntity::RaiseBehaviorEvent( const CName& eventName )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		return ac->GetBehaviorStack()->GenerateBehaviorEvent( eventName );
	}

	return false;
}

Bool CEntity::SetBehaviorVariable( const CName varName, Float value, Bool inAllInstances )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		return ac->GetBehaviorStack()->SetBehaviorVariable( varName, value, inAllInstances );
	}
	else
	{
		return false;
	}
}

Float CEntity::GetBehaviorFloatVariable( const CName varName, Float defValue )
{
	Float var = defValue;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		var = ac->GetBehaviorStack()->GetBehaviorFloatVariable( varName, defValue );
	}
	return var;
}


Bool CEntity::RaiseBehaviorForceEventForAll( const CName& eventName )
{
	Bool processed = false;
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		CAnimatedComponent* ac = *it;
		if( ac->GetBehaviorStack() )
		{
			processed |= ac->GetBehaviorStack()->GenerateBehaviorForceEvent( eventName );
		}
	}

	return processed;
}

Bool CEntity::RaiseBehaviorForceEvent( const CName& eventName )
{
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		return ac->GetBehaviorStack()->GenerateBehaviorForceEvent( eventName );
	}

	return false;
}

#ifndef NO_EDITOR
CSkeletalAnimationSetEntry* CEntity::FindAnimation( const CName& animationName ) const
{
	for ( Uint32 i=0; i<m_components.Size(); i++ )
	{
		CAnimatedComponent* animatedComponent = Cast< CAnimatedComponent >( m_components[i] );
		if ( animatedComponent && animatedComponent->GetAnimationContainer() )
		{
			CSkeletalAnimationSetEntry* animationEntry = animatedComponent->GetAnimationContainer()->FindAnimation( animationName );
			if ( animationEntry && animationEntry->GetAnimation() )
			{
				// Return found animation
				return animationEntry;
			}
		}
	}

	// Animation not found
	return NULL;
}
#endif

Bool CEntity::HasAnimation( const CName& animationName, Bool rootOnly ) const
{
	if ( rootOnly )
	{
		CAnimatedComponent* animatedComponent = GetRootAnimatedComponent();

		return animatedComponent && animatedComponent->GetAnimationContainer() ? animatedComponent->GetAnimationContainer()->HasAnimation( animationName ) : false;
	}
	else
	{
		for ( Uint32 i=0; i<m_components.Size(); i++ )
		{
			CAnimatedComponent* animatedComponent = Cast< CAnimatedComponent >( m_components[i] );
			if ( animatedComponent && animatedComponent->GetAnimationContainer() )
			{
				if ( animatedComponent->GetAnimationContainer()->HasAnimation( animationName ) )
				{
					return true;
				}
			}
		}
	}

	// Animation not found
	return false;
}

void CEntity::FreezeAllAnimatedComponents()
{
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		(*it)->Freeze();
	}
}

void CEntity::UnfreezeAllAnimatedComponents()
{
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		(*it)->Unfreeze();
	}
}

#ifndef NO_EDITOR
void CEntity::EditorOnTransformChangeStart()
{
	TBaseClass::EditorOnTransformChangeStart();
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->EditorOnTransformChangeStart();
	}
}

void CEntity::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->EditorOnTransformChanged();
	}
}

void CEntity::EditorOnTransformChangeStop()
{
	TBaseClass::EditorOnTransformChangeStop();
	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->EditorOnTransformChangeStop();
	}
}

void CEntity::EditorPreDeletion()
{
	TBaseClass::EditorPreDeletion();

	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->EditorPreDeletion();
	}
}
void CEntity::EditorPostCreation()
{
	
}
void CEntity::EditorPostDuplication( CNode* originalNode )
{
	if ( !originalNode )
		return;


	TBaseClass::EditorPostDuplication( originalNode );

	for ( auto it = m_components.Begin(), end = m_components.End(); it != end; ++it )
	{
		(*it)->EditorPostDuplication( originalNode );
	}
}
#endif

void CEntity::OnCutsceneStarted()
{
	CallEvent( CNAME( OnCutsceneStarted ) );

	for ( CComponent* component : m_components )
	{
		component->OnCutsceneStarted();
	}
}

void CEntity::OnCutsceneEnded()
{
	CallEvent( CNAME( OnCutsceneEnded ) );

	for ( CComponent* component : m_components )
	{
		component->OnCutsceneEnded();
	}
}

#ifndef NO_DEBUG_PAGES
void CEntity::OnDebugPageInfo( class CDebugPageHTMLDocument& doc )
{
	TBaseClass::OnDebugPageInfo( doc );

	// has streaming data ?
	if ( CheckStaticFlag( ESF_Streamed ) )
	{
		CDebugPageHTMLInfoBlock info( doc, "Entity streaming" );

		/// Show data
		if ( GetLocalStreamedComponentDataBuffer().GetSize() > 0 )
		{
			info.Info( "Internal streaming data: ").
				Link( "/streamingdata/?id=%d", GetObjectIndex() ).Writef("(Load)" );

			info.Info( "Internal streaming tables: ").
				Link( "/streamingdatatables/?id=%d", GetObjectIndex() ).Writef("(Show)" );
		}
		else if ( GetEntityTemplate() && GetEntityTemplate()->GetEntityObject() && GetEntityTemplate()->GetEntityObject()->GetLocalStreamedComponentDataBuffer().GetSize() > 0 )
		{
			info.Info( "Template streaming data: ").
				Link( "/streamingdata/?id=%d", GetEntityTemplate()->GetEntityObject()->GetObjectIndex() ).
				Writef("(Load)" );

			info.Info( "Template streaming tables: ").
				Link( "/streamingdatatables/?id=%d", GetEntityTemplate()->GetEntityObject()->GetObjectIndex() ).
				Writef("(Show)" );
		}

		/// Breaks
		{
			/// URL action
			StringAnsi action;
			if ( doc.GetURL().GetKey( "action", action ) )
			{
				if ( action == "bson" )
				{
					SetDynamicFlag(EDF_DebugBreakOnStreamIn, true);
				}
				else if ( action == "bsoff" )
				{
					SetDynamicFlag(EDF_DebugBreakOnStreamIn, false);
				}
				else if ( action == "buon" )
				{
					SetDynamicFlag(EDF_DebugBreakOnStreamOut, true);
				}
				else if ( action == "buoff" )
				{
					SetDynamicFlag(EDF_DebugBreakOnStreamOut, false);
				}
			}

			if ( CheckDynamicFlag( EDF_DebugBreakOnStreamIn ) )
			{
				info.Info( "Break on stream in is enabled " ).Link( "/object/?id=%d&action=bsoff", GetObjectIndex() ).Write("(Disable)");
			}
			else
			{
				info.Info( "Break on stream in is disabled " ).Link( "/object/?id=%d&action=bson", GetObjectIndex() ).Write("(Enable)");
			}

			if ( CheckDynamicFlag( EDF_DebugBreakOnStreamOut ) )
			{
				info.Info( "Break on unstream in is enabled " ).Link( "/object/?id=%d&action=buoff", GetObjectIndex() ).Write("(Disable)");
			}
			else
			{
				info.Info( "Break on unstream in is disabled " ).Link( "/object/?id=%d&action=buon", GetObjectIndex() ).Write("(Enable)");
			}
		}
	}
}
#endif

CAnimatedComponent* CEntity::GetRootAnimatedComponent() const
{
	return m_components.Size() > 0 ? Cast< CAnimatedComponent >( m_components[0] ) : NULL;
}

#ifndef NO_EDITOR
Bool CEntity::AllowStreamingForComponent( CComponent* component )
{
	// Explicitly ignore subclasses which have a streamed superclass
    if ( component->GetClass()->IsA( SRTTI::GetInstance().FindClass( CName( TXT("CGameplayLightComponent") ) ) ) )
    {
        return false;
    }


	// For now only allow static mesh components
	return	component->IsA< CStaticMeshComponent >() ||
		component->IsA< CMeshComponent >() ||
		component->IsA< CRigidMeshComponent >() ||									
		component->IsA< CEffectDummyComponent>() ||
		component->IsA< CDecalComponent >() ||
		component->IsA< CDimmerComponent >() ||					
		component->IsA< CDestructionSystemComponent >() ||			
		component->IsA< CClothComponent >() ||
		component->IsA< CPhantomComponent >() ||
		component->IsA< CAnimatedComponent >() ||			
		component->IsA< CFlareComponent >() ||
		component->IsA< CAnimDangleComponent >() ||
		component->IsA< CSoundEmitterComponent >() ||			
		component->IsA< CDynamicFoliageComponent >() ||
		component->IsA< CSwitchableFoliageComponent >() ||
		component->IsA< CSwarmRenderComponent >() ||
		component->IsA< COverrideStreamingDistanceComponent >() ||
		// temp hack
		// don't want to include game project here
		// technically this is needed for the sake of resave
		component->GetClass()->IsA( SRTTI::GetInstance().FindClass( CName( TXT("CInteractionComponent") ) ) );
}
#endif

#ifndef NO_EDITOR_ENTITY_VALIDATION
Bool CEntity::OnValidate( TDynArray< String >& log ) const
{
	Bool result = true;

	// Check root animated component
	{
		Int32 rootAc = 0;
		String acError;

		{
			// Root
			for ( Uint32 i=0; i<m_components.Size(); i++ )
			{
				CAnimatedComponent* ac = Cast<CAnimatedComponent>( m_components[i] );

				if ( ac && ac->GetParentAttachments().Empty() )
				{
					rootAc++;
					acError.Append( ac->GetFriendlyName().AsChar(), ac->GetFriendlyName().GetLength() );
					acError.Append( TXT("\n"), 1 );
				}
			}

			if ( rootAc > 1 )
			{
				log.PushBack( String::Printf( TXT("Entity has got more than one root animated component:\n%s"), acError.AsChar() ) );
				result = false;
			}
		}
	}

	// Validate all components
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		if ( !m_components[i]->OnValidate( log ) )
		{
			result = false;
		}
	}
	return result;
}
#endif // NO_EDITOR_ENTITY_VALIDATION

#ifndef NO_DATA_VALIDATION

static Bool IsSingularComponent( const CComponent* component )
{
	if ( component->IsA< CMimicComponent >() ) return true;
	return false;
}

void CEntity::FullValidation( const String& additionalContext ) const
{
	// check for duplicates of singular components
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		CComponent* component = m_components[i];
		if ( component )
		{
			// report components that are singular but exist more than once
			if ( IsSingularComponent( component ) )
			{
				const CClass* componentClass = component->GetClass();

				for ( Uint32 j=i+1; j<m_components.Size(); ++j )
				{
					CComponent* otherComponent = m_components[j];
					if ( otherComponent && otherComponent->IsA( componentClass ) )
					{
						DATA_HALT( DES_Uber, CResourceObtainer::GetResource( this ), TXT("World"), TXT("Singular component '%ls' in entity '%ls' %ls is duplicated by component '%ls'"), 
							component->GetName().AsChar(), GetName().AsChar(), additionalContext.AsChar(), otherComponent->GetName().AsChar() );
					}
				}
			}
		}
	}
}

void CEntity::OnCheckDataErrors( Bool isInTemplate ) const
{
	// Pass to base class
	TBaseClass::OnCheckDataErrors( isInTemplate );

	if ( !isInTemplate )
	{
		// No components ?
		if ( m_components.Empty() && GetLocalStreamedComponentDataBuffer().GetSize() == 0 )
		{
			if ( GetClass() == ClassID< CEntity >() )
			{
				DATA_HALT( DES_Minor, CResourceObtainer::GetResource( this ), TXT("World"), TXT("Entity '%ls' has no components, no streaming components and is an CEntity. Seems like it's not needed."), GetName().AsChar() );
			}
		}

		// Check components - only when not templated
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];
			if ( component )
			{
				component->OnCheckDataErrors( isInTemplate );
			}
		}
	}
}
#endif // NO_DATA_VALIDATION

void CEntity::OnLayerUnloading()
{
}

void CEntity::OnAppearanceChanged( const CEntityAppearance& appearance )
{
}

Bool CEntity::OnPoolRequest() 
{ 
	for ( ComponentIterator< CAnimatedComponent > it( this ); it; ++it )
	{
		CAnimatedComponent* ac = *it;
		if ( ac->GetBehaviorStack() )
		{
			ac->GetBehaviorStack()->Reset();
		}
	}

	return true; 
}

namespace MaterialReplacementHelperFunctions
{
	RED_INLINE Bool IncludeExcludeListAppliesToComponent( CComponent* component, const TDynArray< CName >& list )
	{
		// Check if the name is part of the list
		if ( list.Exist( CName( component->GetName() ) ) )
		{
			return true;
		}

		// Otherwise check if the list contains any of the tags
		const TDynArray< CName >& tags = component->GetTags().GetTags();
		for ( const CName& tag : tags )
		{
			if ( list.Exist( tag ) )
			{
				return true;
			}
		}

		// This component does not match the list
		return false;
	}

	RED_INLINE Bool CanApplyToComponent( CComponent* component, const SMaterialReplacementInfo& mri )
	{
		// If we have an include list, check if the component is part of that list
		if ( !mri.includeList.Empty() )
		{
			return IncludeExcludeListAppliesToComponent( component, mri.includeList );
		}
		else if ( !mri.excludeList.Empty() ) // otherwise if we have an exclude list, make sure it isn't in the list
		{
			return !IncludeExcludeListAppliesToComponent( component, mri.excludeList );
		}
		else // if both lists are empty we can apply the effect to all components
		{
			return true;
		}
	}

	RED_INLINE void ProcessComponentOnEnable( CComponent* component, IMaterial* material, Bool drawOriginal, Bool findExcludeTag, Bool findIncludeTag, const CName& exclusionTag, const CName& tag, Bool forceMeshAlternatives, IRenderProxy* proxy )
	{
		if ( !material )
		{
			return;
		}

		IRenderProxyInterface* inter = component->QueryRenderProxyInterface();

		if ( !proxy && ( !inter || inter->GetNumberOfRenderProxies() == 0 ) )
		{
			return;
		}

		Bool includeComp = !findIncludeTag;

		if( findIncludeTag || findExcludeTag )
		{
			const TagList& tags = component->GetTags();
			if ( findIncludeTag && tags.HasTag( tag ) )
			{
				includeComp = true;
			}

			if ( findExcludeTag && tags.HasTag( exclusionTag ) )
			{
				includeComp = false;
			}
		}

		if ( includeComp )
		{
			// For fur components with forced mesh alternative we need to fallback to the mesh version
			// The effect will be applied by OnProxyAttached below
			if ( forceMeshAlternatives && component->IsA< CFurComponent >() )
			{
				static_cast< CFurComponent* >( component )->SetForceMesh( true );
			}
			else
			{
				if ( proxy )
				{
					( new CRenderCommand_OverrideProxyMaterial( proxy, material, drawOriginal ) )->Commit();
				}
			
				for ( RenderProxyIterator it( inter, RPT_None ); it; ++it )
				{
					IRenderProxy* proxyElem = (*it);
					if ( proxyElem )
					{
						if ( proxyElem->GetType() == RPT_Mesh || proxyElem->GetType() == RPT_Apex || proxyElem->GetType() == RPT_Fur )
						{
							( new CRenderCommand_OverrideProxyMaterial( proxyElem, material, drawOriginal ) )->Commit();
						}
					}
				}
			}
		}

	}

	RED_INLINE void ProcessComponentOnDisable( CComponent* component, Bool hadForcedMeshAlternatives )
	{
		IRenderProxyInterface* inter = component->QueryRenderProxyInterface();
		if ( inter )
		{
			// Fur components will need to recreate the proxy anyway
			if ( hadForcedMeshAlternatives && component->IsA< CFurComponent >() && static_cast< CFurComponent* >( component )->IsMeshForced() )
			{
				static_cast< CFurComponent* >( component )->SetForceMesh( false );
			}
			else
			{
				for ( RenderProxyIterator it( inter, RPT_None ); it; ++it )
				{
					IRenderProxy* proxyElem = (*it);
					if ( proxyElem )
					{
						if ( proxyElem->GetType() == RPT_Mesh || proxyElem->GetType() == RPT_Apex )
						{
							( new CRenderCommand_DisableProxyMaterialOverride( proxyElem ) )->Commit();
						}
					}
				}
			}
		}
	}
}

Bool CEntity::SetMaterialReplacement( IMaterial* material, Bool drawOriginal /* = false */, const CName& tag /* = CName::NONE */, const CName& exclusionTag /* = CName::NONE */, const TDynArray< CName >* includeList /* = nullptr */, const TDynArray< CName >* excludeList /* = nullptr */, Bool forceMeshAlternatives )
{
	ASSERT( material );

	if ( HasMaterialReplacement() )
	{
		WARN_ENGINE( TXT("Attempted to set a material replacement with another active, this will replace the current one") );
		DisableMaterialReplacement();
	}

	m_materialReplacementInfo = new SMaterialReplacementInfo();
	m_materialReplacementInfo->material = material;
	m_materialReplacementInfo->drawOriginal = drawOriginal;
	m_materialReplacementInfo->tag = tag;
	m_materialReplacementInfo->exclusionTag = exclusionTag;
	m_materialReplacementInfo->forceMeshAlternatives = forceMeshAlternatives;
	if ( includeList != nullptr )
	{
		m_materialReplacementInfo->includeList.PushBack( *includeList );
	}
	if ( excludeList != nullptr )
	{
		m_materialReplacementInfo->excludeList.PushBack( *excludeList );
	}

	const Bool findIncludeTag = tag != CName::NONE;
	const Bool findExcludeTag = exclusionTag != CName::NONE;
	const Bool findTags = findIncludeTag || findExcludeTag;

	for ( CComponent* comp : m_components )
	{
		if ( MaterialReplacementHelperFunctions::CanApplyToComponent( comp, *m_materialReplacementInfo ) )
		{
			MaterialReplacementHelperFunctions::ProcessComponentOnEnable( comp, material,drawOriginal, findExcludeTag, findIncludeTag, exclusionTag, tag, forceMeshAlternatives, NULL );
		}
	}
	
	// Propagate to attached items
	PropagateCallToItemEntities([&](CEntity* attachedEntity){
		attachedEntity->SetMaterialReplacement( material, drawOriginal, tag, exclusionTag, includeList, excludeList, forceMeshAlternatives );
	});

	return true;
}

void CEntity::DisableMaterialReplacement()
{
	// Crash guard
	if ( !HasMaterialReplacement() )
	{
		return;
	}

	// Remove material replacement references to avoid it being reapplied
	Bool forceMeshAlternatives = m_materialReplacementInfo->forceMeshAlternatives;
	delete m_materialReplacementInfo;
	m_materialReplacementInfo = NULL;

	// Propagate to components
	for ( CComponent* component : m_components )
	{
		MaterialReplacementHelperFunctions::ProcessComponentOnDisable( component, forceMeshAlternatives );
	}

	// Propagate to attached items
	PropagateCallToItemEntities([&](CEntity* attachedEntity){
		attachedEntity->DisableMaterialReplacement();
	});
}

Bool CEntity::HasMaterialReplacement()
{
	return m_materialReplacementInfo != NULL;
}

void CEntity::SendParametersMaterialReplacement( const Vector& params )
{
	for ( CComponent* comp : m_components )
	{
		IRenderProxyInterface* proxyInterface = comp->QueryRenderProxyInterface();
		if ( proxyInterface )
		{
			for ( RenderProxyIterator it( proxyInterface, RPT_None ); it; ++it )
			{
				IRenderProxy* proxy = *it;
				ASSERT( proxy );
				
				if ( proxy->GetType() == RPT_Mesh || proxy->GetType() == RPT_Apex )
				{
					( new CRenderCommand_UpdateOverrideParametersBatch( proxy, params ) )->Commit();
				}
			}
		}
	}

	// Propagate to attached items
	PropagateCallToItemEntities([&](CEntity* attachedEntity){
		attachedEntity->SendParametersMaterialReplacement( params );
	});
}

void CEntity::ApplyMeshComponentColoringUsing( const SEntityTemplateColoringEntry& entry )
{
	Matrix m1, m2;
	entry.m_colorShift1.CalculateColorShiftMatrix( m1 );
	entry.m_colorShift2.CalculateColorShiftMatrix( m2 );

	for ( ComponentIterator< CMeshTypeComponent > it( this ); it; ++it )
	{
		CMeshTypeComponent* component = *it;
		if ( component->GetName() == entry.m_componentName.AsString() )
		{
			component->SendColorShiftMatrices( m1, m2 );
		}
	}
}

Bool CEntity::ApplyMeshComponentColoring()
{
	// Cannot use mesh component coloring without an entity template
	if ( !m_template ) return false;

	CAppearanceComponent* appearanceComponent = CAppearanceComponent::GetAppearanceComponent( this );
	THashSet<CName> componentNames;
	TDynArray<SEntityTemplateColoringEntry> entries, noneEntries;

	// Fill entries from appearance (if any)
	if ( appearanceComponent )
	{
		const CName& appearance = appearanceComponent->GetAppearance();
		TDynArray<SEntityTemplateColoringEntry> appearanceEntries;
		m_template->GetColoringEntriesForAppearance( appearance, appearanceEntries );
		entries.PushBack( appearanceEntries );
		for ( Uint32 i=0; i<appearanceEntries.Size(); ++i )
		{
			componentNames.Insert( appearanceEntries[i].m_componentName );
		}
	}

	// Fill entries from the coloring entries without an appearance while
	// ignoring components which have already been colored by an appearance
	// above
	m_template->GetColoringEntriesWithoutAppearance( noneEntries );
	for ( Uint32 i=0; i<noneEntries.Size(); ++i )
	{
		if ( !componentNames.Exist( noneEntries[i].m_componentName ) )
		{
			entries.PushBack( noneEntries[i] );
		}
	}

	// Use the collected entries to color the entity
	for ( Uint32 i=0; i<entries.Size(); ++i )
	{
		ApplyMeshComponentColoringUsing( entries[i] );
	}

	return entries.Size() > 0;
}

void CEntity::SetAllComponentsFloatParameter( CFXTrackItemParameterFloatPlayData* playData )
{
	if ( !m_allComponentsEffectParamInfo )
	{
		m_allComponentsEffectParamInfo = new SAllComponentsEffectParamInfo;
	}
		
	m_allComponentsEffectParamInfo->m_floatParamPlayData.PushBack( playData );
}

void CEntity::DisableAllComponentsFloatParameter( CFXTrackItemParameterFloatPlayData* playData )
{
	ASSERT( m_allComponentsEffectParamInfo );
	if ( !m_allComponentsEffectParamInfo )
	{
		return;
	}

	m_allComponentsEffectParamInfo->m_floatParamPlayData.Remove( playData );

	if ( m_allComponentsEffectParamInfo->m_floatParamPlayData.Empty() )
	{
		delete m_allComponentsEffectParamInfo;
		m_allComponentsEffectParamInfo = NULL;
	}
}

Bool CEntity::HasAllComponentsFloatParameter()
{
	return m_allComponentsEffectParamInfo != NULL;
}

void CEntity::OnAttachmentCreated()
{
	m_transform.SetPosition( Vector::ZERO_3D_POINT );
	m_transform.SetRotation( EulerAngles::ZEROS );
}

void CEntity::OnAttachmentBroken()
{
	m_transform.SetPosition( GetWorldPosition() );
	m_transform.SetRotation( GetWorldRotation() );
}

Bool CEntity::CreateAttachmentImpl( THandle< CEntity > parentHandle, CName slot, const Vector& relativePosition, const EulerAngles& relativeRotation )
{	
	if ( GetTransformParent() )
	{
		return false;		
	}

	Bool ret = false;

	CEntity* parentEntity = parentHandle.Get();
	if ( parentEntity )
	{
		THandle< CEntity > childHandle( this );

		ret = CallEvent( CNAME( OnCanCreateParentAttachment ), parentHandle ) != CR_EventFailed;
		if ( !ret )
		{			
			return false;
		}

		ret = parentEntity->CallEvent( CNAME( OnCanCreateChildAttachment ), childHandle ) != CR_EventFailed;
		if ( !ret )
		{			
			return false;
		}

		if ( slot != CName::NONE )
		{
			// Attach entity to slot

			if ( !parentEntity->GetEntityTemplate() )
			{
				WARN_ENGINE( TXT("Error in CreateAttachment - parent entity doesn't have entity template") );				
				return false;
			}

			const EntitySlot* entitySlot = parentEntity->GetEntityTemplate()->FindSlotByName( slot, true );
			if ( entitySlot )
			{
				OnAttachmentCreated();

				// Create attachment using slot.
				HardAttachmentSpawnInfo spawnInfo;
				spawnInfo.m_relativePosition = relativePosition;
				spawnInfo.m_relativeRotation = relativeRotation;
				IAttachment* att = entitySlot->CreateAttachment( spawnInfo, parentEntity, this, nullptr );
				ret = ( att != nullptr );
					
				ForceUpdateTransformNodeAndCommitChanges();
			}
			else
			{
				WARN_ENGINE( TXT("Error in CreateAttachment - parent entity doesn't have slot '%ls' in entity template"), slot.AsString().AsChar() );				
				return false;
			}
		}
		else
		{
			// Attach entity to entity

			OnAttachmentCreated();

			HardAttachmentSpawnInfo ainfo;
			ainfo.m_relativePosition = relativePosition;
			ainfo.m_relativeRotation = relativeRotation;
			IAttachment* att = parentEntity->Attach( this, ainfo );
			ret = att != NULL;

			ForceUpdateTransformNodeAndCommitChanges();
		}

		if ( ret )
		{
			CallEvent( CNAME( OnParentAttachmentCreated ), parentHandle );
			parentEntity->CallEvent( CNAME( OnChildAttachmentCreated ), childHandle );
		}
		else
		{
			WARN_ENGINE( TXT("Error in CreateAttachment - Couldn't create atachment between parent entity '%ls' and child entity '%ls'"), parentEntity->GetFriendlyName().AsChar(), this->GetFriendlyName().AsChar() );
		}
	}
	return ret;
}

Bool CEntity::CreateAttachmentAtBoneWSImpl( THandle< CEntity > parentHandle, CName bone, Vector const & worldLocation, EulerAngles const & worldRotation )
{	
	if ( GetTransformParent() )
	{
		return false;		
	}

	Bool ret = false;

	CEntity* parentEntity = parentHandle.Get();
	if ( parentEntity )
	{
		THandle< CEntity > childHandle( this );

		ret = CallEvent( CNAME( OnCanCreateParentAttachment ), parentHandle ) != CR_EventFailed;
		if ( !ret )
		{			
			return false;
		}

		ret = parentEntity->CallEvent( CNAME( OnCanCreateChildAttachment ), childHandle ) != CR_EventFailed;
		if ( !ret )
		{			
			return false;
		}

		// Attach entity to slot

		if ( !parentEntity->GetEntityTemplate() )
		{
			WARN_ENGINE( TXT("Error in CreateAttachment - parent entity doesn't have entity template") );				
			return false;
		}

		Int32 boneIdx = parentEntity->GetRootAnimatedComponent()->FindBoneByName( bone );
		if ( boneIdx != -1 )
		{
			OnAttachmentCreated();

			// build attachment matrix in world space
			Matrix attachmentMatWS = worldRotation.ToMatrix();
			attachmentMatWS.SetTranslation( worldLocation );
			// get attachment in bone space
			Matrix boneMatWS = parentEntity->GetRootAnimatedComponent()->GetBoneMatrixWorldSpace( boneIdx );
			Matrix attachmentMatBS = attachmentMatWS * boneMatWS.FullInverted();
			// get relative loc and rotation
			Vector relativeLoc = attachmentMatBS.GetTranslation();
			EulerAngles relativeRotation = attachmentMatBS.ToEulerAnglesFull();

			HardAttachmentSpawnInfo ainfo;
			ainfo.m_parentSlotName = bone;
			ainfo.m_relativePosition = relativeLoc;
			ainfo.m_relativeRotation = relativeRotation;

			CComponent* parentComponent = parentEntity->GetRootAnimatedComponent();
			if ( parentComponent )
			{
				IAttachment* att = parentComponent->Attach( this, ainfo );
				ret = att != NULL;
			}
			else
			{
				IAttachment* att = parentEntity->Attach( this, ainfo );
				ret = att != NULL;
			}
			ForceUpdateTransformNodeAndCommitChanges();
		}
		else
		{
			WARN_ENGINE( TXT("Error in CreateAttachmentAtBone - parent entity's root animated component doesn't have bone '%ls'"), bone.AsString().AsChar() );				
			return false;
		}

		if ( ret )
		{
			CallEvent( CNAME( OnParentAttachmentCreated ), parentHandle );
			parentEntity->CallEvent( CNAME( OnChildAttachmentCreated ), childHandle );
		}
		else
		{
			WARN_ENGINE( TXT("Error in CreateAttachment - Couldn't create atachment between parent entity '%ls' and child entity '%ls'"), parentEntity->GetFriendlyName().AsChar(), this->GetFriendlyName().AsChar() );
		}
	}
	return ret;
}

Bool CEntity::BreakAttachment()
{
	CNode* parentNode = GetTransformParent() ? GetTransformParent()->GetParent() : NULL;
	if ( parentNode )
	{
		CEntity* parentEntity = parentNode->AsEntity();
		if ( !parentEntity )
		{
			parentEntity = Cast< CEntity >( parentNode->GetParent() );
		}

		ASSERT( parentEntity );
		if ( !parentEntity )
		{
			return false;
		}

		THandle< CEntity > parentHandle( parentEntity );
		THandle< CEntity > childHandle( this );

		if ( CallEvent( CNAME( OnCanBreakParentAttachment ), parentHandle ) == CR_EventFailed )
		{
			return false;
		}

		if ( parentEntity->CallEvent( CNAME( OnCanBreakChildAttachment ), childHandle ) == CR_EventFailed )
		{
			return false;
		}

		GetTransformParent()->Break();

		OnAttachmentBroken();

		CallEvent( CNAME( OnParentAttachmentBroken ), parentHandle );
		parentEntity->CallEvent( CNAME( OnChildAttachmentBroken ), childHandle );

		return true;
	}

	return false;
}

ERenderVisibilityResult CEntity::GetLastFrameVisibility() const
{
	if ( m_visibilityQuery )
	{
		return GetLayer()->GetWorld()->GetRenderSceneEx()->GetVisibilityQueryResult( m_visibilityQuery );
	}
	else
	{
		return RVR_NotTested;
	}
}

Bool CEntity::WasVisibleLastFrame() const
{
	const ERenderVisibilityResult result = GetLastFrameVisibility();
	if ( result == RVR_Visible )
	{
		return true;
	}
	if ( WasInventoryVisibleLastFrame() )
	{
		return true;
	}
	return result == RVR_NotTested;
}

void CEntity::OnProxyDetached( CComponent* comp )
{
	if ( HasAllComponentsFloatParameter() )
	{
		TDynArray< CFXTrackItemParameterFloatPlayData* >& playDatas = m_allComponentsEffectParamInfo->m_floatParamPlayData;
		for ( Uint32 i=0; i<playDatas.Size(); ++i )
		{
			playDatas[i]->RemoveComponent( comp );
		}
	}

	// Make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	// It's assumed that this isn't going to be happening often, and that the listeners list should be small enough that the
	// extra copy and checks won't be too expensive.
	TDynArray< IEntityListener* > tempListeners = m_entityListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		IEntityListener* listener = tempListeners[ i ];

		// Make sure this listener hasn't been unregistered since we started out. If it was unregistered, maybe it's been destroyed?
		if ( m_entityListeners.Exist( listener ) )
		{
			listener->OnNotifyEntityRenderProxyRemoved( this, comp );
		}
	}
}

void CEntity::OnProxyAttached( CComponent* comp, IRenderProxy* proxy )
{
	if ( HasMaterialReplacement() && MaterialReplacementHelperFunctions::CanApplyToComponent( comp, *m_materialReplacementInfo ) )
	{
		Bool forceMeshAlternatives = m_materialReplacementInfo->forceMeshAlternatives;

		// Do not re-force the mesh alternative if this proxy was attached because
		// we switched on the mesh alternative forcing
		if ( forceMeshAlternatives && comp->IsA< CFurComponent >() )
		{
			forceMeshAlternatives = !static_cast< CFurComponent* >( comp )->IsMeshForced();
		}

		MaterialReplacementHelperFunctions::ProcessComponentOnEnable( comp, 
			m_materialReplacementInfo->material,
			m_materialReplacementInfo->drawOriginal,
			m_materialReplacementInfo->exclusionTag != CName::NONE, 
			m_materialReplacementInfo->tag != CName::NONE, 
			m_materialReplacementInfo->exclusionTag, 
			m_materialReplacementInfo->tag,
			forceMeshAlternatives,
			proxy );
	}

	if ( HasAllComponentsFloatParameter() )
	{
		TDynArray< CFXTrackItemParameterFloatPlayData* >& playDatas = m_allComponentsEffectParamInfo->m_floatParamPlayData;
		for ( Uint32 i=0; i<playDatas.Size(); ++i )
		{
			playDatas[i]->AddComponent( comp );
		}
	}

	if ( IsRenderingSuspended() )
	{
		proxy->SetVisible( false );
	}

	// HACK : If this is a fur component, force re-apply skinning. With fur, we can switch between fur/mesh on the fly, by
	// applying or clearing material replacements, and we need to be sure the data is not missing for a frame.
	if ( CFurComponent* furComp = Cast< CFurComponent >( comp ) )
	{
		if ( furComp->GetTransformParent() != nullptr )
		{
			if ( CMeshSkinningAttachment* skinningAttachment = furComp->GetTransformParent()->ToSkinningAttachment() )
			{
				// To avoid any potential problems with updating skinning twice in one frame, we'll force the attachment
				// to discard the old data. This sounds wasteful, but it shouldn't really be, because generally the fur
				// and mesh fallback will probably have different bone counts anyways.
				skinningAttachment->DiscardSkinningData();

				Box dummyBox;
				SMeshSkinningUpdateContext updateContext;
				skinningAttachment->UpdateTransformAndSkinningData( dummyBox, updateContext );
				updateContext.CommitCommands();
			}
		}

#ifdef RED_ASSERTS_ENABLED
		// Scan through all parent attachments, and make sure that if there's a skinning attachment, it is the one
		// given by GetTransformParent. Sanity check that the above works correct.
		{
			const CMeshSkinningAttachment* foundSkinningAttachment = nullptr;
			for ( const IAttachment* att : furComp->GetParentAttachments() )
			{
				if ( att->IsA< CMeshSkinningAttachment >() )
				{
					RED_FATAL_ASSERT( foundSkinningAttachment == nullptr, "Already found a skinning attachment" );
					foundSkinningAttachment = static_cast< const CMeshSkinningAttachment* >( att );
				}
			}
			RED_FATAL_ASSERT( foundSkinningAttachment == furComp->GetTransformParent(), "foundSkinningAttachment doesn't match the fur component's transform parent" );
		}
#endif
	}

	// Make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	// It's assumed that this isn't going to be happening often, and that the listeners list should be small enough that the
	// extra copy and checks won't be too expensive.
	TDynArray< IEntityListener* > tempListeners = m_entityListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		IEntityListener* listener = tempListeners[ i ];

		// Make sure this listener hasn't been unregistered since we started out. If it was unregistered, maybe it's been destroyed?
		if ( m_entityListeners.Exist( listener ) )
		{
			listener->OnNotifyEntityRenderProxyAdded( this, comp, proxy );
		}
	}
}

void CEntity::OnCollectAnimationSyncTokens( CName animationName, TDynArray< CAnimationSyncToken* >& tokens ) const
{
	for ( ComponentIterator< CAnimDangleComponent > it( this ); it; ++it )
	{
		CAnimDangleComponent* c = *it;
		c->OnCollectAnimationSyncTokens( animationName, tokens );
	}
}

Bool CEntity::RegisterEntityListener( IEntityListener* listener )
{
	ASSERT( listener, TXT("Registering NULL listener") );
	if ( !listener ) return false;

	if ( m_entityListeners.PushBackUnique( listener ) )
	{
#ifndef RED_FINAL_BUILD
		listener->m_listenedEntities.PushBack( this );
#endif
		return true;
	}

	HALT("Trying to register an IEntityListener that is already registered to this entity");

	return false;
}

Bool CEntity::UnregisterEntityListener( IEntityListener* listener )
{
	ASSERT( listener, TXT("Unregistering NULL listener") );
	if ( !listener ) return false;

	if ( m_entityListeners.RemoveFast( listener ) )
	{
#ifndef RED_FINAL_BUILD
		listener->m_listenedEntities.RemoveFast( this );
#endif
		return true;
	}

	HALT("Trying to unregister an IEntityListener that is not registered to this entity" );

	return false;
}

void CEntity::FixRootAnimatedComponentPosition( void )
{
    // Check root animated component
    CAnimatedComponent* acRooot = GetRootAnimatedComponent();
    if ( acRooot == NULL || !acRooot->GetParentAttachments().Empty() )
    {
        for ( Uint32 i=1; i<m_components.Size(); i++ )
        {
            CAnimatedComponent* ac = Cast<CAnimatedComponent>( m_components[i] );										
            if ( ac && ac->GetParentAttachments().Empty() )
            {
                m_components.Swap( 0, i );
                ASSERT( GetRootAnimatedComponent() );
                break;
            }		
        }
    }
}

void CEntity::SendNotifyComponentAdded( CComponent* component )
{
	// Make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	// It's assumed that this isn't going to be happening often, and that the listeners list should be small enough that the
	// extra copy and checks won't be too expensive.
	TDynArray< IEntityListener* > tempListeners = m_entityListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		IEntityListener* listener = tempListeners[ i ];

		// Make sure this listener hasn't been unregistered since we started out. If it was unregistered, maybe it's been destroyed?
		if ( m_entityListeners.Exist( listener ) )
		{
			listener->OnNotifyEntityComponentAdded( this, component );
		}
	}
}

void CEntity::SendNotifyComponentRemoved( CComponent* component )
{
	// Make a temp copy of the listeners array, so listeners can be (un/)registered while processing, without affecting what
	// we're doing here.
	// It's assumed that this isn't going to be happening often, and that the listeners list should be small enough that the
	// extra copy and checks won't be too expensive.
	TDynArray< IEntityListener* > tempListeners = m_entityListeners;
	const Uint32 numListeners = tempListeners.Size();
	for ( Uint32 i = 0; i < numListeners; ++i )
	{
		IEntityListener* listener = tempListeners[ i ];

		// Make sure this listener hasn't been unregistered since we started out. If it was unregistered, maybe it's been destroyed?
		if ( m_entityListeners.Exist( listener ) )
		{
			listener->OnNotifyEntityComponentRemoved( this, component );
		}
	}
}

void CEntity::PurgeStreamingBuffer()
{
	m_streamingDataBuffer.Clear();
}

void CEntity::StreamedFlagChanged()
{
#ifndef NO_EDITOR
	if ( IsAttached() )
	{
		// If the flag was set to false, make sure all the components are streamed in
		// and remove any object/component flags that might make them become lost
		if ( !ShouldBeStreamed()  )
		{
			// Save previous streaming lock state and unlock the entity
			Bool wasLocked = CheckDynamicFlag( EDF_StreamingLocked );
			ClearDynamicFlag( EDF_StreamingLocked );

			// Revert it temporarily so we'll stream in the components
			SetStaticFlag( ESF_Streamed );
			CreateStreamedComponents( SWN_DoNotNotifyWorld );
			ClearStaticFlag( ESF_Streamed );

			// Convert all components to non-streamed
			for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
			{
				THandle< CComponent >& componentHandle = *it;
				
				// Make sure the component hasn't been destroyed behind our backs
				if ( componentHandle.IsValid() )
				{
					CComponent* component = componentHandle.Get();
					
					// Ignore imported components
					if ( component->IsUsedInAppearance() || component->HasFlag( NF_IncludedFromTemplate ) )
					{
						continue;
					}

					// Remove the flags
					component->ClearComponentFlag( CF_StreamedComponent );
					component->ClearFlag( OF_Referenced );
					component->ClearFlag( OF_Transient );
				}
			}

			// Clear this streaming arrays
			m_streamingComponents.Clear();
			m_streamingAttachments.Clear();

			// Unregister us from the streaming manager
			if ( m_streamingProxy && GetLayer() && GetLayer()->GetWorld() )
			{
				UnregisterFromStreamingGrid( GetLayer()->GetWorld() );
			}

			// Restore previous streaming lock state
			if ( wasLocked )
			{
				SetDynamicFlag( EDF_StreamingLocked );
			}
		}
		else
		{
			// Register in the streaming grid
			if ( !m_streamingProxy && GetLayer() && GetLayer()->GetWorld() )
			{
				RegisterInStreamingGrid( GetLayer()->GetWorld() );
			}
		}
	}
#endif
}

void CEntity::ForceAutoHideDistanceChanged()
{
#ifndef NO_EDITOR
	for ( CComponent* component : m_components )
	{
		if ( component->IsA< CMeshTypeComponent >() )
		{
			CMeshTypeComponent* meshTypeComponent = Cast< CMeshTypeComponent >( component );
			meshTypeComponent->ForceAutoHideDistance( m_forceAutoHideDistance );
		}
	}
#endif
}

CEntity* CEntity::Duplicate( CLayer* placeOnLayer ) const
{
	CObject* cloned = Clone( nullptr );
	if ( CEntity* clonedEntity = Cast< CEntity >( cloned ) )
	{
		if ( placeOnLayer )
		{
			placeOnLayer->AddEntity( clonedEntity );
		}
		else
		{
			GetLayer()->AddEntity( clonedEntity );
		}
		return clonedEntity;
	}
	else
	{
		return nullptr;
	}
}

#ifndef NO_RESOURCE_USAGE_INFO

namespace 
{
	static void GatherResourceFromStreamableData( const void* data, size_t size, const CName customFlag, const Float distStart, const Float distEnd, class IResourceUsageCollector& collector )
	{
		if ( size > 0 )
		{
			CMemoryFileReader reader( (const Uint8*)data, size, 0 );

			// load the import/export tables
			DependencyLoadingContext loadingContext;
			CDependencyLoader loader( reader, nullptr );
			if ( loader.LoadObjects( loadingContext ) )
			{
				for ( CObject* obj : loadingContext.m_loadedRootObjects )
				{
					CComponent* component = Cast< CComponent >( obj );
					if ( component )
					{
						collector.PushComponent( component->GetName(), component->GetClass()->GetName(), component->GetWorldPositionRef() );
						collector.ReportComponentFlag( customFlag, true );
						collector.ReportVisibilityDistance( distStart, distEnd );
						component->CollectResourceUsage( collector, true ); // streamable part
						collector.PopComponent();
					}
				}				

				// discard loaded objects
				for ( CObject* obj : loadingContext.m_loadedRootObjects )
				{
					obj->Discard();
				}
			}
		}
	}
}

void CEntity::CollectResourceUsage( class IResourceUsageCollector& collector ) const
{
	// bounding box
	collector.ReportBoundingBox( CalcBoundingBox() );

	// template data
	if ( m_template )
		collector.ReportTemplate( m_template->GetDepotPath() );

	// hidden 
	if ( IsHiddenInGame() )
		collector.ReportEntityFlag( CName( TXT("hidden") ), true );

	// process existing components
	for ( CComponent* component : m_components )
	{
		if ( component )
		{
			collector.PushComponent( component->GetName(), component->GetClass()->GetName(), component->GetWorldPosition() );
			component->CollectResourceUsage( collector, false ); // not stremable part
			collector.PopComponent();
		}
	}

	// process streaming LODs (HARDCODED RANGES!)
	GatherResourceFromStreamableData( GetLocalStreamedComponentDataBuffer().GetData(), GetLocalStreamedComponentDataBuffer().GetSize(), CName( TXT("Streaming") ), 0.0f, 2040.0f, collector );
}

#endif

//////////////////////////////////////////////////////////////////////////

Bool CEntity::IsAutoBindingPropertyTypeSupported( const IRTTIType* type ) const
{
	// only handle to components can be resolved in here
	if ( type && type->GetType() == RT_Handle )
	{
		const CRTTIHandleType* handleType = static_cast< const CRTTIHandleType* >( type );
		const CClass* pointedClass = handleType->GetPointedType();
		return pointedClass && pointedClass->IsA< CComponent >();
	}

	// cannot be used for auto bindable property
	return false;
}

Bool CEntity::ValidateAutoBindProperty( const CProperty* autoBindProperty, CName bindingName ) const
{
	return true;
}

Bool CEntity::ResolveAutoBindProperty( const CProperty* autoBindProperty, CName bindingName, void* resultData ) const
{
	RED_FATAL_ASSERT( autoBindProperty != nullptr, "No auto bind property specified for reference - script VM error" );
	RED_FATAL_ASSERT( autoBindProperty->GetType() != nullptr, "Invalid auto bind property specified for reference - script VM error" );
	RED_FATAL_ASSERT( autoBindProperty->GetType()->GetType() == RT_Handle, "Only handles are supported as auto bind properties ATM - script VM error" );

	const CRTTIHandleType* handleType = static_cast< const CRTTIHandleType* >( autoBindProperty->GetType() );
	const CClass* pointedClass = handleType->GetPointedType();

	// any binding - find first component with proper class
	if ( bindingName == CNAME(AnyBindingName) )
	{
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];
			if ( component && component->IsA( pointedClass ) )
			{
				*(BaseSafeHandle*)resultData = BaseSafeHandle( component, pointedClass );
				return true;
			}
		}
	}

	// single component - it will fail if there are two
	else if ( bindingName == CNAME(SingleBindingName) )
	{
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];
			if ( component && component->IsA( pointedClass ) )
			{
				// make sure there are no more components of this class
				CComponent* duplicatedComponent = nullptr;
				for ( Uint32 j=(i+1); j<m_components.Size(); ++j )
				{
					CComponent* otherComponent = m_components[j];
					if ( otherComponent && otherComponent->IsA( pointedClass ) )
					{
						duplicatedComponent = otherComponent;
						break;
					}
				}

				// bind only if singular
				if ( duplicatedComponent )
				{
					WARN_CORE( TXT("Inconclusive component resolve in '%ls': both '%ls' and '%ls' are matching"),
						GetFriendlyName().AsChar(), component->GetName().AsChar(), duplicatedComponent->GetName().AsChar() );

					return false;
				}
				else
				{
					*(BaseSafeHandle*)resultData = BaseSafeHandle( component, pointedClass );
					return true;
				}
			}
		}
	}

	// direct search
	else
	{
		for ( Uint32 i=0; i<m_components.Size(); ++i )
		{
			CComponent* component = m_components[i];
			if ( component && component->IsA( pointedClass ) && component->GetName() == bindingName.AsChar() )
			{
				*(BaseSafeHandle*)resultData = BaseSafeHandle( component, pointedClass );
				return true;
			}
		}
	}

	// not resolved
	return false;
}

void CEntity::SetName( const String & name )
{
#ifndef NO_EDITOR
	m_name = name;
#else
	RED_UNUSED( name );
#endif
}

//////////////////////////////////////////////////////////////////////////

void CEntity::funcAddTimer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, timerName, CName::NONE );
	GET_PARAMETER( Float, period, 0.0f );
	GET_PARAMETER_OPT( Bool, repeats, false );
	GET_PARAMETER_OPT( Bool, scatter, false );
	GET_PARAMETER_OPT( ETickGroup, group, TICK_Main );
	GET_PARAMETER_OPT( Bool, savable, false );
	GET_PARAMETER_OPT( Bool, overrideExisting, true );
	FINISH_PARAMETERS;
	const Uint32 id = AddTimer( timerName, period, repeats, scatter, group, savable, overrideExisting );
	RETURN_INT( id );
}

void CEntity::funcAddGameTimeTimer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, timerName, CName::NONE );
	GET_PARAMETER( GameTime, period, GameTime( 0 ) );
	GET_PARAMETER_OPT( Bool, repeats, false );
	GET_PARAMETER_OPT( Bool, scatter, false );
	GET_PARAMETER_OPT( ETickGroup, group, TICK_Main );
	GET_PARAMETER_OPT( Bool, savable, false );
	GET_PARAMETER_OPT( Bool, overrideExisting, true );
	FINISH_PARAMETERS;
	const Uint32 id = AddTimer( timerName, period, repeats, scatter, group, savable, overrideExisting );
	RETURN_INT( id );
}

void CEntity::funcRemoveTimer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, timerName, CName::NONE );
	GET_PARAMETER_OPT( ETickGroup, group, TICK_Main );
	FINISH_PARAMETERS;
	RemoveTimer( timerName, group );
}

void CEntity::funcRemoveTimerById( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Uint32, id, 0 );
	GET_PARAMETER_OPT( ETickGroup, group, TICK_Main );
	FINISH_PARAMETERS;
	RemoveTimer( id, group );
}

void CEntity::funcRemoveTimers( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RemoveTimers();
}

void CEntity::funcDestroy( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( IsAttaching() || IsDetaching() )
	{
		SCRIPT_WARNING( stack, TXT( "Cannot destroy entity %" ) RED_PRIWs TXT( " because it's attaching" ), GetFriendlyName().AsChar() );
		return;
	}

	if ( !IsAttached() )
	{
		SCRIPT_WARNING( stack, TXT( "Cannot destroy entity %" ) RED_PRIWs TXT( " because it's not attached" ), GetFriendlyName().AsChar() );
		return;
	}

	if ( !IsInitialized() )
	{
		SCRIPT_WARNING( stack, TXT( "Cannot destroy entity %" ) RED_PRIWs TXT( " because it's not initialized" ), GetFriendlyName().AsChar() );
		return;
	}

	CLayer* layer = GetLayer();
	Bool canDestroy = CheckEntityFlag( EF_DestroyableFromScript ) || (layer && layer->GetLayerInfo() && layer->GetLayerInfo()->GetLayerType() == LT_NonStatic);
	if( canDestroy )
	{
		SCRIPT_LOG( stack, TXT( "Destroying entity %" ) RED_PRIWs TXT( " from script" ), GetFriendlyName().AsChar() );
		if ( layer )
		{
			Destroy();
		}
	}
	else
	{
		SCRIPT_WARNING( stack, TXT( "Cannot destroy entity %" ) RED_PRIWs TXT( " that was not created by script" ), GetFriendlyName().AsChar() );
	}
}

void CEntity::funcTeleport( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	FINISH_PARAMETERS;

	Teleport( pos, GetRotation() );
}

void CEntity::funcTeleportWithRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, pos, Vector::ZERO_3D_POINT );
	GET_PARAMETER( EulerAngles, rot, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Teleport( pos, rot );
}

void CEntity::funcTeleportToNode( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, node, NULL );
	GET_PARAMETER_OPT( Bool, applyRotation, true);
	FINISH_PARAMETERS;

	Bool res = false;
	CNode *pNode = node.Get();
	if( pNode )
	{
		res = Teleport( pNode, applyRotation );
	}
	else
	{
		SCRIPT_WARNING( stack, TXT( "TeleportToNode invalid node, entity '%" ) RED_PRIWs TXT( "'" ), GetFriendlyName().AsChar() );
	}

	RETURN_BOOL( res );
}

void CEntity::funcGetPathComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	THandle<CPathComponent>& path = *(THandle<CPathComponent>*) result;

	path = THandle< CPathComponent >( FindComponent< CPathComponent >() );
}

void CEntity::funcGetComponent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( String, name, String::EMPTY );
	FINISH_PARAMETERS;

	RETURN_OBJECT( FindComponent( name ) );
}

void CEntity::funcGetComponentByClassName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	CComponent* component = NULL;
	CClass* c = SRTTI::GetInstance().FindClass( className );	
	if( c )
	{
		for( Uint32 i=0; i<m_components.Size(); i++ )
		{
			if( m_components[i] && m_components[i]->GetClass()->IsA( c ) )
			{			
				component = m_components[i];
				break;
			}
		}
	}

	RETURN_OBJECT( component );
}


void CEntity::funcGetComponentsByClassName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	if( result )
	{
		TDynArray< THandle<CComponent> > & resultArr = *(TDynArray< THandle<CComponent> >*) result;

		CClass* c = SRTTI::GetInstance().FindClass( className );
		if( c )
		{	
			for( Uint32 i=0; i<m_components.Size(); i++ )
			{
				if( m_components[i] && m_components[i]->GetClass()->IsA( c ) )
				{
					resultArr.PushBackUnique( m_components[i] );
				}
			}
		}
	}
}

void CEntity::funcGetComponentByUsedBoneName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, boneIndex, 0 );
	FINISH_PARAMETERS;

	TDynArray< THandle<CComponent> > & resultArr = *(TDynArray< THandle<CComponent> >*) result;

	for ( ComponentIterator< CMeshTypeComponent > it( this ); it; ++it )
	{
		CMeshTypeComponent* component = *it;

		CMeshSkinningAttachment* attachment = Cast< CMeshSkinningAttachment >( component->GetTransformParent() );
		if( !attachment ) continue;

		const TDynArray< Int32 >& mappings = attachment->GetBoneMapping();
		for( Uint32 j = 0; j != mappings.Size(); j++ )
		{
			Int32 mapping = mappings[ j ];
			if( mapping == boneIndex )
			{
				resultArr.PushBackUnique( component );
				break;
			}
		}
	}
}

void CEntity::funcGetComponentsCountByClassName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, className, CName::NONE );
	FINISH_PARAMETERS;

	Uint32 count = 0;
	CClass* c = SRTTI::GetInstance().FindClass( className );
	if ( c != nullptr )
	{
		for ( BaseComponentIterator it( this, c ); it; ++it )
		{	
			count++;
		}
	}

	RETURN_INT( count );
}

void CEntity::funcRaiseEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	Bool ret = RaiseBehaviorEvent( eventName );

	RETURN_BOOL( ret );
}

void CEntity::funcRaiseForceEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	ACTION_START_TEST;

	Bool ret = RaiseBehaviorForceEvent( eventName );

	RETURN_BOOL( ret );
}

void CEntity::funcRaiseForceEventWithoutTestCheck( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = RaiseBehaviorForceEvent( eventName );

	RETURN_BOOL( ret );
}

void CEntity::funcRaiseEventWithoutTestCheck( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, eventName, CName::NONE );
    FINISH_PARAMETERS;

    Bool ret = RaiseBehaviorEvent( eventName );

    RETURN_BOOL( ret );
}

void CEntity::funcGetBoneIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, bone, CName::NONE );
	FINISH_PARAMETERS;

	Int32 boneIndex = -1;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		const ISkeletonDataProvider* provider = ac->QuerySkeletonDataProvider();
		if ( provider )
		{
			boneIndex = provider->FindBoneByName( bone );
		}
	}

	RETURN_INT( boneIndex );
}

void CEntity::funcGetBoneWorldMatrixByIndex( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, boneIndex, -1 );
	FINISH_PARAMETERS;

	Matrix boneWorldMatrix( Matrix::IDENTITY );

	if ( boneIndex != -1 )
	{
		GetSubObjectWorldMatrix( boneIndex, boneWorldMatrix );
	}

	RETURN_STRUCT( Matrix, boneWorldMatrix );
}

void CEntity::funcGetBoneReferenceMatrixMS( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Int32, boneIndex, -1 );
	FINISH_PARAMETERS;

	Matrix matrix( Matrix::IDENTITY );

	if ( boneIndex != -1 )
	{
		matrix = GetBoneReferenceMatrixMS( boneIndex );
	}

	RETURN_STRUCT( Matrix, matrix );
}

void CEntity::funcGetMoveTarget( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Vector target = Vector::ZERO_3D_POINT;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		target = ac->GetMoveTargetWorldSpace();
	}

	RETURN_STRUCT( Vector, target );
}

void CEntity::funcGetMoveHeading( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Float heading = 0.f;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac )
	{
		heading = ac->GetMoveHeadingWorldSpace();
	}

	RETURN_FLOAT( heading );
}

void CEntity::funcActivateBehaviorsSync( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;
	
	//ACTION_START_TEST;

	Bool ret = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		ret = ac->GetBehaviorStack()->ActivateBehaviorInstances( names );
	}
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance activation fail - '%ls'"), GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CEntity::funcPreloadBehaviorsToActivate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;

	Bool failed = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		for ( Uint32 i = 0; i < names.Size(); ++i )
		{
			const CName& name = names[i];
			if ( !ac->GetBehaviorStack()->IsGraphAvailable(name) )
			{
				// Currently all graphs are loaded at game startup!
				failed = true;
			}
		}
	}

	RETURN_BOOL( !failed )
}

void CEntity::funcActivateBehaviors( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, names, TDynArray< CName >() );
	FINISH_PARAMETERS;

	//ACTION_START_TEST;
	Bool ret = false;
	Bool failed = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		for ( Uint32 i = 0; i < names.Size(); ++i )
		{
			const CName& name = names[i];
			if ( !ac->GetBehaviorStack()->IsGraphAvailable( name ) )
			{
				// Currently all graphs are loaded at game startup!
				failed = true;
			}
		}

		ret = ac->GetBehaviorStack()->ActivateBehaviorInstances( names );
	}
	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance activation fail - '%ls'"), GetName().AsChar() );
	}
	RETURN_BOOL( ret && !failed );
}

void CEntity::funcAttachBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	
	//ACTION_START_TEST;

	Bool ret = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		ret = ac->GetBehaviorStack()->AttachBehaviorInstance( name );
	}

	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance attach error - '%ls' '%ls'"),
			name.AsString().AsChar(), GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CEntity::funcAttachBehaviorSync( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, name, CName::NONE );
    FINISH_PARAMETERS;

    ACTION_START_TEST;

    Bool ret = false;
    CAnimatedComponent* ac = GetRootAnimatedComponent();
    if ( ac && ac->GetBehaviorStack() )
    {
        ret = ac->GetBehaviorStack()->AttachBehaviorInstance( name );
    }
    if ( !ret )
    {
        BEH_ERROR( TXT("Script - Behavior instance attach error - '%ls' '%ls'"),
            name.AsString().AsChar(), GetName().AsChar() );
    }
    RETURN_BOOL( ret );
}

void CEntity::funcDetachBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	FINISH_PARAMETERS;
	Bool ret = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		ret = ac->GetBehaviorStack()->DetachBehaviorInstance( name );
	}

	if ( !ret )
	{
		BEH_ERROR( TXT("Script - Behavior instance detach error - '%ls' '%ls'"),
		name.AsString().AsChar(), GetName().AsChar() );
	}
	RETURN_BOOL( ret );
}

void CEntity::funcSetBehaviorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Float, var, 0.0f );
	GET_PARAMETER_OPT( Bool, inAllInstances, false );
	FINISH_PARAMETERS;
	
	Bool res = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		res = ac->GetBehaviorStack()->SetBehaviorVariable( varName, var, inAllInstances );
	}

	RETURN_BOOL( res );
}

void CEntity::funcGetBehaviorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Float, defVal, 0.0f );
	FINISH_PARAMETERS;

	RETURN_FLOAT( GetBehaviorFloatVariable( varName, defVal ) );
}

void CEntity::funcSetBehaviorVectorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	GET_PARAMETER( Vector, var, Vector::ZEROS );
	GET_PARAMETER_OPT( Bool, inAllInstances, false );
	FINISH_PARAMETERS;

	Bool res = false;
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		res = ac->GetBehaviorStack()->SetBehaviorVariable( varName, var, inAllInstances );
	}

	RETURN_BOOL( res );
}

void CEntity::funcGetBehaviorVectorVariable( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, varName, CName::NONE );
	FINISH_PARAMETERS;

	Vector var = Vector::ZEROS;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	if ( ac && ac->GetBehaviorStack() )
	{
		var = ac->GetBehaviorStack()->GetBehaviorVectorVariable( varName );
	}

	RETURN_STRUCT( Vector, var );
}

void CEntity::funcGetBehaviorGraphInstanceName( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_OPT( Uint32, index, 0 );
	FINISH_PARAMETERS;
	CName instanceName = CName::NONE;
	if ( CAnimatedComponent const * ac = GetRootAnimatedComponent() )
	{
		if ( CBehaviorGraphStack const * stack = ac->GetBehaviorStack() )
		{
			instanceName = stack->GetInstanceName( index );
		}
	}
	RETURN_NAME( instanceName );
}

void CEntity::funcGetRootAnimatedComponent( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	CAnimatedComponent* ac = GetRootAnimatedComponent();
	RETURN_OBJECT( ac );
}

void CEntity::funcPreloadEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false; 

	if ( m_template && effectName != CName::NONE )
	{
		ret = m_template->PreloadEffect( effectName );
	}
	RETURN_BOOL( ret );
}

void CEntity::funcPreloadEffectForAnimation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animName, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false; 

	if ( m_template && animName != CName::NONE )
	{
		ret = m_template->PreloadEffectForAnimation( animName );
	}
	RETURN_BOOL( ret );
}

void CEntity::funcPlayEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER_OPT( THandle< CNode >, targetNode, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( PlayEffect( effectName, CName::NONE, targetNode.Get() ) );
}

void CEntity::funcPlayEffectOnBone( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER( CName, boneName, CName::NONE );
	GET_PARAMETER_OPT( THandle< CNode >, targetNode, NULL );
	FINISH_PARAMETERS;

	RETURN_BOOL( PlayEffect( effectName, boneName, targetNode.Get() ) );
}

void CEntity::funcGetAutoEffect( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_NAME( m_autoPlayEffectName );
}

void CEntity::funcSetAutoEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	FINISH_PARAMETERS;

	if ( m_autoPlayEffectName != CName::NONE )
	{
		if ( m_autoPlayEffectName == effectName )
		{
			RETURN_BOOL( true );
			return;
		}
		StopEffect( m_autoPlayEffectName );
	}
	m_autoPlayEffectName = effectName;
	//
	// This method is called so m_autoPlayEffectName can be properly marked to be saved during next serialization.
	//
	OnSetAutoPlayEffectName();

	if ( effectName != CName::NONE )
	{
		RETURN_BOOL( PlayEffect( m_autoPlayEffectName ) );
	}
	else
	{
		RETURN_BOOL( true );
	}
}

void CEntity::funcStopEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectTag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( StopEffect( effectTag ) );
}

void CEntity::funcDestroyEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectTag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( DestroyEffect( effectTag ) );
}

void CEntity::funcStopAllEffects( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	StopAllEffects();
}

void CEntity::funcDestroyAllEffects( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	DestroyAllEffects();
}

void CEntity::funcHasEffect( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectTag, CName::NONE );
	FINISH_PARAMETERS;

	RETURN_BOOL( HasEffect( effectTag ) );
}

void CEntity::funcSetCollisionType( CScriptStackFrame& stack, void* result )
{
    GET_PARAMETER( CName, typeName, CName::NONE);
    FINISH_PARAMETERS;

    CAnimatedComponent* animComp = GetRootAnimatedComponent();
    /*
    CMovingPhysicalAgentComponent* physcomp = SafeCast<CMovingPhysicalAgentComponent>( animComp );

    if( physcomp )
    {
        physcomp->SetCollisionType(typeName);
    }*/
}

void CEntity::funcGetDisplayName( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_STRING( GetDisplayName() );
}

void CEntity::funcFade( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, fadeIn, true );
	FINISH_PARAMETERS;
	
	// Notify the drawable rendering proxy
	for ( CComponent* component : m_components )
	{
		component->SetAutoFade( fadeIn );
	}
}

void CEntity::funcWaitForEventProcessing( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, eventName, CName::NONE );
	GET_PARAMETER( Float, timeout, 10.0f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// In this function we do not make any changes to the behavior tree. We only need some information from it, so the test is not needed here.
	//ACTION_START_TEST;

	const CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		if ( ac->GetBehaviorStack()->IsEventProcessed( eventName ) )
		{
			RETURN_BOOL( true );
			return;
		}

		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime < timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}
	}
	
	SET_ERROR_STATE( this, TXT("Event ") + eventName.AsString() + TXT(" was not processed - timeout ") + ToString(timeout) );
	LOG_ENGINE( TXT("CEntity:WaitForEventProcessing: Event %s was not processed (timeout %f)."), eventName.AsString().AsChar(), timeout );

	RETURN_BOOL( false );
}

void CEntity::funcWaitForBehaviorNodeActivation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, activationName, CName::NONE );
	GET_PARAMETER( Float, timeout, 10.0f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// In this function we do not make any changes to the behavior tree. We only need some information from it, so the test is not needed here.
	//ACTION_START_TEST;

	const CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		if ( ac->GetBehaviorStack()->ActivationNotificationReceived( activationName ) )
		{
			RETURN_BOOL( true );
			return;
		}

		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime < timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}
	}

	SET_ERROR_STATE( this, TXT("Notification ") + activationName.AsString() + TXT(" was not occurred - timeout ") + ToString(timeout) );
	LOG_ENGINE( TXT("CEntity:WaitForBehaviorNodeActivation: Notification %s was not occurred (timeout %f)."), activationName.AsString().AsChar(), timeout );

	RETURN_BOOL( false );
}

void CEntity::funcWaitForBehaviorNodeDeactivation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, deactivationName, CName::NONE );
	GET_PARAMETER( Float, timeout, 10.0f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// In this function we do not make any changes to the behavior tree. We only need some information from it, so the test is not needed here.
	//ACTION_START_TEST;

	const CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac && ac->GetBehaviorStack() )
	{
		if ( ac->GetBehaviorStack()->DeactivationNotificationReceived( deactivationName ) )
		{
			RETURN_BOOL( true );
			return;
		}

		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime < timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}
	}

	SET_ERROR_STATE( this, TXT("Notification ") + deactivationName.AsString() + TXT(" was not occurred - timeout ") + ToString(timeout) );
	LOG_ENGINE( TXT("CEntity:WaitForBehaviorNodeDeactivation: Notification %s was not occurred (timeout %f)."), deactivationName.AsString().AsChar(), timeout );

	RETURN_BOOL( false );
}

void CEntity::funcWaitForAnimationEvent( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, animEventName, CName::NONE );
	GET_PARAMETER( Float, timeout, 10.0f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	// In this function we do not make any changes to the behavior tree. We only need some information from it, so the test is not needed here.
	//ACTION_START_TEST;

	const CAnimatedComponent* ac = GetRootAnimatedComponent();

	if ( ac )
	{
		if ( ac->HasAnimationEventOccurred( animEventName ) )
		{
			RETURN_BOOL( true );
			return;
		}

		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime < timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}
	}

	SET_ERROR_STATE( this, TXT("Animation event ") + animEventName.AsString() + TXT(" was not occurred - timeout ") + ToString(timeout) );
	LOG_ENGINE( TXT("CEntity:WaitForAnimationEvent: Animation even %s was not occurred (timeout %f)."), animEventName.AsString().AsChar(), timeout );

	RETURN_BOOL( false );
}

extern Bool GLatentFunctionStart;

void CEntity::funcActivateAndSyncBehavior( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, name, CName::NONE );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	//ACTION_START_TEST;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	CBehaviorGraphStack* behStack = ac ? ac->GetBehaviorStack() : NULL;

	if ( behStack )
	{
		if ( !behStack->IsGraphAvailable(name) )
		{
			RETURN_BOOL( false );
			return;
		}
        // If we're here, then it's the first tick after the resource was loaded.
		if ( behStack->ActivateAndSyncBehaviorInstances( name ) )
		{
			BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Wating for synchronization... - '%ls'"), GetName().AsChar() );
		}
		else
		{
			// Default
			Bool ret = behStack->ActivateBehaviorInstances( name );

			String resStr = ret ? TXT("Success") : TXT("Failure");

			BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Couldn't synchronize stack. Default activation was processed - '%ls' - '%ls'"), resStr.AsChar(), GetName().AsChar() );

			RETURN_BOOL( ret );
			return;
		}
		

		if ( !behStack->IsSynchronizing() )
		{
			LOG_ENGINE( TXT("CEntity:ActivateAndSyncBehaviors - Finished - '%ls'"), GetName().AsChar() );
			RETURN_BOOL( true );
			return;
		}

		// Timeout
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}

		BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Timeout '%f' - '%ls'"), timeout, GetName().AsChar() );
	}

	LOG_ENGINE( TXT("CEntity:ActivateAndSyncBehaviors - Fail - '%ls'"), GetName().AsChar() );
	RETURN_BOOL( false );
}

void CEntity::funcActivateAndSyncBehaviors( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( TDynArray< CName >, names, TDynArray< CName >() );
	GET_PARAMETER_OPT( Float, timeout, 10.f );
	FINISH_PARAMETERS;

	ASSERT( stack.m_thread );

	ACTION_START_TEST;

	CAnimatedComponent* ac = GetRootAnimatedComponent();
	CBehaviorGraphStack* behStack = ac ? ac->GetBehaviorStack() : NULL;

	if ( behStack )
	{
		// Starting
		if ( GLatentFunctionStart )
		{
			if ( behStack->ActivateAndSyncBehaviorInstances( names ) )
			{
				BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Wating for synchronization... - '%ls'"), GetName().AsChar() );
			}
			else
			{
				// Default
				Bool ret = behStack->ActivateBehaviorInstances( names );

				String resStr = ret ? TXT("Success") : TXT("Failure");

				BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Couldn't synchronize stack. Default activation was processed - '%ls' - '%ls'"), resStr.AsChar(), GetName().AsChar() );

				RETURN_BOOL( ret );
				return;
			}
		}

		if ( !behStack->IsSynchronizing() )
		{
			LOG_ENGINE( TXT("CEntity:ActivateAndSyncBehaviors - Finished - '%ls'"), GetName().AsChar() );
			RETURN_BOOL( true );
			return;
		}

		// Timeout
		const Float waitedTime = stack.m_thread->GetCurrentLatentTime();
		if ( waitedTime <= timeout )
		{
			stack.m_thread->ForceYield();
			return;
		}

		BEH_LOG( TXT("CEntity:ActivateAndSyncBehaviors - Timeout '%f' - '%ls'"), timeout, GetName().AsChar() );
	}

	LOG_ENGINE( TXT("CEntity:ActivateAndSyncBehaviors - Fail - '%ls'"), GetName().AsChar() );
	RETURN_BOOL( false );
}

void CEntity::funcBehaviorNodeDeactivationNotificationReceived( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, deactivationNotificationName, CName::NONE );
	FINISH_PARAMETERS;

	const CAnimatedComponent* root = GetRootAnimatedComponent();
	if ( !root )
	{
		ASSERT( root );
		RETURN_BOOL( false );
		return;
	}

	const CBehaviorGraphStack* behStack = root->GetBehaviorStack();
	if ( !behStack )
	{
		ASSERT( !behStack );
		RETURN_BOOL( false );
		return;
	}

	RETURN_BOOL( behStack->DeactivationNotificationReceived( deactivationNotificationName ) );
}

void CEntity::funcCreateAttachment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, parentHandle, NULL );
	GET_PARAMETER_OPT( CName, slot, CName::NONE );
	GET_PARAMETER_OPT( Vector, relativeLocation, Vector::ZERO_3D_POINT );
	GET_PARAMETER_OPT( EulerAngles, relativeRotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;
	
	Bool ret = CreateAttachmentImpl( parentHandle, slot, relativeLocation, relativeRotation );

	RETURN_BOOL( ret );
}

void CEntity::funcCreateAttachmentAtBoneWS( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CEntity >, parentHandle, NULL );
	GET_PARAMETER( CName, bone, CName::NONE );
	GET_PARAMETER( Vector, worldLocation, Vector::ZEROS );
	GET_PARAMETER( EulerAngles, worldRotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	Bool ret = CreateAttachmentAtBoneWSImpl( parentHandle, bone, worldLocation, worldRotation );

	RETURN_BOOL( ret );
}

void CEntity::funcBreakAttachment( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	
	RETURN_BOOL( BreakAttachment() );
}

void CEntity::funcHasAttachment( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	CEntity* parentEntity = GetTransformParent() ? Cast< CEntity >( GetTransformParent()->GetParent() ) : NULL;
	Bool ret = parentEntity != NULL;

	RETURN_BOOL( ret );
}

void CEntity::funcHasSlot( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slotName, CName::NONE );
	GET_PARAMETER_OPT( Bool, recursive, false );
	FINISH_PARAMETERS;

	Bool ret = false;
	if ( slotName != CName::NONE )
	{
		ret = ( GetEntityTemplate()->FindSlotByName( slotName, recursive ) != nullptr );
	}

	RETURN_BOOL( ret );
}

void CEntity::funcCreateChildAttachment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, childHandle, NULL );
	GET_PARAMETER_OPT( CName, slot, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;

	CNode* childNode = childHandle.Get();

	if( childNode )
	{
		ret = childNode->CallEvent( CNAME( OnCanCreateParentAttachment ), this ) != CR_EventFailed;
		if ( !ret )
		{
			RETURN_BOOL( false );
			return;
		}

		ret = CallEvent( CNAME( OnCanCreateChildAttachment ), childHandle ) != CR_EventFailed;
		if ( !ret )
		{
			RETURN_BOOL( false );
			return;
		}

		if ( slot != CName::NONE )
		{
			// Attach entity to slot

			if ( !GetEntityTemplate() )
			{
				WARN_ENGINE( TXT("Error in CreateAttachment - parent entity doesn't have entity template") );
				RETURN_BOOL( false );
				return;
			}

			const EntitySlot* entitySlot = GetEntityTemplate()->FindSlotByName( slot, true );
			if ( entitySlot )
			{
				ForceUpdateTransformNodeAndCommitChanges();

				HardAttachmentSpawnInfo ainfo;
				ainfo.m_parentSlotName = entitySlot->GetBoneName();
				ainfo.m_relativePosition = entitySlot->GetTransform().GetPosition();
				ainfo.m_relativeRotation = entitySlot->GetTransform().GetRotation();

				CComponent* parentComponent = FindComponent( entitySlot->GetComponentName() );
				if ( parentComponent )
				{
					IAttachment* att = parentComponent->Attach( childNode, ainfo );
					ret = att != NULL;
				}
				else
				{
					IAttachment* att = Attach( childNode, ainfo );
					ret = att != NULL;
				}
			}
			else
			{
				WARN_ENGINE( TXT("Error in CreateAttachment - parent entity doesn't have slot '%ls' in entity template"), slot.AsString().AsChar() );
				RETURN_BOOL( false );
				return;
			}
		}
		else
		{
			ForceUpdateTransformNodeAndCommitChanges();

			HardAttachmentSpawnInfo ainfo;

			IAttachment* att = Attach( childNode, ainfo );
			ret = att != NULL;
		}

		if ( ret )
		{
			childNode->CallEvent( CNAME( OnParentAttachmentCreated ), this );
			CallEvent( CNAME( OnChildAttachmentCreated ), childHandle );
		}
		else
		{
			WARN_ENGINE( TXT("Error in CreateAttachment - Couldn't create atachment between parent entity '%ls' and child entity '%ls'"), GetFriendlyName().AsChar(), childNode->GetFriendlyName().AsChar() );
		}
	}

	RETURN_BOOL( ret );
}

void CEntity::funcBreakChildAttachment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, childHandle, NULL );
	GET_PARAMETER_OPT( CName, slot, CName::NONE );
	FINISH_PARAMETERS;

	Bool ret = false;

	CNode* childNode = childHandle.Get();
	CNode* parent = 0;

	if ( slot != CName::NONE )
	{
		CNode* childNode = childHandle.Get();

		const EntitySlot* entitySlot = GetEntityTemplate()->FindSlotByName( slot, true );
		if ( entitySlot )
		{
			CComponent* parentComponent = FindComponent( entitySlot->GetComponentName() );
			if ( parentComponent )
			{
				parent = parentComponent;
			}
			else
			{
				parent = this;
			}
		}
	}
	else
	{
		parent = this;
	}
	const TList< IAttachment* >& attachments = parent->GetChildAttachments();
	for ( TList< IAttachment* >::const_iterator it=attachments.Begin(); it!=attachments.End(); ++it )
	{
		CHardAttachment* attachment = (*it)->ToHardAttachment();
		if( attachment->GetChild() == childNode )
		{
			ret = attachment->GetChild()->CallEvent( CNAME( OnCanBreakParentAttachment ), attachment->GetParent() ) != CR_EventFailed;
			if ( !ret )
			{
				RETURN_BOOL( false );
				return;
			}

			ret = attachment->GetParent()->CallEvent( CNAME( OnCanBreakChildAttachment ), attachment->GetChild() ) != CR_EventFailed;
			if ( !ret )
			{
				RETURN_BOOL( false );
				return;
			}

			attachment->Break();

			attachment->GetChild()->CallEvent( CNAME( OnParentAttachmentBroken ), attachment->GetParent() );
			attachment->GetParent()->CallEvent( CNAME( OnChildAttachmentBroken ), attachment->GetChild() );

			RETURN_BOOL( true );
			return;
		}
	}

	RETURN_BOOL( false );

}

void CEntity::funcHasChildAttachment( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle< CNode >, childHandle, NULL );
	FINISH_PARAMETERS;

	CNode* node = childHandle.Get();

	for ( TList< IAttachment* >::iterator it=m_childAttachments.Begin(); it!=m_childAttachments.End(); ++it )
	{
		CHardAttachment* attachment = (*it)->ToHardAttachment();
		if( attachment->GetChild() == node )
		{
			RETURN_BOOL( true );
			return;
		}
	}

	RETURN_BOOL( false );
}

void CEntity::funcCalcEntitySlotMatrix( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, slot, CName::NONE );
	GET_PARAMETER_REF( Matrix, slotMatrix, Matrix::IDENTITY );
	FINISH_PARAMETERS;

	const CEntityTemplate* templ = GetEntityTemplate();
	Bool foundSlot = false;

	if ( templ )
	{
		const EntitySlot* entitySlot = templ->FindSlotByName( slot, true );
		if ( entitySlot )
		{
			entitySlot->CalcMatrix( this, slotMatrix, NULL );
			foundSlot = true;
		}
	}

	RETURN_BOOL( foundSlot );
}

void CEntity::funcSetEffectIntensity( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER( Float, intensity, 0.0f );
	GET_PARAMETER_OPT( CName, specificComponentName, CName::NONE );
	GET_PARAMETER_OPT( CName, effectParameterName, CName::NONE );
	FINISH_PARAMETERS;

	SetEffectIntensity( effectName, intensity, specificComponentName, effectParameterName );

	RETURN_VOID();
}
void CEntity::funcSetKinematic( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, decision, false );
	FINISH_PARAMETERS;

	if ( CAnimatedComponent* component = GetRootAnimatedComponent() )
	{
		if ( CPhysicsRagdollWrapper* wrapper = component->GetRagdollPhysicsWrapper() )
		{
			wrapper->SwitchToKinematic( decision );
		}
	}

	RETURN_VOID();
}

void CEntity::funcSetStatic( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	if ( CAnimatedComponent* component = GetRootAnimatedComponent() )
	{
		if ( CPhysicsRagdollWrapper* wrapper = component->GetRagdollPhysicsWrapper() )
		{
			wrapper->SwitchToStatic( );
		}
	}

	RETURN_VOID();
}

void CEntity::funcIsRagdolled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool isRagdolled = false;

	if ( CAnimatedComponent* component = GetRootAnimatedComponent() )
	{
		isRagdolled = component->IsRagdolled();
	}

	RETURN_BOOL( isRagdolled );
}

void CEntity::funcIsStatic( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	Bool isStatic = false;

	if ( CAnimatedComponent* component = GetRootAnimatedComponent() )
	{
		isStatic = component->IsStatic();
	}

	RETURN_BOOL( isStatic );
}

void CEntity::funcIsEffectActive( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, effectName, CName::NONE );
	GET_PARAMETER_OPT( Bool, treatStoppingAsPlaying, true );
	FINISH_PARAMETERS;

	RETURN_BOOL( IsPlayingEffect( effectName, treatStoppingAsPlaying ) );
}

void CEntity::funcDuplicate( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( THandle<CLayer>, placeOnLayer, NULL );
	FINISH_PARAMETERS;

	CEntity* newEntity = Duplicate( placeOnLayer.Get() );
	newEntity->SetFlag( OF_ScriptCreated );

	RETURN_HANDLE( CEntity, newEntity );
}

void CEntity::funcSetHideInGame( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, hide, true );
	FINISH_PARAMETERS;

	SetHideInGame( hide, false, HR_Scripts );
}


void CEntity::funcGetGuidHash( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;

	RETURN_INT( static_cast< Int32 >( ::GetHash< CGUID >( m_guid ) ) );
}

void CEntity::funcCalcBoundingBox( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER_REF( Box, box, Box() );
	FINISH_PARAMETERS;

	box = CalcBoundingBox();

	RETURN_VOID();
}

void CEntity::funcHasTagInLayer( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( CName, tagName, CName::NONE );
	FINISH_PARAMETERS;

	CLayer* layer = GetLayer();
	if( layer )
	{
		CLayerInfo* layerInfo	= layer->GetLayerInfo();
		if( layerInfo )
		{
			RETURN_BOOL( layerInfo->GetTags().HasTag( tagName ) );
		}
	}
	else
	{
		RETURN_BOOL( false );
	}
}

void CEntity::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flags )
{
	TBaseClass::OnGenerateEditorFragments( frame, flags );

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_EntityVisibility ) )
	{
		const Box entityBox = CalcBoundingBox();

		const ERenderVisibilityResult result = GetLastFrameVisibility();
		if ( result == RVR_NotVisible )
		{
			frame->AddDebugBox( entityBox, Matrix::IDENTITY, Color::BLACK, false, true );
		}
		else if ( result == RVR_PartialyVisible )
		{
			frame->AddDebugBox( entityBox, Matrix::IDENTITY, Color::YELLOW, false, true );
		}
		else if ( result == RVR_Visible )
		{
			frame->AddDebugBox( entityBox, Matrix::IDENTITY, Color::RED, false, true );
		}		
	}

	if ( frame->GetFrameInfo().IsShowFlagOn( SHOW_LodInfo ) )
	{
		String text = String::EMPTY;
		Uint32 maxLod = NumericLimits< Uint32 >::Max();
		Int32 entityLod = -1;
		Color textCol = Color::WHITE;
		const Uint32 size = m_components.Size();

		if ( GetLayer() == nullptr || GetLayer()->GetWorld() == nullptr )
		{
			return;
		}

		entityLod = GetRootAnimatedComponent()? GetRootAnimatedComponent()->GetLod() : 0;

		for ( Uint32 i=0; i<size; ++i )
		{
			CMeshComponent* meshCmp = Cast<CMeshComponent>(m_components[i]);
			if ( meshCmp != nullptr )
			{
				CMesh* m = meshCmp->TryGetMesh();
				if ( m != nullptr )
				{
					// calculation. max available lod for entity from all components
					Uint32 maxAvailableLOD = m->GetNumLODLevels()-1;
					maxLod = Min(maxLod, maxAvailableLOD);
				}
			}
		}
		if ( maxLod < NumericLimits< Uint32 >::Max() )
		{
			if ( entityLod > (Int32)maxLod )
			{
				textCol = Color::RED;
			}
			else
			{
				textCol = Color::GREEN;
			}
			text = this->GetName();
			if ( text.Empty() )
			{
				text = this->GetUniqueName();
			}
			text += String::Printf( TXT("\nLOD: %i/%i"), entityLod, maxLod );
			frame->AddDebugText( GetWorldPosition(), text, true, textCol, Color::BLACK, nullptr );
		}
	}

	// Show streaming ranges
	if( IsSelected() && ShouldBeStreamed() )
	{
		frame->AddDebugSphere( GetWorldPosition(), (Float) GetStreamingDistance(), Matrix::IDENTITY, Color::WHITE );
	}
}

Bool CEntity::IsGameplayLODable()
{
	CAnimatedComponent* animatedComponent = GetRootAnimatedComponent();
	return animatedComponent ? animatedComponent->IsGameplayLODable() : true;
}

void CEntity::SetForceNoLOD(Bool enable)
{
	SetDynamicFlag( EEntityDynamicFlags::EDF_ForceNoLOD, enable );
}

void CEntity::DestroyFromPool()
{
	RED_FATAL_ASSERT( CheckEntityFlag( EF_Poolable ), "Entity is not Poolable. This function is for CEntityPool eyes only" );

	SetFlag( NF_Destroyed );
	DestroyEntityInternals();
	DestroyAllComponents();
	Uninitialize();

	// Destroy instantly
	{
		// Dispatch system wide event
		EDITOR_DISPATCH_EVENT( CNAME( Destroyed ), CreateEventData( this ) );

		// Destroy components
		const TDynArray< CComponent* >& components = GetComponents();
		for ( Uint32 j=0; j<components.Size(); ++j )
		{
			CComponent* entityComponent = components[ j ];
			if ( entityComponent )
			{
				ASSERT( !entityComponent->IsAttached() );
				ASSERT( entityComponent->GetParent() == this );
				entityComponent->OnDestroyed();
			}
		}

		// Discard entity object
		Discard();
	}
}

//////////////////////////////////////////////////////////////////////////

Bool CEntity::IsRenderingReady() const
{
	for ( ComponentIterator< CDrawableComponent > dc( this ); dc; ++dc )
	{
		if ( !(*dc)->IsRenderingReady() )
		{
			return false;
		}
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////

Bool CEntity::ExportExternalAppearance( const CName &name, CEntityExternalAppearance* entityExternalAppearance ) const
{	
	CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( GetTemplate() );
	if( entityTemplate )
	{
		if( entityTemplate->ExportAppearance( name, entityExternalAppearance->m_appearance ) )
		{
			for(CEntityTemplateParam* param : entityExternalAppearance->m_appearance.GetAppearanceParams())
			{
				if(param != nullptr)
				{
					param->SetParent(entityExternalAppearance);
				}
			}
			return true;
		}
	}

	return false;
}

//////////////////////////////////////////////////////////////////////////

Bool CEntity::ImportExternalAppearance( const CEntityExternalAppearance* entityExternalAppearance )
{
	CEntityTemplate* entityTemplate = Cast< CEntityTemplate >( GetTemplate() );
	if( entityTemplate )
	{
		const CName& name = entityExternalAppearance->m_appearance.GetName();
		const CEntityAppearance *appearance = entityTemplate->GetAppearance( name, false );
		if( appearance == nullptr )
		{
			if( entityTemplate->ImportAppearance( name, entityExternalAppearance->m_appearance ) )
			{
				return true;
			}
		}		
	}
	return false;
}
