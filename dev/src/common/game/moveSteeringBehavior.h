/**
 * Copyright © 2010 CD Projekt Red. All Rights Reserved.
 */
#pragma once

class CMovingAgentComponent;
struct SMoveLocomotionGoal;
class IMovementCommandBuffer;
class IMoveSteeringNode;


///////////////////////////////////////////////////////////////////////////////

// A listener that gets notified if a block gets activated etc.
class ISteeringBehaviorListener
{
public:
	virtual ~ISteeringBehaviorListener() {}

	virtual void OnFrameStart( const SMoveLocomotionGoal& goal ) = 0;

	virtual void OnNodeActivation( const IMoveSteeringNode& node ) = 0;

	virtual Uint32 AddFrame( const Char* txt ) = 0;

	virtual void AddText( Uint32 frameIdx, const Char* format, ... ) = 0;
};

///////////////////////////////////////////////////////////////////////////////

class CMoveSteeringBehavior : public CResource
{
	DECLARE_ENGINE_RESOURCE_CLASS( CMoveSteeringBehavior, CResource, "w2steer", "Steering" );

private:
	IMoveSteeringNode*						m_root;
	InstanceDataLayout						m_dataLayout;		//!< Layout of data in this tree
	
public:
	CMoveSteeringBehavior();

	//! CObject interaface
	virtual void OnPostLoad();
	
	void OnStructureModified();

	// Calculates the steering output for the specified agent
	// using the configured steering profile
	void CalculateMovement( IMovementCommandBuffer& comm, InstanceBuffer& data, Float timeDelta, ISteeringBehaviorListener* listener = NULL ) const;

	// Returns the root node of the behavior
	RED_INLINE IMoveSteeringNode* GetRootNode() { return m_root; }

	// Sets a new root node of the behavior
	RED_INLINE void SetRootNode( IMoveSteeringNode* node ) { m_root = node; MarkModified(); }

	// Called when steering behavior gets activated
	void Activate( CMovingAgentComponent* owner, InstanceBuffer* data );

	// Called when steering behavior gets deactivated
	void Deactivate( CMovingAgentComponent* owner, InstanceBuffer* data );

	// Creates a runtime data instance
	InstanceBuffer* CreateRealtimeDataInstance( CMovingAgentComponent* owner );

	// Releases the runtime data instance 
	void ReleaseRealtimeDataInstance( CMovingAgentComponent* owner, InstanceBuffer* data );

	// Generate editor rendering fragments
	void GenerateDebugFragments( const CMovingAgentComponent& agent, CRenderFrame* frame ) const;

private:
	void CompileDataLayout();
};
BEGIN_CLASS_RTTI( CMoveSteeringBehavior );
	PARENT_CLASS( CResource );
	PROPERTY( m_root );
END_CLASS_RTTI();

///////////////////////////////////////////////////////////////////////////////
