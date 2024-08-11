/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "build.h"

#include "crowdAgent.h"
#include "crowdSpaceImplementation.h"

struct SCrowdObstacleEdge;

struct RVOLine 
{	
	Vector2 point;
	Vector2 direction;
};

// implementation of 'Optimal Reciprocal Collision Avoidance'
// algorithm http://gamma.cs.unc.edu/ORCA/
// implementation http://gamma.cs.unc.edu/RVO2/

class ORVOSolver
{
	static const Float			NEIGHBORHOOD_DISTANCE;
	static const Float			TIME_HORIZON;
	static const Float			OBSTACLE_TIME_HORIZON;
	static const Float			AGENT_RADIUS;
	static const Float			RVO_EPSILON;
	static const Float			MAX_OBSTACLE_DISTANCE;
	static const TAgentIndex	MAX_NEIGHBOURS;	
	static const TObstacleIndex	MAX_OBSTACLES;	

private:
	CCrowdManager*				m_manager;
	const CCrowdSpaceImpl*		m_crowdSpace;
	Float						m_deltaTime;

	static RED_INLINE Float Det( const Vector2 &vector1, const Vector2 &vector2 )
	{
		return vector1.X * vector2.Y - vector1.Y * vector2.X;
	}

	static RED_INLINE Float Sqr( const Float a )
	{
		return a * a;
	}

			void			ComputeNewVelocity( SCrowdAgent& agent, TAgentIndex *neighbors, TAgentIndex neighborAmount );
			RVOLine			ComputeAgentLine(  const SCrowdAgent* agent, const SCrowdAgent* other );
			Bool			ComputeObstacleLine( const SCrowdAgent* agent, const SCrowdObstacleEdge* obstacle, RVOLine* obstacleORCALines, Int32 numOfLones, RVOLine& outLine );
	static	TAgentIndex		LinearProgram2( RVOLine* lines , TAgentIndex neighborAmount, Float radius, const Vector2 &optVelocity, Bool directionOpt, Vector2 &result );
	static	void			LinearProgram3( RVOLine* lines, Int16 obstacleLinesAmount,  Int16 linesAmount, TAgentIndex beginLine, Float radius, Vector2 &result );
	static	Bool			LinearProgram1( RVOLine* lines, TAgentIndex lineNo, Float radius, const Vector2 &optVelocity, Bool directionOpt, Vector2 &result );
	

public:
	RED_INLINE void		SetCrowdManager( CCrowdManager* mgr )					{ m_manager = mgr; }
	RED_INLINE void		SetCrowdSpaceImpl( const CCrowdSpaceImpl* crowdSpace )	{ m_crowdSpace = crowdSpace; }
	RED_INLINE void		SetDeltaTime( Float deltaTime )							{ m_deltaTime = deltaTime; }

				void		UpdateAgent( const SCrowdAgent& input, SCrowdAgent& output );
				void		UpdateAgent( SCrowdAgent& agent );
};