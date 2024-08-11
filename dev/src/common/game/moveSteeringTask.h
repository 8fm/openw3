/**
* Copyright © 2010 CD Projekt Red. All Rights Reserved.
*/
#pragma once


class IMovementCommandBuffer;

///////////////////////////////////////////////////////////////////////////////

class IMoveSteeringTask : public CObject
{
	DECLARE_ENGINE_ABSTRACT_CLASS( IMoveSteeringTask, CObject );

public:
	virtual ~IMoveSteeringTask() {}

	virtual void CalculateSteering( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta ) const;

	// Returns the name of the task
	virtual String GetTaskName() const;

	// Builds the runtime data layout for this task
	virtual void OnBuildDataLayout( InstanceDataLayoutCompiler& compiler );

	// Initializes the runtime data of this task
	virtual void OnInitData( CMovingAgentComponent& agent, InstanceBuffer& data );

	// Deinitializes the runtime data of this task
	virtual void OnDeinitData( CMovingAgentComponent& agent, InstanceBuffer& data );

	// Do some initialization when given steering behavior gets activated
	virtual void OnGraphActivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const;

	// Do some deinitialization when given steering behavior gets deactivated
	virtual void OnGraphDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const;

	// Do some deinitialization if your steering branch gets deactivated
	virtual void OnBranchDeactivation( CMovingAgentComponent& agent, InstanceBuffer& data ) const;

	// Returns the debug fragments
	virtual void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const;

};
BEGIN_ABSTRACT_CLASS_RTTI( IMoveSteeringTask );
	PARENT_CLASS( CObject );
END_CLASS_RTTI();

