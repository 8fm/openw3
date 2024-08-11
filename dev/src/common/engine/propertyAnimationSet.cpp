#include "build.h"
#include "propertyAnimationSet.h"
#ifndef NO_EDITOR
	#include "curveEntity.h"
#endif

#include "../core/feedback.h"
#include "../core/gameSave.h"
#include "game.h"
#include "layer.h"
#include "world.h"
#include "tickManager.h"
#include "component.h"



IMPLEMENT_RTTI_ENUM( EPropertyCurveMode );

IMPLEMENT_ENGINE_CLASS( SPropertyAnimation );

RED_DEFINE_NAME( PropAnims );
RED_DEFINE_NAME( NumPropAnims );
RED_DEFINE_NAME( PropAnim );
RED_DEFINE_NAME( PropAnim_PropertyName );
RED_DEFINE_NAME( PropAnim_AnimationName );
RED_DEFINE_NAME( PropAnim_LengthScale );
RED_DEFINE_NAME( PropAnim_Time );
RED_DEFINE_NAME( PropAnim_Timer );
RED_DEFINE_NAME( PropAnim_Count );
RED_DEFINE_NAME( PropAnim_Counter );
RED_DEFINE_NAME( PropAnim_State );
RED_DEFINE_NAME( PropAnim_Mode );
RED_DEFINE_NAME( PropAnim_Paused );
RED_DEFINE_NAME( propertyAnimationSet );
RED_DEFINE_NAME( OnPropertyAnimationFinished );

SPropertyAnimation::SPropertyAnimation()
	: m_property( NULL )
	, m_playOnStartup( false )
{
}

SPropertyAnimationInstance::SPropertyAnimationInstance()
	: m_animation( NULL )
	, m_timer( 0.0f )
	, m_lengthScale( 1.0f )
	, m_counter( 1 )
	, m_count( 0 )
	, m_mode( PCM_Forward )
	, m_paused( false )
{
}

IMPLEMENT_ENGINE_CLASS( CPropertyAnimationSet );

CPropertyAnimationSet::CPropertyAnimationSet()
	: m_isRegisteredToWorld( false )
{
}

void CPropertyAnimationSet::Update( Float dt )
{
	struct ScriptCallback
	{
		CObject*	m_object;
		CName		m_propertyName;
		CName		m_animationName;
	};
	// Update all animation instances

	const Float timeScale = GGame->GetGameplayConfig().m_timeScale;
	Bool notifyListeners = false;

	TStaticArray< ScriptCallback, 64 > callbacks;

	for ( Int32 i = m_animationInstances.SizeInt() - 1; i >= 0; --i )
	{
		SPropertyAnimationInstance* animationInstance = &m_animationInstances[ i ];
		SPropertyAnimation* animation = animationInstance->m_animation;
		SMultiCurve* curve = &animation->m_curve;

		if ( animationInstance->m_paused )
		{
			continue;
		}

		// Update

		Bool toRemove = false;

		// Get time direction
		Float dir = 1.0f;
		if ( animationInstance->m_mode == PCM_Backward )
		{
			dir = -1.0f;
		}

		// Advance timer
		animationInstance->m_timer += dir * timeScale * dt / animationInstance->m_lengthScale;

		// Forward case
		if ( dir > 0.0f )
		{
			if ( animationInstance->m_timer >= animation->m_curve.GetTotalTime() )
			{
				animationInstance->m_timer = fmodf( animationInstance->m_timer, animation->m_curve.GetTotalTime() );
				animationInstance->m_counter++;
			}

			// check no of played loops
			if ( animationInstance->m_counter && animationInstance->m_counter == animationInstance->m_count )
			{
				animationInstance->m_timer = animation->m_curve.GetTotalTime();
				toRemove = true;
				
				if ( animation->m_property )
				{
					ScriptCallback callback;
					callback.m_object = animation->m_propertyParent;
					callback.m_propertyName = animation->m_property->GetName();
					callback.m_animationName = animation->m_animationName;
					callbacks.PushBack( callback );
				}
			}
		}

		// Backward case
		else
		{
			if ( animationInstance->m_timer < 0.0f )
			{
				animationInstance->m_timer += animation->m_curve.GetTotalTime();
				animationInstance->m_counter++;
			}

			// check no of played loops
			if ( animationInstance->m_counter && animationInstance->m_counter == animationInstance->m_count )
			{
				animationInstance->m_timer = 0.0f;
				toRemove = true;
			}
		}

		// Update property value

		if ( animation->m_property ) // Property may not be bound yet (entity is still being streamed in)
		{
			UpdatePropertyValue( animationInstance );
		}

		// Remove dead animation instance
		if ( toRemove )
		{
			notifyListeners = true;
			m_animationInstances.RemoveAtFast( i );
		}
	}

	// do script callback after iteration
	if ( !callbacks.Empty() )
	{
		for ( auto it = callbacks.Begin(), end = callbacks.End(); it != end; ++it )
		{
			ScriptCallback& callback = *it;
			callback.m_object->CallEvent( CNAME( OnPropertyAnimationFinished ), callback.m_propertyName, callback.m_animationName );
		}
	}

	if ( notifyListeners )
	{
		NotifyListeners();
	}

	// Unregister from tick manager if there's no active animations

	if ( m_animationInstances.Empty() && m_isRegisteredToWorld )
	{
		UnregisterFromWorld( GetWorld() );
	}
}
SPropertyAnimationInstance* CPropertyAnimationSet::FindAnyAnimationInstance( CName animationName )
{
	for ( SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_animationName == animationName )
		{
			return &animationInstance;
		}
	}
	return NULL;
}
SPropertyAnimationInstance* CPropertyAnimationSet::FindAnimationInstance( CName propertyName, CName animationName )
{
	for ( SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_animationName == animationName && animationInstance.m_animation->m_propertyName == propertyName )
		{
			return &animationInstance;
		}
	}
	return NULL;
}

SPropertyAnimation* CPropertyAnimationSet::GetPropertyAnimation( CName propertyName, CName animationName )
{
	for ( SPropertyAnimation& animation : m_animations )
	{
		if ( animation.m_propertyName == propertyName && animation.m_animationName == animationName )
		{
			return &animation;
		}
	}

	return NULL;
}

Bool CPropertyAnimationSet::Play( CName animationName, Uint32 count, Float lengthScale, EPropertyCurveMode mode )
{
	Bool notifyListeners = false;

	// Start animations with matching name

	Bool startedAnimation = false;
	for ( auto it = m_animations.Begin(), end = m_animations.End(); it != end; ++it )
	{
		if ( it->m_animationName == animationName )
		{
			SPropertyAnimationInstance* animationInstance = FindAnimationInstance( it->m_propertyName, animationName );
			if ( !animationInstance )
			{
				m_animationInstances.PushBack( SPropertyAnimationInstance() );
				animationInstance = &m_animationInstances.Back();
				animationInstance->m_timer = ( mode == PCM_Forward ) ? 0.0f : it->m_curve.GetTotalTime();
				notifyListeners = true;
			}
			animationInstance->m_animation = &(*it);
			animationInstance->m_mode = mode;
			animationInstance->m_count = count;
			animationInstance->m_counter = 0;
			animationInstance->m_lengthScale = lengthScale;
			PlayEffect( animationInstance );

			if ( !m_isRegisteredToWorld )
			{
				RegisterToWorld( GetWorld() );
			}

			startedAnimation = true;
		}
	}

	if ( notifyListeners )
	{
		NotifyListeners();
	}

	return startedAnimation;
}

void CPropertyAnimationSet::Stop( CName animationName, Bool restoreInitialValues )
{
	Bool notifyListeners = false;

	if ( restoreInitialValues )
	{
		Rewind( animationName, 0.0f );
	}

	for ( Uint32 i = 0; i < m_animationInstances.Size(); ++i )
	{
		SPropertyAnimationInstance* animationInstance = &m_animationInstances[ i ];
		if ( animationInstance->m_animation->m_animationName == animationName )
		{
			notifyListeners = true;
			StopEffect( animationInstance );
			m_animationInstances.EraseFast( m_animationInstances.Begin() + i );
			--i;
		}
	}

	if ( notifyListeners )
	{
		NotifyListeners();
	}

	if ( m_animationInstances.Empty() && m_isRegisteredToWorld )
	{
		UnregisterFromWorld( GetWorld() );
	}
}

void CPropertyAnimationSet::PlayEffect( SPropertyAnimationInstance* instance )
{
	SPropertyAnimation* animation = instance->m_animation;
	const CName effectName = animation->m_effectToPlay;
	if ( !effectName )
	{
		return;
	}

	if ( CEntity* parent = Cast< CEntity >( GetParent() ) )
	{
		parent->PlayEffect( effectName );
	}
}

void CPropertyAnimationSet::StopEffect( SPropertyAnimationInstance* instance )
{
	SPropertyAnimation* animation = instance->m_animation;
	const CName effectName = animation->m_effectToPlay;
	if ( !effectName )
	{
		return;
	}

	if ( CEntity* parent = Cast< CEntity >( GetParent() ) )
	{
		parent->StopEffect( effectName );
	}
}

void CPropertyAnimationSet::UpdatePropertyValue( SPropertyAnimationInstance* animationInstance )
{
	SPropertyAnimation* animation = animationInstance->m_animation;
	if ( !animation->m_propertyParent )
	{
		return;
	}

	void* propertyData = animation->m_property->GetOffsetPtr( animation->m_propertyParent );
	switch ( animation->m_curve.GetCurveType() )
	{
		case ECurveType_Float:
		{
			Float* value = (Float*) propertyData;
			*value = animation->m_curve.GetFloat( animationInstance->m_timer );
			break;
		}
		case ECurveType_Vector:
		{
			Vector* value = (Vector*) propertyData;
			animation->m_curve.GetRootPosition( animationInstance->m_timer, *value );
			break;
		}
		case ECurveType_EulerAngles:
		{
			EulerAngles* value = (EulerAngles*) propertyData;
			animation->m_curve.GetRootRotation( animationInstance->m_timer, *value );
			break;
		}
		case ECurveType_EngineTransform:
		{
			EngineTransform* value = (EngineTransform*) propertyData;
			animation->m_curve.GetRootTransform( animationInstance->m_timer, *value );
			break;
		}
	}

	// Notify object of property change
	animation->m_propertyParent->OnPropertyExternalChanged( animation->m_property->GetName() );

	// hack
	if ( animation->m_property->GetName() == CNAME( transform ) )
	{
		m_dirtyPropertiesForSaving.PushBackUnique( animation->m_propertyName );
	}
}

void CPropertyAnimationSet::NotifyListeners()
{
	for ( IAnimationSetListener* listener : m_listeners )
	{
		listener->OnStateChanged( this );
	}
}

void CPropertyAnimationSet::StopAll( Bool restoreInitialValues )
{
	if ( restoreInitialValues )
	{
		for ( SPropertyAnimation& animation : m_animations )
		{
			SPropertyAnimationInstance dummyAnimInstance;
			dummyAnimInstance.m_animation = &animation;
			dummyAnimInstance.m_timer = 0.0f;
			UpdatePropertyValue( &dummyAnimInstance );
		}
	}

	for ( SPropertyAnimationInstance& instance : m_animationInstances )
	{
		StopEffect( &instance );
	}
	m_animationInstances.ClearFast();

	if ( m_isRegisteredToWorld )
	{
		UnregisterFromWorld( GetWorld() );
	}
}

void CPropertyAnimationSet::Rewind( CName animationName, Float time )
{
	for ( SPropertyAnimation& animation : m_animations )
	{
		// If the animation is currently being played, modify its instance timer and update property value

		if ( SPropertyAnimationInstance* animationInstance = FindAnimationInstance( animation.m_propertyName, animation.m_animationName ) )
		{
			animationInstance->m_timer = Clamp( time, 0.0f, animation.m_curve.GetTotalTime() );
			UpdatePropertyValue( animationInstance );
		}

		// Otherwise, just update property value

		else
		{
			SPropertyAnimationInstance dummyAnimInstance;
			dummyAnimInstance.m_animation = &animation;
			dummyAnimInstance.m_timer = Clamp( time, 0.0f, animation.m_curve.GetTotalTime() );

			UpdatePropertyValue( &dummyAnimInstance );
		}
	}
}

void CPropertyAnimationSet::Pause( CName animationName )
{
	for ( SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_animationName == animationName )
		{
			StopEffect( &animationInstance );
			animationInstance.m_paused = true;
		}
	}
}

void CPropertyAnimationSet::Unpause( CName animationName )
{
	for ( SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_animationName == animationName )
		{
			animationInstance.m_paused = false;
		}
	}
}

Bool CPropertyAnimationSet::GetAnimationInstanceTime( CName propertyName, CName animationName, Float& outTime )
{
	if ( SPropertyAnimationInstance* animationInstance = FindAnimationInstance( propertyName, animationName ) )
	{
		outTime = animationInstance->m_timer;
		return true;
	}
	return false;
}

Bool CPropertyAnimationSet::GetAnimationTransformAt( CName propertyName, CName animationName, Float time, EngineTransform& outTransform )
{
	if ( SPropertyAnimation* animation = GetPropertyAnimation( propertyName, animationName ) )
	{
		animation->m_curve.GetRootTransform( time, outTransform );
		return true;
	}
	return false;
}

Bool CPropertyAnimationSet::GetAnimationLength( CName propertyName, CName animationName, Float& length )
{
	if ( SPropertyAnimation* animation = GetPropertyAnimation( propertyName, animationName ) )
	{
		length = animation->m_curve.GetTotalTime();
		return true;
	}
	return false;
}

Bool CPropertyAnimationSet::IsPlaying( CName animationName ) const
{
	for ( const SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_animationName == animationName )
		{
			return true;
		}
	}

	return false;
}
Bool CPropertyAnimationSet::IsPlaying( CName propertyName, CName animationName ) const
{
	for ( const SPropertyAnimationInstance& animationInstance : m_animationInstances )
	{
		if ( animationInstance.m_animation->m_propertyName == propertyName &&
			 animationInstance.m_animation->m_animationName == animationName )
		{
			return true;
		}
	}

	return false;
}

CWorld* CPropertyAnimationSet::GetWorld()
{
	if ( CNode* node = Cast<CNode>( GetParent() ) )
	{
		return node->GetLayer()->GetWorld();
	}
	return NULL;
}

void CPropertyAnimationSet::UnbindAllProperties()
{
	for ( SPropertyAnimation& animation : m_animations )
	{
		animation.m_property = NULL;
		animation.m_propertyParent = NULL;
		animation.m_curve.SetParent( NULL );
	}
}

void CPropertyAnimationSet::BindAllProperties()
{
	// Bind object properties

	CEntity* parent = Cast< CEntity >( GetParent() );
	ASSERT( parent );

	BindObjectProperties( parent );

	// Bind component properties

	for ( CComponent* component : parent->GetComponents() ) 
	{
		BindObjectProperties( component );
	}
}

void CPropertyAnimationSet::BindObjectProperties( CNode* propertyParent )
{
	TDynArray<CProperty*> props;
	propertyParent->GetClass()->GetProperties( props );

	CComponent* component = Cast< CComponent >( propertyParent );
	CEntity* entity = component ? component->GetEntity() : Cast< CEntity >( propertyParent );
	const Bool isRestoredFromLayerStorage = entity ? entity->CheckDynamicFlag( EDF_RestoredFromLayerStorage ) : false;

	for ( auto it = m_animations.Begin(); it != m_animations.End(); ++it )
	{
		SPropertyAnimation& animation = *it;

		// Find property

		for ( Uint32 j = 0; j < props.Size(); ++j )
		{
			CProperty* prop = props[j];

			const String propFullName = component ?
				(component->GetName() + TXT("/") + prop->GetName().AsString() ) :
				prop->GetName().AsString();

			if ( propFullName == animation.m_propertyName.AsString() )
			{
				if ( !prop->IsEditable() )
				{
					GFeedback->ShowWarn( TXT( "Property to be animated via curve isn't editable! [%s]" ), animation.m_propertyName.AsString().AsChar() );
					break;
				}

 				animation.m_property = prop;
				animation.m_propertyParent = propertyParent;
				animation.m_curve.SetParent( propertyParent );

				// Initialize initial transform for newly spawned entities

				if ( !isRestoredFromLayerStorage )
				{
					it->m_curve.UpdateInitialTransformFromParent( true );
				}

				// Get property type

				ECurveType desiredCurveType = ECurveType_Uninitialized;
				const CName& propTypeName = prop->GetType()->GetName();
				if ( propTypeName == CNAME( EngineTransform ) )
					desiredCurveType = ECurveType_EngineTransform;
				else if ( propTypeName == CNAME( Vector ) )
					desiredCurveType = ECurveType_Vector;
				else if ( propTypeName == CNAME( Float ) )
					desiredCurveType = ECurveType_Float;
				else if ( propTypeName == CNAME( EulerAngles ) )
					desiredCurveType = ECurveType_EulerAngles;

				if ( desiredCurveType != animation.m_curve.GetCurveType() )
				{
					// Set curve type and initial value

					const void* initialValue = NULL;
					if ( !propertyParent->IsA<CEntity>() || prop->GetName() != CNAME( transform ) ) // Don't set initial value for m_transform property
					{
						initialValue = prop->GetOffsetPtr( propertyParent );
					}

					animation.m_curve.SetCurveType( desiredCurveType, initialValue );

					// Set up other curve properties

					animation.m_curve.SetShowFlags( SHOW_AnimatedProperties );
					animation.m_curve.EnableAutomaticTimeByDistanceRecalculation( true );
				}
				break;
			}
		}
	}

#if 0

	// Verify assignment operation

	CEntity* parent = Cast< CEntity >( GetParent() );
	if ( !parent->IsInGame() )
	{
		for ( auto it = m_animations.Begin(); it != m_animations.End(); ++it )
		{
			const String propertyName = it->m_propertyName.AsString();

			size_t splitterIndex;
			if ( propertyName.FindCharacter( TXT('/'), splitterIndex ) ) // Is it component property?
			{
				if ( !component || !propertyName.BeginsWith( component->GetName() ) ) // Does the component name match?
				{
					continue;
				}
			}
			else if ( component )
			{
				continue;
			}

			if ( it->m_propertyName != CName::NONE && !it->m_property )
			{
				GFeedback->ShowWarn( TXT( "Cannot find or unsupported type of property \'%s\' in object [%s]" ), it->m_propertyName.AsString().AsChar(), GetParent()->GetFriendlyName().AsChar() );
			}
		}
	}
#endif
}

void CPropertyAnimationSet::UnbindObjectProperties( CNode* propertyParent )
{
	for ( auto it = m_animations.Begin(); it != m_animations.End(); ++it )
	{
		if ( !propertyParent || it->m_propertyParent == propertyParent )
		{
			it->m_property = NULL;
			it->m_propertyParent = NULL;
			it->m_curve.SetParent( NULL );
		}
	}
}

void CPropertyAnimationSet::OnAttached( CWorld* world )
{
	CEntity* parent = Cast<CEntity>( GetParent() );
	BindAllProperties();
	parent->RegisterEntityListener( this );
}

void CPropertyAnimationSet::OnAttachFinished( CWorld* world )
{
	CEntity* parent = Cast<CEntity>( GetParent() );
	ASSERT( parent );

	if ( parent->IsInGame() )
	{
		for ( SPropertyAnimation& animation : m_animations )
		{
			if ( animation.m_playOnStartup && !IsPlaying( animation.m_propertyName, animation.m_animationName ) )
			{
				if ( !m_isRegisteredToWorld )
				{
					RegisterToWorld( GetWorld() );
				}

				Play( animation.m_animationName );
			}
		}
	}
}

void CPropertyAnimationSet::OnNotifyEntityComponentAdded( CEntity* entity, CComponent* component )
{
	BindObjectProperties( component );
}

void CPropertyAnimationSet::OnNotifyEntityComponentRemoved( CEntity* entity, CComponent* component )
{
	UnbindObjectProperties( component );
}

Bool CPropertyAnimationSet::AddListener( IAnimationSetListener* listener )
{
	return m_listeners.PushBackUnique( listener );
}

Bool CPropertyAnimationSet::RemoveListener( IAnimationSetListener* listener )
{
	return m_listeners.Remove( listener );
}

void CPropertyAnimationSet::OnDetached( CWorld* world )
{
	for ( SPropertyAnimationInstance& instance : m_animationInstances )
	{
		StopEffect( &instance );
	}
	m_animationInstances.ClearFast();

	m_listeners.ClearFast();

	UnbindObjectProperties( NULL );

	CEntity* parent = Cast<CEntity>( GetParent() );
	parent->UnregisterEntityListener( this );

	if ( m_isRegisteredToWorld )
	{
		UnregisterFromWorld( world );
	}

#ifndef NO_EDITOR
	for ( auto it = m_animations.Begin(); it != m_animations.End(); ++it )
	{
		CCurveEntity::DeleteEditor( world, &it->m_curve );
	}
#endif
}

void CPropertyAnimationSet::UpdateInitialTransformFrom( CNode* node )
{
	for ( auto it = m_animations.Begin(); it != m_animations.End(); ++it )
	{
		if ( !node || it->m_propertyParent == node )
		{
			it->m_curve.UpdateInitialTransformFromParent( true );
		}
	}
}

void CPropertyAnimationSet::RegisterToWorld( CWorld* world )
{
	ASSERT( !m_isRegisteredToWorld );
	ASSERT( world );
	world->GetTickManager()->AddPropertyAnimationSet( this );
	m_isRegisteredToWorld = true;
}

void CPropertyAnimationSet::UnregisterFromWorld( CWorld* world )
{
	ASSERT( m_isRegisteredToWorld );
	ASSERT( world );
	world->GetTickManager()->RemovePropertyAnimationSet( this );
	m_isRegisteredToWorld = false;
}

void CPropertyAnimationSet::OnSaveGameplayState( IGameSaver* saver )
{
	CGameSaverBlock block( saver, CNAME(PropAnims) );

	const Uint32 numAnimationInstances = m_animationInstances.Size();
	saver->WriteValue( CNAME(NumPropAnims), numAnimationInstances );

	for ( auto it = m_animationInstances.Begin(); it != m_animationInstances.End(); ++it )
	{
		const SPropertyAnimationInstance& animationInstance = *it;

		CGameSaverBlock block( saver, CNAME(PropAnim) );
		saver->WriteValue( CNAME(PropAnim_PropertyName), animationInstance.m_animation->m_propertyName );
		saver->WriteValue( CNAME(PropAnim_AnimationName), animationInstance.m_animation->m_animationName );
		saver->WriteValue( CNAME(PropAnim_Timer), animationInstance.m_timer );
		saver->WriteValue( CNAME(PropAnim_LengthScale), animationInstance.m_lengthScale );
		saver->WriteValue( CNAME(PropAnim_Count), animationInstance.m_count );
		saver->WriteValue( CNAME(PropAnim_Counter), animationInstance.m_counter );
		saver->WriteValue( CNAME(PropAnim_Mode), (Uint32)animationInstance.m_mode );
		saver->WriteValue( CNAME(PropAnim_Paused), animationInstance.m_paused );
	}

	// hack
	{
		CGameSaverBlock block( saver, CNAME( properties ) );

		Uint32 numPropertiesModified = m_dirtyPropertiesForSaving.Size();
		saver->WriteValue( CNAME( n ), numPropertiesModified );

		for ( auto p : m_dirtyPropertiesForSaving )
		{
			saver->WriteValue( CNAME( p ), p );

			CNode* object = nullptr;
			CProperty* prop = nullptr;

			if ( ResolveProperty( p, object, prop ) )
			{
				saver->WriteProperty( object, prop );
			}
			else
			{
				RED_LOG( Save, TXT("Can't resolve animated property: %s in %s"), p.AsChar(), GetFriendlyName().AsChar() );
			}
		}
	}
}

void CPropertyAnimationSet::OnLoadGameplayState( IGameLoader* loader )
{
	CGameSaverBlock block( loader, CNAME(PropAnims) );

	const Uint32 numAnimationInstances = loader->ReadValue<Uint32>( CNAME(NumPropAnims) );

	m_animationInstances.Reserve( numAnimationInstances );
	for ( Uint32 i = 0; i < numAnimationInstances; i++ )
	{
		CGameSaverBlock block( loader, CNAME(PropAnim) );

		// Find animation

		const CName propertyName = loader->ReadValue<CName>( CNAME(PropAnim_PropertyName), CName::NONE );
		const CName animationName = loader->ReadValue<CName>( CNAME(PropAnim_AnimationName), CName::NONE );
		SPropertyAnimation* animation = GetPropertyAnimation( propertyName, animationName );
		if ( !animation )
		{
			continue;
		}

		// Make sure there's no duplicates (could happen in old savegames due to bugs that are already fixed)

		if ( FindAnimationInstance( propertyName, animationName ) )
		{
			continue;
		}

		// Create animation instance

		SPropertyAnimationInstance animationInstance;
		animationInstance.m_animation = animation;
		animationInstance.m_timer = loader->ReadValue<Float>( CNAME(PropAnim_Timer) );
		animationInstance.m_lengthScale = loader->ReadValue<Float>( CNAME(PropAnim_LengthScale) );
		animationInstance.m_count = loader->ReadValue<Uint32>( CNAME(PropAnim_Count) );
		animationInstance.m_counter = loader->ReadValue<Uint32>( CNAME(PropAnim_Counter) );
		animationInstance.m_mode = (EPropertyCurveMode)loader->ReadValue<Uint32>( CNAME(PropAnim_Mode) );
		animationInstance.m_paused = loader->ReadValue< Bool >( CNAME(PropAnim_Paused) );

		m_animationInstances.PushBack( animationInstance );

		if ( !m_isRegisteredToWorld )
		{
			RegisterToWorld( GetWorld() );
		}
	}

	// hack
	{
		CGameSaverBlock block( loader, CNAME( properties ) );

		Uint32 numPropertiesModified = loader->ReadValue< Uint32 > ( CNAME( n ), 0 );
		m_dirtyPropertiesForSaving.ClearFast();
		m_dirtyPropertiesForSaving.Reserve( numPropertiesModified );

		for ( Uint32 i = 0; i < numPropertiesModified; ++i )
		{
			CName propName = loader->ReadValue< CName > ( CNAME( p ), CName::NONE );
			if ( propName )
			{
				m_dirtyPropertiesForSaving.PushBackUnique( propName );

				CNode* node = nullptr;
				CProperty* prop = nullptr;

				if ( ResolveProperty( propName, node, prop ) )
				{
					loader->ReadProperty( node, node->GetClass(), node );
					if ( propName == CNAME( transform ) )
					{
						if ( CEntity* e = FindParent< CEntity >() )
						{
							e->ForceUpdateTransformNodeAndCommitChanges();
						}
					}
				}
				else
				{
					RED_LOG( Save, TXT("Can't resolve animated property: %s in %s"), propName.AsChar(), GetFriendlyName().AsChar() );
				}
			}
		}
	}
}

void CPropertyAnimationSet::OnPropertyPostChange( IProperty* property )
{
	TBaseClass::OnPropertyPostChange( property );

	UnbindAllProperties();
	BindAllProperties();

#ifndef NO_EDITOR
	if ( CWorld* world = GetWorld() )
	{
		// Delete curve editors that don't have a valid curve associated with them
		// This can be the case if we added entry to animations or changed animation/property name
		CCurveEntity::DeleteEditors( world, true );
	}
#endif
}

Bool CPropertyAnimationSet::CheckShouldSave() const
{
	// a slightly hacky "pragmatic solution" here...

	// WHAT:
	// We save property animations if they are animating transform

	// WHY:
	// Becase quests are hacked. Because quests are moving object directly by playing property animation on transform,
	// which is a hack. And they are doing it many times. What they should do is to write a simple script calss for each type
	// of an object they want to move, and operate on states and save only current state. Well, they don't and they won't
	// because they are out of time. So we solve this particular problem here by saving those hand-fired property anims.

	static const Char transformName[] = TXT("transform");
	static const Uint32 transformNameLength = ( Uint32 ) Red::System::StringLengthCompileTime( transformName );

	for ( const SPropertyAnimation& anim : m_animations )
	{
		const Char* propName = anim.m_propertyName.AsChar();
		const Uint32 propNameLength = ( Uint32 ) Red::System::StringLength( propName );

		// Test if property name ends with "transform"
		
		if ( propNameLength >= transformNameLength &&
			!Red::System::StringCompare( propName + propNameLength - transformNameLength, transformName ) )
		{
			return true;
		}
	}

	return false;
}

Bool CPropertyAnimationSet::ResolveProperty( CName propName, CNode*& objPtr, CProperty*& propPtr )
{
	String propString = propName.AsString();
	String componentName, propertyName;

	size_t idx;
	if ( propString.FindCharacter( L'/', idx ) )
	{
		componentName = propString.LeftString( idx );
		propertyName = propString.RightString( propString.GetLength() - idx - 1 );
	}
	else
	{
		propertyName = propString;
	}

	if ( propertyName.Empty() )
	{
		return false;
	}

	if ( componentName.Empty() )
	{
		objPtr = Cast< CNode > ( GetParent() );
	}
	else
	{
		CEntity* parent = Cast< CEntity >( GetParent() );
		for ( ComponentIterator< CComponent > it( parent ); it; ++it )
		{
			if ( componentName == ( *it )->GetName() )
			{
				objPtr = *it;
				break;
			}
		}
	}

	if ( nullptr == objPtr )
	{
		return false;
	}

	// this sucks, big time
	CName name = CName( propertyName );
	propPtr = objPtr->GetClass()->FindProperty( name );

	return propPtr != nullptr;
}

CAnimatedPropertyCapture::CAnimatedPropertyCapture()
	: m_animationSet( nullptr )
{}

void CAnimatedPropertyCapture::Init( CPropertyAnimationSet* animationSet )
{
	m_animationSet = animationSet;
}

void CAnimatedPropertyCapture::CaptureAnimatedProperties( const CName animationName )
{
	ASSERT( m_animationSet );

	m_values.ClearFast();

	for ( auto it = m_animationSet->m_animations.Begin(), end = m_animationSet->m_animations.End(); it != end; ++it )
	{
		SPropertyAnimation* animation = &( *it );
		if ( animation->m_animationName != animationName || !animation->m_propertyParent )
		{
			continue;
		}

		void* propertyData = it->m_property->GetOffsetPtr( animation->m_propertyParent );

		PropertyOriginalValue value;
		value.m_animation = animation;
		value.m_value.Init( animation->m_property->GetType()->GetName(), propertyData );

		m_values.PushBack( value );
	}
}

void CAnimatedPropertyCapture::RestoreAnimatedProperties()
{
	ASSERT( m_animationSet );

	for ( auto it = m_values.Begin(), end = m_values.End(); it != end; ++it )
	{
		PropertyOriginalValue& value = *it;
		if ( !value.m_animation->m_propertyParent )
		{
			continue;
		}

		void* propertyData = value.m_animation->m_property->GetOffsetPtr( value.m_animation->m_propertyParent );
		value.m_value.GetRTTIType()->Copy( propertyData, value.m_value.GetData() );

		// Notify object of property change

		value.m_animation->m_propertyParent->OnPropertyExternalChanged( value.m_animation->m_property->GetName() );
	}
}
