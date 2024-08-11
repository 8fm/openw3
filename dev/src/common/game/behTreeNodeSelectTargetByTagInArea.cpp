/*
* Copyright © 2015 CD Projekt Red. All Rights Reserved.
*/

#include "build.h"
#include "behTreeNodeSelectTargetByTagInArea.h"

#include "../engine/areaComponent.h"

BEHTREE_STANDARD_SPAWNDECORATOR_FUNCTION( CBehTreeNodeSelectTargetByTagInAreaDecoratorDefinition )

	
CNode* CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance::ComputeTarget()
{
	struct CollectorBase : public CTagManager::DefaultNodeIterator
	{
		CollectorBase( CAreaComponent* area )
			: m_testBBox( area->GetBoundingBox() )
			, m_testArea( area )
			, m_bestTarget( nullptr ) {}

		Bool AreaTest( const Vector& nodePos )
		{
			if ( !m_testBBox.Contains( nodePos ) )
			{
				return false;
			}
			return m_testArea->TestPointOverlap( nodePos );
		}

		Box				m_testBBox;
		CAreaComponent* m_testArea;
		CNode*			m_bestTarget;
	};

	struct CollectorRandom : public CollectorBase
	{
		CollectorRandom( CAreaComponent* area )
			: CollectorBase( area )
			, m_nodesFound( 0 )
			, m_randomGenerator( GEngine->GetRandomNumberGenerator() ) {}

		void Process( CNode* node, Bool isInitialList )
		{
			if ( !AreaTest( node->GetWorldPositionRef() ) )
			{
				return;
			}

			if ( ++m_nodesFound == 1 )
			{
				m_bestTarget = node;
			}
			else
			{
				if ( m_randomGenerator.Get< Uint32 >( m_nodesFound ) == 1 )
				{
					m_bestTarget = node;
				}
			}
		}

		Uint32			m_nodesFound;
		CStandardRand&	m_randomGenerator;
	};



	struct CollectorClosest : public CollectorBase
	{
		CollectorClosest( CAreaComponent* area, const Vector& position )
			: CollectorBase( area )
			, m_position( position )
			, m_closestSq( FLT_MAX ) {}

		void Process( CNode* node, Bool isInitialList )
		{
			const Vector& targetPos = node->GetWorldPositionRef();
			if ( !AreaTest( targetPos ) )
			{
				return;
			}

			Float distSq = targetPos.DistanceSquaredTo( m_position );
			if ( distSq < m_closestSq )
			{
				m_closestSq = distSq;
				m_bestTarget = node;
			}
		}

		Vector			m_position;
		Float			m_closestSq;
	};



	CAreaComponent* areaComponent = m_area.GetArea();
	if ( areaComponent )
	{
		CTagManager* tagManager = GGame->GetActiveWorld()->GetTagManager();

		if ( m_getClosest )
		{
			CollectorClosest collector( areaComponent, m_owner->GetActor()->GetWorldPositionRef() );
			tagManager->IterateTaggedNodes( m_tag, collector );
			return collector.m_bestTarget;
		}
		else
		{
			CollectorRandom collector( areaComponent );
			tagManager->IterateTaggedNodes( m_tag, collector );
			return collector.m_bestTarget;
		}
	}
	return nullptr;
}


Bool CBehTreeNodeSelectTargetByTagInAreaDecoratorInstance::Activate()
{
	if ( !m_reselectOnActivate )
	{
		CNode* node = m_node.Get();
		if ( node )
		{
			m_owner->SetActionTarget( node );
			return IBehTreeNodeDecoratorInstance::Activate();
		}
	}

	CNode* node = ComputeTarget();
	if ( node )
	{
		m_node = node;	
	}
	else if( !m_allowActivationWhenNoTarget )
	{
		return false;
	}
	m_owner->SetActionTarget( node );

	return IBehTreeNodeDecoratorInstance::Activate();
}