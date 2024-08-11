
#include "build.h"
#include "animationSelectors.h"
#include "animationGameParams.h"
#include "../core/mathUtils.h"

void AnimationSelector_Trajectory::Init( ComponentAnimationIterator& animationIterator )
{
	for ( ; animationIterator; ++animationIterator )
	{
		const CSkeletalAnimationSetEntry* animEntry = *animationIterator;
		if ( animEntry )
		{
			const CSkeletalAnimationAttackTrajectoryParam* param = animEntry->FindParam< CSkeletalAnimationAttackTrajectoryParam >();

			Vector point;

			if ( param && param->IsParamValid() && param->GetSyncPointRightMS( point ) )
			{
				CName id;

				param->GetTagId( id );

				m_animations.PushBack( animEntry );
				m_points.PushBack( point );
				m_ids.PushBack( id );
			}
		}
	}
}

const CSkeletalAnimationSetEntry* AnimationSelector_Trajectory::DoSelect( const AnimationSelector_Trajectory::InputData& input, const Matrix& localToWorld ) const
{
	const CSkeletalAnimationSetEntry* best = NULL;
	Float bestDist = NumericLimits< Float >::Max();

	const Bool filterId = input.m_tagId != CName::NONE;

	const Uint32 size = m_points.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		if ( filterId && input.m_tagId != m_ids[ i ] )
		{
			continue;
		}

		const Vector& lPointMS = m_points[ i ];
		const Vector lPointWS = localToWorld.TransformPoint( lPointMS );

		const Float dist = lPointWS.DistanceSquaredTo( input.m_pointWS );

		if ( dist < bestDist )
		{
			bestDist = dist;
			best = m_animations[ i ];
		}
	}

	return best;
}

//////////////////////////////////////////////////////////////////////////

AnimationSelector_Blend2::AnimationSelector_Blend2()
{

}

void AnimationSelector_Blend2::Init( ComponentAnimationIterator& animationIterator )
{
	for ( ; animationIterator; ++animationIterator )
	{
		const CSkeletalAnimationSetEntry* animEntry = *animationIterator;
		if ( animEntry )
		{
			const CSkeletalAnimationAttackTrajectoryParam* param = animEntry->FindParam< CSkeletalAnimationAttackTrajectoryParam >();

			Vector point;

			if ( param && param->IsParamValid() && param->GetSyncPointRightMS( point ) )
			{
				CName id;

				param->GetTagId( id );

				m_animations.PushBack( animEntry );
				m_points.PushBack( point );
				m_ids.PushBack( id );
			}
		}
	}
}

Bool AnimationSelector_Blend2::DoSelect( const AnimationSelector_Blend2::InputData& input, const Matrix& localToWorld, AnimationSelector_Blend2::OutputData& output ) const
{
	TDynArray< Uint32 > indexes;

	const Uint32 size = m_animations.Size();
	indexes.Resize( size );

	for ( Uint32 i = 0; i < size; ++i )
	{
		indexes[i] = i;
	}

	return SelectFiltered( input, localToWorld, indexes, output );
}

Bool AnimationSelector_Blend2::SelectFiltered( const InputData& input, const Matrix& localToWorld, const TDynArray< Uint32 >& indexes, OutputData& output ) const
{
	const CSkeletalAnimationSetEntry* bestA = NULL;
	const CSkeletalAnimationSetEntry* bestB = NULL;

	Float bestDistA = NumericLimits< Float >::Max();
	Float bestDistB = NumericLimits< Float >::Max();

	Int32 bestIdxInSec = -1;

	const Bool filterId = input.m_tagId != CName::NONE;

	const Uint32 size = indexes.Size();
	ASSERT( size < 32 && size <= m_animations.Size() );

	TStaticArray< Vector, 32 > secondPassWS;
	TStaticArray< Uint32, 32 > secondPassIdx;

	for ( Uint32 i = 0; i < size; ++i )
	{
		const Uint32 index = indexes[i];
		ASSERT( index < m_animations.Size() );

		if ( filterId && input.m_tagId != m_ids[ index ] )
		{
			continue;
		}

		const Vector& lPointMS = m_points[ index ];
		const Vector lPointWS = localToWorld.TransformPoint( lPointMS );

		secondPassWS.PushBack( lPointWS );
		secondPassIdx.PushBack( index );

		const Float dist = lPointWS.DistanceSquaredTo( input.m_pointWS );

		if ( dist < bestDistA )
		{
			bestDistA = dist;
			bestIdxInSec = (Int32)secondPassIdx.Size() - 1;
			bestA = m_animations[ index ];
		}
	}

	Bool ret = false;
	Float weight = 0.f;

	if ( bestA )
	{
		ASSERT( secondPassIdx.Size() == secondPassWS.Size() );

		const Uint32 bestIdx = secondPassIdx[ bestIdxInSec ];
		const Vector bestPointWS = secondPassWS[ bestIdxInSec ];

		const Uint32 secSize = secondPassWS.Size();
		for ( Uint32 i=0; i<secSize; ++i )
		{
			const Uint32 index = secondPassIdx[ i ];

			if ( index == bestIdx )
			{
				continue;
			}

			const Vector& pointWS = secondPassWS[ i ];

			const Float p = MathUtils::GeometryUtils::ProjectVecOnEdgeUnclamped( input.m_pointWS, bestPointWS, pointWS );
			if ( p >= 0.f && p <= 1.f )
			{
				Vector pointOnEdge;
				MathUtils::GeometryUtils::GetPointFromEdge( p, bestPointWS, pointWS, pointOnEdge );

				const Float dist = pointOnEdge.DistanceSquaredTo( input.m_pointWS );

				if ( dist < bestDistB )
				{
					bestDistB = dist;
					bestB = m_animations[ i ];
					weight = p;
				}
			}
		}

		ret = bestA && bestB;
		if ( ret )
		{
			ASSERT( weight >= 0.f && weight <= 1.f );

			output.m_animationA = bestA;
			output.m_animationB = bestB;
			output.m_weight = weight;
		}
	}

	return ret;
}

//////////////////////////////////////////////////////////////////////////

AnimationSelector_HitPoint::AnimationSelector_HitPoint()
{

}

void AnimationSelector_HitPoint::Init( const TDynArray< const CSkeletalAnimationSetEntry* >& animations )
{
	const Uint32 size = animations.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const CSkeletalAnimationSetEntry* animEntry = animations[ i ];
		if ( animEntry )
		{
			const CSkeletalAnimationHitParam* param = animEntry->FindParam< CSkeletalAnimationHitParam >();
			if ( param && param->IsParamValid() )
			{
				m_animations.PushBack( animEntry );
				m_points.PushBack( param->GetPointMS() );
			}
		}
	}
}

void AnimationSelector_HitPoint::Init( ComponentAnimationIterator& animationIterator )
{
	for ( ; animationIterator; ++animationIterator )
	{
		const CSkeletalAnimationSetEntry* animEntry = *animationIterator;
		if ( animEntry )
		{
			const CSkeletalAnimationHitParam* param = animEntry->FindParam< CSkeletalAnimationHitParam >();
			if ( param && param->IsParamValid() )
			{
				m_animations.PushBack( animEntry );
				m_points.PushBack( param->GetPointMS() );
			}
		}
	}
}

const CSkeletalAnimationSetEntry* AnimationSelector_HitPoint::DoSelect( const AnimationSelector_HitPoint::InputData& input, const Matrix& localToWorld ) const
{
	const CSkeletalAnimationSetEntry* best = NULL;
	Float bestDist = NumericLimits< Float >::Max();

	const Uint32 size = m_points.Size();
	for ( Uint32 i=0; i<size; ++i )
	{
		const Vector& lPointMS = m_points[ i ];
		const Vector lPointWS = localToWorld.TransformPoint( lPointMS );

		const Float dist = lPointWS.DistanceSquaredTo( input.m_pointWS );

		if ( dist < bestDist )
		{
			bestDist = dist;
			best = m_animations[ i ];
		}
	}

	return best;
}

Vector AnimationSelector_Blend2Direction::GetDirectionFromType( EAnimationAttackType type ) const
{
	switch( type )
	{
		case AAT_46:
			return Vector( 1.f, 0.f, 0.f );
		case AAT_64:
			return Vector( -1.f, 0.f, 0.f );
		case AAT_82:
			return Vector( 0.f, -1.f, 0.f );
		case AAT_28:
			return Vector( 0.f, 1.f, 0.f );
		case AAT_73:
			return Vector( 0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f, 0.f );
		case AAT_91:
			return Vector( -0.70710678118654752440084436210485f, -0.70710678118654752440084436210485f, 0.f );
		case AAT_19:
			return Vector( 0.70710678118654752440084436210485f, 0.70710678118654752440084436210485f, 0.f );
		case AAT_37:
			return Vector( -0.70710678118654752440084436210485f, 0.70710678118654752440084436210485f, 0.f );
		default:
			return Vector::ZERO_3D_POINT;
	}
}

void AnimationSelector_Blend2Direction::Init( ComponentAnimationIterator& animationIterator )
{
	for ( ; animationIterator; ++animationIterator )
	{
		const CSkeletalAnimationSetEntry* animEntry = *animationIterator;
		if ( animEntry )
		{
			const CSkeletalAnimationAttackTrajectoryParam* param = animEntry->FindParam< CSkeletalAnimationAttackTrajectoryParam >();

			Vector point;

			if ( param && param->IsParamValid() && param->GetSyncPointRightMS( point ) )
			{
				CName id;

				param->GetTagId( id );

				m_animations.PushBack( animEntry );
				m_points.PushBack( point );
				m_types.PushBack( param->GetAttackType() );
				m_ids.PushBack( id );
			}
		}
	}
}

Bool AnimationSelector_Blend2Direction::DoSelect( const InputData& input, const Matrix& localToWorld, OutputData& output ) const
{
	TDynArray< Uint32 > indexes;
	TDynArray< EAnimationAttackType > calculatedTypes;

	const Uint32 size = m_animations.Size();
	indexes.Resize( size );

	Float maxDot = 0.f;
	EAnimationAttackType bestType = AAT_None;

	for ( Uint32 i = 0; i < size; ++i )
	{
		// if we calculated the dot for a type already, we don't need to do it again
		if( calculatedTypes.Exist( m_types[i] ) )
		{
			if( m_types[i] == bestType )
			{
				indexes.PushBack( i );
			}
		}
		else
		{
			const Vector directionWS = localToWorld.TransformVector( GetDirectionFromType( m_types[i] ) );
			const Float dot = MAbs( directionWS.Dot3( input.m_directionWS ) );
			if( dot > maxDot )
			{
				maxDot = dot;
				bestType = m_types[i];

				// clear all the previous ones, we found a better type
				indexes.ClearFast();
				indexes.PushBack( i );
			}

			calculatedTypes.PushBack( m_types[i] );
		}
	}

	return SelectFiltered( input, localToWorld, indexes, output );
}