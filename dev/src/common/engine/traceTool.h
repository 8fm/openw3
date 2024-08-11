/**
 * Copyright © 2009 CD Projekt Red. All Rights Reserved.
 */
#pragma once

// Marks invalid Z - trace tool was unable to compute valid one
#define INVALID_Z FLT_MAX
#define MAC_TRACE_TEST_RADIUS	0.2f

struct SPhysicsContactInfo;
struct SPhysicsOverlapInfo;
class CRenderFrame;

// Result of trace
struct TraceResultPlacement
{
	Float				m_height;					//!< Target height
	Int8				m_soundMaterial;			//!< Sound material used
	Bool				m_isTerrainCollision : 1;	//!< True when trace collides with terrain
	Bool				m_isValid			 : 1;	//!< True when valid

	TraceResultPlacement()
		: m_height( FLT_MAX )
		, m_soundMaterial( -1 /*default */ )
		, m_isValid( false )
		, m_isTerrainCollision( false )
	{};
};

// Trace tool
class CTraceTool
{
public:
	//! Static trace test that checks the height at which we can put an agent
	static Bool StaticAgentPlacementTraceTest( const class CWorld* world, const Vector& position, Float radius, TraceResultPlacement& result )
	{
		if ( !world )
		{
			return false;
		}
		// No valid hit found
		return false;
	}

	//! Static trace test ( not cached, but good for one time use anyway, returns true if result is valid )
	static Bool StaticPlacementTraceTest( class CWorld* world, const Vector& position, Float radius, TraceResultPlacement& result )
	{
		return false;
	}

	//! How much the point is above something static or terrain
	static Float StaticPointProjectionTest( class CWorld* world, const Vector& point, Float rayLength )
	{
		return -1.f;
	}

	//! How much the point is above or below something static or terrain
	static Bool StaticPointProjectionTestTwoWay( class CWorld* world, Vector& point, EulerAngles& hitNormal, Float rayHalfLength )
	{
		return false;
	}
};