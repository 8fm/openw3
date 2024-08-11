/**
* Copyright © 2007 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "component.h"
#include "renderCommands.h"

#include "hitProxyMap.h"
#include "hitProxyObject.h"

#include "externalProxy.h"
#include "attachment.h"
#include "renderProxy.h"
#include "renderProxyIterator.h"
#include "persistentEntity.h"
#include "layer.h"
#include "world.h"
#include "dynamicLayer.h"
#include "animatedComponent.h"
#include "tickManager.h"

#include "../core/scriptStackFrame.h"
#include "../core/gameSave.h"
#include "../core/fileSkipableBlock.h"

RED_DEFINE_STATIC_NAME( graphPositionX );
RED_DEFINE_STATIC_NAME( graphPositionY );
RED_DEFINE_STATIC_NAME( isStreamed );
RED_DEFINE_STATIC_NAME( CMovingAgentComponent );

//////////////////////////////////////////////////////////////////////////

CComponentTickProxy::CComponentTickProxy( CComponent* component )
	: m_component( component )
	, m_totalRecentTickTime( 0.0f )
	, m_accumulatedDeltaTime( 0.0f )
	, m_tickMask( 0 )
	, m_tickSuppressionMask( 0 )
	, m_tickBudgetingMask( 0 )
{
	static CClass* macClass = SRTTI::GetInstance().FindClass( CNAME( CMovingAgentComponent ) );
	m_useImmediateJobs_MainPass = component->IsA< CAnimatedComponent >();
	m_useImmediateJobs_ActorPass = component->IsA( macClass );
}

//////////////////////////////////////////////////////////////////////////
// CComponent

IMPLEMENT_ENGINE_CLASS( CComponent )

CComponent::CComponent()
	: m_nextAttachedComponent( NULL )
	, m_prevAttachedComponent( NULL )
	, m_tickProxy( nullptr )
	, m_isStreamed( false )
	, m_componentFlags( 0 )
{
}

CComponent::~CComponent()
{
	RED_FATAL_ASSERT( !m_prevAttachedComponent, "Component was destroyed while still being attached to component list. Crashes will follow." );
	RED_FATAL_ASSERT( !m_nextAttachedComponent, "Component was destroyed while still being attached to component list. Crashes will follow." );
}

void CComponent::SuppressTick( Bool suppress, ETickSuppressReason reason )
{
	GetWorld()->GetTickManager()->Suppress( this, suppress, reason );
}

void CComponent::SetTickBudgeted( Bool enable, ETickBudgetingReason reason )
{
	GetWorld()->GetTickManager()->SetBudgeted( this, enable, reason );
}

void CComponent::Destroy()
{
	// Break all parent attachments
	BreakAllAttachments();

	GetEntity()->DestroyComponent( this );
}

void CComponent::OnPropertyExternalChanged( const CName& propertyName )
{
	TBaseClass::OnPropertyExternalChanged( propertyName );

	if( propertyName == CNAME( transform ) )
	{
		CheckUpdateTransformMode();
		ScheduleUpdateTransformNode();
	}
}

IRenderProxyInterface* CComponent::QueryRenderProxyInterface()
{
	return NULL;
}

const IRenderProxyInterface* CComponent::QueryRenderProxyInterface() const
{
	return NULL;
}

void CComponent::OnFinalize()
{
	// Pass to base class
	TBaseClass::OnFinalize();

	// WTF is that here for ?
	if ( !GIsCooker )
	{
		// TODO: this has some strange dependency over Darek's code
		BreakAllAttachments();
	}

	// Make sure we are not in the component list
	/*
	ASSERT( !m_prevAttachedComponent );
	ASSERT( !m_nextAttachedComponent );
	*/

	// The comment above was a lie, it did nothing!
	// Now *really* make sure we are not in the component list...
	if ( m_prevAttachedComponent || m_nextAttachedComponent )
	{
		UnlinkFromAttachedComponentsList();
	}

	// Make sure we are not in any tick group
	ASSERT( !GetTickMask() );
}

void CComponent::OnSerialize( IFile& file )
{
	// Restore GUID
	if ( file.IsWriter() && GetGUID().IsZero() && !IsA<CExternalProxyComponent>() )
	{
		SetGUID( CGUID::Create() );
	}

	TBaseClass::OnSerialize( file );

	// Restore GUID
	if ( file.IsReader() && GetGUID().IsZero() && !IsA<CExternalProxyComponent>() )
	{
		WARN_ENGINE( TXT("Loaded component '%ls' with no GUID. Restoring."), GetFriendlyName().AsChar() );

		SetGUID( CGUID::Create() );

		RED_ASSERT( !GetGUID().IsZero(), TXT("Failed to generate guid for '%ls'"), GetFriendlyName().AsChar() );
	}
}

void CComponent::OnSpawned( const SComponentSpawnInfo& spawnInfo )
{
	SetPosition( spawnInfo.m_spawnPosition );
	SetRotation( spawnInfo.m_spawnRotation );
	SetScale( spawnInfo.m_spawnScale );
}

void CComponent::BreakAllAttachments()
{
	// Break all parent attachments
	{
		TList< IAttachment* > list = m_parentAttachments;
		for ( TList< IAttachment* >::iterator it=list.Begin(); it!=list.End(); ++it )
		{
			IAttachment* attachment = *it;
			if ( attachment )
			{
				attachment->OnComponentDestroyed( this );
			}
		}
	}

	// Break all child attachments
	{
		TList< IAttachment* > list = m_childAttachments;
		for ( TList< IAttachment* >::iterator it=list.Begin(); it!=list.End(); ++it )
		{
			IAttachment* attachment = *it;
			if ( attachment )
			{
				attachment->OnComponentDestroyed( this );
			}
		}
	}

	// Attachment list should be empty
	ASSERT( m_parentAttachments.Empty() );
	ASSERT( m_childAttachments.Empty() );
	ASSERT( m_transformParent == NULL );
}

void CComponent::OnDestroyed()
{
	ASSERT( (!m_prevAttachedComponent && !m_nextAttachedComponent), TXT("Components are still attached. Check why they're still attached") );

	// Unlink from handle system
	DiscardHandles();

	// Set the destroyed flag
	SetFlag( NF_Destroyed );
}

EAttachmentGroup CComponent::GetAttachGroup() const
{
	return ATTACH_GROUP_0;
}

CLayer* CComponent::GetLayer() const
{
	CEntity* entity = GetEntity();
	if ( entity )
	{
		return entity->GetLayer();
	}
	else
	{
		return NULL;
	}
}

#ifndef NO_COMPONENT_GRAPH

void CComponent::GetGraphPosition( Int32& x, Int32& y ) const
{
	x = m_graphPositionX;
	y = m_graphPositionY;
}

void CComponent::SetGraphPosition( Int32 x, Int32 y )
{
	m_graphPositionX = (Int16) Clamp< Int32 >( x, -32000, 32000 );
	m_graphPositionY = (Int16) Clamp< Int32 >( y, -32000, 32000 );
}

#endif

#ifndef NO_EDITOR
void CComponent::OnNavigationCook( CWorld* world, CNavigationCookingContext* context )
{

}
void CComponent::OnNavigationCookerInitialization( CWorld* world, CNavigationCookingContext* context )
{

}
void CComponent::PostNavigationCook( CWorld* world )
{

}
Bool CComponent::RemoveOnCookedBuild()
{
	return false;
}

#endif

void CComponent::OnInitialized()
{
	//ASSERT( IsInitializing() );
	ASSERT( !IsAttached() );
	ASSERT( !HasFlag( OF_Discarded ) );
	ASSERT( !HasFlag( OF_Finalized ) );
	m_objectFlags |= NF_Initialized;
}

void CComponent::OnPostComponentInitializedAsync()
{
	
}

void CComponent::OnUninitialized()
{
	//ASSERT( IsUninitializing() );
	ASSERT( !IsAttached() );
	ASSERT( !HasFlag( OF_Discarded ) );
	ASSERT( !HasFlag( OF_Finalized ) );
	m_objectFlags &= ~NF_Initialized;
}

void CComponent::Initialize()
{
	ASSERT( !IsInitialized() );
	//ASSERT( !IsUninitializing() );
	ASSERT( !IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );
	//m_objectFlags |= NF_Initializing;

	OnInitialized();

	ASSERT( IsInitialized() );
	//ASSERT( IsInitializing() );
	//m_objectFlags &= ~NF_Initializing;
}

void CComponent::Uninitialize()
{
	ASSERT( IsInitialized() );
	//ASSERT( !IsInitializing() );
	ASSERT( !IsAttached() );
	ASSERT( !IsAttaching() );
	ASSERT( !IsDetaching() );
	//m_objectFlags |= NF_Uninitializing;

	OnUninitialized();

	ASSERT( !IsInitialized() );
	//ASSERT( IsUninitializing() );
	//m_objectFlags &= ~NF_Uninitializing;
}

void CComponent::PerformAttachment( CWorld* world )
{
	// Component should be initialized before it can be attached
	RED_ASSERT( IsInitialized(), TXT("Trying to attach UNINITIALIZED component '%ls'"), GetFriendlyName().AsChar() );

	// Attach only if not already attached
	if ( !IsAttached() )
	{	
		// Attach component
		AttachToWorld( world );
		RED_ASSERT( IsAttached(), TXT("OnAttach signal not propagated correctly in component '%ls'"), GetFriendlyName().AsChar() );

		// Count components attached to layer
		CLayer* layer = GetLayer();
		RED_ASSERT( layer != nullptr, TXT("Trying to attach component '%ls' that is not on any layer"), GetFriendlyName().AsChar() );
		if ( layer != nullptr )
			layer->UpdateAttachedComponentsCount( 1 );

		// Link to list of all attached components
		LinkToAttachedComponentsList( world->m_allAttachedComponents );

		// Validate that entity exists
		CEntity *parentEntity = GetEntity();
		RED_ASSERT( parentEntity != nullptr, TXT("Trying to attach component '%ls' that is not in any entity"), GetFriendlyName().AsChar() );

		// Refresh component transform
		// TODO: this is causing a huge performance hit
		if ( UsesAutoUpdateTransform() )
		{
			ScheduleUpdateTransformNode();
		}

		// Dispatch system wide event
		EDITOR_DISPATCH_EVENT( CNAME( Attached ), CreateEventData( this ) );
	}
}

void CComponent::PerformDetachment( CWorld* world )
{
	// Component should be attached
	if ( IsAttached() )
	{
		// Unlink from internal list of all attached components
		UnlinkFromAttachedComponentsList();

		// Detach
		DetachFromWorld( world );
		RED_ASSERT( !IsAttached(), TXT("OnDetach signal not propagated correctly in component '%ls'"), GetFriendlyName().AsChar() );

		// Count components attached to layer
		CLayer* layer = GetLayer();
		RED_ASSERT( layer != nullptr, TXT("Trying to attach component '%ls' that is not on any layer"), GetFriendlyName().AsChar() );
		if ( layer != nullptr )
			layer->UpdateAttachedComponentsCount( -1 );

		// Unlink from handle system
		DiscardHandles();

		// Dispatch system wide event
		EDITOR_DISPATCH_EVENT( CNAME( Detached ), CreateEventData( this ) );
	}
}

void CComponent::PerformUninitialization()
{
	if ( IsInitialized() )
	{
		RED_ASSERT( !IsAttached(), TXT("Trying to unitialize ATTACHED component '%ls'"), GetFriendlyName().AsChar() );

		Uninitialize();
		RED_ASSERT( !IsInitialized(), TXT("Uninitialize signal not propagated correctly in component '%ls'"), GetFriendlyName().AsChar() );
	}
}

void CComponent::PerformInitialization()
{
	PC_SCOPE_PIX(CComponent_PerformInitialization);
	if ( !IsInitialized() )
	{
		RED_ASSERT( !IsAttached(), TXT("Trying to ininitialize ATTACHED component '%ls'"), GetFriendlyName().AsChar() );

		Initialize();
		RED_ASSERT( IsInitialized(), TXT("Initialize signal not propagated correctly in component '%ls'"), GetFriendlyName().AsChar() );
	}
}

void CComponent::PerformFullRecreation()
{
	// 1. detach if attached
	CWorld* worldWeWereAttachedTo = nullptr;
	if ( IsAttached() )
	{
		worldWeWereAttachedTo = GetWorld();
		RED_ASSERT( worldWeWereAttachedTo != nullptr, TXT("Component '%ls' is attached but there's no world"), GetFriendlyName().AsChar() );

		if ( worldWeWereAttachedTo )
		{
			PerformDetachment( worldWeWereAttachedTo );
		}
	}

	// 2. reinitialize if initialized
	if ( IsInitialized() )
	{
		PerformUninitialization();
		PerformInitialization();
	}

	// 3. attach back to world
	if ( worldWeWereAttachedTo )
	{
		PerformAttachment( worldWeWereAttachedTo );
	}	
}

void CComponent::OnAttached( CWorld* world )
{
	CNode::OnAttached( world );

	ASSERT( IsAttaching() );
	ASSERT( !IsAttached() );
	ASSERT( !HasFlag( OF_Discarded ) );
	ASSERT( !HasFlag( OF_Finalized ) );
	m_objectFlags |= NF_Attached;

	ASSERT( 0 == ( m_objectFlags & NF_WasAttachedInGame ) );

	if ( GGame->IsActive() )
	{
		m_objectFlags |= NF_WasAttachedInGame;
	}
}

void CComponent::OnDetached( CWorld* world )
{
	CNode::OnDetached( world );

	ASSERT( IsDetaching() );
	ASSERT( IsAttached() );
	ASSERT( !HasFlag( OF_Discarded ) );
	ASSERT( !HasFlag( OF_Finalized ) );
	m_objectFlags &= ~NF_Attached;

	//world->GetUpdateTransformManager().UnscheduleComponent( this );

	// fix for UT problems
	ClearFlag( NF_ScheduledUpdateTransform );
	ClearFlag( NF_MarkUpdateTransform );

	// Check that in game attachment and detachment is symmetric
	// const Bool wasInGame = 0 != ( m_objectFlags & NF_WasAttachedInGame );
	m_objectFlags &= ~NF_WasAttachedInGame;

	// Assertions
	/*if ( GGame->IsActive() != wasInGame )
	{
		if ( wasInGame )
		{
			WARN_ENGINE( TXT("Component '%ls' was in game when it was attached but on detach the game is NOT active. Fix this."), GetFriendlyName().AsChar() );
		}
		else
		{
			WARN_ENGINE( TXT("Component '%ls' was NOT in game when it was attached but on detach the game is active. Fix this."), GetFriendlyName().AsChar() );
		}
	}*/

	if ( m_tickProxy )
	{
		delete m_tickProxy;
		m_tickProxy = nullptr;
	}
}

void CComponent::AttachToWorld( CWorld* world )
{
	RED_ASSERT( SIsMainThread() );

	// Attach, with lots of sanity checks
	ASSERT( IsInitialized() );
	//ASSERT( !IsInitializing() );
	//ASSERT( !IsUninitializing() );
	ASSERT( !IsAttached() );
	ASSERT( !IsDetaching() );
	m_objectFlags |= NF_Attaching;
	OnAttached( world );
	ASSERT( IsAttached() );
	ASSERT( IsAttaching() );
	m_objectFlags &= ~NF_Attaching;
}

void CComponent::DetachFromWorld( CWorld* world )
{
	RED_ASSERT( SIsMainThread() );

	// Detach, with lots of sanity checks
	ASSERT( IsInitialized() );
	//ASSERT( !IsInitializing() );
	//ASSERT( !IsUninitializing() );
	ASSERT( IsAttached() );
	ASSERT( !IsAttaching() );
	m_objectFlags |= NF_Detaching;
	OnDetached( world );
	ASSERT( !IsAttached() );
	ASSERT( IsDetaching() );
	m_objectFlags &= ~NF_Detaching;
}

void CComponent::OnTickPrePhysics( Float timeDelta )
{

}

void CComponent::OnTickPrePhysicsPost( Float timeDelta )
{

}

void CComponent::OnTick( Float timeDelta )
{
}

void CComponent::OnTickPostPhysics( Float timeDelta )
{
}

void CComponent::OnTickPostPhysicsPost( Float timeDelta )
{
}

void CComponent::OnTickPostUpdateTransform( Float timeDelta )
{
}

void CComponent::OnUpdateTransformNode( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	OnUpdateTransformComponent( context, prevLocalToWorld );
}

void CComponent::OnUpdateTransformComponent( SUpdateTransformContext& context, const Matrix& prevLocalToWorld )
{
	
}

#ifndef NO_EDITOR_FRAGMENTS
void CComponent::OnGenerateEditorFragments( CRenderFrame* frame, EShowFlags flag )
{
	TBaseClass::OnGenerateEditorFragments( frame, flag );
}
#endif

void CComponent::OnGenerateEditorHitProxies( CHitProxyMap& map )
{

#ifndef NO_COMPONENT_GRAPH

	// Register simple hit proxy object for this component
	m_hitProxyId = map.RegisterHitProxy( new CHitProxyObject( this ) );

#endif

}

void CComponent::OnSaveGameplayState( IGameSaver* saver )
{
	#ifndef NO_SAVE_VERBOSITY
		static Uint32 cnt = 0;
		RED_LOG( Save, TXT("Saving component %ld: %s"), cnt++, GetClass()->GetName().AsChar() ); 
	#endif

	// Store the enable flag for component
	const Bool isEnabled = IsEnabled();
	saver->WriteValue( CNAME(i), isEnabled );
}

void CComponent::OnLoadGameplayState( IGameLoader* loader )
{
	// Restore the enable flag for component
	const Bool isEnabled = loader->ReadValue< Bool >( CNAME(i) );
	SetEnabled( isEnabled );
	SetShouldSave( true );
}

void CComponent::OnPostInstanced()
{
	TBaseClass::OnPostInstanced();

	SetShouldSave( CheckShouldSave() );
}

PathLib::IComponent* CComponent::AsPathLibComponent()
{
	return nullptr;
}
Bool CComponent::IsEnabled() const
{
	return true;
}

void CComponent::SetEnabled( Bool enabled )
{
	// By default this has no effect
	//WARN_ENGINE( TXT("Calling SetEnabled(%s) on '%ls' has no effect"), enabled ? TXT("true") : TXT("false"), GetFriendlyName().AsChar() );
}

void CComponent::ChangeParent(CEntity* newParent)
{
	// Break all parent attachments
	while( m_parentAttachments.Size() )
	{
		( *m_parentAttachments.Begin() )->Break();
	}
	// Set new parent
	SetParent(newParent);
}


void CComponent::SetAutoFade( Bool fadeIn )
{
	if ( IsAttached() && GetLayer() )
	{
		if( GetEntity() && GetEntity()->GetLastFrameVisibility() != RVR_NotVisible )
		{
			CWorld* world = GetLayer()->GetWorld();
			if ( world && world->GetRenderSceneEx() )
			{
				IRenderProxyInterface* renderInterface = QueryRenderProxyInterface();
				if ( renderInterface )
				{		
					const EFadeType componentFadeType = fadeIn ? FT_FadeIn : FT_FadeOut;
					for ( RenderProxyIterator it( renderInterface, RPT_None ); it; ++it )
					{
						( new CRenderCommand_SetAutoFade( world->GetRenderSceneEx(), *it, componentFadeType ) )->Commit();
					}
				}
			}
		}		
	}
}

void CComponent::GetAllChildAttachedComponents( TDynArray< CComponent* > &components ) const
{
	for ( TList<IAttachment*>::const_iterator it = m_childAttachments.Begin(); it != m_childAttachments.End(); ++it )
	{
		// Add component, only once
		CComponent* comp = Cast< CComponent >( (*it)->GetChild() );
		if ( comp )
		{
			components.PushBackUnique( comp );

			// Recurse
			comp->GetAllChildAttachedComponents( components );
		}
	}
}

void CComponent::LinkToAttachedComponentsList( CComponent*& list )
{
	m_nextAttachedComponent = list;
	if ( list ) list->m_prevAttachedComponent = &m_nextAttachedComponent;
	m_prevAttachedComponent = &list;
	list = this;
}

void CComponent::UnlinkFromAttachedComponentsList()
{
	if ( m_nextAttachedComponent )
	{
		m_nextAttachedComponent->m_prevAttachedComponent = m_prevAttachedComponent;
	}

	if ( m_prevAttachedComponent )
	{
		*m_prevAttachedComponent = m_nextAttachedComponent;
	}

	m_nextAttachedComponent = NULL;
	m_prevAttachedComponent = NULL;
}

Bool CComponent::ShouldWriteToDisk() const
{
	// Yes, write the component to disk
	return true;
}

Bool CComponent::HasInstanceProperties() const
{
	TDynArray< CProperty* > modifiedProperties;
	CollectModifiedInstanceProperties( modifiedProperties );
	return !modifiedProperties.Empty();
}

void CComponent::SaveInstanceProperties( IFile& file ) const
{
	// Collect the modified properties
	TDynArray< CProperty* > modifiedProperties;
	CollectModifiedInstanceProperties( modifiedProperties );

	// Save the number of properties
	Uint32 count = modifiedProperties.Size();
	file << count;

	// Save the properties themselves
	for ( auto it=modifiedProperties.Begin(); it != modifiedProperties.End(); ++it )
	{
		CFileSkipableBlock block( file );
		CProperty* prop = *it;

		// Save the type and name of the property
		CName propertyType = prop->GetType()->GetName();
		CName propertyName = prop->GetName();
		file << propertyType;
		file << propertyName;

		// Ask the type to serialize the property's value (const cast because Serialize figures out at runtime that we're writing)
		prop->GetType()->Serialize( file, prop->GetOffsetPtr( const_cast<CComponent*>( this ) ) );
	}
}

void CComponent::LoadInstanceProperties( IFile& file )
{
#ifndef RED_FINAL_BUILD
	// For non-final builds make sure the loaded properties are still in the instance properties list
	TDynArray< CName > instancePropertyNames;
	GetInstancePropertyNames( instancePropertyNames );

	// if the entity doesn't have a template then the overrides should be burned into it already thus false is a good default value
	Bool isCooked = false;
	if ( GetEntity() != nullptr && GetEntity()->GetEntityTemplate() != nullptr )
	{
		isCooked = GetEntity()->GetEntityTemplate()->IsCooked();
	}
#endif

	// Load the number of properties
	Uint32 count = 0;
	file << count;

	// Load the properties
	for ( Uint32 i=0; i < count; ++i )
	{
		CFileSkipableBlock block( file );

		// Load the type and name of the property
		CName propertyType;
		CName propertyName;
		file << propertyType;
		file << propertyName;

		// Attempt to find the property
		CProperty* prop = nullptr;
		if ( !propertyType.Empty() && !propertyName.Empty() 
#ifndef RED_FINAL_BUILD
			// this check shouldn't happen in cooked data because otherwise we are not going to load 
			// any instance properties because the instance property names are not cooked
			&& ( instancePropertyNames.Exist( propertyName ) || isCooked )
#endif
			)
		{
			prop = GetClass()->FindProperty( propertyName );

			// We found the property
			if ( prop != nullptr && prop->IsSerializable() )
			{
				// Check if it is of the proper type
				if ( prop->GetType()->GetName() == propertyType )
				{
					// Read the value
					prop->GetType()->Serialize( file, prop->GetOffsetPtr( this ) );
				}
				else // wrong type, try to load anyway
				{
					CVariant value( propertyType, nullptr );
					if ( value.IsValid() )
					{
						// Load the value
						value.GetRTTIType()->Serialize( file, value.GetData() );

						// Inform the component that we've got the property but of a different type
						OnPropertyTypeMismatch( propertyName, prop, value );
					}
				}
			}
			else // property not found, try to load and inform the component
			{
				CVariant value( propertyType, nullptr );
				if ( value.IsValid() )
				{
					// Load the value
					value.GetRTTIType()->Serialize( file, value.GetData() );

					// Inform the component that we've lost the property
					OnPropertyMissing( propertyName, value );
				}
			}
		}

		// Make sure we are at the proper position in the file (in case the property data was removed or wrong)
		block.Skip();
	}
}

static CComponent* FindComponentInTemplateHierarchy( CEntityTemplate* tpl, const CName& name, const CName& className )
{
	// Make sure the template has an entity object
	if ( tpl->GetEntityObject() == nullptr )
	{
		return nullptr;
	}

	// Scan the template
	CComponent* component = tpl->GetEntityObject()->FindComponent( name, className );
	if ( component != nullptr )
	{
		return component;
	}

	// Scan the template's includes
	const TDynArray< THandle< CEntityTemplate > >& includes = tpl->GetIncludes();
	for ( auto it=includes.Begin(); it != includes.End(); ++it )
	{
		CEntityTemplate* inctpl = (*it).Get();
		if ( inctpl != nullptr )
		{
			component = FindComponentInTemplateHierarchy( inctpl, name, className );
			if ( component != nullptr )
			{
				return component;
			}
		}
	}

	// Fail
	return nullptr;
}

void CComponent::CollectModifiedInstanceProperties( TDynArray< CProperty* >& modifiedProperties ) const
{
	// We need an entity to get the template from
	if ( GetEntity() == nullptr )
	{
		return;
	}

	// Get the entity template from which this component originates
	CEntityTemplate* entityTemplate = GetEntity()->GetEntityTemplate();
	if ( entityTemplate == nullptr )
	{
		return;
	}

	// Try to find this component in the template
	CComponent* originalComponent = FindComponentInTemplateHierarchy( entityTemplate, CName( GetName() ), GetClass()->GetName() );
	if ( originalComponent == nullptr )
	{
		return;
	}

	// Get the instance property names for this component
	TDynArray<CName> instancePropertyNames;
	GetInstancePropertyNames( instancePropertyNames );

	// Scan the properties to see if they have been modified from the template
	for ( auto it=instancePropertyNames.Begin(); it != instancePropertyNames.End(); ++it )
	{
		// Find the property
		const CName& propertyName = *it;
		CProperty* prop = GetClass()->FindProperty( propertyName );

		if ( prop != nullptr )
		{
			ASSERT( prop, TXT("GetInstancePropertyNames() includes a name for a non-existent property") );
			ASSERT( prop->IsSerializable(), TXT("GetInstancePropertyNames() includes a name for a non-serializable property") );

			// Get the property value data pointers
			const void* originalProperty = prop->GetOffsetPtr( originalComponent );
			const void* instanceProperty = prop->GetOffsetPtr( this );

			// Compare the property values (NOTE: consider using ComparePropertyValues from entityTemplate.cpp for more robust results)
			if ( !prop->GetType()->Compare( originalProperty, instanceProperty, 0 ) )
			{
				modifiedProperties.PushBack( prop );
			}
		}
	}
}

void CComponent::GetInstancePropertyNames( TDynArray< CName >& instancePropertyNames ) const
{
	// Ask the entity template for names
	if ( GetEntity() != nullptr && GetEntity()->GetEntityTemplate() != nullptr )
	{
		GetEntity()->GetEntityTemplate()->GetInstancePropertiesForComponent( CName( GetName() ), instancePropertyNames );
	}
}

Bool CComponent::CanOverridePropertyViaInclusion( const CName& propertyName ) const
{
	return !( propertyName == CNAME( graphPositionX ) ||	// Do not override graph position
		      propertyName == CNAME( graphPositionY ) ||	//   ...same
			  propertyName == CNAME( isStreamed ) ||		// Do not override streaming flag (cannot change its state)
			  propertyName == CNAME( guid ) ||				// Do not override the GUID if it happens to be regenerated
			  propertyName == CNAME( name ) );				// Do not allow changing the name since it is used to identify components
}

Bool CComponent::OnPropertyMissing( CName propertyName, const CVariant& readValue )
{
#ifndef NO_COMPONENT_GRAPH
	if ( propertyName == TXT("graphPosition") && readValue.GetType() == GetTypeName< Vector >() )
	{
		Vector pos = Vector::ZEROS;
		if ( readValue.AsType<Vector>( pos ) )
		{
			m_graphPositionX = (Int16) pos.A[0];
			m_graphPositionY = (Int16) pos.A[1];
			return true;
		}
	}
#endif

	if ( propertyName == TXT("componentId") )
	{
		return true;
	}

	if ( propertyName == TXT("streamingLOD") )
	{
		Int8 streamingLOD = -1;
		if ( readValue.AsType<Int8>( streamingLOD ) )
		{
			m_isStreamed = streamingLOD != -1;
		}
		else
		{
			m_isStreamed = false;
		}
		return true;
	}

	return TBaseClass::OnPropertyMissing( propertyName, readValue );
}

void CComponent::OnRefreshVisibilityFlag()
{
}

void CComponent::OnVisibilityForced()
{
}

void CComponent::OnParentAttachmentAdded( IAttachment* attachment )
{
	// Pass to base class
	TBaseClass::OnParentAttachmentAdded( attachment );

	// Refresh visibility flag when attaching during active game
	if ( GGame->IsActive() )
	{
		RefreshVisibilityFlag();
	}
}

void CComponent::OnParentAttachmentBroken( IAttachment* attachment )
{
	// Pass to base class
	TBaseClass::OnParentAttachmentBroken( attachment );
}

void CComponent::RefreshVisibilityFlag( NodeProcessingContext& context, Bool force )
{
	if ( force )
	{
		// Update visibility flag
		OnVisibilityForced();
	}
	else
	{
		// Update visibility flag
		OnRefreshVisibilityFlag();
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

void CComponent::RefreshVisibilityFlag( Bool force )
{
	NodeProcessingContext context;

	context.Reserve( m_childAttachments.Size() + 1 );
	context.Insert( this );

	RefreshVisibilityFlag( context, force );
}

void CComponent::RefreshNodeVisibilityFlag( NodeProcessingContext& context, Bool force )
{
	RefreshVisibilityFlag( context, force );
}


void CComponent::ToggleVisibility( Bool visible )
{
	for ( TList< IAttachment* >::iterator attachmentIter = m_childAttachments.Begin();
		attachmentIter != m_childAttachments.End(); ++attachmentIter )
	{
		CComponent* childComponent = Cast< CComponent >( ( *attachmentIter)->GetChild() );
		if ( childComponent != NULL )
		{
			childComponent->ToggleVisibility( visible );
		}
	}
}

void CComponent::OnAppearanceChanged( Bool added )
{
	if ( added && GetParentAttachments().Empty() )
	{
		if ( QueryAnimatedObjectInterface() )
		{
			PC_SCOPE( CComponent OnAppearanceChanged )
			CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
			if ( animComponent )
			{
				animComponent->Attach( this, ClassID< CAnimatedAttachment >() );
			}
		}
	}
}

void CComponent::OnStreamIn()
{
	if ( IsUsedInAppearance() && QueryAnimatedObjectInterface() != nullptr && GetParentAttachments().Empty() )
	{
		PC_SCOPE( CComponent OnStreamIn )
		CAnimatedComponent* animComponent = GetEntity()->GetRootAnimatedComponent();
		if ( animComponent )
		{
			animComponent->Attach( this, ClassID< CAnimatedAttachment >() );
		}
	}
}

void CComponent::OnStreamOut()
{
}

CWorld* CComponent::GetWorld() const			// const here is for convenience (yes, i know, looks like a nasty hack...) 
{
	if ( CEntity* ent = GetEntity() )
	{
		if ( CLayer* lay = ent->GetLayer() )
		{
			return lay->GetWorld();
		}
	}

	return nullptr;
}

void CComponent::SetStreamed( Bool enable )
{ 
	m_isStreamed = enable;
	if ( m_isStreamed )
	{
		SetFlag( OF_Referenced );
		SetFlag( OF_Transient );
	}
	else
	{
		ClearFlag( OF_Referenced );
		ClearFlag( OF_Transient );
	}
#ifndef NO_EDITOR
	if ( GetEntity() != nullptr )
	{
		GetEntity()->RemoveComponentFromStreaming( this );
	}
#endif
}

void CComponent::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	if ( property->GetName() == CNAME(isStreamed) )
	{
		SetStreamed( IsStreamed() );
	}

#ifndef NO_EDITOR
	// Make the component itself as modified
	MarkModified();

	// Transient objects will not mark their parents as modified, however streamable
	// components are marked as transient too and their parents *need* to be marked
	// (except components which reside on the dynamic layer), so mark them here
	if ( IsTransient() && GetParent() && GetWorld() && GetLayer() != GetWorld()->GetDynamicLayer() )
	{
		GetParent()->MarkModified();
	}
#endif
}

#ifndef NO_EDITOR

void CComponent::EditorOnTransformChanged()
{
	TBaseClass::EditorOnTransformChanged();
	GetEntity()->OnComponentTransformChanged( this );
}

#endif

#ifndef NO_RESOURCE_USAGE_INFO

void CComponent::CollectResourceUsage( class IResourceUsageCollector& collector, const Bool isStremable ) const
{
	// nothing here
}

#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////

void CComponent::funcGetEntity( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_OBJECT( GetEntity() );
}

void CComponent::funcIsEnabled( CScriptStackFrame& stack, void* result )
{
	FINISH_PARAMETERS;
	RETURN_BOOL( IsEnabled() );
}

void CComponent::funcSetEnabled( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, flag, false );
	FINISH_PARAMETERS;
	SetEnabled( flag );
}

void CComponent::funcSetPosition( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, position, Vector::ZEROS );
	FINISH_PARAMETERS;

	SetPosition( position );
}

void CComponent::funcSetRotation( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EulerAngles, rotation, EulerAngles::ZEROS );
	FINISH_PARAMETERS;

	SetRotation( rotation );
}

void CComponent::funcSetScale( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Vector, scale, Vector::ZEROS );
	FINISH_PARAMETERS;

	SetScale( scale );
}

void CComponent::SetShouldSave( Bool should )
{
	if ( should ) 
	{ 
		SetFlag( NF_ShouldSave ); 
		GetEntity()->SetFlag( NF_ShouldSave ); 
	} 
	else 
	{ 
		ClearFlag( NF_ShouldSave ); 
		CPeristentEntity* entity = Cast< CPeristentEntity > ( GetEntity() );
		if ( entity )
		{
			if ( false == entity->CheckShouldSave() )
			{
				entity->ClearFlag( NF_ShouldSave ); 
			}
		}
	}
}

CEntity* CComponent::GetEntity() const
{
	RED_FATAL_ASSERT( !GetParent() || GetParent()->IsA< CEntity >(), "" );
	return static_cast< CEntity* >( GetParent() );
}

CEntity* CComponent::GetRootEntity() const
{
	CEntity* e = GetEntity();

	while ( CHardAttachment* att = e->GetTransformParent() )
	{
		if ( CNode* parentNode = att->GetParent() ) // Crash shield - shouldn't be necesary :(
		{
			CEntity* parentEntity = parentNode->AsEntity();
			if ( !parentEntity )
			{
				CComponent* parentCompoennt = parentNode->AsComponent();
				RED_FATAL_ASSERT( parentCompoennt, "CNode is not a entity or component. Please debug." );
				parentEntity = parentCompoennt->GetEntity();
			}

			RED_FATAL_ASSERT( parentEntity, "CNode is not a entity or component. Please debug." );
			e = parentEntity;
		}
		else
		{
			break;
		}
	}

	return e;
}

void CComponent::SetResource( CResource* resource )
{
	RED_HALT( "[BIG ERROR] Component class %ls doesn't implement SetResource. If it uses resources, implement it!", GetClass()->GetName().AsChar() );
}

void CComponent::GetResource( TDynArray< const CResource* >& resources ) const
{
	// if the component uses resources, implement a proper version of this function
}

Float CComponent::CalculateAutoHideDistance( const Float autoHideDistance, const Box& boundingBox, const Float defaultDistance, const Float maxDistance )
{
	if ( autoHideDistance >= 0.0f )
	{
		return autoHideDistance;
	}

	Float boundingBoxDiagonal = ( boundingBox.Max - boundingBox.Min ).Mag3();
	// this 0.58f scalar multiplied by diag should return 1 for non scaled bounding box,
	// so default distance will be correct
	// if bounding box is scaled up/down than distance is correctly recalculated, but still clamped to maxDistance
	return Min( 0.58f * boundingBoxDiagonal * defaultDistance, maxDistance );
}

void CComponent::OnCutsceneStarted()
{
	// TODO - this should not be called for every type of component - we need it now because of boat
	CallEvent( CNAME( OnCutsceneStarted ) );
}

void CComponent::OnCutsceneEnded()
{
	// TODO - this should not be called for every type of component - we need it now because of boat
	CallEvent( CNAME( OnCutsceneEnded ) );
}

void CComponent::funcSetShouldSave( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( Bool, shouldSave, false );
	FINISH_PARAMETERS;
	SetShouldSave( shouldSave );
}
