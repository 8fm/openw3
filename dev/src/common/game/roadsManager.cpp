/**
 * Copyright © 2014 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"
#include "roadsManager.h"
#include "../core/mathUtils.h"
#include "../engine/stripeComponent.h"
#include "../../common/engine/renderFrame.h"

IMPLEMENT_ENGINE_CLASS( CRoadsManager );

CRoadsManager::CRoadsManager()
	: m_lastSelectedRoad( nullptr )
	, m_wasOnTheRoad( false )
	, m_coolDownTimeout( 0.f )
{
}

CRoadsManager::SRoadParameters::SRoadParameters()
	: m_roadComponent( nullptr )
	, m_desiredDirectionDotProduct()
	, m_distanceToDesiredPoint( -1.f ) 
	, m_angleToDesiredPointTangent( -1.f )
	, m_roadScore( -1.f )
{
}

void CRoadsManager::Reset()
{
	m_currentCandidates.ClearFast();
	m_currentRoadData.m_collectedParameters.ClearFast();
	m_wasOnTheRoad = false;
	m_lastSelectedRoad = nullptr;

	m_coolDownTimeout = GGame->GetGameplayConfig().m_horseRoadFollowingCooldownTime;
}

void CRoadsManager::OnGameEnd( const CGameInfo& gameInfo )
{
	Reset();
}

void CRoadsManager::OnWorldEnd( const CGameInfo& gameInfo )
{
	Reset();
}

void CRoadsManager::RegisterRoad( CStripeComponent* road )
{
	m_roads.Add( road );
}

void CRoadsManager::UnregisterRoad( CStripeComponent* road )
{
	m_roads.Remove( road );
}

void CRoadsManager::Tick( Float timeDelta )
{
	if ( m_coolDownTimeout > 0.f )
	{
		m_coolDownTimeout -= timeDelta;
		m_coolDownTimeout = Max( m_coolDownTimeout, 0.f );
	}
}

Bool CRoadsManager::FindClosestRoad( const Vector& riderPosition, Float speed, Float maxAngle, Float maxDistance, Vector& riderDirection )
{
	m_currentCandidates.ClearFast();

	const SGameplayConfig& gameplayConfig = GGame->GetGameplayConfig();
	const Float lookAheadDistance = m_coolDownTimeout > 0.f ? gameplayConfig.m_horseRoadFollowingCooldownDistance : ( speed < 15.f ? gameplayConfig.m_horseRoadSearchDistanceSlow : gameplayConfig.m_horseRoadSearchDistanceFast );
	const Vector lookAheadPos = riderPosition + riderDirection * lookAheadDistance;
	const Float distanceCoeff = gameplayConfig.m_horseRoadSelectionDistanceCoeff;
	const Float angleCoeff = gameplayConfig.m_horseRoadSelectionAngleCoeff;
	const Float turnAmountFee = gameplayConfig.m_horseRoadSelectionTurnAmountFeeCoeff;
	const Float currentRoadReward = gameplayConfig.m_horseRoadSelectionCurrentRoadPreferenceCoeff;

	struct SSearchFunctor : public Red::System::NonCopyable
	{
		enum { SORT_OUTPUT = false };

		SSearchFunctor( CRoadsManager& manager,
						const Vector& referencePosition,
						const Vector& referenceDirection,
						const Vector& lookAheadPosition,
						Float distanceCoeff,
						Float angleCoeff,
						Float turnAmountFee,
						Float currentRoadReward,
						TDynArray< SRoadParameters >& candidates )
			: m_candidates( candidates )
			, m_roadsManager( manager )
			, m_referencePosition( referencePosition )
			, m_referenceDirection( referenceDirection )
			, m_lookAheadPosition( lookAheadPosition )
			, m_distanceCoeff( distanceCoeff )
			, m_angleCoeff( angleCoeff )
			, m_turnAmountFee( turnAmountFee )
			, m_currentRoadReward( currentRoadReward )
		{}

		RED_FORCE_INLINE Bool operator()( const TPointerWrapper< CStripeComponent >& stripe )
		{
			CRoadsManager::SRoadParameters newCandidate;
			
			CStripeComponent* const element = stripe.Get();
			newCandidate.m_roadComponent = element;
			m_roadsManager.EvaluateCandidate( *element, newCandidate, m_referencePosition, m_referenceDirection, m_lookAheadPosition, m_distanceCoeff, m_angleCoeff, m_turnAmountFee, m_currentRoadReward );
			m_candidates.PushBack( newCandidate );
			return true;
		}

		TDynArray< SRoadParameters >&	m_candidates;
		CRoadsManager&					m_roadsManager;
		const Vector&					m_referencePosition;
		const Vector&					m_referenceDirection;
		const Vector&					m_lookAheadPosition;
		const Float						m_distanceCoeff;
		const Float						m_angleCoeff;
		const Float						m_turnAmountFee;
		const Float						m_currentRoadReward;
	} functor( *this, riderPosition, riderDirection, lookAheadPos, distanceCoeff, angleCoeff, turnAmountFee, currentRoadReward, m_currentCandidates );

	const Float searchRadius = gameplayConfig.m_horseRoadSearchRadius;
	const Box searchBox( Vector( -searchRadius, -searchRadius, -2.f ), Vector( searchRadius, searchRadius, 2.f ) );
	m_roads.TQuery( lookAheadPos, functor, searchBox, true, nullptr, 0 );

	// saving current parameters for visual debug rendering
	m_currentRoadData.m_searchBox = searchBox;
	m_currentRoadData.m_lookAheadPosition = lookAheadPos;
	m_currentRoadData.m_referencePosition = riderPosition;
	m_currentRoadData.m_referenceDirection = riderDirection;
	m_currentRoadData.m_collectedParameters = m_currentCandidates;

	m_wasOnTheRoad = SelectBestDirectionToFollow( maxAngle, maxDistance, riderDirection );
	if ( !m_wasOnTheRoad )
	{
		m_lastSelectedRoad = nullptr;
	}

	return m_lastSelectedRoad != nullptr;
}

void CRoadsManager::EvaluateCandidate( CStripeComponent& element,
									   SRoadParameters& candidate,
									   const Vector& referencePosition,
									   const Vector& referenceDirection,
									   const Vector& lookAheadPosition,
									   Float distanceCoeff,
									   Float angleCoeff,
									   Float turnAmountFee,
									   Float currentRoadReward ) const
{
	const SMultiCurve& roadCurve = element.RequestCurve();
	const Vector2 preferableVelocity = referenceDirection.AsVector2();
	const Float roadWidth = element.GetStripeWidth() * 0.5f;

	// looking for the closest (to our current position) point on the road and compute our current distance from the center of this road
	Vector closestRoadPosition;
	SMultiCurvePosition closestPointIndex;
	roadCurve.GetClosestPointOnCurve( referencePosition, closestPointIndex, closestRoadPosition );

	// getting road direction
	Vector closestPointTangent;
	roadCurve.CalculateAbsoluteTangentFromCurveDirection( closestPointIndex, closestPointTangent );

	// if road direction is roughly opposite to our orientation (roads have one direction, but we go both directions), just negate it
	const Float dotProduct = preferableVelocity.Dot( closestPointTangent.AsVector2() );
	if ( dotProduct < 0 )
	{
		closestPointTangent.Negate();
	}

	// check which side of the road we're currently at
	const Float direction = dotProduct > 0.f ? 1.f : -1.f;

	roadCurve.GetClosestPointOnCurve( lookAheadPosition, candidate.m_desiredPointIndex, candidate.m_desiredPointPosition );

	// looking for the closest (to our predicted further position) point on the road and getting its direction
	roadCurve.CalculateAbsoluteTangentFromCurveDirection( candidate.m_desiredPointIndex, candidate.m_desiredPointTangent );

	if ( dotProduct < 0 )
	{
		candidate.m_desiredPointTangent.Negate();
	}

	const Vector2 lineEnd = closestRoadPosition + closestPointTangent.Negated() * 10.f;
	Vector2 output;
	const Float distanceFromRoad = MathUtils::GeometryUtils::DistancePointToLine2D( referencePosition.AsVector2(), closestRoadPosition.AsVector2(), lineEnd, output );

	// attempt to align with the right 'lane' of the road by shifting the desired position perpendicularly
	const Float desiredDistanceToRoad = Clamp( distanceFromRoad, 0.f, 0.7f * roadWidth );
	Vector2 perpendicularOffset = MathUtils::GeometryUtils::PerpendicularR( candidate.m_desiredPointTangent.AsVector2() );
	perpendicularOffset.Normalize();
	perpendicularOffset *= desiredDistanceToRoad;

	candidate.m_rightLanePosition = candidate.m_desiredPointPosition.AsVector3();
	const Bool fromLeft = closestPointTangent.AsVector2().CrossZ( referencePosition - closestRoadPosition ) >= 0.f;
	if ( !fromLeft )
	{
		candidate.m_rightLanePosition.AsVector2() += perpendicularOffset;
	}

	candidate.m_directionToDesiredPoint = referenceDirection;
	candidate.m_directionToDesiredPoint.AsVector2() = ( candidate.m_rightLanePosition - referencePosition ).Normalized2();
	candidate.m_distanceToDesiredPoint = candidate.m_rightLanePosition.DistanceTo( lookAheadPosition );

	// check whether the best point is, in fact, behind us
	candidate.m_desiredDirectionDotProduct = preferableVelocity.Dot( candidate.m_directionToDesiredPoint.AsVector2() );
	candidate.m_angleToDesiredPointTangent = Abs( MathUtils::VectorUtils::GetAngleDegAroundAxis( preferableVelocity, candidate.m_desiredPointTangent.AsVector2(), Vector::EZ ) );
	const Float rotationAmountToFollowRoad = Abs( MathUtils::VectorUtils::GetAngleDegAroundAxis( preferableVelocity, candidate.m_directionToDesiredPoint.AsVector2(), Vector::EZ ) );

	const Float rewardCoeff = candidate.m_roadComponent == m_lastSelectedRoad ? currentRoadReward : 1.f;
	candidate.m_roadScore = rotationAmountToFollowRoad * turnAmountFee * ( ( candidate.m_distanceToDesiredPoint * distanceCoeff ) + ( candidate.m_angleToDesiredPointTangent * angleCoeff ) ) / rewardCoeff;
}

Bool CRoadsManager::SelectBestDirectionToFollow( Float maxAngle, Float maxDistance, Vector& outDirection )
{
	Float bestScore = -1.f;
	for ( Uint32 i = 0; i < m_currentCandidates.Size(); ++i )
	{
		const SRoadParameters& parameters = m_currentCandidates[i];

 		if ( parameters.m_roadComponent != m_lastSelectedRoad )
 		{
 			if ( parameters.m_angleToDesiredPointTangent > maxAngle || parameters.m_distanceToDesiredPoint > maxDistance)
 				continue;
 		}
		// consider only roads with the similar direction (i.e. difference between angles isn't > maxAngle), not too far (within max distance) and skip those being behind
		if ( parameters.m_desiredDirectionDotProduct >= 0  && ( bestScore == -1 || ( bestScore > 0 &&  parameters.m_roadScore < bestScore ) ) )
		{
			outDirection = parameters.m_directionToDesiredPoint;
			bestScore = parameters.m_roadScore;
			m_lastSelectedRoad = parameters.m_roadComponent;
		}
	}

	return !m_currentCandidates.Empty() && bestScore != -1;
}

#ifndef NO_EDITOR
void CRoadsManager::OnGenerateDebugFragments( CRenderFrame* frame )
{
	if ( !frame->GetFrameInfo().IsShowFlagOn( SHOW_RoadFollowing ) )
		return;
	
	const Vector position = m_currentRoadData.m_referencePosition + Vector( 0, 0, 0.25f );

	// our position and preferable direction (may be current heading, may be something else computed from horse heading and/or input)
	frame->AddDebugSphere( position, 0.2f, Matrix::IDENTITY, Color::DARK_YELLOW );
	frame->AddDebugLineWithArrow( position, position + m_currentRoadData.m_referenceDirection, 1.0f, 0.2f, 0.2f, Color::CYAN );

	// our search bounding box => only intersecting stripe components were taken into consideration
	const Box searchBox( m_currentRoadData.m_searchBox.Min + m_currentRoadData.m_lookAheadPosition, m_currentRoadData.m_searchBox.Max + m_currentRoadData.m_lookAheadPosition );
	frame->AddDebugBox( searchBox, Matrix::IDENTITY, Color::BLUE );

	for ( Uint32 i = 0; i < m_currentRoadData.m_collectedParameters.Size(); ++i )
	{
		const SRoadParameters& parameters = m_currentRoadData.m_collectedParameters[i];
		const Bool isBestRoad = ( m_lastSelectedRoad == parameters.m_roadComponent );

		const Vector closestRoadPosition = parameters.m_desiredPointPosition + Vector( 0, 0, 0.3f );
		const Vector shiftedClosestPosition = parameters.m_rightLanePosition + Vector( 0, 0, 0.3f );

		// position and direction of the point on the road that is closest to us and its orientation (may be negated, because road has one direction but in fact we can move in the opposite as well)
		frame->AddDebugSphere( closestRoadPosition, 0.2f, Matrix::IDENTITY, isBestRoad ? Color::LIGHT_MAGENTA : Color::DARK_MAGENTA );
		frame->AddDebugLineWithArrow( closestRoadPosition, closestRoadPosition + parameters.m_desiredPointTangent, 1.0f, 0.2f, 0.2f, Color::CYAN );

		// shifted (to the center of the 'right lane') desired position
		frame->AddDebugSphere( shiftedClosestPosition, 0.1f, Matrix::IDENTITY, isBestRoad ? Color::LIGHT_RED : Color::DARK_RED );

		// evaluated score for this point (product of the distance and angle difference between preferable direction and point direction)
		frame->AddDebugText( closestRoadPosition, String::Printf( TXT("Distance: %.4f Angle %.4f Score: %.4f"), parameters.m_distanceToDesiredPoint, parameters.m_angleToDesiredPointTangent, parameters.m_roadScore ), false, Color::GREEN );

		if ( isBestRoad && parameters.m_roadComponent )
		{
			// our corrected direction => we should be heading towards the next (to the closest) point on this road
			frame->AddDebugLineWithArrow( position, position + parameters.m_directionToDesiredPoint, 1.0f, 0.2f, 0.2f, Color::GREEN );

			parameters.m_roadComponent->Highlight( frame );
		}
	}
}
#endif