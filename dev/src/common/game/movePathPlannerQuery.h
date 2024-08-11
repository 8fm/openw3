/**
 * Copyright © 2010 CDProjekt Red, Inc. All Rights Reserved.
 */
#pragma once

#include "movementObject.h"


enum ERelPathPosition : CEnum::TValueType
{
	RPP_Unknown,
	RPP_Front,
	RPP_Back
};

///////////////////////////////////////////////////////////////////////////////

// A helper structure describing a navigation goal
struct SNavigationGoal
{
	Bool						hasPosition;
	Vector						position;
	Float						radius;

	Bool						hasOrientation;
	Float						orientation;

	SNavigationGoal()
		: hasPosition( false )
		, hasOrientation( false )
		, position( 0.0f, 0.0f, 0.0f )
		, orientation( 0.0f ) 
	{
	}

	RED_INLINE void SetPosition( const Vector& pos, Float	rad )
	{
		hasPosition = true;
		position = pos;
		radius = rad;
	}

	RED_INLINE void SetOrientation( Float yaw )
	{
		hasOrientation = true;
		orientation = yaw;
	}

	RED_INLINE void Reset()
	{
		hasPosition = false;
		hasPosition = false;
	}
};

///////////////////////////////////////////////////////////////////////////////

// A common interface for path planner queries
class IMovePathPlannerQuery : public IMovementObject
{
public:
	virtual ~IMovePathPlannerQuery() {}

	// Returns the number of goals a query has
	virtual Uint32 GetGoalsCount() const = 0;

	// Returns the specified goal
	virtual const SNavigationGoal& GetGoal( Uint32 idx ) const = 0;

	// Checks where the 'checkedPos' is located on the queried path with respect
	// to the specified 'anchorPos'
	virtual ERelPathPosition GetRelativePosition( const Vector& anchorPos, const Vector& checkedPos ) const = 0;

	//! Casts the specified position onto the queried path.
	virtual Vector CastPosition( const Vector& anchorPos, const Vector& checkedPos ) const = 0;

	//! Debug draw
	virtual void DebugDraw( const Vector& currAgentPos, Uint32 currGoalIdx, IDebugFrame& debugFrame ) const = 0;

	//! Debug description
	virtual String GetDescription() const = 0;
};

///////////////////////////////////////////////////////////////////////////////
