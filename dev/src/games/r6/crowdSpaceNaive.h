/**
 * Copyright © 2013 CD Projekt Red. All Rights Reserved.
 */
#pragma once

#include "crowdAgent.h"

class CCrowdArea;

class CCrowdSpace_Naive
{
private:
	CCrowdManager*			m_manager;

private:
	// Get the number of agents stored in this crowd space
	RED_INLINE TAgentIndex GetNumAgents() const;

	// Get agent position
	RED_INLINE Vector2 GetAgentPos2( TAgentIndex agent ) const;

	// Get the actual Z of an agent's position
	RED_INLINE Float GetAgentZ( TAgentIndex agent ) const;

	// Get the bounding box of a specific agent
	RED_INLINE Box CalcAgentBox( TAgentIndex agent ) const;  

	// Frustum test - returns true if an agent overlaps with specified frustum
	RED_INLINE Bool FrustumTest( TAgentIndex agent, const CFrustum& frustum ) const { return 0 != frustum.TestBox( CalcAgentBox( agent ) ); }

	// Test if agent is active
	RED_INLINE Bool IsAgentActive( TAgentIndex agent ) const;

public:
	CCrowdSpace_Naive();

	// Called by crowd manager
	void OnInit( CCrowdManager* manager );

	// Called by crowd manager
	void Reset();

	// Called by crowd manager
	void UpdateSpace();

public:
	// Get all agents within the frustum (maximum: maxAgents). Returns number of agents in frustum.
	TAgentIndex GetAgentsInFrustum( const CFrustum& frustum, TAgentIndex maxAgents, TAgentIndex* result ) const;

	// Writes indices of a maximum n agents nearest to specified point. Returns actual number of agents found.
	TAgentIndex GetNNearestAgents( const Vector2& pos, TAgentIndex n, TAgentIndex* result ) const;

	// Writes indices of a maximum n agents nearest to specified point, but no futher away than radius. Returns actual number of agents found.
	TAgentIndex GetNearestAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const;	

	// Writes indices of a maximum n agents being within a radius from some point. Returns actual number of agents found.
	TAgentIndex GetAnyAgentsWithinRadius( const Vector2& pos, Float radius, TAgentIndex maxAgents, TAgentIndex* result ) const;	

	// Calculates number of agents currently residing within an area
	TAgentIndex GetAgentsCountWithinArea( const CCrowdArea* area ) const;	

	#ifndef NO_GET_AGENTS_AREA
		// Get the CrowdArea of a specific agent
		TAgentIndex GetAgentsAreaIndex( const SCrowdAgent& agent ) const;
		const CCrowdArea* GetAgentsArea( const SCrowdAgent& agent ) const;
	#endif

	// Raycast - test 2D ray against the crowd. Returns true if any crowd agent was hit.
	Bool Ray2Test_Any( const SCrowdRay2& ray ) const;

	// Raycast - test 2D ray against the crowd. Returns true if any crowd agent was hit, outputs index of an hit agent 
	// to hitAgent (index of an agent closest to ray.m_start who was hit by that ray). 
	Bool Ray2Test_Closest( const SCrowdRay2& ray, TAgentIndex& hitAgent ) const;

	// Raycast - test 2D ray against the crowd. Returns number of outputed agents. Outputs each hit agents index 
	// in order of a distance to ray.m_start, until maxAgents limit is reached (or all hit agents are outputed).
	TAgentIndex Ray2Test_NClosest( const SCrowdRay2& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const;

	// TODO: implement 3D versions:

	// Raycast - test 3D ray against the crowd. Returns true if any crowd agent was hit.
	// NOT IMPLEMENTED YET
	Bool Ray3Test_Any( const SCrowdRay3& ray ) const;

	// Raycast - test 3D ray against the crowd. Returns true if any crowd agent was hit, outputs index of an hit agent 
	// to hitAgent (index of an agent closest to ray.m_start who was hit by that ray). 
	// NOT IMPLEMENTED YET
	Bool Ray3Test_Closest( const SCrowdRay3& ray, TAgentIndex& hitAgent ) const;

	// Raycast - test 3D ray against the crowd. Returns number of outputed agents. Outputs each hit agents index 
	// in order of a distance to ray.m_start, until maxAgents limit is reached (or all hit agents are outputed).
	// NOT IMPLEMENTED YET
	TAgentIndex Ray3Test_NClosest( const SCrowdRay3& ray, TAgentIndex maxAgents, TAgentIndex* outputArray ) const;

	// Writes indices of a maximum maxObstacles obstacles near to specified point, but no futher away than radius. Returns actual number of obstacles found. 
	// The result is NOT sorted, so if you provide too small result buffer, it might happen that closest obstacles aren't there.
	TObstacleIndex GetObstaclesWithinRadius( const Vector2& pos, Float radius, TObstacleIndex maxObstacles, TObstacleIndex* result ) const;	
};
