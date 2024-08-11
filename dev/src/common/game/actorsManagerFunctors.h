#pragma once

#include "build.h"
#include "binaryStorage.h"
#include "actorsManager.h"
#include "../core/mathUtils.h"

namespace ActorsManagerFunctors
{
	struct LineTestFunctor : Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = false };

		LineTestFunctor( const Vector2& posFrom, const Vector2& posTo, Float radius, Bool ignoreGhostCharacters = false )
			: m_posFrom( posFrom )
			, m_posTo( posTo )
			, m_radius( radius )
			, m_ignoreGhostCharacters( ignoreGhostCharacters )
			, m_hasHit( false ) {}

		Bool operator()( const CActorsManagerMemberData& member )
		{
			if ( !m_hasHit )
			{
				CActor* actor = member.Get();
				Float radius = actor->GetRadius() + m_radius;
				if ( MathUtils::GeometryUtils::TestIntersectionCircleLine2D( actor->GetWorldPositionRef().AsVector2(), radius, m_posFrom, m_posTo ) )
				{
					if ( m_ignoreGhostCharacters )
					{
						CMovingAgentComponent* mac = actor->GetMovingAgentComponent();
						if ( !mac || !mac->IsCharacterCollisionsEnabled() )
						{
							return true;
						}
					}

					m_hasHit = true;			
					return false;
				}
			}
			return true;
		}
		Vector2			m_posFrom;
		Vector2			m_posTo;
		Float			m_radius;
		Bool			m_ignoreGhostCharacters;

		Bool			m_hasHit;
	};

	struct LineTestIgnoreActorFunctor : public LineTestFunctor
	{
		LineTestIgnoreActorFunctor( const Vector2& posFrom, const Vector2& posTo, Float radius, CActor* ignoreActor, Bool ignoreGhostCharacters = false )
			: LineTestFunctor( posFrom, posTo, radius, ignoreGhostCharacters )
			, m_ignoreActor( ignoreActor ) {}
		RED_INLINE Bool operator()( const CActorsManagerMemberData& member )
		{
			CActor* actor = member.Get();
			if ( actor == m_ignoreActor )
			{
				return true;
			}
			return LineTestFunctor::operator()( member );
		}
		const CActor*			m_ignoreActor;
	};
	struct LineTestIgnoreMultipleFunctor : public LineTestFunctor
	{
		LineTestIgnoreMultipleFunctor( const Vector2& posFrom, const Vector2& posTo, Float radius, CActor** ignoreActor, Uint32 ignoredActorsCount, Bool ignoreGhostCharacters = false )
			: LineTestFunctor( posFrom, posTo, radius, ignoreGhostCharacters )
			, m_ignoreActor( ignoreActor )
			, m_ignoreActorsCount( ignoredActorsCount ) {}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& member )
		{
			CActor* actor = member.Get();
			for ( Uint32 i = 0; i < m_ignoreActorsCount; ++i )
			{
				if ( actor == m_ignoreActor[ i ] )
				{
					return true;
				}
			}
			return LineTestFunctor::operator()( member );
		}

		CActor**				m_ignoreActor;
		const Uint32			m_ignoreActorsCount;
	};

	struct CollectActorsFunctor : public CQuadTreeStorage< CActor, CActorsManagerMemberData >::DefaultFunctor
	{
		CollectActorsFunctor( TDynArray< TPointerWrapper< CActor > >& output, Int32 maxElems )
			: m_output( output )
			, m_maxElems( maxElems )										{}

		enum { SORT_OUTPUT = true };

		RED_INLINE Bool operator()( const CActorsManagerMemberData& element )
		{
			m_output.PushBack( element.Get() );
			return (--m_maxElems) > 0;
		}

		TDynArray< TPointerWrapper< CActor > >&					m_output;
		Int32														m_maxElems;
	};

	struct CollectActorsOnLineFunctor : public LineTestFunctor
	{
		enum { SORT_OUTPUT = true };

		CollectActorsOnLineFunctor( CActor** outputArray, Int32 maxElems, const Vector2& posFrom, const Vector2& posTo, Float radius )
			: LineTestFunctor( posFrom, posTo, radius )
			, m_outputArray( outputArray )
			, m_nextElem( 0 )
			, m_maxElems( maxElems ){}

		RED_INLINE Bool operator()( const CActorsManagerMemberData& element )
		{
			if( m_nextElem >= m_maxElems )
			{
				return false;
			}

			m_hasHit = false;
			LineTestFunctor::operator()( element );
			if( m_hasHit )
			{
				m_outputArray[ m_nextElem++] = element.Get();
				return  m_nextElem < m_maxElems;
			}

			return true;
		}

		CActor**			m_outputArray;
		Int32				m_nextElem;
		Int32				m_maxElems;
	};
}