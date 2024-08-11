/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "../core/gameSave.h"
#include "gameSaveManager.h"
#include "persistentEntity.h"
#include "layer.h"
#include "layerInfo.h"
#include "dynamicLayer.h"
#include "component.h"

IMPLEMENT_ENGINE_CLASS( CPeristentEntity );

#ifndef NO_SAVE_VERBOSITY
	CSaveClassStatsCollector GSaveStatsCollector;
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////

CPeristentEntity::CPeristentEntity()
	: m_isSaveable( true )
{
}

CPeristentEntity::~CPeristentEntity()
{
}

void CPeristentEntity::SetIdTag( const IdTag& tag )
{
	const IdTag oldTag = m_idTag;
	m_idTag = tag;

	if ( IsAttached() )
	{
		GetLayer()->GetWorld()->OnPersistentEntityIdTagChanged( this, oldTag );
	}
}

void CPeristentEntity::OnPostLoad()
{
	// Check if this entity should be saved
	SetShouldSave( CheckShouldSave() );

	TBaseClass::OnPostLoad();
}

void CPeristentEntity::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );
	world->RegisterPersistentEntity( this );
}

void CPeristentEntity::OnDetached( CWorld* world )
{
	world->UnregisterPersistentEntity( this );
	TBaseClass::OnDetached( world );
}

CPeristentEntity* CPeristentEntity::FindByIdTag( const IdTag& tag )
{
	CWorld* world = GGame->GetActiveWorld();
	return world ? world->FindPersistentEntity( tag ) : nullptr;
}

void CPeristentEntity::OnCreated( CLayer* layer, const EntitySpawnInfo& info )
{
    SetIdTag( ( GIsEditor && !info.m_idTag.IsValid() ) ? IdTag::AllocateStaticTag() : info.m_idTag );
	TBaseClass::OnCreated( layer, info );
}

void CPeristentEntity::OnPasted( CLayer* layer )
{
	TBaseClass::OnPasted( layer );
	SetIdTag( IdTag::AllocateStaticTag() );
}

void CPeristentEntity::OnLayerSavedInEditor()
{
	// Pass to base class
	TBaseClass::OnLayerSavedInEditor();

	// If this has invalid IdTag restore it
	if ( !m_idTag.IsValid() )
	{
		SetIdTag( IdTag::AllocateStaticTag() );
	}
}

void CPeristentEntity::OnSaveGameplayState( IGameSaver* saver )
{
	#ifndef NO_SAVE_VERBOSITY
		RED_LOG( Save, TXT("CPeristentEntity::OnSaveGameplayState(): class: %s"), GetClass()->GetName().AsChar() );
		GSaveStatsCollector.OnEntityClass( GetClass()->GetName() );
	#endif

	CGameSaverBlock block( saver, CNAME( Entity ) );

	SaveProperties( saver );
	SaveComponents( saver );	
}

void CPeristentEntity::OnLoadGameplayState( IGameLoader* loader )
{
	#ifndef NO_SAVE_VERBOSITY
		RED_LOG( Save, TXT("CPeristentEntity::OnLoadGameplayState(): %s"), GetFriendlyName().AsChar() );
	#endif

	Uint32 saveVersion = loader->GetSaveVersion();
	if ( saveVersion < SAVE_VERSION_WRITEPROPERTY )
	{
		if ( saveVersion < SAVE_VERSION_TRANSFERABLE_ENTITIES )
		{
			CGameSaverBlock block( loader, CNAME( eB ) );
		}	

		{
			CGameSaverBlock block( loader, CNAME( autoEffects ) );
			LoadAutoEffects( loader );
		}

		{
			CGameSaverBlock block( loader, CNAME( properties ) );
			LoadProperties( loader );
		}	

		{
			CGameSaverBlock block( loader, CNAME( cs ) );
			LoadComponents( loader );
		}
	}
	else 
	{
		CGameSaverBlock block( loader, CNAME( Entity ) );

		if ( saveVersion < SAVE_VERSION_DIRECT_STREAM_SAVES )
		{
			LoadAutoEffects( loader );
		}

		LoadProperties( loader );
		LoadComponents( loader );
	}

	SetShouldSave( true );
}

void CPeristentEntity::LoadAutoEffects( IGameLoader* loader )
{
	// DEPRECATED

	// Read boolean value, saying if there is auto effect name in the stream...
	Bool storeAutoEffectName = false;
	loader->ReadValue( CNAME(c), storeAutoEffectName );

	if ( storeAutoEffectName )
	{
		// Read auto effect name
		loader->ReadValue( CNAME( n ), m_autoPlayEffectName );
	}
}

void CPeristentEntity::SaveProperties( IGameSaver* saver )
{
	// Get properties to save
	const Uint32 maxNumProperties( 128 );
	Uint32 numPropertiesToSave( 0 );
	CProperty* propertiesToSave[ maxNumProperties ];
	CClass* thisClass = GetClass();

	if ( GetLayer()->IsA< CDynamicLayer >() ) // Optimize out properties of objects in dynamic layer
	{			
		CProperty* saveableProperties[ maxNumProperties ];
		const Uint32 numProperties = saver->GetSavableProperties( thisClass, saveableProperties, maxNumProperties );
		if ( numProperties >= maxNumProperties )
		{
			HALT( "IGameSaver::OnSaveGameplayState() reached maxNumProperties (=%ld) limit! Please increase this limit or reduce number of saveable properties in class %ls.", maxNumProperties, thisClass->GetName().AsChar() ); 
		}

		void* defaultObject = thisClass->GetDefaultObjectImp();
		for ( Uint32 i = 0; i < numProperties; ++i )
		{
			CProperty* prop = saveableProperties[ i ];

			// Skip property if it has the same value as in the default object
			if ( defaultObject )
			{
				const void* data = prop->GetOffsetPtr( this );
				const void* defaultData = prop->GetOffsetPtr( defaultObject );

				if ( prop->GetType()->Compare( data, defaultData, 0 ) )
				{
					continue;
				}
			}

			propertiesToSave[ numPropertiesToSave ] = prop;
			++numPropertiesToSave;
		}
	}
	else // Can't optimize static objects because their difference wrt. default object is meaningless - what matters is the difference wrt. in-editor-authored object state
	{
		numPropertiesToSave = saver->GetSavableProperties( thisClass, propertiesToSave, maxNumProperties );
		if ( numPropertiesToSave >= maxNumProperties )
		{
			HALT( "IGameSaver::OnSaveGameplayState() reached maxNumProperties (=%ld) limit! Please increase this limit or reduce number of saveable properties in class %ls.", maxNumProperties, GetClass()->GetName().AsChar() ); 
		}
	}

	// Save count
	saver->WriteValue( CNAME( nP ), numPropertiesToSave );

	// Save properties
	for ( Uint32 i = 0; i < numPropertiesToSave; ++i )
	{
		saver->WriteProperty( this, propertiesToSave[ i ] );
	}
}

void CPeristentEntity::LoadProperties( IGameLoader* loader )
{
	const Uint32 saveVersion = loader->GetSaveVersion();
	const Uint32 numProperties = loader->ReadValue< Uint32 >( CNAME( nP ) );

	if ( saveVersion < SAVE_VERSION_WRITEPROPERTY )
	{
		for ( Uint32 i=0; i<numProperties; i++ )
		{
			CGameSaverBlock block( loader, CNAME(p) );

			// Read type name
			CName propName;
			if ( loader->GetSaveVersion() < SAVE_VERSION_USE_HASH_NAMES_WHERE_SAFE )
			{
				propName = loader->ReadValue< CName >( CNAME( n ), CName::NONE );
			}
			else
			{
				Uint32 hash = loader->ReadValue< Uint32 >( CNAME( n ), 0 );
				propName = CName::CreateFromHash( Red::CNameHash( hash ) );
			}

			// Find property
			CProperty* prop = GetClass()->FindProperty( propName );
			if ( prop )
			{
				// Load directly
				void* destData = prop->GetOffsetPtr( this );
				loader->ReadValue( CNAME(v), prop->GetType(), destData, this );
			}
			else
			{
				extern Bool GDebugSaves;
				if ( GDebugSaves )
				{
					HALT( "Fuckup in save system" );
					LOG_ENGINE( TXT("CPeristentEntity::OnLoadGameplayState(): Property '%ls' not found in class '%ls' during load of '%ls'"), propName.AsString().AsChar(), GetClass()->GetName().AsString().AsChar(), GetFriendlyName().AsChar() );
				}
			}
		}
	}
	else
	{
		CClass* theClass = GetClass();
		for ( Uint32 i = 0; i < numProperties; ++i )
		{
			loader->ReadProperty( this, theClass, this );
		}
	}
}

CLayerStorage::EntityData* CPeristentEntity::GetEntityData( Bool createIfNull )
{
	if ( CLayer* layer = GetLayer() )
	{
		CLayerStorage* storage = layer->GetLayerStorage();
		return storage ? storage->GetEntityData( m_idTag, createIfNull ) : nullptr;
	}
	return nullptr;
}

void CPeristentEntity::SaveComponents( IGameSaver* saver )
{
	Uint32 numToSave = 0;

	for ( Uint32 i = 0; i < m_components.Size(); ++i )
	{
		if ( m_components[ i ]->ShouldSave() && !m_components[ i ]->CanBeSavedDirectlyToStream() && !m_components[ i ]->IsStreamed() )
		{
			++numToSave;
		}
	}

	// Store count
	saver->WriteValue( CNAME(nC), numToSave );

	// For each component let it store it's gameplay state
	for ( Uint32 i=0; i<m_components.Size(); ++i )
	{
		if ( m_components[ i ]->ShouldSave() )
		{
			CComponent* component = m_components[i];
			if ( !component->CanBeSavedDirectlyToStream() )
			{
				// if it's a streamable component save component data separately
				if ( component->IsStreamed() )
				{
					CLayerStorage::EntityData* entityData = GetEntityData( true );
					if ( entityData )
					{
						SaveStreamedComponent( component, entityData );
					}
				}
				// save component data with entity data
				else
				{
					CGameSaverBlock block( saver, CNAME(c) );
					// Save component class and name
					CComponent* component = m_components[i];
					saver->WriteValue( CNAME(n), component->GetName() );

					// Save component data
					component->OnSaveGameplayState( saver );

#ifndef NO_SAVE_VERBOSITY
					GSaveStatsCollector.OnComponentClass( component->GetClass()->GetName() );
#endif
				}
			}
		}
	}
}

void CPeristentEntity::SaveStreamedComponent( CComponent* component, CLayerStorage::EntityData* entityData )
{
	if ( !ShouldLoadAndSaveComponentsState() )
	{
		return;
	}
	IGameSaver* compSaver = entityData->GetComponentSaver( component->GetGUID() );
	if ( compSaver )
	{
		{
			CGameSaverBlock block( compSaver, CNAME( Component ) );
			component->OnSaveGameplayState( compSaver );
		}
		delete compSaver;
	}	
}

Bool CPeristentEntity::ShouldLoadAndSaveComponentsState() 
{ 
	if ( CLayer* layer = GetLayer() )
	{
		if ( CLayerInfo* layerInfo = layer->GetLayerInfo() )
		{
			return !layerInfo->IsEnvironment() && ( GIsGame || GIsEditorGame ); 
		}
	}
	return false;
}

void CPeristentEntity::LoadStreamedComponentsState()
{
	if ( !ShouldLoadAndSaveComponentsState() )
	{
		return;
	}
	CLayerStorage::EntityData* entityData = GetEntityData( false );
	if ( entityData && entityData->m_componentsData )
	{
		for ( auto it : m_streamingComponents )
		{
			IGameLoader* compLoader = entityData->GetComponentLoader( it->GetGUID() );
			if ( compLoader )
			{
				{
					CGameSaverBlock block( compLoader, CNAME( Component ) );
					it->OnLoadGameplayState( compLoader );
				}
				delete compLoader;
			}
		}
	}
}

void CPeristentEntity::SaveStreamedComponentsState()
{
	CLayerStorage::EntityData* entityData = GetEntityData( true );
	if ( entityData )
	{
		for ( auto it=m_streamingComponents.Begin(); it != m_streamingComponents.End(); ++it )
		{
			if ( !(*it).IsValid() )
			{
				continue;
			}
			CComponent* component = (*it).Get();
			if ( !component->ShouldSave() )
			{
				continue;
			}
			SaveStreamedComponent( component, entityData );
		}
	}
}

void CPeristentEntity::LoadComponents( IGameLoader* loader )
{
	// Read count
	const Uint32 numComponents = loader->ReadValue< Uint32 >( CNAME(nC) );

	// For each component let it load it's gameplay state
	for ( Uint32 i=0; i<numComponents; ++i )
	{
		CGameSaverBlock block( loader, CNAME(c) );

		// Read component class and name
		String componentName;
		CName componentClass;
		loader->ReadValue( CNAME(n), componentName );

		// Find component
		for ( Uint32 j=0; j<m_components.Size(); ++j )
		{
			CComponent* component = m_components[j];
			if ( component && component->GetName() == componentName )
			{
				// Load component data
				component->OnLoadGameplayState( loader );
				break;
			}
		}
	}
}

Bool CPeristentEntity::CheckShouldSave() const
{
	// Entity should be saved when:
	// - or it does have at least one saveable property
	// - or it does have at least one saveable component
	// - or it does have auto play effect to be saved

	if ( IGameSaver::TestForSavableProperties( GetClass() ) )
	{
		return true;
	}
 
	const TDynArray< CComponent* >& components = GetComponents();
	for ( const auto& component : components )
	{
		if ( component->ShouldSave() )
		{
			return true;
		}
	}

	CEntityTemplate* entityTemplate = GetEntityTemplate();
	if ( entityTemplate )
	{
		const CEntity* baseEntity = entityTemplate->GetEntityObject();
		if ( baseEntity )
		{
			const CName defaultAutoPlayEffectName = baseEntity->GetAutoPlayEffectName();
			if ( defaultAutoPlayEffectName != m_autoPlayEffectName )
			{
				return true;
			}
		}
	}

	return false;
}

void CPeristentEntity::SetShouldSave( Bool should )
{
	if ( should )
	{
		SetFlag( NF_ShouldSave );
	}
	else
	{
		ClearFlag( NF_ShouldSave );
	}
}

void CPeristentEntity::ConvertToManagedEntity( const IdTag& dynamicTag )
{
	SetEntityFlag( EF_ManagedEntity );
	SetIdTag( dynamicTag );

	CLayerStorage* const storage = GetLayer()->GetLayerStorage();
	if ( storage && GGame->IsActive() )
	{
		storage->RegisterManagedEntity( this );
	}
}

void CPeristentEntity::ForgetTheState()
{
	CLayerStorage* const storage = GetLayer()->GetLayerStorage();
	if ( storage )
	{
		if ( IsManaged() )
		{
			storage->UnregisterManagedEntity( this );
		}
		else
		{
			storage->RemoveEntityState( this );
		}
	}

	ClearEntityFlag( EF_ManagedEntity );

	IdTag zero;
	SetIdTag( zero );

	SetShouldSave( false );
}

void CPeristentEntity::OnSetAutoPlayEffectName()
{
	//
	// This code is executed after setting m_autoPlayEffectName in CEntity::funcSetAutoEffect( CScriptStackFrame& stack, void* result )
	// m_autoPlayEffectName will be always saved after it's first value got changed
	//
	SetShouldSave( true );
}

void CPeristentEntity::SaveState( IGameSaver* saver, ISaveFile* directStream )
{
	// non-stream code path
	OnSaveGameplayState( saver );

	// stream code path
	SaveStreamData( directStream );
}

void CPeristentEntity::SaveStreamData( ISaveFile* directStream )
{
	const CEntityTemplate* entityTemplate = GetEntityTemplate();
	Bool	storeAutoEffectName = false;

	// Check if auto effect is different
	if ( entityTemplate )
	{
		const CEntity* baseEntity = entityTemplate->GetEntityObject();
		if ( baseEntity )
		{
			const CName defaultAutoPlayEffectName = baseEntity->GetAutoPlayEffectName();
			if ( defaultAutoPlayEffectName != m_autoPlayEffectName )
			{
				// auto play effect has changed during gameplay, save it
				storeAutoEffectName = true;
			}
		}
	}

	*directStream << m_autoPlayEffectName;

	// compute the count of components that can be saved directly
	Uint8 num = 0;
	for ( const auto& component : m_components )
	{
		num += ( component->CanBeSavedDirectlyToStream() ) ? 1 : 0;
	}

	// save num
	*directStream << num;

	// save direct components
	for ( const auto& component : m_components )
	{
		if ( component->CanBeSavedDirectlyToStream() )
		{
			Uint32 nameHash = component->GetName().CalcHash();
			*directStream << nameHash;

			CName className = component->GetClass()->GetName();
			*directStream << className;

			component->StreamSave( directStream );
		}
	}
}

void CPeristentEntity::RestoreState( IGameLoader* loader, ISaveFile* directStream, Uint32 version )
{
	// non-stream code path
	OnLoadGameplayState( loader );

	if ( version >= SAVE_VERSION_DIRECT_STREAM_SAVES )
	{
		// stream code path
		LoadStreamData( directStream, version );
	}
}

void CPeristentEntity::LoadStreamData( ISaveFile* directStream, Uint32 version )
{
	if ( version >= SAVE_VERSION_STORE_AUTOPLAY_EFFECT_NAME_ALWAYS )
	{
		*directStream << m_autoPlayEffectName;
	}
	else
	{
		Bool storeAutoEffectName = false;
		*directStream << storeAutoEffectName;
		if ( storeAutoEffectName )
		{
			*directStream << m_autoPlayEffectName;
		}
	}

	Uint8 num = 0;
	*directStream << num;

	// direct components
	for ( Uint16 j = 0; j < num; ++j )
	{
		Uint32 nameHash = 0;
		*directStream << nameHash;

		CName className;
		*directStream << className;

		Bool foundComponent = false;
		for ( auto& component : m_components )
		{
			if ( component->CanBeSavedDirectlyToStream() && component->GetClass()->GetName() == className )
			{
				if ( component->GetName().CalcHash() == nameHash )
				{
					// this is it
					component->StreamLoad( directStream, version );
					foundComponent = true;
					break;
				}
			}
		}

		if ( foundComponent )
		{
			continue;
		}
		
		IRTTIType* type = SRTTI::GetInstance().FindType( className );
		if ( nullptr == type || type->GetType() != RT_Class || false == static_cast< CClass* > ( type )->IsA( CComponent::GetStaticClass() ) )
		{
			ASSERT( false, TXT("DO NOT REMOVE/RENAME COMPONENT CLASSES SAVED AS STREAM!") );
			break;
		}
			
		// we have a component saved in stream, we have no skip offset to use, but we don't have a component...
		// ok, instead of creating, loading, and trashing component here should be something like static FakeLoad() for each type that supports StreamLoad()
		// ...but that's still TODO ;) so i'm leaving this here for now.
		SComponentSpawnInfo info;
		CComponent* component = CreateComponent( static_cast< CClass* > ( type ), info );
		component->StreamLoad( directStream, version );
		DestroyComponent( component );
	}
}
