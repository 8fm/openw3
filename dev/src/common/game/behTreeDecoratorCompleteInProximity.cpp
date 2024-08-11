/*
* Copyright © 2014 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeDecoratorCompleteInProximity.h"

#include "behTreeInstance.h"

///////////////////////////////////////////////////////////////////////////////
// TBehTreeNodeDecoratorCompleteInProximityInstance
///////////////////////////////////////////////////////////////////////////////
template< class FunDistance, class FunTarget >
void TBehTreeNodeDecoratorCompleteInProximityInstance< FunDistance, FunTarget >::Update()
{
	CNode* target = FunTarget::GetTarget( m_owner );
	if ( !target )
	{
		Complete( BTTO_FAILED );
		return;
	}
	const Vector& myPos = m_owner->GetActor()->GetWorldPositionRef();
	const Vector& targetPos = target->GetWorldPositionRef();
	if ( FunDistance::GetDistanceSq( myPos, targetPos ) < m_distanceSq )
	{
		Complete( BTTO_SUCCESS );
		return;
	}
	Super::Update();
}


///////////////////////////////////////////////////////////////////////////////
// CBehTreeNodeDecoratorCompleteInProximityDefinition
///////////////////////////////////////////////////////////////////////////////
IBehTreeNodeDecoratorInstance* CBehTreeNodeDecoratorCompleteInProximityDefinition::SpawnDecoratorInternal( CBehTreeInstance* owner, CBehTreeSpawnContext& context, IBehTreeNodeInstance* parent ) const
{
	struct Dist3D
	{
		static Float GetDistanceSq( const Vector& v0, const Vector& v1 )
		{
			return (v0 - v1).SquareMag3();
		}
	};
	struct Dist2D
	{
		static Float GetDistanceSq( const Vector& v0, const Vector& v1 )
		{
			return (v0.AsVector2() - v1.AsVector2()).SquareMag();
		}
	};
	struct ActionTarget
	{
		static CNode* GetTarget( CBehTreeInstance* owner )
		{
			return owner->GetActionTarget().Get();
		}
	};
	struct CombatTarget
	{
		static CNode* GetTarget( CBehTreeInstance* owner )
		{
			return owner->GetCombatTarget().Get();
		}
	};

	if ( m_3D )
	{
		if ( m_useCombatTarget )
		{
			return new TBehTreeNodeDecoratorCompleteInProximityInstance< Dist3D, CombatTarget >( *this, owner, context, parent );
		}
		else
		{
			return new TBehTreeNodeDecoratorCompleteInProximityInstance< Dist3D, ActionTarget >( *this, owner, context, parent );
		}
	}
	else 
	{
		if ( m_useCombatTarget )
		{
			return new TBehTreeNodeDecoratorCompleteInProximityInstance< Dist2D, CombatTarget >( *this, owner, context, parent );
		}
		else
		{
			return new TBehTreeNodeDecoratorCompleteInProximityInstance< Dist2D, ActionTarget >( *this, owner, context, parent );
		}
	}
}