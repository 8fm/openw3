#pragma once

#include "../../common/engine/multiCurve.h"

#include "../../common/game/moveSteeringCondition.h"
#include "../../common/game/moveSteeringTask.h"

#ifndef NO_EDITOR
#define DEBUG_ROAD_MOVEMENT
#endif

///////////////////////////////////////////////////////////////////////////////

class CMoveSCRoadMovement : public IMoveSteeringCondition
{
	DECLARE_ENGINE_CLASS( CMoveSCRoadMovement, IMoveSteeringCondition, 0 );


public:
	CMoveSCRoadMovement()														{}

	Bool Evaluate( const IMovementCommandBuffer& comm, InstanceBuffer& data ) const override;

	String GetConditionName() const override;
};
BEGIN_CLASS_RTTI( CMoveSCRoadMovement )
	PARENT_CLASS( IMoveSteeringCondition )
END_CLASS_RTTI()

///////////////////////////////////////////////////////////////////////////////

class CMoveSTOnRoad : public IMoveSteeringTask
{
	DECLARE_ENGINE_ABSTRACT_CLASS( CMoveSTOnRoad, IMoveSteeringTask );
protected:
	TInstanceVar< SMultiCurvePosition > i_actorCurvePosition;
	TInstanceVar< SMultiCurvePosition >	i_anticipatedCurvePosition;
	TInstanceVar< THandle< CNode > >	i_lastCurve;
	//TInstanceVar< Float >				i_randPositionOnRoad;

	TInstanceVar< Float >				i_cachedDirection;
	TInstanceVar< Uint32 >				i_cachedWaypoint;

	Float			m_headingImportance;
	Float			m_speedImportance;
	Float			m_anticipatedPositionDistance;

	Float			m_roadMaxDist;

	enum ERState
	{
		RSTATE_UNITIALIZED,
		RSTATE_OK,
		RSTATE_OFF,
		RSTATE_TOO_FAR,
		RSTATE_WALKING_OUT,
		RSTATE_NO_LINE_TO_ROAD,
		RSTATE_NO_LINE_TO_DEST,
		
	};
#ifdef DEBUG_ROAD_MOVEMENT
	TInstanceVar< Vector3 >				i_currentFollowPos;
	TInstanceVar< Uint8 >				i_currentState;
	RED_INLINE void SetRState( InstanceBuffer& data, ERState s ) const	{ data[ i_currentState ] = s; }
#else
	RED_INLINE void SetRState( InstanceBuffer& data, ERState s ) const	{}
#endif

public:
	CMoveSTOnRoad();

	// IMoveSteeringTask interface
	void	CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const override;
	String	GetTaskName() const override;

	void	OnBuildDataLayout( InstanceDataLayoutCompiler& compiler ) override;
	void	OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data ) override;

	// On graph activation/deactivation play with 'support roads' pathfollowing flag
	void	OnGraphActivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;
	void	OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const override;

#ifdef DEBUG_ROAD_MOVEMENT
	void	GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const override;
#endif

protected:
	Float	DetermineDirection( const SMultiCurve& curve, InstanceBuffer& data, const CPathAgent& pathAgent ) const;
};

BEGIN_CLASS_RTTI( CMoveSTOnRoad )
	PARENT_CLASS( IMoveSteeringTask )
	PROPERTY_EDIT( m_headingImportance, TXT( "Heading importance" ) )
	PROPERTY_EDIT( m_speedImportance, TXT("Speed importance") )
	PROPERTY_EDIT( m_anticipatedPositionDistance, TXT( "Distance at which the road is sampled to determine road heading" ) )
	PROPERTY_EDIT( m_roadMaxDist, TXT( "Maximum distance to road for this behavior to be active" ) )
END_CLASS_RTTI()
