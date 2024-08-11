/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */

#include "build.h"

#include "orvoSolver.h"
#include "crowdManager.h"
#include "crowdEntryPoint.h"
#include "crowdArea.h"
#include "crowdObstacle.h"

const Float ORVOSolver::NEIGHBORHOOD_DISTANCE			= 10.f;
const Float ORVOSolver::TIME_HORIZON					= 4.f;
const Float ORVOSolver::OBSTACLE_TIME_HORIZON			= 1.f;
const Float ORVOSolver::AGENT_RADIUS					= 0.5f;
const Float ORVOSolver::MAX_OBSTACLE_DISTANCE			= 2.5f;
const Float ORVOSolver::RVO_EPSILON						= 0.00001f;
const TObstacleIndex ORVOSolver::MAX_OBSTACLES			= 32;
const TAgentIndex ORVOSolver::MAX_NEIGHBOURS			= 10;


void ORVOSolver::UpdateAgent( SCrowdAgent& agent )
{
	static const Float TARGET_TOLERANCE_SQRT = 3.f * 3.f;

	if ( false == agent.IsSimulated() )
	{
		return;
	}

	if ( agent.DistanceToTargetSqr() <= TARGET_TOLERANCE_SQRT )
	{
		const CCrowdArea* currentArea = m_manager->GetAreaByIndex( agent.m_area );
		CCrowdEntryPoint* entry = currentArea->RandomEntry();

		if ( entry )
		{
			agent.m_tgt = entry->RandomPositionInside2();			
			agent.AssignRandomVelToTarget();
		}
	}	
	else
	{
		agent.m_prefVelocity = ( agent.m_tgt - agent.m_pos ).Normalized() * agent.m_prefVelocity.Mag();
	}

	TAgentIndex neighbors[ MAX_NEIGHBOURS ];	

	TAgentIndex foundNeighbors = m_crowdSpace->GetNearestAgentsWithinRadius( agent.m_pos, NEIGHBORHOOD_DISTANCE, MAX_NEIGHBOURS, neighbors );
	ComputeNewVelocity( agent, neighbors, foundNeighbors );
	if ( agent.m_newVel.SquareMag() > Sqr( SCrowdAgent::AGENT_MAX_SPEED ) )
	{
		agent.m_newVel = agent.m_newVel.Normalized() * SCrowdAgent::AGENT_MAX_SPEED;
	}
}

void ORVOSolver::UpdateAgent( const SCrowdAgent& input, SCrowdAgent& output )
{
	UpdateAgent( output );
}

void ORVOSolver::ComputeNewVelocity( SCrowdAgent& agent, TAgentIndex *neighbors, TAgentIndex neighborAmount )
{	
	RVOLine orcaLines[ MAX_NEIGHBOURS + MAX_OBSTACLES ];	
	Int16 numOfLines = 0;

	TObstacleIndex obstacles[ MAX_OBSTACLES ];
	TObstacleIndex numObstacles = m_crowdSpace->GetObstaclesWithinRadius( agent.m_pos, MAX_OBSTACLE_DISTANCE, MAX_OBSTACLES, obstacles );
	#ifdef DEBUG_CROWD
		ASSERT( numObstacles < MAX_OBSTACLES, TXT("MAX_OBSTACLES apparently is a too low number. Consider increasing it, or lowering the search radius (or consider both).") );
	#endif

	for ( TObstacleIndex i = 0; i < numObstacles; ++i )
	{
		SCrowdObstacleEdge& obstacle = m_manager->GetObstacle( obstacles[ i ] );
		if ( obstacle.IsValid() )
		{
			if ( ComputeObstacleLine( &agent, &obstacle, orcaLines, numOfLines, orcaLines[ numOfLines ] ) )
			{
 				++numOfLines;
			}
		}

		if ( numOfLines >= MAX_OBSTACLES )
		{
			break;
		}
	}

	const Int16 numOfObstacleLines = numOfLines;
	for ( TAgentIndex i = 0; i < neighborAmount; ++i ) 
	{		
		const SCrowdAgent* const other = m_manager->GetAgentIn( neighbors[ i ] );
		if ( other ==  &agent || !other->IsActive() ) 
		{		
			continue;
		}
		
		orcaLines[ numOfLines++ ] = ComputeAgentLine( &agent, other );;
	}

	const TAgentIndex lineFail = LinearProgram2( orcaLines, numOfLines, SCrowdAgent::AGENT_MAX_SPEED, agent.m_prefVelocity, false, agent.m_newVel );
	if ( lineFail < neighborAmount ) 
	{
		LinearProgram3( orcaLines, numOfObstacleLines, numOfLines, lineFail, SCrowdAgent::AGENT_MAX_SPEED, agent.m_newVel );
	}
}


RVOLine ORVOSolver::ComputeAgentLine( const SCrowdAgent* agent, const SCrowdAgent* other )
{
	static const Float radius = CCrowdManager::AGENT_RADIUS * 1.2f;
	static const Float invTimeHorizon = 1.0f / TIME_HORIZON;

	const Vector2 relativePosition = other->m_pos - agent->m_pos;
	const Vector2 relativeVelocity = agent->m_vel - other->m_vel;
	const Float distSq = relativePosition.SquareMag( );
	const Float combinedRadius = 2*radius;
	const Float combinedRadiusSq = Sqr( combinedRadius );

	RVOLine line;
	Vector2 u;

	if ( distSq > combinedRadiusSq ) 
	{
		// No collision
		const Vector2 w = relativeVelocity - relativePosition * invTimeHorizon;
		// Vector from cutoff center to relative velocity
		const Float wLengthSq = w.SquareMag( );

		const Float dotProduct1 = w.Dot( relativePosition );

		if ( dotProduct1 < 0.0f && Sqr( dotProduct1 ) > combinedRadiusSq * wLengthSq )
		{
			//Project on cut-off circle
			const Float wLength = MSqrt( wLengthSq );
			const Vector2 unitW = w / wLength;

			line.direction = Vector2( unitW.Y, -unitW.X );
			u = unitW * (combinedRadius * invTimeHorizon - wLength);
		}
		else 
		{
			//Project on legs
			const Float leg = MSqrt( distSq - combinedRadiusSq );

			if ( Det( relativePosition, w ) > 0.0f ) 
			{
				//Project on left leg
				line.direction = Vector2( relativePosition.X * leg - relativePosition.Y * combinedRadius, relativePosition.X * combinedRadius + relativePosition.Y * leg) / distSq;
			}
			else 
			{
				//Project on right leg
				line.direction = -Vector2( relativePosition.X * leg + relativePosition.Y * combinedRadius, -relativePosition.X * combinedRadius + relativePosition.Y * leg) / distSq;
			}

			const Float dotProduct2 = relativeVelocity.Dot( line.direction );

			u = line.direction * dotProduct2 - relativeVelocity;
		}
	}
	else 
	{
		//Collision. Project on cut-off circle of time timeStep
		const Float invTimeStep = 1.0f / m_deltaTime;

		//Vector from cutoff center to relative velocity
		const Vector2 w = relativeVelocity - relativePosition * invTimeStep;

		const Float wLength = w.Mag();
		const Vector2 unitW = w / wLength;

		line.direction = Vector2( unitW.Y, -unitW.X );
		u = unitW * (combinedRadius * invTimeStep - wLength);
	}

	line.point = agent->m_vel + u * agent->ComputeCollisionAvoidence( other );

	return line;
}

Bool ORVOSolver::ComputeObstacleLine( const SCrowdAgent* agent, const SCrowdObstacleEdge* obstacle, RVOLine* obstacleORCALines, Int32 numOfLines, RVOLine& outLine )
{	
	const Float invTimeHorizonObst = 1.0f / OBSTACLE_TIME_HORIZON;
	static const Float radius = CCrowdManager::AGENT_RADIUS * 1.2f; //obstalce collision radius

	const SCrowdObstacleEdge* nextObstacle = &m_manager->GetObstacle( obstacle->m_nextEdge );

	const Vector2 relativePosition1 = obstacle->m_begin - agent->m_pos;
	const Vector2 relativePosition2 = nextObstacle->m_begin - agent->m_pos;

	/*
	* Check if velocity obstacle of obstacle is already taken care of by
	* previously constructed obstacle ORCA lines.
	*/			
	for ( Int16 j = 0; j < numOfLines; ++j ) 
	{
		if (	Det( relativePosition1 * invTimeHorizonObst - obstacleORCALines[ j ].point, obstacleORCALines[ j ].direction ) - invTimeHorizonObst * radius >= -RVO_EPSILON 
			&&	Det( relativePosition2 * invTimeHorizonObst - obstacleORCALines[ j ].point, obstacleORCALines[ j ].direction ) - invTimeHorizonObst * radius >= -RVO_EPSILON ) 
		{
			return false;
		}
	}
			
	const Float distSq1 = relativePosition1.SquareMag();
	const Float distSq2 = relativePosition2.SquareMag();
	const Float radiusSq = Sqr( CCrowdManager::AGENT_RADIUS );
	const Vector2 obstacleVector = nextObstacle->m_begin - obstacle->m_begin;
	const Float s = -relativePosition1.Dot( obstacleVector ) / obstacleVector.SquareMag();
	const Float distSqLine = ( -relativePosition1 - obstacleVector * s ).SquareMag();
			
	if ( s < 0.0f && distSq1 <= radiusSq ) 
	{
		/* Collision with left vertex. Ignore if non-convex. */
		if ( obstacle->IsConvex() ) 
		{
			outLine.point = Vector2( 0.0f, 0.0f );
			outLine.direction = Vector2( -relativePosition1.Y, relativePosition1.X );
			outLine.direction.Normalize();
			return true;
		}

		return false;
	}
	else if ( s > 1.0f && distSq2 <= radiusSq ) 
	{
		/* Collision with right vertex. Ignore if non-convex
			* or if it will be taken care of by neighoring obstace */
		if ( nextObstacle->IsConvex() && Det( relativePosition2, nextObstacle->m_direction ) >= 0.0f ) 
		{
			outLine.point = Vector2(0.0f, 0.0f);
			outLine.direction = Vector2(-relativePosition2.Y, relativePosition2.X );
			outLine.direction.Normalize();
			return true;
		}

		return false;
	}
	else if ( s >= 0.0f && s < 1.0f && distSqLine <= radiusSq ) 
	{
		/* Collision with obstacle segment. */
		outLine.point = Vector2( 0.0f, 0.0f );
		outLine.direction = -obstacle->m_direction;
		return true;		
	}

	/*
		* No collision.
		* Compute legs. When obliquely viewed, both legs can come from a single
		* vertex. Legs extend cut-off line when nonconvex vertex.
		*/

	Vector2 leftLegDirection, rightLegDirection;

	if ( s < 0.0f && distSqLine <= radiusSq ) 
	{
		/*
			* Obstacle viewed obliquely so that left vertex
			* defines velocity obstacle.
			*/
		if ( !obstacle->IsConvex() ) 
		{
			return false;
		}

		nextObstacle = obstacle;

		const Float leg1 = MSqrt( distSq1 - radiusSq );
		leftLegDirection = Vector2( relativePosition1.X * leg1 - relativePosition1.Y * radius, relativePosition1.X * radius + relativePosition1.Y * leg1 ) / distSq1;
		rightLegDirection = Vector2( relativePosition1.X * leg1 + relativePosition1.Y * radius, -relativePosition1.X * radius + relativePosition1.Y * leg1 ) / distSq1;
	}
	else if ( s > 1.0f && distSqLine <= radiusSq ) 
	{
		/*
			* Obstacle viewed obliquely so that
			* right vertex defines velocity obstacle.
			*/
		if ( !nextObstacle->IsConvex() ) 
		{
			/* Ignore obstacle. */
			return false;
		}

		obstacle = nextObstacle;

		const Float leg2 =	MSqrt( distSq2 - radiusSq );
		leftLegDirection =	Vector2( relativePosition2.X * leg2 - relativePosition2.Y * radius, relativePosition2.X * radius + relativePosition2.Y * leg2 ) / distSq2;
		rightLegDirection = Vector2( relativePosition2.X * leg2 + relativePosition2.Y * radius, -relativePosition2.X * radius + relativePosition2.Y * leg2 ) / distSq2;
	}
	else 
	{
		/* Usual situation. */
		if ( obstacle->IsConvex() ) 
		{
			const Float leg1 = MSqrt( distSq1 - radiusSq );
			leftLegDirection = Vector2( relativePosition1.X * leg1 - relativePosition1.Y * radius, relativePosition1.X * radius + relativePosition1.Y * leg1 ) / distSq1;
		}
		else 
		{
			/* Left vertex non-convex; left leg extends cut-off line. */
			leftLegDirection = -obstacle->m_direction;
		}

		if ( nextObstacle->IsConvex() ) 
		{
			const Float leg2 = MSqrt( distSq2 - radiusSq );
			rightLegDirection = Vector2( relativePosition2.X * leg2 + relativePosition2.Y * radius, -relativePosition2.X * radius + relativePosition2.Y * leg2 ) / distSq2;
		}
		else 
		{
			/* Right vertex non-convex; right leg extends cut-off line. */
			rightLegDirection = obstacle->m_direction;
		}
	}

	/*
		* Legs can never point into neighboring edge when convex vertex,
		* take cutoff-line of neighboring edge instead. If velocity projected on
		* "foreign" leg, no constraint is added.
		*/

	const SCrowdObstacleEdge *const leftNeighbor = &m_manager->GetObstacle( obstacle->m_prevEdge );

	Bool isLeftLegForeign = false;
	Bool isRightLegForeign = false;

	if ( obstacle->IsConvex() && Det( leftLegDirection, -leftNeighbor->m_direction ) >= 0.0f ) 
	{
		/* Left leg points into obstacle. */
		leftLegDirection = -leftNeighbor->m_direction;
		isLeftLegForeign = true;
	}

	if ( nextObstacle->IsConvex() && Det( rightLegDirection, nextObstacle->m_direction ) <= 0.0f) 
	{
		/* Right leg points into obstacle. */
		rightLegDirection = nextObstacle->m_direction;
		isRightLegForeign = true;
	}

	/* Compute cut-off centers. */
	const Vector2 leftCutoff = ( obstacle->m_begin - agent->m_pos ) * invTimeHorizonObst;
	const Vector2 rightCutoff = ( nextObstacle->m_begin - agent->m_pos ) * invTimeHorizonObst;
	const Vector2 cutoffVec = rightCutoff - leftCutoff;

	/* Project current velocity on velocity obstacle. */

	/* Check if current velocity is projected on cutoff circles. */
	const Float t = ( obstacle == nextObstacle ? 0.5f : ( agent->m_vel - leftCutoff ).Dot( cutoffVec ) / cutoffVec.SquareMag() );
	const Float tLeft = ( ( agent->m_vel - leftCutoff ).Dot( leftLegDirection ) );
	const Float tRight = ( ( agent->m_vel - rightCutoff ).Dot( rightLegDirection ) );

	if( ( t < 0.0f && tLeft < 0.0f ) || ( obstacle == nextObstacle && tLeft < 0.0f && tRight < 0.0f ) ) 
	{
		/* Project on left cut-off circle. */
		const Vector2 unitW = ( agent->m_vel - leftCutoff ).Normalized();

		outLine.direction = Vector2( unitW.Y, -unitW.X );
		outLine.point = leftCutoff + unitW * radius * invTimeHorizonObst;				
		return true;
	}
	else if (t > 1.0f && tRight < 0.0f) 
	{
		/* Project on right cut-off circle. */
		const Vector2 unitW = ( agent->m_vel - rightCutoff).Normalized();

		outLine.direction = Vector2( unitW.Y, -unitW.X );
		outLine.point = rightCutoff + unitW * radius * invTimeHorizonObst;			
		return true;
	}

	/*
		* Project on left leg, right leg, or cut-off line, whichever is closest
		* to velocity.
		*/
	const Float distSqCutoff = ((t < 0.0f || t > 1.0f || obstacle == nextObstacle ) ? std::numeric_limits<float>::infinity() : ( agent->m_vel - (leftCutoff +  cutoffVec * t ) ).SquareMag() );
	const Float distSqLeft = ((tLeft < 0.0f) ? std::numeric_limits<float>::infinity() : ( agent->m_vel - (leftCutoff + leftLegDirection * tLeft )).SquareMag() );
	const Float distSqRight = ((tRight < 0.0f) ? std::numeric_limits<float>::infinity() : ( agent->m_vel - (rightCutoff + rightLegDirection * tRight ) ).SquareMag() );

	if( distSqCutoff <= distSqLeft && distSqCutoff <= distSqRight ) 
	{
		/* Project on cut-off line. */
		outLine.direction = -obstacle->GetDirection();
		outLine.point = leftCutoff + Vector2( -outLine.direction.Y, outLine.direction.X ) * radius * invTimeHorizonObst;				
		return true;
	}
	else if (distSqLeft <= distSqRight) 
	{
		/* Project on left leg. */
		if( isLeftLegForeign ) 
		{
			return false;
		}

		outLine.direction = leftLegDirection;
		outLine.point = leftCutoff + Vector2( -outLine.direction.Y, outLine.direction.X ) * radius * invTimeHorizonObst;				
		return true;
	}
	else 
	{
		/* Project on right leg. */
		if (isRightLegForeign)
		{
			return false;
		}

		outLine.direction = -rightLegDirection;
		outLine.point = rightCutoff + Vector2( -outLine.direction.Y, outLine.direction.X ) * radius * invTimeHorizonObst;				
		return true;
	}	
}

Bool ORVOSolver::LinearProgram1( RVOLine* lines, TAgentIndex lineNo, Float radius, const Vector2 &optVelocity, Bool directionOpt, Vector2 &result )
{
	const Float dotProduct = lines[ lineNo ].point.Dot( lines[ lineNo ].direction );
	const Float discriminant = Sqr( dotProduct ) + Sqr( radius ) - lines[ lineNo ].point.SquareMag();

	if ( discriminant < 0.0f ) 
	{
		//Max speed circle fully invalidates line lineNo
		return false;
	}

	const Float sqrtDiscriminant = MSqrt( discriminant );
	Float tLeft = -dotProduct - sqrtDiscriminant;
	Float tRight = -dotProduct + sqrtDiscriminant;

	for ( TAgentIndex i = 0; i < lineNo; ++i ) 
	{
		const Float denominator = Det (lines[ lineNo ].direction, lines[ i ].direction );
		const Float numerator = Det( lines[ i ].direction, lines[ lineNo ].point - lines[ i ].point );

		if ( Abs( denominator ) <= RVO_EPSILON ) 
		{
			//Lines lineNo and i are (almost) parallel
			if ( numerator < 0.0f ) 
			{
				return false;
			}
			else 
			{
				continue;
			}
		}

		const Float t = numerator / denominator;

		if ( denominator >= 0.0f ) 
		{
			//Line i bounds line lineNo on the right
			tRight = Min( tRight, t );
		}
		else 
		{
			//Line i bounds line lineNo on the left
			tLeft = Max( tLeft, t );
		}

		if ( tLeft > tRight ) 
		{
			return false;
		}
	}

	if ( directionOpt ) 
	{
		//Optimize direction
		if ( optVelocity.Dot( lines[ lineNo ].direction ) > 0.0f ) 
		{
			//Take right extreme
			result = lines[ lineNo ].point + lines[ lineNo ].direction * tRight;
		}
		else 
		{
			//Take left extreme
			result = lines[ lineNo ].point + lines[ lineNo ].direction * tLeft;
		}
	}
	else
	{
		//Optimize closest point
		const Float t = lines[ lineNo ].direction.Dot( optVelocity - lines[ lineNo ].point );

		if ( t < tLeft ) 
		{
			result = lines[ lineNo ].point + lines[ lineNo ].direction * tLeft;
		}
		else if ( t > tRight ) 
		{
			result = lines[ lineNo ].point + lines[ lineNo ].direction * tRight;
		}
		else 
		{
			result = lines[ lineNo ].point + lines[ lineNo ].direction * t;
		}
	}

	return true;
}

TAgentIndex ORVOSolver::LinearProgram2( RVOLine* lines , TAgentIndex neighborAmount, Float radius, const Vector2 &optVelocity, Bool directionOpt, Vector2 &result )
{
	if (directionOpt) 
	{					
		result = optVelocity * radius;
	}
	else if ( optVelocity.SquareMag() > Sqr( radius ) ) 
	{
		// preferred velocity is to hight
		result = optVelocity.Normalized() * radius;
	}
	else 
	{		
		result = optVelocity;
	}

	for ( TAgentIndex i = 0; i < neighborAmount; ++i ) 
	{
		if ( Det( lines[ i ].direction, lines[ i ].point - result ) > 0.0f ) 
		{
			//Result does not satisfy constraint i. Compute new optimal result
			const Vector2 tempResult = result;

			if ( !LinearProgram1( lines, i, radius, optVelocity, directionOpt, result ) ) 
			{
				result = tempResult;
				return i;
			}
		}
	}

	return neighborAmount;
}

void ORVOSolver::LinearProgram3( RVOLine* lines , Int16 obstacleLinesAmount,  Int16 linesAmount, TAgentIndex beginLine, Float radius, Vector2 &result)
{
	Float distance = 0.0f;

	for ( Int16 i = beginLine; i < linesAmount; ++i ) 
	{
		if ( Det( lines[ i ].direction, lines[ i ].point - result) > distance ) 
		{
			// Result does not satisfy constraint of line i

			RVOLine projLines[ MAX_NEIGHBOURS + MAX_OBSTACLES ];
			// Rewrite lines from obstacles - these can not be relaxed
			for( Int16 j = 0; j < obstacleLinesAmount; ++j )
			{
				projLines[ j ] = lines[ j ];
			}
			TAgentIndex linesCounter = obstacleLinesAmount;
			for ( Int16 j = obstacleLinesAmount; j < i; ++j ) 
			{
				RVOLine line;

				Float determinant = Det( lines[ i ].direction, lines[ j ].direction );

				if ( Abs( determinant ) <= RVO_EPSILON ) 
				{
					// Line i and line j are parallel
					if ( lines[ i ].direction.Dot( lines[ j ].direction ) > 0.0f )
					{
						// Line i and line j point in the same direction
						continue;
					}
					else 
					{
						// Line i and line j point in opposite direction
						line.point = ( lines[ i ].point + lines[ j ].point ) * 0.5f;
					}
				}
				else 
				{
					line.point = lines[i].point + lines[ i ].direction * ( Det(lines[ j ].direction, lines[ i ].point - lines[ j ].point) / determinant );
				}

				line.direction = ( lines[ j ].direction - lines[ i ].direction).Normalized();
				projLines[ linesCounter++ ] = line;
			}

			const Vector2 tempResult = result;

			if ( LinearProgram2( projLines, linesCounter, radius, Vector2( -lines[ i ].direction.Y, lines[ i ].direction.X ), true, result ) < linesCounter ) 
			{				
				result = tempResult;
			}

			distance = Det( lines[ i ].direction, lines[ i ].point - result );
		}
	}
}
