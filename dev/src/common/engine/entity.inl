#pragma once
#include "componentIterator.h"
#include "animatedComponent.h"
#include "attachment.h"

template< class T > RED_INLINE
T* CEntity::FindComponent() const
{
	T* component = NULL;

	ComponentIterator< T > it( this );
	if( it )
	{
		component = *it;
	}

	// Old functionality (Generates unreachable code warning)
	// 	for ( ComponentIterator< T > it( this ); it; ++it )
	// 	{
	// 		component = static_cast<T*>( *it );
	// 		break;
	// 	}

	return component;
}

template< class T > RED_INLINE T* CEntity::FindComponent( const String& name ) const { return Cast< T >( FindComponent( name ) ); }

// Find component by class and name
template< class T > RED_INLINE T* CEntity::FindComponent( const Char* name ) const { return Cast< T >( FindComponent( name ) ); }

// Find component by class and name
template< class T > RED_INLINE T* CEntity::FindComponent( CName name ) const { return Cast< T >( FindComponent( name ) ); }

// Propagate call
template<typename F> RED_INLINE void CEntity::PropagateCallToItemEntities( F func ) const
{
	// Propagate to attached items, if any
	CAnimatedComponent* rootAnimatedComponent = GetRootAnimatedComponent();
	if ( rootAnimatedComponent != nullptr )
	{
		// Scan entity hierarchy of attached components to find components
		// which belong to other entities (all items are external entities)
		TDynArray< CEntity* > attachedEntities;
		struct {
			TDynArray< CEntity* >*	attachedEntities;
			const CEntity*			rootEntity;

			void CollectFrom( CNode* node )
			{
				const TList< IAttachment* >& attachments = node->GetChildAttachments();
				for ( auto it=attachments.Begin(); it != attachments.End(); ++it )
				{
					CComponent* childComponent = Cast< CComponent >( (*it)->GetChild() );
					if ( childComponent != nullptr )
					{
						if ( childComponent->GetEntity() != rootEntity )
						{
							attachedEntities->PushBackUnique( childComponent->GetEntity() );
						}
						else
						{
							CollectFrom( childComponent );
						}
					}
					else
					{
						CEntity* childEntity = Cast< CEntity >( (*it)->GetChild() );
						if ( childEntity != nullptr )
						{
							attachedEntities->PushBackUnique( childEntity );
						}
					}
				}
			}
		} local;
		local.rootEntity = this;
		local.attachedEntities = &attachedEntities;
		local.CollectFrom( rootAnimatedComponent );

		// Propagate the func call to the entity
		for ( auto it=attachedEntities.Begin(); it != attachedEntities.End(); ++it )
		{
			CEntity* attachedEntity = *it;
			func( attachedEntity );
		}
	}
}

RED_INLINE void CEntity::SetStreamingDistance( Uint32 distance )
{
	// Rationale: far clipping plane is at 2km, 40 meters gives us some time to stream in and 2040 == 255*8
	RED_ASSERT( distance <= 2040, TXT("Streaming distance cannot be more than 2040 meters!") );
	RED_ASSERT( (distance % 8) == 0, TXT("Streaming distance must be a multiple of 8!") )
	m_streamingDistance = static_cast< Uint8 >( distance >> 3 );
}

//////////////////////////////////////////////////////////////
RED_INLINE void CEntity::SetDisableAllDissolves( Bool disable )
{
	SetDynamicFlag( EDF_DisableAllDissolves, disable );
}