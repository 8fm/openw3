/**
* Copyright © 2013 CD Projekt Red. All Rights Reserved.
*/
#include "build.h"
#include "gameplayEffectsComponent.h"
#include "renderCommands.h"
#include "../core/scriptStackFrame.h"
#include "animatedComponent.h"
#include "componentIterator.h"


IMPLEMENT_RTTI_ENUM( EEntityGameplayEffectFlags );
IMPLEMENT_ENGINE_CLASS( CGameplayEffectsComponent );


CGameplayEffectsComponent::CGameplayEffectsComponent()
	: m_flags( EGEF_None )
	, m_mask( EGEF_None )
	, m_enabled( true )
{
}


CGameplayEffectsComponent::~CGameplayEffectsComponent()
{
}


void CGameplayEffectsComponent::OnDestroyed()
{
	StopListening();
	TBaseClass::OnDestroyed();
}


void CGameplayEffectsComponent::SetEnabled( Bool enabled )
{
	if ( m_enabled != enabled )
	{
		m_enabled = enabled;
		if ( m_enabled )
		{
			// If we have active flags, get started!
			if ( m_mask != 0 )
			{
				StartListening();
			}

			SetFlagsOnAllComponents( m_flags );
		}
		else
		{
			// Reset, instead of clearing, so that when this is disabled it's as though it weren't even there.
			ResetFlagsOnAllComponents();

			// Don't need to listen for changes if we're disabled.
			StopListening();
		}
	}
}

void CGameplayEffectsComponent::OnAttached( CWorld* world )
{
	TBaseClass::OnAttached( world );

	PC_SCOPE_PIX( CGameplayEffectsComponent_OnAttached );

	if ( m_mask != 0 && m_enabled )
	{
		StartListening();
	}
}

void CGameplayEffectsComponent::OnDetached( CWorld* world )
{
	StopListening();

	TBaseClass::OnDetached( world );
}


void CGameplayEffectsComponent::StartListening()
{
	// If m_entities is not empty, then we're already listening, so don't do anything.
	if ( !m_entities.Empty() )
	{
		return;
	}


	// Attach to any child entities of our entity
	CEntity* entity = GetEntity();

	// Want to know about any changes to our entity.
	entity->RegisterEntityListener( this );

	m_entities.PushBack( entity );

	// Also want to know about any entities that get attached to our entity.
	AttachToChildEntities( entity );

	// Also want to know about any entities that get attached to our animated components. This is usually the case for weapons and stuff.
	for ( ComponentIterator< CAnimatedComponent > it( entity ); it; ++it )
	{
		AttachToChildEntities( *it );
	}

	if ( m_enabled )
	{
		SetFlagsOnAllComponents( m_flags );
	}
}

void CGameplayEffectsComponent::StopListening()
{
	// Basically the opposite of StartListening. Unregister entity/node listeners on everything we may have previously
	// registered. So we'll stop getting notifications about changes, and so not do extra work when we've got nothing to do.

	// If m_entities is empty, we're already not listening to anything, so don't do anything.
	if ( m_entities.Empty() )
	{
		return;
	}

	ResetFlagsOnAllComponents();


	CEntity* entity = GetEntity();
	entity->UnregisterEntityListener( this );
	m_entities.RemoveFast( entity );
	DetachFromChildEntities( entity );

	for ( ComponentIterator< CAnimatedComponent > it( entity ); it; ++it )
	{
		DetachFromChildEntities( *it );
	}

	ASSERT( m_entities.Empty(), TXT("Didn't detach from all entities? May not have detached from Animated Components either...") );

	m_entities.ClearFast();
}


void CGameplayEffectsComponent::OnNotifyNodeChildAttached( CNode* parent, CNode* child, IAttachment* attachment )
{
	// If this is a new entity, propagate the flags down into it. This allows equipment and items to get the same settings
	// as the entity holding them.
	CEntity* childEntity = Cast< CEntity >( child );
	if ( childEntity )
	{
		// If we were previously registered for entity notifications, we don't need to attach again.
		if ( childEntity->RegisterEntityListener( this ) )
		{
			m_entities.PushBack( childEntity );

			if ( m_enabled )
			{
				SetFlagsOnComponents( childEntity->GetComponents(), m_flags, m_mask );
			}
		}
	}
}

void CGameplayEffectsComponent::OnNotifyNodeChildDetached( CNode* parent, CNode* child )
{
	// Can ignore the entity now that it's detached.
	CEntity* childEntity = Cast< CEntity >( child );
	if ( childEntity )
	{
		if ( childEntity->UnregisterEntityListener( this ) )
		{
			m_entities.RemoveFast( childEntity );

			// Reset all components.
			if ( m_enabled )
			{
				ResetFlagsOnComponents( childEntity->GetComponents(), m_mask );
			}
		}
	}
}


void CGameplayEffectsComponent::OnNotifyEntityComponentAdded( CEntity* entity, CComponent* component )
{
	// If this is an AC, start watching for any entities that might be added as children.
	if ( CAnimatedComponent* ac = Cast< CAnimatedComponent >( component ) )
	{
		AttachToChildEntities( ac );
	}

	if ( m_enabled )
	{
		SetFlagsOnComponent( component, m_flags, m_mask );
	}
}

void CGameplayEffectsComponent::OnNotifyEntityComponentRemoved( CEntity* entity, CComponent* component )
{
	if ( CAnimatedComponent* ac = Cast< CAnimatedComponent >( component ) )
	{
		ac->UnregisterNodeAttachListener( this );
	}

	// Unset this component.
	if ( m_enabled )
	{
		ResetFlagsOnComponent( component, m_mask );
	}
}

void CGameplayEffectsComponent::OnNotifyEntityRenderProxyAdded( CEntity* entity, CComponent* component, IRenderProxy* proxy )
{
	if ( m_enabled )
	{
		if ( Cast< CDrawableComponent >( component ) )
		{
			Uint8 lightChannels, lightChannelsMask;
			CalcLightChannels( m_flags, m_mask, lightChannels, lightChannelsMask );
			( new CRenderCommand_SetProxyLightChannels( proxy, lightChannels, lightChannelsMask ) )->Commit();
		}
	}
}


void CGameplayEffectsComponent::AttachToChildEntities( CNode* parentNode )
{
	if ( parentNode->RegisterNodeAttachListener( this ) )
	{
		// Attach to child entities of the parent node.
		const TList< IAttachment* >& childAttachments = parentNode->GetChildAttachments();
		for ( TList< IAttachment* >::const_iterator it = childAttachments.Begin(); it != childAttachments.End(); ++it )
		{
			IAttachment* att = *it;
			CEntity* childEntity = Cast< CEntity >( att->GetChild() );
			if ( childEntity )
			{
				if ( childEntity->RegisterEntityListener( this ) )
				{
					m_entities.PushBack( childEntity );
				}
			}
		}
	}
}

void CGameplayEffectsComponent::DetachFromChildEntities( CNode* parentNode )
{
	if ( parentNode->UnregisterNodeAttachListener( this ) )
	{
		const TList< IAttachment* >& childAttachments = parentNode->GetChildAttachments();
		for ( TList< IAttachment* >::const_iterator it = childAttachments.Begin(); it != childAttachments.End(); ++it )
		{
			IAttachment* att = *it;
			CEntity* childEntity = Cast< CEntity >( att->GetChild() );
			if ( childEntity )
			{
				if ( childEntity->UnregisterEntityListener( this ) )
				{
					m_entities.RemoveFast( childEntity );
				}
			}
		}
	}
}


void CGameplayEffectsComponent::SetGameplayEffectFlag( EEntityGameplayEffectFlags flag, Bool value )
{
	Uint32 newFlags = value ? ( m_flags | flag ) : ( m_flags & ~flag );
	Uint32 newMask = m_mask | flag;
	if ( m_flags != newFlags || m_mask != newMask )
	{
		m_flags = newFlags;
		m_mask = newMask;

		if ( m_enabled )
		{
			// Make sure we know about everything we need to affect.
			StartListening();
			SetFlagsOnAllComponents( m_flags );
		}
	}
}

void CGameplayEffectsComponent::ResetGameplayEffectFlag( EEntityGameplayEffectFlags flag )
{
	Uint32 newFlags = m_flags & ~flag;
	Uint32 newMask = m_mask & ~flag;
	if ( m_flags != newFlags || m_mask != newMask )
	{
		m_flags = newFlags;
		m_mask = newMask;

		if ( m_enabled )
		{
			for ( Uint32 i = 0; i < m_entities.Size(); ++i )
			{
				CEntity* ent = m_entities[ i ].Get();
				if ( ent )
				{
					ResetFlagsOnComponents( ent->GetComponents(), flag );
				}
			}

			// No active flags, so stop listening for changes.
			if ( m_mask == 0 )
			{
				StopListening();
			}
		}
	}
}

void CGameplayEffectsComponent::ResetFlagsOnAllComponents()
{
	for ( Uint32 i = 0; i < m_entities.Size(); ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( ent )
		{
			ResetFlagsOnComponents( ent->GetComponents(), m_mask );
		}
	}
}


void CGameplayEffectsComponent::SetFlagsOnAllComponents( Uint32 flags )
{
	for ( Uint32 i = 0; i < m_entities.Size(); ++i )
	{
		CEntity* ent = m_entities[ i ].Get();
		if ( ent )
		{
			SetFlagsOnComponents( ent->GetComponents(), flags, m_mask );
		}
	}
}


void CGameplayEffectsComponent::ResetFlagsOnComponents( const TDynArray< CComponent* >& components, Uint32 mask )
{
	for ( Uint32 i = 0; i < components.Size(); ++i )
	{
		ResetFlagsOnComponent( components[ i ], mask );
	}
}
void CGameplayEffectsComponent::ResetFlagsOnComponent( CComponent* component, Uint32 mask )
{
	if ( CDrawableComponent* dc = Cast< CDrawableComponent >( component ) )
	{
		// Set light channels directly on the proxy, rather than the DC. This way we don't lose the defaults if we later need
		// to reset them.
		IRenderProxy* proxy = dc->GetRenderProxy();
		if ( proxy )
		{
			Uint8 lightChannels, lightChannelsMask;
			// Only really care about the resulting lc mask, so we can get the appropriate bits from the component.
			CalcLightChannels( 0, mask, lightChannels, lightChannelsMask );
			lightChannels = dc->GetLightChannels() & lightChannelsMask;

			( new CRenderCommand_SetProxyLightChannels( proxy, lightChannels, lightChannelsMask ) )->Commit();
		}
	}
}


void CGameplayEffectsComponent::SetFlagsOnComponents( const TDynArray< CComponent* >& components, Uint32 flags, Uint32 mask )
{
	for ( Uint32 i = 0; i < components.Size(); ++i )
	{
		SetFlagsOnComponent( components[ i ], flags, mask );
	}
}


void CGameplayEffectsComponent::SetFlagsOnComponent( CComponent* component, Uint32 flags, Uint32 mask )
{
	if ( CDrawableComponent* dc = Cast< CDrawableComponent >( component ) )
	{
		// Set light channels directly on the proxy, rather than the DC. This way we don't lose the defaults if we later need
		// to reset them.
		IRenderProxy* proxy = dc->GetRenderProxy();
		if ( proxy )
		{
			Uint8 lightChannels, lightChannelsMask;
			CalcLightChannels( flags, mask, lightChannels, lightChannelsMask );
			( new CRenderCommand_SetProxyLightChannels( proxy, lightChannels, lightChannelsMask ) )->Commit();
		}
	}
}


void CGameplayEffectsComponent::CalcLightChannels( Uint32 flags, Uint32 mask, Uint8& lc, Uint8& lcMask )
{
	lc = 0;
	lcMask = 0;

	if ( ( mask & EGEF_FocusModeHighlight ) != 0 )
	{
		if ( ( ( flags & EGEF_FocusModeHighlight ) != 0 ) )
		{
			lc |= LC_Custom0;
		}
		lcMask |= LC_Custom0;
	}

	if ( ( mask & EGEF_CatViewHiglight ) != 0 )
	{
		if ( ( ( flags & EGEF_CatViewHiglight ) != 0 ) )
		{
			lc |= LC_VisibleThroughtWalls;
		}
		lcMask |= LC_VisibleThroughtWalls;
	}

}


void CGameplayEffectsComponent::funcSetGameplayEffectFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EEntityGameplayEffectFlags, flag, EGEF_None );
	GET_PARAMETER( Bool, value, false );
	FINISH_PARAMETERS;

	SetGameplayEffectFlag( flag, value );

	RETURN_VOID();
}

void CGameplayEffectsComponent::funcGetGameplayEffectFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EEntityGameplayEffectFlags, flag, EGEF_None );
	FINISH_PARAMETERS;

	RETURN_BOOL( GetGameplayEffectFlag( flag ) );
}

void CGameplayEffectsComponent::funcResetGameplayEffectFlag( CScriptStackFrame& stack, void* result )
{
	GET_PARAMETER( EEntityGameplayEffectFlags, flag, EGEF_None );
	FINISH_PARAMETERS;

	ResetGameplayEffectFlag( flag );

	RETURN_VOID();
}
