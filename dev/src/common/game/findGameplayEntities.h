/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#pragma once

#include "gameplayStorage.h"
#include "actorsManager.h"
#include "entityParams.h"
#include "movingPhysicalAgentComponent.h"
#include "movableRepresentationPhysicalCharacter.h"
#include "../engine/physicsCharacterWrapper.h"
#include "../engine/physicsCharacterVirtualController.h"

//////////////////////////////////////////////////////////////////////////
// stuff in this file was moved from gameplayStorage.h
//////////////////////////////////////////////////////////////////////////

namespace FindGameplayEntities
{

template < class Origin >
struct OriginPosition
{
};

template <>
struct OriginPosition< const Vector >
{
	static const Vector& Get( const Vector& v ) { return v; }
	static const Vector& GetActorQueryType( const Vector& v ) { return v; }
	static const CEntity* GetEntityType( const Vector& v ) { return nullptr; }
};

template <>
struct OriginPosition< CGameplayEntity >
{
	static const Vector& Get( const CGameplayEntity& entity ) { return entity.GetWorldPositionRef(); }
	static const CActor& GetActorQueryType( const CGameplayEntity& entity ) { return static_cast< const CActor& >( entity ); }
	static const CEntity* GetEntityType( const CGameplayEntity& entity ) { return &entity; }
};

template < class Origin, class CustomAcceptor >
struct SQueryFunctor : public CGameplayStorage::DefaultFunctor
{
	TDynArray< THandle< CGameplayEntity > >*		m_output;	// may be nullptr if functor is used only for testing/acceptance
	const Origin&									m_origin;
	const CGameplayStorage::SSearchParams&			m_searchParams;
	const CustomAcceptor&							m_acceptor;

	enum { SORT_OUTPUT = true };

	SQueryFunctor( TDynArray< THandle< CGameplayEntity > >* output, const Origin& origin, const CGameplayStorage::SSearchParams& searchParams, const CustomAcceptor& acceptor )
		: m_output( output )
		, m_origin( origin )
		, m_searchParams( searchParams )
		, m_acceptor( acceptor )
	{
	}

	RED_FORCE_INLINE Bool operator()( const CActorsManagerMemberData& data )
	{
		return AcceptAndCollect( data.Get() );
	}

	RED_FORCE_INLINE Bool operator()( const TPointerWrapper< CGameplayEntity >& ptr )
	{
		return AcceptAndCollect( ptr.Get() );
	}

	Bool AcceptAndCollect( const CGameplayEntity* entity ) const
	{
		if ( m_output == nullptr )
		{
			return false;
		}
		if ( Accept( entity ) )
		{
			m_output->PushBack( entity );
			return !m_searchParams.IsFull( m_output->Size() );
		}
		return true;
	}

	Bool Accept( const CGameplayEntity* entity ) const
	{
		Bool res = false;
		if ( TestAllPossibleCylinders( entity, res ) )
		{
			return res;
		}
		else
		{
			if ( m_acceptor( entity ) )
			{
				return ( !m_searchParams.ShouldTestLineOfSight() || TestLineOfSight( entity->GetWorldPosition(), entity ) );
			}
		}
		return false;
	}

	Bool TestLineOfSight( const AACylinder& target, const CEntity* ignoreEntity = nullptr ) const
	{
		const Vector sourcePosition = GetLineOfSightSourcePosition();
		Vector targetToSource = sourcePosition - target.GetPosition();
		
		// if we're inside cylinder, line of sight test automatically passes
		const Float dist = targetToSource.Normalize2();
		const Float radius = target.GetRadius();
		if ( dist <= radius && sourcePosition.Z >= target.GetPosition().Z && sourcePosition.Z <= target.GetPosition().Z + target.GetHeight() )
		{
			return true;
		}

		// if not, the target test position is the intersection of cylinder surface with targetToSource, moved a little bit up
		const Float checkRatio = 0.7f;
		const Vector targetPosition = target.GetPosition() + Vector( targetToSource.X * radius, targetToSource.Y * radius, target.GetHeight() * checkRatio, 0.0f );
		return TestLineOfSight( targetPosition, ignoreEntity );
	}

	Bool TestLineOfSight( const Vector& targetPosition, const CEntity* ignoreEntity = nullptr ) const
	{
		RED_ASSERT( m_searchParams.ShouldTestLineOfSight() );
		if ( GGame->GetActiveWorld() == nullptr )
		{
			return false;
		}
		static TDynArray< const CEntity* > ignoreEntities( 2 );
		ignoreEntities[ 0 ] = OriginPosition< Origin >::GetEntityType( m_origin );
		ignoreEntities[ 1 ] = ignoreEntity;
		return GGame->GetActiveWorld()->TestLineOfSight( GetLineOfSightSourcePosition(), targetPosition, &ignoreEntities );
	}

private:

	Vector GetLineOfSightSourcePosition() const
	{
		return ( m_searchParams.m_losPosition != Vector::ZERO_3D_POINT ) ? m_searchParams.m_losPosition : OriginPosition< Origin >::Get( m_origin );
	}

	// The following method returns true if there were any cylinders found.
	// Moreover, it returns result of the test in "res" param.
	Bool TestAllPossibleCylinders( const CGameplayEntity* entity, Bool& res ) const
	{
		// Similar code for obtaining and testing physical and virtual controllers
		// can be found in CAtor::GetStorageBounds.
		// Keep them both compatible.
		
		Bool found = false;

		if ( entity->IsA< CActor >() )
		{
			const CActor* actor = static_cast< const CActor* >( entity );
			CMovingPhysicalAgentComponent* mpac = Cast< CMovingPhysicalAgentComponent >( actor->GetMovingAgentComponent() );
			if ( mpac != nullptr && mpac->GetPhysicalCharacter() != nullptr )
			{
				//  first we check the physical controller
				AACylinder cylinder;
				Vector worldPosition;
				actor->PredictWorldPosition( m_searchParams.m_predictPositionInTime, worldPosition );
				if ( mpac->GetPhysicalCharacter()->GetCollisionControllerExtents( cylinder, worldPosition ) )
				{
					found = true;
					if ( TestCylinder( cylinder, entity ) )
					{
						res = true;
						return true;
					}
				}
#ifdef USE_PHYSX
				// next, if actor is in combat, we test its virtual controllers (if there are any)
				if ( actor->IsInCombat() && mpac->GetPhysicalCharacter()->GetCharacterController() != nullptr )
				{
					typedef TDynArray< CVirtualCharacterController > VCCArray;
					const VCCArray& virtualControllers = mpac->GetPhysicalCharacter()->GetCharacterController()->GetVirtualControllers();
					VCCArray::const_iterator itEnd = virtualControllers.End();
					for ( VCCArray::const_iterator it = virtualControllers.Begin(); it != itEnd; ++it )
					{
						if ( it->IsEnabled() )
						{
							found = true;
							if ( TestCylinder( AACylinder( it->GetGlobalPosition(), it->GetCurrentRadius(), it->GetCurrentHeight() ), entity ) )
							{
								res = true;
								return true;
							}
						}
					}
				}
#endif
			}		
		}

		// entity is not an actor or does not have controllers (that's actually not possible, but...)
		const CEntityTemplate* entityTemplate = entity->GetEntityTemplate();
		const CAttackableArea* params = entityTemplate ? entityTemplate->FindGameplayParamT< CAttackableArea >() : nullptr;
		if ( params != nullptr )
		{
			found = true;
			const Matrix& localToWorld = entity->GetLocalToWorld();
			const Vector cylinderWorldPos = localToWorld.TransformPoint( params->GetOffset() );
			if ( TestCylinder( AACylinder( cylinderWorldPos, params->GetRadius(), params->GetHeight() ), entity ) )
			{
				res = true;
				return true;
			}
		}

		return found;
	}

	Bool TestCylinder( const AACylinder& cylinder, const CGameplayEntity* entity ) const
	{
		if ( m_acceptor( cylinder ) )
		{
			return ( !m_searchParams.ShouldTestLineOfSight() || TestLineOfSight( cylinder, entity ) );
		}
		return false;
	}
};

}

///////////////////////////////////////////////////////////////////////////////

//! Find gameplay entities in given (box) range, starting from the origin, based on params and using acceptor functor
template < class Origin, class CustomAcceptor >
void FindGameplayEntitiesInRange( TDynArray< THandle< CGameplayEntity > >& output, const Origin& origin, const CGameplayStorage::SSearchParams& params, const CustomAcceptor& acceptor )
{
	FindGameplayEntities::SQueryFunctor< Origin, CustomAcceptor > functor( &output, origin, params, acceptor );	

	if ( params.m_flags & FLAGMASK_OnlyActors )
	{
		GCommonGame->GetActorsManager()->TQuery( FindGameplayEntities::OriginPosition< Origin >::GetActorQueryType( origin ), functor, params );
	}
	else
	{
		GCommonGame->GetGameplayStorage()->TQuery( functor, params );
	}
}

///////////////////////////////////////////////////////////////////////////////
